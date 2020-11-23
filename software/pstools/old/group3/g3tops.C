#include <stdio.h>

#define global
#define unless(x)   if(!(x))
#define until(x)    while(!(x))

#define VERSION	    1
#define G3TOPBM	    "/usr/fisher/mipsbin/g3topbm"

typedef unsigned int uint;
typedef unsigned char uchar;

union word
  { word(int nx)   { n = nx; }
    word(char *sx) { s = sx; }
    int n; char *s;
  };

extern "C"
  { ftruncate(int, int);
  };

static void usage();
static FILE *mktmpfile();
static void convertdoc();
static bool extractpage();
static void g3_to_pbm(), pbm_to_ps();
static void truncfile(FILE*);
static void giveup(char*, word = 0);

static FILE *g3file, *pbmfile;
static int xres, yres;


global void main(int argc, char **argv)
  { unless (argc == 1) usage();
    g3file = mktmpfile();
    pbmfile = mktmpfile();
    convertdoc();
    exit(0);
  }

static void usage()
  { fprintf(stderr, "Usage: g3tops\n");
    exit(2);
  }

static FILE *mktmpfile()
  { FILE *fi = tmpfile();
    if (fi == NULL) giveup("can't create temp file");
    return fi;
  }

static void convertdoc()
  { int vsn;
    int ni = scanf("!<Group 3> %d %d %d\n", &vsn, &xres, &yres);
    unless (ni == 3 && vsn == VERSION && xres == 200 && (yres == 200 || yres == 100))
      giveup("input is not in Group3 format");
    printf("%%!PS-Adobe-3.0\n");
    printf("%%%%Creator: PostScript created by g3tops\n");
    printf("%%%%EndComments\n\n");
    int pn = 0;
    bool ok = extractpage();
    while (ok)
      { fprintf(stderr, "[%d] ", ++pn);
	printf("%%%%Page: g3tops %d\n", pn);
	g3_to_pbm();
	pbm_to_ps();
	putchar('\n');
	ok = extractpage();
      }
    printf("%%%%Trailer\n");
    fprintf(stderr, "Wrote %d pages\n", pn);
  }

static bool extractpage()
  { /* Transfer one page from stdin to g3file */
    rewind(g3file);
    uint prevbits = ~0; int neols = 0;
    while (neols < 5)
      { int ch = getchar();
	if (ch < 0) return false;   /* ignore (we hope, short) partial page */
	putc(ch, g3file);
	uchar c = ch;
	for (int j = 0; j < 8 && neols < 5; j++)
	  { prevbits = (prevbits << 1) | (c >> 7);
	    c <<= 1;
	    /* 6 consecutive EOLs mark end of page; note that 2 consec. EOLs never occur in body of page */
	    if ((prevbits & 0xffffff) == 0x001001)
	      { fprintf(stderr, "Ping\n");
		neols++;
	      }
	  }
      }
    if (fflush(g3file) != 0) giveup("write error on temp file");
    truncfile(g3file);	/* truncate */
    return true;
  }

static void g3_to_pbm()
  { /* convert one page */
    rewind(g3file); rewind(pbmfile); truncfile(pbmfile);
    char cmd[256]; sprintf(cmd, "%s <&%d >&%d", G3TOPBM, fileno(g3file), fileno(pbmfile));
    int code = system(cmd);
    if (code != 0) giveup("g3topbm failed");
  }

static void pbm_to_ps()
  { /* convert one page */
    rewind(pbmfile);
    int nx, ny;
    int ni = fscanf(pbmfile, "P4\n%d %d\n", &nx, &ny);
    unless (ni == 2) giveup("fmt error in pbm file\n");
    printf("%% nx=%d ny=%d\n", nx, ny);
    float sx = (float) nx / (float) xres, sy = (float) ny / (float) yres;
    printf("gsave 0 7 translate %.3f %.3f scale ", sx*72.0, sy*72.0);
    printf("%d %d 1 [ %d 0 0 %d 0 %d ] ", nx, ny, nx, -ny, ny);
    printf("currentfile /ASCIIHexDecode filter image\n");
    int bpl = 0;
    int ch = getc(pbmfile);
    while (ch >= 0)
      { if (bpl >= 60) { putchar('\n'); bpl = 0; }
	printf("%02x", ch ^ 0xff);      /* bits are inverted! */
	bpl++;
	ch = getc(pbmfile);
      }
    if (bpl > 0) putchar('\n');
    printf("> grestore showpage\n");
  }

static void truncfile(FILE *fi)
  { fflush(fi);
    ftruncate(fileno(fi), ftell(fi));
  }

static void giveup(char *msg, word p1)
  { fprintf(stderr, "g3tops: "); fprintf(stderr, msg, p1); putc('\n', stderr);
    exit(1);
  }

