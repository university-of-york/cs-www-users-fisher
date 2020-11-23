/* Pre-processor (metarule expander) for GPSG parser-generator
   A.J. Fisher	 May 1986
   C version   AJF   August 1988 */

#include <stdio.h>
#include <setjmp.h>

#define global
#define forward
#define bool unsigned int
#define false ((bool) 0)
#define true ((bool) 1)
#define unless(x) if (!(x))
#define until(x) while (!(x))
#define heap(n,t) (t*) cheap((n)*sizeof(t))

#define MAXNAMELEN  80
#define NUMSYSWORDS 6
#define MAXMETALEN  500
#define MAXCONTEXT  10
#define ERRMSGFILE  "/usr/fisher/lib/g2psg.emf"

enum symbol
  { s_begin, s_bus, s_colon, s_comma, s_empty, s_end, s_eof, s_include,
    s_lbrac, s_name, s_punct, s_rbrac, s_semico, s_sub, s_with
  };

union item
  { char *sval;
    char cval;
  };

struct dictnode
  { struct dictnode *lft, *rgt;
    char *str;
  };

struct blob
  { struct blob *link;
    enum symbol bsy;
    union item bit;
    int bln, bns;
  };

struct blobvec
  { int n;
    struct blob *v[MAXMETALEN];
  };

struct syswent
  { union item item;
    enum symbol sym;
  };

static bool prerrors;
static FILE *input;
static char *filename, *outfn;
static int context, clnum, outln, ch, linenum, numspaces, syswtop;
static enum symbol symb;
static union item item;
static struct blob *blobstack;
static struct dictnode *dictionary;
static struct syswent syswords[NUMSYSWORDS];
static jmp_buf recover[MAXCONTEXT];

forward struct blob *makeblob(), *newblob(), *copyblob();
forward char *cheap();

extern char *malloc();


global main(argc,argv) int argc; char *argv[];
  { prerrors = false;
    openfiles(argv);
    dictionary = NULL; blobstack = NULL;
    clnum = context = 0;
    outfn = NULL; /* force pragmat on first symbol output */
    declsyswords();
    grammar();
    putchar('\n');
    fclose(input);
    exit(prerrors ? 2 : 0);
  }

static openfiles(argv) char *argv[];
  { int k = 0;
    if (argv[k] != NULL) k++; /* skip program name */
    filename = argv[k];
    if (filename == NULL)
      { fprintf(stderr, "Usage: metagee infn\n");
	exit(1);
      }
    input = fopen(filename,"r");
    if (input == NULL)
      { fprintf(stderr, "metagee: can't open %s\n", filename);
	exit(1);
      }
  }

static declsyswords()
  { syswtop = 0;
    dsw("BEGIN",s_begin);       dsw("EMPTY",s_empty);
    dsw("END",s_end);           dsw("INCLUDE",s_include);
    dsw("WITH",s_with);
  }

static dsw(s,sy) char *s; enum symbol sy;
  { lookupword(s);
    if (syswtop < NUMSYSWORDS)
      { syswords[syswtop].item = item;
	syswords[syswtop].sym = sy;
	syswtop++;
      }
    else prerror(206);
  }

static grammar()
  { clnum = 0; ch = '\n';
    rsymb();
    if (context < MAXCONTEXT)
      { _setjmp(recover[context++]); /* the "_" variety avoids the sigblock nonsense */
	rgram();
	context--;
      }
    else prerror(209); /* INLCUDEd files nested too deeply */
  }

static rgram()
  { until (symb == s_eof)
      { switch (symb)
	  { default:
		wsymb(); rsymb();
		break;

	    case s_bus:
	      { int ln = linenum, nsp = numspaces;
		rsymb();
		if (symb == s_sub)
		  { wsymbol(s_comma,0,ln,nsp);
		    rsymb();
		  }
		else wsymbol(s_bus,0,ln,nsp);
		break;
	      }

	    case s_include:
		include();
		break;

	    case s_begin:
		metarule();
		break;

	    case s_with:    case s_end:
		prerror(215);
		rsymb();
		break;

	    case s_empty:
		prerror(216);
		rsymb();
		break;

	    case s_lbrac:
	      { struct blobvec bv;
		int d = 0; bool e = false;
		bv.n = 0;
		rsymb();
		until (bv.n >= MAXMETALEN || symb == s_eof || (symb == s_rbrac && d == 0))
		  { if (symb == s_lbrac) d++;
		    if (symb == s_rbrac) d--;
		    if (d == 0 && symb == s_empty) e = true;
		    stuffblob(makeblob(), &bv);
		    rsymb();
		  }
		unless (symb == s_rbrac && d == 0) prerror(219);
		if (e) releasebv(&bv);
		else while (bv.n > 0) putbackblob(bv.v[--bv.n]);
		rsymb();
		break;
	      }
	  }
      }
  }

