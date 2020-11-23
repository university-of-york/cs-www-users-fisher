/* Parser for Generalized Generalized Phrase Structure Grammars (G2PSGs)
   A.J. Fisher	 September 1990 */

typedef long time_t;

#include <stdio.h>
#include <sys/times.h>

#define global
#define forward
#define uint unsigned int
#define bool unsigned int
#define false ((bool) 0)
#define true ((bool) 1)
#define unless(x) if (!(x))
#define until(x) while (!(x))
#define heap(t) (t*) cheap(sizeof(t))
#define seq(s1,s2) (strcmp(s1,s2) == 0)

#define FMASKSIZE  16	  /* must agree with value in PG (g2psg.h) */
#define TMASKSIZE  15	  /* must agree with value in PG (g2psg.h) */

#define TOPBIT	   (1 << 31)
#define MAXINT	   (~TOPBIT)
#define MAXFSPECS  FMASKSIZE*32
#define STOPPER	   (TMASKSIZE*32 - 1)
#define MAXLINE	   256
#define HASHSIZE   509	  /* should be a prime number */
#define MAXSENTLEN 50
#define NUMSEGS	   400	  /* num. of segments in heap */
#define SEGSIZE	   10000  /* size of a segment in heap */

struct fmask { uint m[FMASKSIZE]; };
struct tmask { uint m[TMASKSIZE]; };

struct fpair
  { struct fmask set;  /* bits set for features */
    struct fmask cpl;  /* complement bits	*/
  };

struct clause
  { uint ord;			/* propagation type  */
    struct fmask pos, neg;	/* features to check */
    struct fpair add;		/* features to add   */
  };

/* parser table */
extern int Prods[];
extern char Terms[], Nnames[], Fnames[];
extern struct clause Clauses[];

struct state
  { struct state *sflk, *shlk;
    int pval, jval, bval;
    struct fpair fval;
    struct statelist *treep, *bpslot;
    struct statelist **backp; /* initially points to bpslot */
    uint mark;
  };

struct stateset
  { struct state *sshd;
    struct state **sstl;
    struct state *hash[HASHSIZE];
  };

struct statelist
  { struct statelist *link;
    struct state *sta, *stb;
  };

enum symbol { s_leaf, s_cycle, s_down, s_across, s_or }; /* ftree symbols	  */

union treeu
  { struct tree *ft;
    struct state *st;
  };

struct tree
  { enum symbol h1;
    union treeu h2, h3;
  };

/* mark bits */
#define m_frec	 001
#define m_blfcr	 002
#define m_blgrp	 004
#define m_comp	 010

/* tokflags bits */
#define tf_prlegal 1
#define tf_unknown 2

/* symbols in production record */
#define x_nonterm  0
#define x_term	   1
#define x_gap	   2
#define x_end	   3

/* propagation types */
#define o_tric	   1
#define o_perc	   2
#define o_end	   4

/* offsets in production record (used by parser, assumed by maketab) */
#define plnk 0
#define plhs 1
#define plhf 2
#define psta (2+2*FMASKSIZE)
#define prhs (2+2*FMASKSIZE+TMASKSIZE)

/* offsets from prhs */
#define rsym 0
#define rval 1
#define rrhf 2
#define rsiz (2+2*FMASKSIZE)

static FILE *input, *output;
static char *segbase[NUMSEGS];
static int segptr[NUMSEGS];
static char cmdline[MAXLINE+1];
static int sentence[MAXSENTLEN+1];
static struct stateset *statesets[MAXSENTLEN+1];
static struct state *statestack, *topstate, *finstate;
static bool iflag, endoffile, infambig, unifyagain, unifyok, nodeorder;
static int cmdlevel, startp, starttime, chptr, sentlen, term, optnum, numtrees;
static char option;
static uint tokflags;
static struct tmask hadlegal;
static struct tree *fintree, *toptree, *fcrtree;

extern char *malloc();
extern int pclose();

forward bool groupsok(), testbit(), isexpanded();
forward char *fntoken(), *cheap();
forward struct statelist *slnode();
forward struct stateset *newstateset();
forward struct state *addstate(), *newstate(), *copystate();
forward struct tree *makeftree(), *makeft(), *expand(), *expandtree(), *copyftree(), *ftnode();


