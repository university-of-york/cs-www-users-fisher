/* generate .gif mag/phase plots
   based on "genplot" in AJF's "mkfilter" package
   A.J. Fisher, University of York   <fisher@minster.york.ac.uk>
   June 1997 */

/* Uses the "gd" gif manipulation library from
   Quest Protein Database Center   <http://siva.cshl.org/gd/gd.html> */

#include <stdio.h>
#include <string.h>
#include <math.h>

extern "C" {
    #include <gd.h>
    #include <gdfonts.h>
};

#include "solvecct.h"
#include "complex.h"

#define XYPIXELS 400
#define BORDER	 40
#define TICKLEN	 5
#define PHEPS	 1e-20
#define TWOPI	 (2.0 * M_PI)

struct Tickvec
  { Tickvec(int n, double *v, char *f) { num = n; vec = v; fmt = f; }
    int num;
    double *vec;
    char *fmt;
  };

static void pgraph(gdImage*, double*, int, int, bool);
static void draw_xtick(gdImage*, double, double, int, int, char*, double = 0.0);
static void draw_ytick(gdImage*, double, double, int, int, char*, double = 0.0);
static int xmap_pm(double), ymap_pm(double);
static Tickvec *placeticks(double, double);


global void drawgraph(complex *vec, int nsteps, double f1, double f2, double logmin, char *fn)
  { unless (nsteps >= 0 && f1 < f2) giveup("Bad parameters for graph");
    double *ymvals = new double[nsteps], *ypvals = new double[nsteps];
    gdImage *gd = gdImageCreate(XYPIXELS + 2*BORDER, XYPIXELS + 2*BORDER);
    gdImageColorAllocate(gd, 255, 255, 255); /* white background */
    int red = gdImageColorAllocate(gd, 255, 0, 0);
    int blue = gdImageColorAllocate(gd, 0, 0, 255);
    int black = gdImageColorAllocate(gd, 0, 0, 0);
    gdImageLine(gd, xmap_pm(0.0), ymap_pm(0.0), xmap_pm(1.0), ymap_pm(0.0), black); /* X axis	    */
    gdImageLine(gd, xmap_pm(0.0), ymap_pm(0.0), xmap_pm(0.0), ymap_pm(1.0), black); /* left Y axis  */
    gdImageLine(gd, xmap_pm(1.0), ymap_pm(0.0), xmap_pm(1.0), ymap_pm(1.0), black); /* right Y axis */
    double maxh = 0.0;
    for (int i = 0; i < nsteps; i++)
      { ymvals[i] = hypot(vec[i]);
	if (ymvals[i] > maxh) maxh = ymvals[i];
      }
    if (maxh == 0.0) maxh = 1.0;	/* best we can do! */
    for (int i = 0; i < nsteps; i++)
      { ymvals[i] /= maxh;
	ypvals[i] = (ymvals[i] > PHEPS) ? atan2(vec[i]) / TWOPI + 0.5 :
		    -1.0; /* out-of-range value will not be plotted */
      }
    /* ymvals, ypvals in range 0 .. 1 */
    if (logmin < 0.0)
      { for (int i = 0; i < nsteps; i++)
	  ymvals[i] = (ymvals[i] > 0.0) ? (20.0 * log10(ymvals[i]) - logmin) / (-logmin) :
					  -1.0; /* out-of-range value will not be plotted */
      }
    pgraph(gd, ymvals, nsteps, red, false);    /* mag graph   */
    pgraph(gd, ypvals, nsteps, blue, true);    /* phase graph */
    if (logmin < 0.0)
      { Tickvec *tv = placeticks(logmin, 0.0);
	for (int i = 0; i < tv -> num; i++)
	  { double y = tv -> vec[i];
	    if (y >= logmin && y <= 0.0) draw_ytick(gd, 0.0, 1.0 - y/logmin, red, -TICKLEN, tv -> fmt, y);
	  }
	delete tv;
      }
    else
      { for (int i = 0; i <= 10; i++) draw_ytick(gd, 0.0, 0.1*i, red, -TICKLEN, "%3.1f", 0.1*i);
      }
    draw_ytick(gd, 1.0, 0.0, blue, TICKLEN, "-pi");
    draw_ytick(gd, 1.0, 0.5, blue, TICKLEN, "0");
    draw_ytick(gd, 1.0, 1.0, blue, TICKLEN, "+pi");
    Tickvec *tv = placeticks(f1, f2);
    for (int i = 0; i < tv -> num; i++)
      { double x = tv -> vec[i];
	char *fmt = (tv -> num < 10 || i%2 == 0) ? tv -> fmt : "";      /* draw every other tick if close together */
	if (x >= f1 && x <= f2) draw_xtick(gd, (x-f1)/(f2-f1), 0.0, black, TICKLEN, fmt, x);
      }
    delete tv;
    FILE *fi = fopen(fn, "w");
    if (fi == NULL) giveup("Can't create %s", fn);
    gdImageGif(gd, fi);
    fclose(fi);
    gdImageDestroy(gd);
    delete ymvals; delete ypvals;
  }

