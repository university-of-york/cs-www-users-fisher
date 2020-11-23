/* Modem for MIPS   AJF	  January 1995
   Main program */

#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <sys/fcntl.h>
#include <fishaudio.h>
#include "modem.h"

#define TOFN "/dev/ttyd2"

global audio *Audio;
global float sinetab[SINELEN];
global int contime, pagessent;
global ushort scanbits;		/* set by fax, used by senddoc */
global bool morepages;		/* set by senddoc, used by fax */
global uint options, bitrates;
global char *telno, *mercurypin;

static char *vmodes[] =		/* must agree with enum vmode in modem.h */
  { "-V21o", "-V21a", "-V23o", "-V23a", "-V22o", "-V22a", "-V32o", NULL,
  };

static audio_pval pvals[] =
  { { AU_INPUT_RATE, AU_RATE_24000   },
    { AU_OUTPUT_RATE, AU_RATE_24000  },
    { AU_INPUT_SOURCE, AU_INPUT_LINE },
    { AU_LEFT_INPUT_ATTEN, 0	     },
    { AU_RIGHT_INPUT_ATTEN, 0	     },
    { AU_SPEAKER_MUTE_CTL, 1	     },
    { AU_MONITOR_CTL, 0		     },
    { -1, -1			     },
  };

static vmode mode;

static void newhandler(), catchsignal(int), sighandler(int);
static bool legaltelno(char*), legalpin(char*);
static void setoptions(char*[]);
static uint bitrate(int), legalbps(vmode);
static void checkoptions(), usage(int);
static void makesinetab(), seizeline();

inline bool isdigit(char ch) { return (ch >= '0') && (ch <= '9'); }


global void main(int argc, char *argv[])
  { contime = -1; /* not connected yet */
    pagessent = 0;
    set_new_handler(newhandler);
    catchsignal(SIGINT); catchsignal(SIGTERM);
#ifdef ALARM
    catchsignal(SIGALRM);   /* safety alarm goes off after 10 mins */
#endif
    setoptions(argv);
    checkoptions();
    if ((options & opt_org) && !legaltelno(telno)) giveup("Illegal telephone number");
    if (options & opt_mod)
      { uint b = legalbps(mode);
	unless (options & opt_bps) bitrates = b;	/* default is the full set */
	if (bitrates & ~b) giveup("illegal mode-bitrate combination");
      }
    if (options & opt_m)
      { mercurypin = getpassword("Enter Mercury PIN: ");
	if (mercurypin == NULL) giveup("Error while reading PIN");
	unless (legalpin(mercurypin)) giveup("Illegal PIN");
      }
    if (options & opt_fax)
      { initdoc();
	unless (morepages) giveup("Empty document; not sent");
      }
    Audio = new audio(AU_IN | AU_OUT | AU_LOCK | AU_SAVE, pvals);
    makesinetab();
    seizeline();
    if (options & opt_org) dialnumber();
    if (options & (opt_fax | opt_mod))
      { if (options & opt_org) waitfortone(CONN_TONE); /* wait for answer (connect) tone */
	if (options & opt_ans) sendanswer();	       /* send answer tone */
	contime = time(NULL);
	infomsg("Connected");
	if (options & opt_fax) becomefax();
	if (options & opt_mod) becomemodem(mode);
	infomsg("Call duration: %d secs", time(NULL) - contime);
      }
    writelog("OK", "");
    delete Audio;
    exit(0);
  }

static void newhandler()
  { giveup("No room");
  }

static void catchsignal(int sig)
  { signal(sig, (SIG_PF) sighandler);
  }

static void sighandler(int sig)
  { giveup("Signal %d!", sig);
  }

static bool legaltelno(char *vec)
  { int n = 0;
    char *dstr = "0123456789*#ABCD";
    if (vec[n] == '+' || vec[n] == '=') n++;
    until (vec[n] == '\0' || strchr(dstr, vec[n]) == NULL) n++;
    return (n <= 16 && vec[n] == '\0');
  }

static bool legalpin(char *vec)
  { int n = 0;
    while (isdigit(vec[n])) n++;
    return (vec[n] == '\0' && (n == 10 || n == 12));
  }

static void setoptions(char *argv[])
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
	else
	  { int k = 0;
	    unless (s[k++] == '-') usage(2);
	    if (s[k] == 'V')
	      { if (options & opt_mod) usage(3);
		int m = 0;
		until (vmodes[m] == NULL || seq(vmodes[m], s)) m++;
		if (vmodes[m] == NULL) usage(4);
		mode = (vmode) m; options |= opt_mod;
	      }
	    else
	      { until (s[k] == '\0')
		  { char ch = s[k++];
		    if (ch == 'v') options |= opt_v;
		    else if (ch == 'm') options |= opt_m;
		    else if (ch == 'p') options |= opt_p;
		    else if (ch == '7') options |= opt_7;
		    else usage(5);
		  }
	      }
	  }
      }
  }

static uint bitrate(int n)
  { switch (n)
      { default:	giveup("unknown bit rate %d", n);
	case 1200:	return bps_1200;
	case 2400:	return bps_2400;
	case 4800:	return bps_4800;
	case 7200:	return bps_7200;
	case 9600:	return bps_9600;
	case 12000:	return bps_12000;
	case 14400:	return bps_14400;
      }
  }

static uint legalbps(vmode m)
  { switch (m)
      { default:	return 0;
	case V22o:	return bps_1200 | bps_2400;
	case V22a:	return bps_1200 | bps_2400;
	case V32o:	return bps_4800 | bps_7200 | bps_9600 | bps_12000 | bps_14400;
      }
  }

static void checkoptions()
  { unless (options & (opt_org | opt_ans)) usage(6);	/* need either org/ans	 */
    unless (~options & (opt_org | opt_ans)) usage(7);	/* but not both		 */
    unless (~options & (opt_fax | opt_mod)) usage(8);	/* mustn't have both fax and mod */
  }

static void usage(int n)
  { fprintf(stderr, "Modem V.5 from <fisher@minster.york.ac.uk>\n");
    fprintf(stderr, "Usage(%d): modem [-{vmp7}] {-<mode> | -fax} [-bps <bps> ...] [+<telnum> | -ans]\n", n);
    fprintf(stderr, "Modes:");
    for (int i=0; vmodes[i] != NULL; i++) fprintf(stderr, " %s", vmodes[i]);
    putc('\n', stderr);
    exit(2);
  }

static void makesinetab()
  { for (int k=0; k < SINELEN; k++)
      { float th = TWOPI * (float) k / (float) SINELEN;
	sinetab[k] = sin(th);
      }
  }

static void seizeline()
  { sleep(2); /* wait in case line has only just been dropped */
    int fd = open(TOFN, O_RDWR); /* this asserts DTR */
    if (fd < 0) giveup("Can't open %s", TOFN);
  }

