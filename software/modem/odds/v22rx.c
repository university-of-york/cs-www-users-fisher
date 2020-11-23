/* Modem for MIPS   AJF	  January 1995
   V.22 receive loop */

#include "modem.h"
#include "/usr/fisher/mipslib/audio.h"

#include <stdio.h>

#define NPOLES	   4
#define DBLEN	   3000
#define TONETHRESH 1e16
#define THRESHFAC  3.16227766e-3    /* (2 / sqrt(10)) / 200.0 */

extern struct audio *rx_audio;	/* from main */
extern float sinetab[];		/* from main */

extern double atan2(), hypot();

static struct complex debugbuf[DBLEN];
static uint sineptr, scrambler;
static uchar quadrant, quadbit, bitcount;
static float xvals[NPOLES+1], yvals[NPOLES+1];
static float thetatab[4][4];
static float threshold;

static uchar quadbittab[4][4] =
  { { 0xb, 0x9, 0x6, 0x7 },
    { 0xa, 0x8, 0x4, 0x5 },
    { 0xd, 0xc, 0x0, 0x2 },
    { 0xf, 0xe, 0x1, 0x3 },
  };

static struct complex czero = { 0.0, 0.0 };

/* Be Bp 2100 .. 2700 Hz */
static float ycoeffs[] = { -0.7613816112, 2.6475472915, -4.0453761161, 3.0348338301 };

forward struct complex nextbaud(), cplus(), rotate();
forward uchar nextquadbit();


global initrx_v22()
  { int i; uchar qb; struct complex zval, totz;
    makethetatab();
    for (i=0; i < NPOLES+1; i++) xvals[i] = yvals[i] = 0.0;
    sineptr = 0;
    for (i=0; i < 60; i++) nextbaud();	/* allow filter to settle */
    /* wait for end of answer tone, then skip silence */
    do zval = nextbaud(); while (zval.re*zval.re + zval.im*zval.im >= TONETHRESH);
    fprintf(stderr, "??? End of answer tone\r\n");
    for (i=0; i < 60; i++) nextbaud();
    totz = czero;
    /* sample for 300 bauds, establish sync */
    for (i=0; i < 300; i++)
      { zval = rotate(nextbaud(), i);			    /* phase rotates c/w by 90 deg per baud */
	if (i > 100) totz = cplus(totz, zval);		    /* accumulate total once phase has settled */
	adjust(zval, thetatab[3][2]);			    /* home in on sync point */
      }
    threshold = THRESHFAC * hypot(totz.im, totz.re);
    /* we're now sync'd to both baud timing and cycle phase */
    quadrant = 0xd;
    do (qb = nextquadbit); while (qb == 0xd);
    bitcount = 0; scrambler = 0;
    fprintf(stderr, "??? synced\r\n");
  }

static makethetatab()
  { int nx, ny;
    for (nx=0; nx < 4; nx++)
      for (ny=0; ny < 4; ny++)
	thetatab[nx][ny] = atan2(ny - 1.5, nx - 1.5);
  }

static char hextab[16] = "0123456789abcdef";

global int geta_v22()	/* asynchronous char input */
  { int i;
    for (i=0; ; i++)
      { bool bit = getbit();
	putc('0'+bit, stderr);
	if (i%128 == 127) { putc('\r', stderr); putc('\n', stderr); }
      }
  }

#ifdef NOTDEF
  { bool bit; int i; uchar n;
    do bit = getbit(); while (bit);
    for (i=0; i < 8; i++)
      { bit = getbit();
	n = (n >> 1) | (bit << 7);
      }
    return n;
  }
#endif

static bool getbit()
  { bool bit, b14, b17;
    if (bitcount == 0) { quadbit = nextquadbit(); bitcount = 2; }	/* was 4 */
    bit = (quadbit >> 3) & 1;
    quadbit <<= 1; bitcount--;
    b14 = (scrambler >> 3) & 1;
    b17 = scrambler & 1;
    scrambler >>= 1;
    if (bit) scrambler |= 0x10000;
    return bit ^ b14 ^ b17;
  }

static uchar phinctab[4] = { 0x4, 0x0, 0x8, 0xc };

