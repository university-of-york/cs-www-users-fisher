#line 1 "infoseqs.F"
#include <stdio.h>	// ???
#include <math.h>
#include <coro.h>

#include <complex.h>
#include <filters.h>
#line 1 "<<generated_fspecs>>"

static float _fstepf_1(filter *fi, float x)
  { /* /usr/fisher/mipsbin/mkfilter -Bu -Lp -o 4 -a 0.03125 -l */
    float *v = fi -> v;
    v[0] = v[1]; v[1] = v[2]; v[2] = v[3]; v[3] = v[4]; 
    v[4] =    (  7.2772549288e-05 * x)
            + ( -0.5980652616 * v[0]) + (  2.6988843913 * v[1])
            + ( -4.5892912321 * v[2]) + (  3.4873077415 * v[3]);
    return    (v[0] + v[4]) + 4 * (v[1] + v[3]) + 6 * v[2];
  }

static fspec _fspecs_1 = { 4, 4, _fstepf_1 };

#line 7 "infoseqs.F"
#include <equalize.h>
#include <debug.h>
#include <myaudio.h>
#include <mystdio.h>
#include <sinegen.h>
#include <goertzel.h>

#include "modem.h"

#define SECS(f) ((int) (f * SAMPLERATE + 0.5))

#define SYMBLEN (SAMPLERATE/600)    /* 600 bps */
#define DELAY	SECS(0.04)	    /* 40 ms */
#define DEBUGMAX 19200

static fspec *fefs = (&_fspecs_1);   /* 400 Hz lowpass */

static cfilter *fe_lpf;
static equalizer *eqz;
static co_debugger *co_debug;
static int mstate;
static sinegen *tx_carrier, *rx_carrier;
static char *debugbuf;
static int debugptr;

static void rxside(), tidydebug(), getranging();
static bool getinfo();
static int getreversal(), gbit();
static complex getsymbol();
static complex gethalfsymb();
static void getprobing();
static void txside();
static void pbit(int), outsymbol(float);
static void debug(char*), debug(char);	/* overloaded */


global void infoseqs()
  { my_alarm(10);		    /* 10 secs timeout */
    setduplex(DELAY - 2*SYMBLEN);   /* 40 ms minus RC filter delay */
    mstate = 0;
    debugbuf = new char[DEBUGMAX];
    debugptr = 0;
    coroutine *rx = new coroutine(rxside);
    coroutine *tx = new coroutine(txside);
    inparallel(rx, tx);
    my_alarm(0);		    /* cancel alarm */
  }

static void rxside()
  { co_debug = new co_debugger(24000);
    atexit(tidydebug);
    getranging();
    getprobing();
    callco(mainco);
  }

static void tidydebug()
  { co_debug -> print("debug_co.grap");
    for (int k = 0; k < debugptr; k++) putc(debugbuf[k], stderr);
    fprintf(stderr, "\r\n");
  }

static void getranging()
  { rx_carrier = new sinegen(2400.0);
    fe_lpf = new cfilter(fefs);
    eqz = new equalizer(0.25);
    bool ack;
    do ack = getinfo(); until (ack);		/* keep looking for an info seq with ack bit set */
    mstate++;					/* 0 to 1 */
    int t1 = getreversal();			/* get reversal */
    mstate++;					/* 1 to 2 */
    int t2 = getreversal();			/* get reversal */
    mstate++;					/* 2 to 3 */
    int dt = t2 - t1;
    float ms = (float) (dt - 2*DELAY) / (float) SAMPLERATE * 1000.0f;
    char rtd[32]; sprintf(rtd, "%.1f", ms); infomsg("RTD = %sms (%d)", rtd, dt);
    delete rx_carrier; fe_lpf; delete eqz;
  }

static bool getinfo()
  { eqz -> reset();
    uchar bits = 0xff;
    until (bits == 0x72) bits = (bits << 1) | gbit();	/* look for sync byte */
    debug("\r\n[ ");
    bool ack;
    for (int i = 0; i < 37; i++)
      { int b = gbit();
	if (i == 16) ack = b;
	if (i%8 == 7) debug(' ');
      }
    debug(" ]\r\n");
    return ack;
  }

static int getreversal()
  { for (int i = 0; i < 30; i++) gbit();
    until (gbit()) ;
    debug('*');
    return samplecount;
  }

static int gbit()
  { complex z = getsymbol();
    bool bit = (z.re > 0.0f);
    complex ez = bit ? complex(+1.0f) : complex(-1.0f);
    eqz -> update(ez - z);
    static uchar bits = 0;
    bits = (bits << 1) | bit;
    uchar tab[] = { 0, 1, 1, 0 };
    int b = tab[bits & 3];   /* diff. decode */
    debug('0'+b);
    return b;
  }

static complex getsymbol()
  { for (int j = 0; j < 2; j++)
      { complex yz = gethalfsymb();
	eqz -> insert(yz);	/* half-point equalization */
      }
    complex z = eqz -> get();
    co_debug -> insert(z);
    return z;
  }

static complex gethalfsymb()
  { /* sample at half-symbol intervals */
    complex yz;
    for (int k = 0; k < SYMBLEN/2; k++)
      { float x = insample();
	complex cz = rx_carrier -> cnext();
	yz = fe_lpf -> fstep(x*cz);	/* translate to baseband */
      }
    return yz;
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
  { tx_carrier = new sinegen(1200.0);
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
    for (int i = 0; i < 38; i++) pbit(0);
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
	float cx = tx_carrier -> fnext();
	outsample(s*cx);
      }
  }

static void debug(char *s)
  { int k = 0;
    until (s[k] == '\0') debug(s[k++]);
  }

static void debug(char c)
  { if (debugptr >= DEBUGMAX) exit(3);
    debugbuf[debugptr++] = c;
  }

