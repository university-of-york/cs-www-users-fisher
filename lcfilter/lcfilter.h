#define global

#define unless(x)   if(!(x))
#define until(x)    while(!(x))

#define MAXSTR	    256
#define MAXORDER    10

extern float *coeffs(char*);

inline bool seq(char *s1, char *s2)    { return strcmp(s1,s2) == 0;		  }
inline bool starts(char *s1, char *s2) { return strncmp(s1, s2, strlen(s2)) == 0; }

#define tabindex(n) ((n)*((n)+1)/2)	/* index triangular array */

