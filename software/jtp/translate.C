/* jtp - Compiler - Java bytecode to Transputer assembly code
   A.J. Fisher	 June 1997 */

#include <stdio.h>
#include <string.h>
#include "jtp.h"
#include "jcodes.h"
#include "tcodes.h"
#include "cftags.h"

static char *obj_seterr_id  = lookupstring("Object::seterror()");
static char *new_id	    = lookupstring("$new");
static char *newarray_id    = lookupstring("$newarray");
static char *newmatrix_id   = lookupstring("$newmatrix");

static char *methodname;
static int numfps, monitors;
static tprog *program;
static tinstr *entryptr;
static word *constpool;
static uchar *consttags;
static uchar *codeptr;

static void transcommand();
static bool isgoto(int);
static void transcall(fullname*), transldc(int);
static int typesize(int), elsize(char), callop(int);
static form *lookupfield(fullname*), *lookupmethod(fullname*);
static void genjump(int, int, bool), gencondit(int), genfpcondit(int);
static void gen(int, int = m_none, word = 0, int = m_none, word = 0);
static int localaddress(int, int);


global void translate(form *x, tprog *y, word *cpool, uchar *ctags)
  { /* translate method from Java bytecode into Transputer code */
    program = y; constpool = cpool; consttags = ctags;
    codeptr = x -> code;
    numfps = x -> fps;
    methodname = mkname(x -> fn);
    gen(t_entry, m_name, methodname);
    entryptr = program -> last();
    entryptr -> lvi = new lvinfo(x -> fps, x -> locs, -1);		/* num. aps unknown until optimize phase */
    monitors = 0;
    bool live = true;
    until (codeptr[0] == j_end)
      { int lab = labelat(codeptr);
	if (lab > 0)
	  { gen(t_label, m_lab, lab);
	    live = true;
	  }
	if (live) transcommand();
	if (isgoto(codeptr[0])) live = false;
	codeptr += jlength(codeptr);
      }
    if (monitors > 0) warn("bug: in `%s': mismatched monitors (%d)", methodname, monitors);
    gen(t_end);
  }

static bool isgoto(int op)
  { switch (op)
      { default:
	    return false;

	case j_goto:
	case j_return:		case j_ireturn:		case j_areturn:		case j_freturn:		case j_dreturn:
	case j_tableswitch:	case j_lookupswitch:	case j_athrow:
	    return true;
      }
  }

