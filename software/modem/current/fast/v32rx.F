#include <stdio.h>	/* sprintf */
#include <coro.h>

#include <complex.h>
#include <filters.h>
#include <scramble.h>
#include <equalize.h>
#include <debug.h>
#include <sinegen.h>
#include <myaudio.h>
#include <mystdio.h>

#include "modem.h"
#include "cancel.h"
#include "coder.h"

#define THRESHOLD 1.0f	/* sqr of radius of error circle */

static fspec *lpf_fs = mkfilter("-Bu -Lp -o 4 -a 0.125");   /* low-pass at 1200 Hz */

static char *rate_strs[] = { "R1", "R2", "R3", "E3" };      /* indexed by mstate */

static sinegen *carrier;
static coroutine *rx1_co, *rx2_co;
static cfilter *fe_lpf;
static equalizer *eqz;
static scrambler *gpa;
static decoder *dec;
static traininggen *trn;
static co_debugger *co_debug;
static debugger *can_debug, *dt_debug, *acq_debug;
static int timing, nextadj;
static char ticker;

static void tidyup();
static void getratesignals();
static ushort getrate(), getrwd();
static void reportrate(ushort);

static void rx2_loop(), roundtrip(), rcvdata();
static void wt_tone(int, int, int, bool);
static int wt_reversal(int, int);
static complex getsymbol();
static void adjtiming();
static void traincanceller();
static complex gethalfsymb();

inline int gbit()	{ return callco(rx2_co); }
inline void pbit(int b) { callco(rx1_co, b);	 }


global void initrx()
  { my_alarm(15);  /* 15 sec timeout */
    carrier = new sinegen(1800.0);
    fe_lpf = new cfilter(lpf_fs);
    eqz = new equalizer(0.25);
    rx1_co = currentco;
    rx2_co = new coroutine(rx2_loop);
    gpa = new scrambler(GPA);
    trn = new traininggen(gpa);
    dec = new decoder;
    // dec -> printtrellis("debugt1.txt");
    co_debug = new co_debugger(24000);
    can_debug = new debugger(1, 24000);
    dt_debug = new debugger(1, 4000);
    acq_debug = new debugger(2, 24000);
    atexit(tidyup);
    getratesignals();
    dec -> setrate(rateword);		    /* tell decoder what bit rate to use */
    for (int i = 0; i < 128; i++) gbit();   /* discard 128 "1" bits (wait for trellis decoder to settle) */
    my_alarm(0);   /* cancel alarm */
  }

static void tidyup()
  { eqz -> print("debug_eqz.grap");
    dec -> printtrellis("debugt2.txt");
    co_debug -> print("debug_co.grap");
    can_debug -> print("debug_can.grap");
    dt_debug -> print("debug_dt.grap");
    acq_debug -> print("debug_acq.grap");
  }

static void getratesignals()
  { ushort wd;
    for (int i=0; i<2; i++)
      { wd = getrate();	    /* R1/R3 */
	reportrate(wd);
      }
    /* look for E */
    until ((wd & 0xf000) == 0xf000) wd = getrwd();
    unless (wd == rateword) giveup("failed to detect valid E3");
    reportrate(wd);
  }

static ushort getrate()
  { ushort wd = getrwd();
l:  until ((wd & 0xf111) == 0x0111) wd = (wd << 1) | gbit();
    ushort rate = wd;
    for (int i = 0; i < 16; i++)	/* look for 16 identical rate signals */
      { wd = getrwd();
	if (wd != rate) goto l;
      }
    return rate;
  }

static ushort getrwd()
  { ushort wd;
    for (int i = 0; i < 16; i++) wd = (wd << 1) | gbit();
    return wd;
  }

static void reportrate(ushort r)
  { infomsg("<<< %s: rates = %04x", rate_strs[mstate-2], r);
    rateword &= r;
    mstate++;	/* from 2 to 3, or 4 to 5, or 5 to 6 */
  }

global int getasync()
  { int b = gbit(), nb = 0;
    while (nb < 10 && b) { b = gbit(); nb++; }
    if (b) return NOCHAR;  /* no char yet */
    int ch = 0;
    for (int i = 0; i < 8; i++)
      { int b = gbit();
	ch = (ch >> 1) | (b << 7);
      }
    return ch;
  }

static void rx2_loop()
  { /* round-trip-delay calculation */
    carrier -> resetphase();
    roundtrip();
    /* train equalizer */
    carrier -> resetphase();
    rcvdata();
    /* train canceller */
    carrier -> resetphase();
    traincanceller();
    /* exchange data */
    rcvdata();			/* never returns */
  }

