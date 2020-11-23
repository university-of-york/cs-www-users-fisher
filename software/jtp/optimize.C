/* jtp - Compiler - Java bytecode to Transputer assembly code
   A.J. Fisher	 June 1997 */

#include <stdio.h>
#include "jtp.h"
#include "tcodes.h"

struct tnode
  { tnode(tinstr *ti, tnode *x = NULL, tnode *y = NULL, tnode *z = NULL)
      { this -> ti = ti;
	this -> h2 = x;
	this -> h3 = y;
	this -> h4 = z;
	this -> slot = -1;
	this -> duped = false;
      }
    tinstr *ti;
    tnode *h2, *h3, *h4;
    int slot; bool duped;
  };

enum atype { ty_int, ty_float, ty_double };

struct actual
  { tnode *x;
    int addr;
    atype ty;
  };

static tinstr **incode;
static tnode *estack;
static int inptr, locals, maxlocals, maxactuals, numfps;
static tinstr *entryptr, *ti_seq, *ti_cmpnd, *ti_comma, *ti_cexpr;
static tprog *program;

static tnode *getprog(), *getstmt(), *getexpr(), *getsexpr(), *getactuals(int);
static void putback(tnode*), checkfor(int);
static void genblock(tnode*), genstmt(tnode*);
static void loadexpr(tnode*), xload(tnode*), transcmpnd(tnode*, tnode*);
static void transcexpr(tnode*, tnode*, tnode*, int, atype), transbranch(tnode*, int, atype);
static void transcall(tnode*), loadactuals(tnode*, int);
static void enumerate(actual*, tnode*, int&, int);
static void putinslot(tnode*, atype);
static tnode *loadfromslot(int);
static void loadpair(tnode*, tnode*, bool), ldusingtemp(tnode*, tnode*);
static int exprdepth(tnode*), pairdepth(tnode*, tnode*);
static atype exprtype(tnode*);
static int alloctemp(atype);
static void loadfrom(int, word, atype), storein(int, word, atype);
static void gen(int, int = m_none, word = 0);


global void optimize(tprog *tp1, tprog *tp2)
  { incode = tp1 -> code;
    inptr = tp1 -> clen;
    program = tp2;
    tnode *x = getprog();
    locals = maxlocals = maxactuals = -1;
    entryptr = NULL;
    genblock(x);
  }

static tnode *getprog()
  { ti_seq = new tinstr(t_seq);
    ti_cmpnd = new tinstr(t_cmpnd);
    ti_comma = new tinstr(t_comma);
    ti_cexpr = new tinstr(t_cexpr);
    estack = NULL;
    tnode *x = getstmt();
    while (inptr > 0)
      { tnode *y = getstmt();
	x = new tnode(ti_seq, y, x);
      }
    if (estack != NULL) giveup("bug: estack not empty after block");
    return x;
  }

static tnode *getstmt()
  { tinstr *ti = incode[--inptr];
    switch (ti -> op)
      { default:
	    giveup("unimplemented: 3 %s", topstring(ti -> op));

	case t_j:	    case t_label:	case t_entry:	    case t_end:
	case t_retv:	    case t_seterr:
	    return new tnode(ti);

	case t_stl:	    case t_cj:		case t_retnv:	    case t_pop:
	  { tnode *x = getexpr();
	    return new tnode(ti, x);
	    break;
	  }

	case t_stnl:	    case t_fpstnlsn:	case t_fpstnldb:    case t_sb:
	  { tnode *y = getexpr();
	    tnode *x = getexpr();
	    return new tnode(ti, x, y);
	  }

	case t_callv:
	  { tnode *x = getactuals(ti -> p2.n);
	    return new tnode(ti, x);
	  }
      }
  }

