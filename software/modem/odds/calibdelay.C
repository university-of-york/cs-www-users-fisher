#line 1 "calibdelay.F"
/* Modem for MIPS   AJF	  August 1996
   Calibrate Tx-to-Rx hardware delay */

#include <stdio.h>
#include <fishaudio.h>
#include "complex.h"
#include "modem.h"
#include "filters.h"
#line 1 "<<generated_fspecs>>"

static float _fstepf_1(filter *fi, float x)
  { /* /usr/fisher/mipsbin/mkfilter -Re 200 -Bp -a 0.125 -l */
    float *v = fi -> v;
    v[0] = v[1]; v[1] = v[2]; 
    v[2] =    (  1.9596451359e-03 * x)
            + ( -0.9960807097 * v[0]) + (  1.4114422056 * v[1]);
    return    (v[2] - v[0]);
  }

static fspec _fspecs_1 = { 2, 2, _fstepf_1 };

static float _fstepf_2(filter *fi, float x)
  { /* /usr/fisher/mipsbin/mkfilter -Re 10 -Bs -a 0.0875 -l */
    float *v = fi -> v;
    v[0] = v[1]; v[1] = v[2]; 
    v[2] =    (  9.7423126515e-01 * x)
            + ( -0.9465060925 * v[0]) + (  1.6590424145 * v[1]);
    return    (v[0] + v[2]) -   1.7052803287 * v[1];
  }

static fspec _fspecs_2 = { 2, 2, _fstepf_2 };

static float _fstepf_3(filter *fi, float x)
  { /* /usr/fisher/mipsbin/mkfilter -Bu -Lp -o 4 -a 0.1 -l */
    float *v = fi -> v;
    v[0] = v[1]; v[1] = v[2]; v[2] = v[3]; v[3] = v[4]; 
    v[4] =    (  4.8243433576e-03 * x)
            + ( -0.1873794924 * v[0]) + (  1.0546654059 * v[1])
            + ( -2.3139884144 * v[2]) + (  2.3695130072 * v[3]);
    return    (v[0] + v[4]) + 4 * (v[1] + v[3]) + 6 * v[2];
  }

static fspec _fspecs_3 = { 4, 4, _fstepf_3 };

#line 9 "calibdelay.F"
#include "debug.h"

#define SCALE	    (0.10 * MAXAMPL)
#define TOPBIT	    (1 << 31)
#define XXPHINC	    ((3000 * SINELEN) / SAMPLERATE)	/* 3000 Hz probe tone */

static fspec *res_fs	= (&_fspecs_1);          /* resonator at 3000 Hz Q=200 */
static fspec *notch_fs	= (&_fspecs_2);          /* notch at 2100 Hz Q=10      */
static fspec *brlpf_fs	= (&_fspecs_3);           /* low-pass at 2400 Hz        */

static filter *res, *notch, *brlpf;
static uint sineptr, timenow;
static f2_debugger *debug;

static float compute_z(int);


global uint calibdelay()
  { /* estimate Tx-to-Rx hardware delay */
    sineptr = timenow = 0;
    res = new filter(res_fs);			/* resonator at 3000 Hz */
    notch = new filter(notch_fs);		/* notch at 2100 Hz */
    brlpf = new filter(brlpf_fs);		/* low-pass filter at 2400 Hz */
    // debug = new f2_debugger(4000);
    int npos = 0; float z;
    while (npos < 200)
      { z = compute_z(+1);			/* send XX, rcv XX */
	if (z > 0.0) npos++; else npos = 0;
      }
    uint pc1 = timenow;
    while (z > 0.0) z = compute_z(-1);		/* send YY, rcv XX, wait for YY */
    uint pc2 = timenow;
    uint nt = pc2-pc1;				/* time in samples, includes 64*BAUDLEN turn-round delay */
    infomsg("TD=%d", nt);
    if (nt < 64*BAUDLEN) giveup("delay calibration failed");
    delete res; delete notch; delete brlpf;
    // debug -> write("debugct.grap");
    return nt;
  }

static float compute_z(int sgn)
  { /* transmit next sample of XX or YY */
    float x = SCALE * sinetab[sineptr & (SINELEN-1)];
    sineptr += XXPHINC;
    if (sgn < 0) x = -x;
    if (x > MAXAMPL || x < -MAXAMPL) giveup("Bug! out of range (V.32 train): %08x", (int) x);
    Audio -> write((int) x);
    /* deal with next received sample */
    x = (float) Audio -> read();
    x = notch -> fstep(x);	      /* remove 2100 Hz (ANS) */
    float y = res -> fstep(x);	      /* resonate at 1100 Hz */
    float xy = brlpf -> fstep(y*x);   /* output goes -ve on phase shift */
    // debug -> insert(x, xy);
    if (++timenow >= 20 * SAMPLERATE) giveup("remote modem is not responding"); /* 20 sec timeout */
    return xy;
  }