static void roundtrip()
  { setduplex(64*SYMBLEN);
    timing = 0; ticker = 'a';
    eqz -> reset();
    wt_tone(0, 3, 100, true);			/* wait for stable AC... */    // WAS 50
    int t1 = wt_reversal(0, 3);			/* then reversal to CA... */
    mstate++;	/* from 0 to 1 */
    wt_tone(3, 0, 100, false);			/* swallow CA... */
    int t2 = wt_reversal(3, 0);			/* wait for reversal to AC... */
    mstate++;	/* from 1 to 2 */
    int dt = t2 - t1;
    setduplex(TRDELAY);
    float ms = (float) (dt - 128*SYMBLEN) / (float) SAMPLERATE * 1000.0f;
    char rtd[32]; sprintf(rtd, "%.1f", ms); infomsg("RTD = %sms (%d)", rtd, dt);
  }

static void rcvdata()
  { timing = 0; ticker = 'A';
    eqz -> reset();
    wt_tone(0, 1, 230, true);			/* wait for stable AB... (S) */
    wt_reversal(0, 1);				/* then BA... (Sbar) */
    /* adj equalizer coeffs and symbol timing; use training sequence */
    int bc = SEG_2 + 2;
    gpa -> reset();				/* reset scrambler before using trn */
    nextadj = samplecount + 2*SAMPLERATE;
    while (bc < SEG_3 + 1024)
      { complex z = getsymbol();		/* get equalized symbol */
	complex ez = trn -> get(bc++);		/* update equalizer using training sequence */
	float p = power(z-ez);
	acq_debug -> insert(z.re, p);
	eqz -> update(ez-z);
	adjtiming();				/* adjust symbol timing */
      }
    acq_debug -> tick('X');
    /* adj equalizer coeffs and symbol timing; use decoded data */
    dec -> reset();
    while (mstate == 2 || mstate >= 4)
      { complex z = getsymbol();		/* get equalized symbol */
	int bits = dec -> decode(z);		/* decode into 2 or 3 bits */
	if (dec -> rate & rb_7200) pbit(gpa -> rev(bits >> 2));
	pbit(gpa -> rev((bits >> 1) & 1));
	pbit(gpa -> rev(bits & 1));
	complex ez = dec -> getez();		/* get exact (quantized) z */
	eqz -> update(ez-z);			/* update equalizer from data sequence */
	adjtiming();				/* adjust symbol timing */
      }
  }

static void wt_tone(int k0, int k1, int max, bool chk)
  { /* wait for a stable tone */
    complex z0 = ztab2[k0], z1 = ztab2[k1];
    int bc = 0, cnt = 0;
    until (cnt >= max && !(bc & 1))
      { complex z = getsymbol();		/* get equalized symbol */
	complex ez = (bc++ & 1) ? z1 : z0;	/* expected z */
	float p = power(z-ez);
	acq_debug -> insert(z.re, p);
	if (p < THRESHOLD || !chk) cnt++; else cnt = 0;
	eqz -> short_update(ez-z);		/* short update here */
      }
    acq_debug -> tick(ticker++);
  }

static int wt_reversal(int k0, int k1)
  { /* wait for a phase reversal */
    complex z0 = ztab2[k0], z1 = ztab2[k1];
    int bc = 0; int t;
    bool rev = false;
    until (rev & !(bc & 1))
      { complex z = getsymbol();		/* get equalized symbol */
	complex ez = (bc++ & 1) ? z1 : z0;	/* expected z */
	float p = power(z-ez);
	acq_debug -> insert(z.re, p);
	if (p >= THRESHOLD) { t = samplecount; rev = true; }
	eqz -> short_update(ez-z);		/* short update here */
      }
    acq_debug -> tick(ticker++);
    return t;
  }

static complex getsymbol()
  { for (int j = timing; j < 2; j++)		    /* timing is -1, 0 or +1 */
      { complex yz = gethalfsymb();
	eqz -> insert(yz);			    /* half-point equalization */
      }
    timing = 0;
    complex z = eqz -> get();
    co_debug -> insert(z);
    return z;
  }

static void adjtiming()
  { if (after(samplecount, nextadj))
      { int dt = eqz -> getdt();
	dt_debug -> insert(dt);
	if (dt > 0) { timing--; eqz -> shift(-1); }
	if (dt < 0) { timing++; eqz -> shift(+1); }
	nextadj = samplecount + 2*SAMPLERATE;	/* adjust every 2 secs */
      }
  }

static void traincanceller()
  { /* train canceller at half-symbol intervals */
    while (mstate == 3)
      { complex yz = gethalfsymb();
	can -> update(yz);
	can_debug -> insert(power(yz));
      }
  }

static complex gethalfsymb()
  { /* sample at half-symbol intervals */
    complex yz;
    for (int k = 0; k < SYMBLEN/2; k++)
      { float x = insample();
	complex cz = carrier -> cnext();
	yz = fe_lpf -> fstep(x*cz);	/* translate to baseband */
      }
    complex pe = can -> get();		/* subtract predicted echo */
    return yz - pe;
  }

