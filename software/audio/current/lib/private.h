#define global

#define unless(x)   if(!(x))
#define until(x)    while(!(x))

typedef unsigned char uchar;
typedef unsigned int uint;
typedef void (*proc)();

#define MASTERFN   "/dev/hdsp/hdsp0master"

#define RBLENGTH   (2 * (MAXSAMPLES+1)) /* num. ints in ring buffer (power of 2) */

struct ringbuf
  { int head, tail, intreq, fillpt;
    int buf[RBLENGTH];
  };

union word
  { word(int nx)   { n = nx; }
    word(char *sx) { s = sx; }
    int n; char *s;
  };

extern "C"
  { int open(char*, uint, uint = 0);
    void close(int);
    int select(int, uint*, uint*, uint*, int*);
    int ioctl(int, int, void*);
    char *getenv(char*);
    int putenv(char*);
    double pow(double, double);
  };

#define doioctl au_doioctl	/* avoid link name conflicts */
#define giveup au_giveup	/* avoid link name conflicts */

extern int au_getset(int, int, int);
extern void doioctl(int, int, int, void*);
extern void giveup(char*, word = 0, word = 0);