global main(argc,argv) int argc; char *argv[];
  { int k;
    for (k=0; k < NUMSEGS; k++) segbase[k] = NULL; /* init heap */
    putenv("PS1=* $ "); /* set prompt for sub-shells */
    input = stdin; output = stdout;
    k = 0; if (argv[k] != NULL) k++; /* skip program name */
    iflag = (argv[k] != NULL && seq(argv[k],"-i"));
    startp = Prods[0];
    finstate = topstate = NULL;
    fintree = toptree = fcrtree = NULL;
    cmdlevel = 0;
    starttime = timenow();
    interact();
    wrstats("End of run");
    releaseheap();
    exit(0);
  }

static interact()
  { readcmdline();
    until (endoffile)
      { unless (cmdline[0] == '\0')
	  { if (cmdline[0] == '@')
	      { char *fn = fntoken(&cmdline[1]);
		FILE *in = input;
		input = fopen(fn,"r");
		if (input != NULL)
		  { cmdlevel++;
		    interact();
		    cmdlevel--;
		    fclose(input);
		  }
		else fprintf(output, "Can't open file \"%s\" for reading\n", fn);
		input = in;
	      }
	    else if (cmdline[0] == '.') command();
	    else
	      { releaseheap();
		parsesentence();
	      }
	  }
	readcmdline();
      }
  }

static readcmdline()
  { int ch;
    if ((iflag && cmdlevel == 0) || isatty(input -> _file)) prompt();
    ch = getc(input);
    if (ch == EOF) endoffile = true;
    else
      { int nc = 0;
	until (ch == '\n' || ch == EOF || nc >= MAXLINE)
	  { cmdline[nc++] = ch;
	    ch = getc(input);
	  }
	until (ch == '\n' || ch == EOF) ch = getc(input);
	cmdline[nc++] = '\0';
	wrstats("Parsing: %s", cmdline);
	endoffile = false;
      }
  }

static prompt()
  { fflush(output);
    fprintf(stderr, "\nParse: ");
    fflush(stderr);
  }

static wrstats(s,p1) char *s, *p1;
  { fprintf(output, "\n*** "); wrheap(); wrtime(); fprintf(output, "  ");
    fprintf(output, s, p1); putc('\n',output);
  }

static wrheap()
  { int k = 0, tot = 0;
    while (k < NUMSEGS && segbase[k] != NULL) tot += segptr[k++];
    fprintf(output, "%d bytes (%d/%d) ", tot, k, NUMSEGS);
  }

static wrtime()
  { static int ktab[] = { 10, 10,  6, 10,  6, 10, 10 };
    static int ptab[] = { 11,  9,  8,  5,  4,  1,  0 };
    int now, dt, i; char s[14];
    now = timenow();	      /* time now */
    dt = (now-starttime+3)/6; /* time since beginning of run (1/10s) */
    strcpy(s, "nnh nnm nn.ns");
    for (i=0; i<7; i++)
      { int k = ktab[i], p = ptab[i];
	s[p] = dt%k + '0';
	dt /= k;
      }
    fprintf(output, "%s ", s);
  }

static parsesentence()
  { /* This is Earley's famous Algorithm. */
    finstate = topstate = NULL;
    fintree = toptree = fcrtree = NULL;
    chptr = sentlen = 0;
    statestack = NULL;
    statesets[0] = newstateset();
    addstate(statesets[0], startp, startp+prhs, 0, (struct fpair *) &Prods[startp+plhf]);
    until ((statesets[sentlen] -> sshd == NULL) || (sentlen >= MAXSENTLEN))
      { nextterm(); /* sets term, tokflags */
	sentence[sentlen] = term;
	statesets[++sentlen] = newstateset();
	if (tokflags & tf_prlegal)
	  { memset(hadlegal.m, 0, sizeof(struct tmask));
	    fprintf(output, " # {");
	  }
	processstates();
	if (tokflags & tf_prlegal) fprintf(output, " }");
	if (tokflags & tf_unknown) fprintf(output, " ?");
	unless (term == STOPPER) { putc(' ',output); writetname(term); }
      }
    unless (statesets[sentlen] -> sshd == NULL) fprintf(output, " TOO LONG!");
    putc('\n',output);
  }

