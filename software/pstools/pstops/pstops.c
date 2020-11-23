/* pstops - rearrange pages in conforming PS file for printing in signatures
   AJF	 January 1994	based on pstops by AJCD 27/1/91
   Usage:   pstops [-q] [-w<dim>] [-h<dim>] <pagespecs> [infile [outfile]]
*/

#include <stdio.h>
#include <fcntl.h>

#define global
#define forward
#define uint		unsigned int
#define bool		int
#define false		0
#define true		1
#define unless(x)	if(!(x))
#define until(x)	while(!(x))
#define heap(n,t)	(t*) cheap((n) * sizeof(t))
#define isdigit(c)	((c) >= '0' && (c) <= '9')
#define starts(s1,s2)	(strncmp(s1,s2,strlen(s2)) == 0)

#define RELEASE	    2
#define PATCHLEVEL  2

#define TMPDIR	    "/tmp"
#define MAXPAGES    5000			/* max pages in document */
#define PTSPERIN    72.0			/* points per inch	 */
#define PTSPERCM    28.346456692913385211	/* points per centimeter */

/* pagespec flags */
#define ADD_NEXT    0x01
#define ROTATE	    0x02
#define SCALE	    0x04
#define OFFSET	    0x08
#define REVERSED    0x10

struct pagespec
  { uint flags;
    int pageno, rotate;
    double xoff, yoff, scale;
    struct pagespec *next;
  };

static FILE *infile, *outfile;
static bool verbose;
static int passes, modulo, pagesperspec, numinpages, numoutpages, maxpage;
static int headerlen, prologlen, filelength;
static int pageptr[MAXPAGES+1]; /* 1 extra slot for trailer pointer */
static double width, height;
static struct pagespec *specs;
static char *specstring;

forward FILE *openfile(), *seekable();
forward struct pagespec *parsespecs(), *newspec();
forward char *cheap();
forward double singledimen(), parsedimen(), parsedouble();

extern char *malloc(), *getenv();
extern double atof();
extern int atoi();


global main(argc, argv) int argc; char *argv[];
  { parseargs(argc, argv);
    scanpages();
    writeheader();
    writeprolog();
    writesetup();
    selectpages();
    writetrailer();
    checkfile();
    exit(0);
  }

static parseargs(argc, argv) int argc; char *argv[];
  { int k;
    specs = NULL;
    infile = stdin;
    outfile = stdout;
    verbose = true;
    width = height = -1.0;
    for (k=1; k < argc; k++) prsarg(argv[k]);
    if (specs == NULL) usage();
    infile = seekable(infile);
    if (infile == NULL)
      { fprintf(stderr, "pstops: can't seek input\n");
	exit(1);
      }
  }

static prsarg(arg) char *arg;
  { if (arg[0] == '-' && !isdigit(arg[1]))
      { switch (arg[1])
	  { default:
		usage();

	    case 'q':
		verbose = false;
		break;

	    case 'w':
		width = singledimen(&arg[2]);
		break;

	    case 'h':
		height = singledimen(&arg[2]);
		break;
	  }
      }
    else if (specs == NULL) specs = parsespecs(arg);
    else if (infile == stdin) infile = openfile(arg, "r", "input");
    else if (outfile == stdout) outfile = openfile(arg, "w", "output");
    else usage();
  }

static FILE *openfile(fn, rw, io) char *fn, *rw, *io;
  { FILE *fi = fopen(fn, rw);
    if (fi == NULL)
      { fprintf(stderr, "pstops: can't open %s file %s\n", io, fn);
	exit(1);
      }
    return fi;
  }

static struct pagespec *parsespecs(str) char *str;
  { struct pagespec *head, *tail;
    bool other = false;
    int num = -1;
    head = newspec(); tail = head;
    modulo = pagesperspec = 1;
    specstring = str;
    until (*str == '\0')
      { if (*str == ' ' || *str == '\t' || *str == '\n') str++;
	else if (isdigit(*str))
	  { num = 0;
	    while (isdigit(*str)) num = (10*num) + (*str++ - '0');
	  }
	else
	  { switch (*str)
	      { default:
		    specusage(str);

		case ':':
		    if (other || head != tail || num < 1) specusage(str);
		    modulo = num;
		    num = -1;
		    str++;
		    break;

		case '-':
		    tail -> flags ^= REVERSED; /* flip it */
		    str++;
		    break;

		case '@':
		    if (num < 0) specusage(str);
		    str++;
		    tail -> scale *= parsedouble(&str);
		    tail -> flags |= SCALE;
		    break;

		case 'l': case 'L':
		    tail -> rotate++;
		    tail -> flags |= ROTATE;
		    str++;
		    break;

		case 'r': case 'R':
		    tail -> rotate--;
		    tail -> flags |= ROTATE;
		    str++;
		    break;

		case 'u': case 'U':
		    tail -> rotate += 2;
		    tail -> flags |= ROTATE;
		    str++;
		    break;

		case '(':
		    str++;
		    tail -> xoff += parsedimen(&str);
		    unless (*str == ',') specusage(str);
		    str++;
		    tail -> yoff += parsedimen(&str);
		    unless (*str == ')') specusage(str);
		    str++;
		    tail -> flags |= OFFSET;
		    break;

		case '+':
		    tail -> flags |= ADD_NEXT;
		    /* drop through */

		case ',':
		    if (num < 0 || num >= modulo) specusage(str);
		    unless (tail -> flags & ADD_NEXT) pagesperspec++;
		    tail -> pageno = num;
		    tail -> next = newspec();
		    tail = tail -> next;
		    num = -1;
		    str++;
		    break;
	      }
	    other = true;
	  }
      }
    if (num >= modulo) specusage(str);
    if (num >= 0) tail -> pageno = num;
    return head;
  }

