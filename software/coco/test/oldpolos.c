/* Convert lat/long to OS grid reference
   A.J. Fisher	 1991
   Updated  AJF	 April 1995 */

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

#define ifix(x)	 ((int) ((x)+0.5))	/* x .gt. 0 */

extern double sin(), cos(), sqrt(), asin();

forward double cvtangle();

struct pco { double lat, lon };	    /* polar coords (latitude, longitude)   */
struct cco { double x, y };	    /* Cartesian coords (easting, northing) */


global main(argc, argv) int argc; char *argv[];
  { for (;;)
      { struct pco pco;
	struct cco cco;
	getlatlong(&pco);
	computeos(&pco, &cco);
	printf("OS coords. %07d %07d\n", ifix(cco.x), ifix(cco.y));
      }
  }

static getlatlong(pc) struct pco *pc;
  { char buf[256];
    printf("Enter latitude:  "); fflush(stdout);
    gets(buf); pc -> lat = cvtangle(buf, "NS");
    printf("Enter longitude: "); fflush(stdout);
    gets(buf); pc -> lon = cvtangle(buf, "EW");
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

static computeos(pc, cc) struct pco *pc; struct cco *cc;
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
    cc -> y = Y0 + SLOPE*erad*asin(lambda*sph);
  }

