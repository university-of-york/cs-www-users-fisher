/* pstog3 - convert multi-page PostScript (on stdin) to Group 3 fax format (on stdout) - AJF March 1995
   Based on: pstops - rearrange pages in conforming PS file for printing in signatures - AJF January 1994
   Based on: pstops by AJCD 27/1/91
*/

#include <stdio.h>
#include <fcntl.h>

#define global
#define forward
#define uint		unsigned int
#define uchar		unsigned char
#define bool		int
#define false		0
#define true		1
#define unless(x)	if(!(x))
#define until(x)	while(!(x))
#define seq(s1,s2)	(strcmp(s1,s2) == 0)
#define starts(s1,s2)	(strncmp(s1,s2,strlen(s2)) == 0)

#define VERSION	    1
#define TMPDIR	    "/tmp"
#define MAXPAGES    5000	/* max pages in document */
#define PBMBIN	    "/york/bin/pbm"

#ifdef mips
#define MYBIN	    "/usr/fisher/mipsbin"
#endif

#ifdef sun
#define MYBIN	    "/usr/fisher/sunbin"
#endif

#define opt_v	1   /* verbose	      */
#define opt_l	2   /* low resolution */
#define opt_n	4   /* no header      */

static FILE *infile, *outfile;
static uchar cloptions;
static int numpages;
static int headerlen, prologlen, filelength;
static int pageptr[MAXPAGES+1]; /* 1 extra slot for trailer pointer */

forward FILE *seekable();

extern char *getenv();


global main(argc, argv) int argc; char *argv[];
  { int pn;
    parseargs(argv);
    scanpages();
    unless (cloptions & opt_n)
      { printf("!<Group3> %d 200 %d\n", VERSION, (cloptions & opt_l) ? 100 : 200); /* pretend horiz resolution is 200 dpi */
	fflush(stdout);
      }
    for (pn=0; pn < numpages; pn++)
      { char cmd[256];
	/* horiz dpi "207.7" is chosen to give 1728 dots per scan line, req'd by fax standard */
	sprintf(cmd, "%s/pstobm  -ppm -dpi 207.7 %d - - | %s/pbmtog3", MYBIN, (cloptions & opt_l) ? 100 : 200, PBMBIN);
	outfile = popen(cmd, "w");
	if (outfile == NULL) giveup("can't create pipe to pstobm | pstog3");
	writeheader();
	writeprolog();
	writesetup();
	selectpage(pn);
	writetrailer();
	if (ferror(outfile)) giveup("output file error");
	pclose(outfile);
      }
    if (cloptions & opt_v) fprintf(stderr, "Wrote %d pages\n", numpages);
    exit(0);
  }

static parseargs(argv) char *argv[];
  { int ap = 0;
    cloptions = 0;
    unless (argv[ap] == NULL) ap++;
    until (argv[ap] == NULL)
      { char *s = argv[ap++];
	int k = 0;
	unless (s[k++] == '-') usage();
	until (s[k] == '\0')
	  { char ch = s[k++];
	    if (ch == 'l') cloptions |= opt_l;
	    else if (ch == 'v') cloptions |= opt_v;
	    else if (ch == 'n') cloptions |= opt_n;
	    else usage();
	  }
      }
    infile = seekable(stdin);
    if (infile == NULL) giveup("can't seek input");
  }

static usage()
  { fprintf(stderr, "Usage: pstog3 [-[lvn]]\n");
    exit(2);
  }

