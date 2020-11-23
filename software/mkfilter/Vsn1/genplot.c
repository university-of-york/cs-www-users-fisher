/* genplot - use "-l" output from mkfilter to generate .gif mag/phase plots
   A.J. Fisher, University of York   <fisher@minster.york.ac.uk>
   April 1995 */

/* Uses the "gd" gif manipulation library from
   Quest Protein Database Center   <http://siva.cshl.org/gd/gd.html> */

#include <stdio.h>
#include "mkfilter.h"
#include "gd.h"
#include "gdfonts.h"

#define NUMSTEPS 100
#define PIXSCALE 4
#define BORDER	 40
#define TICKLEN	 5

static char cmdline[256];
static struct complex topcoeffs[MAXPOLES+1], botcoeffs[MAXPOLES+1];
static int npoles;
static struct complex response[NUMSTEPS];

extern struct complex evaluate();	/* from complex.o */
extern struct complex cexp(), cdiv();	/* from complex.o */

extern double fabs(), hypot(), atan2();


global main(argc, argv) int argc; char *argv[];
  { unless (argc == 2) usage();
    setpath();
    readdata();
    computeresponse();
    creategraph(argv[1]);
    exit(0);
  }

static usage()
  { fprintf(stderr, "Usage: genplot out_fn.gif\n");
    exit(1);
  }

static setpath()
  { int code;
#ifdef sparc
    code = putenv("PATH=/usr/fisher/sparcbin:/bin:/usr/bin:/sun4/bin");
#else
    code = -1;
#endif
    if (code != 0) giveup("putenv failed!");
  }

static readdata()
  { int ni, i;
    ni = scanf("%[^\n]\n", cmdline);
    unless (ni == 1) formaterror(1);
    ni = scanf("%d\n", &npoles);
    unless (ni == 1 && npoles >= 0 && npoles <= MAXPOLES) formaterror(2);
    for (i=0; i <= npoles; i++)
      { ni = scanf(" %lg %lg\n", &topcoeffs[i].re, &botcoeffs[i].re);
	unless (ni == 2) formaterror(3);
	topcoeffs[i].im = botcoeffs[i].im = 0.0;
	botcoeffs[i].re = -botcoeffs[i].re;
      }
  }

static formaterror(n) int n;
  { giveup("input format error (%d)", n);
  }

static computeresponse()
  { int i;
    for (i=0; i < NUMSTEPS; i++)
      { struct complex s, z;
	s.re = 0.0;
	s.im = PI * (double) i / (double) NUMSTEPS;
	z = cexp(s);
	response[i] = evaluate(topcoeffs, botcoeffs, npoles, z);
      }
  }

