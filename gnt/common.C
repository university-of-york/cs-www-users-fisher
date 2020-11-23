#include <stdio.h>
#include <string.h>
#include <libcgi.h>

extern "C" {
  #include <gd.h>
};

#include "gnt.h"

#define VBMWID	700		/* width of verse bitmap */
#define VBMHGT	300		/* height of verse bitmap */	// WAS 200 but Rev 20:4 wdn't fit
#define WORDSP	10		/* Gk word spacing */
#define WORDWID 220		/* width of word bitmap */
#define CHHGT	30		/* height of character bitmap */
#define CHWID	24		/* width of character bitmap */

#define ZIPFILE	    "/www/usr/fisher/gnt/gnt.bin.gz"
#define BINFILE	    "/www/usr/fisher/tmpdir/gnt/gnt.bin"
#define TEMPDIR	    "/www/usr/fisher/tmpdir/gnt"
#define TEMPURL	    "/~fisher/tmpdir/gnt"

static FILE *binfile;
static int glyphindex[256];
static int zipfiletime, timenow;

global int fileptr;

static void makeversegif(Context, char*);
static void highlight(gdImage*, rect, int);
static void layoutverse(int, rect*);
static void makewordgif(char*, char*), makechargif(int, char*);
static void splatword(char*, gdImage*, point, int), splatchar(int, gdImage*, point&, int);
static void writegif(gdImage*, char*, int, int);
static void touchfile(char*), setfiletime(char*, int);

inline int max(int a, int b) { return (a > b) ? a : b; }

inline bool readable(char *fn)
  { /* return T if fn exists *and* is later than zip file */
    return (filetime(fn) >= zipfiletime);
  }


global void openbinfile()
  { timenow = time(NULL);
    zipfiletime = filetime(ZIPFILE);
    if (zipfiletime == 0) hfatal("%s does not exist!", ZIPFILE);
    unless (readable(BINFILE))
      { char cmd[MAXSTR+1];
	sprintf(cmd, "gunzip -cq %s >%s", ZIPFILE, BINFILE);
	int code = system(cmd);
	if (code != 0) unlink(BINFILE);
      }
    binfile = fopen(BINFILE, "r");
    if (binfile == NULL) hfatal("can't open %s!", BINFILE);
    char magic[12];
    if (fgets(magic, 12, binfile) == NULL || !seq(magic, "!GNT.1\n")) hfatal("Bad magic!");
    seekto(get3(25));	/* read glyph index */
    for (int i = 0; i < 256; i++) glyphindex[i] = next3();
    touchfile(BINFILE);		/* touch file so it doesn't get deleted */
  }

global void closebinfile()
  { fclose(binfile);
  }

global char *verseimage(Context ctxt)
  { char vid[9], fn[MAXSTR+1], img[MAXSTR+1];
    sprintf(vid, "%02d%02d%02d", ctxt.bk, ctxt.ch, ctxt.vn);
    if (ctxt.wn >= 0) sprintf(&vid[6], "%02d", ctxt.wn);
    sprintf(fn, "%s/%s.gif", TEMPDIR, vid);
    unless (readable(fn)) makeversegif(ctxt, fn);
    touchfile(fn);
    sprintf(img, "<img src=%s/%s.gif ismap border=0>", TEMPURL, vid);
    return copystring(img);
  }

static void makeversegif(Context ctxt, char *gfn)
  { int vso = findvso(ctxt);	/* seek to verse */
    int nwd = get1(vso);
    rect *rects = new rect[nwd];
    layoutverse(vso, rects);
    gdImage *gd = gdImageCreate(VBMWID, VBMHGT);
    gdImageColorAllocate(gd, 255, 255, 255); /* white background */
    int black = gdImageColorAllocate(gd, 0, 0, 0);
    int red = gdImageColorAllocate(gd, 255, 85, 85);
    for (int n = 0; n < nwd; n++)
      { rect r = rects[n];
	if (n == ctxt.wn) highlight(gd, r, red);
	int wdo = get3(vso + 1 + 13*n);
	char *grks = getword(wdo);
	splatword(grks, gd, point(r.x,r.y), black);
	delete grks;
      }
    int hgt = (nwd > 0) ? (rects[nwd-1].y + rects[nwd-1].h) : CHHGT;	/* height used in bitmap */
    writegif(gd, gfn, VBMWID, hgt);
    gdImageDestroy(gd);
    delete rects;
  }

