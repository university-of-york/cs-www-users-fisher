/* mkfilter -- given n, compute recurrence relation
   to implement Butterworth or Bessel filter of order n
   A.J. Fisher, University of York   <fisher@minster.york.ac.uk>
   September 1992 */

/* Header file */

#define global
#define forward

#define uint	    unsigned int
#define bool	    int
#define true	    1
#define false	    0
#define word	    int		    /* or char* or ... */
#define unless(x)   if(!(x))
#define until(x)    while(!(x))
#define ifix(x)	    (int) (((x) >= 0.0) ? (x) + 0.5 : (x) - 0.5)
#define seq(s1,s2)  (strcmp(s1,s2) == 0)

#define PI	    3.14159265358979323846
#define TWOPI	    (2.0 * PI)
#define EPS	    1e-10
#define MAXORDER    10
#define MAXPOLES    (2*MAXORDER)    /* to allow for doubling of poles in BP filter */

struct complex { double re, im };

