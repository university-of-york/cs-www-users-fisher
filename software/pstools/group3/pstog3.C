/* pstog3 -- convert PostScript to Group3 fax
   AJF	 December 1998 */

#include <stdio.h>
#include <string.h>
#include <setjmp.h>

#include "tables.h"

#define global
#define unless(x)  if(!(x))
#define until(x)   while(!(x))

#define VERSION	   1
#define MAXSTR	   256

#define A4WID	   8.2765	/* same as in psrender */
#define A4HGT	   11.705	/* same as in psrender */

#define XDOTS	   1728
#define YDOTS_L	   1170		/* low resolution */
#define YDOTS_H	   2340		/* high resolution */

#define PSRENDER   "/usr/fisher/mipslib/psrender"

typedef void (*proc)();
typedef unsigned char uchar;

union word
  { word(int nx)   { n = nx; }
    word(char *sx) { s = sx; }
    int n; char *s;
  };

extern "C"
  { proc set_new_handler(proc);
  };

struct node
  { node()
      { lft = rgt = NULL;
	tent = NULL;
      }
    node *lft, *rgt;
    tableentry *tent;
  };

struct IBitpipe
  { IBitpipe(FILE *f) { fi = f; len = 0; }
    int get();
private:
    FILE *fi; int len; uchar w;
  };

struct OBitpipe
  { OBitpipe(FILE *f) { fi = f; len = 0; }
    ~OBitpipe() { flush(); }
    void put(int), flush();
private:
    FILE *fi; int len; uchar w;
  };

static bool hires;
static node *whitetree, *blacktree;
static jmp_buf eof;

static void newhandler(), readcmdline(char**), usage();
static void maketrees(), maketree(node**, tableentry*, tableentry*), mktree(node**, tableentry*);
static void addnode(node**, tableentry*);
static void processfile(), processpage(IBitpipe*, OBitpipe*), processline(IBitpipe*, OBitpipe*);
static void readeol(IBitpipe*), writeeol(OBitpipe*);
static void giveup(char*, word = NULL);

inline bool seq(char *s1, char *s2) { return strcmp(s1,s2) == 0; }


global void main(int argc, char **argv)
  { set_new_handler(newhandler);
    readcmdline(argv);
    maketrees();
    processfile();
    exit(0);
  }

static void newhandler()
  { giveup("no room!");
  }

static void readcmdline(char **argv)
  { hires = true;
    int ap = 0;
    unless (argv[ap] == NULL) ap++;
    until (argv[ap] == NULL)
      { if (seq(argv[ap], "-l")) { hires = false; ap++; }
	else usage();
      }
  }

static void usage()
  { fprintf(stderr, "Usage: pstog3 [-l]\n");
    exit(1);
  }

static void maketrees()
  { maketree(&whitetree, twtable, mwtable);
    maketree(&blacktree, tbtable, mbtable);
  }

static void maketree(node **rootptr, tableentry *ttab, tableentry *mtab)
  { *rootptr = NULL;
    mktree(rootptr, ttab); mktree(rootptr, mtab);
    mktree(rootptr, extable);
  }

static void mktree(node **rootptr, tableentry *tab)
  { for (int i = 0; tab[i].count >= 0; i++) addnode(rootptr, &tab[i]);
  }

static void addnode(node **rootptr, tableentry *tent)
  { int len = tent -> len;
    if (len > 16) giveup("table bug!");
    ushort w = (tent -> code) << (16 - len);	/* posn at top of word */
    node **x = rootptr;
    while (len > 0)
      { if (*x == NULL) *x = new node;
	x = (w & 0x8000) ? &(*x) -> rgt : &(*x) -> lft;
	w <<= 1; len--;
      }
    if (*x != NULL) giveup("Duplicate! code=0x%x", tent -> code);
    *x = new node;
    (*x) -> tent = tent;
  }

static void processfile()
  { printf("!<Group3> %d %d %d\n", VERSION, 200, (hires ? 200 : 100));
    char cmd[MAXSTR+1];
    sprintf(cmd, "%s -g3 -dpi %.3f %.3f - -", PSRENDER, XDOTS / A4WID, (hires ? YDOTS_H : YDOTS_L) / A4HGT);
    FILE *pipe = popen(cmd, "r");
    if (pipe == NULL) giveup("pipe failed! %s", cmd);
    IBitpipe *ibp = new IBitpipe(pipe);
    OBitpipe *obp = new OBitpipe(stdout);
    int pn = 0;
    unless (_setjmp(eof))
      { for (;;)
	  { processpage(ibp, obp);
	    fprintf(stderr, "[%d] ", ++pn);
	  }
      }
    if (pn > 0) putc('\n', stderr);
    delete ibp; delete obp;
    if (pclose(pipe) != 0) giveup("command failed: %s", cmd);
  }

static void processpage(IBitpipe *ibp, OBitpipe *obp)
  { int nlines = (hires ? YDOTS_H : YDOTS_L);
    for (int i = 0; i < nlines; i++) processline(ibp, obp);
    for (int i = 0; i < 6; i++) writeeol(obp);	/* RTC */
    obp -> flush();
  }

static void processline(IBitpipe *ibp, OBitpipe *obp)
  { readeol(ibp); writeeol(obp);
    int bw = 0, dots = 0;
    for (;;)
      { node *x = bw ? blacktree : whitetree;
	until (x == NULL || x -> tent != NULL)
	  { int b = ibp -> get();
	    obp -> put(b);
	    x = b ? x -> rgt : x -> lft;
	  }
	if (x == NULL) giveup("Invalid code");
	int count = x -> tent -> count;
	dots += count;
	if (count < 64)
	  { if (dots >= XDOTS) break;
	    bw ^= 1;	/* swap between white and black */
	  }
      }
    if (dots > XDOTS) giveup("bad line length: %d", dots);
  }

static void readeol(IBitpipe *ibp)
  { int nz = 0;
    until (ibp -> get()) nz++;	    /* deal with possible fill bits */
    if (nz < 11) giveup("expected EOL at start of line");
  }

static void writeeol(OBitpipe *obp)
  { for (int j = 0; j < 11; j++) obp -> put(0);
    obp -> put(1);
  }

int IBitpipe::get()
  { if (len == 0)
      { int ch = getc(fi);
	if (ch < 0) _longjmp(eof, true);
	w = ch; len = 8;
      }
    int b = w >> 7;
    w <<= 1; len--;
    return b;
  }

void OBitpipe::put(int b)
  { if (len == 8) flush();
    w = (w << 1) | b;
    len++;
  }

void OBitpipe::flush()
  { if (len > 0)
      { while (len < 8) put(0);
	putc(w, fi);
	len = 0;
      }
  }

static void giveup(char *msg, word p1)
  { fprintf(stderr, "pstog3: "); fprintf(stderr, msg, p1); putc('\n', stderr);
    exit(1);
  }
