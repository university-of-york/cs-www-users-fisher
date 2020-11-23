#include <stdio.h>	// ???
#include <math.h>
#include <coro.h>

#include <complex.h>
#include <filters.h>
#include <phrdec.h>
#include <debug.h>
#include <myaudio.h>
#include <mystdio.h>
#include <sinegen.h>
#include <goertzel.h>

#include "modem.h"

#define SECS(f) ((int) (f * SAMPLERATE + 0.5))

#define SYMBLEN (SAMPLERATE/600)    /* 600 bps */
#define DELAY	SECS(0.04)	    /* 40 ms */

struct Bitsample
  { Bitsample(float fx, float tx) { f = fx; t = tx; }
    float f;	/* phase output from phase reversal detector */
    float t;	/* timing resonator output */
  };

static fspec *fefs = mkfilter("-Bu -Bp -o 4 -a 0.21875 0.28125");   /* 2100 .. 2700 Hz bandpass     */
static fspec *bpfs = mkfilter("-Re 500 -Bp -a 0.25");               /* 2400 Hz resonator            */   // was Q=100
static fspec *trfs = mkfilter("-Re 100 -Bp -a 0.0625");             /* 600 Hz resonator             */
static fspec *lpfs1 = mkfilter("-Av 8");                            /* removes harmonics of 1200 Hz */
static fspec *lpfs2 = mkfilter("-Bu -Lp -o 4 -a 0.03125");          /* 300  Hz lowpass              */

static phr_detector *pdec;
static filter *fef, *lpf, *tres;
static debugger *rdbg;
static int mstate;
static sinegen *carrier;

static void rxside(), tidydebug(), getranging(), getinfo(), getreversal();
static int gbit();
static Bitsample nextsample();
static void getprobing();
static void txside();
static void pbit(int), outsymbol(float);


global void infoseqs()
  { my_alarm(10);		    /* 10 secs timeout */
    setduplex(DELAY - 2*SYMBLEN);   /* 40 ms minus RC filter delay */
    mstate = 0;
    coroutine *rx = new coroutine(rxside);
    coroutine *tx = new coroutine(txside);
    inparallel(rx, tx);
    my_alarm(0);		    /* cancel alarm */
  }

static void rxside()
  { rdbg = new debugger(3, 4000);
    atexit(tidydebug);
    getranging();
    getprobing();
    callco(mainco);
  }

static void tidydebug()
  { rdbg -> print("debug_rg.grap");
  }

static void getranging()
  { pdec = new phr_detector(bpfs, lpfs1);
    lpf = new filter(lpfs2);
    fef = new filter(fefs);
    tres = new filter(trfs);
    int t1;
    getinfo();
    mstate++;					/* 0 to 1 */
    bool got = false;
    until (got)
      { getinfo();
	int t = samplecount;
	getreversal();
	t1 = samplecount;
	if (after(t1, t+460)) got = true;	/* got a genuine phase reversal */
      }
    mstate++;					/* 1 to 2 */
    getreversal();
    int nt = samplecount - t1;
    float ms = (float) (nt - 2*DELAY) / (float) SAMPLERATE * 1000.0f;
    char nts[32]; sprintf(nts, "%.1f", ms); infomsg("NT=%d; actual RTD = %sms", nt, nts);
    delete pdec; delete lpf; delete fef; delete tres;
  }

static void getinfo()
  { uchar bits = 0xff;
    until (bits == 0x72) bits = (bits << 1) | gbit();	/* look for sync byte */
    for (int i=0; i<37; i++)
      { int b = gbit();
	putc('0' + b, stderr);
	if (i%8 == 7) putc(' ', stderr);
      }
    putc('\r', stderr); putc('\n', stderr);
  }

static void getreversal()
  { while (pdec -> reversal()) nextsample();	/* wait for phase to stabilize */
    until (pdec -> reversal()) nextsample();	/* wait for phase change */
  }

