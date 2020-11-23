#include "vwg.h"
#include <stdio.h>
#include <setjmp.h>

extern struct ruleset *hypersyntax;
extern struct hyperrule *toplevelhr;
extern struct hypernotion *emptypn;
extern struct dictnode *ghead, *metalist;
extern int linenum;

extern initdictionary();				 /* from common */
extern struct dictnode *newmetanotion(), *newterminal(); /* from common */
extern error();						 /* from common */
extern bool ispn();					 /* from common */

static FILE *gfile;
static enum symbol symb;
static struct dictnode *item_z;
static int item_s;
static jmp_buf recover;
static int ch;

forward char *symbof();
forward struct hyperalt *hyperalt();
forward struct metarhs *metarhs();
forward struct hypernotion *makehn(), *hypercomp(), *hypernotion();


global readgrammar(fn) char *fn;
  { gfile = fopen(fn, "r");
    if (gfile == NULL)
      { fprintf(stderr, "vwg: can't open %s\n", fn);
	exit(1);
      }
    initdictionary();
    hypersyntax = NULL;
    metalist = NULL;
    ghead = newmetanotion("_GHEAD");
    emptypn = makehn("*");
    maketoplevel();
    linenum = 0;
    ch = '\n'; /* for linenum count */
    nextsymb();
    _setjmp(recover);
    until (symb == s_eof)
      { readrule();
	checkfor(s_dot);
      }
    fclose(gfile);
  }

static maketoplevel()
  { /* make a hyperrule "_top: program" */
    struct hypernotion *lhs; struct hypernotion **r; struct hyperalt *rhs;
    lhs = makehn("_top*");
    r = heap(1, struct hypernotion *);
    r[0] = makehn("program*");   /* starting symbol */
    rhs = heap(1, struct hyperalt);
    rhs -> rlen = 1;
    rhs -> rdef = r;
    addhyperrule(lhs, rhs);
    toplevelhr = hypersyntax -> hr;
  }

static struct hypernotion *makehn(s) char *s;
  { struct hitem *vec; struct hypernotion *hn; int i;
    int nc = strlen(s);
    vec = heap(nc, struct hitem);
    for (i=0; i < nc; i++)
      { vec[i].sy = s_ssm;
	vec[i].it_s = s[i];
      }
    hn = heap(1, struct hypernotion);
    hn -> hlen = nc;
    hn -> hdef = vec;
    hn -> flags = 0;
    hn -> lnum = -1;
    addtoghead(hn);
    return hn;
  }

static readrule()
  { struct hypernotion *lhs = hypernotion();
    if (symb == s_colon)
      { /* hyperrule */
	struct hyperalt *rhs;
	nextsymb();
	rhs = hyperalt();
	addhyperrule(lhs, rhs);
	while (symb == s_semico)
	  { nextsymb();
	    rhs = hyperalt();
	    addhyperrule(lhs, rhs);
	  }
	addstopper(lhs);
	addtoghead(lhs);
      }
    else if (symb == s_dcolon)
      { /* metarule */
	nextsymb();
	if (lhs -> hlen == 1 && lhs -> hdef[0].sy == s_meta)
	  { struct dictnode *y = lhs -> hdef[0].it_z;	 /* the lhs metanotion */
	    if (y -> rhs != NULL) error("metanotion %s is multiply defined", y -> str);
	    y -> rhs = metarhs();
	  }
	else error("bad lhs of metarule");
      }
    else error(": or :: expected");
  }

static addhyperrule(lhs, rhs) struct hypernotion *lhs; struct hyperalt *rhs;
  { struct hyperrule *hr; struct ruleset *rs;
    hr = heap(1, struct hyperrule);
    hr -> lhs = lhs;
    hr -> rhs = rhs;
    rs = heap(1, struct ruleset);
    rs -> hr = hr;
    rs -> lnk = hypersyntax; hypersyntax = rs;
  }

