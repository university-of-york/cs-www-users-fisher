/* Modem for MIPS   AJF	  January 1995
   FSK receive routines */

#include <fishaudio.h>
#include <coro.h>
#include "modem.h"
#include "filters.h"

/* Bandpass filter coeffs constructed by:
   mkfilter -Bu -Bp -o 2 -a (A1) (A2)
   where A1 = (F0 - Bd/2) / 24000, A2 = (F0 + Bd/2) / 24000 */

static fspec *bpfspecs[] =
  { mkfilter("-Bu -Bp -o 4 -a 0.01437500000 0.01812500000"),    /*  345 ..  435 Hz, centre  390 Hz    [0] */
    mkfilter("-Bu -Bp -o 4 -a 0.01687500000 0.02062500000"),    /*  405 ..  495 Hz, centre  450 Hz    [1] */
    mkfilter("-Bu -Bp -o 4 -a 0.03458333333 0.04708333333"),    /*  830 .. 1130 Hz, centre  980 Hz    [2] */
    mkfilter("-Bu -Bp -o 4 -a 0.04291666667 0.05541666667"),    /* 1030 .. 1330 Hz, centre 1180 Hz    [3] */
    mkfilter("-Bu -Bp -o 4 -a 0.02916666667 0.07916666667"),    /*  700 .. 1900 Hz, centre 1300 Hz    [4] */
    mkfilter("-Bu -Bp -o 4 -a 0.06250000000 0.07500000000"),    /* 1500 .. 1800 Hz, centre 1650 Hz    [5] */
    mkfilter("-Bu -Bp -o 4 -a 0.07083333333 0.08333333333"),    /* 1700 .. 2000 Hz, centre 1850 Hz    [6] */
    mkfilter("-Bu -Bp -o 4 -a 0.06250000000 0.11250000000"),    /* 1500 .. 2700 Hz, centre 2100 Hz    [7] */
  };

/* Lpwpass filter coeffs constructed by:
   mkfilter -Bu -Lp -o 2 -a (A1)
   where A1 = (Bd/2) / 24000 */

static fspec *lpfspecs[] =
  { mkfilter("-Bu -Lp -o 4 -a 0.0015625"),      /*  37.5 Hz   [0] */
    mkfilter("-Bu -Lp -o 4 -a 0.0062500"),      /* 150   Hz   [1] */
    mkfilter("-Bu -Lp -o 4 -a 0.0250000"),      /* 600   Hz   [2] */
  };

struct info
  { int hbl;		  /* half bit length (num. samples)	  */
    fspec *lpfs;	  /* low-pass filter spec		  */
    fspec *bpfs[2];	  /* bandpass filter specs for 0, 1 tones */
  };

static info infotab[] =
  { {  40, lpfspecs[1], bpfspecs[6], bpfspecs[5] },	/* V21o	  300 bd */
    {  40, lpfspecs[1], bpfspecs[3], bpfspecs[2] },	/* V21a	  300 bd */
    {  10, lpfspecs[2], bpfspecs[7], bpfspecs[4] },	/* V23o	 1200 bd */
    { 160, lpfspecs[0], bpfspecs[1], bpfspecs[0] },	/* V23a	   75 bd */
  };

static ushort hbitlen;

/* the following are init'ed so that first "delete" doesn't fail */
static coroutine *syncco = NULL;
static filter *bpf[2] = { NULL, NULL };
static filter *lpf[2] = { NULL, NULL };

static void init(vmode), syncloop();
static int gasync(), gsync(), gbit();
static bool getsample();

global rxhandler fsk_rxhandler = { init, gasync, gsync, gbit };


static void init(vmode mode)
  { unless (mode >= 0 && mode < 4) giveup("Bug! bad mode %d in fsk rx init", mode);
    info *inf = &infotab[mode];
    for (int i=0; i<2; i++)
      { delete bpf[i]; bpf[i] = new filter(inf -> bpfs[i]);
	delete lpf[i]; lpf[i] = new filter(inf -> lpfs);
      }
    hbitlen = inf -> hbl;   /* num. samples in half a bit */
    Audio -> idiscard();    /* empty Rx ring buffer */
    delete syncco;
    syncco = new coroutine(syncloop);
  }

static int gasync()	/* asynchronous input */
  { bool bit; int i, j; uchar n;
    do bit = getsample(); while (bit);
    for (j=0; j < 3*hbitlen; j++) bit = getsample();
    for (i=0; i < 8; i++)
      { n = (n >> 1) | (bit << 7);
	for (j=0; j < 2*hbitlen; j++) bit = getsample();
      }
    return n;
  }

static int gsync()	/* synchronous input */
  { return callco(syncco, 0);
  }

static int gbit()	/* bit input */
  { giveup("Bug: bit input not supported in fsk modes");
  }

static void syncloop()
  { uchar valid = 0, framing = 0x55, bitcount = 0;
    uchar thebits, thechar;
    for (;;)
      { int j = 0; bool bit;
	while (j < 2*hbitlen)
	  { bit = getsample();
	    framing = (framing << 1) | bit;
	    if (framing == 0xf0 || framing == 0x0f) j = hbitlen+4; else j++;
	  }
	thebits = (thebits << 1) | bit;
	valid = (valid << 1) | 1;
	switch (thebits)
	  { case 0x7c:	case 0x7d:
		valid &= ~2;	/* delete bit-stuffing */
		break;

	    case 0x7e:
		callco(NULL, HDLC_FLAG);	/* call caller */
		valid = bitcount = 0;
		break;

	    case 0x7f:
		callco(NULL, HDLC_ABORT);	/* call caller */
		valid = bitcount = 0;
		break;
	  }
	if (valid & 0x80)
	  { thechar = (thechar << 1) | (thebits >> 7);
	    if (++bitcount == 8)
	      { callco(NULL, thechar);		/* call caller */
		bitcount = 0;
	      }
	  }
      }
  }

static bool getsample()
  { float x = (float) Audio -> read();
    float p[2];
    for (int i=0; i<2; i++)
      { float y = bpf[i] -> fstep(x);
	p[i] = lpf[i] -> fstep(sqr(y));
      }
    return (p[1] >= p[0]);
  }