static FILE *seekable(fp) FILE *fp;
  { /* make a file seekable; trick stolen from Chris Torek's libdvi */
    int fd, tf, n, w; char *tmpdir, *p;
    char buf[BUFSIZ];
    fd = fileno(fp);
    if (lseek(fd, 0L, 1) >= 0 && !isatty(fd)) return fp;
    tmpdir = getenv("TMPDIR");
    if (tmpdir == NULL) tmpdir = TMPDIR;
    sprintf(buf, "%s/#%d", tmpdir, getpid());
    tf = open(buf, O_RDWR | O_CREAT | O_EXCL, 0666);
    if (tf < 0) return NULL;
    unlink(buf);
    while ((n = read(fd, p = buf, BUFSIZ)) > 0)
      { do
	  { if ((w = write(tf, p, n)) < 0)
	      { close(tf); fclose(fp);
		return NULL;
	      }
	    p += w; n -= w;
	  }
	while (n > 0);
      }
    if (n < 0)
      { close(tf); fclose(fp);
	return NULL;
      }
    /* discard the input file, and rewind and open the temporary */
    fclose(fp); lseek(tf, 0L, 0);
    fp = fdopen(tf, "r");
    if (fp == NULL) close(tf);
    return fp;
  }

static scanpages()
  { /* build array of pointers to start/end of pages */
    char buf[BUFSIZ];
    int nesting = 0;
    bool gottr = false;
    numpages = headerlen = prologlen = 0;
    fseek(infile, 0L, 0);
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
    pageptr[numpages] = ftell(infile);	  /* pointer to trailer */
    fseek(infile, 0L, 2);		    /* seek to end of file */
    filelength = ftell(infile);		    /* size of file */
  }

static writeheader()
  { /* write header, altering "%%Pages:" comment */
    int nb = 0;
    fseek(infile, 0L, 0);
    while (nb < headerlen)
      { char buf[BUFSIZ];
	readbuf(buf, "copying header");
	unless (starts(buf, "%%Pages:") || starts(buf, "%%EndComments")) fputs(buf, outfile);
	nb += strlen(buf);
      }
    fprintf(outfile, "%%%%Pages: 1\n");
    fprintf(outfile, "%%%%EndComments\n");
  }

static writeprolog()
  { /* write prologue (between end of header and %%EndProlog) */
    fseek(infile, headerlen, 0);
    fcopy(prologlen - headerlen, "copying prologue");
  }

static writesetup()
  { /* write setup section (between end of prologue and start of first page) */
    int nb = prologlen;
    fseek(infile, prologlen, 0);
    fprintf(outfile, "%%%%BeginSetup\n");
    while (nb < pageptr[0])
      { char buf[BUFSIZ];
	readbuf(buf, "copying setup section");
	unless (starts(buf, "%%BeginSetup") || starts(buf, "%%EndSetup")) fputs(buf, outfile);
	nb += strlen(buf);
      }
    fprintf(outfile, "%%%%EndSetup\n");
  }

static selectpage(pn) int pn;
  { if (cloptions & opt_v) fprintf(stderr, "[%d] ", pn);
    fprintf(outfile, "%%%%Page: pstog3 %d\n", pn+1);
    copypagebody(pn);
  }

static copypagebody(pn) int pn;
  { char buf[BUFSIZ], msg[32]; int plen;
    sprintf(msg, "seeking page %d", pn);
    fseek(infile, pageptr[pn], 0);
    readbuf(buf, msg);
    unless (starts(buf, "%%Page:")) ioerror(msg);
    plen = pageptr[pn+1] - pageptr[pn] - strlen(buf);
    sprintf(msg, "copying page %d", pn);
    fcopy(plen, msg);
  }

static writetrailer()
  { int tlen = pageptr[numpages];
    fseek(infile, tlen, 0);
    fcopy(filelength - tlen, "copying trailer");
  }

static readbuf(buf, msg) char *buf, *msg;
  { if (fgets(buf, BUFSIZ, infile) == NULL) giveup("I/O error %s", msg);
  }

static fcopy(len, msg) int len; char *msg;
  { while (len > 0)
      { char buf[BUFSIZ];
	int n = (len > BUFSIZ) ? BUFSIZ : len;
	if (fread(buf, sizeof(char), n, infile) == NULL ||
	    fwrite(buf, sizeof(char), n, outfile) == NULL) ioerror(msg);
	len -= n;
      }
  }

static ioerror(msg) char *msg;
  { giveup("I/O error %s", msg);
  }

static giveup(msg, p1) char *msg, *p1;
  { fprintf(stderr, "pstog3: "); fprintf(stderr, msg, p1); putc('\n', stderr);
    exit(1);
  }

