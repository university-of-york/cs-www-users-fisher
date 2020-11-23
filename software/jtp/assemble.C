/* jtp - Compiler - Java bytecode to Transputer assembly code
   A.J. Fisher	 June 1997 */

#include <stdio.h>
#include "jtp.h"
#include "tcodes.h"

#define MEMSTART	(MINT + 0x70)	/* transputer starts executing from here */
#define MAXNAMES	500
#define NUMPSECTS	3		/* text, data, $end */

#define MAXPASSES	20
#define ALIGNING	100
#define LISTING		200

#define vf_def	1	/* symbol is defined	      */
#define vf_ref	2	/* symbol is referred to      */
#define vf_mdef 4	/* symbol is multiply defined */

struct symbol
  { char *nam;
    int val;
    uchar flg;
    int dpn, dps;	/* pass number & psect number on which defined */
  };

typedef void (*procsy)(symbol*);

static bool needpass;
static symbol names[MAXNAMES];
static symbol *labels;
static int numnames, passnum, psectnum, reduction;
static fprog *program;

static void scannames(procsy), alignsym(symbol*), checksym(symbol*), initlabels();
static char *mklabname(int);
static void dopass(tprog*), dopsect(tprog*, int);
static void geninstr(tinstr*), spaces(int), genoffset(tinstr*), geni(tinstr*), genpadding(int);
static int directop(int), indirectop(int), fpentryop(int), opndvalue(int, int, word);
static void genword(uint), genascii(char*), prefix(int, int), genbyte(uchar);
static symbol *lookupsymbol(int, word);
static void initname(symbol*, char*);


global void assemble(tprog *tp, fprog *fp)
  { program = fp;
    numnames = passnum = 0;
    labels = new symbol[labno];
    initlabels();
    do
      { passnum++;
	dopass(tp);
      }
    while (needpass && passnum < MAXPASSES);
    if (needpass) giveup("Too many passes!");
    scannames(alignsym);
    passnum = ALIGNING;
    dopass(tp);		/* alignment pass */
    if (needpass) warn("bug: need pass after alignment");
    if (options & opt_l)
      { passnum = LISTING;
	dopass(tp);	/* listing pass */
	if (needpass) warn("bug: need pass after listing");
      }
    scannames(checksym);
    delete labels;
  }

static void scannames(procsy p)
  { for (int i = 0; i < numnames; i++) p(&names[i]);
    for (int i = 0; i < labno; i++) p(&labels[i]);
  }

static void alignsym(symbol *x)
  { /* force word alignment for symbols defined in psects 1 & 2 */
    if (x -> dps > 0)
      { // fprintf(stderr, "??? Re-defining `%s' from %08x to %08x\n", x -> nam, x -> val, (x -> val) & ~3);
	x -> val &= ~3;
      }
  }

static void checksym(symbol *x)
  { uint f = (x -> flg) & (vf_ref | vf_def);
    if (f == vf_ref) warn("not defined: `%s'", x -> nam);
    if ((f == vf_def) && (options & opt_u)) warn("not used: `%s'", x -> nam);
  }

static void initlabels()
  { for (int n = 0; n < labno; n++)
      { symbol *x = &labels[n];
	initname(x, mklabname(n+1));
      }
  }

static char *mklabname(int n)
  { /* used only for printing error msgs.
       don't call lookupstring, to avoid polluting dict space */
    char *name = new char[8];
    sprintf(name, "L%d", n);
    return name;
  }

static void dopass(tprog *tp)
  { program -> clen = 0;
    reduction = 0;
    needpass = false;
    for (int ps = 0; ps < NUMPSECTS; ps++) dopsect(tp, ps);
    // fprintf(stderr, "??? Pass %3d, size=%6d, reduction=%6d\n", passnum, program -> clen, reduction);
  }

static void dopsect(tprog *tp, int ps)
  { psectnum = ps;
    int thisps = 0;
    for (int i = 0; i < (tp -> clen); i++)
      { tinstr *ti = tp -> code[i];
	if (ti -> op == t_psect) thisps = ti -> p1.n;
	else if (psectnum == thisps) geninstr(tp -> code[i]);
      }
    if (psectnum == 0)
      { if (passnum >= ALIGNING)
	  { while ((MEMSTART + program -> clen) & 3) genbyte(0);
	  }
	else
	  { for (int i = 0; i < 3; i++) genbyte(0);
	  }
      }
  }

