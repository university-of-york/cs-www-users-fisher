/* tmprog - compute TM projection by AJF's FFT method
   A.J. Fisher	 May 1995 */

#include <stdio.h>
#include "hdr.h"
#include "projconsts.h"

static derivatives derivs;
static double alphadelta[NHARMS];	/* delta for odd indices, alpha for even indices */
static int bign;

static complex czero = { 0.0, 0.0 };

static void setup(int, char*[]), usage();
static void testlonlat(), makeabgd(int);
static double meridist(double);


global int main(int argc, char *argv[])
  { setup(argc, argv);
    makederivs("tm.harms", &derivs);
    testlonlat();
    return 0;
  }

static void setup(int argc, char *argv[])
  { unless (argc == 2) usage();
    bign = atoi(argv[1]);
    fprintf(stderr, "bign=%d\n", bign);
  }

static void usage()
  { fprintf(stderr, "Usage: tmprog bign\n");
    exit(1);
  }

static void testlonlat()
  { /* for (int ilon = -8; ilon <= +8; ilon++) */
    for (int ilon = 2; ilon <= 2; ilon += 2)
      { complex fftvec[MAXDATA+1]; /* extra slot reqd for filling-in loop */
	cco myccos[2][MAXDATA]; int i, j;
	for (i=0; i < bign; i++) fftvec[i] = czero;
	makeabgd(ilon);
	for (i=0; i < NHARMS; i++)
	  { fftvec[i].im = alphadelta[i];
	    fftvec[bign-i].im = (i&1) ? alphadelta[i] : -alphadelta[i];
	  }
	makeabgd(ilon+1);
	for (i=0; i < NHARMS; i++)
	  { fftvec[i].re = -alphadelta[i];
	    fftvec[bign-i].re = (i&1) ? -alphadelta[i] : alphadelta[i];
	  }
	dofft(fftvec, bign, -1, 0, bign); /* fwd fft */
	fftvec[bign] = fftvec[0];
	for (i=0; i < bign; i++)
	  { double lt = derivs.linterm * TWOPI * ((double) i / (double) bign);
	    myccos[0][i].y = (Y0-M0) + ARAD * (lt + (0.25 * (fftvec[i].re - fftvec[bign-i].re)));
	    myccos[0][i].x = X0 + ARAD * (0.25 * (fftvec[i].im + fftvec[bign-i].im));
	    myccos[1][i].y = (Y0-M0) + ARAD * (lt + (0.25 * (fftvec[i].im - fftvec[bign-i].im)));
	    myccos[1][i].x = X0 + ARAD * (0.25 * -(fftvec[i].re + fftvec[bign-i].re));
	  }
	for (j=0; j < 2; j++)
	  { for (i=0; i < bign; i++)
	      { pco pco = { (double) (ilon+j), 360.0 * ((double) i / (double) bign) };
		cco pjcco;
		tm_convert(&pco, &pjcco);
		printline(&pco, &myccos[j][i], &pjcco);
	      }
	  }
      }
  }

static void makeabgd(int ilon)
  { int k;
    int opcount = 1; /* assume (lam*lam) eval outside loop */
    for (k=0; k < NHARMS; k++)
      { double lam, adsum, term; int i; bool cvg;
	lam = (ilon - THETA0) * RADIANS;
	adsum = 0.0; term = 1.0; cvg = false;
	for (i=0; i < NDERIVS-1 && !cvg; i += 2)
	  { double t = (k&1) ? lam * term * derivs.bvec[i+1][k] : term * derivs.avec[i][k];
	    adsum += t;
	    if (fabs(t) < 1e-12) cvg = true;
	    term *= -(lam*lam);
	    opcount += (k&1) ? 3 : 2;
	  }
	/* fprintf(stderr, "ilon %3d, harm %2d, %2d iterations\n", ilon, k, i/2); /* ?? */
	unless (cvg) giveup("Failed to converge! (makeabgd)");
	alphadelta[k] = adsum;
      }
    fprintf(stderr, "??? %d ops in makeabgd\n", opcount);
  }

