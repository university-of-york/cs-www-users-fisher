/* jtp - Compiler - Java bytecode to Transputer assembly code
   A.J. Fisher	 June 1997 */

#include <stdio.h>
#include <setjmp.h>
#include "jtp.h"
#include "tcodes.h"

#define MAXOPCODES 81
#define MAXDEFS	   10

enum symbol
  { s_eof, s_eol, s_ident, s_label, s_number, s_wp, s_equ, s_bitnot,
    s_plus, s_minus, s_times, s_eq,
  };

struct opcode
  { char *s;
    int op, m;
  };

struct definition
  { char *s;
    int n;
  };

struct templ
  { int op, m;
  };

static templ templates[] =
  { { t_adc,	      m_num  },	       { t_and,		 m_none },
    { t_ajw,	      m_num  },	       { t_bitcnt,	 m_none },
    { t_blkb,	      m_num  },
    { t_call,	      m_name },	       { t_cj,		 m_name },
    { t_clrhalterr,   m_none },	       { t_diff,	 m_none },
    { t_div,	      m_none },	       { t_dup,		 m_none },
    { t_eqc,	      m_num  },	       { t_fpadd,	 m_none },
    { t_fpdiv,	      m_none },	       { t_fpi32tor32,	 m_none },
    { t_fpi32tor64,   m_none },	       { t_fpeq,	 m_none },
    { t_fpgt,	      m_none },	       { t_fpint,	 m_none },
    { t_fpldnldb,     m_none },	       { t_fpldnlsn,	 m_none },
    { t_fpldzerodb,   m_none },	       { t_fpldzerosn,	 m_none },
    { t_fpmul,	      m_none },	       { t_fpremfirst,	 m_none },
    { t_fpremstep,    m_none },	       { t_fprev,	 m_none },
    { t_fpstnldb,     m_none },	       { t_fpstnli32,	 m_none },
    { t_fpstnlsn,     m_none },	       { t_fpsub,	 m_none },
    { t_fpuabs,	      m_none },	       { t_fpur32tor64,	 m_none },
    { t_fpur64tor32,  m_none },	       { t_fpurz,	 m_none },
    { t_fpusqrtfirst, m_none },	       { t_fpusqrtstep,	 m_none },
    { t_fpusqrtlast,  m_none },	       { t_gajw,	 m_none },
    { t_gcall,	      m_none },	       { t_gt,		 m_none },
    { t_in,	      m_none },	       { t_j,		 m_name },
    { t_lb,	      m_none },	       { t_ldc,		 m_num	},
    { t_ldl,	      m_wksp },	       { t_ldlp,	 m_wksp },
    { t_ldnl,	      m_num  },	       { t_ldnlp,	 m_num	},
    { t_ldpi,	      m_name },	       { t_lend,	 m_name },
    { t_mint,	      m_none },	       { t_not,		 m_none },
    { t_or,	      m_none },
    { t_out,	      m_none },	       { t_outbyte,	 m_none },
    { t_prod,	      m_none },	       { t_psect,	 m_num	},
    { t_rem,	      m_none },	       { t_ret,		 m_none },
    { t_rev,	      m_none },	       { t_runp,	 m_none },
    { t_sb,	      m_none },	       { t_seterr,	 m_none },
    { t_sethalterr,   m_none },
    { t_shl,	      m_none },	       { t_shr,		 m_none },
    { t_startp,	      m_none },	       { t_sthf,	 m_none },
    { t_stl,	      m_wksp },	       { t_stlf,	 m_none },
    { t_stnl,	      m_num  },	       { t_stopp,	 m_none },
    { t_sttimer,      m_none },	       { t_sum,		 m_none },
    { t_testerr,      m_none },	       { t_word,	 m_num	},
    { t_wcnt,	      m_none },	       { t_wsub,	 m_none },
    { t_wsubdb,	      m_none },	       { t_xor,		 m_none },
    { t_end,	      m_none }, /* stopper */
  };

static FILE *infile;
static char *fname, *equid, *wpid;
static int symb, ch, numopcodes, lineno, numdefs;
static word item;
static bool soline;
static tprog *program;
static opcode opcodes[MAXOPCODES];
static definition definitions[MAXDEFS];
static jmp_buf abort;

static void processline();
static word parseopnd(int);
static int expression(), primary();
static void define(char*, int);
static void checkfor(int);
static char *expected(int);
static void nextsymb(), identifier();
static bool isidchar(int);
static void number(), getnum(int);
static int hexdigit(int);
static void getdecnum();
static void nextchar();
static void gen(int, int = m_none, word = 0);
static void awarn(char*, word = 0, word = 0);