static processstates()
  { struct state *st = statesets[sentlen-1] -> sshd;
    until (st == NULL)
      { int j = st -> jval;
	switch (Prods[j+rsym])
	  { default:
		fprintf(output, " BUG(%d)", Prods[j+rsym]);
		break;

	    case x_nonterm:
		predictor(st);
		break;

	    case x_term:
		scanner(st);
		break;

	    case x_gap:
		scangap(st);
		break;

	    case x_end:
		completer(st);
		break;
	  }
	st = st -> sflk;
      }
  }

static predictor(st) struct state *st;
  { int j = st -> jval;
    int q = Prods[j+rval];
    while (q != 0 && Prods[q+plhs] == Prods[j+rval])
      { if ((tokflags & tf_prlegal) || testbit(term, (uint *) &Prods[q+psta]))
	  { struct fpair *rf = (struct fpair *) &Prods[j+rrhf]; /* rhs features */
	    struct fpair *lf = (struct fpair *) &Prods[q+plhf]; /* lhs features */
	    struct fpair af;
	    if (groupsok(lf, rf, &af))
	      { struct state *nst = addstate(statesets[sentlen-1], q, q+prhs, sentlen-1, &af);
		/* add ptr to state which predicted new state */
		*(nst -> backp) = slnode(*(nst -> backp), st, NULL);
	      }
	  }
	q = Prods[q+plnk];
      }
  }

static bool groupsok(f1, f2, af) struct fpair *f1, *f2, *af;
  { /* or's f1 with f2, putting result in af,
       checking group complement masks, returns T/F */
    int k = 0;
    while (k < FMASKSIZE && (f1 -> set.m[k] & f2 -> cpl.m[k]) == 0 && (f2 -> set.m[k] & f1 -> cpl.m[k]) == 0)
      { af -> set.m[k] = f1 -> set.m[k] | f2 -> set.m[k];
	af -> cpl.m[k] = f1 -> cpl.m[k] | f2 -> cpl.m[k];
	k++;
      }
    return (k >= FMASKSIZE);
  }

static scanner(st) struct state *st;
  { int p = st -> pval, j = st -> jval, b = st -> bval;
    struct fpair *f = &st -> fval;
    int t = Prods[j+rval]; /* the terminal to scan */
    if (tokflags & tf_prlegal)
      { unless (testbit(t, hadlegal.m))
	  { putc(' ',output); writetname(t);
	    setbit(t,hadlegal.m);
	  }
      }
    if (term == t)
      { struct state *nst = addstate(statesets[sentlen], p, j+rsiz, b, f);
	nst -> backp = st -> backp;
	nst -> treep = slnode(nst -> treep, st, NULL);
      }
  }

static scangap(st) struct state *st;
  { int p = st -> pval, j = st -> jval, b = st -> bval;
    struct fpair *f = &st -> fval;
    struct state *nst = addstate(statesets[sentlen-1], p, j+rsiz, b, f);
    nst -> backp = st -> backp;
    nst -> treep = slnode(nst -> treep, st, NULL);
  }

static completer(st) struct state *st;
  { struct statelist *x = *(st -> backp); /* list of states which predicted st */
    if (st -> pval == startp)
      { fprintf(output, " YES");
	topstate = st;
      }
    finstate = st; /* record most recently completed state */
    until (x == NULL)
      { struct state *sb = x -> sta;
	int pb = sb -> pval, jb = sb -> jval, bb = sb -> bval;
	struct fpair *fb = &sb -> fval;
	struct state *nst = addstate(statesets[sentlen-1], pb, jb+rsiz, bb, fb);
	nst -> backp = sb -> backp;
	nst -> treep = slnode(nst -> treep, sb, st);
	x = x -> link;
      }
    unless (st -> pval == startp) st -> mark |= m_comp;
  }

static struct statelist *slnode(x, sta, stb) struct statelist *x; struct state *sta, *stb;
  { struct statelist *y = heap(struct statelist);
    y -> link = x;
    y -> sta = sta;
    y -> stb = stb;
    return y;
  }

static struct stateset *newstateset()
  { int i;
    struct stateset *sts = heap(struct stateset);
    sts -> sshd = NULL; sts -> sstl = &sts -> sshd;
    for (i=0; i < HASHSIZE; i++) sts -> hash[i] = NULL;
    return sts;
  }

