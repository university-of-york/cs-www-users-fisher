#define global
#define unless(x)  if(!(x))
#define until(x)   while(!(x))

#define SAMPLERATE 8000
#define TONELEN	   (SAMPLERATE/5)  /* 0.5 secs	   */
#define PI	   3.14159265358979323846
#define TWOPI	   (2.0 * PI)
#define MAXSTR	   256

union word	/* for error msgs */
  { word(int ix)   { i = ix; }
    word(char *sx) { s = sx; }
    int i; char *s;
  };

typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned int uint;
typedef signed char schar;

typedef void (*proc)();
typedef void (*eproc)(char*, word = 0, word = 0);   /* error handler proc (scripterror, giveup) */

enum symbol
  { s_number, s_string, s_colon, s_eof, s_redirect,
    s_chdir, s_error, s_print, s_speak, s_record, s_seize, s_release, s_wtring, s_sleep,
    s_loop, s_accept, s_fax, s_run, s_connect, s_end,
    s_seq, s_or, s_branch,
  };

struct pnode
  { pnode(symbol, pnode* = NULL, pnode* = NULL);    /* def'd in common */
    pnode(symbol, char*, char* = NULL);		    /* def'd in common */
    pnode(symbol, int);				    /* def'd in common */
    symbol h1;
    pnode *h2, *h3;
    char *str1, *str2;
    int num;
  };

struct tone
  { tone(float f);	/* def'd in common */
    ~tone();		/* def'd in common */
    uchar *vec;
  };

extern "C"
  { void close(int), dup2(int, int), sleep(int), nice(int), setsid();
    void execvp(char*, char**);
    int read(int, void*, int);
    int time(int*), fork(), wait(int*), chdir(char*), access(char*, int), atoi(char*);
    proc set_new_handler(proc);
    void atexit(proc);
    char *_getpty(int*, int, int, int), *getenv(char*), *ctime(int*);
    double sin(double), hypot(double, double);
  };

struct audio;							/* def'd in fishaudio.h */

extern audio *Audio;						/* from main	*/

extern pnode *parsescript(char*);				/* from parse	*/
extern void obeyscript(pnode*);					/* from obey	*/
extern bool runprog(char*, char*, bool);			/* from runprog */
extern void initdtmf(), tidydtmf();				/* from dtmf	*/
extern int getdtmf(bool);					/* from dtmf	*/
extern char *substparams(char*, eproc, char*, char*, char*);	/* from common	*/
extern char *copystr(char*);					/* from common	*/
extern void giveup(char*, word = 0, word = 0);			/* from common	*/

inline bool seq(char *s1, char *s2) { return strcmp(s1,s2) == 0; }	/* equal strings?   */
inline bool exists(char *fn)	    { return access(fn,4) == 0;	 }	/* does file exist? */
inline float sgn(float x)	    { return (x > 0.0f) ? +1.0f : (x < 0.0f) ? -1.0f : 0.0f; }

