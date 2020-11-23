#include "filters.h"

filter::filter(fspec *xfs)
  { fs = xfs;
    int np = fs -> np;
    v = new float[np+1];
    for (int i=0; i < np; i++) v[i+1] = 0.0;
 }

filter::~filter()
  { delete v;
  }

