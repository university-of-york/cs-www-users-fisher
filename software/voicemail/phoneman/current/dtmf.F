/* phoneman -- telephone manager
   A.J. Fisher	 January 1996
   DTMF recognition routines */

#include <stdio.h>
#include <fishaudio.h>

#include "filters.h"
#include "phoneman.h"

#define TIMEOUT	    10			/* secs */
#define THRESHOLD   0.1
#define NUMFREQS    9

#define DIG_INVALID (-1)
#define DIG_NOTONE  (-2)
#define DIG_TIMEOUT (-3)

struct tone_detector
  { tone_detector(fspec*, int);
    ~tone_detector();
    bool fstep(float, bool);
    filter *bpf, *lpf;
    int max, count;
  };

static fspec *lpfs = mkfilter("-Lp -Bu -o 2 -a 0.0025");        /* 20 Hz lpf         */
static fspec *fefs = mkfilter("-Bp -Bu -o 4 -a 0.075 0.225");   /* 600 - 1800 Hz bpf */

static fspec *fspecs[NUMFREQS] =
  { mkfilter("-Bp -Re 50 -a 0.087125"),        /*  697 Hz */
    mkfilter("-Bp -Re 50 -a 0.096250"),        /*  770 Hz */
    mkfilter("-Bp -Re 50 -a 0.106500"),        /*  852 Hz */
    mkfilter("-Bp -Re 50 -a 0.117625"),        /*  941 Hz */
    mkfilter("-Bp -Re 50 -a 0.151125"),        /* 1209 Hz */
    mkfilter("-Bp -Re 50 -a 0.167000"),        /* 1336 Hz */
    mkfilter("-Bp -Re 50 -a 0.184625"),        /* 1477 Hz */
    mkfilter("-Bp -Re 50 -a 0.204125"),        /* 1633 Hz */
    mkfilter("-Bp -Re 50 -a 0.137500"),        /* 1100 Hz */
  };

static filter *fefilter;
static tone_detector *tdecs[NUMFREQS];
static schar digits[1 << NUMFREQS];
static tone *dt1, *dt2;
static int timenow;

static int getdigit(bool);
static void debugdigit(int);


global void initdtmf()	/* called at startup to initialize tables */
  { fefilter = new filter(fefs);
    dt1 = new tone(350.0); dt2 = new tone(450.0);   /* 2 components of dial tone */
    for (int i=0; i < NUMFREQS; i++)
      { int max = (i == NUMFREQS-1) ? SAMPLERATE/4 : 1;	    /* require at least 1/4 sec of CNG */
	tdecs[i] = new tone_detector(fspecs[i], max);
      }
    digits[0] = DIG_NOTONE;
    for (int i=1; i < (1 << NUMFREQS); i++) digits[i] = DIG_INVALID;
    for (int i=0; i < 17; i++)
      { static ushort dt[17] =
	  { 0x028, 0x011, 0x021, 0x041, 0x012, 0x022, 0x042, 0x014,	/* 01234567 */
	    0x024, 0x044, 0x018, 0x048, 0x081, 0x082, 0x083, 0x084,	/* 89*#ABCD */
	    0x100,							/* fax CNG  */
	  };
	digits[dt[i]] = i;
      }
  }

global void tidydtmf()
  { delete dt1; delete dt2; delete fefilter;
    for (int i=0; i < NUMFREQS; i++) delete tdecs[i];
  }

global int getdtmf(bool first)
  { int ifill = Audio -> control(AU_SETIFILL, 1);	/* needed to avoid blocking while doing simultaneous I/O */
    if (first)
      { /* put dial tone in output buffer */
	for (int i=0; i < TONELEN; i++)
	  { int x = mu_expand(dt1 -> vec[i]) + mu_expand(dt2 -> vec[i]);
	    Audio -> write(x << 7); /* 200 ms */
	  }
      }
    timenow = 0;
    // putc('[', stderr);
    int d = getdigit(first);
    until (d == DIG_NOTONE || d == DIG_TIMEOUT) d = getdigit(first);	/* wait for silence or timeout */
    until (d >= 0 || d == DIG_TIMEOUT) d = getdigit(first);		/* wait for valid tone, or timeout */
    // debugdigit(d);
    // putc(']', stderr);
    Audio -> control(AU_SETIFILL, ifill);	/* restore */
    return d;
  }

static int getdigit(bool first)
  { if (first)
      { int i = timenow % TONELEN;
	int x = mu_expand(dt1 -> vec[i]) + mu_expand(dt2 -> vec[i]);
	Audio -> write(x << 7);
      }
    float x = (float) Audio -> read() * 2e-6f;	/* scale to avoid overflow */
    x = fefilter -> fstep(x);			/* broad bandpass filter to remove dc offset */
    x = sgn(x);					/* hard limiting */
    if (timenow++ >= TIMEOUT*SAMPLERATE) return DIG_TIMEOUT;
    ushort result = 0;
    for (int j = NUMFREQS-1; j >= 0; j--)
      { bool p = tdecs[j] -> fstep(x, false);	/* is tone present? */
	result = (result << 1) | p;
      }
    unless (first) result &= ~0x100;	/* recognize fax CNG tone on first digit only */
    return digits[result];		/* return 0-15, or 16 if fax tone */
  }

static void debugdigit(int d)
  { if (d < -3 || d > 16) giveup("Bug: ilgl digit %d", d);
    static char *debugstr = "T.?0123456789*#ABCDf";
    putc(debugstr[d+3], stderr);
  }

tone_detector::tone_detector(fspec *bfs, int mx)
  { bpf = new filter(bfs);
    lpf = new filter(lpfs);
    max = mx; count = 0;
  }

tone_detector::~tone_detector()
  { delete bpf;
    delete lpf;
  }

bool tone_detector::fstep(float x, bool dbg)
  { float f = bpf -> fstep(x);
    float p = lpf -> fstep(f*f);
    if (dbg && (timenow % (SAMPLERATE/10) == 0)) fprintf(stderr, "%g\n", p);
    if (p > THRESHOLD) count++; else count = 0;
    return (count >= max);
  }

