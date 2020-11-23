#include <stdio.h>

#include "private.h"
#include "complex.h"
#include "filters.h"
#include "phrdec.h"

inline int isgn(float f) { return (f > 0.0f) ? +1 : (f < 0.0f) ? -1 : 0;	  }

phr_detector::phr_detector(fspec *bpfs, fspec *lpfs)
  { bpf = new filter(bpfs);
    lpf = new filter(lpfs);
    pol2 = +1;	/* pol2 must not be init'ed to zero */
  }

phr_detector::~phr_detector()
  { delete bpf; delete lpf;
  }

void phr_detector::insert(float x)
  { pol1 = pol2;
    float y = bpf -> fstep(x*pol1);
    phase = lpf -> fstep(x*y);
    pol2 = isgn(phase);
  }