static tnode *getexpr()
  {
top:
    if (estack != NULL)
      { tnode *x = estack;
	estack = estack -> h2;
	return x -> h3;
      }
    tinstr *ti = incode[--inptr];
    switch (ti -> op)
      { default:
	    giveup("unimplemented: 4 %s", topstring(ti -> op));

	case t_rev:
	  { tnode *y = getexpr();
	    tnode *x = getexpr();
	    putback(y); putback(x);
	    goto top;
	  }

	case t_rot:
	  { tnode *z = getexpr();
	    tnode *y = getexpr();
	    tnode *x = getexpr();
	    putback(y); putback(z); putback(x);
	    goto top;
	  }

	case t_ldc:	    case t_ldl:		case t_ldpi:	    case t_ldlp:	case t_mint:
	case t_fpldzerosn:  case t_fpldzerodb:
	    return new tnode(ti);

	case t_dup:
	  { tnode *x = getsexpr(); x -> duped = true;
	    putback(x);
	    return x;
	  }

	case t_dup2:
	  { tnode *y = getexpr(); y -> duped = true;
	    if (exprtype(y) == ty_double) putback(y);
	    else
	      { tnode *x = getsexpr(); x -> duped = true;
		putback(x); putback(y); putback(x);
	      }
	    return y;
	  }

	case t_dupx1:
	  { tnode *y = getsexpr(); y -> duped = true;
	    tnode *x = getsexpr(); x -> duped = true;
	    putback(y); putback(x);
	    return y;
	  }

	case t_dupx2:
	  { tnode *z = getsexpr(); z -> duped = true;
	    tnode *y = getsexpr(); y -> duped = true;
	    tnode *x = getsexpr(); x -> duped = true;
	    putback(z); putback(x); putback(y);
	    return z;
	  }

	case t_dup2x2:
	  { tnode *z = getexpr(); z -> duped = true;
	    tnode *y = getexpr(); y -> duped = true;
	    atype zty = exprtype(z), yty = exprtype(y);
	    if (zty == ty_double && yty == ty_double)
	      { putback(z); putback(y);
	      }
	    else
	      { tnode *x = getsexpr(); x -> duped = true;
		if (zty == ty_double)
		  { putback(z); putback(x); putback(y);
		  }
		else warn("bug: bad combination of types in dup2x2");
	      }
	    return z;
	  }

	case t_ldnl:	    case t_ldnlp:	case t_eqc:	    case t_adc:		case t_fpldnlsn:    case t_fpldnldb:
	case t_fpcvtfi:	    case t_fpcvtif:	case t_fpcvtdi:	    case t_fpcvtid:	case t_fpur32tor64: case t_fpur64tor32:
	case t_lb:	    case t_case:	case t_not:
	  { tnode *x = getexpr();
	    return new tnode(ti, x);
	  }

	case t_sum:	    case t_diff:	case t_prod:	    case t_div:		case t_rem:
	case t_and:	    case t_or:		case t_xor:	    case t_shl:		case t_shr:
	case t_gt:	    case t_wsub:	case t_wsubdb:
	case t_fpadd:	    case t_fpsub:	case t_fpmul:	    case t_fpdiv:	case t_fprem:
	case t_fpeq:	    case t_fpgt:
	  { tnode *y = getexpr();
	    tnode *x = getexpr();
	    return new tnode(ti, x, y);
	  }

	case t_calli:	    case t_callf:	case t_calld:
	  { tnode *x = getactuals(ti -> p2.n);
	    return new tnode(ti, x);
	  }

	case t_callv:	    case t_stl:		case t_stnl:	    case t_fpstnlsn:	case t_fpstnldb:
	case t_sb:	    case t_pop:
	  { inptr++;	/* back up */
	    tnode *y = getstmt();
	    tnode *x = getexpr();
	    return new tnode(ti_cmpnd, x, y);
	  }

	case t_label:
	  { /* conditional expression, possibly "cascaded" (a ? b : c ? ... ) */
	    int lab = ti -> p1.n;
	    tnode *z = getexpr();
	    while (inptr >= 2 && incode[inptr-2] -> op == t_j && incode[inptr-2] -> p1.n == lab)
	      { checkfor(t_label); checkfor(t_j);
		tnode *y = getexpr();
		checkfor(t_cj);
		tnode *x = getexpr();
		z = new tnode(ti_cexpr, x, y, z);
	      }
	    return z;
	  }
      }
  }

