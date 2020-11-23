/* genplot - use "-l" output from mkfilter to generate .gif mag/phase plots
   A.J. Fisher, University of York   <fisher@minster.york.ac.uk>
   April 1995 */

/* Uses the "gd" gif manipulation library, originally from
   Quest Protein Database Center, and now available from:
   http://www.boutell.com/gd
*/

#include <stdio.h>
#include <math.h>
#include <string.h>
#include <new.h>

extern "C" {
   #include <gd.h>
   #include <gdfonts.h>
};

#include "mkfilter.h"
#include "complex.h"

#define MINSTEPS 100		/* min for locus graph	*/
#define MAXSTEPS 100000		/* max for locus graph	*/
#define FRSTEPS	 100		/* for freq response	*/

#define XYPIXELS 400
#define BORDER	 40
#define TICKLEN	 5
#define PHEPS	 1e-20

union word	/* for error msgs */
  { int i; void *p;
    word(int ix)   { i = ix; }
    word(void *px) { p = px; }
  };

global char *progname;

static enum { unknown, locus, phmag, impulse, step } gtype;    /* type of graph */
static char *outfn;
static double alpha1, alpha2, logmin;
static bool logopt, dflag;
static double xcoeffs[MAXPZ+1], ycoeffs[MAXPZ+1];
static int npoles, nzeros, nirsteps;

static void readcmdline(char*[]);
static double getfarg(char*);
static int getiarg(char*);
static void usage(), drawgraph();
static void computefr(complex[], int), computeir(double[], int, bool);
static void locusgraph(char*, complex[], int, double&);
static int xmap_lc(double), ymap_lc(double);
static void frgraph(char*, complex[], int), irgraph(char*, double[], int);
static void pgraph(gdImagePtr, double[], int, int, bool);
static void draw_xtick(gdImagePtr, double, double, int, int, char*, double = 0.0);
static void draw_ytick(gdImagePtr, double, double, int, int, char*, double = 0.0);
static int xmap_pm(double), ymap_pm(double);
static char *choosefmt(double, double);
static void newhandler();
static void giveup(char*, word = 0);


global void main(int argc, char *argv[])
  { set_new_handler(newhandler);
    readcmdline(argv);
    char junkv[MAXSTRING+1]; double junkd;
    readdata(junkv, junkd, nzeros, xcoeffs, npoles, ycoeffs);
    drawgraph();
    exit(0);
  }

static void readcmdline(char *argv[])
  { outfn = NULL; gtype = unknown; logopt = dflag = false;
    alpha1 = 0.0; alpha2 = 0.5;
    int ap = 0;
    progname = (argv[ap] != NULL) ? argv[ap++] : "???";
    until (argv[ap] == NULL)
      { char *s = argv[ap++];
	if (seq(s,"-a"))
	  { alpha1 = getfarg(argv[ap++]);
	    alpha2 = getfarg(argv[ap++]);
	    unless (alpha2 > alpha1) giveup("Invalid values for -a: a1 must be .lt. a2");
	  }
	else if (gtype == unknown && seq(s,"-l")) gtype = locus;
	else if (gtype == unknown && (seq(s,"-i") || seq(s,"-s")))
	  { gtype = (s[1] == 's') ? step : impulse;
	    nirsteps = getiarg(argv[ap++]);
	    if (nirsteps < 0 || nirsteps > MAXSTEPS) giveup("Invalid value for num steps: limits are 0 .. %d", MAXSTEPS);
	  }
	else if (seq(s,"-log"))
	  { logmin = getfarg(argv[ap++]);
	    if (logmin >= 0.0) giveup("Invalid value for log min (%s): must be .lt. 0", argv[ap-1]);
	    logopt = true;
	  }
	else if (seq(s,"-d")) dflag = true;
	else
	  { if (outfn != NULL) usage();
	    outfn = s;
	  }
      }
    if (outfn == NULL) usage();
    if (gtype == unknown) gtype = phmag;    /* default */
  }

static double getfarg(char *s)
  { if (s == NULL) usage();
   return atof(s);
  }

static int getiarg(char *s)
  { if (s == NULL) usage();
    return atoi(s);
  }

static void usage()
  { fprintf(stderr, "Genplot V.%s from <fisher@minster.york.ac.uk>\n", VERSION);
    fprintf(stderr, "Usage: genplot [-i <n> | -s <n> -a <a1> <a2> | -l] [-log <min>] [-d] out.gif\n");
    exit(1);
  }

