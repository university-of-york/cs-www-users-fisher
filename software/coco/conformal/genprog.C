/* testpolos -- test polar-to-OS routine
   A.J. Fisher	 May 1995 */

#include <stdio.h>
#include "hdr.h"
#include "projconsts.h"

static derivatives derivs;
static double alpha[NHARMS], beta[NHARMS], gamma[NHARMS], delta[NHARMS];
static int bign, offset, numwanted;

static complex czero = { 0.0, 0.0 };

typedef void (*cvproc)(pco*, cco*);
static cvproc convert;

static char *derivfn;

static void setup(int, char*[]), usage();
static void testlonlat(), makeabgd(int);


global int main(int argc, char *argv[])
  { setup(argc, argv);
    makederivs(derivfn, &derivs);
    testlonlat();
    return 0;
  }

static void setup(int argc, char *argv[])
  { unless (argc == 5) usage();
    switch (argv[1][0])
      { default:
	    usage();

	case 'T':
	    convert = tm_convert;
	    derivfn = "tm.harms";
	    break;

	case 'L':
	    convert = lcc_convert;
	    derivfn = "lcc.harms";
	    break;

	case 'S':
	    convert = st_convert;
	    derivfn = "stereo.harms";
	    break;
      }
    bign = atoi(argv[2]); offset = atoi(argv[3]); numwanted = atoi(argv[4]);
    fprintf(stderr, "bign=%d offset=%d numwanted=%d\n", bign, offset, numwanted);
  }

static void usage()
  { fprintf(stderr, "Usage: genprog [TLS] bign offs numw\n");
    exit(1);
  }

static void testlonlat()
  { /* for (int ilon = -8; ilon <= +8; ilon++) */
    for (int ilon = 2; ilon <= 2; ilon++)
      { complex fftvec[MAXDATA+1]; /* extra slot reqd for filling-in loop */
	cco myccos[MAXDATA]; int i;
	makeabgd(ilon);
	for (i=0; i < bign; i++) fftvec[i] = czero;
	for (i=0; i < NHARMS; i++)
	  { fftvec[i].re = beta[i] - gamma[i];
	    fftvec[i].im = delta[i] + alpha[i];
	    fftvec[bign-i].re = beta[i] + gamma[i];
	    fftvec[bign-i].im = delta[i] - alpha[i];
	  }
	dofft(fftvec, bign, -1, offset, numwanted); /* fwd fft */
	for (i=0; i < bign; i++)
	  { double lt = derivs.linterm * TWOPI * ((double) i / (double) bign);
	    myccos[i].y = (Y0-M0) + ARAD * (lt + (0.5 * fftvec[i].re));
	    myccos[i].x = X0 + ARAD * (0.5 * fftvec[i].im);
	  }
	for (i=0; i < numwanted; i++)
	  { pco pco = { (double) ilon, 360.0 * ((double) (offset+i) / (double) bign) };
	    cco pjcco;
	    convert(&pco, &pjcco);
	    printline(&pco, &myccos[i], &pjcco);
	  }
      }
  }

static void makeabgd(int ilon)
  { int k;
    int opcount = 1; /* assume (lam*lam) eval outside loop */
    for (k=0; k < NHARMS; k++)
      { double lam, asum, bsum, gsum, dsum, term; int i; bool cvg;
	lam = (ilon - THETA0) * RADIANS;
	asum = bsum = gsum = dsum = 0.0; term = 1.0; cvg = false;
	for (i=0; i < NDERIVS-1 && !cvg; i += 2)
	  { double at, bt, gt, dt;
	    at = term * derivs.avec[i][k];
	    bt = term * derivs.bvec[i][k];
	    gt = lam * term * derivs.avec[i+1][k];
	    dt = lam * term * derivs.bvec[i+1][k];
	    asum += at; bsum += bt; gsum += gt; dsum += dt;
	    if (fabs(at) + fabs(bt) + fabs(gt) + fabs(dt) < 1e-12) cvg = true;
	    term *= -(lam*lam);
	    opcount += 7;
	  }
	/* fprintf(stderr, "ilon %3d, harm %2d, %2d iterations\n", ilon, k, i/2); /* ?? */
	unless (cvg) giveup("Failed to converge! (makeabgd)");
	alpha[k] = asum; beta[k] = bsum; gamma[k] = gsum; delta[k] = dsum;
      }
    fprintf(stderr, "??? %d ops in makeabgd\n", opcount);
  }

