/* Convert lat/long to OS grid reference
   A.J. Fisher	 1991
   Updated  AJF	 April 1995

   Spherical trig formulae, compensated for Airy spheroid
   by first-order interpolation from table

   Accuracy:

   Lat range	Lon range    X error	Y error	  Dist error
   +49 .. +61	-7 .. +3     46.87 m	12.77 m	  48.58 m
   +50 .. +60	-6 .. +2     19.54 m	 5.18 m	  20.21 m

   For better accuracy use the power series expansion
   ("built up from fourth or fifth differences using National Cash Register Machines")
   published by the OS (1950). */

#include <stdio.h>

#define forward
#define global
#define bool	    int
#define false	    0
#define true	    1
#define unless(x)   if(!(x))

#define AIRY_A	    6377563.394	   /* radius of earth (semi-major axis)			   */
#define AIRY_B	    6356256.910	   /* radius of earth (semi-minor axis)			   */
#define ESQUARED    6.67054e-3	   /* eccentricity squared, = (a^2-b^2)/a^2		   */
#define SCALE	    0.9996012717   /* scale factor on central meridian			   */
#define X0	    400000.0	   /* X origin						   */
#define Y0	    (-5570551.97)  /* Y origin, modified to minimize error due to spheroid */
#define THETA0	    (-2.0)	   /* central meridian (degrees)			   */
#define RADIANS	    0.01745329252  /* 2 pi / 360   */

#ifdef USA
#define Maths	    Math
#endif

/* The exact calculation of X and Y requires the evaluation of an incomplete Legendre
   elliptic integral of the second kind (i.e. Difficult Maths).	 This is not evaluable
   in closed form.  We approximate the arc length Y along a meridan, properly given by
   "integral of r dphi", by "SLOPE * r * phi", where SLOPE is looked up in a table
   indexed by phi.  X is obtained similarly; in this case the table is indexed by theta.

   If ~200m accuracy over the UK is sufficient,
   just use the mean X and Y slopes from the tables as constants.  */

static double xslopes[] =	    /* symmetrical about central meridian */
  { -1.0633885366e-03, -8.1474863172e-04, -5.9905915196e-04, -4.1626223821e-04,	    /* lon -10 .. - 7 */
    -2.6646499377e-04, -1.4981885233e-04, -6.6817666257e-05, -1.7255653765e-05,	    /* lon - 6 .. - 3 */
    +0.0000000000e+00, -1.7255653765e-05, -6.6817666257e-05, -1.4981885233e-04,	    /* lon - 2 .. + 1 */
    -2.6646499377e-04, -4.1626223821e-04, -5.9905915196e-04, -8.1474863172e-04,	    /* lon + 2 .. + 5 */
    -1.0633885366e-03,								    /* lon + 6 .. + 6 */
  };

static double yslopes[] =
  { +5.5027765781e-03, +5.4435299170e-03, +5.3929447959e-03, +5.3504239569e-03,	    /* lat +45 .. +48 */
    +5.3153580846e-03, +5.2872171454e-03, +5.2654897559e-03, +5.2496988175e-03,	    /* lat +49 .. +52 */
    +5.2393814093e-03, +5.2341209585e-03, +5.2334940328e-03, +5.2371354295e-03,	    /* lat +53 .. +56 */
    +5.2446541989e-03, +5.2557292018e-03, +5.2700270097e-03, +5.2872180996e-03,	    /* lat +57 .. +60 */
    +5.3070062389e-03, +5.3291114408e-03, +5.3532545442e-03, +5.3791714038e-03,	    /* lat +61 .. +64 */
    +5.4066257798e-03,								    /* lat +65 .. +65 */
  };

#define ifix(x)	 ((int) ((x)+0.5))	/* x .gt. 0 */

extern double sin(), cos(), sqrt(), asin(), floor();

forward double cvtangle(), slope();

struct pco { double lat, lon };	    /* polar coords (latitude, longitude)   */
struct cco { double x, y };	    /* Cartesian coords (easting, northing) */


global main(argc, argv) int argc; char *argv[];
  { for (;;)
      { struct pco pco;
	struct cco cco;
	getlatlong(&pco);
	convert(&pco, &cco);
	printf("OS coords. %7.0f %7.0f\n", cco.x, cco.y);
      }
  }

static getlatlong(pc) struct pco *pc;
  { char buf[256];
    printf("Enter latitude:  "); fflush(stdout);
    getstr(buf); pc -> lat = cvtangle(buf, "NS");
    printf("Enter longitude: "); fflush(stdout);
    getstr(buf); pc -> lon = cvtangle(buf, "EW");
  }

static getstr(buf) char *buf;
  { char *x = gets(buf);
    if (x == NULL) { putchar('\n'); exit(0); }
  }

static double cvtangle(s, dir) char *s, *dir;
  { int deg, min; char chs[2]; double sec, ang;
    int ni = sscanf(s, "%d%d%lf%1s", &deg, &min, &sec, chs);
    unless (ni == 4 && (chs[0] == dir[0] || chs[0] == dir[1]))
      { fprintf(stderr, "Format error: %s\n", s);
	exit(1);
      }
    ang = deg + min/60.0 + sec/3600.0;
    if (chs[0] == dir[1]) ang = -ang;
    return ang;
  }

static convert(pc, cc) struct pco *pc; struct cco *cc;
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
    cc -> x = X0 + slope(pc -> lon + 10.0, xslopes, 17) * (lambda*xrad*cph*sth);
    cc -> y = Y0 + slope(pc -> lat - 45.0, yslopes, 21) * (erad*asin(lambda*sph));
  }

static double slope(lat, tab, nent) double lat; double tab[]; int nent;
  { /* interpolate from table */
    double fl, s; int k;
    fl = floor(lat); k = ifix(fl);
    if (k < 0) s = tab[0];
    else if (k+1 > nent-1) s = tab[nent-1];
    else
      { double a = lat-fl;
	s = ((1.0-a) * tab[k]) + (a * tab[k+1]);
      }
    return 1.0 + s;
  }

