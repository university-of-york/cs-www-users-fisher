#include <stdio.h>

#include "private.h"
#include "complex.h"
#include "debug.h"

co_debugger::co_debugger(int xl)
  { len = xl; ptr = 0;
    vec = new complex[len];
  }

void co_debugger::insert(complex z)
  { if (ptr < len) vec[ptr++] = z;
  }

co_debugger::~co_debugger()
  { delete vec;
  }

void co_debugger::print(char *fn)
  { FILE *fi = fopen(fn, "w");
    if (fi != NULL)
      { int n = 0;
	while (n < ptr)
	  { int n1 = n + 500;
	    if (n1 > ptr) n1 = ptr;
	    fprintf(fi, ".sp 0.5i\n");
	    fprintf(fi, "%d ... %d\n", n, n1-1);
	    fprintf(fi, ".G1 8i\n");
	    for (int i = n; i < n1; i++) fprintf(fi, "%g %g\n", vec[i].re, vec[i].im);
	    fprintf(fi, ".G2\n.bp\n");
	    n = n1;
	  }
	fclose(fi);
      }
    else fprintf(stderr, "can't create %s\n", fn);
  }