struct pagespec *newspec()
  { struct pagespec *temp = heap(1, struct pagespec);
    temp -> pageno = temp -> flags = temp -> rotate = 0;
    temp -> scale = 1.0;
    temp -> xoff = temp -> yoff = 0.0;
    temp -> next = NULL;
    return temp;
  }

static char *cheap(nb) int nb;
  { /* called by "heap" macro */
    char *x = malloc(nb);
    if (x == NULL)
      { fprintf(stderr, "pstops: no room!\n");
	exit(1);
      }
    return x;
  }

static double singledimen(str) char *str;
  { double num = parsedimen(&str);
    unless (str[0] == '\0') usage();
    return num;
  }

static double parsedimen(sp) char **sp;
  { double num = parsedouble(sp);
    char *s = *sp;
    if (starts(s, "pt")) s += 2;
    else if (starts(s, "in")) { num *= PTSPERIN; s += 2; }
    else if (starts(s, "cm")) { num *= PTSPERCM; s += 2; }
    else if (starts(s, "w") || starts(s, "h"))
      { double *xx = (s[0] == 'w') ? &width : &height;
	if (*xx < 0.0)
	  { fprintf(stderr, "pstops: w/h dimension requires width/height resp.\n");
	    exit(1);
	  }
	num *= *xx; s++;
      }
    *sp = s;
    return num;
  }

static double parsedouble(sp) char **sp;
  { char *s = *sp; int k = 0; char buf[32];
    if (s[k] == '+' || s[k] == '-') k++;
    while (k<32 && isdigit(s[k])) k++;
    if (k<32 && s[k] == '.') k++;
    while (k<32 && isdigit(s[k])) k++;
    if (k == 0 || k >= 32) specusage(s);
    memcpy(buf, s, k); buf[k] = ':';
    *sp = &s[k];
    return atof(buf);
  }

static usage()
  { fprintf(stderr, "pstops release %d patchlevel %d\n", RELEASE, PATCHLEVEL);
    fprintf(stderr, "Usage: pstops [-q] [-w<dim>] [-h<dim>] <pagespecs> [infile [outfile]]\n");
    fprintf(stderr, "  <pagespecs> = [modulo:]<spec>\n");
    fprintf(stderr, "  <spec>      = [-]pageno[@scale][L|R|U][(xoff,yoff)][,spec|+spec]\n");
    fprintf(stderr, "                modulo>=1, 0<=pageno<modulo\n");
    exit(1);
  }

