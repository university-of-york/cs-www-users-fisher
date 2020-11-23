#define global
#define unless(x)	if (!(x))
#define until(x)	while (!(x))

#define PI		3.14159265358979324
#define TWOPI		(2.0 * PI)
#define MAXSTR		256
#define MAXSEQLEN	8192

typedef void (*proc)();
typedef unsigned char uchar;

struct complex
  { complex() { };
    complex (double r, double i = 0.0)
      { re = r;
	im = i;
      }
    double re, im;
  };

extern void initcompute(int), compute(complex[], complex[]);   /* from compute */
extern int computedelay(complex[]);			       /* from compute */

extern double cmag(complex), carg(complex), creal(complex), cimag(complex);	/* from complex */
extern complex cconj(complex), expj(double);					/* from complex */
extern complex operator + (complex, complex), operator - (complex, complex);	/* from complex */
extern complex operator * (complex, complex), operator / (complex, complex);	/* from complex */

extern "C"
  { proc set_new_handler(proc);
    int strcmp(const char*, const char*), strncmp(const char*, const char*, int);
    int close(int);
    double fabs(double), sin(double), cos(double), hypot(double, double), atan2(double, double);
  };

inline bool operator == (complex z1, complex z2)
  { return z1.re == z2.re && z1.im == z2.im;
  }

inline bool seq(char *s1, char *s2)	    { return strcmp(s1, s2) == 0;				}
inline bool starts(char *s1, char *s2)	    { return strncmp(s1, s2, strlen(s2)) == 0;			}
inline bool isalpha(char c)		    { return (c >= 'a' && c <= 'z') || (c >=  'A' && c <= 'Z'); }
inline int ifix(double x)		    { return (x >= 0.0) ? (int) (x+0.5) : (int) (x-0.5);	}