global void initopcodes()
  { numopcodes = 0;
    until (templates[numopcodes].op == t_end)
      { if (numopcodes >= MAXOPCODES) giveup("bug: too many opcodes!");
	templ *t = &templates[numopcodes];
	opcode *x = &opcodes[numopcodes];
	char *s = topstring(t -> op);
	if (s[0] == '?') giveup("bug: initopcodes");
	x -> s = lookupstring(s);
	x -> op = t -> op;
	x -> m = t -> m;
	numopcodes++;
      }
  }

global tprog *readassembly(char *fn)
  { program = new tprog;
    fname = fn;
    infile = fopen(fname, "r");
    if (infile != NULL)
      { ch = '\n'; lineno = numdefs = 0;
	equid = lookupstring("equ");
	wpid = lookupstring("(wp)");
	nextchar(); nextsymb();
	until (symb == s_eof)
	  { _setjmp(abort);	/* come here if "checkfor" fails */
	    processline();
	  }
	gen(t_end);
	fclose(infile);
      }
    else warn("can't open assembly source file %s", fname);
    return program;
  }

static void processline()
  { top:
    switch (symb)
      { default:
	    awarn("syntax error");
	    break;

	case s_eol:
	    break;

	case s_label:
	  { char *s = item.s;
	    nextsymb();
	    if (symb == s_equ)
	      { nextsymb();
		int n = expression();
		define(s, n);
	      }
	    else
	      { gen(t_label, m_name, s);
		goto top;
	      }
	    break;
	  }

	case s_ident:
	  { char *s = item.s;
	    nextsymb();
	    int n = 0;
	    while (n < numopcodes && opcodes[n].s != s) n++;
	    if (n < numopcodes)
	      { int m = opcodes[n].m;
		word p = parseopnd(m);
		gen(opcodes[n].op, m, p);
	      }
	    else awarn("unknown opcode: %s", s);
	    break;
	  }
      }
    checkfor(s_eol);
  }

static word parseopnd(int m)
  { switch (m)
      { default:
	    giveup("bug! bad addrmode in parseopnd: %d %s", m, addrfmt(m));

	case m_none:
	    return 0;

	case m_num:
	    return expression();

	case m_wksp:
	  { int n = item.n;
	    checkfor(s_number);
	    checkfor(s_wp);
	    return n;
	  }

	case m_name:
	  { char *s = item.s;
	    checkfor(s_ident);
	    return s;
	  }
      }
  }

static int expression()
  { int n = primary();
    while (symb == s_plus || symb == s_minus || symb == s_times)
      { int sy = symb;
	nextsymb();
	int e = primary();
	switch (sy)
	  { case s_plus:    n += e; break;
	    case s_minus:   n -= e; break;
	    case s_times:   n *= e; break;
	  }
      }
    return n;
  }

static int primary()
  { switch (symb)
      { default:
	    awarn("bad start to expression (%d)", symb);
	    return 0;

	case s_number:
	  { int n = item.n;
	    nextsymb();
	    return n;
	  }

	case s_ident:
	  { int k = 0; int n;
	    while (k < numdefs && definitions[k].s != item.s) k++;
	    if (k < numdefs) n = definitions[k].n;
	    else
	      { awarn("undefined identifier: %s", item.s);
		n = 0;
	      }
	    nextsymb();
	    return n;
	  }

	case s_bitnot:
	    nextsymb();
	    return ~primary();

	case s_plus:
	    nextsymb();
	    return +primary();

	case s_minus:
	    nextsymb();
	    return -primary();

	case s_eq:
	  { nextsymb();
	    char *name = item.s;
	    checkfor(s_ident);
	    Class *cl = lookupclass(name);
	    int nw = (cl != NULL) ? (cl -> size) : -1;
	    if (nw < 0) awarn("can't find class ``%s''", name);
	    return nw;
	  }
      }
  }

static void define(char *s, int n)
  { if (numdefs >= MAXDEFS) giveup("too many \"equ\"s in assembly modules!");
    definitions[numdefs].s = s;
    definitions[numdefs].n = n;
    numdefs++;
  }

static void checkfor(int sy)
  { if (symb == sy) nextsymb();
    else
      { awarn("%s expected", expected(sy));
	unless (symb == s_eol || symb == s_eof)
	  { awarn("ignoring rest of line");
	    until (symb == s_eol || symb == s_eof) nextsymb();
	  }
	if (symb == s_eol) nextsymb();
	_longjmp(abort, 1);
      }
  }

static char *expected(int sy)
  { switch (sy)
      { default:	return "???";
	case s_eol:	return "end-of-line";
	case s_wp:	return "(wp)";
	case s_number:	return "number";
	case s_ident:	return "identifier";
      }
  }

