#define global

#define unless(x)   if(!(x))
#define until(x)    while(!(x))

#define MAXSTR	    256
#define VOWELS	    128		/* offset of vowels block in font */

union word
  { word(int nx)   { n = nx; }
    word(char *sx) { s = sx; }
    int n; char *s;
  };

struct point
  { point() { }
    point(int ix, int iy) { x = ix; y = iy; }
    int x, y;
  };

struct rect
  { rect() { }
    rect(int ix, int iy, int iw, int ih) { x = ix; y = iy; w = iw; h = ih; }
    int x, y, w, h;
  };

struct Context
  { Context() { }
    Context(int b, int c, int v, int w) { bk = b; ch = c; vn = v; wn = w; }
    int bk, ch, vn, wn;
  };

template<class T> struct vector
  { /* growable vector template */
    vector()  { max = 10; num = 0; vec = new T[10]; }
    ~vector() { delete vec;			    }
    void add(T);
    int max, num;
    T *vec;
  };

typedef vector<Context> ContextVec;
typedef vector<int> IntVec;

typedef unsigned int uint;
typedef unsigned char uchar;

typedef void (*proc)();

extern bool hadtitle;			/* from main	 */

extern int fileptr;			/* from common	 */

extern ContextVec *performsearch();	/* from search	 */
extern void writesearch();		/* from search	 */

extern char *latin_to_greek(char*);	/* from translit */
extern char *greek_to_latin(char*);	/* from translit */

extern void writemorph(uint);		/* from morph	 */
extern void parsemorph(uint&, uint&);	/* from morph	 */

extern void openbinfile(), closebinfile();				/* from common */
extern char *verseimage(Context);					/* from common */
extern void locateword(point, Context&);				/* from common */
extern char *grksimage(char*), *wordimage(int), *charimage(int);	/* from common */
extern char *bookname(int);						/* from common */
extern int textstart(int), findvso(Context), findcho(Context);		/* from common */
extern char *getword(int);						/* from common */
extern int get4(int), next4(), get3(int), next3(), get1(int), next1();	/* from common */
extern void seekto(int);						/* from common */

extern char *copystring(char*);						/* from util */
extern void hfatal(char*, word = 0, word = 0, word = 0, word = 0);	/* from util */
extern void prtitle(char*);						/* from util */
extern void prheading(char*, int);					/* from util */

extern "C"
  { int atoi(char*), system(char*), unlink(char*);
    int utime(char*, int*), time(int*);
    void atexit(proc);
    void *realloc(void*, int);
  };

inline bool seq(char *s1, char *s2)    { return strcmp(s1, s2) == 0;				  }
inline bool starts(char *s1, char *s2) { return strncmp(s1, s2, strlen(s2)) == 0;		  }
inline bool charin(char c, char *s)    { return strchr(s, c) != NULL;				  }
inline bool isalpha(char c)	       { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'); }
inline bool isdigit(char c)	       { return (c >= '0' && c <= '9');				  }
inline bool isalphanum(char c)	       { return isalpha(c) || isdigit(c);			  }
inline bool isprintable(char c)	       { return (c >= 041 && c <= 0176);			  }
inline char cvtlc(char c)	       { return (c >= 'A' && c <= 'Z') ? c + ('a'-'A') : c;	  }