static void geninstr(tinstr *ti)
  { int loc = program -> clen;
    if (passnum >= LISTING) printf("%08x: ", MEMSTART+loc);
    geni(ti);
    if (passnum >= LISTING)
      { int nb = (program -> clen) - loc;
	if (nb <= 16)
	  { for (int i = 0; i < nb; i++) printf("%02x", program -> code[loc+i]);
	    spaces(38 - 2*nb);
	  }
	else
	  { for (int i = 0; i < 16; i++) printf("%02x", program -> code[loc+i]);
	    printf(" ...  ");
	  }
	tlist(ti);
	if (ti -> op == t_label || ti -> op == t_end) putchar('\n');
      }
  }

static void spaces(int nsp)
  { for (int i = 0; i < nsp; i++) putchar(' ');
  }

static void geni(tinstr *ti)
  { switch (ti -> op)
      { default:
	    giveup("unimplemented: 8 %s", topstring(ti -> op));

	case t_label:	case t_entry:
	  { symbol *x = lookupsymbol(ti -> m1, ti -> p1);
	    unless (x -> flg & vf_mdef)
	      { int loc = MEMSTART + (program -> clen);	    /* loc ctr */
		unless (x -> flg & vf_def) x -> val = ~loc;
		if (x -> val != loc)
		  { // fprintf(stderr, "??? `%s' changed from %08x to %08x\n", x -> nam, x -> val, loc);
		    needpass = true;   /* newly defined, or value of label changed */
		  }
		if ((passnum == 1) && (x -> flg & vf_def))
		  { warn("multiply defined: `%s'", x -> nam);
		    x -> flg |= vf_mdef;
		  }
		x -> val = loc;
		x -> flg |= vf_def;
	      }
	    x -> dpn = passnum;
	    x -> dps = psectnum;
	    break;
	  }

	case t_end:
	    break;

	case t_word:
	  { if (ti -> m1 == m_float) genword(ti -> p1.w[0].n);
	    else if (ti -> m1 == m_double)
	      { genword(ti -> p1.w[1].n);	/* N.B. order! */
		genword(ti -> p1.w[0].n);
	      }
	    else
	      { int n = opndvalue(t_word, ti -> m1, ti -> p1);
		genword(n);
	      }
	    break;
	  }

	case t_blkb:
	  { int n = opndvalue(t_blkb, ti -> m1, ti -> p1);
	    for (int i = 0; i < n; i++) genbyte(0);
	    break;
	  }

	case t_ascii:
	    genascii(ti -> p1.s);
	    break;

	case t_ldnl:	case t_ldnlp:	case t_ldc:	case t_ldl:	case t_ldlp:	case t_adc:
	case t_ajw:	case t_eqc:	case t_stl:	case t_stnl:
	  { int op = directop(ti -> op);
	    if (op < 0) giveup("unimplemented: 8d %s", topstring(ti -> op));
	    int n = opndvalue(ti -> op, ti -> m1, ti -> p1);
	    prefix(op, n);
	    break;
	  }

	case t_j:	case t_cj:	case t_call:	case t_ldpi:	case t_lend:
	    genoffset(ti);
	    break;

	case t_and:	    case t_diff:	case t_div:	    case t_gt:		case t_mint:	    case t_not:
	case t_or:	    case t_prod:	case t_rem:	    case t_ret:		case t_rev:	    case t_gajw:
	case t_shl:	    case t_shr:		case t_sum:	    case t_xor:		case t_sthf:	    case t_stlf:
	case t_sttimer:	    case t_clrhalterr:	case t_sethalterr:  case t_testerr:	case t_seterr:	    case t_dup:
	case t_gcall:	    case t_lb:		case t_sb:	    case t_in:		case t_out:	    case t_outbyte:
	case t_startp:	    case t_runp:	case t_stopp:	    case t_wsub:	case t_wsubdb:	    case t_wcnt:
	case t_bitcnt:	    case t_fpldzerosn:	case t_fpldzerodb:  case t_fpldnlsn:	case t_fpldnldb:    case t_fpstnlsn:
	case t_fpstnldb:    case t_fpadd:	case t_fpsub:	    case t_fpmul:	case t_fprev:
	case t_fpdiv:	    case t_fpint:	case t_fpstnli32:   case t_fpi32tor32:	case t_fpi32tor64:
	case t_fpeq:	    case t_fpgt:	case t_fpremfirst:  case t_fpremstep:
	  { int op = indirectop(ti -> op);
	    if (op < 0) giveup("unimplemented: 8i %s", topstring(ti -> op));
	    prefix(15, op);	/* opr */
	    break;
	  }

	case t_fpuabs:	    case t_fpur32tor64: case t_fpur64tor32: case t_fpurz:
	case t_fpusqrtfirst:case t_fpusqrtstep: case t_fpusqrtlast:
	  { int op = fpentryop(ti -> op);
	    if (op < 0) giveup("unimplemented: 8f %s", topstring(ti -> op));
	    prefix(4, op);	/* ldc */
	    prefix(15, 0xab);	/* fpentry */
	    break;
	  }
      }
  }

