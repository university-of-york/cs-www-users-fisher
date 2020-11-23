#line 1 "rxside.F"
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

#line 7 "rxside.F"
#include <equalize.h>
#include <debug.h>
#include <myaudio.h>
#include <sinegen.h>
#include <goertzel.h>

#include "modem.h"

#define DEBUGMAX 19200

static fspec *fefs = (&_fspecs_1);   /* 400 Hz lowpass */

static cfilter *fe_lpf;
static equalizer *eqz;
static sinegen *carrier;
static char *debugbuf;
static int debugptr;

static void tidydebug(), getranging();
static bool getinfo();
static int getreversal(), gbit();
static complex getsymbol();
static complex gethalfsymb();
static void getprobing();
static void debug(char*), debug(char);	/* overloaded */


global void rxside()
  { debugbuf = new char[DEBUGMAX];
    debugptr = 0;
    atexit(tidydebug);
    carrier = new sinegen(2400.0);
    fe_lpf = new cfilter(fefs);
    eqz = new equalizer(0.25);
    setduplex(DELAY);
    getranging();
    setduplex(DELAY);
    getprobing();
    callco(currentco -> creator);
  }

static void tidydebug()
  { for (int k = 0; k < debugptr; k++) putc(debugbuf[k], stderr);
    fprintf(stderr, "\r\n");
  }

static void getranging()
  { bool ack;
    do ack = getinfo(); until (ack);		/* keep looking for an info seq with ack bit set */
    mstate++;					/* 0 to 1 (Tx starts sending B) */
    int t1 = getreversal();			/* get reversal */
    mstate++;					/* 1 to 2 (Tx sends Bbar for 10 ms, then silence) */
    int t2 = getreversal();			/* get reversal */
    int dt = t2 - t1;
    float ms = (float) (dt - 2*DELAY) / (float) SAMPLERATE * 1000.0f;
    char rtd[32]; sprintf(rtd, "%.1f", ms); infomsg("RTD = %sms (%d)", rtd, dt);
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
  { int cnt = 0;
    while (cnt < 20)
      { if (gbit()) cnt = 0; else cnt++;
      }
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
    return eqz -> get();
  }

static complex gethalfsymb()
  { /* sample at half-symbol intervals */
    complex yz;
    for (int k = 0; k < SYMBLEN/2; k++)
      { float x = insample();
	complex cz = carrier -> cnext();
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

static void debug(char *s)
  { int k = 0;
    until (s[k] == '\0') debug(s[k++]);
  }

static void debug(char c)
  { if (debugptr >= DEBUGMAX) exit(3);
    debugbuf[debugptr++] = c;
  }