#define eqst(s1,s2) \
    ( s1 -> pval == s2 -> pval && \
      s1 -> jval == s2 -> jval && \
      s1 -> bval == s2 -> bval && \
      memcmp(&s1 -> fval.set, &s2 -> fval.set, sizeof(struct fmask)) == 0 \
    )

static struct state *addstate(sts, p, j, b, f) struct stateset *sts; int p,j,b; struct fpair *f;
  { struct state *nst = newstate(p,j,b,f);
    int hv = hashval(nst);
    struct state **zz = &sts -> hash[hv];
    struct state *z = *zz;
    struct state *st;
    until (z == NULL || (!(z -> mark & m_comp) && eqst(z,nst)))
      { zz = &z -> shlk;
	z = *zz;
      }
    if (z != NULL)
      { st = z; /* the already-existing state */
	statestack = nst; /* make nst available for re-use */
      }
    else
      { /* it's a new state; append it to state set */
	*zz = nst; nst -> shlk = NULL;
	*(sts -> sstl) = nst; sts -> sstl = &nst -> sflk; nst -> sflk = NULL;
	st = nst; /* the new state */
      }
    return st;
  }

static struct state *newstate(p, j, b, f) int p, j, b; struct fpair *f;
  { struct state *st;
    if (statestack != NULL)
      { st = statestack;
	statestack = NULL;
      }
    else
      { st = heap(struct state);
	st -> treep = NULL;
	st -> backp = &st -> bpslot;
	st -> bpslot = NULL;
	st -> mark = 0;
      }
    st -> pval = p;
    st -> jval = j;
    st -> bval = b;
    st -> fval = *f; /* struct assignment */
    return st;
  }

static int hashval(st) struct state *st;
  { uint sum = 0; int k;
    sum = (sum*65599) + (st -> pval);
    sum = (sum*65599) + (st -> jval);
    sum = (sum*65599) + (st -> bval);
    for (k=0; k < FMASKSIZE; k++) sum = (sum*65599) + (st -> fval.set.m[k]);
    return (sum & MAXINT) % HASHSIZE;
  }

static nextterm()
  { /* Assemble a token.
       This proc assumes that the terms table is ordered so that
       e.g. "abc" comes before "ab".
       Sets term, tokflags */
    bool found = false;
    term = STOPPER; tokflags = 0;
    until (found || cmdline[chptr] == '\0')
      { if (cmdline[chptr] == '#')
	  { tokflags |= tf_prlegal;
	    chptr++;
	  }
	else
	  { int p = 0, t = 0;
	    int tlen;
	    until (found || Terms[p] == '\0')
	      { int k = 0;
		until (Terms[p] == '\0' || Terms[p] != cmdline[chptr+k]) { p++; k++; }
		if (Terms[p] == '\0') { found = true; term = t; tlen = k; }
		until (Terms[p] == '\0') p++; /* point to first char of next entry */
		p++; t++;
	      }
	    if (found) chptr += tlen; /* point past token */
	    else
	      { unless (cmdline[chptr] == ' ' || cmdline[chptr] == '\t') tokflags |= tf_unknown;
		chptr++;
	      }
	  }
      }
  }

static command()
  { int p = 1; short cmd = 0;
    while (p < 3 && cmdline[p] != '\0') cmd = (cmd >> 8) | (cmdline[p++] << 8);
    if (cmd == 'sh') system("exec sh"); /* don't redirect .sh */
    else
      { option = '\0'; optnum = 0;
	until (cmdline[p] == '\0' || cmdline[p] == '>' || cmdline[p] == '|')
	  { if (cmdline[p] >= 'a' && cmdline[p] <= 'z') option = cmdline[p++];
	    else if (cmdline[p] >= '0' && cmdline[p] <= '9')
	      { optnum = 0;
		/* legal range is 0-9999 */
		while (optnum <= 999 && cmdline[p] >= '0' && cmdline[p] <= '9')
		  optnum = (optnum*10) + (cmdline[p++]-'0');
		if (cmdline[p] >= '0' && cmdline[p] <= '9')
		  { fprintf(output, "Number too large!\n");
		    optnum = 9999;
		    while (cmdline[p] >= '0' && cmdline[p] <= '9') p++;
		  }
	      }
	    else
	      { unless (cmdline[p] == ' ' || cmdline[p] == '\n')
		  fprintf(output, "Bad option character: /%c/\n", cmdline[p]);
		p++;
	      }
	  }
	if (cmdline[p] == '>' || cmdline[p] == '|')
	  { char *fn = fntoken(&cmdline[p+1]);
	    FILE *fi = ((cmdline[p] == '|') ? popen : fopen) (fn, "w");
	    if (fi != NULL)
	      { FILE *out = output;
		fflush(output); output = fi;
		docmd(cmd);
		((cmdline[p] == '|') ? pclose : fclose) (fi);
		output = out;
	      }
	    else fprintf(output, "Can't open file file/pipe \"%s\" for writing\n", fn);
	  }
	else docmd(cmd);
      }
  }