static void drawgraph()
  { switch (gtype)
      { default:
	    giveup("bug (gtype)");

	case locus:
	  { int nsteps = MINSTEPS;
	    bool ok = false; double stepl;
	    until (ok || nsteps > MAXSTEPS)
	      { complex *fr = new complex[nsteps+1];
		computefr(fr, nsteps);
		locusgraph(outfn, fr, nsteps, stepl);
		if (stepl < 0.1) ok = true;
		nsteps *= 10;
		delete fr;
	      }
	    unless (ok) fprintf(stderr, "genplot: warning: step length %g in locus plot\n", stepl);
	    break;
	  }

	case phmag:
	  { complex *fr = new complex[FRSTEPS+1];
	    computefr(fr, FRSTEPS);
	    frgraph(outfn, fr, FRSTEPS);
	    delete fr;
	    break;
	  }

	case impulse:
	  { double *ir = new double[nirsteps+1];
	    computeir(ir, nirsteps, false);
	    irgraph(outfn, ir, nirsteps);
	    delete ir;
	    break;
	  }

	case step:
	  { double *ir = new double[nirsteps+1];
	    computeir(ir, nirsteps, true);
	    irgraph(outfn, ir, nirsteps);
	    delete ir;
	    break;
	  }
      }
  }

static void computefr(complex fr[], int nsteps)
  { complex topcos[MAXPZ+1], botcos[MAXPZ+1]; int i;
    for (i=0; i <= nzeros; i++) topcos[i] = complex(xcoeffs[i]);
    for (i=0; i <= npoles; i++) botcos[i] = complex(-ycoeffs[i]);
    double maxmag = 0.0;
    for (i = 0; i <= nsteps; i++)
      { double a = (double) i / (double) nsteps;
	double theta = TWOPI * (alpha1 + a * (alpha2-alpha1));
	complex z = expj(theta);
	fr[i] = evaluate(topcos, nzeros, botcos, npoles, z);
	double mag = hypot(fr[i]);
	if (mag > maxmag) maxmag = mag;
	if (dflag) fr[i] = fr[i] * expj(0.5 * nzeros * theta);
      }
    if (maxmag > 0.0)
      { for (i=0; i <= nsteps; i++) fr[i] /= maxmag;  /* scale so max mag is 1.0 */
      }
  }

inline void shiftdown(double v[], int n)
  { memcpy(&v[0], &v[1], n * sizeof(double));
  }

static void computeir(double ir[], int nsteps, bool stp)
  { double xv[MAXPZ+1], yv[MAXPZ+1];
    double maxmag = 0.0; int i, j;
    for (i=0; i <= nzeros; i++) xv[i] = 0.0;
    for (i=0; i <= npoles; i++) yv[i] = 0.0;
    for (i=0; i <= nsteps; i++)
      { shiftdown(xv, nzeros);
	xv[nzeros] = (i == 0 || stp) ? 1.0 : 0.0;	/* impulse or step */
	shiftdown(yv, npoles);
	yv[npoles] = 0.0;
	for (j=0; j <= nzeros; j++) yv[npoles] += (xcoeffs[j] * xv[j]);
	for (j=0; j < npoles; j++) yv[npoles] += (ycoeffs[j] * yv[j]);
	ir[i] = yv[npoles];
	double mag = fabs(ir[i]);
	if (mag > maxmag) maxmag = mag;
      }
    if (maxmag > 0.0)
      { for (i=0; i <= nsteps; i++) ir[i] /= maxmag;  /* scale to range +-1 */
      }
  }

static void locusgraph(char *fn, complex fr[], int nsteps, double &stepl)
  { stepl = 0.0;
    gdImagePtr im = gdImageCreate(XYPIXELS + 2*BORDER, XYPIXELS + 2*BORDER);
    gdImageColorAllocate(im, 255, 255, 255); /* white background */
    int red = gdImageColorAllocate(im, 255, 0, 0);
    int black = gdImageColorAllocate(im, 0, 0, 0);
    gdImageLine(im, xmap_lc(-1.0), ymap_lc(0.0), xmap_lc(+1.0), ymap_lc(0.0), black); /* X axis	 */
    gdImageLine(im, xmap_lc(0.0), ymap_lc(-1.0), xmap_lc(0.0), ymap_lc(+1.0), black); /* Y axis	 */
    for (int i = 1; i <= nsteps; i++)
      { complex z0 = fr[i-1], z1 = fr[i];
	gdImageLine(im, xmap_lc(z0.re), ymap_lc(z0.im), xmap_lc(z1.re), ymap_lc(z1.im), red);
	double sl = hypot(z1 - z0);
	if (sl > stepl) stepl = sl;
      }
    FILE *fi = fopen(fn, "wb");
    if (fi == NULL) giveup("can't create %s", fn);
    gdImageGif(im, fi);
    fclose(fi);
    gdImageDestroy(im);
  }

