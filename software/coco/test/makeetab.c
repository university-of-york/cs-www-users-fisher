#include <stdio.h>

#define global

#define ECC2 6.670540000123428e-3	/* eccentricity squared, = (a^2-b^2)/a^2 */

#define ECC4 (ECC2*ECC2)
#define ECC6 (ECC4*ECC2)
#define ECC8 (ECC6*ECC2)

#define POLY0(x,a0)		(a0)
#define POLY1(x,a1,a0)		(POLY0(x,a1) * x + a0)
#define POLY2(x,a2,a1,a0)	(POLY1(x,a2,a1) * x + a0)
#define POLY3(x,a3,a2,a1,a0)	(POLY2(x,a3,a2,a1) * x + a0)
#define POLY4(x,a4,a3,a2,a1,a0) (POLY3(x,a4,a3,a2,a1) * x + a0)


global main()
  { preps();
    prmeps();
    exit(0);
  }

static preps()
  { printf("EPSK = %24.16e\n",        POLY4(ECC2, -175, -320, -768, -4096, 16384) / 16384.0);
    printf("EPS0 = %24.16e\n", ECC2 * POLY3(ECC2, -175, -320, -768, 12288)        / 16384.0);
    printf("EPS1 = %24.16e\n", ECC4 * POLY2(ECC2, -175, -320, 11520)              / 24576.0);
    printf("EPS2 = %24.16e\n", ECC6 * POLY1(ECC2, -35, 2240)                      / 6144.0);
    printf("EPS3 = %24.16e\n", ECC8 * POLY0(ECC2, 315)                            / 1024.0);
    putchar('\n');
  }

static prmeps()
  { printf("MEPSK = %24.16e\n",        POLY4(ECC2, -175, -320, -768, -4096, 16384) / 16384.0);
    printf("MEPS2 = %24.16e\n", ECC2 * POLY2(ECC2, 45, 96, 384)                    / 1024.0);
    printf("MEPS4 = %24.16e\n", ECC4 * POLY1(ECC2, 45, 60)                         / 1024.0);
    printf("MEPS6 = %24.16e\n", ECC6 * POLY0(ECC2, 35)                             / 3072.0);
    putchar('\n');
  }