static void genoffset(tinstr *ti)
  { if (passnum == 1)
      { /* assume worst case to start with */
	genpadding(10);
	ti -> jlen = 10;
	needpass = true;
      }
    else
      { int dest = opndvalue(ti -> op, ti -> m1, ti -> p1);
	int loc1 = program -> clen;		    /* loc ctr of jump instr */
	int e = loc1 + (ti -> jlen);		    /* estimate of loc ctr after instr */
	int opnd = dest - (MEMSTART+e);
	switch (ti -> op)
	  { default:
	      { int op = directop(ti -> op);
		if (op < 0) giveup("unimplemented: 8d %s", topstring(ti -> op));
		prefix(op, opnd);
		break;
	      }

	    case t_ldpi:
		prefix(4, opnd);		    /* ldc */
		prefix(15, 0x1b);		    /* ldpi */
		break;

	    case t_lend:
		prefix(4, -opnd);		    /* ldc : note arg is negated! */
		prefix(15, 0x21);		    /* ldpi */
		break;
	  }
	int loc2 = program -> clen;		    /* loc ctr after jump instr */
	if (loc2 != e)
	  { /* estimate was wrong : subtle */
	    int adj = loc2-e;
	    // fprintf(stderr, "??? %s at %08x estimate wrong : %+d\n", topstring(ti -> op), MEMSTART+loc1, adj);
	    if (adj > 0)
	      { /* prog has got bigger because of "optimization" */
		giveup("bug: %s at %08x: pessimization (%+d)", topstring(ti -> op), MEMSTART+loc1, adj);
	      }
	    if (passnum >= ALIGNING)
	      { unless (ti -> op == t_ldpi)
		  giveup("ilgl cross-psect reference: %s at %08x", topstring(ti -> op), MEMSTART+loc1);
		genpadding(-adj);
		// fprintf(stderr, "??? padding cross-psect ldpi: %d bytes at %08x\n", -adj, MEMSTART+loc1);
	      }
	    else
	      { ti -> jlen += adj;
		reduction -= adj;
		needpass = true;
	      }
	  }
      }
  }

static void genpadding(int nb)
  { for (int i = 0; i < nb; i++) genbyte(0x20);	    /* pfix 0 */
  }

static int directop(int op)
  { switch (op)
      { default:	return -1;
	case t_adc:	return 8;
	case t_ajw:	return 11;
	case t_call:	return 9;
	case t_cj:	return 10;
	case t_eqc:	return 12;
	case t_j:	return 0;
	case t_ldc:	return 4;
	case t_ldl:	return 7;
	case t_ldlp:	return 1;
	case t_ldnl:	return 3;
	case t_ldnlp:	return 5;
	case t_stl:	return 13;
	case t_stnl:	return 14;
      }
  }

