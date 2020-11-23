// #include <stdio.h>
#include <coro.h>

#include <complex.h>
#include <scramble.h>
#include <sinegen.h>
#include <myaudio.h>
#include <mystdio.h>

#include "modem.h"
#include "cancel.h"
#include "coder.h"

#define TX_TRNLEN   8000	/* length of training sequence transmitted (WAS 4096) */

static float shapetab[2*SYMBLEN+1] =
  { /* Raised cosine pulse shaping with Beta = 0.5; square-root, with x/sinx compensation */
    +1.1510798492, +0.9848450677, +0.5795790021, +0.1495226763,
    -0.1156411181, -0.1625782395, -0.0750914609, +0.0185567369,
    +0.0448950047,
  };

static sinegen *carrier;
static coroutine *tx1_co, *tx2_co;
static scrambler *gpc;
static traininggen *trn;
static encoder *enc;

static void sendrate(ushort);
static void tx2_loop();
static void sendsymbol(complex);

inline void pbit(int b) { callco(tx2_co, b);	 }
inline int gbit()	{ return callco(tx1_co); }


global void inittx()
  { carrier = new sinegen(1800.0);
    tx1_co = currentco;
    tx2_co = new coroutine(tx2_loop);
    gpc = new scrambler(GPC);
    trn = new traininggen(gpc);
    enc = new encoder;
    pbit(1);					    /* sync to symbol boundary */
    ushort r2 = rateword;
    infomsg(">>> R2: rates = %04x", r2);
    while (mstate < 3) sendrate(r2);		    /* send R2 */
    /* now rateword == R2 & R3 */
    rateword |= 0xf000;
    /* pick highest common bit rate, leave just one rate bit set */
    if (rateword & rb_14400) rateword &= ~(rb_12000 | rb_9600 | rb_7200 | rb_4800);
    else if (rateword & rb_12000) rateword &= ~(rb_9600 | rb_7200 | rb_4800);
    else if (rateword & rb_9600) rateword &= ~(rb_7200 | rb_4800);
    else if (rateword & rb_7200) rateword &= ~rb_4800;
    else if (rateword & rb_4800) ;
    else giveup("can't agree on a speed!");
    infomsg(">>> E2: rates = %04x", rateword);      /* send E2 */
    sendrate(rateword);
    enc -> setrate(rateword);			    /* tell encoder what bit rate to use */
    while (mstate < 4) pbit(1);			    /* sync to Rx side */
    for (int i = 0; i < 128; i++) pbit(1);	    /* followed by 128 "1" bits */
  }

static void sendrate(ushort wd)
  { for (int i=0; i < 16; i++) { pbit(wd >> 15); wd <<= 1; }
  }

global void putasync(int n)			/* asynchronous output */
  { uint un = (n >= 0) ? (n << 1) | 0x200 :	/* add start bit, 1 stop bit */
			 0x3ff;			/* send mark bits while idle */
    until (un == 0) { pbit(un & 1); un >>= 1; }
  }

static void tx2_loop()
  { /* train equalizer */
    while (mstate == 0) sendsymbol(0.0);
    /* train canceller */
    carrier -> resetphase();
    gpc -> reset();	/* reset scrambler before using trn */
    for (int bc = SEG_1; bc < SEG_3 + TX_TRNLEN; bc++) sendsymbol(trn -> get(bc));
    mstate++;		/* from 1 to 2 */
    /* exchange data */
    enc -> reset();
    for (;;)
      { int bits = 0;
	bits = (bits << 1) | gpc -> fwd(gbit());
	bits = (bits << 1) | gpc -> fwd(gbit());
	if (enc -> rate & rb_7200) bits = (bits << 1) | gpc -> fwd(gbit());
	sendsymbol(enc -> encode(bits));
      }
  }

static void sendsymbol(complex z)
  { static complex a0 = 0.0, a1 = 0.0, a2 = 0.0, a3 = 0.0;
    for (int k = 0; k < SYMBLEN; k++)
      { /* baseband pulse shaping */
	complex s = shapetab[SYMBLEN + k]   * a0
		  + shapetab[k]		    * a1
		  + shapetab[SYMBLEN - k]   * a2
		  + shapetab[2*SYMBLEN - k] * a3;
	/* insert baseband sample into canceller */
	can -> insert(s);
	/* modulate onto carrier */
	complex cz = carrier -> cnext();
	outsample(0.2f * (s.re*cz.re + s.im*cz.im));
      }
    a0 = a1; a1 = a2; a2 = a3; a3 = z;
  }