static include()
  { char v[MAXNAMELEN+1]; int nc = 0;
    while (ch == ' ' || ch == '\t' || ch == '{')
      if (ch == '{') skipcomment(); else rch();
    until (nc >= MAXNAMELEN || ch == ' ' || ch == '\t' || ch == '\n' || ch == '{' || ch == EOF)
      { v[nc++] = ch;
	rch();
      }
    v[nc++] = '\0';
    lookupword(v);
    doinclude(item.sval);
    rsymb();
  }

static doinclude(fn) char *fn;
  { FILE *in = input; char sch = ch; char *sfn = filename; int sln = clnum;
    input = fopen(fn,"r");
    if (input != NULL)
      { filename = fn;
	grammar();
	fclose(input);
      }
    else prerror(203,fn);
    input = in; ch = sch; filename = sfn; clnum = sln;
  }

static metarule()
  { struct blobvec pat; /* pattern vector (BEGIN ... WITH)   */
    struct blobvec fpv; /* formal parameter vector (the "W") */
    struct blobvec apv; /* actual vector (WITH ... END)	     */
    collectpattern(&pat);
    checkfor(s_with,211);
    collectformals(&fpv);
    checkfor(s_colon,205);
    collectactuals(&apv,fpv.n);
    substitute(&pat,&fpv,&apv);
    releasebv(&pat); releasebv(&fpv); releasebv(&apv);
    rsymb();
  }

static releasebv(bv) struct blobvec *bv;
  { struct blob **v = bv -> v; int *p = &bv -> n;
    while (*p > 0) free(v[--(*p)]);
  }

static collectpattern(pat) struct blobvec *pat;
  { /* collect the pattern */
    int d = 0;
    pat -> n = 0;
    rsymb();
    until (pat -> n >= MAXMETALEN || (symb == s_with && d == 0) || symb == s_eof)
      { if (symb == s_begin) d++;
	if (symb == s_end) d--;
	stuffblob(makeblob(), pat);
	rsymb();
      }
  }

static collectformals(fpv) struct blobvec *fpv;
  { /* collect the formal parameter list */
    fpv -> n = 0;
    stuffformal(fpv);
    while (fpv -> n < MAXMETALEN && symb == s_comma)
      { rsymb();
	stuffformal(fpv);
      }
    if (symb == s_comma) prerror(213);
  }

static stuffformal(fpv) struct blobvec *fpv;
  { struct blob *x = makeblob();
    checkfor(s_name,208);
    stuffblob(x,fpv);
  }

static collectactuals(apv,nfp) struct blobvec *apv; int nfp;
  { /* collect the actual parameter list */
    apv -> n = 0;
    colactual(apv,nfp);
    while (symb == s_semico)
      { stuffactual(apv);
	colactual(apv,nfp);
      }
    unless (symb == s_end) prerror(212);
  }

static colactual(apv,nfp) struct blobvec *apv; int nfp;
  { int n = 0;
    until (apv -> n >= MAXMETALEN || symb == s_semico || symb == s_end || symb == s_eof)
      { int d = 0;
	until (apv -> n >= MAXMETALEN || (symb == s_comma && d == 0) || symb == s_semico || symb == s_end || symb == s_eof)
	  { if (symb == s_sub) d++;
	    if (symb == s_bus) d--;
	    stuffactual(apv);
	  }
	unless (apv -> n >= MAXMETALEN || symb == s_semico || symb == s_end || symb == s_eof) stuffactual(apv);
	n++;
      }
    unless (n == nfp) prerror(217,nfp,n); /* wrong num. of parameters */
  }

static stuffactual(apv) struct blobvec *apv;
  { stuffblob(makeblob(), apv);
    rsymb();
  }

static substitute(pat,fpv,apv) struct blobvec *pat, *fpv, *apv;
  { /* substitute and push back */
    int p2 = apv -> n - 1;
    until (p2 < 0) /* for each actual */
      { int p1 = p2; int j;
	until (p1 < 0 || apv -> v[p1] -> bsy == s_semico) p1--;
	for (j = pat -> n - 1; j >= 0; j--) /* for each blob in pattern */
	  { struct blob *x = pat -> v[j];
	    wrsubst(x,fpv,apv,p1+1,p2);
	  }
	p2 = p1-1;
      }
  }

