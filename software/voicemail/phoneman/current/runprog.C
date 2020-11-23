/* phoneman -- telephone manager
   A.J. Fisher	 January 1996 */

#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>

#include "phoneman.h"

#define MAXARGS	 10
#define CONNECT	 "/usr/fisher/mipsbin/connect"

static int forkconnect(char*, char*, int);
static int forkmodem(char*, int, int), xfork();
static void execcmd(char*);

global bool runprog(char *cmd, char *fn, bool con)
  { SIG_PF osigi = signal(SIGINT, SIG_IGN);	/* ignore interrupts (Ctl-C) */
    SIG_PF osigq = signal(SIGQUIT, SIG_IGN);	/* ignore quits (Ctl-\)	     */
    int lpid, mpid, lstat, mstat;
    if (con)
      { int ptyfd;
	char *ptydev = _getpty(&ptyfd, O_RDWR, 0600, false);  /* make a pseudo-terminal */
	if (ptydev == NULL) giveup("no free ptys");
	lpid = forkconnect(CONNECT, ptydev, ptyfd);
	/* wait for "ok" msg from child */
	bool ok; int nb = read(ptyfd, &ok, sizeof(bool));
	unless (nb == sizeof(bool) && ok) giveup("bug: connect child terminated");
	mpid = forkmodem(cmd, ptyfd, ptyfd);
	close(ptyfd);
      }
    else
      { lpid = 0;
	if (fn != NULL)
	  { fprintf(stderr, "phoneman: creating new %s\n", fn);
	    int fd = open(fn, O_WRONLY | O_CREAT, 0666);    /* create or open and truncate */
	    if (fd < 0) giveup("can't create new msg file %s", fn);
	    mpid = forkmodem(cmd, -1, fd);
	    close(fd);
	  }
	else mpid = forkmodem(cmd, -1, -1);
      }
    /* wait for children */
    until (lpid == 0 && mpid == 0)
      { int stat, pid;
	do pid = wait(&stat); while (pid < 0 && errno == EINTR);
	if (pid == lpid) { lstat = stat; lpid = 0; }
	if (pid == mpid) { mstat = stat; mpid = 0; }
      }
    // fprintf(stderr, "??? wait done: lstat=%d mstat=%d\n", lstat & 0xffff, mstat & 0xffff);
    signal(SIGINT, osigi); signal(SIGQUIT, osigq);	/* restore interrupt handlers */
    mstat &= 0xffff;
    if (mstat != 0) fprintf(stderr, "phoneman: modem terminated with status %d\n", mstat);
    return (mstat == 0);
  }

static int forkconnect(char *cmd, char *ptydev, int ptyfd)
  { /* fork & exec "connect" prog */
    int cpid = xfork();
    if (cpid == 0)
      { /* child */
	close(ptyfd);
	setsid();	/* assign controlling tty */
	nice(10);	/* try to give modem process priority */
	/* redirect standard input, output, error to/from pty */
	for (int k=0; k<3; k++) { close(k); open(ptydev, O_RDWR); }
	execcmd(cmd);	/* doesn't return */
      }
    return cpid;
  }

static int forkmodem(char *cmd, int fd0, int fd1)
  { /* fork & exec modem */
    int cpid = xfork();
    if (cpid == 0)
      { /* child */
	if (fd0 >= 0) dup2(fd0, 0);	/* redirect stdin  */
	if (fd1 >= 0) dup2(fd1, 1);	/* redirect stdout */
	execcmd(cmd);			/* doesn't return */
      }
    return cpid;
  }

static int xfork()
  { int cpid = fork();
    if (cpid < 0) giveup("fork failed");
    return cpid;
  }

static void execcmd(char *str)
  { char cmd[MAXSTR+1]; strcpy(cmd, str);
    char *argv[MAXARGS+1];
    int j = 0, ap = 0;
    until (cmd[j] == '\0')	/* tokenize */
      { argv[ap++] = &cmd[j];
	until (cmd[j] == ' ' || cmd[j] == '\0') j++;
	if (cmd[j] == ' ') cmd[j++] = '\0';
	while (cmd[j] == ' ') j++;
      }
    if (ap > 0)
      { argv[ap++] = NULL;
	execvp(argv[0], argv);
      }
    _exit(255); /* if exec failed; this will cause parent to see EOF */
  }

