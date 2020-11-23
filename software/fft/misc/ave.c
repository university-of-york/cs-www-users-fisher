#define NUMI 31
#define NUMJ 512

main ()
  { int i;
    double data[NUMJ];
    for (i=0; i<NUMJ; i++) data[i] = 0.0;
    for (i=0; i<NUMI; i++)
      { int j;
	for (j=0; j<NUMJ; j++)
	  { double val;
	    scanf (" %lf", &val);
	    data[j] += val;
	  }
      }
    for (i=0; i<NUMJ; i++) printf ("%lf\n", data[i]);
  }
