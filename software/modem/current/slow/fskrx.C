#line 1 "fskrx.F"
/* Modem for MIPS   AJF	  January 1995
   FSK receive routines */

#include <coro.h>

#include <complex.h>
#include <filters.h>
#line 1 "<<generated_fspecs>>"

static float _fstepf_1(filter *fi, float x)
  { /* /usr/fisher/mipsbin/mkfilter -Hp -Bu -o 2 -a 0.03125 -l */
    float *v = fi -> v;
    v[0] = v[1]; v[1] = v[2]; 
    v[2] =    (  8.7033077934e-01 * x)
            + ( -0.7575469445 * v[0]) + (  1.7237761728 * v[1]);
    return    (v[0] + v[2]) - 2 * v[1];
  }

static fspec _fspecs_1 = { 2, 2, _fstepf_1 };

static float _fstepf_2(filter *fi, float x)
  { /* /usr/fisher/mipsbin/mkfilter -Bu -Bp -o 4 -a 0.03593750000 0.04531250000 -l */
    float *v = fi -> v;
    v[0] = v[1]; v[1] = v[2]; v[2] = v[3]; v[3] = v[4]; 
    v[4] = v[5]; v[5] = v[6]; v[6] = v[7]; v[7] = v[8]; 
    v[8] =    (  6.9752013701e-07 * x)
            + ( -0.8573132535 * v[0]) + (  6.7668969461 * v[1])
            + (-23.5932085800 * v[2]) + ( 47.4460745570 * v[3])
            + (-60.1843751000 * v[4]) + ( 49.3079205940 * v[5])
            + (-25.4811693030 * v[6]) + (  7.5951586426 * v[7]);
    return    (v[0] + v[8]) - 4 * (v[2] + v[6]) + 6 * v[4];
  }

static fspec _fspecs_2 = { 8, 8, _fstepf_2 };

static float _fstepf_3(filter *fi, float x)
  { /* /usr/fisher/mipsbin/mkfilter -Bu -Bp -o 4 -a 0.04218750000 0.05156250000 -l */
    float *v = fi -> v;
    v[0] = v[1]; v[1] = v[2]; v[2] = v[3]; v[3] = v[4]; 
    v[4] = v[5]; v[5] = v[6]; v[6] = v[7]; v[7] = v[8]; 
    v[8] =    (  6.9752016299e-07 * x)
            + ( -0.8573132535 * v[0]) + (  6.6923550121 * v[1])
            + (-23.1543073350 * v[2]) + ( 46.3522570580 * v[3])
            + (-58.7085652640 * v[4]) + ( 48.1711764630 * v[5])
            + (-25.0071445210 * v[6]) + (  7.5114928473 * v[7]);
    return    (v[0] + v[8]) - 4 * (v[2] + v[6]) + 6 * v[4];
  }

static fspec _fspecs_3 = { 8, 8, _fstepf_3 };

static float _fstepf_4(filter *fi, float x)
  { /* /usr/fisher/mipsbin/mkfilter -Bu -Bp -o 4 -a 0.08645833333 0.11770833333 -l */
    float *v = fi -> v;
    v[0] = v[1]; v[1] = v[2]; v[2] = v[3]; v[3] = v[4]; 
    v[4] = v[5]; v[5] = v[6]; v[6] = v[7]; v[7] = v[8]; 
    v[8] =    (  7.2772549304e-05 * x)
            + ( -0.5980652616 * v[0]) + (  4.0990376798 * v[1])
            + (-13.2484867760 * v[2]) + ( 25.9936474290 * v[3])
            + (-33.7188652410 * v[4]) + ( 29.5600799340 * v[5])
            + (-17.1334523570 * v[6]) + (  6.0282616021 * v[7]);
    return    (v[0] + v[8]) - 4 * (v[2] + v[6]) + 6 * v[4];
  }

static fspec _fspecs_4 = { 8, 8, _fstepf_4 };

