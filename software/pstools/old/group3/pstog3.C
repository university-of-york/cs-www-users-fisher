/* pstog3 - convert multi-page PostScript (on stdin) to Group 3 fax format (on stdout) - AJF March 1995
   Based on: pstops - rearrange pages in conforming PS file for printing in signatures - AJF January 1994
   Based on: pstops by AJCD 27/1/91
*/

#include <stdio.h>
#include <string.h>
#include <fcntl.h>

#define global
#define unless(x)   if(!(x))
#define until(x)    while(!(x))

#define VERSION	    1
#define TMPDIR	    "/tmp"
#define MAXPAGES    5000	/* max pages in document */
#define PBMBIN	    "/york/bin/pbm"
#define MYBIN	    "/usr/fisher/mipsbin"

#define opt_l	2   /* low resolution */
#define opt_n	4   /* no header      */

typedef unsigned int uint;
typedef unsigned char uchar;

static FILE *infile, *outfile;
static uchar options;
static int numpages;
static int headerlen, prologlen, filelength;
static int pageptr[MAXPAGES+1]; /* 1 extra slot for trailer pointer */

static void parseargs(char**), usage();
static bool is_seekable(FILE*);
static FILE *make_seekable(FILE*);
static void scanpages(), writeheader(), writeprologue(), writesetup();
static void selectpage(int), copypagebody(int), writetrailer();
static void readbuf(char*, char*), fcopy(int, char*);
static void ioerror(char*), giveup(char*, char* = NULL);

inline bool seq(char *s1, char *s2)	{ return strcmp(s1,s2) == 0;		   }
inline bool starts(char *s1, char *s2)	{ return strncmp(s1, s2, strlen(s2)) == 0; }

extern "C"
  { char *getenv(char*);
    bool isatty(int);
  };


global void main(int argc, char **argv)
  { parseargs(argv);
    infile = (is_seekable(stdin)) ? stdin : make_seekable(stdin);
    scanpages();
    unless (options & opt_n)
      { printf("!<Group3> %d 200 %d\n", VERSION, (options & opt_l) ? 100 : 200); /* pretend horiz resolution is 200 dpi */
	fflush(stdout);
      }
    for (int pn = 0; pn < numpages; pn++)
      { char cmd[256];
	/* horiz dpi "207.7" is chosen to give 1728 dots per scan line, req'd by fax standard */
	sprintf(cmd, "%s/pstobm -ppm -m -dpi 207.7 %d - - | %s/pbmtog3",
		     MYBIN, (options & opt_l) ? 100 : 200, PBMBIN);
	outfile = popen(cmd, "w");
	if (outfile == NULL) giveup("can't create pipe to pstobm | pstog3");
	writeheader();
	writeprologue();
	writesetup();
	selectpage(pn);
	writetrailer();
	if (pclose(outfile) != 0) giveup("pstobm | pbmtog3 failed");
      }
    fprintf(stderr, "Wrote %d pages\n", numpages);
    exit(0);
  }

static void parseargs(char **argv)
  { options = 0;
    int ap = 0;
    unless (argv[ap] == NULL) ap++;
    until (argv[ap] == NULL)
      { char *s = argv[ap++];
	int k = 0;
	unless (s[k++] == '-') usage();
	until (s[k] == '\0')
	  { char ch = s[k++];
	    if (ch == 'l') options |= opt_l;
	    else if (ch == 'n') options |= opt_n;
	    else usage();
	  }
      }
  }

static void usage()
  { fprintf(stderr, "Usage: pstog3 [-[lvn]]\n");
    exit(2);
  }

static bool is_seekable(FILE *fi)
  { return (fseek(fi, 0, 1) == 0 && !isatty(fileno(fi)));
  }

static FILE *make_seekable(FILE *ifi)
  { FILE *ofi = tmpfile();
    if (ofi == NULL) giveup("can't create temp file");
    int ch = getc(ifi);
    while (ch >= 0) { putc(ch, ofi); ch = getc(ifi); }
    if (fflush(ofi) != 0) giveup("write error on temp file");
    return ofi;
  }

