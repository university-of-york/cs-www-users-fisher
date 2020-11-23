/* fifi -- filter filter
   AJF	 September 1996 */

#include <stdio.h>
#include <regexp.h>
#include <string.h>

#define global
#define unless(x)   if(!(x))
#define until(x)    while(!(x))

#define MAXSTR	    256
#define MAXFSPECS   100

#ifdef mips
   #define MKFDIR     "/usr/fisher/mipsbin"
#endif

union word
  { word(int nx)   { n = nx; }
    word(char *sx) { s = sx; }
    int n; char *s;
  };

typedef void (*proc)();

extern "C"
  { proc set_new_handler(proc);
    double fmod(double, double);
    void free(void*);
  };

static void newhandler(), usage();
static FILE *openfile(char*, char*);
static void pass1();
static char *getsubst(regexp*, int);
static void pass2(), setlinenum(int, char*), writefspecs();
static void appends(char*, int&, char*, word = 0, word = 0, word = 0);
static bool readline(FILE*);
static void writeline(FILE*);
static regexp *x_regcomp(char*);
static bool starts(char*, char*);
static void giveup(char*, word = 0, word = 0);

static FILE *tempfile, *infile, *outfile;
static char line[MAXSTR+1];
static char *fspeclist[MAXFSPECS];
static int linenum, numfspecs;
static char *sourcefn;


global main(int argc, char **argv)
  { set_new_handler(newhandler);
    unless (argc == 3) usage();
    tempfile = tmpfile();
    if (tempfile == NULL) giveup("can't create temp file");
    sourcefn = argv[1];
    infile = openfile(sourcefn, "r");
    outfile = openfile(argv[2], "w");
    pass1();
    rewind(tempfile);
    pass2();
    fclose(tempfile); fclose(infile); fclose(outfile);
    exit(0);
  }

static void newhandler()
  { giveup("no room!");
  }

static void usage()
  { fprintf(stderr, "Usage: fifi infn.F outfn.C\n");
    exit(2);
  }

static FILE *openfile(char *fn, char *rw)
  { FILE *fi = fopen(fn, rw);
    if (fi == NULL)
      { char *ops = (rw[0] == 'r') ? "open" : "create";
	giveup("can't %s %s", ops, fn);
      }
    return fi;
  }

static void pass1()
  { regexp *re = x_regcomp("(.*)mkfilter\\(\"([^\"]*)\"\\)(.*)");
    numfspecs = linenum = 0;
    bool ok = readline(infile);
    while (ok)
      { if (regexec(re, line, true))
	  { char *s1 = getsubst(re, 1), *s2 = getsubst(re, 2), *s3 = getsubst(re, 3);
	    if (numfspecs >= MAXFSPECS) giveup("too many filter specs!");
	    fspeclist[numfspecs++] = s2;				/* arg of mkfilter(...) */
	    sprintf(line, "%s(&_fspecs_%d)%s", s1, numfspecs, s3);      /* replace "mkfilter(...)" by "(&_fspecs_n)" */
	    delete s1; delete s3;
	  }
	writeline(tempfile);
	ok = readline(infile);
      }
    free(re);
  }

static char *getsubst(regexp *re, int ns)
  { char *s1 = re -> startp[ns], *s2 = re -> endp[ns];
    unless (s1 != NULL && s2 != NULL && s2 >= s1) giveup("bad regexp subst!");
    int len = s2-s1;
    char *s = new char[len+1];
    memcpy(s, s1, len); s[len] = '\0';
    return s;
  }

static void pass2()
  { regexp *re = x_regcomp("^[ \t]*#include [<\"]filters.h[>\"]");
    linenum = 0;
    bool ok = readline(tempfile);
    bool incl = false;
    setlinenum(linenum, sourcefn);
    while (ok && !incl)
      { writeline(outfile);
	if (regexec(re, line, true)) incl = true;
	ok = readline(tempfile);
      }
    unless (incl) giveup("no ``#include'' of file ``filters.h'' in input file");
    setlinenum(1, "<<generated_fspecs>>");
    writefspecs();
    setlinenum(linenum, sourcefn);
    while (ok)
      { writeline(outfile);
	ok = readline(tempfile);
      }
    free(re);
  }

static void setlinenum(int ln, char *fn)
  { fprintf(outfile, "#line %d \"%s\"\n", ln, fn);
  }

static void writefspecs()
  { putc('\n', outfile);
    for (int i=0; i < numfspecs; i++)
      { char *spec = fspeclist[i];
	char *prep = starts(spec, "-Av") ? "mkaverage" : "mkfilter";
	char cmd[MAXSTR+1]; int p = 0;
	appends(cmd, p, "%s/%s %s -l | ", MKFDIR, prep, spec);
	appends(cmd, p, "%s/gencode -f", MKFDIR);
	FILE *pfi = popen(cmd, "r");
	if (pfi == NULL) giveup("popen failed! (%s)", cmd);
	int ch = getc(pfi);
	until (ch < 0)
	  { if (ch == '$') fprintf(outfile, "%d", i+1);
	    else putc(ch, outfile);
	    ch = getc(pfi);
	  }
	int code = pclose(pfi);
	if (code != 0) giveup("gencode failed: cmd was \"%s\"\n", cmd);
      }
  }

static void appends(char *cmd, int &p, char *fmt, word p1, word p2, word p3)
  { sprintf(&cmd[p], fmt, p1, p2, p3);
    until (cmd[p] == '\0') p++;
  }

static bool readline(FILE *fi)
  { linenum++;
    int n = 0, ch = getc(fi);
    until (n >= MAXSTR || ch == '\n' || ch < 0)
      { line[n++] = ch;
	ch = getc(fi);
      }
    unless (ch == '\n' || ch < 0) giveup("line too long!");
    line[n] = '\0';
    return !(ch < 0 && n == 0);
  }

static void writeline(FILE *fi)
  { fputs(line, fi); putc('\n', fi);
  }

static regexp *x_regcomp(char *str)
  { regexp *re = regcomp(str);
    if (re == NULL) giveup("regcomp failed! (%s)", str);
    return re;
  }

static bool starts(char *s1, char *s2)
  { int len1 = strlen(s1), len2 = strlen(s2);
    return (len1 >= len2 && strncmp(s1, s2, len2) == 0);
  }

static void giveup(char *msg, word p1, word p2)
  { fprintf(stderr, "fifi: "); fprintf(stderr, msg, p1, p2); putc('\n', stderr);
    exit(1);
  }

