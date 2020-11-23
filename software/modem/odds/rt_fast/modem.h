#define global
#define unless(x)   if(!(x))
#define until(x)    while(!(x))

#define BAUDRATE    2400		    /* symbols per sec	       */
#define SYMBLEN	    (SAMPLERATE / BAUDRATE) /* num. samples per symbol */
#define TRDELAY	    (SAMPLERATE/5)	    /* 0.2 secs Tx-to-Rx delay */

#define NULL	    0

/* bit rates - must agree with values in ../slow/modem.h */
#define bps_4800    0x04
#define bps_7200    0x08
#define bps_9600    0x10
#define bps_12000   0x20
#define bps_14400   0x40

/* bits in rate word */
#define RWORD	    0x0991		/* V.32 bis */
#define rb_4800	    0x0400
#define rb_9600	    0x0200
#define rb_7200	    0x0040
#define rb_12000    0x0020
#define rb_14400    0x0008

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

struct complex;		/* def'd in complex.h	*/
struct canceller;	/* def'd in cancel.h	*/
struct scrambler;	/* def'd in scramble.h	*/
struct coroutine;	/* def'd om coro.h	*/

extern int mstate;		/* from modem */
extern ushort rateword;		/* from modem */
extern canceller *can;		/* from modem */

extern "C"
  { int atoi(char*);
    proc set_new_handler(proc);
    void atexit(proc), exit(int);
  };

inline bool seq(char *s1, char *s2)
  { return strcmp(s1,s2) == 0;
  }

extern int roundtrip();						/* from roundtrip */

extern void inittx(), putasync(int);				/* from v32tx	  */

extern void initrx();						/* from v32rx	  */
extern int getasync();						/* from v32rx	  */

extern void giveup(char*, word = 0, word = 0, word = 0);	/* from common	  */
extern void infomsg(char*, word = 0, word = 0, word = 0);	/* from common	  */

