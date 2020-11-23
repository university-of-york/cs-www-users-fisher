#include <stdio.h>
#include <signal.h>
#include <gd.h>

#define global
#define unless(x)    if(!(x))
#define until(x)     while(!(x))

#define MAXDATA	     25000
#define MAXSTRING    256
#define DOTSPERFRAME 400
#define FRAMESIZE    512	/* X and Y */

typedef void (*proc)();

union word
  { word(int ix)   { i = ix; }
    word(char *sx) { s = sx; }
    int i; char *s;
  };

struct complex
  { float re, im;
  };

extern "C"
  { proc set_new_handler(proc);
    void *realloc(void*, int);
    void atexit(proc);
    int mkdir(char*, int);
    double atof(char*);
  };

static char *infn, *outfn, *compression;
static complex *data;
static int numdata;
static char tempdfn[12];
static float maxxy;

static void newhandler(), sighandler(int), decodecmdline(char**), usage();
static void readdata(), makegifs(), unlinkdir(), makegif(complex*, char*);
static void drawdot(gdImagePtr, int, int, int);
static void makemovie();
static void appends(char*, int&, char*, word = 0, word = 0);
static bool ends(char *s1, char *s2);
static void giveup(char*, word = 0);

inline bool isdigit(char c)	    { return (c >= '0' && c <= '9'); }
inline bool seq(char *s1, char *s2) { return strcmp(s1, s2) == 0;    }


global main(int argc, char **argv)
  { set_new_handler(newhandler);
    signal(SIGINT, (SIG_PF) sighandler); signal(SIGTERM, (SIG_PF) sighandler);
    decodecmdline(argv);
    readdata();
    makegifs();
    delete data;
    makemovie();
    exit(0);
  }

static void newhandler()
  { giveup("No room!");
  }

static void sighandler(int sig)
  { giveup("Signal %d", sig);
  }

static void decodecmdline(char **argv)
  { int ap = 0;
    unless (argv[ap] == NULL) ap++;	/* skip program name */
    compression = NULL;
    while (argv[ap] != NULL && argv[ap][0] == '-')
      { char *s = argv[ap++];
	if (seq(s,"-c"))
	  { compression = argv[ap++];
	    if (compression == NULL) usage();
	  }
	else usage();
      }
    if (argv[ap] == NULL || argv[ap+1] == NULL || argv[ap+2] == NULL) usage();
    infn = argv[ap++]; outfn = argv[ap++]; maxxy = atof(argv[ap++]);
    if (maxxy <= 0.0) usage();
    unless (argv[ap] == NULL) usage();
  }

static void usage()
  { fprintf(stderr, "Usage: mkmovie [-c comp] co.grap out.qt maxxy\n");
    exit(1);
  }

static void readdata()
  { data = new complex[MAXDATA];
    numdata = 0;
    FILE *fi = fopen(infn, "r");
    if (fi == NULL) giveup("can't open %s", infn);
    for (;;)
      { char line[MAXSTRING+1];
	if (fgets(line, MAXSTRING, fi) == NULL) break;
	unless (line[0] == '.' || line[0] == '\0')
	  { if (numdata >= MAXDATA) giveup("too many data!");
	    int ni = sscanf(line, "%g %g\n", &data[numdata].re, &data[numdata].im);
	    if (ni == 2) numdata++;
	  }
      }
    fclose(fi);
    data = (complex*) realloc(data, numdata * sizeof(complex));
    if (data == NULL) giveup("No room! (realloc)");
  }

static void makegifs()
  { strcpy(tempdfn, "/tmp/XXXXXX"); mktemp(tempdfn);
    atexit(unlinkdir);
    int code = mkdir(tempdfn, 0777);
    unless (code == 0) giveup("can't create temp direcory");
    int frnum = 0;
    for (int i = 0; i <= numdata - DOTSPERFRAME; i += DOTSPERFRAME/8)
      { fprintf(stderr, "%d ", frnum);
	char gfn[MAXSTRING+1];
	sprintf(gfn, "%s/f%04d.gif", tempdfn, frnum++);
	makegif(&data[i], gfn);
      }
    putc('\n', stderr);
  }

static void unlinkdir()
  { char cmd[MAXSTRING+1];
    sprintf(cmd, "/bin/rm -r %s", tempdfn);
    system(cmd);
  }

inline int scale(float f) { return (int) ((f + maxxy) / (2.0f * maxxy) * FRAMESIZE); }

static void makegif(complex *vec, char *gfn)
  { gdImagePtr im = gdImageCreate(FRAMESIZE, FRAMESIZE);
    if (im == NULL) giveup("No room! (gdImageCreate)");
    gdImageColorAllocate(im, 64, 64, 64); /* dk grey background */
    /* alloc colours so that most red = 255 = newest; most blue = 1 = oldest */
    for (int i = 1; i < 128; i++)
      { int k = 2*i;
	gdImageColorAllocate(im, 0, k, 255-k);
      }
    for (int i = 128; i < 256; i++)
      { int k = 2*(i-128);
	gdImageColorAllocate(im, k, 255-k, 0);
      }
    for (int i = 0; i < DOTSPERFRAME; i++)
      { complex z = vec[i];
	int ix = scale(z.re), iy = FRAMESIZE - scale(z.im);
	int col = ((i*255) / DOTSPERFRAME) + 1;	 /* range 1 .. 255; "dimmest" to "brightest" */
	unless (col >= 1 && col <= 255) giveup("bug! col %d", col);
	if ((ix >= 0 && ix < FRAMESIZE) && (iy >= 0 && iy < FRAMESIZE)) drawdot(im, ix, iy, col);
      }
    FILE *fi = fopen(gfn, "w");
    if (fi == NULL) giveup("can't create %s", gfn);
    gdImageGif(im, fi);
    fclose(fi);
    gdImageDestroy(im);
  }

static void drawdot(gdImagePtr im, int ix, int iy, int col)
  { gdImageSetPixel(im, ix, iy-1, col);
    gdImageSetPixel(im, ix-1, iy, col);
    gdImageSetPixel(im, ix, iy, col);
    gdImageSetPixel(im, ix+1, iy, col);
    gdImageSetPixel(im, ix, iy+1, col);
  }

static void makemovie()
  { char cmd[MAXSTRING+1]; int p = 0;
    appends(cmd, p, "/usr/sbin/dmconvert -v -f qt -p video");
    if (compression != NULL) appends(cmd, p, ",comp=%s", compression);
    appends(cmd, p, " %s/f####.gif '%s'", tempdfn, outfn);
    int code = system(cmd);
    unless (code == 0) giveup("dmconvert failed");
  }

static void appends(char *vec, int &p, char *fmt, word p1, word p2)
  { sprintf(&vec[p], fmt, p1, p2);
    until (vec[p] == '\0') p++;
  }

static bool ends(char *s1, char *s2)
  { int n1 = strlen(s1), n2 = strlen(s2);
    return n1 >= n2 && seq(&s1[n1-n2], s2);
  }

static void giveup(char *msg, word p1)
  { fprintf(stderr, "mkmovie: "); fprintf(stderr, msg, p1); putc('\n', stderr);
    exit(1);
  }