static void nextsymb()
  { top:
    switch (ch)
      { default:
	    awarn("ilgl char %c (%03o)", ch, ch & 0xff);
	    nextchar();
	    goto top;

	case '#':
	    do nextchar(); until (ch == '\n' || ch < 0);
	    goto top;

	case ' ':  case '\t':
	    nextchar();
	    goto top;

	case '\n':
	    symb = s_eol;
	    nextchar();
	    break;

	case EOF:
	    symb = s_eof;
	    break;

	case 'A':   case 'B':	case 'C':   case 'D':	case 'E':   case 'F':	case 'G':
	case 'H':   case 'I':	case 'J':   case 'K':	case 'L':   case 'M':	case 'N':
	case 'O':   case 'P':	case 'Q':   case 'R':	case 'S':   case 'T':	case 'U':
	case 'V':   case 'W':	case 'X':   case 'Y':	case 'Z':
	case 'a':   case 'b':	case 'c':   case 'd':	case 'e':   case 'f':	case 'g':
	case 'h':   case 'i':	case 'j':   case 'k':	case 'l':   case 'm':	case 'n':
	case 'o':   case 'p':	case 'q':   case 'r':	case 's':   case 't':	case 'u':
	case 'v':   case 'w':	case 'x':   case 'y':	case 'z':
	case '$':   case '(':	case '_':
	    identifier();
	    break;

	case '0':   case '1':	case '2':   case '3':	case '4':   case '5':	case '6':
	case '7':   case '8':	case '9':
	    number();
	    break;

	case '~':
	    symb = s_bitnot;
	    nextchar();
	    break;

	case '+':
	    symb = s_plus;
	    nextchar();
	    break;

	case '-':
	    symb = s_minus;
	    nextchar();
	    break;

	case '*':
	    symb = s_times;
	    nextchar();
	    break;

	case '=':
	    symb = s_eq;
	    nextchar();
	    break;
      }
  }

static void identifier()
  { char vec[MAXSTRING+1]; int n = 0;
    bool sol = soline;
    while (isidchar(ch) && n < MAXSTRING)
      { vec[n++] = ch;
	nextchar();
      }
    if (isidchar(ch)) awarn("identifier too long");
    vec[n] = '\0';
    item.s = lookupstring(vec);
    symb = (item.s == equid) ? s_equ :
	   (item.s == wpid) ? s_wp :
	   sol ? s_label : s_ident;
  }

static bool isidchar(int c)
  { switch (c)
      { default:
	    return false;

	case 'A':   case 'B':	case 'C':   case 'D':	case 'E':   case 'F':	case 'G':
	case 'H':   case 'I':	case 'J':   case 'K':	case 'L':   case 'M':	case 'N':
	case 'O':   case 'P':	case 'Q':   case 'R':	case 'S':   case 'T':	case 'U':
	case 'V':   case 'W':	case 'X':   case 'Y':	case 'Z':
	case 'a':   case 'b':	case 'c':   case 'd':	case 'e':   case 'f':	case 'g':
	case 'h':   case 'i':	case 'j':   case 'k':	case 'l':   case 'm':	case 'n':
	case 'o':   case 'p':	case 'q':   case 'r':	case 's':   case 't':	case 'u':
	case 'v':   case 'w':	case 'x':   case 'y':	case 'z':
	case '0':   case '1':	case '2':   case '3':	case '4':   case '5':	case '6':
	case '7':   case '8':	case '9':
	case '/':   case ':':	case '(':   case ')':	case '<':   case '>':	case ';':
	case '$':   case '_':
	    return true;
      }
  }

static void number()
  { if (ch == '0')
      { while (ch == '0') nextchar();
	if (ch == 'x') { nextchar(); getnum(4); }
	else getnum(3);
      }
    else getdecnum();
    symb = s_number;
  }

static void getnum(int nb)
  { item.n = 0;
    bool bad = false;
    uint mask = (1 << nb) - 1;
    int d = hexdigit(ch);
    while (d >= 0)
      { if (d > mask) bad = true;
	for (int i = 0; i < nb; i++)
	  { if (item.n < 0) bad = true;
	    item.n <<= 1;
	  }
	item.n |= (d & mask);
	nextchar();
	d = hexdigit(ch);
      }
    if (bad) awarn("bad number");
  }

static int hexdigit(int c)
  { return (c >= '0' && c <= '9') ? c-'0' :
	   (c >= 'a' && c <= 'f') ? (c-'a') + 10 :
	   (c >= 'A' && c <= 'F') ? (c-'A') + 10 :
	   -1;
  }

static void getdecnum()
  { item.n = 0;
    while (ch >= '0' && ch <= '9')
      { item.n = (item.n*10) + (ch-'0');
	nextchar();
      }
  }

static void nextchar()
  { soline = (ch == '\n');
    if (soline) lineno++;
    ch = getc(infile);
  }

static void gen(int op, int m, word p)
  { program -> gen(op, m, p);
  }

static void awarn(char *msg, word p1, word p2)
  { fprintf(stderr, "%s:%d: ", fname, lineno);
    fprintf(stderr, msg, p1, p2); putc('\n', stderr);
    anyerrors = true;
  }