static void highlight(gdImage *gd, rect r, int col)
  { gdImageFilledRectangle(gd, r.x, r.y, r.x + r.w - 1, r.y + r.h - 1, col);
  }

global void locateword(point clickpt, Context &ctxt)
  { int vso = findvso(ctxt);	/* seek to verse */
    int nwd = get1(vso);
    rect *rects = new rect[nwd];
    layoutverse(vso, rects);
    ctxt.wn = -1;
    for (int i = 0; i < nwd; i++)
      { rect r = rects[i];
	if (clickpt.x >= r.x && clickpt.x < r.x+r.w && clickpt.y >= r.y && clickpt.y < r.y+r.h)
	  { ctxt.wn = i;
	    break;
	  }
      }
    delete rects;
  }

static void layoutverse(int vso, rect *rects)
  { int nwd = get1(vso);
    point currpt = point(WORDSP, 0);
    for (int n = 0; n < nwd; n++)
      { int wdo = get3(vso + 1 + 13*n);
	int ngl = get1(wdo);
	int wwid = 0;
	for (int i = 0; i < ngl; i++)
	  { int c = get1(wdo + 1 + i);
	    int glo = glyphindex[c];
	    int gwid = get1(glo);
	    wwid += gwid;
	  }
	if (currpt.x + wwid + WORDSP > VBMWID)
	  { currpt.x = WORDSP;
	    currpt.y += CHHGT;
	  }
	if (currpt.y + CHHGT >= VBMHGT) hfatal("verse won't fit in bitmap!");
	rects[n] = rect(currpt.x, currpt.y, wwid, CHHGT);
	currpt.x += (wwid + WORDSP);
      }
  }

global char *grksimage(char *grks)
  { if (grks == NULL || grks[0] == '\0') return wordimage(0);	/* blank */
    char uid[16], fn[MAXSTR+1], img[MAXSTR+1];
    sprintf(uid, "U%07d", uniqueid());
    sprintf(fn, "%s/%s.gif", TEMPDIR, uid);
    makewordgif(grks, fn);
    sprintf(img, "<img src=%s/%s.gif>", TEMPURL, uid);
    return copystring(img);
  }

global char *wordimage(int wdo)
  { char wid[16], fn[MAXSTR+1], img[MAXSTR+1];
    sprintf(wid, "W%07d", wdo);
    sprintf(fn, "%s/%s.gif", TEMPDIR, wid);
    unless (readable(fn))
      { char *grks = (wdo != 0) ? getword(wdo) : (char*) NULL;
	makewordgif(grks, fn);
	delete grks;
      }
    touchfile(fn);
    sprintf(img, "<img src=%s/%s.gif>", TEMPURL, wid);
    return copystring(img);
  }

static void makewordgif(char *grks, char *gfn)
  { gdImage *gd = gdImageCreate(WORDWID, CHHGT);
    gdImageColorAllocate(gd, 255, 255, 255);
    if (grks != NULL)
      { int black = gdImageColorAllocate(gd, 0, 0, 0);
	splatword(grks, gd, point(0,0), black);
      }
    writegif(gd, gfn, WORDWID, CHHGT);
    gdImageDestroy(gd);
  }

global char *charimage(int ch)
  { char name[MAXSTR+1], fn[MAXSTR+1], img[MAXSTR+1];
    if (ch >= VOWELS) sprintf(name, "D%03d", ch-VOWELS); else sprintf(name, "C%03o", ch);
    sprintf(fn, "%s/%s.gif", TEMPDIR, name);
    unless (readable(fn)) makechargif(ch, fn);
    touchfile(fn);
    sprintf(img, "<img src=%s/%s.gif>", TEMPURL, name);
    return copystring(img);
  }

static void makechargif(int ch, char *gfn)
  { gdImage *gd = gdImageCreate(WORDWID, CHHGT);
    gdImageColorAllocate(gd, 255, 255, 255);
    int black = gdImageColorAllocate(gd, 0, 0, 0);
    point p = point(0,0);
    splatchar(ch, gd, p, black);	/* updates p */
    writegif(gd, gfn, p.x, CHHGT);	/* truncate to exact width */
    gdImageDestroy(gd);
  }

