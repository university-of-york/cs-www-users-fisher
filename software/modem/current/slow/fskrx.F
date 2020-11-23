/* Modem for MIPS   AJF	  January 1995
   FSK receive routines */

#include <coro.h>

#include <complex.h>
#include <filters.h>
#include <myaudio.h>
#include <mystdio.h>
#include <tonedec.h>

#include "modem.h"

static fspec *fefs = mkfilter("-Hp -Bu -o 2 -a 0.03125");       /* 300 Hz hpf */

/* Bandpass filter coeffs constructed by:
   mkfilter -Bu -Bp -o 2 -a (A1) (A2)
   where A1 = (F0 - bps/2) / SAMPLERATE, A2 = (F0 + bps/2) / SAMPLERATE */

static fspec *bpfspecs[] =
  { mkfilter("-Bu -Bp -o 4 -a 0.03593750000 0.04531250000"),    /*  345 ..  435 Hz, centre  390 Hz    [0] */
    mkfilter("-Bu -Bp -o 4 -a 0.04218750000 0.05156250000"),    /*  405 ..  495 Hz, centre  450 Hz    [1] */
    mkfilter("-Bu -Bp -o 4 -a 0.08645833333 0.11770833333"),    /*  830 .. 1130 Hz, centre  980 Hz    [2] */
    mkfilter("-Bu -Bp -o 4 -a 0.10729166667 0.13854166667"),    /* 1030 .. 1330 Hz, centre 1180 Hz    [3] */
    mkfilter("-Bu -Bp -o 4 -a 0.07291666667 0.19791666667"),    /*  700 .. 1900 Hz, centre 1300 Hz    [4] */
    mkfilter("-Bu -Bp -o 4 -a 0.15625000000 0.18750000000"),    /* 1500 .. 1800 Hz, centre 1650 Hz    [5] */
    mkfilter("-Bu -Bp -o 4 -a 0.17708333333 0.20833333333"),    /* 1700 .. 2000 Hz, centre 1850 Hz    [6] */
    mkfilter("-Bu -Bp -o 4 -a 0.15625000000 0.28125000000"),    /* 1500 .. 2700 Hz, centre 2100 Hz    [7] */
  };

/* Lpwpass filter coeffs constructed by:
   mkfilter -Bu -Lp -o 2 -a (A1)
   where A1 = (bps/2) / SAMPLERATE */

static fspec *lpfspecs[] =
  { mkfilter("-Bu -Lp -o 4 -a 0.00390625"),     /*  37.5 Hz   [0] */
    mkfilter("-Bu -Lp -o 4 -a 0.01562500"),     /* 150   Hz   [1] */
    mkfilter("-Bu -Lp -o 4 -a 0.06250000"),     /* 600   Hz   [2] */
  };

struct info
  { int bitlen;		    /* bit length (num. samples)	    */
    fspec *lpfs;	    /* low-pass filter spec		    */
    fspec *bpfs0, *bpfs1;   /* bandpass filter specs for 0, 1 tones */
  };

static info infotab[] =
  { {  32, lpfspecs[1], bpfspecs[6], bpfspecs[5] },	/* V21o	  300 bps */
    {  32, lpfspecs[1], bpfspecs[3], bpfspecs[2] },	/* V21a	  300 bps */
    {	8, lpfspecs[2], bpfspecs[7], bpfspecs[4] },	/* V23o	 1200 bps */
    { 128, lpfspecs[0], bpfspecs[1], bpfspecs[0] },	/* V23a	   75 bps */
  };

static int bitlen;

static bool inited = false;	/* statically initialized */

static coroutine *syncco;
static tone_detector *td0, *td1;

static void syncloop();
static int getsample();


global void initrx_fsk(vmode mode)
  { if (inited) { delete td0; delete td1; }
    unless (inited)	/* once-only initialization */
      { syncco = new coroutine(syncloop);
	inited = true;
      }
    unless (mode >= 0 && mode < 4) giveup("Bug! bad mode %d in fsk rx init", mode);
    info *inf = &infotab[mode];
    td0 = new tone_detector(fefs, inf -> bpfs0, inf -> lpfs, false);
    td1 = new tone_detector(fefs, inf -> bpfs1, inf -> lpfs, false);
    bitlen = inf -> bitlen; /* num. samples in a bit */
    syncco -> reset();
  }

global int getasync()	    /* asynchronous input */
  { int i, j; uchar n;
    int b = getsample(), nb = 0;
    while (nb < 10*bitlen && b) { b = getsample(); nb++; }
    if (b) return NOCHAR;   /* no char yet */
    for (j = 0; j < (3*bitlen)/2; j++) b = getsample();	   /* position to centre of first data bit */
    for (i = 0; i < 8; i++)
      { n = (n >> 1) | (b << 7);
	for (j = 0; j < bitlen; j++) b = getsample();
      }
    return n;
  }

global int getsync()	    /* synchronous input */
  { return callco(syncco);
  }

static void syncloop()
  { uchar valid = 0, framing = 0x55, bitcount = 0;
    uchar bits, byte;
    for (;;)
      { int j = 0; int bit;
	while (j < bitlen)
	  { bit = getsample();
	    framing = (framing << 1) | bit;
	    j = (framing == 0xf0 || framing == 0x0f) ? (bitlen/2)+4 : j+1;
	  }
	bits = (bits << 1) | bit;
	valid = (valid << 1) | 1;
	switch (bits)
	  { case 0x7c:	case 0x7d:
		valid &= ~2;	/* delete bit-stuffing */
		break;

	    case 0x7e:
		callco(currentco -> creator, HDLC_FLAG);	/* return a flag */
		valid = bitcount = 0;
		break;

	    case 0x7f:
		callco(currentco -> creator, HDLC_ABORT);	/* return an abort */
		valid = bitcount = 0;
		break;
	  }
	if (valid & 0x80)
	  { byte = (byte << 1) | (bits >> 7);
	    if (++bitcount == 8)
	      { callco(currentco -> creator, byte);		/* return a regular byte */
		bitcount = 0;
	      }
	  }
      }
  }

static int getsample()
  { float x = insample();
    td0 -> insert(x); td1 -> insert(x);
    return (td1 -> pow) > (td0 -> pow);
  }

