/* Modem for MIPS   AJF	  December 1995
   V.22 receive routines
*/

#include <stdio.h>
#include <co.h>
#include <fishaudio.h>
#include "complex.h"
#include "modem.h"

#define LPOLES	    4			/* num. low-pass poles	*/
#define BPOLES	    8			/* num. bandpass poles	*/
#define BAUDLEN	    40			/* num. samples per baud (600 bd/sec)  */

#define W0	    1.073741824e8	/* PLL params for fc = 600 Hz	 */
#define MAXUF	    1789569.707		/* max vco offset is 10 Hz	 */
#define ALPHA	    1989552.007		/* for zeta=1/sqrt(2), fn = 0.2 Hz */
#define BETA	    1987712.179

#ifdef NOTDEF
#define ALPHA	    397616.0289		/* for zeta=1/sqrt(2), fn = 1.0 Hz */
#define BETA	    397542.4358
#endif

static float hbp_yco[] =
  { -0.6630104844, +4.5251046761, -14.5157382898, +28.2019444330,     /* Bu Bp 2100 .. 2700 Hz */
    -36.1502905020, +31.2545072472, -17.8281695379, +6.1592104879
  };

static float lbp_yco[] =
  { -0.4382651423, +3.7218610775, -14.0055655874, +30.4941815738,     /* Bu Bp	600 .. 1800 Hz */
    -42.0101179091,+37.4947423167, -21.1711762051, +6.9143198339,
  };

struct info
  { ushort cf;		/* carrier frequency		    */
    float *yco;		/* front-end bandpass filter coeffs */
  };

static info info[] =
  { { 2400, hbp_yco },	/* V22o */
    { 1200, lbp_yco },	/* V22a */
  };

static float lp_yco[]  = { -0.6630104844, +2.9240526562, -4.8512758825, +3.5897338871 };    /* Bu Lp 600 Hz */

static float xvals[BPOLES+1], yvals[BPOLES+1], bp_yco[BPOLES];
static complex cxvals[LPOLES+1], cyvals[LPOLES+1];
static uint phinc, scrambler;
static Corout *dibitco, *equalco;

static void storedebug(complex), printdebug();
#define DEBUGLEN 600
static complex debugbuf[DEBUGLEN];
static volatile int debugptr;

static void init(int);
static int gasync(), gsync(), gbit();
static void dibitloop(), equalizer();

static void lp_filterstep(complex[], complex[]);
static void bp_filterstep(float[], float[]);

global rxhandler v22_rxhandler = { init, gasync, gsync, gbit };


static void init(int mode)
  { unless (mode == 4 || mode == 5) giveup("Bug! bad mode %d in V.22 rx init", mode);
    memcpy(bp_yco, info[mode-4].yco, BPOLES * sizeof(float));
    phinc = (info[mode-4].cf * SINELEN) / SAMPLERATE;
    int i;
    for (i=0; i < BPOLES+1; i++) xvals[i] = yvals[i] = 0.0;
    for (i=0; i < LPOLES+1; i++) cxvals[i] = cyvals[i] = (complex) 0.0;
    rx_audio.flush();	/* empty Rx ring buffer */
    scrambler = 0; debugptr = -1;
    dibitco = startco(dibitloop);
    equalco = startco(equalizer);
    int n = 0;
    until (n >= 64)
      { if (gbit()) n++; else n = 0; }
    infomsg("V.22 carrier");
  }

static int gasync()
  { bool bit; uchar n;
    do bit = gbit(); while (bit);
    for (int i=0; i < 8; i++)
      { bit = gbit();
	n = (n >> 1) | (bit << 7);
      }
    static char debug_prevchar = 0;
    if (debug_prevchar == '*')
      { switch (n)
	  { case 'd':
		fprintf(stderr, "?? Collecting debug\r\n");
		debugptr = 0;
		break;
	  }
      }
    debug_prevchar = n;
    return n;
  }

static int gsync()
  { giveup("Bug! sync mode not supported in V.22");
  }

static int gbit()
  { int bit = icallco(dibitco);
    int b14 = (scrambler >> 3) & 1;
    int b17 = scrambler & 1;
    scrambler >>= 1;
    if (bit) scrambler |= 0x10000;
    return bit ^ b14 ^ b17;
  }

static complex opttab[4] = { { 0,1 }, { 1,0 }, { -1,0 }, { 0,-1 } };