static specusage(s) char *s;
  { fprintf(stderr, "pstops: page specification error:  (type `pstops' for usage)\n");
    fprintf(stderr, "%s\n", specstring);
    fprintf(stderr, "%*c\n", s-specstring+1, '^');
    exit(1);
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
    numinpages = headerlen = prologlen = 0;
    passes = 1;
    fseek(infile, 0L, 0);
    while (!gottr && fgets(buf, BUFSIZ, infile) != NULL)
      { if (buf[0] == '%' && buf[1] == '%')
	  { char *comment = &buf[2];
	    if (nesting == 0 && starts(comment, "Page:"))
	      { if (numinpages >= MAXPAGES)
		  { fprintf(stderr, "pstops: too many pages in input file!\n");
		    exit(1);
		  }
		pageptr[numinpages++] = ftell(infile) - strlen(buf);
	      }
	    else if (headerlen == 0 && starts(comment, "EndComments")) headerlen = ftell(infile);
	    else if (headerlen == 0 && starts(comment, "PstopsPasses:"))
	      { int np = atoi(&comment[13]);
		if (np > 0) passes = np+1;
		else fprintf(stderr, "pstops: warning; dubious comment: %s\n", buf);
	      }
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
    maxpage = ((numinpages+modulo-1)/modulo)*modulo;
    pageptr[numinpages] = ftell(infile);    /* pointer to trailer */
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
	unless (starts(buf, "%%Pages:") || starts(buf, "%%PstopsPasses:") || starts(buf, "%%EndComments"))
	  fputs(buf, outfile);
	nb += strlen(buf);
      }
    fprintf(outfile, "%%%%Pages: %d\n", (maxpage/modulo) * pagesperspec);
    fprintf(outfile, "%%%%PstopsPasses: %d\n", passes);
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
    if (passes == 1)
      { fprintf(outfile, "/pstops_init matrix currentmatrix def\n");
	fprintf(outfile, "/pstops_fwd matrix def\n");
	fprintf(outfile, "/pstops_bwd matrix def\n");
      }
    while (nb < pageptr[0])
      { char buf[BUFSIZ];
	readbuf(buf, "copying setup section");
	unless (starts(buf, "%%BeginSetup") || starts(buf, "%%EndSetup"))
	  fputs(buf, outfile);
	nb += strlen(buf);
      }
    if (passes == 1)
      { /* define pstops_fwd as the matrix which gets us from "pre-setup" (default) space
	   to "post-setup" (page) space */
	fprintf(outfile, "/pstops_fwd matrix currentmatrix ");
	fprintf(outfile, "pstops_init matrix invertmatrix matrix concatmatrix store\n");
	/* define pstops_bwd as the inverse of pstops_fwd */
	fprintf(outfile, "/pstops_bwd pstops_fwd matrix invertmatrix store\n");
      }
    fprintf(outfile, "%%%%EndSetup\n");
  }

static selectpages()
  { int thispg;
    numoutpages = 0;
    for (thispg = 0; thispg < maxpage; thispg += modulo)
      { bool add_last = false;
	struct pagespec *ps;
	for (ps = specs; ps != NULL; ps = ps -> next)
	  { bool add_next = ps -> flags & ADD_NEXT;
	    bool tform = (ps -> flags & (OFFSET+ROTATE+SCALE));
	    int actualpg = ps -> pageno + ((ps -> flags & REVERSED) ? maxpage-modulo-thispg : thispg);
	    unless (add_last)
	      { numoutpages++;
		if (verbose) fprintf(stderr, "[%d] ", numoutpages);
		fprintf(outfile, "%%%%Page: pstops %d\n", numoutpages);
	      }
	    fprintf(outfile, "/pstopssaved save def\n");
	    if (tform) fprintf(outfile, "pstops_bwd concat\n"); /* transform back to dflt coord system */
	    if (ps -> flags & OFFSET) fprintf(outfile, "%lf %lf translate\n", ps -> xoff, ps -> yoff);
	    if (ps -> flags & ROTATE) fprintf(outfile, "%d rotate\n", (ps -> rotate & 3) * 90);
	    if (ps -> flags & SCALE) fprintf(outfile, "%lf %lf scale\n", ps -> scale, ps -> scale);
	    if (tform) fprintf(outfile, "pstops_fwd concat\n");
	    if (add_next) fprintf(outfile, "/showpage {} def /copypage {} def /erasepage {} def\n");
	    if (actualpg < numinpages) copypagebody(actualpg);
	    else fprintf(outfile, "showpage\n");
	    fprintf(outfile, "pstopssaved restore\n");
	    add_last = add_next;
	  }
      }
  }

static copypagebody(p) int p;
  { char buf[BUFSIZ], msg[32]; int plen;
    sprintf(msg, "seeking page %d", p);
    fseek(infile, pageptr[p], 0);
    readbuf(buf, msg);
    unless (starts(buf, "%%Page:")) ioerror(msg);
    plen = pageptr[p+1] - pageptr[p] - strlen(buf);
    sprintf(msg, "copying page %d/%d", p, numoutpages);
    fcopy(plen, msg);
  }

static writetrailer()
  { int tlen = pageptr[numinpages];
    fseek(infile, tlen, 0);
    fcopy(filelength - tlen, "copying trailer");
    if (verbose) fprintf(stderr, "Wrote %d pages\n", numoutpages);
  }

static readbuf(buf, msg) char *buf, *msg;
  { if (fgets(buf, BUFSIZ, infile) == NULL)
      { fprintf(stderr, "pstops: I/O error %s\n", msg);
	exit(1);
      }
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
  { fprintf(stderr, "pstops: I/O error %s\n", msg);
    exit(1);
  }

static checkfile()
  { if (ferror(outfile))
      { fprintf(stderr, "pstops: output file error\n");
	exit(1);
      }
  }

