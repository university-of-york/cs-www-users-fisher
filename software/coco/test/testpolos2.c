/* testpolos -- test polar-to-OS routine
   A.J. Fisher	 May 1995

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
#define until(x)    while(!(x))

#define AIRY_A	    6377563.394	   /* radius of earth (semi-major axis)		*/
#define AIRY_B	    6356256.910	   /* radius of earth (semi-minor axis)		*/
#define ESQUARED    0.006670540000123428	/* from f77 prog */
#define ECC	    (sqrt(ESQUARED))	/* ??? */
/* #define ESQUARED    6.67054e-3     /* eccentricity squared, = (a^2-b^2)/a^2	   */
#define SCALE	    0.9996012717   /* scale factor on central meridian		*/
#define X0	    400000.0	   /* X origin					*/
#define Y0	    (-100000.0)	   /* Y origin					*/
#define THETA0	    (-2.0)	   /* central meridian (degrees)		*/
#define PHI0	    49.0	   /* "true origin" latitude                    */
#define PI	    3.1415926535897932384626433
#define RADIANS	    (PI / 180.0)

#ifdef USA
#define Maths	    Math
#endif

typedef double (*func)();

/* The exact calculation of X and Y requires the evaluation of an incomplete Legendre
   elliptic integral of the second kind (i.e. Difficult Maths).	 This is not evaluable
   in closed form.  We approximate the arc length Y along a meridan, properly given by
   "integral of r dphi", by "SLOPE * r * phi", where SLOPE is looked up in a table
   indexed by phi.  X is obtained similarly; in this case the table is indexed by theta.

   If ~200m accuracy over the UK is sufficient,
   just use the mean X and Y slopes from the tables as constants.  */

#define TEMPFN	    "/tmp/tmp"

#define ifix(x)	 ((int) ((x)+0.5))	/* x .gt. 0 */

extern double sin(), cos(), tan(), sqrt(), pow(), asin(), atan2(), hypot();
extern double log(), fabs(), atof();

forward double getval(), radfunc(), integrate(), xqtrap(), trapzd();

struct pco { double lat, lon };	    /* polar coords (latitude, longitude)   */
struct cco { double x, y };	    /* Cartesian coords (easting, northing) */


global main(argc, argv) int argc; char *argv[];
  { int ilat, ilon;
    double mindx = +1e10, maxdx = -1e10;
    double mindy = +1e10, maxdy = -1e10;
    double maxdh = -1e10;
    /* for (ilat = 49; ilat <= 49; ilat++) */
    for (ilat = 49; ilat <= 61; ilat++)
      { for (ilon = -6; ilon <= +2; ilon++)
	/* for (ilon = -2; ilon <= -2; ilon++) */
	  { struct pco pco; struct cco mycco, gdcco; double dx, dy, dh;
	    pco.lat = (double) ilat;
	    pco.lon = (double) ilon;
	    my_convert(&pco, &mycco);
	    gd_convert(&pco, &gdcco);
	    printf("lat %2d lon %+2d   ", ilat, ilon);
	    printf("my x,y %12.4f %12.4f   gd x,y %12.4f %12.4f   ", mycco.x, mycco.y, gdcco.x, gdcco.y);
	    dx = mycco.x - gdcco.x; dy = mycco.y - gdcco.y; dh = hypot(dx, dy);
	    printf("dx %8.4f   dy %8.4f   ", dx, dy);
	    putchar('\n');
	    if (dx < mindx) mindx = dx;
	    if (dy < mindy) mindy = dy;
	    if (dx > maxdx) maxdx = dx;
	    if (dy > maxdy) maxdy = dy;
	    if (dh > maxdh) maxdh = dh;
	  }
	putchar('\n');
      }
    printf("dx %.4f .. %.4f   dy %.4f .. %.4f   ", mindx, maxdx, mindy, maxdy);
    printf("dh %.4f\n", maxdh);
    exit(0);
  }

#define X_FIDDLE1    2.38
#define X_FIDDLE2    0.104
#define Y_FIDDLE1    (-11516.0)
#define Y_FIDDLE2    14379.0

static my_convert(pc, cc) struct pco *pc; struct cco *cc;
  { /* given lat, lon, compute OS coords x, y */
    double phi, theta, phi_dash, theta_dash, sph, cph, sth, cth;
    double xrad, xcorrect, ycorrect;
    /* cvt to radians and move pole */
    phi = pc -> lat * RADIANS;
    theta = (pc -> lon - THETA0) * RADIANS;
    sph = sin(phi); cph = cos(phi); sth = sin(theta); cth = cos(theta);
    phi_dash = atan2(sph, cph*cth);
    theta_dash = atan2(cph*sth, hypot(sph, cph*cth));
    /* do std Mercator on shifted coord system */
    xrad = (AIRY_A*SCALE) / sqrt(1.0 - ESQUARED*sph*sph);
    xcorrect = (X_FIDDLE1*phi + X_FIDDLE2) * theta;
    ycorrect = (Y_FIDDLE1*phi + Y_FIDDLE2) * (theta*theta);
    cc -> x = X0 + xrad * log(tan(0.25*PI + 0.5*theta_dash)) + xcorrect;
    cc -> y = Y0 + (AIRY_A*SCALE) * integrate(radfunc, PHI0*RADIANS, phi_dash) + ycorrect;
  }

static double radfunc(phi) double phi;
  { double sph = sin(phi);
    return (1.0 - ESQUARED) / pow(1.0 - ESQUARED*sph*sph, 1.5);
  }

#define EPS  1e-10
#define MAXJ 20

static double integrate(f, a, b) func f; double a, b;
  { return (a < b) ? xqtrap(f, a, b) :
	   (a > b) ? -xqtrap(f, b, a) :
	   0.0;
  }

static double xqtrap(f, a, b) func f; double a, b;
  { int j; double olds, news;
    j = 0;
    olds = trapzd(f, a, b, j++);
    news = trapzd(f, a, b, j++);
    until (j >= MAXJ || fabs(news-olds) < EPS * fabs(olds))
      { olds = news;
	news = trapzd(f, a, b, j++);
      }
    if (j >= MAXJ) giveup("Too many steps in xqtrap");
    return news;
  }

static double trapzd(f, a, b, n) func f; double a, b; int n;
  { static double s;
    if (n == 0)
      { /* first step: init s */
	s = 0.5 * (b-a) * (f(a) + f(b));
      }
    else
      { /* refine s */
	int it, j; double dx, x, sum;
	it = 1 << (n-1);
	dx = (b-a) / (double) it;
	x = a + 0.5*dx;
	sum = 0.0;
	for (j=0; j < it; j++) { sum += f(x); x += dx; }
	s = 0.5 * (s + (b-a) * sum / (double) it);
      }
    return s;
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

