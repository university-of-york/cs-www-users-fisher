/* Modem for MIPS   AJF	  January 1995
   Modem routines */

#include <stdio.h>
#include <signal.h>
#include <spawn.h>
#include "modem.h"

#define NOCHAR (-3)

static void rxloop(vmode), txloop(vmode);
static int nextchar();


global void becomemodem(vmode mode)
  { init_ttymodes();		/* init tty modes package */
    setraw();			/* set raw tty mode	  */
    setbuf(stdout, NULL);	/* unbuffered stdout	  */
    if (mode == V22o || mode == V22a || mode == V32o)
      { /* exec fmodem program to do all the work */
	SIG_PF osig = signal(SIGINT, SIG_IGN);	/* ignore interrupt (Ctl-C) */
	FILE *fi; char vec[256]; bool ferrs = false;
	spawn("/usr/fisher/mipslib/fmodem", "fmodem -log %f -bps %d", &fi, bitrates);
	char *s = fgets(vec, 256, fi);
	until (s == NULL)	/* wait until child exits */
	  { int len = strlen(s);
	    while (len > 0 && s[len-1] == '\n') len--;
	    s[len] = '\0';
	    if (len > 0)
	      { writelog("ER", "fmodem: %s", s);
		ferrs = true;
	      }
	    s = fgets(vec, 256, fi);
	  }
	signal(SIGINT, osig);	/* restore interrupt handler */
	if (ferrs) exit(1);	/* fmodem has already written a msg to stderr */
      }
    else
      { int cpid = fork();
	if (cpid < 0) giveup("fork failed");
	if (cpid == 0) rxloop(mode);		/* child: loop until killed by parent */
	else
	  { txloop(mode);
	    kill(cpid, SIGKILL);		/* EOF from terminal: kill child */
	  }
      }
    setcooked();				/* reset cooked mode		 */
  }

static void rxloop(vmode mode)
  { /* forked as a child */
    initrx(mode);
    for (;;)			/* until killed by parent */
      { int ch = getaval();	/* from 'phone line */
	putchar(ch);		/* to stdout */
      }
  }

static void txloop(vmode mode)
  { inittx(mode);
    bool ok = true;
    while (ok)
      { int ch = nextchar();	/* from stdin */
	switch (ch)
	  { default:
	      { putaval(ch);	/* to line */
		static char prev[3] = { 0, 0, 0 };
		prev[0] = prev[1]; prev[1] = ch;
		if (seq(prev,"\033\032")) ok = false;  /* termination sequence is ESC CTLZ */
		break;
	      }

	    case NOCHAR:
		putbit(1);	/* send mark while idle */
		break;

	    case EOF:
		ok = false;
		break;
	  }
      }
  }

static int nextchar()
  { int ch;
    if (stdin -> _cnt > 0) ch = getchar();
    else
      { uint ib = 1; /* test stdin */
	int tmo[2] = { 0, 0 }; /* immediate return */
	int nb = select(32, &ib, NULL, NULL, tmo);
	ch = (nb > 0) ? getchar() : NOCHAR;
      }
    return ch;
  }

