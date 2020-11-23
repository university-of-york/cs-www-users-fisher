/* testpolos -- test polar-to-OS routine
   A.J. Fisher	 May 1995 */

/* NB:	Results correct to +- 200m everywhere in the UK,
	and +-150m almost everywhere in the UK.
	For better accuracy use the power series expansion
	("built up from fourth or fifth differences using National
	Cash Register Machines") published by the OS (1950). */

#include <stdio.h>

#define forward
#define global
#define bool	    int
#define false	    0
#define true	    1
#define unless(x)   if(!(x))

#define AIRY_A	    6377563.394	   /* radius of earth (semi-major axis)	       */
#define AIRY_B	    6356256.910	   /* radius of earth (semi-minor axis)	       */
#define ESQUARED    0.00667054	   /* eccentricity squared, = (a^2-b^2)/a^2    */
#define SCALE	    0.9996012717   /* scale factor on central meridian	       */
#define X0	    400000.0
#define THETA0	    (-2.0)	   /* degrees	   */
#define RADIANS	    0.01745329252  /* 2 pi / 360   */

/* The exact calculation of Y requires the evaluation of an incomplete Legendre
   elliptic integral of the second kind (i.e. Difficult Maths).	 We approximate
   the arc length on an ellipse, properly given by "integral of r dphi", by
   "YSLOPE * r * phi". */

#define Y0	    (-5570546.8)   /* computed to minimize (maxslope - minslope) */
#define YSLOPE	    1.005259917	   /* linear approx to elliptic integral */

#ifdef USA
#define Maths	    Math
#endif

#define TEMPFN	    "/tmp/tmp"

#define ifix(x)	 ((int) ((x)+0.5))	/* x .gt. 0 */

extern double sin(), cos(), sqrt(), asin(), fabs(), hypot();

forward double findmaxdx(), getval();

struct pco { double lat, lon };	    /* polar coords (latitude, longitude)   */
struct cco { double x, y };	    /* Cartesian coords (easting, northing) */

static double xslope;


global main(argc, argv) int argc; char *argv[];
  { double y1, y2, s1, s2;
    y1 = 0.994; y2 = 1.0;
    s1 = findmaxdx(y1); s2 = findmaxdx(y2);
    for (;;)
      { double ym = (y1+y2)/2.0;
	double sm = findmaxdx(ym);
	if (fabs(s1-sm) < fabs(s2-sm)) { y2 = ym; s2 = sm; }
	else { y1 = ym; s1 = sm; }
      }
  }

static double findmaxdx(arg) double arg;
  { int ilat, ilon; double maxdx = -1.0, maxdy = -1.0, maxdh = -1.0;
    xslope = arg;
    for (ilat = 50; ilat <= 60; ilat++)
      { for (ilon = -6; ilon <= +2; ilon++)
	  { struct pco pco; struct cco mycco, gdcco; double dx, dy, dh;
	    pco.lat = (double) ilat;
	    pco.lon = (double) ilon;
	    my_convert(&pco, &mycco);
	    gd_convert(&pco, &gdcco);
	    /* printf("lat %2d lon %+2d  my x,y %12.4f %12.4f   gd x,y %12.4f %12.4f   ",
		       ilat, ilon, mycco.x, mycco.y, gdcco.x, gdcco.y); */
	    dx = mycco.x - gdcco.x; dy = mycco.y - gdcco.y; dh = hypot(dx, dy);
	    /* printf("dx %14.6e   dy %14.6e   dh %14.6e\n", dx, dy, dh); */
	    if (fabs(dx) > maxdx) maxdx = fabs(dx);
	    if (fabs(dy) > maxdy) maxdy = fabs(dy);
	    if (dh > maxdh) maxdh = dh;
	  }
      }
    printf("xslope = %18.10e   maxdx = %18.10e\n", xslope, maxdx);
    return maxdx;
  }

static my_convert(pc, cc) struct pco *pc; struct cco *cc;
  { /* given theta, phi, compute OS coords x, y */
    double cph, sph, cth, sth, efac, xrad, erad, lambda;
    cph = cos(pc -> lat * RADIANS);
    sph = sin(pc -> lat * RADIANS);
    cth = cos((pc -> lon - THETA0) * RADIANS);
    sth = sin((pc -> lon - THETA0) * RADIANS);
    efac = 1.0 - ESQUARED*sph*sph;
    xrad = (AIRY_A*SCALE) / sqrt(efac);		/* radius of curvature of earth in X direction */
    erad = (AIRY_A*SCALE) * sqrt(efac);		/* radius of earth */
    lambda = 1.0 / sqrt(cph*cph*cth*cth + sph*sph);
    cc -> x = X0 + xslope*lambda*xrad*cph*sth;
    cc -> y = Y0 + YSLOPE*erad*asin(lambda*sph);
  }

static gd_convert(pc, cc) struct pco *pc; struct cco *cc;
  { FILE *tfi, *pfi; char cmd[256];
    tfi = fopen(TEMPFN, "w");
    if (tfi == NULL) giveup("can't open %s", TEMPFN);
    fprintf(tfi, "%d\n0\n0\n", ifix(pc -> lat));            /* assumed integer! */
    fprintf(tfi, "%d\n0\n0\n", ifix(fabs(pc -> lon)));      /* assumed integer! */
    fprintf(tfi, "%c\n", (pc -> lon >= 0.0) ? 'E' : 'W');
    fprintf(tfi, "Q\n");
    fclose(tfi);
    sprintf(cmd, "cat %s | polar_os | grep ING", TEMPFN);
    pfi = popen(cmd, "r");
    if (pfi == NULL) giveup("popen failed");
    cc -> x = getval(pfi);
    cc -> y = getval(pfi);
    pclose(pfi);
  }

static double getval(pfi) FILE *pfi;
  { int ni; double val;
    ni = fscanf(pfi, "%*[^=]=%lg\n", &val);
    unless (ni == 1) giveup("fscanf error, ni = %d", ni);
    return val;
  }

static giveup(msg, p1) char *msg; int p1;
  { fprintf(stderr, msg, p1); putc('\n', stderr);
    exit(1);
  }