static char *fntoken(s) char *s;
  { int p = 0; char *tok;
    /* skip leading blanks in token */
    while (s[p] == ' ' || s[p] == '\t') p++;
    tok = &s[p];
    /* remove trailing blanks */
    until (s[p] == '\0') p++;
    while (p > 0 && (s[p-1] == ' ' || s[p-1] == '\t')) p--;
    s[p] = '\0'; /* this clobbers cmdline */
    return tok;
  }

static docmd(cmd) short cmd;
  { switch (cmd)
      { default:
	    fprintf(output, "I don't understand.\n");
	    break;

	case 'pr':
	    printpartial(finstate,&fintree);
	    break;

	case 'pt':
	    printpartial(topstate,&toptree);
	    break;

	case 'di':
	    applyfcrs();
	    break;

	case 'rc':
	    starttime = timenow();
	    break;
      }
  }

static printpartial(st,xx) struct state *st; struct tree **xx;
  { if (option == 'b' || option == '\0')
      { if (st == NULL) fprintf(output, "No parse.\n");
	else
	  { if (*xx == NULL) *xx = makeftree(st);
	    printftree(*xx);
	  }
      }
    else fprintf(output, "Illegal .pr/.pt command.\n");
  }

static applyfcrs()
  { if (option == 'b' || option == 'x' || option == '\0')
      { if (topstate == NULL) fprintf(output, "No parse.\n");
	else
	  { if (fcrtree == NULL)
	      { if (toptree == NULL) toptree = makeftree(topstate);
		infambig = false; /* set true by copyftree */
		fcrtree = copyftree(expand(toptree));
	      }
	    if (infambig) fprintf(output, "Infinitely ambiguous!\n");
	    else
	      { numtrees = 0;
		scanftrees(fcrtree);
		putc('\n',output);
		if (optnum > numtrees) fprintf(output, "Tree number %d does not exist.\n", optnum);
		fprintf(output, (numtrees == 1) ? "There was %d tree." : "There were %d trees.", numtrees);
		putc('\n',output);
	      }
	  }
      }
    else fprintf(output, "Illegal .di command.\n");
  }

static struct tree *makeftree(st) struct state *st;
  { return makeft(st, NULL, true); }

static struct tree *makeft(stc, stp, v) struct state *stc, *stp; bool v;
  { /* Convert Earley-style factorised "tree"
       into a more manageable binary parse tree.
       stc is completed state "X: a ."
       stp is predicted state "Y: b . X c" */
    struct tree *ft;
    if (stc == NULL) ft = NULL;
    else if (!(stc -> mark & m_frec))
      { struct statelist *x = stc -> treep;
	stc -> mark |= m_frec; /* to trap cycles */
	ft = NULL;
	until (x == NULL)
	  { struct tree *ft1 = makeft(x -> sta, NULL, false);
	    struct tree *ft2 = makeft(x -> stb, x -> sta, true);
	    struct tree *ftx = (ft1 == NULL) ? ft2 : (ft2 == NULL) ? ft1 : ftnode(s_across, ft1, ft2);
	    if (ft == NULL) ft = ftx; else ft = ftnode(s_or, ft, ftx);
	    x = x -> link;
	  }
	stc -> mark &= ~m_frec;
      }
    else ft = ftnode(s_cycle);
    if (v) ft = (ft == NULL) ? ftnode(s_leaf, stp) : ftnode(s_down, stc, ft);
    return ft;
  }

static struct tree *expand(x) struct tree *x;
  { /* Expand tree
       Nodes dominate in this order:  OR --> (ACROSS, DOWN, LEAF) */
    return isexpanded(x,0) ? x : expandtree(x);
  }

