#include <stdio.h>
#include <string.h>
#include <math.h>

#include "solvecct.h"
#include "complex.h"


global bool solve_eqns(complex **a, complex *b, complex *x, int size)
  { /* Solves "ax = b" for x by Gauss-Jordan method, from Hawgood p.86 */
    int *pivrow = new int[size], *pivcol = new int[size];
    int i, j, n;
    bool ok = true;
    for (i = 0; i < size; i++) pivrow[i] = pivcol[i] = i;
    for (n = 0; n < size && ok; n++)
      { int prn = pivrow[n], pcn = pivcol[n];
	complex pivot = a[prn][pcn];
	double pivpow = power(pivot);
	int io = n, jo = n;
	int bi = pivrow[n], bj = pivcol[n];
	for (i = n; i < size; i++)
	  { for (j = n; j < size; j++)
	      { int ic = pivrow[i], jc = pivcol[j];
		complex comp = a[ic][jc];
		double comppow = power(comp);
		if (comppow > pivpow)
		  { pivot = comp; pivpow = comppow;
		    bi = ic; io = i;
		    bj = jc; jo = j;
		  }
	      }
	  }
	if (pivpow == 0.0) ok = false;	/* singular */
	pivrow[io] = pivrow[n];
	pivcol[jo] = pivcol[n];
	pivrow[n] = bi; pivcol[n] = bj;
	b[bi] /= pivot;
	for (j = n+1; j < size; j++)
	  { int jc = pivcol[j];
	    a[bi][jc] /= pivot;
	  }
	for (i = 0; i < size; i++)
	  { unless (i == bi)
	      { complex mult = a[i][bj];
		b[i] -= (b[bi] * mult);
		for (j = n+1; j < size; j++)
		  { int jc = pivcol[j];
		    a[i][jc] -= (a[bi][jc] * mult);
		  }
	      }
	  }
      }
    for (i = 0; i < size; i++)
      { int pci = pivcol[i], pri = pivrow[i];
	x[pci] = b[pri];
      }
    delete pivrow; delete pivcol;
    return ok;
  }

