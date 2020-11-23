#define forward
#define global
#define bool	    int
#define false	    0
#define true	    1
#define unless(x)   if(!(x))
#define until(x)    while(!(x))
#define seq(s1,s2)  (strcmp(s1,s2) == 0)

struct pco    { double lon, lat };	/* polar coords (latitude, longitude)	     */
struct triple { double lon, lat, hgt }; /* polar coords plus height, for datum shift */
struct cco    { double x, y };		/* Cartesian coords (easting, northing)	     */