static bool isexpanded(x,p) struct tree *x; int p;
  { switch (x -> h1)
      { default:
	    treebug(x, "isexpanded");
	    return true;

	case s_or:
	    return (p <= 0 && isexpanded(x -> h2.ft, 0) && isexpanded(x -> h3.ft, 0));

	case s_across:
	    return (p <= 1 && isexpanded(x -> h2.ft, 1) && isexpanded(x -> h3.ft, 1));

	case s_down:
	    return (p <= 1 && isexpanded(x -> h3.ft, 1));

	case s_leaf:	case s_cycle:
	    return true;
      }
  }

static struct tree *expandtree(x) struct tree *x;
  { switch (x -> h1)
      { default:
	    treebug(x, "expandtree");
	    return x;

	case s_or:
	    return ftnode(s_or, expand(x -> h2.ft), expand(x -> h3.ft));

	case s_across:
	  { struct tree *x1 = expand(x -> h2.ft);
	    struct tree *x2 = expand(x -> h3.ft);
	    struct tree *z;
	    if (x1 -> h1 == s_or)
	      { struct tree *p = ftnode(s_across, x1 -> h2, x2);
		struct tree *q = ftnode(s_across, x1 -> h3, x2);
		z = ftnode(s_or, expand(p), expand(q));
	      }
	    else if (x2 -> h1 == s_or)
	      { struct tree *p = ftnode(s_across, x1, x2 -> h2);
		struct tree *q = ftnode(s_across, x1, x2 -> h3);
		z = ftnode(s_or, expand(p), expand(q));
	      }
	    else z = ftnode(s_across, expand(x1), expand(x2));
	    return z;
	  }

	case s_down:
	  { struct tree *x1 = expand(x -> h3.ft);
	    struct state *stc = x -> h2.st;
	    struct tree *z;
	    if (x1 -> h1 == s_or)
	      { struct tree *p = ftnode(s_down, stc, x1 -> h2);
		struct tree *q = ftnode(s_down, stc, x1 -> h3);
		z = ftnode(s_or, expand(p), expand(q));
	      }
	    else z = ftnode(s_down, stc, x1);
	    return z;
	  }
      }
  }

static struct tree *copyftree(x) struct tree *x;
  { switch (x -> h1)
      { default:
	    treebug(x, "copyftree");
	    return x;

	case s_or:	case s_across:
	    return ftnode(x -> h1, copyftree(x -> h2.ft), copyftree(x -> h3.ft));

	case s_down:
	    return ftnode(s_down, copystate(x -> h2.st), copyftree(x -> h3.ft));

	case s_leaf:
	    return x;

	case s_cycle:
	    infambig = true;
	    return x;
      }
  }

static struct tree *ftnode(sy, x2, x3) enum symbol sy; union treeu x2, x3;
  { struct tree *ft = heap(struct tree);
    ft -> h1 = sy;
    ft -> h2 = x2;
    ft -> h3 = x3;
    return ft;
  }

static struct state *copystate(st) struct state *st;
  { struct state *nst = heap(struct state);
    *nst = *st; /* struct assignment */
    return nst;
  }

static scanftrees(x) struct tree *x;
  { if (x -> h1 == s_or)
      { scanftrees(x -> h2.ft);
	scanftrees(x -> h3.ft);
      }
    else unifyandprint(x);
  }

static unifyandprint(x) struct tree *x;
  { unifyftree(x);
    if (unifyok || option == 'x')
      { numtrees++;
	if (optnum == 0 || optnum == numtrees)
	  { printftree(x);
	    if (option == 'x')
	      { putc('\n',output);
		fprintf(output, unifyok ? "Successful tree" : "Inadmissible tree");
		putc('\n',output);
	      }
	  }
      }
  }

static unifyftree(x) struct tree *x;
  { unifyagain = unifyok = true;
    while (unifyagain && (unifyok || option == 'x'))
      { unifyagain = false;
	nodeorder = o_perc; scanlevels(x, NULL);   /* bottom-up for percolating features */
	nodeorder = o_tric; scanlevels(x, NULL);   /* top-down for trickling features	 */
      }
  }

