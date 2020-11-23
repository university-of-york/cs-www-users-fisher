#include <stdio.h>
#include <math.h>

#define TWOPI	    (2.0 * M_PI)
#define FREQ	    2000.0
#define SAMPLERATE  8000.0

main()
  { for (int i = 0; i < 1600; i++) printf("%g\n", 0.25 * sin(TWOPI * FREQ * (double) i / SAMPLERATE));
    exit(0);
  }