static tnode *getsexpr()
  { tnode *x = getexpr();
    if (exprtype(x) == ty_double) warn("bug: got double in dup, expected single");
    return x;
  }

static tnode *getactuals(int nwds)
  { tnode *x = NULL;
    int n = 0;
    while (n < nwds)
      { tnode *ap = getexpr();
	x = (x == NULL) ? ap : new tnode(ti_comma, ap, x);
	n += (exprtype(ap) == ty_double) ? 2 : 1;
      }
    return x;
  }

static void putback(tnode *x)
  { /* put x on stack, to be returned by next call of getexpr */
    estack = new tnode(ti_comma, estack, x);
  }

static void checkfor(int op)
  { unless (incode[--inptr] -> op == op) giveup("bug: %s exp in cond expr", topstring(op));
  }

static void genblock(tnode *x)
  { while (x -> ti -> op == t_seq)
      { genblock(x -> h2);
	x = x -> h3;
      }
    genstmt(x);
  }

static void genstmt(tnode *x)
  { tinstr *ti = x -> ti;
    switch (ti -> op)
      { default:
	    giveup("unimplemented: 5b %s", topstring(ti -> op));

	case t_entry:
	    locals = maxlocals = ti -> lvi -> locs;
	    numfps = ti -> lvi -> fps;
	    maxactuals = 0;
	    gen(t_entry, ti -> m1, ti -> p1);
	    entryptr = program -> last();	/* ptr to generated t_entry */
	    gen(t_ajw, m_num, -1);		/* adjusted by peephole opt */
	    break;

	case t_end:
	    entryptr -> lvi = new lvinfo(numfps, maxlocals, maxactuals);	/* update the entry tinstr */
	    gen(t_end);
	    break;

	case t_j:	    case t_label:	case t_seterr:
	    gen(ti -> op, ti -> m1, ti -> p1);
	    break;

	case t_retv:
	    gen(t_ajw, m_num, +1);		/* adjusted by peephole opt */
	    gen(t_ret);
	    break;

	case t_retnv:
	  { int locs = locals;
	    loadexpr(x -> h2);
	    gen(t_ajw, m_num, +1);		/* adjusted by peephole opt */
	    gen(t_ret);
	    locals = locs;
	    break;
	  }

	case t_stl:	case t_cj:
	  { int locs = locals;
	    loadexpr(x -> h2);
	    gen(ti -> op, ti -> m1, ti -> p1);
	    locals = locs;
	    break;
	  }

	case t_pop:
	    if (exprdepth(x -> h2) > 1)
	      { int locs = locals;
		loadexpr(x -> h2);
		locals = locs;
	      }
	    break;

	case t_stnl:	    case t_fpstnlsn:	case t_fpstnldb:    case t_sb:
	  { int locs = locals;
	    loadpair(x -> h2, x -> h3, false);
	    gen(ti -> op, ti -> m1, ti -> p1);
	    locals = locs;
	    break;
	  }

	case t_callv:
	    transcall(x);
	    break;
      }
  }

static void loadexpr(tnode *x)
  { if (x -> slot >= 0)
      { atype ty = exprtype(x);
	loadfrom(m_loc, x -> slot, ty);
      }
    else
      { xload(x);
	if (x -> duped)
	  { atype ty = exprtype(x);
	    x -> slot = alloctemp(ty);
	    storein(m_loc, x -> slot, ty);
	    loadfrom(m_loc, x -> slot, ty);
	  }
      }
  }