static scanlevels(x, mst) struct tree *x; struct state *mst;
  { /* visit each node in the tree x */
    if (unifyok || option == 'x')
      { switch (x -> h1)
	  { default:
		treebug(x, "scanlevels");
		break;

	    case s_across:
		scanlevels(x -> h2.ft, mst);
		scanlevels(x -> h3.ft, mst);
		break;

	    case s_down:
	      { struct state *dst = x -> h2.st;
		if (nodeorder & o_perc)
		  { /* visit nodes bottom-up */
		    scanlevels(x -> h3.ft, dst);
		    unifyfcrs(dst);
		    unless (mst == NULL)
		      { /* copy mother features in daughter onto base features in mother */
			instantiate(&dst -> fval, &mst -> fval, mst, FMASKSIZE/2, 0);
		      }
		  }
		if (nodeorder & o_tric)
		  { /* visit nodes top-down */
		    unless (mst == NULL)
		      { /* copy base features in mother onto mother features in daughter */
			instantiate(&mst -> fval, &dst -> fval, dst, 0, FMASKSIZE/2);
		      }
		    unifyfcrs(dst);
		    scanlevels(x -> h3.ft, dst);
		  }
		break;
	      }

	    case s_leaf:
		break;
	  }
      }
  }

static unifyfcrs(st) struct state *st;
  { /* attempt to satisfy fcrs on a node */
    struct fpair *f = &st -> fval;
    int mp = 0;
    until ((Clauses[mp].ord & o_end) || !(unifyok || option == 'x'))
      { /* try negatives first */
	  { struct fmask *nm = &Clauses[mp].neg;
	    int k = 0;
	    while (k < FMASKSIZE && (nm -> m[k] & ~f -> set.m[k]) == 0) k++;
	    if (k >= FMASKSIZE)
	      { /* negatives not satisfied; try positives */
		struct fmask *pm = &Clauses[mp].pos;
		int j = 0;
		while (j < FMASKSIZE && (pm -> m[j] & f -> set.m[j]) == 0) j++;
		if (j >= FMASKSIZE)
		  { /* positives not satisfied; a new feature is required */
		    if (nodeorder == 0 || Clauses[mp].ord == nodeorder)
		      { struct fpair *add = &Clauses[mp].add;
			int i = 0;
			while (i < FMASKSIZE && add -> set.m[i] == 0) i++;
			if (i < FMASKSIZE)
			  { /* copy all features (base + mother) */
			    instantiate(add, f, st, 0, 0);
			    instantiate(add, f, st, FMASKSIZE/2, FMASKSIZE/2);
			  }
			else
			  { unifyok = false; /* there isn't one */
			    st -> mark = m_blfcr;
			  }
		      }
		  }
	      }
	  }
	mp++;
      }
  }

static instantiate(f1, f2, st, j1, j2) struct fpair *f1, *f2; struct state *st; int j1, j2;
  { /* copy half the feature space from offset j1 in f1 to offset j2 in f2 */
    int k;
    for (k=0; k < FMASKSIZE/2; k++)
      { if ((f1 -> set.m[j1] & f2 -> cpl.m[j2]) != 0 || (f2 -> set.m[j2] & f1 -> cpl.m[j1]) != 0)
	  { unifyok = false;
	    st -> mark |= m_blgrp;
	  }
	if ((f1 -> set.m[j1] & ~f2 -> set.m[j2]) != 0) unifyagain = true;
	f2 -> set.m[j2] |= f1 -> set.m[j1];
	f2 -> cpl.m[j2] |= f1 -> cpl.m[j1];
	j1++; j2++;
      }
  }

static printftree(x) struct tree *x;
  { fprintf(output, "\n{\n");
    prftree(x, 0);
    fprintf(output, "}\n");
  }

static prftree(x, k) struct tree *x; int k;
  { switch (x -> h1)
      { default:
	    treebug(x, "pftree");
	    break;

	case s_or:
	  { int n = 0;
	    pralts(x, k, &n);
	    break;
	  }

	case s_across:
	    prftree(x -> h2.ft, k);
	    prftree(x -> h3.ft, k);
	    break;

	case s_down:
	    disptreestate(x -> h2.st, k);
	    prftree(x -> h3.ft, k+2);
	    break;

	case s_leaf:
	    disptreestate(x -> h2.st, k);
	    break;

	case s_cycle:
	  { int i;
	    for (i=0; i<k; i++) putc('.',output);
	    fprintf(output, "*RECURSIVE*");
	    putc('\n',output);
	    break;
	  }
      }
  }