static float _fstepf_5(filter *fi, float x)
  { /* /usr/fisher/mipsbin/mkfilter -Bu -Bp -o 4 -a 0.10729166667 0.13854166667 -l */
    float *v = fi -> v;
    v[0] = v[1]; v[1] = v[2]; v[2] = v[3]; v[3] = v[4]; 
    v[4] = v[5]; v[5] = v[6]; v[6] = v[7]; v[7] = v[8]; 
    v[8] =    (  7.2772549293e-05 * x)
            + ( -0.5980652616 * v[0]) + (  3.6644426643 * v[1])
            + (-11.1300599420 * v[2]) + ( 21.0682850680 * v[3])
            + (-27.0350338160 * v[4]) + ( 23.9582210380 * v[5])
            + (-14.3932202710 * v[6]) + (  5.3891231873 * v[7]);
    return    (v[0] + v[8]) - 4 * (v[2] + v[6]) + 6 * v[4];
  }

static fspec _fspecs_5 = { 8, 8, _fstepf_5 };

static float _fstepf_6(filter *fi, float x)
  { /* /usr/fisher/mipsbin/mkfilter -Bu -Bp -o 4 -a 0.07291666667 0.19791666667 -l */
    float *v = fi -> v;
    v[0] = v[1]; v[1] = v[2]; v[2] = v[3]; v[3] = v[4]; 
    v[4] = v[5]; v[5] = v[6]; v[6] = v[7]; v[7] = v[8]; 
    v[8] =    (  1.0209485168e-02 * x)
            + ( -0.1203895999 * v[0]) + (  0.8607078056 * v[1])
            + ( -3.0834704657 * v[2]) + (  6.9712707669 * v[3])
            + (-10.8265470100 * v[4]) + ( 11.8178544010 * v[5])
            + ( -8.9162166465 * v[6]) + (  4.2594924397 * v[7]);
    return    (v[0] + v[8]) - 4 * (v[2] + v[6]) + 6 * v[4];
  }

static fspec _fspecs_6 = { 8, 8, _fstepf_6 };

static float _fstepf_7(filter *fi, float x)
  { /* /usr/fisher/mipsbin/mkfilter -Bu -Bp -o 4 -a 0.15625000000 0.18750000000 -l */
    float *v = fi -> v;
    v[0] = v[1]; v[1] = v[2]; v[2] = v[3]; v[3] = v[4]; 
    v[4] = v[5]; v[5] = v[6]; v[6] = v[7]; v[7] = v[8]; 
    v[8] =    (  7.2772549288e-05 * x)
            + ( -0.5980652616 * v[0]) + (  2.4115616747 * v[1])
            + ( -6.3503651535 * v[2]) + ( 10.6437685460 * v[3])
            + (-13.4950048970 * v[4]) + ( 12.1025995180 * v[5])
            + ( -8.2105790765 * v[6]) + (  3.5465701416 * v[7]);
    return    (v[0] + v[8]) - 4 * (v[2] + v[6]) + 6 * v[4];
  }

static fspec _fspecs_7 = { 8, 8, _fstepf_7 };

static float _fstepf_8(filter *fi, float x)
  { /* /usr/fisher/mipsbin/mkfilter -Bu -Bp -o 4 -a 0.17708333333 0.20833333333 -l */
    float *v = fi -> v;
    v[0] = v[1]; v[1] = v[2]; v[2] = v[3]; v[3] = v[4]; 
    v[4] = v[5]; v[5] = v[6]; v[6] = v[7]; v[7] = v[8]; 
    v[8] =    (  7.2772549288e-05 * x)
            + ( -0.5980652616 * v[0]) + (  1.8020335084 * v[1])
            + ( -4.7377941052 * v[2]) + (  7.1414399699 * v[3])
            + ( -9.4084488721 * v[4]) + (  8.1198574059 * v[5])
            + ( -6.1246826679 * v[6]) + (  2.6501657835 * v[7]);
    return    (v[0] + v[8]) - 4 * (v[2] + v[6]) + 6 * v[4];
  }

static fspec _fspecs_8 = { 8, 8, _fstepf_8 };

static float _fstepf_9(filter *fi, float x)
  { /* /usr/fisher/mipsbin/mkfilter -Bu -Bp -o 4 -a 0.15625000000 0.28125000000 -l */
    float *v = fi -> v;
    v[0] = v[1]; v[1] = v[2]; v[2] = v[3]; v[3] = v[4]; 
    v[4] = v[5]; v[5] = v[6]; v[6] = v[7]; v[7] = v[8]; 
    v[8] =    (  1.0209480791e-02 * x)
            + ( -0.1203895999 * v[0]) + (  0.2546702491 * v[1])
            + ( -0.9309959180 * v[2]) + (  1.2682731462 * v[3])
            + ( -2.4167363057 * v[4]) + (  2.1128659855 * v[5])
            + ( -2.5766910046 * v[6]) + (  1.2603185349 * v[7]);
    return    (v[0] + v[8]) - 4 * (v[2] + v[6]) + 6 * v[4];
  }

