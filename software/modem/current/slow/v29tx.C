/* Modem for MIPS   AJF	  January 1995
   V.29 transmit routines */

#include <coro.h>

#include <complex.h>
#include <scramble.h>
#include <sinegen.h>
#include <mystdio.h>
#include <myaudio.h>

#include "modem.h"
#include "coder.h"

static float shapetab[2*SYMBLEN+1] =
  { /* Raised cosine pulse shaping with Beta = 0.5; square-root, with x/sinx compensation */
    +1.1510798492, +0.9848450677, +0.5795790021, +0.1495226763,
    -0.1156411181, -0.1625782395, -0.0750914609, +0.0185567369,
    +0.0448950047,
  };

static bool inited = false;	/* statically init'ed */

static sinegen *carrier;
static coroutine *bitco;
static scrambler *scr;
static encoder *enc;
static traininggen *trn;

static void bitloop(), sendsymbol(complex);


global void inittx_v29()
  { unless (inited)
      { /* perform once-only initialization */
	carrier = new sinegen(1700.0);
	bitco = new coroutine(bitloop);
	scr = new scrambler(GPC);
	enc = new encoder;
	trn = new traininggen;
	inited = true;
      }
    bitco -> reset();
    for (int i = 0; i < 144; i++) putbit(1);	/* scrambled 1s */
  }

global void putbit(int bit)	/* V.29 bit output (7200 bit/s) */
  { callco(bitco, bit);
  }

static void bitloop()
  { trn -> reset();
    for (int bc = SEG_1; bc < SEG_4; bc++) sendsymbol(trn -> get(bc));	/* send training sequence */
    scr -> reset(); enc -> reset();
    for (;;)
      { int bits = 0;
	for (int i = 0; i < 3; i++)
	  { int b = callco(currentco -> creator);
	    bits = (bits << 1) | scr -> fwd(b);
	  }
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
	/* modulate onto carrier */
	complex cz = carrier -> cnext();
	outsample(0.2 * (s.re*cz.re + s.im*cz.im));
      }
    a0 = a1; a1 = a2; a2 = a3; a3 = z;
  }

