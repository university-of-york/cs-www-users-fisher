/* Modem for MIPS   AJF	  January 1995
   FSK transmit routines */

#include "modem.h"

struct fskinfo
  { ushort bd;		/* Tx baud rate	 */
    ushort f0, f1;	/* Tx tone freqs */
  };

static fskinfo fskinfo[] =
  { {  300, 1180,  980 },   /* V21o */
    {  300, 1850, 1650 },   /* V21a */
    {	75,  450,  390 },   /* V23o */
    { 1200, 2100, 1300 },   /* V23a */
  };

static ushort tone0, tone1, bdlen;
static uchar prevbits;

static void init(vmode), pasync(int), psync(int), putoctet(uchar), pbit(int);

global txhandler fsk_txhandler = { init, pasync, psync, pbit };


static void init(vmode mode)
  { unless (mode >= 0 && mode < 4) giveup("Bug! bad mode %d in fsk tx init", mode);
    tone0 = fskinfo[mode].f0;
    tone1 = fskinfo[mode].f1;
    bdlen = SAMPLERATE / fskinfo[mode].bd;  /* num. samples in one baud */
    prevbits = 0;
  }

static void pasync(int n)	/* asynchronous output */
  { uint un = (n << 1) | 0x600; /* add start bit, 2 stop bits */
    until (un == 0) { pbit(un & 1); un >>= 1; }
  }

static void psync(int x)	/* synchronous output */
  { if (x == HDLC_FLAG) putoctet(0x7e);
    else if (x == HDLC_ABORT) putoctet(0x7f);
    else
      { uchar n = x; int i;
	for (i=0; i < 8; i++)
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
  { sendfreq(bit ? tone1 : tone0, bdlen);
    prevbits = (prevbits << 1) | bit;
  }

