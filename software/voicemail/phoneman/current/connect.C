/* phoneman -- telephone manager
   Connect program
   A.J. Fisher	 April 1998 */

#include <stdio.h>
#include <sgtty.h>
#include <signal.h>

#define global
#define unless(x)   if(!(x))
#define until(x)    while(!(x))

#define CASEMAP	    false

extern "C"
  { int write(int, void*, int), putenv(char*);
    void alarm(int), close(int);
    void execl(char*, ...);
  };

static void sighandler();


global void main(int argc, char **argv)
  { signal(SIGINT, SIG_IGN); signal(SIGQUIT, SIG_IGN);
    signal(SIGALRM, (SIG_PF) sighandler);
    alarm(60);						/* 1 min timeout */
    termios tmode;
    tcgetattr(0, &tmode);				/* get pty attrs for "standard" mode */
    tmode.c_lflag = 05073;				/* "sane" values */
    termios rmode = tmode;
    rmode.c_iflag = rmode.c_oflag = rmode.c_lflag = 0;	/* modify for raw mode	*/
    rmode.c_cc[VMIN] = rmode.c_cc[VTIME] = 1;		/* set timeout params	*/
    tcsetattr(0, TCSADRAIN, &rmode);			/* set raw mode		*/
    bool ok = true; write(1, &ok, sizeof(bool));	/* send "ok" to parent  */
    int ch1, ch2 = 0;
    do { ch1 = ch2; ch2 = getchar(); }
    until ((ch1 == '\r' && ch2 == '\r') || (ch2 < 0));	/* wait for 2 cr's from modem, or alarm */
    if (ch2 < 0)
      { /* alarm went off */
	tcsetattr(0, TCSADRAIN, &tmode);		/* reset sane mode */
	exit(1);
      }
    printf("Tony Fisher's Indy\r\n");
    printf("casemap: %s\r\n", CASEMAP ? "yes" : "no");
    if (CASEMAP)
      { /* map uc/lc : serviceable but not perfect */
	tmode.c_lflag |= XCASE; tmode.c_iflag |= IUCLC; tmode.c_oflag |= OLCUC;
      }
    fprintf(stderr, "??? lflag was %06o ", tmode.c_lflag);
    tmode.c_lflag &= ~ISIG;				/* disable signals */
    fprintf(stderr, "is %06o\r\n", tmode.c_lflag);
    tcsetattr(0, TCSADRAIN, &tmode);			/* set modified mode */
    alarm(0);						/* reset alarm timer */
    putenv("SHELL=/dev/null");                          /* anti-hacking precaution */
    execl("/usr/bsd/telnet", "telnet", "-e", "\020", NULL);
    printf("Can't exec telnet!\r\n");
    exit(1);
  }

static void sighandler()
  { close(0);	/* sneaky trick to make getchar() return -1 from now on */
  }

