#define global
#define bool	    int
#define false	    0
#define true	    1
#define unless(x)   if(!(x))
#define until(x)    while(!(x))
#define seq(s1,s2)  (strcmp(s1,s2) == 0)

struct pco    { double lon, lat; };	 /* polar coords (latitude, longitude)	      */
struct cco    { double x, y; };		 /* Cartesian coords (easting, northing)      */

extern void polar_to_os(pco*, cco*);	/* from convert */

extern "C"
  { void getpco(char*[], pco*);		/* from io	*/
    void printpco(pco*, char*);		/* from io	*/
    void printcco(cco*);		/* from io	*/
  };

