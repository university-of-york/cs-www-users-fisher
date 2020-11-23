/* bbox -- read a PostScript file, deduce bounding box, and either
   report the bounding box coords, or
   add an appropriate %%BoundingBox comment to the PostScript file
   AJF	 January 1993 */

#include "gfxlib.h"

#include <stdio.h>

#define global
#define forward
#define ushort	   unsigned short
#define uint	   unsigned int
#define word	   int
#define bool	   int
#define until(x)   while(!(x))
#define unless(x)  if(!(x))

#define MAXSTRLEN  256

#define opt_report 1	/* -r : report bbox coords			*/
#define opt_addbb  2	/* -a : add %%BoundingBox comment to PS file	*/
#define opt_border 4	/* -b : insert border round bbox (used with -a) */

struct bbox { int llx, lly, urx, ury };		/* dim. in points */

static uint opts;
static char *psfn;
static struct bitmap *bitmap;
static struct bbox bbox;
static int tempnum;

extern struct bitmap *ReadBitmap();
extern char *malloc();

forward char *newtemp();
forward tidytemp();


global main(argc, argv) int argc; char *argv[];
  { readcmdline(argc, argv);
    tempnum = 0;
    atexit(tidytemp);
    if (opts & opt_report) reportbbox();
    if (opts & opt_addbb) addbbox();
    exit(0);
  }

static readcmdline(argc, argv) int argc; char *argv[];
  { char *s; int k;
    unless (argc == 2 || argc == 3) usage();
    s = argv[1]; k = 0;
    unless (s[k++] == '-') usage();
    opts = 0;
    until (s[k] == '\0')
      { int c = s[k++];
	if (c == 'r') opts |= opt_report;
	else if (c == 'a') opts |= opt_addbb;
	else if (c == 'b') opts |= opt_border;
	else usage();
      }
    psfn = (argc == 3) ? argv[2] : NULL;
  }

static usage()
  { fprintf(stderr, "Usage: bbox -ar [f.ps]\n");
    exit(1);
  }

static reportbbox()
  { if (opts & (opt_addbb | opt_border)) badoptions();
    computebbox();
    prbbox();
  }

static addbbox()
  { FILE *fi; int nc;
    char line[MAXSTRLEN];
    if (opts & opt_report) badoptions();
    computebbox();
    fi = fopen(psfn, "r");
    if (fi == NULL) fail("can't open %s", psfn);
    nc = rdline(fi, line);
    unless (nc >= 2 && strncmp(line, "%!", 2) == 0)
      fail("file %s is not a PostScript file (doesn't start with %%!)", psfn);
    wrline(line, nc);
    printf("%%%%BoundingBox: "); prbbox();
    nc = rdline(fi, line);
    while (nc >= 0 && strncmp(line, "%%", 2) == 0)
      { wrline(line, nc);
	nc = rdline(fi, line);
      }
    if (opts & opt_border)
      { printf("%% --- added by bbox\n");
	printf("newpath ");
	printf("%d %d moveto ", bbox.llx, bbox.lly);
	printf("%d %d lineto ", bbox.urx, bbox.lly);
	printf("%d %d lineto ", bbox.urx, bbox.ury);
	printf("%d %d lineto ", bbox.llx, bbox.ury);
	printf("%d %d lineto ", bbox.llx, bbox.lly);
	printf("stroke\n");
	printf("%% --- \n");
      }
    while (nc >= 0)
      { wrline(line, nc);
	nc = rdline(fi, line);
      }
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
    unless (ch == EOF || ch == '\n') fail("line too long in PostScript file!");
    return nc;
  }

static wrline(line, nc) char line[]; int nc;
  { int i;
    for (i=0; i<nc; i++) putchar(line[i]);
    putchar('\n');
  }

static badoptions()
  { fail("bad combination of options");
  }

static prbbox()
  { printf("%d %d %d %d\n", bbox.llx, bbox.lly, bbox.urx, bbox.ury);
  }

static computebbox()
  { struct rect rect;
    ensurefile();
    makebitmap();
    ComputeBBox(bitmap, &rect);
    /* PS coords are "Y is up"; BM coords are "Y is down" */
    bbox.llx = rect.left;
    bbox.urx = rect.left + rect.width;
    bbox.lly = (bitmap -> hgt) - (rect.top + rect.height);
    bbox.ury = (bitmap -> hgt) - (rect.top);
  }

static ensurefile()
  { /* if input is stdin, put it into a "real" file */
    if (psfn == NULL)
      { FILE *fi; int c;
	psfn = newtemp();
	fi = fopen(psfn, "w");
	if (fi == NULL) fail("can't create %s", psfn);
	c = getchar();
	until (c == EOF)
	  { putc(c, fi);
	    c = getchar();
	  }
	fclose(fi);
      }
  }

static makebitmap()
  { char *bmfn; char cmd[MAXSTRLEN+1]; int code;
    bmfn = newtemp();
    /* -ppm option creates a Portable PixMap which is not cropped to the BoundingBox
      (which we don't yet know!) */
    sprintf(cmd, "/usr/fisher/mipsbin/pstobm -ppm %s %s", psfn, bmfn);
    code = system(cmd);
    if (code != 0) fail("bbox: pstobm failed, code %d", code);
    bitmap = ReadBitmap(NULL, bmfn, BM_BITS);
    if (bitmap == NULL) fail("bbox: can't read bitmap file %s", bmfn);
    unlinktemp(); /* delete bmfn */
  }

static char *newtemp()
  { char *fn = malloc(20);
    if (fn == NULL) fail("no room!");
    makename(fn, ++tempnum);
    return fn;
  }

static tidytemp()
  { /* delete temp. files; called by exit() */
    while (tempnum > 0) unlinktemp();
  }

static unlinktemp()
  { char fn[20];
    makename(fn, tempnum--);
    unlink(fn);
  }

static makename(fn, k) char *fn; int k;
  { sprintf(fn, "/tmp/bbox_%05d%04d", getpid(), k);
  }

static fail(msg, p1) char *msg; word p1;
  { fprintf(stderr, "bbox: ");
    fprintf(stderr, msg, p1); putc('\n', stderr);
    exit(1);
  }

