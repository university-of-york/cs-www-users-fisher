#include <stdio.h>

#define global

#define BETA	    1.0		/* rolloff factor (see e.g. Proakis) */
#define PI	    3.14159265358979323846
#define TWOPI	    (2.0 * PI)
#define NUMT	    2
#define SHAPELEN    10

extern "C" double sin(double), cos(double);

static double cosfac(double), sqr(double), sinc(double);

inline int iabs(int n) { return (n >= 0) ? +n : -n; }


global int main()
  { double shape[NUMT*SHAPELEN+1];
    for (int i=0; i <= NUMT*SHAPELEN; i++)
      { double x = PI * (double) i / (double) SHAPELEN;
	shape[i] = cosfac(x) * sinc(x);
      }
    printf("static float shapetab[%d] =\n", 2*NUMT*SHAPELEN+1);
    printf("  { /* Raised cosine pulse shaping with Beta = 1.0 (see e.g. Proakis) */\n");
    for (int i = -NUMT*SHAPELEN; i <= +NUMT*SHAPELEN; i++)
      { if ((i+5000)%5 == 0) printf("   ");
	printf("%14.10f,", shape[iabs(i)]);
	if ((i+5000)%5 == 4) putchar('\n');
      }
    printf("\n  };\n");
    return 0;
  }

static double cosfac(double x)
  { double bx = BETA * x;
    double top = cos(bx), bot = 1.0 - sqr(2.0 * bx / PI);
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

