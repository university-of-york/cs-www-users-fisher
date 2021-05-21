#define global
#define unless(x)    if(!(x))
#define until(x)     while(!(x))

#define MAXSTR	     256

#define TEMP_DIR     "/www/usr/fisher/tmpdir/misc"
#define TEMP_URL     "http://www-users.cs.york.ac.uk/~fisher/tmpdir/misc"

typedef unsigned char uchar;

union word
  { word(int ix)   { i = ix; }
    word(char *sx) { s = sx; }
    int i; char *s;
  };

struct component
  { component(uchar xt, uchar xo, int xl, double xv, int xn1, int xn2, int xn3, int xx, int xy)
      { type = xt; orient = xo; length = xl; val = xv;
	n1 = xn1; n2 = xn2; n3 = xn3; px = xx; py = xy;
	name = NULL;
      }
    uchar type, orient;
    int length;
    double val;
    int n1, n2, n3;
    int px, py;
    char *name;
  };

struct complex;	    /* def'd fully in complex.h */

extern "C"
  { int ftruncate(int, int);
    double atof(char*);
  };

extern bool solve_eqns(complex**, complex*, complex*, int);		/* from matrix	*/
extern void drawcircuit(component**, int, char*);			/* from drawcct */
extern void drawgraph(complex*, int, double, double, double, char*);	/* from graphs	*/

extern char *copystring(char*);						/* from common	*/
extern void giveup(char*, word = 0, word = 0);				/* from common	*/

inline bool seq(char *s1, char *s2)    { return strcmp(s1,s2) == 0;		  }
inline bool starts(char *s1, char *s2) { return strncmp(s1, s2, strlen(s2)) == 0; }

