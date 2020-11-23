#line 1 "v29rx.F"
/* Modem for MIPS   AJF	  March 1998
   V.29 receive routines */

#include <coro.h>

#include <complex.h>
#include <filters.h>
#line 1 "<<generated_fspecs>>"

static float _fstepf_1(filter *fi, float x)
  { /* /usr/fisher/mipsbin/mkfilter -Bu -Lp -o 4 -a 0.125 -l */
    float *v = fi -> v;
    v[0] = v[1]; v[1] = v[2]; v[2] = v[3]; v[3] = v[4]; 
    v[4] =    (  1.0209480791e-02 * x)
            + ( -0.1203895999 * v[0]) + (  0.7244708295 * v[1])
            + ( -1.7358607092 * v[2]) + (  1.9684277869 * v[3]);
    return    (v[0] + v[4]) + 4 * (v[1] + v[3]) + 6 * v[2];
  }

static fspec _fspecs_1 = { 4, 4, _fstepf_1 };

#line 8 "v29rx.F"
#include <scramble.h>
#include <equalize.h>
#include <debug.h>
#include <sinegen.h>
#include <myaudio.h>

#include "modem.h"
#include "coder.h"

#define THRESHOLD 1.0f	/* sqr of radius of error circle */

static fspec *lpf_fs = (&_fspecs_1);   /* low-pass at 1200 Hz */

static bool inited = false;	/* statically initialized */

static sinegen *carrier;
static coroutine *bitco;
static cfilter *fe_lpf;
static scrambler *scr;
static equalizer *eqz;
static decoder *dec;
static traininggen *trn;
static co_debugger *co_debug;
static debugger *acq_debug;
static int timing, nextadj;

static void tidydebug(), bitloop(), wt_tone(), wt_reversal(), train_eqz(), dataloop();
static complex getsymbol(), gethalfsymb();
static void adjtiming();


global void initrx_v29()
  { unless (inited)
      { /* perform once-only initialization */
	carrier = new sinegen(1700.0);
	bitco = new coroutine(bitloop);
	fe_lpf = new cfilter(lpf_fs);
	scr = new scrambler(GPC);
	eqz = new equalizer(0.5);   /* was 0.25; V.29 has short training sequence so needs fast equalization */
	dec = new decoder();
	trn = new traininggen;
	co_debug = new co_debugger(24000);
	acq_debug = new debugger(2, 4000);
	atexit(tidydebug);
	inited = true;
      }
    bitco -> reset();
    for (int i = 0; i < 144; i++) getbit();	/* scrambled 1s */
  }

static void tidydebug()
  { co_debug -> print("debug_co.grap");
    acq_debug -> print("debug_acq.grap");
    eqz -> print("debug_eqz.grap");
  }

global int getbit()		/* bit input */
  { return callco(bitco);
  }

static void bitloop()
  { wt_tone();					/* look for stable ABAB... */
    wt_reversal();				/* look for CDCD... */
    train_eqz();
    dataloop();
  }

static void wt_tone()
  { /* wait for a stable ABAB... */
    timing = 0; eqz -> reset();
    complex z0 = complex(-3.0f, 0.0f),	/* A */
	    z1 = complex(1.0f, -1.0f);	/* B */
    int bc = 0, cnt = 0;
    until (cnt >= 64 && !(bc & 1))
      { complex z = getsymbol();		/* get equalized symbol */
	complex ez = (bc++ & 1) ? z1 : z0;	/* expected z */
	float p = power(z-ez);
	acq_debug -> insert(z.re, p);
	if (p < THRESHOLD) cnt++; else cnt = 0;
	eqz -> short_update(ez-z);		/* short update here */
      }
    acq_debug -> tick('A');
  }

static void wt_reversal()
  { /* wait for a phase reversal */
    complex z0 = complex(-3.0f, 0.0f),	/* A */
	    z1 = complex(1.0f, -1.0f);	/* B */
    int bc = 0;
    bool rev = false;
    until (rev & !(bc & 1))
      { complex z = getsymbol();		/* get equalized symbol */
	complex ez = (bc++ & 1) ? z1 : z0;	/* expected z */
	float p = power(z-ez);
	acq_debug -> insert(z.re, p);
	if (p >= THRESHOLD) rev = true;
	eqz -> short_update(ez-z);		/* short update here */
      }
    acq_debug -> tick('B');
  }

static void train_eqz()
  { /* adj equalizer coeffs and symbol timing; use training sequence */
    nextadj = samplecount + 2*SAMPLERATE;
    int bc = SEG_3;					/* need to start here because of scrambler in training gen! */
    trn -> reset(); trn -> get(bc++); trn -> get(bc++); /* but we've already read the first 2 symbols */
    while (bc < SEG_4)
      { complex z = getsymbol();		/* get equalized symbol */
	complex ez = trn -> get(bc++);		/* update equalizer using training sequence */
	float p = power(z-ez);
	acq_debug -> insert(z.re, p);
	eqz -> update(ez-z);
	adjtiming();				/* adjust symbol timing */
      }
    acq_debug -> tick('C');
  }

static void dataloop()
  { /* adj equalizer coeffs and symbol timing; use decoded data */
    scr -> reset(); dec -> reset();
    for (;;)
      { complex z = getsymbol();		/* get equalized symbol */
	int bits = dec -> decode(z);		/* get 3 bits */
	for (int i = 0; i < 3; i++)
	  { int b = scr -> rev((bits >> 2) & 1);	/* unscramble */
	    callco(currentco -> creator, b);		/* return the bit */
	    bits <<= 1;
	  }
	complex ez = dec -> getez();		/* get exact (quantized) z */
	eqz -> update(ez-z);			/* update equalizer from data sequence */
	adjtiming();				/* adjust symbol timing */
      }
  }

static complex getsymbol()
  { for (int j = timing; j < 2; j++)		/* timing is -1, 0 or +1 */
      { complex yz = gethalfsymb();
	eqz -> insert(yz);			/* half-point equalization */
      }
    timing = 0;
    complex z = eqz -> get();
    co_debug -> insert(z);
    return z;
  }

static complex gethalfsymb()
  { /* sample at half-symbol intervals */
    complex yz;
    for (int k = 0; k < SYMBLEN/2; k++)
      { float x = insample();
	complex cz = carrier -> cnext();
	yz = fe_lpf -> fstep(x*cz);		/* translate to baseband */
      }
    return yz;
  }

static void adjtiming()
  { if (after(samplecount, nextadj))
      { int dt = eqz -> getdt();
	if (dt > 0) { timing--; eqz -> shift(-1); }
	if (dt < 0) { timing++; eqz -> shift(+1); }
	nextadj = samplecount + 2*SAMPLERATE;	/* adjust every 2 secs */
      }
  }

