/* Routines for converting polar (lon-lat) to OS coords, and vice-versa
   A.J. Fisher	 May 1995 */

#include <stdio.h>
#include <hdr.h>

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
#define POLY3(x,a3,a2,a1,a0)	(POLY2(x,a3,a2,a1) * x + a0)

extern double sin(), cos(), sqrt(), pow(), fabs();

forward double inv_meridist(), meridist();


global os_to_polar(cc, pc) struct cco *cc; struct pco *pc;
  { /* given OS coords x, y, compute polar coords lon, lat, by series method */
    double m0, lam, phi, n, sph, cph, tph, t, nu, d, d2, poly;
    m0 = meridist(PHI0*RADIANS);
    phi = inv_meridist(m0 + ((cc -> y - Y0) / ARAD));	/* foot-point latitude */
    sph = sin(phi); cph = cos(phi); tph = sph/cph; t = tph*tph;
    nu = ARAD / sqrt(1.0 - ESQUARED*sph*sph);	/* R of C perp to meridian */
    n = (ESQUARED*cph*cph) / (1.0-ESQUARED);	/* n = eta^2 = (nu/rho)-1      */
    d = (cc -> x - X0) / nu;
    d2 = d*d;

    poly = POLY3(d2, POLY3(t, 720, 1320, 662, 61) / 5040,
		     (POLY2(t, 24, 28, 5) + (8*t*n + 6*n)) / 120,
		     (2*t + n + 1) / 6,
		     -1);

    lam = - (d*poly / cph);
    pc -> lon = THETA0 + (lam / RADIANS);

    poly = POLY3(d2, POLY3(t, 1574, 4095, 3633, 1385) / 40320,
		     (POLY2(t, 45, 90, 61) + (-252*t*n + 46*n)) / 720,
		     (3*t + 9*t*n + POLY2(n, -4, 1, 5)) / 24,
		     -0.5);

    phi += d2 * tph * (1+n) * poly;
    pc -> lat = phi / RADIANS;
  }

global polar_to_os(pc, cc) struct pco *pc; struct cco *cc;
  { /* given polar coords lon, lat, compute OS coords x, y, by series method */
    double lam, phi, sph, cph, tph, t, lcph, lsph, lcph2, nu, n, poly;
    lam = (pc -> lon - THETA0) * RADIANS;
    phi = pc -> lat * RADIANS;
    sph = sin(phi); cph = cos(phi); tph = sph/cph; t = tph*tph;
    lcph = lam*cph; lsph = lam*sph; lcph2 = lcph*lcph;
    nu = ARAD / sqrt(1.0 - ESQUARED*sph*sph);	/* R of C perp to meridian */
    n = (ESQUARED*cph*cph) / (1.0-ESQUARED);	/* n = eta^2 = (nu/rho)-1      */

    poly = POLY3(lcph2, POLY3(t, -1, 179, -479, 61) / 5040,
			(POLY2(t, 1, -18, 5) + (-58*t*n + 14*n)) / 120,
			(-t + n + 1) / 6,
			1);

    cc -> x = X0 + lcph*nu*poly;

    poly = POLY3(lcph2, POLY3(t, -1, 543, -3111, 1385) / 40320,
			(POLY2(t, 1, -58, 61) + (-330*t*n + 270*n)) / 720,
			(-t + POLY2(n, 4, 9, 5)) / 24,
			0.5);

    cc -> y = Y0 + ARAD * (meridist(phi) - meridist(PHI0*RADIANS)) + lsph*lcph*nu*poly;
  };

/* inverse meridional distance -- given distance, return latitude
   For typical ellipsoids, accuracy is about 1e-11 radians */

static double inv_meridist(double arg)
  { double phi = arg;
    int nits = 0; bool cvg = false;
    until (cvg || nits >= 10)	/* rarely goes over 5 iterations */
      { double sph, rho, dphi;
	sph = sin(phi);
	rho = (1.0 - ESQUARED) / pow(1.0 - ESQUARED*sph*sph, 1.5);	/* R of C along meridian */
	dphi = (arg - meridist(phi)) / rho;
	if (fabs(dphi) < 1e-11) cvg = true;
	phi += dphi; nits++;
      }
    unless (cvg)
      { fprintf(stderr, "Failed to converge! (inv_meridist)\n");
	exit(1);
      }
    return phi;
  }

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
    return (EPSK*phi - p*cs);
  }