static void dibitloop()
  { complex prevz = (complex) 1.0;
    uint sineptr = 0, phi = 0; uint prevphi;
    float sx = 0.0, ud = 0.0, uf = 0.0; float prevsx, u1, prevud;
    for (;;)
      { /* read next sample */
	shiftdown(xvals, BPOLES);
	xvals[BPOLES] = (float) rx_audio.read();
	bp_filterstep(xvals, yvals);	    /* front-end bp filter */
	/* translate to baseband */
	shiftdown(cxvals, LPOLES);
	cxvals[LPOLES].re = yvals[BPOLES] * sinetab[sineptr & (SINELEN-1)];
	cxvals[LPOLES].im = yvals[BPOLES] * sinetab[(sineptr + SINELEN/4) & (SINELEN-1)];
	lp_filterstep(cxvals, cyvals);	    /* low-pass filter at baud rate */
	/* pll to recover baud clock */
	prevsx = sx; sx = power(cyvals[LPOLES]);
	u1 = sx - prevsx;
	prevud = ud;
	ud = (phi & 0x80000000) ? +u1 : -u1;
	uf += (ALPHA*ud) - (BETA*prevud);
	if (uf > MAXUF) uf = MAXUF;
	if (uf < -MAXUF) uf = -MAXUF;
	prevphi = phi;
	phi += (int) (W0 + uf);	    /* KO = 1 */
	if (~prevphi & phi & 0x80000000)    /* low-to-high ZC is sampling point */
	  { complex z = cyvals[LPOLES];
	    complex dz = z / prevz;	    /* delta phase */
	    if (debugptr >= 0) storedebug(dz);
	    bool b1 = (dz.re < -dz.im), b2 = (dz.re > +dz.im);
	    /*** icallco(_invokingco, b1);	 /* return 2 bits of dibit */
	    /*** icallco(_invokingco, b2); */
	    complex optdz = opttab[(b1 << 1) | b2];
	    fcallco(equalco, uf);
	    /*** fcallco(equalco, power(dz - optdz)); */
	    icallco(&_mainco, b1);
	    icallco(&_mainco, b2);
	    prevz = z;
	  }
	sineptr += phinc;
      }
  }

static void lp_filterstep(complex xv[], complex yv[])
  { shiftdown(yv, 4);
    yv[4] = (xv[0] + xv[4]) + 4.0 * (xv[1] + xv[3]) + 6.0 * xv[2]
	  + (lp_yco[0] * yv[0]) + (lp_yco[1] * yv[1]) + (lp_yco[2] * yv[2]) + (lp_yco[3] * yv[3]);
  }

static void bp_filterstep(float xv[], float yv[])
  { shiftdown(yv, 8);
    yv[8] = (xv[0] + xv[8]) - 4.0 * (xv[2] + xv[6]) + 6.0 * xv[4]
	  + (bp_yco[0] * yv[0]) + (bp_yco[1] * yv[1]) + (bp_yco[2] * yv[2]) + (bp_yco[3] * yv[3])
	  + (bp_yco[4] * yv[4]) + (bp_yco[5] * yv[5]) + (bp_yco[6] * yv[6]) + (bp_yco[7] * yv[7]);
  }

static float geterror()
  { float error = 0.0;
    for (int i=0; i < 1000; i++) error += fcallco(dibitco);
    return error;
  }

static void equalizer()
  { for (;;)
      { float uf = geterror();
	fprintf(stderr, "??? uf*1000 = %10.5f\r\n", uf);
      }
  }

static void storedebug(complex z)
  { debugbuf[debugptr++] = z;
    if (debugptr >= DEBUGLEN)
      { printdebug();
	debugptr = -1;
      }
  }

static void printdebug()
  { int cpid = fork();
    if (cpid < 0) giveup("fork failed");
    if (cpid == 0)
      { /* child */
	fprintf(stderr, "?? printing debug\r\n");
	FILE *fi = fopen("debug.grap", "w");
	if (fi == NULL) giveup("debug fopen failed");
	for (int i=0; i < DEBUGLEN; i += 600)
	  { if (i > 0) fprintf(fi, ".bp\n");
	    fprintf(fi, ".sp 0.5i\n.G1 8i\n");
	    for (int j=0; j < 600; j++)
	      { complex z = debugbuf[i+j];
		fprintf(fi, "%g %g\n", z.re, z.im);
	      }
	    fprintf(fi, ".G2\n");
	  }
	fclose(fi);
	fprintf(stderr, "?? done\r\n");
	_exit(0);
      }
  }

