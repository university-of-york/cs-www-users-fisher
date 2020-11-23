/* Modem for MIPS   AJF	  August 1996
   V.32 round-trip routines */

#include <stdio.h>
#include <fishaudio.h>
#include "complex.h"
#include "modem.h"
#include "filters.h"
#include "debug.h"

#define SCALE	    (0.10 * MAXAMPL)
#define TOPBIT	    (1 << 31)

static fspec *acres_fs	= mkfilter("-Re 200 -Bp -a 0.125");          /* resonator at 3000 Hz Q=200    */
static fspec *notch1_fs = mkfilter("-Re 10 -Bs -a 0.025");           /* notch at 600 Hz Q=10          */
static fspec *notch2_fs = mkfilter("-Re 10 -Bs -a 0.075");           /* notch at 1800 Hz Q=10         */
static fspec *brlpf_fs	= mkfilter("-Bu -Lp -o 4 -a 0.1");           /* low-pass at 2400 Hz           */

static filter *acres, *notch1, *notch2, *brlpf;
static uint sineptr, timenow;
static f2_debugger *debug;

static uint lookforphaseshift(int);
static float compute_z(int);


global uint roundtrip()
  { /* estimate round-trip delay NT */
    sineptr = timenow = 0;
    acres = new filter(acres_fs);		/* resonator at 3000 Hz */
    notch1 = new filter(notch1_fs);		/* notch filter at 600 Hz */
    notch2 = new filter(notch2_fs);		/* notch filter at 1800 Hz */
    brlpf = new filter(brlpf_fs);		/* low-pass filter at 2400 Hz */
    // debug = new f2_debugger(120000);
    uint pc1 = lookforphaseshift(+1);		/* send AA, rcv AC, wait for CA */
    uint pc2 = lookforphaseshift(-1);		/* send CC, rcv CA, wait for AC */
    uint nt = pc2-pc1;				/* time in samples, includes 64*BAUDLEN turn-round delay */
    float ms = (float) (int) (nt - 64*BAUDLEN) / (float) SAMPLERATE * 1000.0;	/* can be negative (just) */
    char nts[32]; sprintf(nts, "%.1f", ms); infomsg("NT=%d (%sms)", nt, nts);
    delete acres; delete notch1; delete notch2; delete brlpf;
    // debug -> write("debugnt.grap");
    return nt;
  }

static uint lookforphaseshift(int sgn)
  { int npos = 0; float z;
    while (npos < 200)
      { z = compute_z(sgn);
	if (z > 0.0) npos++; else npos = 0;
      }
    until (z < 0.0) z = compute_z(sgn);
    return timenow;
  }

static float compute_z(int sgn)
  { /* transmit next sample of AA or CC */
    float x = SCALE * sinetab[sineptr & (SINELEN-1)];
    sineptr += PHINC;
    if (sgn < 0) x = -x;
    if (x > MAXAMPL || x < -MAXAMPL) giveup("Bug! out of range (V.32 train): %08x", (int) x);
    Audio -> write((int) x);
    /* deal with next received sample */
    x = (float) Audio -> read();
    x = notch1 -> fstep(x);	      /* remove 1800 Hz */
    x = notch2 -> fstep(x);	      /* remove 600 Hz	*/
    if (sgn < 0) x = -x;
    float y = acres -> fstep(x);      /* resonate at 3000 Hz */
    float xy = brlpf -> fstep(y*x);   /* output goes -ve on phase shift */
    // debug -> insert(xy);
    if (++timenow >= 20 * SAMPLERATE) giveup("remote modem is not responding"); /* 20 sec timeout */
    return xy;
  }