static int xmap_lc(double x)
  { return (int) (XYPIXELS * 0.5 * (x + 1.0)) + BORDER;
  }

static int ymap_lc(double y)
  { return (int) (XYPIXELS * 0.5 * (1.0 - y)) + BORDER;
  }

static void frgraph(char *fn, complex fr[], int nsteps)
  { double *ymvals = new double[nsteps+1], *ypvals = new double[nsteps+1]; int i; char *fmt;
    gdImagePtr im = gdImageCreate(XYPIXELS + 2*BORDER, XYPIXELS + 2*BORDER);
    gdImageColorAllocate(im, 255, 255, 255); /* white background */
    int red = gdImageColorAllocate(im, 255, 0, 0);
    int blue = gdImageColorAllocate(im, 0, 0, 255);
    int black = gdImageColorAllocate(im, 0, 0, 0);
    if (dflag)
      { char str[MAXSTRING+1];
	sprintf(str, "FILTER DELAY: %g SAMPLES.", 0.5 * nzeros);
	gdImageString(im, gdFontSmall, 20, 10, str, black);
      }
    gdImageLine(im, xmap_pm(0.0), ymap_pm(0.0), xmap_pm(1.0), ymap_pm(0.0), black); /* X axis	    */
    gdImageLine(im, xmap_pm(0.0), ymap_pm(0.0), xmap_pm(0.0), ymap_pm(1.0), black); /* left Y axis  */
    gdImageLine(im, xmap_pm(1.0), ymap_pm(0.0), xmap_pm(1.0), ymap_pm(1.0), black); /* right Y axis */
    for (i=0; i <= nsteps; i++)
      { ymvals[i] = hypot(fr[i]);
	ypvals[i] = (ymvals[i] > PHEPS) ? atan2(fr[i]) / TWOPI + 0.5 :
		    -1.0; /* out-of-range value will not be plotted */
      }
    /* ymvals, ypvals in range 0 .. 1 */
    if (logopt)
      { for (i=0; i <= nsteps; i++)
	  ymvals[i] = (ymvals[i] > 0.0) ? (20.0 * log10(ymvals[i]) - logmin) / (-logmin) :
					  -1.0; /* out-of-range value will not be plotted */
      }
    pgraph(im, ymvals, nsteps, red, false);    /* mag graph   */
    pgraph(im, ypvals, nsteps, blue, true);    /* phase graph */
    if (logopt)
      { fmt = choosefmt(logmin, 0.0);
	for (i = 0; i <= 10; i++) draw_ytick(im, 0.0, 0.1*i, red, -TICKLEN, fmt, (1.0 - 0.1*i) * logmin);
      }
    else
      { for (i = 0; i <= 10; i++) draw_ytick(im, 0.0, 0.1*i, red, -TICKLEN, "%3.1f", 0.1*i);
      }
    draw_ytick(im, 1.0, 0.0, blue, TICKLEN, "-pi");
    draw_ytick(im, 1.0, 0.5, blue, TICKLEN, "0");
    draw_ytick(im, 1.0, 1.0, blue, TICKLEN, "+pi");
    for (i = 1; i <= 9; i++) draw_ytick(im, 1.0, 0.1*i, blue, TICKLEN, "", 0.2*(i-5));
    fmt = choosefmt(alpha1, alpha2);
    for (i = 0; i <= 10; i++)
      { double a = (double) i / 10.0;
	double x = alpha1 + a * (alpha2-alpha1);
	draw_xtick(im, 0.1*i, 0.0, black, TICKLEN, fmt, x);
      }
    FILE *fi = fopen(fn, "wb");
    if (fi == NULL) giveup("can't create %s", fn);
    gdImageGif(im, fi);
    fclose(fi);
    gdImageDestroy(im);
    delete ymvals; delete ypvals;
  }

