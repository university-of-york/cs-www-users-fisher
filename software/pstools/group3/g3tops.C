/* g3tops -- convert Group3 fax to PostScript
   AJF	 December 1998 */

#include <stdio.h>
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

typedef void (*proc)();
typedef unsigned char uchar;

union word
  { word(int nx)   { n = nx; }
    word(char *sx) { s = sx; }
    int n; char *s;
  };

extern "C"
  { proc set_new_handler(proc);
    int ftruncate(int, int);
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
  { OBitpipe(FILE *f) { fi = f; len = bpl = 0; }
    ~OBitpipe() { flushbits(); flushbytes(); }
    void put(int), flushbits(), flushbytes();
private:
    FILE *fi; int len, bpl; uchar w;
  };

static node *whitetree, *blacktree;
static jmp_buf eop, eof;

static void newhandler(), usage();
static void maketrees(), maketree(node**, tableentry*, tableentry*), mktree(node**, tableentry*), addeol(node**);
static void addnode(node**, tableentry*);
static void convertdoc();
static void processline(IBitpipe*, OBitpipe*);
static void copyfile(FILE*, FILE*, int, int);
static void giveup(char*, word = 0);


global void main(int argc, char **argv)
  { set_new_handler(newhandler);
    unless (argc == 1) usage();
    maketrees();
    convertdoc();
    exit(0);
  }

static void newhandler()
  { giveup("no room!");
  }

static void usage()
  { fprintf(stderr, "Usage: g3tops\n");
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
    addeol(rootptr);
  }

static void mktree(node **rootptr, tableentry *tab)
  { for (int i = 0; tab[i].count >= 0; i++) addnode(rootptr, &tab[i]);
  }

static void addeol(node **rootptr)
  { static tableentry eol = { 1, 12, -1 };
    addnode(rootptr, &eol);
    /* add a loop to allow for fill bits (0s) before eol */
    node *x = *rootptr;
    for (int i = 0; i < 11; i++) x = x -> lft;
    x -> lft = x;
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

static void convertdoc()
  { int vsn, xdpi, ydpi;
    int ni = scanf("!<Group 3> %d %d %d\n", &vsn, &xdpi, &ydpi);
    unless (ni == 3 && vsn == VERSION && xdpi == 200 && (ydpi == 200 || ydpi == 100))
      giveup("input is not in Group3 format");
    printf("%%!PS-Adobe-3.0\n");
    printf("%%%%Creator: PostScript created by g3tops\n");
    printf("%%%%EndComments\n\n");
    int pn = 0;
    IBitpipe *ibp = new IBitpipe(stdin);
    unless (_setjmp(eof))
      { for (;;)
	  { FILE *tfi = tmpfile();
	    if (tfi == NULL) giveup("can't create temp file");
	    OBitpipe *obp = new OBitpipe(tfi);
	    volatile int nl = 0;	/* needed to defeat optimizer! */
	    unless (_setjmp(eop))
	      { for (;;)
		  { processline(ibp, obp);
		    nl++;
		  }
	      }
	    delete obp;
	    if (fflush(tfi) != 0) giveup("write error on temp file");
	    fprintf(stderr, "[%d] ", ++pn);
	    rewind(tfi); copyfile(tfi, stdout, pn, nl);
	    fclose(tfi);
	  }
      }
    delete ibp;
    printf("%%%%Trailer\n");
    if (pn > 0) putc('\n', stderr);
  }

static void processline(IBitpipe *ibp, OBitpipe *obp)
  { int neols = 0;
l:  int bw = 0, dots = 0;
    for (;;)
      { node *x = bw ? blacktree : whitetree;
	until (x == NULL || x -> tent != NULL)
	  { int b = ibp -> get();
	    x = b ? x -> rgt : x -> lft;
	  }
	if (x == NULL) giveup("Invalid code");
	// putc(' ', stderr);
	int count = x -> tent -> count;
	if (count < 0)
	  { /* EOL, possibly preceded by fill */
	    if (++neols > 3) _longjmp(eop, true);
	    goto l;
	  }
	for (int i = 0; i < count; i++) obp -> put(bw ^ 1);
	dots += count;
	if (count < 64)
	  { if (dots >= XDOTS) break;
	    bw ^= 1;	/* swap between white and black */
	  }
      }
    if (dots > XDOTS) giveup("bad line length: %d", dots);
  }

static void copyfile(FILE *ifi, FILE *ofi, int pn, int nl)
  { fprintf(ofi, "%%%%Page: g3tops %d\n", pn);
    fprintf(ofi, "gsave 0 7 translate %.3f %.3f scale ", 1.015 * A4WID * 72.0, 0.980 * A4HGT * 72.0);
    fprintf(ofi, "%d %d 1 [ %d 0 0 %d 0 %d ] ", XDOTS, nl, XDOTS, -nl, nl);
    fprintf(ofi, "currentfile /ASCIIHexDecode filter image\n");
    int ch = getc(ifi);
    while (ch >= 0) { putc(ch, ofi); ch = getc(ifi); }
    fprintf(ofi, "> grestore showpage\n\n");
  }

int IBitpipe::get()
  { if (len == 0)
      { int ch = getc(fi);
	if (ch < 0) _longjmp(eof, true);
	w = ch; len = 8;
      }
    int b = w >> 7;
    // putc('0'+b, stderr);
    w <<= 1; len--;
    return b;
  }

void OBitpipe::put(int b)
  { if (len == 8) flushbits();
    w = (w << 1) | b;
    len++;
  }

void OBitpipe::flushbits()
  { if (len > 0)
      { while (len < 8) put(1);	    /* pad with 1 = white */
	if (bpl >= 60) flushbytes();
	fprintf(fi, "%02x", w);
	bpl++; len = 0;
      }
  }

void OBitpipe::flushbytes()
  { if (bpl > 0)
      { putc('\n', fi); bpl = 0;
      }
  }

static void giveup(char *msg, word p1)
  { fprintf(stderr, "g3tops: "); fprintf(stderr, msg, p1); putc('\n', stderr);
    exit(1);
  }

