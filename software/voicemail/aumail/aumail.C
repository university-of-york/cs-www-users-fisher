/* aumail -- audio mail "reader"   A.J. Fisher   February 1996 */

#include <fishaudio.h>
#include <stdio.h>
#include <signal.h>
#include <errno.h>

#define global
#define unless(x)  if(!(x))
#define until(x)   while(!(x))

#define MAILFILE   "/usr/fisher/mbox/ansphone.au"
#define SAMPLERATE 8000
#define BEEPLEN	   (SAMPLERATE/5)	/* 200 ms */
#define PI	   3.14159265358979323846
#define TWOPI	   (2.0 * PI)
#define MAXMSGS	   100
#define MAXSTR	   256

typedef void (*proc)();

extern "C"
  { proc set_new_handler(proc);
    void atexit(proc), unlink(char*);
  };

inline bool seq(char *s1, char *s2) { return strcmp(s1, s2) == 0; }

struct msgptr
  { int beg, end;
  };

union word
  { word(char *sx) { s = sx; }
    word(int ix)   { i = ix; }
    char *s; int i;
  };

static char *mailfile;
static audio *Audio;
static short **statetab;
static msgptr msgptrs[MAXMSGS+1];
static int nummsgs;
static bool anychanges;
static char tempfn[32];

static void newhandler(), sighandler(int), readcmdline(char**), usage(), maketempfn(), tidytemp();
static short **makestatetab();
static void readmessages(), editfile();
static void playmessage(int), savemessage(int, char*);
static void updatefile(), appendmessage(FILE*, FILE*, int, int[]), ensurefile(char*);
static void readheader(FILE*, int[]), writeheader(FILE*, int[]);
static FILE *openfile(char*, char*);
static void closefile(FILE*);
static void seekfile(FILE*, int), readfile(FILE*, void*, int, int), writefile(FILE*, void*, int, int);
static void renamefile(char*, char*);
static void giveup(char*, word = 0, word = 0);


global void main(int argc, char *argv[])
  { set_new_handler(newhandler);
    signal(SIGINT, (SIG_PF) sighandler); signal(SIGTERM, (SIG_PF) sighandler);
    readcmdline(argv);
    maketempfn();
    statetab = makestatetab();
    Audio = new audio(AU_OUT | AU_LOCK);
    Audio -> control(AU_OUTPUT_RATE, AU_RATE_8000);
    readmessages();
    editfile();
    if (anychanges) updatefile();
    printf("%d msgs retained.\n", nummsgs);
    delete Audio;
    exit(0);
  }

static void newhandler()
  { giveup("No room");
  }

static void sighandler(int sig)
  { giveup("Signal %d!", sig);
  }

static void readcmdline(char **argv)
  { int ap = 0;
    unless (argv[ap] == NULL) ap++;
    mailfile = MAILFILE;	/* default */
    until (argv[ap] == NULL)
      { if (seq(argv[ap],"-f") && argv[ap+1] != NULL)
	  { mailfile = argv[ap+1];
	    ap += 2;
	  }
	else usage();
      }
  }

static void usage()
  { fprintf(stderr, "Usage: aumail [-f fn]\n");
    exit(2);
  }

static void maketempfn()
  { strcpy(tempfn, "/tmp/aumail_XXXXXX"); mktemp(tempfn);
    atexit(tidytemp);
  }

static void tidytemp()
  { unlink(tempfn);
  }

