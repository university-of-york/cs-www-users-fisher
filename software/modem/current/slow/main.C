/* Modem for MIPS   AJF	  January 1995
   Main program */

#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <sys/fcntl.h>

#include <complex.h>
#include <sinegen.h>
#include <myaudio.h>
#include <bitrates.h>

#include "modem.h"

#define TOFN "/dev/ttyd2"

global int numpages;
global uint options, bitrates;
global vmode veemode;

struct vminfo
  { char *s;
    vmode v;
  };

static vminfo vmodes[] =
  { { "-V21o", V21o }, { "-V21a", V21a }, { "-V23o", V23o }, { "-V23a", V23a },
    { "-V32o", V32o }, { "-V34o", V34o },
    { NULL, (vmode) 0 },	/* terminator */
  };

static char *telno;

static void newhandler(), catchsignal(int), sighandler(int);
static bool legaltelno(char*);
static void setoptions(char**);
static uint bitrate(int), legalbps(vmode);
static void checkoptions(), usage(int);
static void seizeline(), sendanswer();

inline bool isdigit(char ch) { return (ch >= '0') && (ch <= '9'); }


global void main(int argc, char **argv)
  { set_new_handler(newhandler);
    catchsignal(SIGINT); catchsignal(SIGTERM);
    catchsignal(SIGUSR1); catchsignal(SIGABRT);
    setoptions(argv);
    checkoptions();
    if ((options & opt_org) && !legaltelno(telno)) giveup("Illegal telephone number");
    if (options & opt_mod)
      { uint b = legalbps(veemode);
	unless (options & opt_bps) bitrates = b;	/* default is the full set */
	if (bitrates & ~b) giveup("illegal mode-bitrate combination");
      }
    if (options & opt_fax)
      { initdoc();
	if (options & opt_org) readdoc();		/* read whole doc before time-critical part */
      }
    openaudio(); atexit(closeaudio);
    seizeline();
    if (options & opt_org) dialnumber(telno);
    if (options & (opt_fax | opt_mod))
      { if (options & opt_org) waitfortone(CONN_TONE);	/* wait for answer (connect) tone */
	if (options & opt_ans) sendanswer();		/* send answer tone */
	int contime = time(NULL);
	infomsg("Connected");
	if (options & opt_v8) startsession();		/* V.8 procedures */
	if (options & opt_fax) becomefax();
	if (options & opt_mod) becomemodem();
	infomsg("Call duration: %d secs", time(NULL) - contime);
      }
    if ((options & opt_fax) && !(options & opt_org)) writedoc();  /* write whole doc after time-critical part */
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

static bool legaltelno(char *vec)
  { int n = 0;
    char *dstr = "0123456789*#ABCD";
    if (vec[n] == '+' || vec[n] == '=') n++;
    until (vec[n] == '\0' || strchr(dstr, vec[n]) == NULL) n++;
    return (n <= 20 && vec[n] == '\0');
  }

static void setoptions(char **argv)
  { int ap = 0;
    unless (argv[ap] == NULL) ap++;
    options = bitrates = 0;
    until (argv[ap] == NULL)
      { char *s = argv[ap++];
	if (s[0] == '+' || s[0] == '=')
	  { if (options & opt_org) usage(1);
	    telno = s; options |= opt_org;
	  }
	else if (seq(s, "-ans")) options |= opt_ans;
	else if (seq(s, "-fax")) options |= opt_fax;
	else if (seq(s, "-bps"))
	  { while (argv[ap] != NULL && isdigit(argv[ap][0]))
	      { int bps = atoi(argv[ap++]);
		bitrates |= bitrate(bps);
	      }
	    options |= opt_bps;
	  }
	else if (seq(s,"-V8")) options |= opt_v8;
	else
	  { int k = 0;
	    unless (s[k++] == '-') usage(2);
	    if (s[k] == 'V')
	      { if (options & opt_mod) usage(3);
		int m = 0;
		until (vmodes[m].s == NULL || seq(vmodes[m].s, s)) m++;
		if (vmodes[m].s == NULL) usage(4);
		veemode = vmodes[m].v;
		options |= opt_mod;
		if (veemode == V34o) options |= opt_v8;	    /* V.34 requires V.8 */
	      }
	    else
	      { until (s[k] == '\0')
		  { char ch = s[k++];
		    if (ch == 'v') options |= opt_v;
		    else usage(5);
		  }
	      }
	  }
      }
  }

static uint bitrate(int n)
  { /* see bitrates.h */
    unless (n >= 2400 && n <= 28800 && n%2400 == 0) giveup("unknown bit rate %d", n);
    return 1 << (n/2400 - 1);
  }

static uint legalbps(vmode m)
  { switch (m)
      { default:
	    return 0;

	case V32o:
	    return bps_4800 | bps_7200 | bps_9600 | bps_12000 | bps_14400;

	case V34o:
	    return bps_2400 | bps_4800 | bps_7200 | bps_9600 | bps_12000 | bps_14400 |
		   bps_16800 | bps_19200 | bps_21600 | bps_24000 | bps_26400 | bps_28800;
      }
  }

static void checkoptions()
  { unless (options & (opt_org | opt_ans)) usage(6);		/* need either org/ans	 */
    unless (~options & (opt_org | opt_ans)) usage(7);		/* but not both		 */
    unless (~options & (opt_fax | opt_mod)) usage(8);		/* mustn't have both fax and mod */
  }

static void usage(int n)
  { fprintf(stderr, "Modem Vsn. 5.1 from <fisher@minster.york.ac.uk>\n");
    fprintf(stderr, "Usage(%d): modem [-v] [-V8] {-<mode> | -fax} [-bps <bps> ...] [+<telnum> | -ans]\n", n);
    fprintf(stderr, "Modes:");
    for (int i = 0; vmodes[i].s != NULL; i++) fprintf(stderr, " %s", vmodes[i].s);
    putc('\n', stderr);
    exit(2);
  }

static void seizeline()
  { sleep(2); /* wait in case line has only just been dropped */
    int fd = open(TOFN, O_RDWR); /* this asserts DTR */
    if (fd < 0) giveup("Can't open %s", TOFN);
  }

static void sendanswer()			/* send V.25 answer sequence   */
  { sendpause(2.15);				/* silence for 2.15 sec	       */
    sinegen *sgen = new sinegen(2100.0);
    for (int i = 0; i < 7; i++)
      { int ns = (int) (0.45 * SAMPLERATE);	/* 2100 Hz for 450 ms	       */
	for (int j = 0; j < ns; j++)
	  { float val = sgen -> fnext();
	    outsample(val);
	  }
	sgen -> flipphase();			/* flip phase for next segment */
      }
    delete sgen;
    sendpause(0.075);				/* silence for 75 ms	       */
  }

