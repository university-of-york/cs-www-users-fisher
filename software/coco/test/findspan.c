/* findslope -- calc slope values for polar-to-OS approximation
   A.J. Fisher	 May 1995 */

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
#define Y0	    (-5568516.0)
#define THETA0	    (-2.0)	   /* degrees	   */
#define RADIANS	    0.01745329252  /* 2 pi / 360   */

#ifdef USA
#define Maths	    Math
#endif

#define SLOPE	    1.004895	/* cosecant of angle between vertical and surface of earth --
				this really depends on phi, but things get too complicated
				(viz. elliptic integrals & Difficult Maths) */

#define TEMPFN	    "/tmp/tmp"

#define ifix(x)	 ((int) ((x)+0.5))	/* x .gt. 0 */

extern double sin(), cos(), sqrt(), asin(), fabs(), atof();

forward double findspan(), getval();

struct pco { double lat, lon };	    /* polar coords (latitude, longitude)   */
struct cco { double x, y };	    /* Cartesian coords (easting, northing) */

static double y0;


global main(argc, argv) int argc; char *argv[];
  { double y1, y2, s1, s2;
    y1 = 5.5e6; y2 = 5.6e6;
    s1 = findspan(y1); s2 = findspan(y2);
    for (;;)
      { double ym = (y1+y2)/2.0;
	double sm = findspan(ym);
	if (fabs(s1-sm) < fabs(s2-sm)) { y2 = ym; s2 = sm; }
	else { y1 = ym; s1 = sm; }
      }
  }

static double findspan(yarg) double yarg;
  { int ilat, ilon; double span;
    double minslope = +100.0, maxslope = -100.0;
    y0 = -yarg;
    for (ilat = 50; ilat <= 60; ilat++)
      { for (ilon = -6; ilon <= +2; ilon++)
	  { struct pco pco; struct cco mycco, gdcco; double slope;
	    pco.lat = (double) ilat;
	    pco.lon = (double) ilon;
	    my_convert(&pco, &mycco);
	    gd_convert(&pco, &gdcco);
	    slope = (gdcco.y - y0) / (mycco.y - y0);
	    /* printf("lat %2d lon %+2d  my x,y %12.4f %12.4f   gd x,y %12.4f %12.4f   slope = %14.10f\n",
		       ilat, ilon, mycco.x, mycco.y, gdcco.x, gdcco.y, slope); */
	    if (slope < minslope) minslope = slope;
	    if (slope > maxslope) maxslope = slope;
	  }
      }
    span = maxslope - minslope;
    printf("y0 = %10.1f   minslope = %14.10f   maxslope = %14.10f   span = %14.6e\n", y0, minslope, maxslope, span);
    return span;
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
    cc -> x = X0 + lambda*xrad*cph*sth;
    cc -> y = y0 + erad*asin(lambda*sph);
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

