#define global
#define forward
#define unless(x)   if(!(x))
#define until(x)    while(!(x))
#define seq(s1,s2)  (strcmp(s1,s2) == 0)
#define heap(n,ty)  (ty*) au_cheap((n) * sizeof(ty))

forward char *au_cheap();

typedef int word;	/* or char* or ... */
typedef unsigned int uint;
typedef unsigned char uchar;
typedef unsigned short ushort;


