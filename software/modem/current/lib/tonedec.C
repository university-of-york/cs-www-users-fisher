#include <stdio.h>

#include "private.h"
#include "complex.h"
#include "filters.h"
#include "tonedec.h"

#define THRESH 0.1	/* tone threshold */

inline float fsgn(float x) { return (x > 0.0f) ? +1.0f : (x < 0.0f) ? -1.0f : 0.0f; }
inline float sqr(float f)  { return f*f;					    }


tone_detector::tone_detector(fspec *fefs, fspec *bpfs, fspec *lpfs, bool lim)
  { fef = new filter(fefs);
    bpf = new filter(bpfs);
    lpf = new filter(lpfs);
    limit = lim;
    pow = 0.0; prescount = 0; present = false;
  }

tone_detector::~tone_detector()
  { delete fef; delete bpf; delete lpf;
  }

void tone_detector::insert(float x)
  { x = fef -> fstep(x);	    /* front-end filter to remove d.c. */
    if (limit) x = fsgn(x);	    /* hard limit */
    float y = bpf -> fstep(x);
    pow = lpf -> fstep(sqr(y));
    present = (pow >= THRESH);
    if (present) prescount++; else prescount = 0;
  }

void tone_detector::debug()
  { fprintf(stderr, "    %14.5e (%5d)", pow, prescount);
  }