static pralts(x, k, nn) struct tree *x; int k; int *nn;
  { if (x -> h1 == s_or)
      { pralts(x -> h2.ft, k, nn);
	pralts(x -> h3.ft, k, nn);
      }
    else
      { int i;
	for (i=0; i<(k-1); i++) putc('.',output);
	fprintf(output, "(%d)\n", ++(*nn));
	prftree(x, k);
      }
  }

static treebug(x, s) struct tree *x; char *s;
  { fprintf(output, "Bug in %s! (%d)\n", s, x -> h1); }

static disptreestate(st, k) struct state *st; int k;
  { int j = st -> jval;
    uint m = st -> mark;
    uint x = Prods[j+rsym];
    int i;
    for (i=0; i<k; i++) putc('.',output);
    if (x == x_term)
      { /* state was created by scanner */
	putc('"',output);
	writetname(sentence[st -> bval]);
	putc('"',output);
      }
    else if (x == x_gap)
      { /* state was created by scangap */
	fprintf(output, "GAP");
      }
    else
      { /* state was created by completer */
	writentname(st -> pval);
	unless (option == 'b')
	  { putc(' ',output);
	    writefnames(&st -> fval.set);
	  }
      }
    if (m & (m_blfcr | m_blgrp))
      { fprintf(output, " *INADM*");
	if (m & m_blfcr) fprintf(output, " fcr");
	if (m & m_blgrp) fprintf(output, " grp");
      }
    putc('\n',output);
  }

static writentname(p) int p;
  { int k = 0, q = 1, qb = 1;
    until (q >= p || qb == 0)
      { q = Prods[q+plnk];
	if (Prods[q+plhs] != qb) { qb = q; k++; }
      }
    wrname(Nnames, k);
  }

static writetname(t) int t;
  { if (t == STOPPER) fprintf(output, "<STOPPER>"); else wrname(Terms, t); }

static wrname(tab, n) char *tab; int n;
  { int k = 0, p = 0;
    until (k >= n || tab[p] == '\0')
      { until (tab[p] == '\0') p++;
	k++; p++;
      }
    fprintf(output, "%s", (tab[p] == '\0') ? "?!?" : &tab[p]);
  }

static writefnames(m) struct fmask *m;
  { int k = 0, p = 0;
    bool pr = false;
    putc('[',output);
    while (k < MAXFSPECS/2) /* don't bother to list 'M' features */
      { bool pres = testbit(k, m -> m);
	if (pres)
	  { if (pr) fprintf(output, ", ");
	    pr = true;
	  }
	until (Fnames[p] == '\0')
	  { if (pres) putc(Fnames[p],output);
	    p++;
	  }
	k++; p++;
      }
    putc(']',output);
  }

static int timenow()
  { struct tms tms;
    times(&tms);
    return (tms.tms_utime + tms.tms_stime); /* user + system */
  }

static bool testbit(k, m) int k; uint *m;
  { int i = k/32, j = k%32;
    return ((m[i] & (1 << j)) != 0);
  }

static setbit(k, m) int k; uint *m;
  { int i = k/32, j = k%32;
    m[i] |= (1 << j);
  }

static char *cheap(nb) int nb; /* called by "heap" macro */
  { int k = 0;
    bool found = false;
    char *x;
    while (k < NUMSEGS && !found)
      { if (segbase[k] == NULL)
	  { segbase[k] = malloc(SEGSIZE);
	    segptr[k] = 0;
	  }
	if (segbase[k] == NULL)
	  { fprintf(output, "Sorry, no room! (1)\n");
	    exit(1);
	  }
	if (segptr[k] + nb <= SEGSIZE)
	  { x = segbase[k] + segptr[k];
	    segptr[k] += nb;
	    /* move segment to front of list for efficiency */
	    swap(&segbase[k], &segbase[0]); swap(&segptr[k], &segptr[0]);
	    found = true;
	  }
	k++;
      }
    unless (found)
      { fprintf(output, "Sorry, no room! (2)\n");
	exit(1);
      }
    return x;
  }

static swap(xx, yy) int *xx, *yy;
  { int t = *xx;
    *xx = *yy; *yy = t;
  }

static releaseheap()
  { /* Don't free() the segments, just mark them as empty */
    int k = 0;
    while (k < NUMSEGS && segbase[k] != NULL) segptr[k++] = 0;
  }
