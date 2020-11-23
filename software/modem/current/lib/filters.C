/* Modem for MIPS   AJF	  January 1995
   Filter constructor & destructor */

#include "private.h"
#include "complex.h"
#include "filters.h"

filter::filter(fspec *xfs)
  { fs = xfs;
    int np = fs -> np;
    v = new float[np+1];
    for (int i = 0; i <= np; i++) v[i+1] = 0.0;
    sum = 0.0; ptr = 0;	    /* mvg avg filters only */
 }

filter::~filter()
  { delete v;
  }

