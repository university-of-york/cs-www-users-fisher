/* mkfilter -- given n, compute recurrence relation
   to implement Butterworth or Bessel filter of order n
   AJF	 September 1992 */

/* Code generation module */

#include <stdio.h>
#include "mkfilter.h"

#define FPSCALE	    15		/* 17:15 fixed point */
#define LOGE2	    0.69314718055994530941

extern int order, numpoles;
extern double raw_alpha, raw_dc_gain, raw_hf_gain, reqd_gain;
extern uint opts;
extern char *glabel;	/* label after -g */

extern double yfacs[];
extern int bcoeffs[];	/* binomial coefficients; e.g. 1,5,10,10,5,1 for order 5 */

extern double log();


global gencode()
  { int i;
    printf("# recursive digital %s-pass filter:\n", (opts & opt_h) ? "high" : "low");
    printf("# %d-pole %s", order, (opts & opt_be) ? "Bessel" : "Butterworth");
    if (numpoles < order) printf(", of which %d poles in this section", numpoles);
    putchar('\n');
    printf("# cutoff/sample = %.5g\n#\n", raw_alpha);
    /* define factors */
    for (i=0; i<numpoles; i++)
      printf("\tset\tfact%d,\t%6d\t# %+.8g\n", i+1, ifix(yfacs[i] * (1 << FPSCALE)), yfacs[i]);
    printf("#\n\tglobal\t%s\n#\n", glabel);
    printf("%s:\n", glabel);
    printf("\tmovm.l\t&0x3000,-(%%sp)\n");      /* save %d2 - %d3 */
    if (opts & opt_c)
      { printf("\tmov.l\t12(%%sp),%%a0\n");     /* fetch addr of Xs */
	printf("\tmov.l\t16(%%sp),%%a1\n");     /* fetch addr of Ys */
      }
    scalevalue();
    fetchy(0, 0);		    /* fetch Y0 in %d0 */
    addx(0, 0);			    /* add X0	       */
    addx(numpoles, 0);		    /* add Xn	       */
    for (i=1; i < (numpoles+1)/2; i++)
      { /* fetch [Yi + Y(n-i) in %d2 */
	fetchy(i, 1);		    /* fetch Yi in %d1 */
	fetchy(numpoles-i, 2);	    /* fetch Y(n-i) in %d2 */
	printf("\tadd.l\t%%d1,%%d2\n");
	addx(i, 2);		    /* add Xi	       */
	addx(numpoles-i, 2);	    /* add X(n-i)      */
	multk(bcoeffs[i], 2, 1);    /* times xco */
	printf("\tadd.l\t%%d1,%%d0\n");
      }
    unless (numpoles & 1)
      { int n = numpoles/2;	    /* deal with middle term */
	fetchy(n, 2);
	addx(n, 2);
	multk(bcoeffs[n], 2, 1);    /* times xco */
	printf("\tadd.l\t%%d1,%%d0\n");
      }
    /* cvt from fixed point to integer, and store it */
    rshift(FPSCALE, 0);
    printf("\tmov.l\t%%d0,%d(%%a1)\n", 4*numpoles);     /* store as new Yn */
    /* shift them up */
    shiftup(0); /* Xi */
    shiftup(1); /* Yi */
    printf("\tmovm.l\t(%%sp)+,&0x000c\n");
    printf("\trts\n\n");
  }

static shiftup(an) int an;
  { int i;
    for (i=0; i<numpoles; i++)
      printf("\tmov.l\t%d(%%a%d),%d(%%a%d)\n", 4*(i+1), an, 4*i, an);
  }

static scalevalue()
  { double gain = reqd_gain / ((opts & opt_h) ? raw_hf_gain : raw_dc_gain);
    int sh = ifix(log(gain) / LOGE2) + FPSCALE;
    if (sh >= 32 || sh <= -32)
      { fprintf(stderr, "mkfilter: gain too high!  requires shift by %d places to normalize\n", sh);
	exit(1);
      }
    unless (sh == 0)
      { printf("\tmov.l\t%d(%%a0),%%d0\n", 4*numpoles); /* normalize Xn */
	if (sh > 0) lshift(sh, 0);
	if (sh < 0) rshift(-sh, 0);
	printf("\tmov.l\t%%d0,%d(%%a0)\n", 4*numpoles);
      }
  }

static fetchy(yn, dn) int yn, dn;
  { /* Dn = Yn * fact(n+1) */
    printf("\tmov.w\t%d(%%a1),%%d%d\n", 4*yn + 2, dn);  /* fetch low-order half of longword */
    printf("\tmuls.w\t&fact%d,%%d%d\n", yn+1, dn);
  }

static addx(xn, dn) int xn, dn;
  { printf("\tadd.l\t%d(%%a0),%%d%d\n", 4*xn, dn);      /* Dn += Xn */
  }

static multk(k, dn1, dn2) int k, dn1, dn2;
  { /* 32-bit multiply by a constant : src is in dn1; result to go in dn2 */
    if (countbits(k) > 1) xmultk(1, dn1, 3);
    xmultk(k, dn1, dn2);
  }

static xmultk(k, dn1, dn2) int k, dn1, dn2;
  { if (k == 0) printf("\tclr.l\t%%d%d\n", dn2);
    else
      { int nz = 0;
	until (k & 1) { k /= 2; nz++; }
	if (k == 1) printf("\tmov.l\t%%d%d,%%d%d\n", dn1, dn2);
	else
	  { if (countbits(k-1) <= countbits(k+1))
	      { xmultk(k-1, dn1, dn2);
		printf("\tadd.l\t%%d%d,%%d%d\n", 3, dn2);
	      }
	    else
	      { xmultk(k+1, dn1, dn2);
		printf("\tsub.l\t%%d%d,%%d%d\n", 3, dn2);
	      }
	  }
	if (nz > 0) lshift(nz, dn2);
      }
  }

static int countbits(n) int n;
  { int nb = 0;
    until (n == 0) { n &= (n-1); nb++; }
    return nb;
  }

static lshift(sh, reg) int sh, reg;
  { printf("\tasl.l\t&%d,%%d%d\n", sh, reg);
  }

static rshift(sh, reg) int sh, reg;
  { printf("\tadd.l\t&%d,%%d%d\n", 1 << (sh-1), reg);   /* N.B. rounding correct for + or - sign */
    printf("\tasr.l\t&%d,%%d%d\n", sh, reg);
  }

