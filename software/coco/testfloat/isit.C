/* Routines for converting polar (lon-lat) to OS coords
   A.J. Fisher	 May 1995 */

#include <stdio.h>
#include "hdr.h"

#define AIRY_A	    6377563.396			/* radius of earth (semi-major axis)	     */
#define AIRY_B	    6356256.910			/* radius of earth (semi-minor axis)	     */
#define ESQUARED    6.670540000123428e-3	/* eccentricity squared, = (a^2-b^2)/a^2     */
#define SCALE	    0.9996012717		/* scale factor on central meridian	     */
#define X0	    400000.0			/* X origin			     */
#define Y0	    (-100000.0)			/* Y origin			     */
#define THETA0	    (-2.0)			/* central meridian (degrees)	     */
#define PHI0	    49.0			/* latitude origin (degrees)	     */

#define ARAD	    (AIRY_A * SCALE)

#define PI	    3.1415926535897932384626433
#define RADIANS	    (PI / 180.0)

#define POLY0(x,a0)		(a0)
#define POLY1(x,a1,a0)		(POLY0(x,a1) * x + a0)
#define POLY2(x,a2,a1,a0)	(POLY1(x,a2,a1) * x + a0)

extern "C" double sin(double), cos(double), sqrt(double);

static float meridist(float);


global void polar_to_os(pco *pc, cco *cc)
  { /* given polar coords lon, lat, compute OS coords x, y, by series method */
    float lam, phi, sph, cph, tph, t, lcph, lsph, lcph2, nu, n, poly;
    lam = (pc -> lon - THETA0) * RADIANS;
    phi = pc -> lat * RADIANS;
    sph = sin(phi); cph = cos(phi); tph = sph/cph; t = tph*tph;
    lcph = lam*cph; lsph = lam*sph; lcph2 = lcph*lcph;
    nu = ARAD / sqrt(1.0 - ESQUARED*sph*sph);	/* R of C perp to meridian */
    n = (ESQUARED*cph*cph) / (1.0-ESQUARED);	/* n = eta^2 = (nu/rho)-1      */

    poly = POLY2(lcph2, (POLY1(t, -18, 5) + (-58*t*n + 14*n)) / 120,
			(-t + n + 1) / 6,
			1);

    cc -> x = X0 + lcph*nu*poly;

    poly = POLY2(lcph2, (POLY2(t, 1, -58, 61) + (-330*t*n + 270*n)) / 720,
			(-t + POLY2(n, 4, 9, 5)) / 24,
			0.5);

    cc -> y = Y0 + ARAD * (meridist(phi) - meridist(PHI0*RADIANS)) + lsph*lcph*nu*poly;
  };

/* meridional distance for ellipsoid */

#define EPS2	1.0820180907122924e-07
#define EPS1	2.0853669848390807e-05
#define EPS0	5.0008134269452229e-03
#define EPSK	9.9833027342682179e-01

static float meridist(float phi)
  { /* meridional distance to latitude phi */
    double sph = sin(phi), cph = cos(phi);
    double cs = cph*sph, ss = sph*sph;
    double p = POLY2(ss, EPS2, EPS1, EPS0);
    return (EPSK*phi - p*cs);
  }

