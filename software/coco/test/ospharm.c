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
#define M0	    5427063.815			/* meridional distance to PHI0, adj by SCALE */
#define THETA0	    (-2.0)			/* central meridian (degrees)		     */
#define PHI0	    49.0			/* "true origin" latitude                    */
#define PI	    3.1415926535897932384626433
#define TWOPI	    (2.0 * PI)
#define RADIANS	    (PI / 180.0)

#define NHARMS	    7
#define NDERIVS	    20
#define EPS	    1e-9

#define POLY0(x,a0)		(a0)
#define POLY1(x,a1,a0)		(POLY0(x,a1) * x + a0)
#define POLY2(x,a2,a1,a0)	(POLY1(x,a2,a1) * x + a0)
#define POLY3(x,a3,a2,a1,a0)	(POLY2(x,a3,a2,a1) * x + a0)

#define MEPSK	9.9833027342682179e-01
#define MEPS2	2.5056370533281126e-03
#define MEPS4	2.6202371293300089e-06
#define MEPS6	3.3816589942819785e-09

#define C1	1.0050365011826925e+00	    /* 1.0 + 3.0*B3			    */
#define C3	1.6788337275641298e-03	    /* 0.25 * (ESQUARED / (1.0 - ESQUARED)) */

extern double sin(), cos(), sqrt(), hypot(), fabs();

struct pco { double lon, lat };		    /* polar coords (latitude, longitude)   */
struct cco { double x, y; int n; };	    /* Cartesian coords (easting, northing) */

static double linterm[NDERIVS];
static double avec[NDERIVS][NHARMS+3], bvec[NDERIVS][NHARMS+3]; /* padding reqd for convolution */

forward double evalderiv(), meridist();


global main(argc, argv) int argc; char *argv[];
  { makederivs();
    printderivs();
    testlatlong();
    exit(0);
  }

#define aval(n)	  (((n) > 0) ? (n)*av[n] : -(n)*av[-(n)])	/* n * a[n] */
#define bval(n)	  (((n) > 0) ? (n)*bv[n] : +(n)*bv[-(n)])	/* n * b[n] */

static makederivs()
  { int j, k;
    for (k=0; k < NHARMS; k++) avec[0][k] = bvec[0][k] = 0.0;
    avec[0][2] = -MEPS2; avec[0][4] = +MEPS4; avec[0][6] = -MEPS6;
    linterm[0] = MEPSK;
    for (j=1; j < NDERIVS; j++)
      { double *av = avec[j-1], *bv = bvec[j-1];    /* j-1'th derivative */
	double *av1 = avec[j], *bv1 = bvec[j];	    /* j'th derivative	 */
	double fac = 1.0 / (double) (2*j);
	/* add zero padding for convolution */
	for (k=0; k < 3; k++) av[NHARMS+k] = bv[NHARMS+k] = 0.0;
	/* convolve */
	for (k=0; k < NHARMS; k++)
	  { av1[k] = -fac * (C1 * (bval(k-1) + bval(k+1)) + C3 * (bval(k-3) + bval(k+3)));
	    bv1[k] = fac * (C1 * (aval(k-1) + aval(k+1)) + C3 * (aval(k-3) + aval(k+3)));
	  }
	bv1[0] *= 0.5;
	bv1[1] += linterm[j-1] * C1;
	bv1[3] += linterm[j-1] * C3;
	linterm[j] = 0.0;
      }
  }

static printderivs()
  { int j;
    for (j=0; j < NDERIVS; j++)
      { printf("Deriv %2d: linterm %14.6e\n", j, linterm[j]);
	if (j & 1) prcoeffs('B', bvec[j]); else prcoeffs('A', avec[j]);
	putchar('\n');
      }
  }

static prcoeffs(ch, vec) char ch; double vec[];
  { int i;
    for (i=0; i < NHARMS; i++)
      { if (i%8 == 0) { putchar(ch); ch = ' '; }
	printf("%14.6e", vec[i]);
	if (i%8 == 7) putchar('\n');
      }
    unless (NHARMS%8 == 0) putchar('\n');
  }

static testlatlong()
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
	    printf("%3d   ", mycco.n);
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
  { double lam, phi, xsum, ysum, xterm, yterm; int i; bool cvg;
    lam = (pc -> lon - THETA0) * RADIANS;
    phi = pc -> lat * RADIANS;
    xsum = ysum = 0.0; xterm = lam; yterm = 1.0; cvg = false;
    for (i=0; i < NDERIVS-1 && !cvg; i += 2)
      { double xt = xterm * evalderiv(i+1, phi);
	double yt = yterm * evalderiv(i, phi);
	xsum += xt; ysum += yt;
	if (hypot(xt, yt) < EPS) cvg = true;
	xterm *= -(lam*lam); yterm *= -(lam*lam);
      }
    unless (cvg) giveup("Failed to converge!");
    cc -> x = X0 + (AIRY_A*SCALE) * xsum;
    cc -> y = Y0 + (AIRY_A*SCALE) * ysum - M0;
    cc -> n = i/2;
  }

static double evalderiv(n, phi) int n; double phi;
  { double *av = avec[n], *bv = bvec[n];
    double sum; int j;
    sum = linterm[n]*phi + bv[0];
    for (j=1; j < NHARMS; j++) sum = sum + (bv[j] * cos(j*phi)) + (av[j] * sin(j*phi));
    return sum;
  }

static pj_convert(pc, cc) struct pco *pc; struct cco *cc;
  { /* given lon, lat, compute OS coords x, y, by series (PROJ) method */
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

static giveup(msg, p1) char *msg; int p1;
  { fprintf(stderr, msg, p1); putc('\n', stderr);
    exit(1);
  }

