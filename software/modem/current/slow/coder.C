/* Modem for MIPS   AJF	  March 1998
   V.29 encoding / decoding routines */

#include <math.h>

#include <complex.h>

#include "modem.h"
#include "coder.h"

/* tables for DPSK encoding */
static uchar fwd_phinctab[8] = { 1, 0, 2, 3, 6, 7, 5, 4 };
static uchar rev_phinctab[8] = { 1, 0, 2, 3, 7, 6, 4, 5 };

static c_complex constellation[8] =
  { { +3.0, 0.0 }, { +1.0, +1.0 }, { 0.0, +3.0 }, { -1.0, +1.0 },	/*  0 -	 3 */
    { -3.0, 0.0 }, { -1.0, -1.0 }, { 0.0, -3.0 }, { +1.0, -1.0 },	/*  4 -	 7 */
  };

static int locate(complex);


complex traininggen::get(int bc)
  { /* V.29 training sequence */
    complex z;
    if (bc < SEG_2)
      { /* segment 1 : silence */
	z = 0.0;
      }
    else
      { int k;
	if (bc < SEG_3)
	  { /* segment 2 : ABAB... */
	    k = (bc & 1) ? 7 : 4;
	  }
	else
	  { /* segment 3 : CDCD... */
	    int b6 = (reg >> 1) & 1;
	    int b7 = reg & 1;
	    k = b7 ? 3 : 0;
	    reg >>= 1;
	    if (b6 ^ b7) reg |= 0x40;
	  }
	z = constellation[k];
      }
    return z;
  }

complex encoder::encode(int bits)
  { state += fwd_phinctab[bits & 7];
    state &= 7;
    return constellation[state];
  }

int decoder::decode(complex z)
  { int nst = locate(z);
    int bits = rev_phinctab[(nst-state) & 7];
    state = nst;
    return bits;
  }

static int locate(complex z)
  { int k;
    float m = fabsf(z.re) - fabsf(z.im);
    if (m > +1.5 || m < -1.5)
      { int b1 = (z.im > z.re);
	int b2 = (z.im < -z.re);
	k = (b2 << 2) | ((b1 ^ b2) << 1);
      }
    else
      { int b1 = (z.re < 0.0);
	int b2 = (z.im < 0.0);
	k = (b2 << 2) | ((b1 ^ b2) << 1) | 1;
      }
    return k;
  }

complex decoder::getez()
  { return constellation[state];    /* return exact (quantized) value of last decoded symbol */
  }