static struct hyperalt *hyperalt()
  { struct hyperalt *alt;
    struct hypernotion *v[MAXRHS];
    int nr = 0;
    if (symb == s_meta || symb == s_ssm)
      { v[nr++] = hypercomp();
	while (symb == s_comma && nr < MAXRHS)
	  { nextsymb();
	    v[nr++] = hypercomp();
	  }
	if (symb == s_comma) error("right-hand side of hyperrule is too long");
      }
    else if (symb == s_term)
      { struct hypernotion *hn = heap(1, struct hypernotion);
	hn -> hlen = -1;
	hn -> flags = 0;
	hn -> lnum = linenum;
	hn -> term = item_z;
	v[nr++] = hn;
	nextsymb();
      }
    alt = heap(1, struct hyperalt);
    alt -> rdef = heap(nr, struct hypernotion *);
    memcpy(alt -> rdef, v, nr * sizeof(struct hypernotion *));
    alt -> rlen = nr;
    return alt;
  }

static struct hypernotion *hypercomp()
  { struct hypernotion *hn = hypernotion();
    addstopper(hn); addtoghead(hn);
    return hn;
  }

static addstopper(hn) struct hypernotion *hn;
  { /* append a stopper hitem to hn */
    struct hitem *x = &hn -> hdef[hn -> hlen++];
    x -> sy = s_ssm;
    x -> it_s = '*';
  }

static addtoghead(hn) struct hypernotion *hn;
   { /* add a new defn to _GHEAD */
    struct metarhs *mr = heap(1, struct metarhs);
    mr -> hn = hn;
    mr -> lnk = ghead -> rhs;
    ghead -> rhs = mr;
  }

static struct metarhs *metarhs()
  { struct metarhs *x = heap(1, struct metarhs);
    x -> hn = hypernotion();
    if (symb == s_semico)
      { nextsymb();
	x -> lnk = metarhs();
      }
    else x -> lnk = NULL;
    return x;
  }

static struct hypernotion *hypernotion()
  { struct hypernotion *hn;
    struct hitem vec[MAXHITEMS-1];
    int nr = 0;
    while ((symb == s_ssm || symb == s_meta) && (nr < MAXHITEMS-1)) /* allow extra slot for stopper */
      { bool stuff = true;
	if (symb == s_meta)
	  { struct metarhs *md = item_z -> rhs;
	    if (md != NULL && md -> lnk == NULL && ispn(md -> hn))
	      { /* it's a metanotion defined as a single protonotion; substitute */
		int j;
		for (j=0; (j < md -> hn -> hlen) && (nr < MAXHITEMS-1); j++)
		  vec[nr++] = md -> hn -> hdef[j];
		if (j < md -> hn -> hlen) error("hypernotion is too long after substitution for %s", item_z -> str);
		stuff = false;
	      }
	  }
	if (stuff)
	  { vec[nr].sy = symb;
	    vec[nr].it_s = item_s;     /* valid for s_ssm  */
	    vec[nr].it_z = item_z;     /* valid for s_meta */
	    nr++;
	  }
	nextsymb();
      }
    if (symb == s_meta || symb == s_ssm) error("hypernotion is too long");
    hn = heap(1, struct hypernotion);
    hn -> lnum = linenum;
    hn -> flags = 0;
    if (nr >= 5 && vecstarts(vec, "where"))
      { hn -> flags |= hn_pospred;
      }
    else if (nr >= 6 && vecstarts(vec, "unless"))
      { int j;
	setstart(vec, "where");
	for (j=0; j < nr-6; j++) vec[5+j] = vec[6+j];
	nr--;
	hn -> flags |= hn_negpred;
      }
    hn -> hdef = heap(nr+1, struct hitem); /* extra slot for stopper if needed */
    memcpy(hn -> hdef, vec, nr * sizeof(struct hitem));
    hn -> hlen = nr;
    return hn;
  }

static bool vecstarts(vec, s) struct hitem vec[]; char *s;
  { int j = 0;
    until ((s[j] == '\0') || !(vec[j].sy == s_ssm && vec[j].it_s == s[j])) j++;
    return (s[j] == '\0');
  }

