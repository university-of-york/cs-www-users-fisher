/* Modem for MIPS   AJF	  January 1995
   V.29 transmit routines */

#include <fishaudio.h>
#include "complex.h"
#include "modem.h"

#define SCALE	    (0.10 * MAXAMPL)
#define PHINC	    ((1700 * SINELEN) / SAMPLERATE)	/* 1700 Hz carrier */
#define BAUDLEN	    10					/* 2400 bd/sec	   */

/* tables for DPSK encoding */
static uchar phinctab[8] = { 1, 6, 2, 5, 0, 7, 3, 4 };

static c_complex constellation[16] =
  { { +3.0, 0.0 }, { +1.0, +1.0 }, { 0.0, +3.0 }, { -1.0, +1.0 },	/*  0 -	 3 */
    { -3.0, 0.0 }, { -1.0, -1.0 }, { 0.0, -3.0 }, { +1.0, -1.0 },	/*  4 -	 7 */
    { +5.0, 0.0 }, { +3.0, +3.0 }, { 0.0, +5.0 }, { -3.0, +3.0 },	/*  8 - 11 */
    { -5.0, 0.0 }, { -3.0, -3.0 }, { 0.0, -5.0 }, { +3.0, -3.0 },	/* 12 - 15 */
  };

static float shapetab[2*BAUDLEN+1] =
  { /* Raised cosine pulse shaping with Beta = 0.5 (see e.g. Proakis) */
    +1.0000000000, +0.9813348538, +0.9267741451, +0.8404773399, +0.7289115301,
    +0.6002108774, +0.4633870725, +0.3274811604, +0.2007514491, +0.0899847309,
    +0.0000000000, -0.0666120735, -0.1095007904, -0.1303353113, -0.1323963064,
    -0.1200421755, -0.0981227060, -0.0714131073, -0.0441321021, -0.0195910769,
    -0.0000000000,
  };

static uint sineptr, scrambler;
static uchar sigelement, bitcount;

static void init(vmode), pasync(int), psync(int), pbit(int), sendbaud();
static complex next_carrier();

global txhandler v29_txhandler = { init, pasync, psync, pbit };


static void init(vmode)
  { sineptr = 0;
    /* Segment 1 */
    sendpause(480);			    /* silence */
    /* Segment 2 */
    for (int i=0; i < 128; i++)
      { sigelement = (i & 1) ? 7 : 4;	    /* A or B */
	sendbaud();
      }
    /* Segment 3 */
    uchar reg = 0x2a;
    for (int i=0; i < 384; i++)
      { uint b6 = (reg >> 1) & 1;
	uint b7 = reg & 1;
	sigelement = b7 ? 3 : 0;	    /* C or D */
	sendbaud();
	reg >>= 1;
	if (b6 ^ b7) reg |= 0x40;
      }
    /* Segment 4 */
    scrambler = bitcount = 0;
    for (int i=0; i < 144; i++) pbit(1);    /* scrambled 1s */
  }

static void pasync(int)		/* asynchronous output */
  { giveup("Bug! async mode not supported in V.29");
  }

static void psync(int)		/* synchronous output */
  { giveup("Bug! sync mode not supported in V.29");
  }

static void pbit(int bit)		    /* V.29 bit output (7200 bit/s) */
  { int b18 = (scrambler >> 5) & 1;
    int b23 = scrambler & 1;
    bit = bit ^ b18 ^ b23;
    scrambler >>= 1;
    if (bit) scrambler |= 0x400000;
    if (++bitcount == 3)
      { uchar x = scrambler >> 20;		    /* delta phase from most recent 3 bits */
	sigelement = (sigelement + phinctab[x]) & 7;
	sendbaud();
	bitcount = 0;
      }
  }

static void sendbaud()
  { static complex a0 = 0.0, a1 = 0.0, a2 = 0.0, a3 = 0.0;
    a0 = a1; a1 = a2; a2 = a3; a3 = constellation[sigelement];
    for (int k=0; k < BAUDLEN; k++)
      { /* baseband pulse shaping */
	complex s = shapetab[BAUDLEN + k]   * a0
		  + shapetab[k]		    * a1
		  + shapetab[BAUDLEN - k]   * a2
		  + shapetab[2*BAUDLEN - k] * a3;
	/* modulate onto carrier */
	complex cz = next_carrier();
	float val = SCALE * (s.re*cz.re + s.im*cz.im);
	if (val > MAXAMPL || val < -MAXAMPL) giveup("Bug! out of range (V.29 sendbaud): %08x", (int) val);
	Audio -> write((int) val);
      }
  }

static complex next_carrier()
  { complex z = complex(sinetab[sineptr & (SINELEN-1)],
			sinetab[(sineptr + SINELEN/4) & (SINELEN-1)]);
    sineptr += PHINC;
    return z;
  }