static void pgraph(gdImage *gd, double *vec, int nsteps, int col, bool wrapy)
  { for (int i = 1; i < nsteps; i++)
      { double x0 = (double) (i-1) / (double) (nsteps-1),
	       x1 = (double) i / (double) (nsteps-1);
	double y0 = vec[i-1], y1 = vec[i];
	if (y0 >= 0.0 && y1 >= 0.0)	/* suppress discontinuities */
	  { if (wrapy && y0-y1 > 0.5)
	      { gdImageLine(gd, xmap_pm(x0), ymap_pm(y0), xmap_pm(x1), ymap_pm(1.0), col);
		gdImageLine(gd, xmap_pm(x0), ymap_pm(0.0), xmap_pm(x1), ymap_pm(y1), col);
	      }
	    else if (wrapy && y1-y0 > 0.5)
	      { gdImageLine(gd, xmap_pm(x0), ymap_pm(y0), xmap_pm(x1), ymap_pm(0.0), col);
		gdImageLine(gd, xmap_pm(x0), ymap_pm(1.0), xmap_pm(x1), ymap_pm(y1), col);
	      }
	    else gdImageLine(gd, xmap_pm(x0), ymap_pm(y0), xmap_pm(x1), ymap_pm(y1), col);
	  }
      }
  }

static void draw_xtick(gdImage *gd, double x, double y, int col, int tlen, char *fmt, double val)
  { int ix = xmap_pm(x), iy = ymap_pm(y);
    gdImageLine(gd, ix, iy, ix, iy+tlen, col);
    char str[32]; sprintf(str, fmt, val);
    int swid = strlen(str) * (gdFontSmall -> w);
    gdImageString(gd, gdFontSmall, ix - swid/2, iy + 2*tlen, str, col);
  }

static void draw_ytick(gdImage *gd, double x, double y, int col, int tlen, char *fmt, double val)
  { int ix = xmap_pm(x), iy = ymap_pm(y);
    gdImageLine(gd, ix, iy, ix+tlen, iy, col);
    char str[32]; sprintf(str, fmt, val);
    int swid = strlen(str) * (gdFontSmall -> w);
    int shgt = gdFontSmall -> h;
    if (tlen < 0) gdImageString(gd, gdFontSmall, ix + 2*tlen - swid, iy - shgt/2, str, col);
    if (tlen > 0) gdImageString(gd, gdFontSmall, ix + 2*tlen, iy - shgt/2, str, col);
  }

static int xmap_pm(double x)
  { return (int) (XYPIXELS * x) + BORDER;
  }

static int ymap_pm(double y)
  { return (int) (XYPIXELS * (1.0 - y)) + BORDER;
  }

static double adjtab[29] =
  { 0.5, 0.5, 0.5,
    1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0,
    2.0, 2.0, 2.0, 2.0, 2.0, 2.0, 2.0, 2.0, 2.0, 2.0, 2.0, 2.0, 2.0, 2.0, 2.0, 2.0,
    5.0, 5.0, 5.0,
  };

static Tickvec *placeticks(double xl, double xh)
  { double d = (xh-xl)/10.0;			/* first estimate: divide range into 10 */
    if (d <= 0.0)
      { fprintf(stderr, "xl=%g xh=%g\n", xl, xh);
	giveup("Math bug 1!");
      }
    d = pow(10.0, floor(log10(d) + 0.5));	/* round to nearest power of 10 */
    int n = (int) ceil((xh-xl)/d);		/* number of intervals: shd be in range 4 .. 32 */
    if (n < 4 || n > 32)
      { fprintf(stderr, "xl=%g xh=%g d=%g n=%d\n", xl, xh, d, n);
	giveup("Math bug 2!");
      }
    d *= adjtab[n-4];				/* use steps of 0.2, 0.5, 1.0, 2.0, 5.0 */
    n = (int) ceil((xh-xl)/d);			/* recalculate number of intervals */
    if (n < 6 || n > 15)
      { fprintf(stderr, "xl=%g xh=%g d=%g n=%d\n", xl, xh, d, n);
	giveup("Math bug 3!");
      }
    int m = (int) floor(xl/d);			/* number of first tick */
    double *vec = new double[n+1];
    for (int i = 0; i <= n; i++) vec[i] = (m+i)*d;
    return new Tickvec(n+1, vec, "%g");
  }