static wrsubst(x,fpv,apv,p1,p2) struct blob *x; struct blobvec *fpv, *apv; int p1, p2;
  { /* write symbol x or its substitute (if it's a formal parameter) */
    /* p1,p2 are ptrs to first & last blobs in actual list	     */
    if (x -> bsy == s_name)
      { int k = 0, d = 0;
	int q1 = p1, q2 = p1;
	while (q2 <= p2 && !(apv -> v[q2] -> bsy == s_comma && d == 0))
	  { if (apv -> v[q2] -> bsy == s_sub) d++;
	    if (apv -> v[q2] -> bsy == s_bus) d--;
	    q2++;
	  }
	/* q1,q2 now delimit the actual parameter */
	while (k < fpv -> n && x -> bit.sval != fpv -> v[k] -> bit.sval)
	  { k++; q1 = ++q2; /* point past the comma */
	    while (q2 <= p2 && !(apv -> v[q2] -> bsy == s_comma && d == 0))
	      { if (apv -> v[q2] -> bsy == s_sub) d++;
		if (apv -> v[q2] -> bsy == s_bus) d--;
		q2++;
	      }
	  }
	if (k < fpv -> n)
	  { /* output actual sequence */
	    int i;
	    for (i = q2-1; i >= q1; i--)
	      { struct blob *ap = copyblob(apv -> v[i]);
		/* keep line number & spaces before of occurrence in pattern */
		ap -> bln = x -> bln;
		if (i == q1) ap -> bns = x -> bns;
		putbackblob(ap);
	      }
	  }
	else putbackblob(copyblob(x)); /* it's not a formal parameter */
      }
    else putbackblob(copyblob(x)); /* it's not a name */
  }

static checkfor(sy,en) enum symbol sy; int en;
  { if (symb != sy)
      { prerror(en);
	unless (symb == s_eof) rsymb();
	if (context > 0) _longjmp(recover[context-1]);
	prerror(210); /* bug! */
      }
    rsymb();
  }

static struct blob *makeblob()
  { return newblob(symb,item,linenum,numspaces); }

static struct blob *newblob(sy,it,ln,ns) enum symbol sy; union item it; int ln, ns;
  { struct blob *x = heap(1, struct blob);
    x -> bsy = sy; x -> bit = it; x -> bln = ln; x -> bns = ns;
    return x;
  }

static struct blob *copyblob(x1) struct blob *x1;
  { struct blob *x2 = heap (1, struct blob);
    *x2 = *x1; /* struct assignment */
    return x2;
  }

static stuffblob(x,bv) struct blob *x; struct blobvec *bv;
  { struct blob **v = bv -> v; int *p = &bv -> n;
    v[(*p)++] = x;
  }

static putbackblob(x) struct blob *x;
  { x -> link = blobstack;
    blobstack = x;
  }

static rsymb()
  { if (blobstack != NULL)
      { struct blob *x = blobstack;
	blobstack = x -> link;
	symb = x -> bsy; item = x -> bit; linenum = x -> bln; numspaces = x -> bns;
	free(x);
      }
    else
      { numspaces = 0;
    l:	linenum = clnum;
	switch (ch)
	  { default:
		symb = s_punct;
		item.cval = ch;
		rch();
		break;

	    case ' ':	case '\t':
		numspaces++;
		rch();
		goto l;

	    case '\n':
		numspaces = 0;
		rch();
		goto l;

	    case '{':
		skipcomment();
		goto l;

	    case EOF:
		symb = s_eof;
		break;

	    case '[':
		symb = s_sub;
		rch();
		break;

	    case ']':
		symb = s_bus;
		rch();
		break;

	    case '<':
		rch();
		if (ch == '<')
		  { symb = s_lbrac;
		    rch();
		  }
		else
		  { symb = s_punct;
		    item.cval = '<';
		  }
		break;

	    case '>':
		rch();
		if (ch == '>')
		  { symb = s_rbrac;
		    rch();
		  }
		else
		  { symb = s_punct;
		    item.cval = '>';
		  }
		break;

	    case ',':
		symb = s_comma;
		rch();
		break;

	    case ':':
		symb = s_colon;
		rch();
		break;

	    case ';':
		symb = s_semico;
		rch();
		break;

	    case '\'':	case '"':
		terminal();
		break;

	    case 'a':	case 'b':   case 'c':	case 'd':   case 'e':	case 'f':
	    case 'g':	case 'h':   case 'i':	case 'j':   case 'k':	case 'l':
	    case 'm':	case 'n':   case 'o':	case 'p':   case 'q':	case 'r':
	    case 's':	case 't':   case 'u':	case 'v':   case 'w':	case 'x':
	    case 'y':	case 'z':
	    case 'A':	case 'B':   case 'C':	case 'D':   case 'E':	case 'F':
	    case 'G':	case 'H':   case 'I':	case 'J':   case 'K':	case 'L':
	    case 'M':	case 'N':   case 'O':	case 'P':   case 'Q':	case 'R':
	    case 'S':	case 'T':   case 'U':	case 'V':   case 'W':	case 'X':
	    case 'Y':	case 'Z':
	    case '0':	case '1':   case '2':	case '3':   case '4':	case '5':
	    case '6':	case '7':   case '8':	case '9':   case '+':	case '-':
		name();
		break;
	  }
      }
  }