static fspec _fspecs_9 = { 8, 8, _fstepf_9 };

static float _fstepf_10(filter *fi, float x)
  { /* /usr/fisher/mipsbin/mkfilter -Bu -Lp -o 4 -a 0.00390625 -l */
    float *v = fi -> v;
    v[0] = v[1]; v[1] = v[2]; v[2] = v[3]; v[3] = v[4]; 
    v[4] =    (  2.1968455520e-08 * x)
            + ( -0.9378758961 * v[0]) + (  3.8116542349 * v[1])
            + ( -5.8096437117 * v[2]) + (  3.9358650214 * v[3]);
    return    (v[0] + v[4]) + 4 * (v[1] + v[3]) + 6 * v[2];
  }

static fspec _fspecs_10 = { 4, 4, _fstepf_10 };

static float _fstepf_11(filter *fi, float x)
  { /* /usr/fisher/mipsbin/mkfilter -Bu -Lp -o 4 -a 0.01562500 -l */
    float *v = fi -> v;
    v[0] = v[1]; v[1] = v[2]; v[2] = v[3]; v[3] = v[4]; 
    v[4] =    (  5.1232059674e-06 * x)
            + ( -0.7736282195 * v[0]) + (  3.2929432847 * v[1])
            + ( -5.2629037982 * v[2]) + (  3.7435067617 * v[3]);
    return    (v[0] + v[4]) + 4 * (v[1] + v[3]) + 6 * v[2];
  }

static fspec _fspecs_11 = { 4, 4, _fstepf_11 };

static float _fstepf_12(filter *fi, float x)
  { /* /usr/fisher/mipsbin/mkfilter -Bu -Lp -o 4 -a 0.06250000 -l */
    float *v = fi -> v;
    v[0] = v[1]; v[1] = v[2]; v[2] = v[3]; v[3] = v[4]; 
    v[4] =    (  9.3349861292e-04 * x)
            + ( -0.3555773823 * v[0]) + (  1.7861066002 * v[1])
            + ( -3.4223095294 * v[2]) + (  2.9768443337 * v[3]);
    return    (v[0] + v[4]) + 4 * (v[1] + v[3]) + 6 * v[2];
  }

static fspec _fspecs_12 = { 4, 4, _fstepf_12 };

#line 8 "fskrx.F"
#include <myaudio.h>
#include <mystdio.h>
#include <tonedec.h>

#include "modem.h"

static fspec *fefs = (&_fspecs_1);       /* 300 Hz hpf */

/* Bandpass filter coeffs constructed by:
   mkfilter -Bu -Bp -o 2 -a (A1) (A2)
   where A1 = (F0 - bps/2) / SAMPLERATE, A2 = (F0 + bps/2) / SAMPLERATE */

static fspec *bpfspecs[] =
  { (&_fspecs_2),    /*  345 ..  435 Hz, centre  390 Hz    [0] */
    (&_fspecs_3),    /*  405 ..  495 Hz, centre  450 Hz    [1] */
    (&_fspecs_4),    /*  830 .. 1130 Hz, centre  980 Hz    [2] */
    (&_fspecs_5),    /* 1030 .. 1330 Hz, centre 1180 Hz    [3] */
    (&_fspecs_6),    /*  700 .. 1900 Hz, centre 1300 Hz    [4] */
    (&_fspecs_7),    /* 1500 .. 1800 Hz, centre 1650 Hz    [5] */
    (&_fspecs_8),    /* 1700 .. 2000 Hz, centre 1850 Hz    [6] */
    (&_fspecs_9),    /* 1500 .. 2700 Hz, centre 2100 Hz    [7] */
  };

/* Lpwpass filter coeffs constructed by:
   mkfilter -Bu -Lp -o 2 -a (A1)
   where A1 = (bps/2) / SAMPLERATE */

static fspec *lpfspecs[] =
  { (&_fspecs_10),     /*  37.5 Hz   [0] */
    (&_fspecs_11),     /* 150   Hz   [1] */
    (&_fspecs_12),     /* 600   Hz   [2] */
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