static short **makestatetab()
  { static uchar beepvec[BEEPLEN];
    /* create mu-compressed beep vector, as written by phoneman */
    for (int i=0; i < BEEPLEN; i++)
      { double x = sin(440.0 * TWOPI * (double) i / (double) SAMPLERATE);
	beepvec[i] = mu_compress((int) ((1 << 14) * x));
      }
    /* make failure links */
    short flinks[BEEPLEN+1];
    flinks[0] = flinks[1] = 0;
    for (int j=2; j <= BEEPLEN; j++)
      { int f = j-1;
	flinks[j] = -1;
	do
	  { f = flinks[f];
	    if (beepvec[j-1] == beepvec[f]) flinks[j] = f+1;
	    else if (f == 0) flinks[j] = 0;
	  }
	while (flinks[j] < 0);
      }
    /* construct automaton (DFSM, state transition table) */
    short **stab = new short*[BEEPLEN+1];
    for (int j=0; j <= BEEPLEN; j++) stab[j] = new short[256];
    for (int k=0; k < 256; k++) stab[0][k] = 0;
    for (int j=1; j <= BEEPLEN; j++)
      { int ch = beepvec[j-1];
	stab[j-1][ch] = j;
	for (int k=0; k < 256; k++)
	  { int f = flinks[j];
	    stab[j][k] = stab[f][k];
	  }
      }
    return stab;
  }

static void readmessages()
  { FILE *fi = openfile(mailfile, "r");
    int hdr[8]; readheader(fi, hdr);
    int fp = hdr[1];
    seekfile(fi, fp);	/* position to start of data */
    nummsgs = 0;
    int val = getc(fi);
    until (val < 0)
      { if (nummsgs >= MAXMSGS) giveup("too many msgs!");
	msgptrs[nummsgs].beg = fp;
	short state = 0;
	until (val < 0 || state == BEEPLEN)
	  { state = statetab[state][val];
	    val = getc(fi); fp++;
	  }
	msgptrs[nummsgs].end = fp;
	nummsgs++;
      }
    msgptrs[nummsgs].beg = fp;	/* size of file */
    printf("[file %s: hdr[1]=%d, hdr[2]=%d, size=%d]\n", mailfile, hdr[1], hdr[2], fp);
    closefile(fi);
    printf("%d messages read.\n", nummsgs);
  }

static void editfile()
  { anychanges = false;
    int mnum = 0, snum = nummsgs;
    bool ex = false;
    while (mnum < nummsgs && !ex)
      { playmessage(mnum);
	bool cok = false;
	until (cok)
	  { printf("Msg %d of %d ? ", mnum+1, nummsgs); fflush(stdout);
	    int ch = getchar();
	    switch (ch)
	      { default:
		    printf("Cmds are:  <cr> <eof> d p x\n");
		    printf("           s <fn>\n");
		    break;

		case '\n':
		    mnum++;
		    cok = true;
		    break;

		case EOF:
		    putchar('\n');
		    ex = true;
		    cok = true;
		    break;

		case 'd':
		    for (int k = mnum; k < nummsgs; k++) msgptrs[k] = msgptrs[k+1];
		    nummsgs--;
		    anychanges = true;
		    cok = true;
		    break;

		case 'p':
		    playmessage(mnum);
		    break;

		case 's':
		  { char buf[MAXSTR+1]; int nc = 0;
		    ch = getchar();
		    while (ch == ' ' || ch == '\t') ch = getchar();
		    until (ch == ' ' || ch == '\t' || ch == '\n' || ch == EOF || nc >= MAXSTR)
		      { buf[nc++] = ch;
			ch = getchar();
		      }
		    buf[nc++] = '\0';
		    savemessage(mnum, buf);
		    for (int k = mnum; k < nummsgs; k++) msgptrs[k] = msgptrs[k+1];
		    nummsgs--;
		    anychanges = true;
		    printf("Msg moved from %s to %s\n", mailfile, buf);
		    cok = true;
		    break;
		  }

		case 'x':
		    nummsgs = snum; /* restore */
		    anychanges = false;
		    ex = true;
		    cok = true;
		    break;
	      }
	    until (ch == '\n' || ch == EOF) ch = getchar();
	  }
      }
  }

static void playmessage(int mn)
  { FILE *fi = openfile(mailfile, "r");
    seekfile(fi, msgptrs[mn].beg);
    int nb = msgptrs[mn].end - msgptrs[mn].beg;
    for (int k=0; k < nb; k++)
      { int val = getc(fi);
	Audio -> write(mu_expand(val) << 8);
      }
    closefile(fi);
  }