static void transcommand()
  { switch (codeptr[0])
      { default:
	    giveup("in `%s': unimplemented: 1 %s", methodname, jopstring(codeptr[0]));

	case j_ldc1:
	    transldc(codeptr[1]);
	    break;

	case j_ldc2:		case j_ldc2w:
	  { int n = (codeptr[1] << 8) | codeptr[2];
	    transldc(n);
	    break;
	  }

	case j_aconstnull:
	    gen(t_ldc, m_num, MINT);		/* optimized into t_mint */
	    break;

	case j_iconstm1:
	    gen(t_ldc, m_num, -1);
	    break;

	case j_iconst0:		case j_iconst1:		case j_iconst2:
	case j_iconst3:		case j_iconst4:		case j_iconst5:
	    gen(t_ldc, m_num, codeptr[0] - j_iconst0);
	    break;

	case j_bipush:
	  { int b = codeptr[1];
	    if (b & 0x80) b |= 0xffffff00;
	    gen(t_ldc, m_num, b);
	    break;
	  }

	case j_sipush:
	  { int n = (codeptr[1] << 8) | codeptr[2];
	    if (n & 0x8000) n |= 0xffff0000;
	    gen(t_ldc, m_num, n);
	    break;
	  }

	case j_fconst0:
	    gen(t_fpldzerosn);
	    break;

	case j_fconst1:		case j_fconst2:
	  { static float fc[] = { 1.0f, 2.0f };
	    word *ptr = (word*) &fc[codeptr[0] - j_fconst1];
	    gen(t_ldpi, m_afloat, ptr);
	    gen(t_fpldnlsn);
	    break;
	  }

	case j_dconst0:
	    gen(t_fpldzerodb);
	    break;

	case j_dconst1:
	  { static double fc = 1.0;
	    word *ptr = (word*) &fc;
	    gen(t_ldpi, m_adouble, ptr);
	    gen(t_fpldnldb);
	    break;
	  }

	case j_iload:
	case j_iload0:		case j_iload1:		case j_iload2:		case j_iload3:
	  { int n = (codeptr[0] == j_iload) ? codeptr[1] : (codeptr[0] - j_iload0);
	    int a = localaddress(n, 1);
	    gen(t_ldl, m_loc, a);
	    break;
	  }

	case j_fload:
	case j_fload0:		case j_fload1:		case j_fload2:		case j_fload3:
	  { int n = (codeptr[0] == j_fload) ? codeptr[1] : (codeptr[0] - j_fload0);
	    int a = localaddress(n, 1);
	    gen(t_ldlp, m_loc, a);
	    gen(t_fpldnlsn);
	    break;
	  }

	case j_dload:
	case j_dload0:		case j_dload1:		case j_dload2:		case j_dload3:
	  { int n = (codeptr[0] == j_dload) ? codeptr[1] : (codeptr[0] - j_dload0);
	    int a = localaddress(n, 2);
	    gen(t_ldlp, m_loc, a);
	    gen(t_fpldnldb);
	    break;
	  }

	case j_aload:
	case j_aload0:		case j_aload1:		case j_aload2:		case j_aload3:
	  { int n = (codeptr[0] == j_aload) ? codeptr[1] : (codeptr[0] - j_aload0);
	    int a = localaddress(n, 1);
	    gen(t_ldl, m_loc, a);
	    break;
	  }

	case j_iaload:		case j_aaload:
	    gen(t_rev); gen(t_wsub);
	    gen(t_ldnl, m_num, 0);
	    break;

	case j_baload:		case j_caload:
	    gen(t_sum); gen(t_lb);
	    gen(t_ldc, m_num, 0xff);
	    gen(t_and);
	    break;

	case j_faload:
	    gen(t_rev); gen(t_wsub);
	    gen(t_fpldnlsn);
	    break;

	case j_daload:
	    gen(t_rev); gen(t_wsubdb);
	    gen(t_fpldnldb);
	    break;

	case j_istore:
	case j_istore0:		case j_istore1:		case j_istore2:		case j_istore3:
	  { int n = (codeptr[0] == j_istore) ? codeptr[1] : (codeptr[0] - j_istore0);
	    int a = localaddress(n, 1);
	    gen(t_stl, m_loc, a);
	    break;
	  }

	case j_fstore:
	case j_fstore0:		case j_fstore1:		case j_fstore2:		case j_fstore3:
	  { int n = (codeptr[0] == j_fstore) ? codeptr[1] : (codeptr[0] - j_fstore0);
	    int a = localaddress(n, 1);
	    gen(t_ldlp, m_loc, a);
	    gen(t_fpstnlsn);
	    break;
	  }

	case j_dstore:
	case j_dstore0:		case j_dstore1:		case j_dstore2:		case j_dstore3:
	  { int n = (codeptr[0] == j_dstore) ? codeptr[1] : (codeptr[0] - j_dstore0);
	    int a = localaddress(n, 2);
	    gen(t_ldlp, m_loc, a);
	    gen(t_fpstnldb);
	    break;
	  }

	case j_astore:
	case j_astore0:		case j_astore1:		case j_astore2:		case j_astore3:
	  { int n = (codeptr[0] == j_astore) ? codeptr[1] : (codeptr[0] - j_astore0);
	    int a = localaddress(n, 1);
	    gen(t_stl, m_loc, a);
	    break;
	  }

	case j_iastore:		case j_aastore:
	    gen(t_rev); gen(t_rot); gen(t_wsub);
	    gen(t_stnl, m_num, 0);
	    break;

	case j_bastore:		case j_castore:
	    /* chars treated as bytes */
	    gen(t_rev); gen(t_rot); gen(t_sum); gen(t_sb);
	    break;

	case j_fastore:
	    gen(t_rev); gen(t_rot); gen(t_wsub); gen(t_fpstnlsn);
	    break;

	case j_dastore:
	    gen(t_rev); gen(t_rot); gen(t_wsubdb); gen(t_fpstnldb);
	    break;

	case j_iinc:
	  { int b = codeptr[2];
	    if (b & 0x80) b |= 0xffffff00;
	    unless (b == 0)
	      { int a = localaddress(codeptr[1], 1);
		gen(t_ldl, m_loc, a);
		gen(t_adc, m_num, b);
		gen(t_stl, m_loc, a);
	      }
	    break;
	  }

	case j_getfield:
	  { int n = (codeptr[1] << 8) | codeptr[2];
	    fullname *fn = constpool[n].f;
	    form *x = lookupfield(fn);
	    unless (x == NULL)
	      { if (x -> rtype == 'F' || x -> rtype == 'D')
		  { gen(t_ldnlp, m_num, x -> addr);
		    gen((x -> rtype == 'D') ? t_fpldnldb : t_fpldnlsn);
		  }
		else gen(t_ldnl, m_num, x -> addr);
	      }
	    break;
	  }

	case j_putfield:
	  { int n = (codeptr[1] << 8) | codeptr[2];
	    fullname *fn = constpool[n].f;
	    form *x = lookupfield(fn);
	    unless (x == NULL)
	      { gen(t_rev);
		if (x -> rtype == 'F' || x -> rtype == 'D')
		  { gen(t_ldnlp, m_num, x -> addr);
		    gen((x -> rtype == 'D') ? t_fpstnldb : t_fpstnlsn);
		  }
		else gen(t_stnl, m_num, x -> addr);
	      }
	    break;
	  }

	case j_getstatic:
	  { int n = (codeptr[1] << 8) | codeptr[2];
	    fullname *fn = constpool[n].f;
	    form *x = lookupfield(fn);
	    unless (x == NULL)
	      { gen(t_ldpi, m_name, mkname(fn));
		if (x -> rtype == 'F') gen(t_fpldnlsn);
		else if (x -> rtype == 'D') gen(t_fpldnldb);
		else gen(t_ldnl, m_num, 0);
	      }
	    break;
	  }

	case j_putstatic:
	  { int n = (codeptr[1] << 8) | codeptr[2];
	    fullname *fn = constpool[n].f;
	    form *x = lookupfield(fn);
	    unless (x == NULL)
	      { gen(t_ldpi, m_name, mkname(fn));
		if (x -> rtype == 'F') gen(t_fpstnlsn);
		else if (x -> rtype == 'D') gen(t_fpstnldb);
		else gen(t_stnl, m_num, 0);
	      }
	    break;
	  }

	case j_callvirtual:	case j_callnonvirtual:	case j_callstatic:
	  { int n = (codeptr[1] << 8) | codeptr[2];
	    transcall(constpool[n].f);
	    break;
	  }

	case j_return:		case j_ireturn:		case j_areturn:		case j_freturn:		case j_dreturn:
	    if (monitors > 0) warn("in `%s': return from within monitor", methodname);
	    gen((codeptr[0] == j_return) ? t_retv : t_retnv);
	    break;

	case j_fcmpl:		case j_fcmpg:		case j_dcmpl:		case j_dcmpg:
	  { /* expected to be followed by j_ifeq (etc) */
	    codeptr += jlength(codeptr);
	    int n = (codeptr[1] << 8) | codeptr[2];
	    if (n & 0x8000) n |= 0xffff0000;
	    genjump(codeptr[0], labelat(&codeptr[n]), true);
	    break;
	  }

	case j_ifeq:		case j_ifnull:		case j_iflt:		case j_ifle:		case j_ifne:
	case j_ifnonnull:	case j_ifgt:		case j_ifge:
	case j_ificmpeq:	case j_ificmpne:	case j_ificmplt:	case j_ificmpgt:
	case j_ificmple:	case j_ificmpge:	case j_ifacmpeq:	case j_ifacmpne:
	case j_goto:		case j_jsr:
	  { int n = (codeptr[1] << 8) | codeptr[2];
	    if (n & 0x8000) n |= 0xffff0000;
	    genjump(codeptr[0], labelat(&codeptr[n]), false);
	    break;
	  }

	case j_tableswitch:
	  { int len = 1;
	    while ((int) (codeptr+len) & 3) len++;	/* skip padding after opcode */
	    tswitch *sw = (tswitch*) &codeptr[len];
	    int ni = (sw -> hi) - (sw -> lo) + 1;
	    int dl = labelat(&codeptr[sw -> dflt]);
	    for (int i = 0; i < ni; i++)
	      { int n = (sw -> lo) + i;
		int l = labelat(&codeptr[sw -> vec[i]]);
		unless (l == dl) gen(t_case, m_num, n, m_lab, l);
	      }
	    gen(t_pop);
	    gen(t_j, m_lab, dl);
	    break;
	  }

	case j_lookupswitch:
	  { int len = 1;
	    while ((int) (codeptr+len) & 3) len++;	/* skip padding after opcode */
	    lswitch *sw = (lswitch*) &codeptr[len];
	    int np = sw -> np;
	    int dl = labelat(&codeptr[sw -> dflt]);
	    for (int i = 0; i < np; i++)
	      { int n = sw -> vec[2*i];
		int l = labelat(&codeptr[sw -> vec[2*i+1]]);
		gen(t_case, m_num, n, m_lab, l);
	      }
	    gen(t_pop);
	    gen(t_j, m_lab, dl);
	    break;
	  }

	case j_new:
	  { int n = (codeptr[1] << 8) | codeptr[2];
	    char *name = constpool[n].s;
	    Class *cl = lookupclass(name);
	    if (cl != NULL)
	      { char buf[MAXSTRING+1]; sprintf(buf, "%s::<mtab>", name);
		gen(t_ldc, m_num, cl -> refbm);		    /* reference fields bitmap */
		gen(t_ldpi, m_name, lookupstring(buf));	    /* method table ptr */
		gen(t_ldc, m_num, cl -> size);		    /* num. words */
		gen(t_calli, m_name, new_id, m_num, 3);
	      }
	    else warn("can't find class `%s'", name);
	    break;
	  }

	case j_newarray:
	    gen(t_ldc, m_num, typesize(codeptr[1]));
	    gen(t_rev);
	    gen(t_ldc, m_num, 0);	/* reference fields bitmap */
	    gen(t_calli, m_name, newarray_id, m_num, 3);
	    break;

	case j_anewarray:
	    gen(t_ldc, m_num, 4);	/* size of reference */
	    gen(t_rev);
	    gen(t_ldc, m_num, -1);	/* reference fields bitmap */
	    gen(t_calli, m_name, newarray_id, m_num, 3);
	    break;

	case j_multianewarray:
	  { int ix = (codeptr[1] << 8) | codeptr[2];
	    char *s = trsig(constpool[ix].s);
	    int n = codeptr[3], m = 0;
	    while (s[m] == 'R' && m < n) m++;
	    gen(t_ldc, m_num, elsize(s[m]));	/* base element size */
	    if (n == 1)
	      { gen(t_rev);
		gen(t_ldc, m_num, (s[m] == 'L' || s[m] == 'R') ? -1 : 0);
		gen(t_calli, m_name, newarray_id, m_num, 3);
	      }
	    else if (n == 2)
	      { gen(t_rot); gen(t_rot);
		gen(t_ldc, m_num, (s[m] == 'L' || s[m] == 'R') ? -1 : 0);
		gen(t_calli, m_name, newmatrix_id, m_num, 4);
	      }
	    else
	      { warn("unimplemented: %d dimensional array of `%s'", n, &s[m]);
		while (n > 0)
		  { gen(t_pop);	    /* pop stack */
		    n--;
		  }
	      }
	    break;
	  }

	case j_arraylength:
	    gen(t_ldnl, m_num, -1);
	    break;

	case j_monitorenter:
	    monitors++;
	    gen(t_callv, m_name, lookupstring("Semaphore::down()"), m_num, 1);
	    break;

	case j_monitorexit:
	    if (monitors > 0) monitors--;
	    else warn("bug: in `%s': mismatched monitors", methodname);
	    gen(t_callv, m_name, lookupstring("Semaphore::up()"), m_num, 1);
	    break;

	case j_dup:
	    gen(t_dup);
	    break;

	case j_dup2:
	    gen(t_dup2);
	    break;

	case j_dupx1:
	    gen(t_dupx1);
	    break;

	case j_dupx2:
	    gen(t_dupx2);
	    break;

	case j_dup2x2:
	    gen(t_dup2x2);
	    break;

	case j_pop:	case j_pop2:
	    gen(t_pop);
	    break;

	case j_swap:
	    gen(t_rev);
	    break;

	case j_iadd:
	    gen(t_sum);
	    break;

	case j_fadd:	case j_dadd:
	    gen(t_fpadd);
	    break;

	case j_isub:
	    gen(t_diff);
	    break;

	case j_fsub:	case j_dsub:
	    gen(t_fpsub);
	    break;

	case j_imul:
	    gen(t_prod);
	    break;

	case j_fmul:	case j_dmul:
	    gen(t_fpmul);
	    break;

	case j_idiv:
	    gen(t_div);
	    break;

	case j_fdiv:	case j_ddiv:
	    gen(t_fpdiv);
	    break;

	case j_irem:
	    gen(t_rem);
	    break;

	case j_frem:	case j_drem:
	    gen(t_fprem);	/* expanded later */
	    break;

	case j_ineg:
	    gen(t_not);
	    gen(t_adc, m_num, 1);
	    break;

	case j_fneg:
	    gen(t_fpldzerosn);
	    gen(t_rev); gen(t_fpsub);
	    break;

	case j_dneg:
	    gen(t_fpldzerodb);
	    gen(t_rev); gen(t_fpsub);
	    break;

	case j_iand:
	    gen(t_and);
	    break;

	case j_ior:
	    gen(t_or);
	    break;

	case j_ixor:
	    gen(t_xor);
	    break;

	case j_ishl:
	    gen(t_shl);
	    break;

	case j_ishr:		/* ??? ought to extend sign */
	case j_iushr:
	    gen(t_shr);
	    break;

	case j_f2i:
	    gen(t_fpcvtfi);	/* expanded by optimize */
	    break;

	case j_d2i:
	    gen(t_fpcvtdi);	/* expanded by optimize */
	    break;

	case j_i2f:
	    gen(t_fpcvtif);	/* expanded by optimize */
	    break;

	case j_i2d:
	    gen(t_fpcvtid);	/* expanded by optimize */
	    break;

	case j_f2d:
	    gen(t_fpur32tor64);
	    break;

	case j_d2f:
	    gen(t_fpur64tor32);
	    break;

	case j_int2byte:	case j_int2char:
	    gen(t_ldc, m_num, 0xff);	/* bytes and chars are unsigned 8-bit in this implementation */
	    gen(t_and);
	    break;

	case j_int2short:
	    gen(t_ldc, m_num, 0xffff);	/* shorts are unsigned 16-bit in this implementation */
	    gen(t_and);
	    break;
      }
  }