static int indirectop(int op)
  { switch (op)
      { default:	    return -1;
	case t_and:	    return 0x46;
	case t_bitcnt:	    return 0x76;
	case t_clrhalterr:  return 0x57;
	case t_sethalterr:  return 0x58;
	case t_diff:	    return 0x04;
	case t_div:	    return 0x2c;
	case t_dup:	    return 0x5a;
	case t_fpadd:	    return 0x87;
	case t_fpdiv:	    return 0x8c;
	case t_fpeq:	    return 0x95;
	case t_fpgt:	    return 0x94;
	case t_fpi32tor32:  return 0x96;
	case t_fpi32tor64:  return 0x98;
	case t_fpint:	    return 0xa1;
	case t_fpldnldb:    return 0x8a;
	case t_fpldnlsn:    return 0x8e;
	case t_fpldzerodb:  return 0xa0;
	case t_fpldzerosn:  return 0x9f;
	case t_fpmul:	    return 0x8b;
	case t_fpremfirst:  return 0x8f;
	case t_fpremstep:   return 0x90;
	case t_fprev:	    return 0xa4;
	case t_fpstnldb:    return 0x84;
	case t_fpstnli32:   return 0x9e;
	case t_fpstnlsn:    return 0x88;
	case t_fpsub:	    return 0x89;
	case t_gajw:	    return 0x3c;
	case t_gcall:	    return 0x06;
	case t_gt:	    return 0x09;
	case t_in:	    return 0x07;
	case t_lb:	    return 0x01;
	case t_mint:	    return 0x42;
	case t_not:	    return 0x32;
	case t_or:	    return 0x4b;
	case t_out:	    return 0x0b;
	case t_outbyte:	    return 0x0e;
	case t_prod:	    return 0x08;
	case t_rem:	    return 0x1f;
	case t_ret:	    return 0x20;
	case t_rev:	    return 0x00;
	case t_runp:	    return 0x39;
	case t_sb:	    return 0x3b;
	case t_seterr:	    return 0x10;
	case t_shl:	    return 0x41;
	case t_shr:	    return 0x40;
	case t_startp:	    return 0x0d;
	case t_sthf:	    return 0x18;
	case t_stlf:	    return 0x1c;
	case t_stopp:	    return 0x15;
	case t_sttimer:	    return 0x54;
	case t_sum:	    return 0x52;
	case t_testerr:	    return 0x29;
	case t_wcnt:	    return 0x3f;
	case t_wsub:	    return 0x0a;
	case t_wsubdb:	    return 0x81;
	case t_xor:	    return 0x33;
      }
  }

static int fpentryop(int op)
  { switch (op)
      { default:	    return -1;
	case t_fpuabs:	    return 0x0b;
	case t_fpur32tor64: return 0x07;
	case t_fpur64tor32: return 0x08;
	case t_fpurz:	    return 0x06;
	case t_fpusqrtfirst:return 0x01;
	case t_fpusqrtstep: return 0x02;
	case t_fpusqrtlast: return 0x03;
      }
  }

static int opndvalue(int op, int m, word p)
  { switch (m)
      { default:
	    giveup("bad addrmode: %d %s op=%s", m, addrfmt(m), topstring(op));

	case m_num:	case m_wksp:
	    return p.n;

	case m_lab:	case m_name:
	  { symbol *x = lookupsymbol(m, p);
	    x -> flg |= vf_ref;
	    int val = x -> val;
	    if (x -> dpn < passnum)
	      { /* fwd ref to label which will be affected by optimization */
		val -= reduction;
	      }
	    return val;
	  }
      }
  }

static void genword(uint n)
  { for (int i = 0; i < 4; i++)
      { genbyte(n & 0xff);
	n >>= 8;
      }
  }

static void genascii(char *s)
  { int k = 0;
    until (s[k] == '\0') genbyte(s[k++] & 0xff);
    genbyte(0); k++;				    /* generate terminating null char so "equals" works */
    while (k & 3) { genbyte(0); k++; }		    /* maintain word alignment */
  }

static void prefix(int op, int e)
  { if (e >= 16) prefix(2, e >> 4);	/* pfix */
    if (e < 0) prefix(6, ~(e >> 4));	/* nfix */
    genbyte((op << 4) | (e & 0xf));
  }

static void genbyte(uchar n)
  { program -> gen(n);
  }

static symbol *lookupsymbol(int m, word p)
  { switch (m)
      { default:
	    giveup("bug: lookupsymbol %s", addrfmt(m));

	case m_lab:
	    if (p.n < 1 || p.n > labno) giveup("label num out of range: L%d", p.n);
	    return &labels[p.n-1];

	case m_name:
	  { int n = 0;
	    until (n >= numnames || names[n].nam == p.s) n++;
	    if (n >= numnames)
	      { if (numnames >= MAXNAMES) giveup("too many names!");
		initname(&names[numnames++], p.s);
	      }
	    return &names[n];
	  }
      }
  }

static void initname(symbol *x, char *name)
  { x -> nam = name;
    x -> flg = 0;
    x -> val = MEMSTART;
    x -> dpn = 0;
    x -> dps = 0;
  }

