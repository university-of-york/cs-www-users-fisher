#include <stdio.h>
#include <math.h>

main ()
  { int i;
    for (i=0; i<2048; i++)
      { double sum = 0.0;
	int j;
	for (j=1; j<200; j++) sum += sin(j*i*0.01);
	printf ("%lf\n", sum);
      }
  }
