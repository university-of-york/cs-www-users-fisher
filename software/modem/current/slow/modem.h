#define global
#define unless(x)   if(!(x))
#define until(x)    while(!(x))

#define BAUDRATE    2400		    /* symbols per sec (V.29)	      */
#define SYMBLEN	    (SAMPLERATE / BAUDRATE) /* num. samples per symbol (V.29) */

#define HDLC_FLAG   (-1)
#define HDLC_ABORT  (-2)

#define DIAL_TONE   1
#define CONN_TONE   2

#define NULL	    0

/* cmd line and other options */
#define opt_fax	    0x001   /* -fax : fax call		  */
#define opt_mod	    0x002   /* -V.. : modem call	  */
#define opt_org	    0x004   /* originate (telno given)	  */
#define opt_ans	    0x008   /* -ans : answer		  */
#define opt_bps	    0x010   /* -bps : bits per sec	  */
#define opt_v	    0x020   /* -v : verbose		  */
#define opt_v8	    0x100   /* -V8 : session control	  */
#define opt_H	    0x200   /* high resolution		  */

union word	/* for message routines */
  { word() { }
    word(char *sx) { s = sx; }
    word(int ix)   { i = ix; }
    char *s; int i;
  };

enum vmode	/* protocols */
  { V21o, V21a, V23o, V23a,	/* handled here */
    V32o,			/* handled by fmodem */
    V34o,			/* handled by zmodem */
    V29,			/* fax mode */
  };

typedef unsigned char uchar;
typedef unsigned int uint;
typedef unsigned short ushort;

struct complex;		/* def'd in complex.h */

typedef void (*proc)();

extern int errno;			    /* from sys lib */

extern int numpages;			    /* from main */
extern uint options, bitrates;		    /* from main */
extern vmode veemode;			    /* from main */

extern "C"
  { int time(int*), atoi(char*);
    int open(char*, uint);
    void close(int), atexit(proc), sleep(int);
    proc set_new_handler(proc);
  };

inline bool seq(char *s1, char *s2) { return strcmp(s1,s2) == 0;			     }
inline float sqr(float x)	    { return x*x;					     }
inline float fsgn(float x)	    { return (x > 0.0f) ? +1.0f : (x < 0.0f) ? -1.0f : 0.0f; }

extern void dialnumber(char*);						/* from dial	 */
extern void waitfortone(int);						/* from progress */
extern void startsession();						/* from session	 */
extern void becomemodem();						/* from modem	 */
extern void becomefax();						/* from fax	 */
extern void initdoc(), readdoc(), sendpage(int, int);			/* from doc	 */
extern void receivepage(int), writedoc();				/* from doc	 */

extern void inittx_fsk(vmode);						/* from fsktx	 */
extern void putasync(int), putsync(int);				/* from fsktx	 */

extern void initrx_fsk(vmode);						/* from fskrx	 */
extern int getasync(), getsync();					/* from fskrx	 */

extern void inittx_v29();						/* from v29tx	 */
extern void putbit(int);						/* from v29tx	 */

extern void initrx_v29();						/* from v29rx	 */
extern int getbit();							/* from v29rx	 */

extern void sendfreq(float, float), sendfreqs(float, float, float);	/* from common	 */
extern void sendpause(float);						/* from common	 */
extern void giveup(char*, word = 0, word = 0, word = 0);		/* from common	 */
extern void infomsg(char*, word = 0, word = 0, word = 0);		/* from common	 */