static void xload(tnode *x)
  { tinstr *ti = x -> ti;
    switch (ti -> op)
      { default:
	    giveup("unimplemented: 5e %s", topstring(ti -> op));

	case t_ldc:	    case t_ldl:		case t_ldpi:	    case t_ldlp:	case t_mint:
	case t_fpldzerosn:  case t_fpldzerodb:
	    gen(ti -> op, ti -> m1, ti -> p1);
	    break;

	case t_ldnl:	    case t_ldnlp:	case t_eqc:	    case t_adc:		case t_lb:	    case t_not:
	case t_fpldnlsn:    case t_fpldnldb:	case t_fpur32tor64: case t_fpur64tor32:
	    loadexpr(x -> h2);
	    gen(ti -> op, ti -> m1, ti -> p1);
	    break;

	case t_case:
	    loadexpr(x -> h2);
	    gen(t_dup);
	    gen(t_eqc, ti -> m1, ti -> p1);
	    gen(t_eqc, m_num, 0);
	    gen(t_cj, ti -> m2, ti -> p2);	/* jumps or pops */
	    break;

	case t_sum:	case t_prod:	case t_and:	case t_or:	case t_xor:
	case t_fpadd:	case t_fpmul:	case t_fpeq:
	    /* commutative ops */
	    loadpair(x -> h2, x -> h3, true);
	    gen(ti -> op);
	    break;

	case t_diff:	case t_div:	case t_rem:	case t_shl:	case t_shr:	case t_gt:
	case t_fpsub:	case t_fpdiv:	case t_fpgt:
	    /* non-commutative ops */
	    loadpair(x -> h2, x -> h3, false);
	    gen(ti -> op);
	    break;

	case t_fpcvtfi: case t_fpcvtdi:
	  { loadexpr(x -> h2);
	    int locs = locals;
	    int temp = alloctemp(ty_int);
	    gen(t_fpurz); gen(t_fpint);	    /* round towards zero */
	    gen(t_ldlp, m_loc, temp);
	    gen(t_fpstnli32);
	    loadfrom(m_loc, temp, ty_int);
	    locals = locs;
	    break;
	  }

	case t_fpcvtif: case t_fpcvtid:
	  { loadexpr(x -> h2);
	    int locs = locals;
	    int temp = alloctemp(ty_int);
	    storein(m_loc, temp, ty_int);
	    gen(t_ldlp, m_loc, temp);
	    gen((ti -> op == t_fpcvtid) ? t_fpi32tor64 : t_fpi32tor32);
	    locals = locs;
	    break;
	  }

	case t_fprem:
	  { int l1 = ++labno, l2 = ++labno;
	    loadpair(x -> h2, x -> h3, false);
	    gen(t_fpremfirst);
	    gen(t_eqc, m_num, 0);
	    gen(t_cj, m_lab, l2);
	    gen(t_label, m_lab, l1);
	    gen(t_fpremstep);
	    gen(t_cj, m_lab, l1);
	    gen(t_label, m_lab, l2);
	    break;
	  }

	case t_wsub:	case t_wsubdb:
	  { tnode *y = x -> h2, *z = x -> h3;
	    if (y -> ti -> op == t_ldc && y -> ti -> m1 == m_num)
	      { loadexpr(z);	/* array */
		int n = y -> ti -> p1.n;
		if (ti -> op == t_wsubdb) n *= 2;
		gen(t_ldnlp, m_num, n);
	      }
	    else
	      { loadpair(y, z, false);
		gen(ti -> op);
	      }
	    break;
	  }

	case t_calli:	case t_callf:	case t_calld:
	    transcall(x);
	    break;

	case t_cmpnd:	/* compound expr */
	  { tnode *e = x -> h2, *b = x -> h3;
	    transcmpnd(e, b);
	    loadfrom(m_loc, e -> slot, exprtype(e));
	    break;
	  }

	case t_cexpr:	/* conditional expr */
	  { atype ty = exprtype(x);
	    int temp = alloctemp(ty);
	    transcexpr(x -> h2, x -> h3, x -> h4, temp, ty);
	    loadfrom(m_loc, temp, ty);
	    break;
	  }
      }
  }