static uchar nextquadbit()
  { struct complex zval; int nx, ny; uchar n, qb;
    zval = nextbaud();
    nx = discretize(zval.re); ny = discretize(zval.im);
    adjust(zval, thetatab[nx][ny]);
    n = quadbittab[nx][ny];
    qb = phinctab[(n >> 2) - (quadrant >> 2)] | (n & 3);
    quadrant = n;   /* identifies absolute quadrant */
    return qb;
  }

static int discretize(x) float x;
  { int n = (int) ((x + 2*threshold) / threshold);
    if (n < 0) n = 0; if (n > 3) n = 3;
    return n;
  }

static adjust(zval, ideal_theta) struct complex zval; float ideal_theta;
  { float theta = atan2(zval.im, zval.re) - ideal_theta;    /* angle to rotate constellation to bring into sync */
    if (theta >= PI) theta -= TWOPI;
    if (theta < -PI) theta += TWOPI;
    sineptr += (int) (theta * (0.5*SINELEN/TWOPI));	    /* roatate constellation (0.5 is damping factor) */
  }

static struct complex nextbaud()
  { struct complex zval;
    zval = czero;
    until ((int) (sineptr - 4*SINELEN) >= 0)	/* careful with signed comparison */
      { int ip, qp;
	nextsample();
	ip = sineptr & (SINELEN-1);
	qp = (sineptr + SINELEN/4) & (SINELEN-1);
	/* integrate & dump */
	zval.re += (sinetab[ip] * yvals[NPOLES]);
	zval.im += (sinetab[qp] * yvals[NPOLES]);
	sineptr += ((2400 * SINELEN) / SAMPLERATE);
      }
    sineptr -= 4*SINELEN;
    return zval;
  }

static nextsample()
  { if (NumFilled(rx_audio) == 0) WaitAudio(rx_audio, RBLEN/2); /* wait until half-full or more */
    shiftdown(xvals, NPOLES);
    xvals[NPOLES] = (float) ReadMono(rx_audio);
    filterstep(xvals, yvals, ycoeffs);
  }

static filterstep(xv, yv, yco) float xv[], yv[], yco[];
  { shiftdown(yv, NPOLES);
    yv[4] = xv[0] + xv[4] - 2.0 * xv[2]
	  + (yco[0] * yv[0]) + (yco[1] * yv[1]) + (yco[2] * yv[2]) + (yco[3] * yv[3]);
  }

static struct complex cplus(z1, z2) struct complex z1, z2;
  { struct complex z;
    z.re = z1.re + z2.re;
    z.im = z1.im + z2.im;
    return z;
  }

static struct complex rotate(z, n) struct complex z; uint n;
  { /* rotate z by n quadrants anticlockwise */
    struct complex z1;
    switch (n & 3)
      { case 0:	 z1.re = +z.re; z1.im = +z.im; break;
	case 1:	 z1.re = -z.im; z1.im = +z.re; break;
	case 2:	 z1.re = -z.re; z1.im = -z.im; break;
	case 3:	 z1.re = +z.im; z1.im = -z.re; break;
      }
    return z1;
  }

static debugit(buf) struct complex buf[];
  { FILE *fi; int i, j;
    fi = fopen("debug.dat", "w");
    if (fi == NULL) giveup("debug fopen failed");
    for (i=0; i < 10; i++)
      { if (i > 0) fprintf(fi, ".bp\n");
	fprintf(fi, ".sp 0.5i\n.G1 8i\n");
	for (j=0; j < 300; j++)
	  { int n = 300*i + j;
	    fprintf(fi, "%g %g\n", buf[n].re, buf[n].im);
	  }
	divlines(fi);
	fprintf(fi, ".G2\n");
      }
    fclose(fi); fprintf(stderr, "done\r\n");
    for (;;) ;
  }

static divlines(fi) FILE *fi;
  { divline(fi, -threshold, -2e9, -threshold, +2e9);
    divline(fi, 0.0, -2e9, 0.0, +2e9);
    divline(fi, +threshold, -2e9, +threshold, +2e9);
    divline(fi, -2e9, -threshold, +2e9, -threshold);
    divline(fi, -2e9, 0.0, +2e9, 0.0);
    divline(fi, -2e9, +threshold, +2e9, +threshold);
  }

static divline(fi, x1, y1, x2, y2) FILE *fi; double x1, y1, x2, y2;
  { fprintf(fi, "line from %g,%g to %g,%g\n", x1, y1, x2, y2);
  }

