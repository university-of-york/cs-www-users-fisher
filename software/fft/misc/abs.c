#define global
#define unless(x) if(!(x))

#include <stdio.h>

extern double atof();


global main(argc, argv) int argc; char *argv[];
  { double x; int nd;
    nd = scanf(" %lf", &x);
    while (nd == 1)
      { printf(x >= 0.0 ? "+1" : "-1");
	nd = scanf(" %lf", &x);
      }
  }