static void scanpages()
  { /* build array of pointers to start/end of pages */
    char buf[BUFSIZ];
    int nesting = 0;
    bool gottr = false;
    numpages = headerlen = prologlen = 0;
    fseek(infile, 0, 0);
    while (!gottr && fgets(buf, BUFSIZ, infile) != NULL)
      { if (buf[0] == '%' && buf[1] == '%')
	  { char *comment = &buf[2];
	    if (nesting == 0 && starts(comment, "Page:"))
	      { if (numpages >= MAXPAGES) giveup("too many pages in input file!");
		pageptr[numpages++] = ftell(infile) - strlen(buf);
	      }
	    else if (headerlen == 0 && starts(comment, "EndComments")) headerlen = ftell(infile);
	    else if (prologlen == 0 && starts(comment, "EndProlog"))
	      { prologlen = ftell(infile);
		if (headerlen == 0) headerlen = prologlen;
	      }
	    else if (starts(comment, "BeginDocument")) nesting++;
	    else if (starts(comment, "EndDocument")) nesting--;
	    else if (nesting == 0 && starts(comment, "Trailer"))
	      { fseek(infile, -strlen(buf), 1);
		gottr = true;
	      }
	  }
	else
	  { /* a regular line signifies end of header and start of prolog */
	    if (buf[0] != '%' || (buf[1] == ' ' || buf[1] == '\t' || buf[1] == '\0'))
	      if (headerlen == 0) headerlen = ftell(infile) - strlen(buf);
	  }
      }
    pageptr[numpages] = ftell(infile);	    /* pointer to trailer */
    fseek(infile, 0, 2);		    /* seek to end of file */
    filelength = ftell(infile);		    /* size of file */
  }

static void writeheader()
  { /* write header, altering "%%Pages:" comment */
    int nb = 0;
    fseek(infile, 0, 0);
    while (nb < headerlen)
      { char buf[BUFSIZ];
	readbuf(buf, "copying header");
	unless (starts(buf, "%%Pages:") || starts(buf, "%%EndComments")) fputs(buf, outfile);
	nb += strlen(buf);
      }
    fprintf(outfile, "%%%%Pages: 1\n");
    fprintf(outfile, "%%%%EndComments\n");
  }

static void writeprologue()
  { /* write prologue (between end of header and %%EndProlog) */
    fseek(infile, headerlen, 0);
    fcopy(prologlen - headerlen, "copying prologue");
  }

static void writesetup()
  { /* write setup section (between end of prologue and start of first page) */
    fseek(infile, prologlen, 0);
    fprintf(outfile, "%%%%BeginSetup\n");
    int nb = prologlen;
    while (nb < pageptr[0])
      { char buf[BUFSIZ];
	readbuf(buf, "copying setup section");
	unless (starts(buf, "%%BeginSetup") || starts(buf, "%%EndSetup")) fputs(buf, outfile);
	nb += strlen(buf);
      }
    fprintf(outfile, "%%%%EndSetup\n");
  }

static void selectpage(int pn)
  { fprintf(stderr, "[%d] ", pn+1);
    fprintf(outfile, "%%%%Page: pstog3 %d\n", pn+1);
    copypagebody(pn);
  }

static void copypagebody(int pn)
  { char buf[BUFSIZ], msg[32]; int plen;
    sprintf(msg, "seeking page %d", pn);
    fseek(infile, pageptr[pn], 0);
    readbuf(buf, msg);
    unless (starts(buf, "%%Page:")) ioerror(msg);
    plen = pageptr[pn+1] - pageptr[pn] - strlen(buf);
    sprintf(msg, "copying page %d", pn);
    fcopy(plen, msg);
  }

static void writetrailer()
  { int tlen = pageptr[numpages];
    fseek(infile, tlen, 0);
    fcopy(filelength - tlen, "copying trailer");
  }

static void readbuf(char *buf, char *msg)
  { if (fgets(buf, BUFSIZ, infile) == NULL) giveup("I/O error %s", msg);
  }

static void fcopy(int len, char *msg)
  { while (len > 0)
      { char buf[BUFSIZ];
	int n = (len > BUFSIZ) ? BUFSIZ : len;
	if (fread(buf, sizeof(char), n, infile) == 0 ||
	    fwrite(buf, sizeof(char), n, outfile) == 0) ioerror(msg);
	len -= n;
      }
  }

static void ioerror(char *msg)
  { giveup("I/O error %s", msg);
  }

static void giveup(char *msg, char *p1)
  { fprintf(stderr, "pstog3: "); fprintf(stderr, msg, p1); putc('\n', stderr);
    exit(1);
  }

