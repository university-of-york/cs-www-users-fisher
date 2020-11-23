/* Goertzel's algorithm for the discrete Fourier transform */

#include <math.h>

#include "private.h"
#include "complex.h"
#include "goertzel.h"
#include "myaudio.h"

#define TWOPI (2.0 * M_PI)

goertzel::goertzel(float f)
  { v1 = v2 = v3 = 0.0;
    float theta = TWOPI * (f / SAMPLERATE);
    w = complex(cos(theta), -sin(theta));
    fac = 2.0 * w.re;
  }

void goertzel::insert(float x)
  { v1 = v2; v2 = v3;
    v3 = fac*v2 - v1 + x;
  }

complex goertzel::result()
  { return complex(v3) - (w * complex(v2));
  }