static void irgraph(char *fn, double ir[], int nsteps)
  { double *ymvals = new double[nsteps+1]; int i;
    gdImagePtr im = gdImageCreate(XYPIXELS + 2*BORDER, XYPIXELS + 2*BORDER);
    gdImageColorAllocate(im, 255, 255, 255); /* white background */
    int red = gdImageColorAllocate(im, 255, 0, 0);
    int black = gdImageColorAllocate(im, 0, 0, 0);
    gdImageLine(im, xmap_pm(0.0), ymap_pm(0.0), xmap_pm(1.0), ymap_pm(0.0), black); /* X axis	    */
    gdImageLine(im, xmap_pm(0.0), ymap_pm(0.0), xmap_pm(0.0), ymap_pm(1.0), black); /* left Y axis  */
    gdImageLine(im, xmap_pm(1.0), ymap_pm(0.0), xmap_pm(1.0), ymap_pm(1.0), black); /* right Y axis */
    for (i=0; i <= nsteps; i++) ymvals[i] = 0.5 * (ir[i] + 1.0);
    /* ymvals in range 0 .. 1 */
    pgraph(im, ymvals, nsteps, red, false);
    for (i=0; i <= 10; i++) draw_ytick(im, 0.0, 0.1*i, red, -TICKLEN, "%3.1f", 0.2*(i-5));
    for (i=0; i <= 10; i++)
      { double x = (double) nsteps * (double) i / 10.0;
	draw_xtick(im, 0.1*i, 0.0, black, TICKLEN, "%g", x);
      }
    FILE *fi = fopen(fn, "wb");
    if (fi == NULL) giveup("can't create %s", fn);
    gdImageGif(im, fi);
    fclose(fi);
    gdImageDestroy(im);
    delete ymvals;
  }

static void pgraph(gdImagePtr im, double vec[], int nsteps, int col, bool wrapy)
  { for (int i = 1; i <= nsteps; i++)
      { double x0 = (double) (i-1) / (double) nsteps,
	       x1 = (double) i / (double) nsteps;
	double y0 = vec[i-1], y1 = vec[i];
	if (y0 >= 0.0 && y1 >= 0.0)	/* suppress discontinuities */
	  { if (wrapy && y0-y1 > 0.5)
	      { gdImageLine(im, xmap_pm(x0), ymap_pm(y0), xmap_pm(x1), ymap_pm(1.0), col);
		gdImageLine(im, xmap_pm(x0), ymap_pm(0.0), xmap_pm(x1), ymap_pm(y1), col);
	      }
	    else if (wrapy && y1-y0 > 0.5)
	      { gdImageLine(im, xmap_pm(x0), ymap_pm(y0), xmap_pm(x1), ymap_pm(0.0), col);
		gdImageLine(im, xmap_pm(x0), ymap_pm(1.0), xmap_pm(x1), ymap_pm(y1), col);
	      }
	    else gdImageLine(im, xmap_pm(x0), ymap_pm(y0), xmap_pm(x1), ymap_pm(y1), col);
	  }
      }
  }

static void draw_xtick(gdImagePtr im, double x, double y, int col, int tlen, char *fmt, double val)
  { int ix = xmap_pm(x), iy = ymap_pm(y);
    gdImageLine(im, ix, iy, ix, iy+tlen, col);
    char str[32]; sprintf(str, fmt, val);
    int swid = strlen(str) * (gdFontSmall -> w);
    gdImageString(im, gdFontSmall, ix - swid/2, iy + 2*tlen, str, col);
  }

static void draw_ytick(gdImagePtr im, double x, double y, int col, int tlen, char *fmt, double val)
  { int ix = xmap_pm(x), iy = ymap_pm(y);
    gdImageLine(im, ix, iy, ix+tlen, iy, col);
    char str[32]; sprintf(str, fmt, val);
    int swid = strlen(str) * (gdFontSmall -> w);
    int shgt = gdFontSmall -> h;
    if (tlen < 0) gdImageString(im, gdFontSmall, ix + 2*tlen - swid, iy - shgt/2, str, col);
    if (tlen > 0) gdImageString(im, gdFontSmall, ix + 2*tlen, iy - shgt/2, str, col);
  }

static int xmap_pm(double x)
  { return (int) (XYPIXELS * x) + BORDER;
  }

static int ymap_pm(double y)
  { return (int) (XYPIXELS * (1.0 - y)) + BORDER;
  }

static char *choosefmt(double a1, double a2)
  { /* choose smallest format such that all ticks are labelled distinctly */
    static char fmt[10]; bool ok;
    int p = 0;
    do
      { sprintf(fmt, "%%.%df", p++);
	ok = true;
	char buf1[20], buf2[20]; buf2[0] = '\0';
	for (int i = 0; i <= 10 && ok; i++)
	  { double a = (double) i / 10.0;
	    double x = a1 + a * (a2-a1);
	    strcpy(buf1, buf2); sprintf(buf2, fmt, x);
	    if (seq(buf1, buf2)) ok = false;
	  }
      }
    until (ok || p > 10);
    unless (ok) strcpy(fmt, "?.?");
    return fmt;
  }

static void newhandler()
  { giveup("No room");
  }

static void giveup(char *msg, word p1)
  { fprintf(stderr, "genplot: ");
    fprintf(stderr, msg, p1.i); putc('\n', stderr);
    exit(1);
  }