static void transcall(fullname *fn)
  { form *x = lookupmethod(fn);
    unless (x == NULL)
      { int op = callop(x -> rtype);	    /* t_callv/f/i */
	if (x -> acc & acc_final)
	  { /* non-virtual call */
	    char *xfn = mkname(fn);
	    if (xfn == obj_seterr_id) gen(t_seterr);	/* inline Object::seterror() */
	    else gen(op, m_name, xfn, m_num, x -> fps);
	  }
	else
	  { /* virtual call */
	    gen(op, m_num, x -> addr, m_num, x -> fps);
	  }
      }
  }

static void transldc(int ix)
  { switch (consttags[ix])
      { default:
	    warn("bug: unknown tag %d", consttags[ix]);
	    break;

	case Constant_Integer:
	    gen(t_ldc, m_num, constpool[ix].n);
	    break;

	case Constant_Float:
	    gen(t_ldpi, m_afloat, &constpool[ix]);
	    gen(t_fpldnlsn);
	    break;

	case Constant_Double:
	    gen(t_ldpi, m_adouble, &constpool[ix]);
	    gen(t_fpldnldb);
	    break;

	case Constant_String:
	    gen(t_ldpi, m_astr, constpool[ix].s);
	    break;
      }
  }

static int typesize(int ty)
  { switch (ty)
      { default:
	    warn("bug: typesize %d", ty);
	    return 0;

	case ty_boolean:    case ty_byte:	case ty_char:
	    return 1;

	case ty_short:
	    return 2;

	case ty_int:	    case ty_float:
	    return 4;

	case ty_double:	    case ty_long:
	    return 8;
      }
  }

