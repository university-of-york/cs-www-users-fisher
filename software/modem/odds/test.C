#include <stdio.h>

extern "C" double floor(double);

main()
  { printf("int(+42.1) = %d\n", (int) +42.1);
    printf("int(-42.1) = %d\n", (int) -42.1);
    printf("int(+42.9) = %d\n", (int) +42.9);
    printf("int(-42.9) = %d\n", (int) -42.9);
    printf("floor(+42.1) = %g\n", floor(+42.1));
    printf("floor(-42.1) = %g\n", floor(-42.1));
    printf("floor(+42.9) = %g\n", floor(+42.9));
    printf("floor(-42.9) = %g\n", floor(-42.9));
    exit(0);
  }