static int gbit()
  { static uchar bits = 0;
    Bitsample z = nextsample();
    while (z.t > 0.0) z = nextsample();
    while (z.t <= 0.0) z = nextsample();
    for (int i = 0; i < SYMBLEN/4; i++) z = nextsample();
    rdbg -> tick('X');
    bits = (bits << 1) | (z.f > 0.0);
    uchar tab[] = { 0, 1, 1, 0 };
    return tab[bits & 3];   /* diff. decode */
  }

static Bitsample nextsample()
  { float x = insample();
    x = fef -> fstep(x);
    pdec -> insert(x);
    float fz = lpf -> fstep(pdec -> phase);
    float tz = tres -> fstep(fz*fz);
    rdbg -> insert(x, fz, tz);
    return Bitsample(fz, tz);
  }

inline float hypot(complex z) { return hypot(z.im, z.re); }
inline float atan2(complex z) { return atan2(z.im, z.re); }

static void getprobing()
  { for (int i = 0; i < SECS(0.18); i++) insample();	/* discard 10 ms A + 160 ms L1 + 10 ms */
    goertzel **gvec = new goertzel*[25];
    for (int i = 0; i < 25; i++) gvec[i] = new goertzel((i+1)*150.0);
    for (int i = 0; i < 6000; i++)
      { float x = insample();
	for (int j = 0; j < 25; j++) gvec[j] -> insert(x);
      }
    uint pm = 0x07a4402;	/* says which cosines are inverted */
    for (int i = 0; i < 25; i++)
      { complex z = gvec[i] -> result();
	float mag = hypot(z) / 6000.0, ph = atan2(z) / M_PI;
	if (pm & 1) ph += 1.0;
	while (ph < -1.0) ph += 2.0;
	while (ph >= 1.0) ph -= 2.0;
	printf("%4d   mag=%6.3f   ph=%6.3f PI\r\n", (i+1)*150, mag, ph);
	pm >>= 1;
	delete gvec[i];
      }
    delete gvec;
  }

static uchar info0c[] = { 0xf7, 0x2f, 0xf8, 0x00, 0x76, 0x8f, 0x80 };

static float shapetab[2*SYMBLEN+1] =
  { +1.0000000000, +0.9926801157, +0.9709416672, +0.9354394838,
    +0.8872360730, +0.8277597175, +0.7587485480, +0.6821831650,
    +0.6002108802, +0.5150649954, +0.4289827154, +0.3441252800,
    +0.2625037258, +0.1859133401, +0.1158793838, +0.0536160576,
    -0.0000000000, -0.0444411170, -0.0795250663, -0.1053974855,
    -0.1225017354, -0.1315382297, -0.1334156708, -0.1291968976,
    -0.1200421673, -0.1071526632, -0.0917168476, -0.0748619803,
    -0.0576127231, -0.0408582688, -0.0253289076, -0.0115824006,
    +0.0000000000,
  };

static void txside()
  { carrier = new sinegen(1200.0);
    pbit(0);	/* the "point of arbitrary phase" */
    while (mstate < 1)
      { int p = 0; uchar w;
	for (int i=0; i<45; i++)
	  { if (i%8 == 0) w = info0c[p++];
	    pbit(w >> 7);
	    w <<= 1;
	  }
      }
    while (mstate < 2) pbit(0);
    pbit(1);	/* phase reversal */
    for (int i=0; i<38; i++) pbit(0);
    for (;;) outsymbol(0.0);	/* silence */
  }

static void pbit(int b)
  { static float x = 1.0;
    if (b) x = -x;  /* diff. encode */
    outsymbol(x);
  }

static void outsymbol(float x)
  { static float a0 = 0.0, a1 = 0.0, a2 = 0.0, a3 = 0.0;
    a0 = a1; a1 = a2; a2 = a3; a3 = x;
    for (int k = 0; k < SYMBLEN; k++)
      { /* baseband pulse shaping */
	float s = shapetab[SYMBLEN + k]	  * a0
		+ shapetab[k]		  * a1
		+ shapetab[SYMBLEN - k]	  * a2
		+ shapetab[2*SYMBLEN - k] * a3;
	/* modulate onto carrier */
	float cx = carrier -> fnext();
	outsample(s*cx);
      }
  }

