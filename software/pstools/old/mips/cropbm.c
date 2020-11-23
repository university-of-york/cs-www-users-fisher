#define global
#define forward
#define bool	    int
#define false	    0
#define true	    1
#define unless(x)   if(!(x))
#define until(x)    while(!(x))
#define seq(s1,s2)  (strcmp(s1,s2) == 0)

#define MAXSTRLEN 80

#include "gfxlib.h"

#include <stdio.h>
#include <X11/Xlib.h>

extern struct bitmap *ReadBitmap(), *Balloc();

static Display *display;
static Window rootwin;
static GC gc_copy1;

forward GC makegc();


global main(argc, argv) int argc; char *argv[];
  { bool ok, border; struct bitmap *bm1, *bm2;
    int bbox[4]; int wid, hgt;
    border = false;
    if (argc >= 2 && seq(argv[1],"-b"))
      { border = true;
	argc--; argv++;
      }
    unless (argc == 3) usage();
    initgraphics();
    bm1 = ReadBitmap(display, argv[2], BM_PIXM);
    if (bm1 == NULL) giveup("can't read bitmap file %s", argv[2]);
    getbbox(argv[1], bbox);
    wid = bbox[2] - bbox[0];
    hgt = bbox[3] - bbox[1];
    bm2 = Balloc(display, wid, hgt, 1, BM_PIXM);
    XCopyArea(display, bm1 -> pixm, bm2 -> pixm, gc_copy1, bbox[0], (bm1 -> hgt) - bbox[3], wid, hgt, 0, 0);
    if (border) XDrawRectangle(display, bm2 -> pixm, gc_copy1, 0, 0, wid, hgt);
    ok = WriteBitmap(display, bm2, argv[2], 'c'); /* write in xor'd binary format */
    unless (ok) giveup("can't write bitmap file %s", argv[2]);
    exit(0);
  }

static usage()
  { fprintf(stderr, "Usage: cropbm [-b] fn.ps fn.bm\n");
    exit(1);
  }

static initgraphics()
  { int snum;
    display = XOpenDisplay(NULL);
    if (display == NULL) giveup("XOpenDisplay failed");
    snum = DefaultScreen(display);
    rootwin = RootWindow(display, snum);
    gc_copy1 = makegc(GXcopy, 1, 1, 0);
  }

static GC makegc(gx, d, fg, bg) int gx; int d, fg, bg;
  { XGCValues values; Pixmap px; GC gc;
    values.function = gx;
    values.foreground = fg;
    values.background = bg;
    px = XCreatePixmap(display, rootwin, 1, 1, d); /* create a dummy pixmap of desired depth */
    gc = XCreateGC(display, px, GCFunction | GCForeground | GCBackground, &values);
    XFreePixmap(display, px);
    return gc;
  }

static getbbox(fn, bbox) char *fn; int *bbox;
  { char line[MAXSTRLEN]; int nc; bool ok;
    FILE *fi = fopen(fn, "r");
    if (fi == NULL) giveup("can't read PostScript file %s", fn);
    nc = rdline(fi, line);
    ok = false;
    while (nc >= 0 && !ok)
      { if (strncmp(line, "%%BoundingBox:", 14) == 0)
	  { int ni = sscanf(&line[14], " %ld %ld %ld %ld", &bbox[0], &bbox[1], &bbox[2], &bbox[3]);
	    if (ni == 4) ok = true;
	  }
	nc = rdline(fi, line);
      }
    unless (ok) giveup("no valid %%%%BoundingBox in file %s", fn);
    fclose(fi);
  }

static int rdline(fi, line) FILE *fi; char line[];
  { int nc = 0;
    int ch = getc(fi);
    until (ch == '\n' || ch == EOF || nc >= MAXSTRLEN)
      { line[nc++] = ch;
	ch = getc(fi);
      }
    if (ch == EOF) nc = -1;
    /* we don't care if the line's too long;
       we're only looking at the first few chars anyway */
    until (ch == EOF || ch == '\n') ch = getc(fi);
    return nc;
  }

static giveup(msg, p1) char *msg, *p1;
  { fprintf(stderr, "cropbm: "); fprintf(stderr, msg, p1); putc('\n', stderr);
    exit(1);
  }

