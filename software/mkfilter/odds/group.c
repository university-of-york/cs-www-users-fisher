static double yfacs[MAXPOLES+1];    /* coeffs. of Yi */
static int bcoeffs[MAXPOLES+1];	    /* binomial coefficients; e.g. 1,5,10,10,5,1 for order 5 */

static printgrouped() /* print recurrence relation with terms grouped for efficiency */
  { int i;
    printf("y[n] = ");
    for (i=0; i < numpoles+1; i++)
      { if (i > 0) printf("     + ");
	printf("%3d * (x[n-%2d]", bcoeffs[i], numpoles-i);
	if (i < numpoles)
	  { double f = yfacs[i];
	    printf((f >= 0.0) ? " + " : " - ");
	    printf("%14.10f * y[n-%2d]", fabs(yfacs[i]), numpoles-i);
	  }
	printf(")\n");
      }
    putchar('\n');
  }

static groupterms()
  { /* given zbotcoeffs, ztopcoeffs, compute yfacs, bcoeffs */
    bool fail = false; int i;
    for (i=0; i < numpoles+1; i++)
      { double xco = ztopcoeffs[i].re / zbotcoeffs[numpoles].re;
	double yco = - (zbotcoeffs[i].re / zbotcoeffs[numpoles].re);
	bcoeffs[i] = ifix(xco); /* shd be an integer */
	/* check computed coeff of x[i] is an integer */
	if (fabs(bcoeffs[i] - xco) > EPS)
	  { fprintf(stderr, "mkfilter: bug: coeff of x[%d] = %g shd be an integer!\n", i, xco);
	    fail = true;
	  }
	if (false && fabs(yco) >= fabs(xco)) ???
	  { fprintf(stderr, "mkfilter: bug: yfacs[%d] = %g/%g is out of range -1 .. +1!\n", i, yco, xco);
	    fail = true;
	  }
	else yfacs[i] = yco/xco;
      }
    if (fail) exit(1);
  }