static creategraph(fn) char *fn;
  { gdImagePtr im; FILE *fi; int i, red, blue, black;
    double xvals[NUMSTEPS], ymvals[NUMSTEPS], ypvals[NUMSTEPS];
    im = gdImageCreate(NUMSTEPS*PIXSCALE + 2*BORDER, NUMSTEPS*PIXSCALE + 2*BORDER);
    gdImageColorAllocate(im, 255, 255, 255); /* white background */
    red = gdImageColorAllocate(im, 255, 0, 0);
    blue = gdImageColorAllocate(im, 0, 0, 255);
    black = gdImageColorAllocate(im, 0, 0, 0);
    gdImageLine(im, xmap(0.0), ymap(0.0), xmap(1.0), ymap(0.0), black); /* X axis	*/
    gdImageLine(im, xmap(0.0), ymap(0.0), xmap(0.0), ymap(1.0), black); /* left Y axis	*/
    gdImageLine(im, xmap(1.0), ymap(0.0), xmap(1.0), ymap(1.0), black); /* right Y axis */
    for (i=0; i < NUMSTEPS; i++)
      { xvals[i] = (double) i / (double) NUMSTEPS;
	ymvals[i] = hypot(response[i].im, response[i].re);
	ypvals[i] = (ymvals[i] > EPS) ? atan2(response[i].im, response[i].re) / TWOPI + 0.5 :
		    -1.0; /* out-of-range value will not be plotted */
      }
    scaleyvals(ymvals);
    /* xvals, ymvals, ypvals now in range 0 .. 1 */
    fi = fopen(fn, "w");
    if (fi == NULL) giveup("can't create %s", fn);
    /* plot mag curve */
    for (i=1; i < NUMSTEPS; i++)
      { double x0 = xvals[i-1], x1 = xvals[i];
	double y0 = ymvals[i-1], y1 = ymvals[i];
	gdImageLine(im, xmap(x0), ymap(y0), xmap(x1), ymap(y1), red);
      }
    /* plot phase curve */
    for (i=1; i < NUMSTEPS; i++)
      { double x0 = xvals[i-1], x1 = xvals[i];
	double y0 = ypvals[i-1], y1 = ypvals[i];
	if ((y0 >= 0.0 && y1 >= 0.0 && fabs(y1-y0) < 0.5)) /* suppress discontinuities */
	  gdImageLine(im, xmap(x0), ymap(y0), xmap(x1), ymap(y1), blue);
      }
    for (i=0; i <= 10; i++) draw_ytick(im, 0.0, 0.1*i, red, -TICKLEN, "%3.1f", 0.1*i);
    draw_ytick(im, 1.0, 0.0, blue, TICKLEN, "-pi");
    draw_ytick(im, 1.0, 0.5, blue, TICKLEN, "0");
    draw_ytick(im, 1.0, 1.0, blue, TICKLEN, "+pi");
    for (i=0; i <= 5; i++) draw_xtick(im, 0.2*i, 0.0, black, TICKLEN, "%3.1f", 0.1*i);
    gdImageGif(im, fi);
    fclose(fi);
    gdImageDestroy(im);
  }

static scaleyvals(yvals) double yvals[];
  { double ymin, ymax; int i;
    ymin = ymax = yvals[0];
    for (i=1; i < NUMSTEPS; i++)
      { if (yvals[i] < ymin) ymin = yvals[i];
	if (yvals[i] > ymax) ymax = yvals[i];
      }
    for (i=0; i < NUMSTEPS; i++) yvals[i] -= ymin;
    ymax -= ymin;
    if (ymax > 0)
      for (i=0; i < NUMSTEPS; i++) yvals[i] /= ymax;
  }

static draw_xtick(im, x, y, col, tlen, fmt, val) gdImagePtr im; double x, y; int col, tlen; char *fmt; double val;
  { int ix, iy, swid; char str[32];
    ix = xmap(x); iy = ymap(y);
    gdImageLine(im, ix, iy, ix, iy+tlen, col);
    sprintf(str, fmt, val);
    swid = strlen(str) * (gdFontSmall -> w);
    gdImageString(im, gdFontSmall, ix - swid/2, iy + 2*tlen, str, col);
  }

static draw_ytick(im, x, y, col, tlen, fmt, val) gdImagePtr im; double x, y; int col, tlen; char *fmt; double val;
  { int ix, iy, swid, shgt; char str[32];
    ix = xmap(x), iy = ymap(y);
    gdImageLine(im, ix, iy, ix+tlen, iy, col);
    sprintf(str, fmt, val);
    swid = strlen(str) * (gdFontSmall -> w);
    shgt = gdFontSmall -> h;
    if (tlen < 0) gdImageString(im, gdFontSmall, ix + 2*tlen - swid, iy - shgt/2, str, col);
    if (tlen > 0) gdImageString(im, gdFontSmall, ix + 2*tlen, iy - shgt/2, str, col);
  }

static int xmap(x) double x;
  { return (int) ((NUMSTEPS*PIXSCALE) * x) + BORDER;
  }

static int ymap(y) double y;
  { return (int) ((NUMSTEPS*PIXSCALE) * (1.0 - y)) + BORDER;
  }

static giveup(msg, p1) char *msg; word p1;
  { fprintf(stderr, "genplot: ");
    fprintf(stderr, msg, p1); putc('\n', stderr);
    exit(1);
  }

