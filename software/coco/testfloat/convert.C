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
#define M0	    0.8513013928368630		/* meridional distance to PHI0	     */
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
    float lam, phi, sph, cph, tph, t, lcph, lsph, lcph2, nu, n;
    lam = (pc -> lon - THETA0) * RADIANS;
    phi = pc -> lat * RADIANS;
    sph = sin(phi); cph = cos(phi); tph = sph/cph; t = tph*tph;
    lcph = lam*cph; lsph = lam*sph; lcph2 = lcph*lcph;
    nu = ARAD / sqrt(1.0 - ESQUARED*sph*sph);	/* R of C perp to meridian */
    n = (ESQUARED*cph*cph) / (1.0-ESQUARED);	/* n = eta^2 = (nu/rho)-1      */

    float xp = (-t + n + 1.0) * lcph2 / 6.0 + 1.0;
    cc -> x = X0 + lcph*nu*xp;

    float yp = (-t + 9.0*n + 5.0) * lcph2 / 24.0 + 0.5;
    cc -> y = Y0 + ARAD * (meridist(phi) - M0) + lsph*lcph*nu*yp;
  };

/* meridional distance for ellipsoid */

#define EPS2	1.0820180907122924e-07
#define EPS1	2.0853669848390807e-05
#define EPS0	5.0008134269452229e-03
#define EPSK	9.9833027342682179e-01

static float meridist(float phi)
  { /* meridional distance to latitude phi */
    float sph = sin(phi), cph = cos(phi);
    float cs = cph*sph, ss = sph*sph;
    float p = POLY2(ss, EPS2, EPS1, EPS0);
    return (EPSK*phi - p*cs);
  }