static void transcmpnd(tnode *e, tnode *b)
  { /* evaluate "e", store in slot; execute "b" */
    if (e -> slot < 0)
      { if (e -> ti -> op == t_cmpnd)
	  { transcmpnd(e -> h2, e -> h3);
	    e -> slot = e -> h2 -> slot;
	  }
	else
	  { atype ty = exprtype(e);
	    xload(e);
	    e -> slot = alloctemp(ty);
	    storein(m_loc, e -> slot, ty);
	  }
      }
    genblock(b);
  }

static void transcexpr(tnode *x, tnode *y, tnode *z, int temp, atype ty)
  { /* we have to use a slot because "j" might time-slice, which loses the ABC stack */
    int l1 = ++labno, l2 = ++labno;
    loadexpr(x);
    gen(t_cj, m_lab, l1);
    transbranch(y, temp, ty);
    gen(t_j, m_lab, l2);
    gen(t_label, m_lab, l1);
    transbranch(z, temp, ty);
    gen(t_label, m_lab, l2);
  }

static void transbranch(tnode *x, int temp, atype ty)
  { if (x -> ti -> op == t_cexpr) transcexpr(x -> h2, x -> h3, x -> h4, temp, ty);
    else
      { loadexpr(x);
	storein(m_loc, temp, ty);
      }
  }

static void transcall(tnode *x)
  { int locs = locals;
    tinstr *ti = x -> ti;
    switch (ti -> m1)
      { default:
	    warn("bug: %d in transcall", ti -> m1);
	    break;

	case m_name:
	    /* non-virtual call */
	    loadactuals(x -> h2, ti -> p2.n);
	    gen(t_call, m_name, ti -> p1.s);
	    break;

	case m_num:
	  { /* virtual call */
	    tnode *args = x -> h2;
	    tnode *farg = args;
	    while (farg != NULL && farg -> ti -> op == t_comma) farg = farg -> h2;   /* get first arg (object) */
	    if (farg != NULL)
	      { farg = new tnode(new tinstr(t_ldnl, m_num, -2), farg);
		farg = new tnode(new tinstr(t_ldnl, m_num, ti -> p1.n), farg);
		args = new tnode(ti_comma, args, farg);
	      }
	    loadactuals(args, ti -> p2.n + 1);
	    char buf[MAXSTRING+1]; sprintf(buf, "$gcall_%d", ti -> p2.n);
	    gen(t_call, m_name, lookupstring(buf));
	    break;
	  }
      }
    locals = locs;
  }

