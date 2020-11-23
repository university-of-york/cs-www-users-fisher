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
#define PHI0	    49.0			/* for TM				     */
#define PHI1	    55.0			/* for LCC, = phi0 = phi2		     */

#define ARAD	    (AIRY_A * SCALE)

#define PI	    3.1415926535897932384626433
#define RADIANS	    (PI / 180.0)

#define POLY0(x,a0)		(a0)
#define POLY1(x,a1,a0)		(POLY0(x,a1) * x + a0)
#define POLY2(x,a2,a1,a0)	(POLY1(x,a2,a1) * x + a0)
#define POLY3(x,a3,a2,a1,a0)	(POLY2(x,a3,a2,a1) * x + a0)

extern double sin(), cos(), tan(), pow(), sqrt(), hypot();

struct pco { double lon, lat };	    /* polar coords (latitude, longitude)   */
struct cco { double x, y };	    /* Cartesian coords (easting, northing) */

typedef (*proc)();

static proc my_convert, pj_convert;

forward double meridist(), msfn(), tsfn();
forward my_tm_convert(), pj_tm_convert();
forward my_lcc_convert(), pj_lcc_convert();
forward my_st_convert(), pj_st_convert();


global main(argc, argv) int argc; char *argv[];
  { setup(argc, argv);
    testlonlat();
    exit(0);
  }

static setup(argc, argv) int argc; char *argv[];
  { unless (argc == 2) usage();
    switch (argv[1][0])
      { default:
	    usage();

	case 'T':
	    my_convert = my_tm_convert;
	    pj_convert = pj_tm_convert;
	    break;

	case 'L':
	    my_convert = my_lcc_convert;
	    pj_convert = pj_lcc_convert;
	    break;

	case 'S':
	    my_convert = my_st_convert;
	    pj_convert = pj_st_convert;
	    break;
      }
  }

static usage()
  { fprintf(stderr, "Usage: testpolos [TLS]\n");
    exit(1);
  }

static testlonlat()
  { int ilat, ilon;
    double mindx = +1e10, maxdx = -1e10;
    double mindy = +1e10, maxdy = -1e10;
    double maxdh = -1e10;
    for (ilon = -6; ilon <= +2; ilon++)
      { for (ilat = 0; ilat <= 90; ilat++)
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
  }

static my_tm_convert(pc, cc) struct pco *pc; struct cco *cc;
  { /* given lon, lat, compute OS coords x, y, by series (PROJ) method */
    double lam, phi, sph, cph, tph, tph2, lcph, lsph, lcph2, nu, eta2, poly;
    lam = (pc -> lon - THETA0) * RADIANS;
    phi = pc -> lat * RADIANS;
    sph = sin(phi); cph = cos(phi); tph = sph/cph; tph2 = tph*tph;
    lcph = lam*cph; lsph = lam*sph; lcph2 = lcph*lcph;
    nu = ARAD / sqrt(1.0 - ESQUARED*sph*sph); /* R of C perp to meridian */
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
    return ARAD * (EPSK*phi - p*cs);
  }

static my_lcc_convert(pc, cc) struct pco *pc; struct cco *cc;
  { double lam, phi, n, rho0, rho;
    lam = (pc -> lon - THETA0) * RADIANS;
    phi = pc -> lat * RADIANS;
    n = sin(PHI1*RADIANS);
    rho0 = msfn(PHI1*RADIANS) / n;
    rho = rho0 * pow(tsfn(phi) / tsfn(PHI1*RADIANS), n);
    cc -> x = rho * sin(n*lam);
    cc -> y = rho0 - rho * cos(n*lam);
  }

static double msfn(phi) double phi;
  { double sph = sin(phi), cph = cos(phi);
    return ARAD * cph / sqrt(1.0 - ESQUARED*sph*sph);
  }

static double tsfn(phi) double phi;
  { double e = sqrt(ESQUARED);
    double esph = e*sin(phi);
    return tan(0.25*PI - 0.5*phi) * pow((1.0+esph) / (1.0-esph), 0.5*e);
  }

static my_st_convert(pc, cc) struct pco *pc; struct cco *cc;	/* equatorial stereographic */
  { my_tm_convert(pc, cc);
  }

static pj_tm_convert(pc, cc) struct pco *pc; struct cco *cc;
  { char cmd[256]; int len;
    sprintf(cmd, "echo '%.4f %.4f' | proj -f %%.6f +proj=tmerc +ellps=airy ", pc -> lon, pc -> lat);
    len = strlen(cmd);
    sprintf(&cmd[len], "+lat_0=%.0f +lon_0=%.0f +k_0=%.10f +x_0=%.0f +y_0=%.0f", PHI0, THETA0, SCALE, X0, Y0);
    obeypj(cmd, cc);
  }

static pj_lcc_convert(pc, cc) struct pco *pc; struct cco *cc;
  { char cmd[256]; int len;
    sprintf(cmd, "echo '%.4f %.4f' | proj -f %%.6f +proj=lcc +ellps=airy ", pc -> lon, pc -> lat);
    len = strlen(cmd);
    sprintf(&cmd[len], "+lat_0=%.0f +lat_1=%.0f +lat_2=%.0f +lon_0=%.0f +k_0=%.10f", PHI1, PHI1, PHI1, THETA0, SCALE);
    obeypj(cmd, cc);
  }

static pj_st_convert(pc, cc) struct pco *pc; struct cco *cc;	/* equatorial stereographic */
  { char cmd[256]; int len;
    sprintf(cmd, "echo '%.4f %.4f' | proj -f %%.6f +proj=stere +ellps=airy ", pc -> lon, pc -> lat);
    len = strlen(cmd);
    sprintf(&cmd[len], "+lat_0=0 +lon_0=%.0f +k_0=%.10f", THETA0, SCALE);
    obeypj(cmd, cc);
  }

static obeypj(cmd, cc) char *cmd; struct cco *cc;
  { FILE *pfi; int ni;
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