static skipcomment()
  { int d = 0;
    rch();
    until ((ch == '}' && d == 0) || ch == EOF)
      { if (ch == '{') d++;
	if (ch == '}') d--;
	rch();
      }
    if (ch == '}') rch(); else prerror(202);
  }

static terminal()
  { char v[MAXNAMELEN+1]; int nc = 0;
    int qu = ch;
    v[nc++] = qu;
    rch();
    while (nc < MAXNAMELEN-1 && !(ch == qu || ch == '\n' || ch == EOF))
      { v[nc++] = ch;
	rch();
      }
    v[nc++] = qu; v[nc++] = '\0';
    unless (ch == qu) prerror(204);
    unless (ch == EOF) rch();
    lookupword(v);
    symb = s_name;
  }

#define namechar(c) \
    ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9'))

static name()
  { char v[MAXNAMELEN+1]; int nc = 0;
    if (ch == '+' || ch == '-')
      { v[nc++] = ch;
	rch();
      }
    else
      { while (nc < MAXNAMELEN && namechar(ch))
	  { v[nc++] = ch;
	    rch();
	  }
	if (namechar(ch)) prerror(201);
      }
    v[nc++] = '\0';
    lookupword(v);
    lookupsysword();
  }

static lookupword(v) char *v;
  { /* derives a canonical version of a string */
    struct dictnode **m = &dictionary;
    bool found = false;
    until (found || (*m == NULL))
      { int k = strcmp(v, (*m) -> str);
	if (k < 0) m = &(*m) -> lft;
	if (k > 0) m = &(*m) -> rgt;
	if (k == 0) found = true;
      }
    unless (found)
      { struct dictnode *t = heap(1, struct dictnode);
	t -> lft = t -> rgt = NULL;
	t -> str = heap(strlen(v)+1, char);
	strcpy(t -> str, v);
	*m = t;
      }
    item.sval = (*m) -> str;
  }

static char *cheap (n) int n;
  { /* called by "heap" macro to allocate n bytes */
    char *s = malloc(n);
    if (s == NULL)
      { fprintf(stderr, "metagee: no room\n");
	exit(1);
      }
    return s;
  }

static lookupsysword()
  { int k = 0;
    while (k < syswtop && syswords[k].item.sval != item.sval) k++;
    symb = (k < syswtop) ? syswords[k].sym : s_name;
  }

static rch()
  { if (ch == '\n') clnum++;
    ch = getc(input);
  }

static wsymb() { wsymbol(symb,item,linenum,numspaces); }

static wsymbol(sy,it,ln,ns) enum symbol sy; union item it; int ln, ns;
  { int i;
    if (filename != outfn || ln < outln || ln > outln+2)
      { printf(" {$F %s %d }", filename, ln-1);
	putchar('\n');
	outfn = filename; outln = ln;
      }
    else
      { while (outln < ln)
	  { putchar('\n');
	    outln++;
	  }
      }
    for (i=0; i<ns; i++) putchar(' ');
    switch (sy)
      { default:
	    prerror(207,sy);
	    break;

	case s_punct:
	    putchar(it.cval);
	    break;

	case s_sub:
	    putchar('[');
	    break;

	case s_bus:
	    putchar(']');
	    break;

	case s_comma:
	    putchar(',');
	    break;

	case s_colon:
	    putchar(':');
	    break;

	case s_semico:
	    putchar(';');
	    break;

	case s_name:
	    fputs(it.sval,stdout);
	    break;
      }
  }

static prerror(n,p1,p2) int n; char *p1, *p2;
  { char v[81];
    getmessage(n,v);
    fprintf(stderr, "*** ");
    writevec(v,0,11);
    fprintf(stderr, "line %d file %s ", clnum, filename);
    writevec(v,12,strlen(v)-1,p1,p2); putc('\n',stderr);
    fputs(" {$E }", stdout);
    prerrors = true;
  }

static getmessage(n,v) int n; char *v;
  { int nc = 0; int c;
    FILE *f = fopen(ERRMSGFILE,"r");
    if (f == NULL)
      { fprintf(stderr, "metagee: can't open error message file %s\n", ERRMSGFILE);
	exit(1);
      }
    fseek(f,(n-100)*80,0);
    c = getc(f);
    until (nc >= 80 || c == '\n' || c == EOF)
      { v[nc++] = c;
	c = getc(f);
      }
    v[nc++] = '\0';
    fclose(f);
  }

static writevec(v,k1,k2,p1,p2) char *v; int k1, k2; char *p1, *p2;
  { char s[81]; int i;
    for (i=k1; i <= k2; i++) s[i-k1] = v[i];
    s[k2-k1+1] = '\0';
    fprintf(stderr,s,p1,p2);
  }
