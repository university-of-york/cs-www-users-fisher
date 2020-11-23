#define AIRY_A	    6377563.396			/* radius of earth (semi-major axis)	     */
#define AIRY_B	    6356256.910			/* radius of earth (semi-minor axis)	     */
#define SCALE	    0.9996012717		/* scale factor on central meridian	     */
#define ESQUARED    6.670540000123428e-3	/* eccentricity squared, = (a^2-b^2)/a^2     */
#define X0	    400000.0			/* X origin				     */
#define Y0	    (-100000.0)			/* Y origin				     */
#define M0	    5427063.815			/* meridional distance to PHI0, adj by SCALE */
#define THETA0	    (-2.0)			/* central meridian (degrees)		     */
#define PHI0	    49.0			/* "true origin" latitude                    */
#define PHI1	    55.0			/* for LCC, = phi2			     */

#define ARAD	    (AIRY_A*SCALE)

