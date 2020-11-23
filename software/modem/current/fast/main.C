/* Modem for MIPS   AJF	  January 1995
   Main program */

#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <coro.h>

#include <myaudio.h>
#include <complex.h>
#include <mystdio.h>
#include <bitrates.h>

#include "modem.h"
#include "cancel.h"

global int mstate;
global ushort rateword;
global canceller *can;	/* canceller is accessed by both v32tx and v32rx */

static uint bitrates;

static void newhandler(), catchsignal(int), sighandler(int);
static void setoptions(char**);
static int getiarg(char*);
static void usage();
static void setrateword(), becomemodem(), rxloop(), txloop();


global void main(int argc, char **argv)
  { set_new_handler(newhandler);
    catchsignal(SIGINT); catchsignal(SIGTERM);
    catchsignal(SIGUSR1); catchsignal(SIGABRT);
    setoptions(argv);
    setrateword();	/* set rateword from bitrates */
    openstdio(); atexit(closestdio);
    openaudio(); atexit(closeaudio);
    becomemodem();
    exit(0);
  }

static void newhandler()
  { giveup("No room");
  }

static void catchsignal(int sig)
  { signal(sig, (SIG_PF) sighandler);
  }

static void sighandler(int sig)
  { switch (sig)
      { default:
	    giveup("Killed by signal %d.", sig);

	case SIGUSR1:
	    giveup("Remote modem is not responding.");
      }
  }

inline bool isdigit(char ch)
  { return (ch >= '0') && (ch <= '9');
  }

static void setoptions(char **argv)
  { int ap = 0;
    unless (argv[ap] == NULL) ap++;
    bitrates = 0;
    until (argv[ap] == NULL)
      { char *s = argv[ap++];
	if (seq(s, "-bps")) bitrates |= getiarg(argv[ap++]);
	else usage();
      }
  }

static int getiarg(char *s)
  { if (s == NULL || !isdigit(s[0])) usage();
    return atoi(s);
  }

static void usage()
  { fprintf(stderr, "V.32 bis modem from <fisher@minster.york.ac.uk>\n");
    fprintf(stderr, "Usage: fmodem [-bps <mask>]\n");
    exit(2);
  }

static void setrateword()
  { /* set rateword from bitrates */
    rateword = RWORD;
    if (bitrates & bps_4800) rateword |= rb_4800;
    if (bitrates & bps_7200) rateword |= rb_7200;
    if (bitrates & bps_9600) rateword |= rb_9600;	/* trellis coding */
    if (bitrates & bps_12000) rateword |= rb_12000;
    if (bitrates & bps_14400) rateword |= rb_14400;
  }

static void becomemodem()
  { can = new canceller(0.01);
    mstate = 0;
    coroutine *rx = new coroutine(rxloop);
    coroutine *tx = new coroutine(txloop);
    inparallel(rx, tx);
    can -> print("debug_cancos.grap");
    delete can;
  }

static void rxloop()
  { initrx();			    /* initialize and handshake */
    for (;;)
      { int ch = getasync();	    /* get char from 'phone line */
	my_putchar(ch);		    /* to stdout */
      }
  }

static void txloop()
  { inittx();			    /* initialize and handshake */
    int ch = my_getchar();	    /* from stdin */
    until (ch == EOF)
      { putasync(ch);		    /* to 'phone line */
	ch = my_getchar();
      }
    callco(currentco -> creator);   /* terminate */
  }