static void loadactuals(tnode *x, int nwds)
  { actual *vec1 = new actual[nwds];
    int naps = 0;
    enumerate(vec1, x, naps, nwds);
    int a = 0;
    for (int i = 0; i < naps; i++)
      { actual *act = &vec1[i];
	act -> ty = exprtype(act -> x);
	act -> addr = a;
	a += (act -> ty == ty_double) ? 2 : 1;
      }
    unless (a == nwds) giveup("bug: loadactuals");
    if (a-3 > maxactuals) maxactuals = a-3;
    /* early fp actuals are passed in integer ABC regs */
    actual *vec2 = new actual[nwds];
    int k = 0;
    for (int i = 0; i < naps; i++)
      { actual *act = &vec1[i];
	if (act -> addr <= 2 && act -> ty == ty_double)
	  { putinslot(act -> x, ty_double);
	    vec2[k].x = loadfromslot(act -> x -> slot);
	    vec2[k].ty = ty_int;
	    vec2[k].addr = act -> addr;
	    vec2[k+1].x = loadfromslot(act -> x -> slot - 1);
	    vec2[k+1].ty = ty_int;
	    vec2[k+1].addr = (act -> addr) + 1;
	    k += 2;
	  }
	else if (act -> addr <= 2 && act -> ty == ty_float)
	  { putinslot(act -> x, ty_float);
	    vec2[k].x = loadfromslot(act -> x -> slot);
	    vec2[k].ty = ty_int;
	    vec2[k].addr = act -> addr;
	    k++;
	  }
	else vec2[k++] = *act;	/* copy the struct */
      }
    naps = k;
    /* params after 3rd go on the stack */
    while (naps > 3)
      { actual *act = &vec2[--naps];
	loadexpr(act -> x);
	storein(m_wksp, (act -> addr) - 3, act -> ty);
      }
    /* first 3 params go in integer ABC regs */
    switch (naps)
      { case 1:
	    loadexpr(vec2[0].x);
	    break;

	case 2:
	    loadpair(vec2[1].x, vec2[0].x, false);
	    break;

	case 3:
	    if (exprdepth(vec2[0].x) > 1)   /* implies not stored in slot */
	      putinslot(vec2[0].x, ty_int);
	    loadpair(vec2[2].x, vec2[1].x, false);
	    loadexpr(vec2[0].x);
	    break;
      }
    delete vec1; delete vec2;
  }

static void enumerate(actual *vec, tnode *x, int &ptr, int max)
  { unless (x == NULL)
      { while (x -> ti -> op == t_comma)
	  { enumerate(vec, x -> h2, ptr, max);
	    x = x -> h3;
	  }
	if (ptr >= max) giveup("bug: enumerate");
	vec[ptr++].x = x;
      }
  }

static void putinslot(tnode *x, atype ty)
  { if (x -> slot < 0)
      { x -> slot = alloctemp(ty);
	xload(x);
	storein(m_loc, x -> slot, ty);
      }
  }

static tnode *loadfromslot(int slot)
  { tinstr *ti = new tinstr(t_ldl, m_loc, slot);
    return new tnode(ti);
  }

static void loadpair(tnode *x1, tnode *x2, bool com)
  { int d1 = exprdepth(x1), d2 = exprdepth(x2);
    if (d1 > 2 && d2 > 2) ldusingtemp(x1, x2);
    else if (d2 > d1)
      { if (com) loadpair(x2, x1, true);
	else
	  { atype ty1 = exprtype(x1), ty2 = exprtype(x2);
	    if (ty1 != ty2) ldusingtemp(x1, x2);
	    else
	      { loadpair(x2, x1, false);
		gen((ty1 == ty_float || ty1 == ty_double) ? t_fprev : t_rev);
	      }
	  }
      }
    else
      { loadexpr(x1);
	loadexpr(x2);
      }
  }

static void ldusingtemp(tnode *x1, tnode *x2)
  { atype ty2 = exprtype(x2);
    xload(x2);
    x2 -> slot = alloctemp(ty2);	/* we know it's not already in a slot */
    storein(m_loc, x2 -> slot, ty2);
    loadexpr(x1); loadexpr(x2);
  }

