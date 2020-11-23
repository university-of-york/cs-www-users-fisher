/* Modem for MIPS   AJF	  January 1995
   FSK transmit routines */

#include <complex.h>
#include <sinegen.h>
#include <myaudio.h>

#include "modem.h"

struct fskinfo
  { int bps;			/* Tx bit rate	*/
    float f0, f1;		/* Tx tone freqs */
  };

static fskinfo fskinfo[] =
  { {  300, 1180.0,  980.0 },	/* V21o */
    {  300, 1850.0, 1650.0 },	/* V21a */
    {	75,  450.0,  390.0 },	/* V23o */
    { 1200, 2100.0, 1300.0 },	/* V23a */
  };

static sinegen *sgen = NULL;	/* statically init'ed */

static float tone0, tone1;
static int bitlen;
static uchar prevbits;

static void putoctet(uchar), pbit(int);


global void inittx_fsk(vmode mode)
  { unless (mode >= 0 && mode < 4) giveup("Bug! bad mode %d in fsk tx init", mode);
    if (sgen == NULL) sgen = new sinegen(0.0);	/* once-only initialization */
    tone0 = fskinfo[mode].f0;
    tone1 = fskinfo[mode].f1;
    bitlen = SAMPLERATE / fskinfo[mode].bps;	/* num. samples in one bit */
    prevbits = 0;
  }

global void putasync(int n)			/* asynchronous output */
  { uint un = (n >= 0) ? (n << 1) | 0x200 :	/* add start bit, 1 stop bit */
			 0x3ff;			/* send mark bits while idle */
    until (un == 0) { pbit(un & 1); un >>= 1; }
  }

global void putsync(int x)			/* synchronous output */
  { if (x == HDLC_FLAG) putoctet(0x7e);
    else if (x == HDLC_ABORT) putoctet(0x7f);
    else
      { uchar n = x;
	for (int i = 0; i < 8; i++)
	  { pbit(n >> 7);
	    if ((prevbits & 0x1f) == 0x1f) pbit(0); /* bit-stuffing */
	    n <<= 1;
	  }
      }
  }

static void putoctet(uchar n)
  { for (int i=0; i < 8; i++) { pbit(n >> 7); n <<= 1; }
  }

static void pbit(int bit)
  { sgen -> setfreq(bit ? tone1 : tone0);
    for (int i = 0; i < bitlen; i++)
      { float val = sgen -> fnext();
	outsample(val);
      }
    prevbits = (prevbits << 1) | bit;
  }

