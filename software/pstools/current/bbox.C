/* bbox -- read a PostScript file, deduce bounding box, and either
   report the bounding box coords, or
   add an appropriate %%BoundingBox comment to the PostScript file
   AJF	 January 1993
   C++ version, calls ps2epsi	AJF   December 1998 */

#include <stdio.h>
#include <string.h>

#define global
#define forward
#define until(x)   while(!(x))
#define unless(x)  if(!(x))

#define MAXSTRLEN  256

#define opt_report 1	/* -r : report bbox coords			*/
#define opt_addbb  2	/* -a : add %%BoundingBox comment to PS file	*/

typedef unsigned int uint;
typedef void (*proc)();

extern "C"
  { proc set_new_handler(proc);
    void atexit(proc), unlink(char*);
  };

static void newhandler(), tidytemps(), readcmdline(int, char**), usage(), ensurefile();
static void findbbox(), addbbox(char*);
static bool rdline(FILE*, char*);
static void fail(char*, char* = NULL);

inline bool starts(char *s1, char *s2) { return strncmp(s1, s2, strlen(s2)) == 0; }
inline bool seq(char *s1, char *s2)    { return strcmp(s1, s2) == 0;		  }

static uint opts;
static char *psfn;
static char pstemp[12], epsitemp[12];


global void main(int argc, char **argv)
  { set_new_handler(newhandler);
    strcpy(pstemp, "bb1.XXXXXX"); mktemp(pstemp);
    strcpy(epsitemp, "bb2.XXXXXX"); mktemp(epsitemp);
    atexit(tidytemps);
    readcmdline(argc, argv);
    ensurefile();
    findbbox();
    exit(0);
  }

static void newhandler()
  { fail("no room!");
  }

static void tidytemps()
  { unlink(pstemp); unlink(epsitemp);
  }

static void readcmdline(int argc, char **argv)
  { unless (argc == 2 || argc == 3) usage();
    char *s = argv[1]; int k = 0;
    unless (s[k++] == '-') usage();
    opts = 0;
    until (s[k] == '\0')
      { int c = s[k++];
	if (c == 'r') opts |= opt_report;
	else if (c == 'a') opts |= opt_addbb;
	else usage();
      }
    psfn = (argc == 3) ? argv[2] : NULL;
  }

static void usage()
  { fprintf(stderr, "Usage: bbox [-{ar}] [f.ps]\n");
    exit(1);
  }

static void ensurefile()
  { /* if input is stdin, put it into a "real" file */
    if (psfn == NULL)
      { psfn = pstemp;
	FILE *fi = fopen(psfn, "w");
	if (fi == NULL) fail("can't create %s", psfn);
	int c = getchar();
	while (c >= 0) { putc(c, fi); c = getchar(); }
	fclose(fi);
      }
  }

static void findbbox()
  { char cmd[MAXSTRLEN+1];
    sprintf(cmd, "/york/mips/bin/ps2epsi %s %s", psfn, epsitemp);
    if (system(cmd) != 0) fail("command failed: %s", cmd);
    FILE *fi = fopen(epsitemp, "r");
    if (fi == NULL) fail("can't open temp EPSI file %s for reading", epsitemp);
    char bbline[MAXSTRLEN+1];
    bool ok = rdline(fi, bbline);
    while (ok && !starts(bbline, "%%BoundingBox: ")) ok = rdline(fi, bbline);
    fclose(fi);
    unless (ok) fail("no %%%%BoundingBox in EPSI file");
    if (opts & opt_report) puts(&bbline[15]);
    if (opts & opt_addbb) addbbox(bbline);
  }

static void addbbox(char *bbline)
  { FILE *fi = fopen(psfn, "r");
    if (fi == NULL) fail("can't open %s", psfn);
    char line[MAXSTRLEN];
    bool ok = rdline(fi, line);
    unless (ok && starts(line, "%!")) fail("file %s is not a PostScript file (doesn't start with %%!)", psfn);
    puts(line);
    puts(bbline);   /* add the BoundingBox line */
    ok = rdline(fi, line);
    while (ok)
      { puts(line);
	ok = rdline(fi, line);
      }
    fclose(fi);
  }

static bool rdline(FILE *fi, char *line)
  { int nc = 0;
    int ch = getc(fi);
    until (ch == '\n' || ch < 0 || nc >= MAXSTRLEN)
      { line[nc++] = ch;
	ch = getc(fi);
      }
    unless (ch == '\n' || ch < 0) fail("line too long in PostScript file!");
    line[nc] = '\0';
    return !(nc == 0 && ch < 0);
  }

static void fail(char *msg, char *p1)
  { fprintf(stderr, "bbox: ");
    fprintf(stderr, msg, p1); putc('\n', stderr);
    exit(1);
  }

