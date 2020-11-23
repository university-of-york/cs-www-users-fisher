#include "private.h"
#include "scramble.h"

global int scrambler::fwd(int ib)
  { int ob = ib ^ bit(tap) ^ bit(23);
    wd = (wd >> 1) | (ob << 22);
    return ob;
  }

global int scrambler::rev(int ib)
  { int ob = ib ^ bit(tap) ^ bit(23);
    wd = (wd >> 1) | (ib << 22);
    return ob;
  }

