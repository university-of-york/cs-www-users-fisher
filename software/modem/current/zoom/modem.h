#define global
#define unless(x)   if(!(x))
#define until(x)    while(!(x))

#define NULL	0

#define SECS(f) ((int) (f * SAMPLERATE + 0.5))

#define DELAY	SECS(0.04)	    /* 40 ms  turnround delay */

#define SYMBLEN (SAMPLERATE/600)    /* 600 bps */

union word	/* for message routines */
  { word() { }
    word(char *sx) { s = sx; }
    word(int ix)   { i = ix; }
    char *s; int i;
  };

typedef unsigned char uchar;
typedef unsigned int uint;
typedef unsigned short ushort;
typedef signed char schar;
typedef void (*proc)();

struct complex;

extern int mstate;	/* from modem */

extern "C"
  { int atoi(char*);
    proc set_new_handler(proc);
    void atexit(proc), exit(int);
  };

inline bool seq(char *s1, char *s2)
  { return strcmp(s1,s2) == 0;
  }

extern void txside();						/* from txside	 */
extern void rxside();						/* from rxside	 */

extern void giveup(char*, word = 0, word = 0, word = 0);	/* from common	 */
extern void infomsg(char*, word = 0, word = 0, word = 0);	/* from common	 */

