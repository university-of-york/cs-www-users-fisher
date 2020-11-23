/* testpolos -- test polar-to-OS routine
   A.J. Fisher	 May 1995 */

#include <stdio.h>

#define forward
#define global
#define bool	    int
#define false	    0
#define true	    1
#define unless(x)   if(!(x))
#define until(x)    while(!(x))

#define AIRY_A	    6377563.396			/* radius of earth (semi-major axis)	     */
#define AIRY_B	    6356256.910			/* radius of earth (semi-minor axis)	     */
#define ESQUARED    6.670540000123428e-3	/* eccentricity squared, = (a^2-b^2)/a^2     */
#define SCALE	    0.9996012717		/* scale factor on central meridian	     */
#define X0	    400000.0			/* X origin				     */
#define Y0	    (-100000.0)			/* Y origin				     */
#define THETA0	    (-2.0)			/* central meridian (degrees)		     */
#define PHI0	    49.0			/* "true origin" latitude                    */
#define PI	    3.1415926535897932384626433
#define RADIANS	    (PI / 180.0)

#define POLY0(x,a0)		(a0)
#define POLY1(x,a1,a0)		(POLY0(x,a1) * x + a0)
#define POLY2(x,a2,a1,a0)	(POLY1(x,a2,a1) * x + a0)
#define POLY3(x,a3,a2,a1,a0)	(POLY2(x,a3,a2,a1) * x + a0)

extern double sin(), cos(), sqrt(), hypot();

struct pco { double lon, lat };	    /* polar coords (latitude, longitude)   */
struct cco { double x, y };	    /* Cartesian coords (easting, northing) */

forward double meridist();


global main(argc, argv) int argc; char *argv[];
  { int ilat, ilon;
    double mindx = +1e10, maxdx = -1e10;
    double mindy = +1e10, maxdy = -1e10;
    double maxdh = -1e10;
    /* for (ilat = 49; ilat <= 49; ilat++) */
    for (ilat = 49; ilat <= 61; ilat++)
      { for (ilon = -6; ilon <= +2; ilon++)
	/* for (ilon = -2; ilon <= -2; ilon++) */
	  { struct pco pco; struct cco mycco, pjcco; double dx, dy, dh;
	    pco.lon = (double) ilon;
	    pco.lat = (double) ilat;
	    my_convert(&pco, &mycco);
	    pj_convert(&pco, &pjcco);
	    printf("lat %2d lon %+2d   ", ilat, ilon);
	    printf("my x,y %12.4f %12.4f   pj x,y %12.4f %12.4f   ", mycco.x, mycco.y, pjcco.x, pjcco.y);
	    dx = mycco.x - pjcco.x; dy = mycco.y - pjcco.y; dh = hypot(dx, dy);
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

static my_convert(pc, cc) struct pco *pc; struct cco *cc;
  { /* given lon, lat, compute OS coords x, y */
    double lam, phi, sph, cph, tph, tph2, lcph, lsph, lcph2, nu, eta2, poly;
    lam = (pc -> lon - THETA0) * RADIANS;
    phi = pc -> lat * RADIANS;
    sph = sin(phi); cph = cos(phi); tph = sph/cph; tph2 = tph*tph;
    lcph = lam*cph; lsph = lam*sph; lcph2 = lcph*lcph;
    nu = (AIRY_A*SCALE) / sqrt(1.0 - ESQUARED*sph*sph); /* R of C perp to meridian */
    eta2 = (ESQUARED*cph*cph) / (1.0-ESQUARED);		/* eta^2 = (nu/rho)-1	   */

    poly = POLY3(lcph2, POLY3(tph2, -1, 179, -479, 61) / 5040,
			(POLY2(tph2, 1, -18, 5) + eta2 * POLY1(tph2, -58, 14)) / 120,
			(1 - tph2 + eta2) / 6,
			1);

    cc -> x = X0 + lcph*nu*poly;

    poly = POLY3(lcph2, POLY3(tph2, -1, 543, -3111, 1385) / 40320,
			(POLY2(tph2, 1, -58, 61) + eta2 * POLY1(tph2, -330, 270)) / 720,
			(5 - tph2 + eta2 * (4*eta2 + 9)) / 24,
			0.5);

    cc -> y = Y0 + meridist(phi) - meridist(PHI0*RADIANS) + lsph*lcph*nu*poly;
  };

/* meridional distance for ellipsoid
   8th degree - accurate to < 1e-5 meters when used in conjunction
   with typical major axis values */

#define EPS3	6.0905227287964770e-10
#define EPS2	1.0820180907122924e-07
#define EPS1	2.0853669848390807e-05
#define EPS0	5.0008134269452229e-03
#define EPSK	9.9833027342682179e-01

static double meridist(phi) double phi;
  { /* meridional distance to latitude phi */
    double sph, cph, cs, ss, p;
    sph = sin(phi); cph = cos(phi);
    cs = cph*sph; ss = sph*sph;
    p = POLY3(ss, EPS3, EPS2, EPS1, EPS0);
    return (AIRY_A*SCALE) * (EPSK*phi - p*cs);
  }

static pj_convert(pc, cc) struct pco *pc; struct cco *cc;
  { FILE *pfi; char cmd[256]; int ni, len;
    sprintf(cmd, "echo '%.4f %.4f' | proj -f %%.6f +proj=tmerc +ellps=airy ", pc -> lon, pc -> lat);
    len = strlen(cmd);
    sprintf(&cmd[len], "+lat_0=%.0f +lon_0=%.0f +k_0=%.10f +x_0=%.0f +y_0=%.0f", PHI0, THETA0, SCALE, X0, Y0);
    pfi = popen(cmd, "r");
    if (pfi == NULL) giveup("popen failed");
    ni = fscanf(pfi, "%lg %lg\n", &cc -> x, &cc -> y);
    pclose(pfi);
    unless (ni == 2) giveup("fscanf error, ni = %d", ni);
  }

static giveup(msg, p1) char *msg; int p1;
  { fprintf(stderr, msg, p1); putc('\n', stderr);
    exit(1);
  }

