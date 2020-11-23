#include <stdio.h>

#define global

#define ALPHA	    0.25	/* BETA/Rb (Shanmugam p. 195) */
#define PI	    3.14159265358979323846
#define TWOPI	    (2.0 * PI)
#define SHAPELEN    40

extern "C" double sin(double), cos(double);

static double cosfac(double), sqr(double), sinc(double);


global int main()
  { for (int i=0; i <= 3*SHAPELEN; i++)
      { double x = PI * (double) i / (double) SHAPELEN;
	printf("%2d ", i);
	printf("cf=%14.10f ", cosfac(x));
	printf("sinc=%14.10f ", sinc(x));
	printf("shape=%14.10f ", cosfac(x) * sinc(x));
	putchar('\n');
      }
    return 0;
  }

static double cosfac(double x)
  { double top = cos(2.0*ALPHA*x);
    double bot = (1.0 - sqr((4.0*ALPHA*x) / PI));
    return (top == 0.0 && bot == 0.0) ? (PI/4.0) :	/* degenerate case */
	   top / bot;
  }

static double sqr(double x)
  { return x*x;
  }

static double sinc(double x)
  { return (x == 0.0) ? 1.0 :	/* degenerate case */
	   sin(x) / x;
  }