static int exprdepth(tnode *x)
  { if (x -> slot >= 0) return 1;
    tinstr *ti = x -> ti;
    switch (ti -> op)
      { default:
	    giveup("unimplemented: 6 %s", topstring(ti -> op));

	case t_ldc:	    case t_ldl:		case t_ldpi:	    case t_ldlp:	case t_mint:
	case t_fpldzerosn:  case t_fpldzerodb:
	    return 1;

	case t_sum:	    case t_diff:	case t_prod:	    case t_div:		case t_rem:
	case t_and:	    case t_or:		case t_xor:	    case t_shl:		case t_shr:
	case t_gt:
	case t_fpadd:	    case t_fpsub:	case t_fpmul:	    case t_fpdiv:
	case t_fpeq:	    case t_fpgt:
	    return pairdepth(x -> h2, x -> h3);

	case t_wsub:	    case t_wsubdb:
	  { tnode *y = x -> h2, *z = x -> h3;
	    return (y -> ti -> op == t_ldc && y -> ti -> m1 == m_num) ? exprdepth(z) : pairdepth(y, z);
	  }

	case t_not:	    case t_adc:		case t_eqc:	    case t_ldnl:	case t_ldnlp:
	case t_fpldnlsn:    case t_fpldnldb:
	case t_fpcvtfi:	    case t_fpcvtif:	case t_fpcvtdi:	    case t_fpcvtid:	case t_fpur32tor64: case t_fpur64tor32:
	case t_lb:
	    return exprdepth(x -> h2);

	case t_case:
	  { int d = exprdepth(x -> h2);
	    return (d > 2) ? d : 2;
	  }

	case t_cexpr:	    case t_cmpnd:	case t_calli:	    case t_callf:	case t_calld:	    case t_fprem:
	    return 3;
      }
  }

static int pairdepth(tnode *x1, tnode *x2)
  { int d1 = exprdepth(x1), d2 = exprdepth(x2);
    return (d1 > d2) ? d1 :
	   (d2 > d1) ? d2 : d1+1;
  }

static atype exprtype(tnode *x)
  { tinstr *ti = x -> ti;
    switch (ti -> op)
      { default:
	    giveup("unimplemented: 7 %s", topstring(ti -> op));

	case t_ldc:	    case t_ldl:		case t_mint:	    case t_wsub:	case t_wsubdb:
	case t_lb:	    case t_ldpi:	case t_ldlp:
	case t_sum:	    case t_diff:	case t_prod:	    case t_div:		case t_rem:
	case t_and:	    case t_or:		case t_xor:	    case t_shl:		case t_shr:
	case t_gt:	    case t_not:		case t_adc:	    case t_eqc:		case t_ldnl:	    case t_ldnlp:
	case t_calli:	    case t_case:
	case t_fpcvtfi:	    case t_fpcvtdi:	case t_fpeq:	    case t_fpgt:
	    return ty_int;

	case t_fpldzerosn:  case t_fpldnlsn:	case t_fpcvtif:	    case t_fpur64tor32: case t_callf:
	    return ty_float;

	case t_fpldzerodb:  case t_fpldnldb:	case t_fpcvtid:	    case t_fpur32tor64: case t_calld:
	    return ty_double;

	case t_fpadd:	    case t_fpsub:	case t_fpmul:	    case t_fpdiv:	case t_fprem:
	  { atype ty1 = exprtype(x -> h2), ty2 = exprtype(x -> h3);
	    if (ty1 != ty2) warn("type mismatch in floating point expr");
	    return ty1;
	  }

	case t_cmpnd:
	    return exprtype(x -> h2);

	case t_cexpr:
	  { atype ty1 = exprtype(x -> h3), ty2 = exprtype(x -> h4);
	    if (ty1 != ty2) warn("type mismatch in conditional expr");
	    return ty1;
	  }
      }
  }

static int alloctemp(atype ty)
  { if (locals < 0) giveup("bug: alloctemp");
    locals += (ty == ty_double ? 2 : 1);
    if (locals > maxlocals) maxlocals = locals;
    return locals-1;	/* this is correct for any size */
  }

static void loadfrom(int m, word p, atype ty)
  { if (ty == ty_float || ty == ty_double)
      { gen(t_ldlp, m, p);
	gen((ty == ty_double) ? t_fpldnldb : t_fpldnlsn);
      }
    else gen(t_ldl, m, p);
  }

static void storein(int m, word p, atype ty)
  { if (ty == ty_float || ty == ty_double)
      { gen(t_ldlp, m, p);
	gen((ty == ty_double) ? t_fpstnldb : t_fpstnlsn);
      }
    else gen(t_stl, m, p);
  }

static void gen(int op, int m, word p)
  { program -> gen(op, m, p);
  }