static int elsize(char ch)
  { switch (ch)
      { default:
	    warn("bug: elsize `%c'", ch);
	    return 0;

	case 'Z':   case 'B':	case 'C':
	    return 1;

	case 'S':
	    return 2;

	case 'I':   case 'F':	case 'R':   case 'L':
	    return 4;

	case 'D':   case 'J':
	    return 8;

      }
  }

static int callop(int ch)
  { switch (ch)
      { default:
	    warn("bug: callop `%c'", ch);
	    return t_callv;

	case 'V':
	    return t_callv;

	case 'I':   case 'C':	case 'L':   case 'Z':	case 'R':
	    return t_calli;

	case 'F':
	    return t_callf;

	case 'D':
	    return t_calld;
      }
  }

static form *lookupfield(fullname *fn)
  { form *x = NULL;
    char *cn = fn -> cl;
    Class *cl = lookupclass(cn);
    if (cl != NULL)
      { formvec *fv = cl -> fields; int n = 0;
	until (n >= fv -> num || seq(fv -> vec[n] -> fn -> id, fn -> id)) n++;
	if (n < fv -> num) x = fv -> vec[n];
	else warn("can't find field `%s' in class `%s'", fn -> id, cn);
      }
    else warn("can't find class `%s'", cn);
    return x;
  }

