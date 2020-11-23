#define global
#define unless(x)   if(!(x))
#define until(x)    while(!(x))

#define SAMPLERATE  24000
#define SINELEN	    32768	/* power of 2 */
#define PI	    3.14159265358979323846
#define TWOPI	    (2.0 * PI)
#define MAXAMPL	    (float) 0x3fffff	/* max amplitude for ADC, DAC (h/w accepts 0x7fffff) */

#define HDLC_FLAG   (-1)
#define HDLC_ABORT  (-2)

#define DIAL_TONE   1
#define MERC_TONE   2
#define CONN_TONE   3

#define NULL	    0

/* cmd line and other options */
#define opt_fax	    0x001   /* -fax : fax call		  */
#define opt_mod	    0x002   /* -V.. : modem call	  */
#define opt_org	    0x004   /* originate (telno given)	  */
#define opt_ans	    0x008   /* -ans : answer		  */
#define opt_bps	    0x010   /* -bps : bits per sec	  */
#define opt_v	    0x020   /* -v : verbose		  */
#define opt_m	    0x040   /* -m : use Mercury 131	  */
#define opt_p	    0x080   /* -p : private call	  */
#define opt_7	    0x100   /* -7 : strip parity on input */
#define opt_H	    0x200   /* high resolution		  */

/* bit rates */
#define bps_1200    0x01
#define bps_2400    0x02
#define bps_4800    0x04
#define bps_7200    0x08
#define bps_9600    0x10
#define bps_12000   0x20
#define bps_14400   0x40

/* V. modes - must agree with table in main.C */
enum vmode
  { V21o, V21a, V23o, V23a,	/* handled here */
    V22o, V22a, V32o,		/* handled by fmodem */
    V29,			/* fax mode */
  };

typedef unsigned char uchar;
typedef unsigned int uint;
typedef unsigned short ushort;

union word	/* for message routines */
  { word() { }
    word(char *sx) { s = sx; }
    word(int ix)   { i = ix; }
    char *s; int i;
  };

typedef void (*proc)();
typedef void (*proci)(int);
typedef void (*procv)(vmode);
typedef int (*ifunc)();

struct rxhandler
  { procv init;		/* init & start reception    */
    ifunc gasync;	/* get async byte	     */
    ifunc gsync;	/* get sync byte	     */
    ifunc gbit;		/* get a bit		     */
  };

struct txhandler
  { procv init;		/* init & start transmission */
    proci pasync;	/* put async byte	     */
    proci psync;	/* put sync byte	     */
    proci pbit;		/* put a bit		     */
  };

extern int errno;			/* from sys lib */

extern struct audio *Audio;		/* from main */
extern float sinetab[];			/* from main */
extern int contime, pagessent;		/* from main */
extern ushort scanbits;			/* from main */		/* set by fax, used by senddoc */
extern bool morepages;			/* from main */		/* set by senddoc, used by fax */
extern uint options, bitrates;		/* from main */
extern char *telno, *mercurypin;	/* from main */

extern txhandler fsk_txhandler;		/* from fsktx */
extern txhandler v29_txhandler;		/* from v29tx */

extern rxhandler fsk_rxhandler;		/* from fskrx */

extern "C"
  { double sin(double);
    float fabsf(float);
    int time(int*), fork();
    void execl(char*, char*, ...);
    int open(char*, uint);
    void close(int), atexit(proc), sleep(int);
    int atoi(char*);
    int select(int, uint*, uint*, uint*, int*);
    void *realloc(void*, uint);
    proc set_new_handler(proc);
  };

inline bool seq(char *s1, char *s2)
  { return strcmp(s1,s2) == 0;
  }

inline float sqr(float x)
  { return x*x;
  }

extern char *getpassword(char*);					/* from getpass	   */
extern void dialnumber();						/* from dial	   */
extern void waitfortone(int);						/* from progress   */
extern void becomemodem(vmode);						/* from modem	   */
extern void init_ttymodes(), setraw(), setcooked();			/* from ttymodes   */
extern void becomefax();						/* from fax	   */
extern void initdoc(), readdocpage(), senddocpage();			/* from doc	   */
extern void inittx(vmode), putaval(int), putsval(int), putbit(int);	/* from txside	   */
extern void initrx(vmode);						/* from rxside	   */
extern int getaval(), getsval(), getbit();				/* from rxside	   */
extern void sendanswer();						/* from common	   */
extern void sendfreq(int, int);						/* from common	   */
extern void sendpause(int);						/* from common	   */

extern void giveup(char*, word = 0, word = 0, word = 0);		/* from common	   */
extern void infomsg(char*, word = 0, word = 0, word = 0);		/* from common	   */
extern void writelog(char*, char*, word = 0, word = 0, word = 0);	/* from common	   */