static void splatword(char *grks, gdImage *gd, point p, int col)
  { for (int i = 0; grks[i] != '\0'; i++) splatchar((uchar) grks[i], gd, p, col);
  }

static void splatchar(int ch, gdImage *gd, point &p, int col)
  { int glo = glyphindex[ch & 0xff];
    int gwid = get1(glo);
    for (int j = 0; j < CHHGT; j++)
      { int w = next3();
	for (int k = 0; k < CHWID; k++)	    /* set all 24 pixels */
	  { if (w & 0x800000) gdImageSetPixel(gd, p.x + k, p.y + j, col);
	    w <<= 1;
	  }
      }
    p.x += gwid;
  }

static void writegif(gdImage *gd, char *gfn, int wid, int hgt)
  { int ssx = gd -> sx, ssy = gd -> sy;	    /* save width & height */
    if (wid < gd -> sx) gd -> sx = wid;	    /* truncate width */
    if (hgt < gd -> sy) gd -> sy = hgt;	    /* truncate height */
    FILE *fi = fopen(gfn, "w");
    if (fi == NULL) hfatal("can't create %s!", gfn);
    gdImageGif(gd, fi);
    fclose(fi);
    gd -> sx = ssx; gd -> sy = ssy;	    /* restore width & height */
  }

static void touchfile(char *fn)
  { setfiletime(fn, max(zipfiletime, timenow)); /* update file time so it doesn't get deleted */
  }

static void setfiletime(char *fn, int t)
  { int tim[] = { t, t };
    utime(fn, tim);
  }

global char *bookname(int bk)
  { int bix = get3(16);
    int nbk = get1(bix);
    if (bk <= 0 || bk > nbk) hfatal("can't find %02d!", bk);
    int bko = get3(bix + 1 + 6*(bk-1));
    return getword(bko);
  }

global int textstart(int bk)
  { int bix = get3(16);
    int nbk = get1(bix);
    return (bk > nbk) ? get3(31) :	/* special case */
			findvso(Context(bk,1,1,0));
  }

global int findvso(Context ctxt)
  { int cho = findcho(ctxt);	/* seek to chapter */
    int nvs = get1(cho);
    if (ctxt.vn <= 0 || ctxt.vn > nvs) hfatal("can't find %02d.%02d.%02d!", ctxt.bk, ctxt.ch, ctxt.vn);
    return get3(cho + 1 + 3*(ctxt.vn-1));
  }

global int findcho(Context ctxt)
  { int bix = get3(16);
    int nbk = get1(bix);
    if (ctxt.bk <= 0 || ctxt.bk > nbk) hfatal("can't find %02d!", ctxt.bk);
    int bko = get3(bix + 4 + 6*(ctxt.bk-1));
    int nch = get1(bko);
    if (ctxt.ch <= 0 || ctxt.ch > nch) hfatal("can't find %02d.%02d!", ctxt.bk, ctxt.ch);
    return get3(bko + 1 + 3*(ctxt.ch-1));
  }

global char *getword(int wdo)
  { int nch = get1(wdo);
    if (nch > MAXSTR) hfatal("Word too long!");
    char vec[MAXSTR+1];
    for (int i = 0; i < nch; i++) vec[i] = next1();
    vec[nch] = '\0';
    return copystring(vec);
  }

global int get4(int offs)
  { seekto(offs);
    return next4();
  }

global int next4()
  { uint n = 0;
    for (int i = 0; i < 4; i++)
      { int b = next1();
	n = (n >> 8) | (b << 24);
      }
    return n;
  }

global int get3(int offs)
  { seekto(offs);
    return next3();
  }

global int next3()
  { uint n = 0;
    for (int i = 0; i < 3; i++)
      { int b = next1();
	n = (n >> 8) | (b << 16);
      }
    return n;
  }

global int get1(int offs)
  { seekto(offs);
    return next1();
  }

global int next1()
  { int ch = getc(binfile);
    if (ch < 0) hfatal("premature eof!");
    fileptr++;
    return ch;
  }

global void seekto(int offs)
  { fileptr = offs;
    fseek(binfile, fileptr, 0);
  }

