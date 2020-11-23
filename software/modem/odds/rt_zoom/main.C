/* Modem for MIPS   AJF	  January 1995
   Main program */

#include <stdio.h>
#include <signal.h>
#include <string.h>

#include <complex.h>
#include <myaudio.h>

#include "modem.h"

static uint bitrates;

static void newhandler(), catchsignal(int), sighandler(int);
static void setoptions(char**);
static int getiarg(char*);
static void usage();
static void becomemodem();


global void main(int argc, char **argv)
  { set_new_handler(newhandler);
    catchsignal(SIGINT); catchsignal(SIGTERM); catchsignal(SIGUSR1);
    setoptions(argv);
    openaudio();
    atexit(closeaudio);
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
	    giveup("Signal %d!", sig);

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
  { fprintf(stderr, "V.34 modem from <fisher@minster.york.ac.uk>\n");
    fprintf(stderr, "Usage: zmodem [-bps <mask>]\n");
    exit(2);
  }

static void becomemodem()
  { infoseqs();
  }

