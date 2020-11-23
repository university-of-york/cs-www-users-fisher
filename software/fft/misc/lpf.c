/* lpf -- Low Pass Filter */

#define global
#define unless(x) if(!(x))

#include <stdio.h>

extern double atof();


global main(argc, argv) int argc; char *argv[];
  { double x, y, alpha; int nd;
    unless (argc == 2)
      { fprintf(stderr, "Usage: lpf alpha\n");
	exit(1);
      }
    alpha = atof(argv[1]);
    y = 0.0;
    nd = scanf(" %lf", &x);
    while (nd == 1)
      { y = (1.0-alpha)*y + alpha*x;
	printf("%10g\n", y);
	nd = scanf(" %lf", &x);
      }
  }

