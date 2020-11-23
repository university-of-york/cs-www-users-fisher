#include <stdio.h>
#include <math.h>

#define global
#define unless(x)   if(!(x))

#define TWOPI (2.0 * M_PI)

typedef unsigned int uint;


global void main()
  { for (int j = 0; j < 64; j++)
      { double tot = 0.0;
	uint pm = 0x07a4402;	    /* says which cosines are inverted */
	for (int i = 0; i < 25; i++)
	  { unless (i == 5 || i == 7 || i == 11 || i == 15)
	      { double x = cos(TWOPI * (i+1) * j / 64.0);
		if (pm & 1) x = -x;
		tot += x;
	      }
	    pm >>= 1;
	  }
	printf("%10.7f, ", tot / 21.0);
	if (j%8 == 7) putchar('\n');
      }
    exit(0);
  }

