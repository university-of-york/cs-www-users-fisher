/* G2PSG parser generator   A.J. Fisher	  September 1990 */
/* Common routines */

#include "g2psg.h"
#include <stdio.h>

extern char *malloc();


global scantree(p,z,p1) proc p; struct dict *z; int p1;
  { unless (z == NULL)
      { scantree(p, z -> llk, p1);
	p(z,p1);
	scantree(p, z -> rlk, p1);
      }
  }

global setbit(k,m) int k; uint *m;
  { int i = k/32, j = k%32;
    m[i] |= (1 << j);
  }

global bool testbit(k, m) int k; uint *m;
  { int i = k/32, j = k%32;
    return m[i] & (1 << j);
  }

global char *cheap(nb) int nb;
  { char *x = malloc(nb);
    if (x == NULL)
      { fprintf(stderr, "g2psg: no room\n");
	exit(1);
      }
    return x;
  }