static void savemessage(int mn, char *ofn)
  { ensurefile(ofn);
    FILE *ifi = openfile(mailfile, "r");
    FILE *ofi = openfile(ofn, "r+");
    int hdr[8]; readheader(ofi, hdr);
    appendmessage(ifi, ofi, mn, hdr);
    seekfile(ofi, 0); writeheader(ofi, hdr);	/* rewrite hdr with correct body size */
    closefile(ifi); closefile(ofi);
  }

static void updatefile()
  { FILE *ifi = openfile(mailfile, "r");
    FILE *ofi = openfile(tempfn, "w");
    int hdr[8]; readheader(ifi, hdr); writeheader(ofi, hdr);
    hdr[2] = 0;
    for (int k=0; k < nummsgs; k++) appendmessage(ifi, ofi, k, hdr);
    seekfile(ofi, 0); writeheader(ofi, hdr);	/* rewrite hdr with correct body size */
    closefile(ifi); closefile(ofi);
    renamefile(tempfn, mailfile);
  }

static void appendmessage(FILE *ifi, FILE *ofi, int mn, int hdr[])
  { /* write message number "mn" at posn in output file identified by "hdr" */
    int nc = msgptrs[mn].end - msgptrs[mn].beg;
    uchar *buf = new uchar[nc];
    seekfile(ifi, msgptrs[mn].beg);
    readfile(ifi, buf, sizeof(uchar), nc);
    seekfile(ofi, hdr[1] + hdr[2]);
    writefile(ofi, buf, sizeof(uchar), nc);
    delete buf;
    hdr[2] += nc;   /* update body size */
  }

static int dummy_hdr[8] =
  { 0x2e736e64,			/* ".snd" magic number */
    32,				/* length of hdr */
    0,				/* length of body */
    1, 8000, 1, 0, 0,		/* don't ask me what this means! */
  };

static void ensurefile(char *fn)
  { FILE *fi = fopen(fn, "r+");
    if (fi == NULL)
      { unless (errno == ENOENT) giveup("Can't open %s", fn);
	printf("Creating new %s\n", fn);
	fi = openfile(fn, "w");
	writeheader(fi, dummy_hdr);
	closefile(fi);
      }
    else closefile(fi);
  }

static void readheader(FILE *fi, int hdr[])
  { readfile(fi, hdr, sizeof(int), 8);
    unless (hdr[0] == 0x2e736e64 && hdr[1] >= 32) giveup("bad header");
  }

static void writeheader(FILE *fi, int hdr[])
  { writefile(fi, hdr, sizeof(int), 8);
  }

static FILE *openfile(char *fn, char *rw)
  { FILE *fi = fopen(fn, rw);
    if (fi == NULL) giveup("can't open %s for %s", fn, (rw[0] == 'r') ? "reading" : "writing");
    return fi;
  }

static void closefile(FILE *fi)
  { int code = fclose(fi);
    if (code != 0) giveup("fclose failed");
  }

static void seekfile(FILE* fi, int p)
  { int code = fseek(fi, p, 0);
    if (code < 0) giveup("fseek failed");
  }

static void readfile(FILE *fi, void *buf, int si, int ni)
  { int n = fread(buf, si, ni, fi); /* read header */
    unless (n == ni) giveup("fread failed");
  }

static void writefile(FILE *fi, void *buf, int si, int ni)
  { int n = fwrite(buf, si, ni, fi); /* read header */
    unless (n == ni) giveup("fwrite failed");
  }

static void renamefile(char *fn1, char *fn2)
  { char cmd[MAXSTR+1]; sprintf(cmd, "/bin/mv %s %s", fn1, fn2);
    int code = system(cmd);
    if (code != 0) giveup("rename failed");
  }

static void giveup(char *msg, word p1, word p2)
  { fprintf(stderr, "aumail: "); fprintf(stderr, msg, p1, p2); putc('\n', stderr);
    exit(1);
  }