static setstart(vec, s) struct hitem vec[]; char *s;
  { int j = 0;
    until (s[j] == '\0')
      { vec[j].sy = s_ssm;
	vec[j].it_s = s[j];
	j++;
      }
  }

static checkfor(sy) enum symbol sy;
  { unless (symb == sy)
      { error("%s found where %s expected", symbof(symb), symbof(sy));
	skipsymbols();
	_longjmp(recover);
      }
    nextsymb();
  }

static skipsymbols()
  { unless (symb == s_eof) nextsymb();
    until (symb == s_dot || symb == s_eof) nextsymb();
    unless (symb == s_eof) nextsymb();
  }

static char *symbof(sy) enum symbol sy;
  { switch (sy)
      { default:	return "???";
	case s_colon:	return ":";
	case s_dcolon:	return "::";
	case s_comma:	return ",";
	case s_semico:	return ";";
	case s_dot:	return ".";
	case s_eof:	return "<end of file>";
	case s_ssm:	return "<small syntactic mark>";
	case s_meta:	return "<metanotion>";
	case s_term:	return "<terminal>";
      }
  }

static nextsymb()
  {
l:  switch (ch)
      { default:
	    error("illegal character '%c'", ch);
	    nextchar();
	    goto l;

	case ' ':   case '\t':	case '\n':
	    nextchar();
	    goto l;

	case '/':
	    do nextchar(); until (ch == '\n' || ch == EOF);
	    goto l;

	case '{':
	    do nextchar(); until (ch == '}' || ch == EOF);
	    if (ch == '}') nextchar(); else error("mismatched { }");
	    goto l;

	case EOF:
	    symb = s_eof;
	    break;

	case 'a':   case 'b':	case 'c':   case 'd':	case 'e':
	case 'f':   case 'g':	case 'h':   case 'i':	case 'j':
	case 'k':   case 'l':	case 'm':   case 'n':	case 'o':
	case 'p':   case 'q':	case 'r':   case 's':	case 't':
	case 'u':   case 'v':	case 'w':   case 'x':	case 'y':
	case 'z':   case '<':	case '>':
	    symb = s_ssm;
	    item_s = ch;
	    nextchar();
	    break;

	case 'A':   case 'B':	case 'C':   case 'D':	case 'E':
	case 'F':   case 'G':	case 'H':   case 'I':	case 'J':
	case 'K':   case 'L':	case 'M':   case 'N':	case 'O':
	case 'P':   case 'Q':	case 'R':   case 'S':	case 'T':
	case 'U':   case 'V':	case 'W':   case 'X':	case 'Y':
	case 'Z':
	    metanotion();
	    break;

	case '"':   case '\'':
	    terminal();
	    break;

	case ':':
	    nextchar();
	    if (ch == ':')
	      { nextchar();
		symb = s_dcolon;
	      }
	    else symb = s_colon;
	    break;

	case ',':
	    nextchar();
	    symb = s_comma;
	    break;

	case ';':
	    nextchar();
	    symb = s_semico;
	    break;

	case '.':
	    nextchar();
	    symb = s_dot;
	    break;
      }
  }

#define ismetach(c) ((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9'))

static metanotion()
  { char v[MAXNAMELEN+1];
    int nc = 0;
    while (ismetach(ch) && nc < MAXNAMELEN)
      { v[nc++] = ch;
	nextchar();
      }
    if (ismetach(ch)) error("metanotion too long");
    v[nc++] = '\0';
    symb = s_meta;
    item_z = newmetanotion(v);
  }

static terminal()
  { char v[MAXNAMELEN+1];
    int nc = 0, qu = ch;
    nextchar();
    until (ch == qu || ch == '\n' || ch == EOF || nc >= MAXNAMELEN)
      { v[nc++] = ch;
	nextchar();
      }
    if (ch == qu) nextchar();
    else error("terminal is too long, or missing %c", qu);
    v[nc++] = '\0';
    symb = s_term;
    item_z = newterminal(v);
  }

static nextchar()
  { if (ch == '\n') linenum++;
    ch = getc(gfile);
  }

