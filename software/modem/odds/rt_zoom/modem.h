#define global
#define unless(x)   if(!(x))
#define until(x)    while(!(x))

#define NULL	    0

/* bit rates - must agree with values in ../slow/modem.h */
#define bps_4800    0x04
#define bps_7200    0x08
#define bps_9600    0x10
#define bps_12000   0x20
#define bps_14400   0x40

union word	/* for message routines */
  { word() { }
    word(char *sx) { s = sx; }
    word(int ix)   { i = ix; }
    char *s; int i;
  };

struct complex;

typedef unsigned char uchar;
typedef unsigned int uint;
typedef unsigned short ushort;
typedef signed char schar;
typedef void (*proc)();

extern "C"
  { int atoi(char*);
    proc set_new_handler(proc);
    void atexit(proc), exit(int);
  };

inline bool seq(char *s1, char *s2)
  { return strcmp(s1,s2) == 0;
  }

extern void infoseqs();						/* from infoseqs */

extern void giveup(char*, word = 0, word = 0, word = 0);	/* from common	 */
extern void infomsg(char*, word = 0, word = 0, word = 0);	/* from common	 */