static form *lookupmethod(fullname *fn)
  { form *x = NULL;
    char *cn = fn -> cl;
    Class *cl = lookupclass(cn);
    if (cl != NULL)
      { formvec *fv = cl -> methods; int n = 0;
	until (n >= fv -> num || seq(fv -> vec[n] -> fn -> id, fn -> id) && seq(fv -> vec[n] -> fn -> sig, fn -> sig)) n++;
	if (n < fv -> num) x = fv -> vec[n];
	else warn("can't find method `%s'", mkname(fn));
      }
    else warn("can't find class `%s'", cn);
    return x;
  }

static void genjump(int op, int lab, bool fp)
  { switch (op)
      { default:
	    (fp ? genfpcondit : gencondit) (op);
	    gen(t_cj, m_lab, lab);
	    break;

	case j_goto:
	    gen(t_j, m_lab, lab);
	    break;
      }
  }

static void gencondit(int op)
  { switch (op)
      { default:
	    giveup("unimplemented: 2i %s", jopstring(op));

	case j_ifeq:
	    break;

	case j_ifne:
	    gen(t_eqc, m_num, 0);
	    break;

	case j_ifgt:	case j_iflt:	case j_ifge:	case j_ifle:
	    gen(t_ldc, m_num, 0);
	    gencondit(op + 6);	/* ifcmpgt etc. */
	    break;

	case j_ifnull:
	    gen(t_ldc, m_num, MINT);	/* optimized into t_mint */
	    gencondit(j_ifacmpeq);
	    break;

	case j_ifnonnull:
	    gen(t_ldc, m_num, MINT);	/* optimized into t_mint */
	    gencondit(j_ifacmpne);
	    break;

	case j_ificmpeq:	case j_ifacmpeq:
	    gen(t_diff);
	    break;

	case j_ificmpne:	case j_ifacmpne:
	    gen(t_diff);
	    gen(t_eqc, m_num, 0);
	    break;

	case j_ificmple:
	    gen(t_gt);
	    break;

	case j_ificmpge:
	    gen(t_rev); gen(t_gt);
	    break;

	case j_ificmpgt:
	    gen(t_gt);
	    gen(t_eqc, m_num, 0);
	    break;

	case j_ificmplt:
	    gen(t_rev); gen(t_gt);
	    gen(t_eqc, m_num, 0);
	    break;
      }
  }

static void genfpcondit(int op)
  { switch (op)
      { default:
	    giveup("unimplemented: 2f %s", jopstring(op));

	case j_ifeq:
	    gen(t_fpeq);
	    gen(t_eqc, m_num, 0);
	    break;

	case j_ifne:
	    gen(t_fpeq);
	    break;

	case j_ifgt:
	    gen(t_fpgt);
	    gen(t_eqc, m_num, 0);
	    break;

	case j_iflt:
	    gen(t_rev); gen(t_fpgt);
	    gen(t_eqc, m_num, 0);
	    break;

	case j_ifge:
	    gen(t_rev); gen(t_fpgt);
	    break;

	case j_ifle:
	    gen(t_fpgt);
	    break;
      }
  }

static void gen(int op, int m1, word p1, int m2, word p2)
  { program -> gen(op, m1, p1, m2, p2);
  }

static int localaddress(int n, int sz)
  { /* translate Java local or fp offset into arg for m_loc */
    if (n < numfps)
      { /* formals are stored in reverse order from Java model, i.e. ascending memory order */
	n = (numfps-1) - n;
      }
    else
      { /* local variable */
	n += (sz-1);	/* point to low-address word of multi-word item */
      }
    return n;
  }

