/* testpolos -- test polar-to-OS routine
   A.J. Fisher	 May 1995 */

#include <stdio.h>

#define global
#define forward
#define bool	    int
#define false	    0
#define true	    1
#define ushort	    unsigned short

#define unless(x)   if(!(x))
#define until(x)    while(!(x))

#define PI	    3.1415926535897932384626433
#define TWOPI	    (2.0 * PI)

struct complex { double re, im };

typedef double (*func)();

#define AIRY_A	    6377563.396			/* radius of earth (semi-major axis)	     */
#define AIRY_B	    6356256.910			/* radius of earth (semi-minor axis)	     */
#define ESQUARED    6.670540000123428e-3	/* eccentricity squared, = (a^2-b^2)/a^2     */
#define SCALE	    0.9996012717		/* scale factor on central meridian	     */
#define PHI1	    55.0			/* for LCC, = phi2			     */

#define ARAD	    (AIRY_A*SCALE)
#define RADIANS	    (PI / 180.0)

#define NUMPOINTS   8192 /* 65536 */

#define POLY0(x,a0)		(a0)
#define POLY1(x,a1,a0)		(POLY0(x,a1) * x + a0)
#define POLY2(x,a2,a1,a0)	(POLY1(x,a2,a1) * x + a0)
#define POLY3(x,a3,a2,a1,a0)	(POLY2(x,a3,a2,a1) * x + a0)

extern double sin(), cos(), tan(), atan(), pow(), sqrt(), hypot(), fabs();
extern struct complex cminus();

static struct complex czero = { 0.0, 0.0 };

forward double cvert(), meridist(), msfn(), tsfn();
forward double tm_convert(), lcc_convert(), st_convert(), real_st_convert();

static func convert;


global main(argc, argv) int argc; char *argv[];
  { setup(argc, argv);
    dothing();
    exit(0);
  }

static setup(argc, argv) int argc; char *argv[];
  { unless (argc == 2) usage();
    switch (argv[1][0])
      { default:
	    usage();

	case 'T':
	    convert = tm_convert;
	    break;

	case 'L':
	    convert = lcc_convert;
	    break;

	case 'S':
	    convert = st_convert;
	    break;
      }
  }

static usage()
  { fprintf(stderr, "Usage: mkseries [TLS]\n");
    exit(1);
  }

static dothing()
  { int i, j; double fmin, fmax, slope;
    struct complex fvec[NUMPOINTS];
    fmin = cvert(0.0); fmax = cvert(360.0);
    slope = (fmax-fmin) / (double) NUMPOINTS;
    for (i = 0; i < NUMPOINTS; i++)
      { double f = cvert((double) i * (360.0 / (double) NUMPOINTS));
	fvec[i].re = f - slope * (double) i;
	fvec[i].im = 0.0;
      }
    for (i=0; i < NUMPOINTS; i++) fvec[i].re += fmin;
/* #ifdef NOTDEF */
    for (i=0; i < NUMPOINTS; i++)
      { double phi = (double) i * (360.0 / (double) NUMPOINTS);
	printf("%8.2f   %24.16e\n", phi, fvec[i]);
      }
    exit(0);	/* ??? */
/* #endif */
    dofft(fvec, NUMPOINTS, +1); /* inverse fft */
    for (i=0; i < NUMPOINTS; i++)
      { double fac = 2.0 / NUMPOINTS;
	fvec[i].re *= fac;
	fvec[i].im *= fac;
      }
    slope *= (double) NUMPOINTS / (TWOPI);
    printf("%24.16e\n", slope);
    for (i=0; i < 100; i++) printf("%24.16e   %24.16e\n", fvec[i].re, fvec[i].im);
#ifdef NOTDEF
    for (i=0; i < NUMPOINTS; i++)
      { double f = cvert((double) i * (360.0 / (double) NUMPOINTS));  /* the "correct" value */
	double sum = slope * TWOPI * ((double) i / (double) NUMPOINTS);
	for (j=0; j < NUMPOINTS/2; j++)
	  { double phi = TWOPI * (double) (i*j) / (double) NUMPOINTS;
	    sum += (fvec[j].re * cos(phi) + fvec[j].im * sin(phi));
	  }
	printf("%24.16e   %24.16e\n", sum, f);
      }
#endif
    exit(0);
  }

static double cvert(phid) double phid;
  { double y;
    y = convert(phid);
    /* fprintf(stderr, "*** phid=%g   y=%g\n", phid, y); */
    return y;
  }

static double tm_convert(phid) double phid;
  { return meridist(phid*RADIANS);
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
    return (EPSK*phi - p*cs);
  }

static double lcc_convert(phid) double phid;
  { double phi, n, rho0, rho;
    phi = phid * RADIANS;
    n = sin(PHI1*RADIANS);
    rho0 = msfn(PHI1*RADIANS) / n;
    rho = rho0 * pow(tsfn(phi) / tsfn(PHI1*RADIANS), n);
    return rho0 - rho;
  }

static double st_convert(phid) double phid;
  { return (phid >= 90.0) ? 4.0 + st_convert(phid-90.0) :
	   real_st_convert(phid);
  }

static double real_st_convert(phid) double phid;    /* equatorial stereographic */
  { double phi, sinphi, tx, ta, stx, ctx;
    phi = phid * RADIANS;
    sinphi = sin(phi);
    tx = 2.0 * atan(tsfn(-phi)) - 0.5*PI;
    stx = sin(tx); ctx = cos(tx);
    ta = 4.0 / (1.0 + ctx);
    return ta*stx;
  }

static double msfn(phi) double phi;
  { double sph = sin(phi), cph = cos(phi);
    return cph / sqrt(1.0 - ESQUARED*sph*sph);
  }

static double tsfn(phi) double phi;
  { double e = sqrt(ESQUARED);
    double esph = e*sin(phi);
    return tan(0.25*PI - 0.5*phi) * pow((1.0+esph) / (1.0-esph), 0.5*e);
  }

static giveup(msg, p1) char *msg; int p1;
  { fprintf(stderr, msg, p1); putc('\n', stderr);
    exit(1);
  }

