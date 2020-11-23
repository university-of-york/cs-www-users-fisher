/* jtp - Compiler - Java bytecode to Transputer assembly code
   A.J. Fisher	 June 1997 */

#include <stdio.h>
#include "jtp.h"
#include "jcodes.h"
#include "cftags.h"

static word *constpool;
static uchar *consttags;

static void prconstant(int);
static char *typestring(int);
static int getwd(uchar*, int&);


global void jlist(uchar *codeptr, word *cpool, uchar *ctags)
  { constpool = cpool;
    consttags = ctags;
    printf("\t%s", jopstring(codeptr[0]));
    switch (codeptr[0])
      { case j_iload:		case j_lload:		case j_fload:		case j_dload:		case j_aload:
	case j_istore:		case j_lstore:		case j_fstore:		case j_dstore:		case j_astore:
	case j_wide:		case j_ret:
	    printf("\t%d", codeptr[1]);
	    break;

	case j_iinc:
	  { int b = codeptr[2];
	    if (b & 0x80) b |= 0xffffff00;
	    printf("\t%d,%d", codeptr[1], b);
	    break;
	  }

	case j_newarray:
	    printf("\t%s", typestring(codeptr[1]));
	    break;

	case j_bipush:
	  { int b = codeptr[1];
	    if (b & 0x80) b |= 0xffffff00;
	    printf("\t%d", b);
	    break;
	  }

	case j_ldc1:
	  { int b = codeptr[1];
	    prconstant(b);
	    break;
	  }

	case j_sipush:
	  { int w = (codeptr[1] << 8) | codeptr[2];
	    if (w & 0x8000) w |= 0xffff0000;
	    printf("\t%d", w);
	    break;
	  }

	case j_ldc2:		case j_ldc2w:
	  { int w = (codeptr[1] << 8) | codeptr[2];
	    prconstant(w);
	    break;
	  }

	case j_anewarray:	case j_new:		case j_checkcast:	case j_instanceof:
	  { int w = (codeptr[1] << 8) | codeptr[2];
	    if (codeptr[0] == j_new) putchar('\t');
	    printf("\tcl=%s", constpool[w].s);
	    break;
	  }

	case j_putfield:	case j_getfield:	case j_putstatic:	case j_getstatic:
	case j_callvirtual:	case j_callnonvirtual:	case j_callstatic:
	  { int w = (codeptr[1] << 8) | codeptr[2];
	    fullname *fn = constpool[w].f;
	    printf("\tcl=%s id=%s sig=%s", fn -> cl, fn -> id, fn -> sig);
	    break;
	  }

	case j_callinterface:
	  { int w = (codeptr[1] << 8) | codeptr[2];
	    fullname *fn = constpool[w].f;
	    printf("\tcl=%s id=%s sig=%s, %d,%d", fn -> cl, fn -> id, fn -> sig, codeptr[3], codeptr[4]);
	    break;
	  }

	case j_multianewarray:
	  { int w = (codeptr[1] << 8) | codeptr[2];
	    printf("\tcl=%s,%d", constpool[w].s, codeptr[3]);
	    break;
	  }

	case j_retw:
	  { int w = (codeptr[1] << 8) | codeptr[2];
	    printf("\t%d", w);
	    break;
	  }

	case j_ifeq:		case j_ifnull:		case j_iflt:		case j_ifle:		case j_ifne:
	case j_ifnonnull:	case j_ifgt:		case j_ifge:
	case j_ificmpeq:	case j_ificmpne:	case j_ificmplt:	case j_ificmpgt:
	case j_ificmple:	case j_ificmpge:	case j_ifacmpeq:	case j_ifacmpne:
	case j_goto:		case j_jsr:
	  { int n = (codeptr[1] << 8) | codeptr[2];
	    if (n & 0x8000) n |= 0xffff0000;
	    printf("\tL%d", labelat(&codeptr[n]));
	    break;
	  }

	case j_gotow:		case j_jsrw:
	  { int n;
	    for (int i = 0; i < 4; i++) n = (n << 8) | codeptr[i+1];
	    printf("\t%d", labelat(&codeptr[n]));
	  }

	case j_tableswitch:	case j_lookupswitch:
	    printf("\t...");
	    break;
      }
    putchar('\n');
  }

static void prconstant(int ix)
  { printf("\t\t");
    int tag = consttags[ix];
    switch (tag)
      { default:
	    printf("??? <tag=%d>", tag);
	    break;

	case Constant_Integer:
	    printf("%d", constpool[ix].n);
	    break;

	case Constant_Float:
	    printf("%14.7e", bitsfp(constpool[ix].n));
	    break;

	case Constant_Double:
	    printf("%18.10e", bitsdp(constpool[ix].n, constpool[ix+1].n));
	    break;

	case Constant_String:
	    printf("\"%s\"", constpool[ix].s);
	    break;
      }
  }

global char *jopstring(int op)
  { switch (op)
      { default:		return "???";
	case j_bipush:		return "bipush";
	case j_sipush:		return "sipush";
	case j_ldc1:		return "ldc1";
	case j_ldc2:		return "ldc2";
	case j_ldc2w:		return "ldc2w";
	case j_aconstnull:	return "aconstnull";
	case j_iconstm1:	return "iconstm1";
	case j_iconst0:		return "iconst0";
	case j_iconst1:		return "iconst1";
	case j_iconst2:		return "iconst2";
	case j_iconst3:		return "iconst3";
	case j_iconst4:		return "iconst4";
	case j_iconst5:		return "iconst5";
	case j_lconst0:		return "lconst0";
	case j_lconst1:		return "lconst1";
	case j_fconst0:		return "fconst0";
	case j_fconst1:		return "fconst1";
	case j_fconst2:		return "fconst2";
	case j_dconst0:		return "dconst0";
	case j_dconst1:		return "dconst1";
	case j_iload:		return "iload";
	case j_iload0:		return "iload0";
	case j_iload1:		return "iload1";
	case j_iload2:		return "iload2";
	case j_iload3:		return "iload3";
	case j_lload:		return "lload";
	case j_lload0:		return "lload0";
	case j_lload1:		return "lload1";
	case j_lload2:		return "lload2";
	case j_lload3:		return "lload3";
	case j_fload:		return "fload";
	case j_fload0:		return "fload0";
	case j_fload1:		return "fload1";
	case j_fload2:		return "fload2";
	case j_fload3:		return "fload3";
	case j_dload:		return "dload";
	case j_dload0:		return "dload0";
	case j_dload1:		return "dload1";
	case j_dload2:		return "dload2";
	case j_dload3:		return "dload3";
	case j_aload:		return "aload";
	case j_aload0:		return "aload0";
	case j_aload1:		return "aload1";
	case j_aload2:		return "aload2";
	case j_aload3:		return "aload3";
	case j_istore:		return "istore";
	case j_istore0:		return "istore0";
	case j_istore1:		return "istore1";
	case j_istore2:		return "istore2";
	case j_istore3:		return "istore3";
	case j_lstore:		return "lstore";
	case j_lstore0:		return "lstore0";
	case j_lstore1:		return "lstore1";
	case j_lstore2:		return "lstore2";
	case j_lstore3:		return "lstore3";
	case j_fstore:		return "fstore";
	case j_fstore0:		return "fstore0";
	case j_fstore1:		return "fstore1";
	case j_fstore2:		return "fstore2";
	case j_fstore3:		return "fstore3";
	case j_dstore:		return "dstore";
	case j_dstore0:		return "dstore0";
	case j_dstore1:		return "dstore1";
	case j_dstore2:		return "dstore2";
	case j_dstore3:		return "dstore3";
	case j_astore:		return "astore";
	case j_astore0:		return "astore0";
	case j_astore1:		return "astore1";
	case j_astore2:		return "astore2";
	case j_astore3:		return "astore3";
	case j_iinc:		return "iinc";
	case j_wide:		return "wide";
	case j_newarray:	return "newarray";
	case j_anewarray:	return "anewarray";
	case j_multianewarray:	return "multianewarray";
	case j_arraylength:	return "arraylength";
	case j_iaload:		return "iaload";
	case j_laload:		return "laload";
	case j_faload:		return "faload";
	case j_daload:		return "daload";
	case j_aaload:		return "aaload";
	case j_baload:		return "baload";
	case j_caload:		return "caload";
	case j_saload:		return "saload";
	case j_iastore:		return "iastore";
	case j_lastore:		return "lastore";
	case j_fastore:		return "fastore";
	case j_dastore:		return "dastore";
	case j_aastore:		return "aastore";
	case j_bastore:		return "bastore";
	case j_castore:		return "castore";
	case j_sastore:		return "sastore";
	case j_nop:		return "nop";
	case j_pop:		return "pop";
	case j_pop2:		return "pop2";
	case j_dup:		return "dup";
	case j_dup2:		return "dup2";
	case j_dupx1:		return "dupx1";
	case j_dup2x1:		return "dup2x1";
	case j_dupx2:		return "dupx2";
	case j_dup2x2:		return "dup2x2";
	case j_swap:		return "swap";
	case j_iadd:		return "iadd";
	case j_ladd:		return "ladd";
	case j_fadd:		return "fadd";
	case j_dadd:		return "dadd";
	case j_isub:		return "isub";
	case j_lsub:		return "lsub";
	case j_fsub:		return "fsub";
	case j_dsub:		return "dsub";
	case j_imul:		return "imul";
	case j_lmul:		return "lmul";
	case j_fmul:		return "fmul";
	case j_dmul:		return "dmul";
	case j_idiv:		return "idiv";
	case j_ldiv:		return "ldiv";
	case j_fdiv:		return "fdiv";
	case j_ddiv:		return "ddiv";
	case j_irem:		return "irem";
	case j_lrem:		return "lrem";
	case j_frem:		return "frem";
	case j_drem:		return "drem";
	case j_ineg:		return "ineg";
	case j_lneg:		return "lneg";
	case j_fneg:		return "fneg";
	case j_dneg:		return "dneg";
	case j_ishl:		return "ishl";
	case j_ishr:		return "ishr";
	case j_iushr:		return "iushr";
	case j_lshl:		return "lshl";
	case j_lshr:		return "lshr";
	case j_lushr:		return "lushr";
	case j_iand:		return "iand";
	case j_land:		return "land";
	case j_ior:		return "ior";
	case j_lor:		return "lor";
	case j_ixor:		return "ixor";
	case j_lxor:		return "lxor";
	case j_i2l:		return "i2l";
	case j_i2f:		return "i2f";
	case j_i2d:		return "i2d";
	case j_l2i:		return "l2i";
	case j_l2f:		return "l2f";
	case j_l2d:		return "l2d";
	case j_f2i:		return "f2i";
	case j_f2l:		return "f2l";
	case j_f2d:		return "f2d";
	case j_d2i:		return "d2i";
	case j_d2l:		return "d2l";
	case j_d2f:		return "d2f";
	case j_int2byte:	return "int2byte";
	case j_int2char:	return "int2char";
	case j_int2short:	return "int2short";
	case j_ifeq:		return "ifeq";
	case j_ifnull:		return "ifnull";
	case j_iflt:		return "iflt";
	case j_ifle:		return "ifle";
	case j_ifne:		return "ifne";
	case j_ifnonnull:	return "ifnonnull";
	case j_ifgt:		return "ifgt";
	case j_ifge:		return "ifge";
	case j_ificmpeq:	return "ificmpeq";
	case j_ificmpne:	return "ificmpne";
	case j_ificmplt:	return "ificmplt";
	case j_ificmpgt:	return "ificmpgt";
	case j_ificmple:	return "ificmple";
	case j_ificmpge:	return "ificmpge";
	case j_lcmp:		return "lcmp";
	case j_fcmpl:		return "fcmpl";
	case j_fcmpg:		return "fcmpg";
	case j_dcmpl:		return "dcmpl";
	case j_dcmpg:		return "dcmpg";
	case j_ifacmpeq:	return "ifacmpeq";
	case j_ifacmpne:	return "ifacmpne";
	case j_goto:		return "goto";
	case j_gotow:		return "gotow";
	case j_jsr:		return "jsr";
	case j_jsrw:		return "jsrw";
	case j_ret:		return "ret";
	case j_retw:		return "retw";
	case j_ireturn:		return "ireturn";
	case j_lreturn:		return "lreturn";
	case j_freturn:		return "freturn";
	case j_dreturn:		return "dreturn";
	case j_areturn:		return "areturn";
	case j_return:		return "return";
	case j_breakpoint:	return "breakpoint";
	case j_tableswitch:	return "tableswitch";
	case j_lookupswitch:	return "lookupswitch";
	case j_putfield:	return "putfield";
	case j_getfield:	return "getfield";
	case j_putstatic:	return "putstatic";
	case j_getstatic:	return "getstatic";
	case j_callvirtual:	return "callvirtual";
	case j_callnonvirtual:	return "callnonvirtual";
	case j_callstatic:	return "callstatic";
	case j_callinterface:	return "callinterface";
	case j_athrow:		return "athrow";
	case j_new:		return "new";
	case j_checkcast:	return "checkcast";
	case j_instanceof:	return "instanceof";
	case j_monitorenter:	return "monitorenter";
	case j_monitorexit:	return "monitorexit";
      }
  }

static char *typestring(int ty)
  { switch (ty)
      { default:		return "???";
	case ty_boolean:	return "boolean";
	case ty_char:		return "char";
	case ty_float:		return "float";
	case ty_double:		return "double";
	case ty_byte:		return "byte";
	case ty_short:		return "short";
	case ty_int:		return "int";
	case ty_long:		return "long";
      }
  }

global int jlength(uchar *cp)
  { switch (cp[0])
      { default:	/* most opcodes are 1 byte long */
	    return 1;

	case j_bipush:		case j_ldc1:
	case j_iload:		case j_lload:		case j_fload:		case j_dload:		case j_aload:
	case j_istore:		case j_lstore:		case j_fstore:		case j_dstore:		case j_astore:
	case j_wide:		case j_newarray:	case j_ret:
	    return 2;

	case j_sipush:		case j_ldc2:		case j_ldc2w:		case j_iinc:		case j_anewarray:
	case j_ifeq:		case j_ifnull:		case j_iflt:		case j_ifle:		case j_ifne:
	case j_ifnonnull:	case j_ifgt:		case j_ifge:
	case j_ificmpeq:	case j_ificmpne:	case j_ificmplt:	case j_ificmpgt:
	case j_ificmple:	case j_ificmpge:	case j_ifacmpeq:	case j_ifacmpne:	case j_goto:
	case j_jsr:		case j_retw:		case j_putfield:	case j_getfield:
	case j_putstatic:	case j_getstatic:
	case j_callvirtual:	case j_callnonvirtual:	case j_callstatic:
	case j_new:		case j_checkcast:	case j_instanceof:
	    return 3;

	case j_multianewarray:
	    return 4;

	case j_gotow:		case j_jsrw:		case j_callinterface:
	    return 5;

	case j_tableswitch:	case j_lookupswitch:
	  { int len = 1;
	    while ((int) (cp+len) & 3) len++;	/* skip padding after opcode */
	    if (cp[0] == j_tableswitch)
	      { tswitch *sw = (tswitch*) &cp[len];
		int ni = (sw -> hi) - (sw -> lo) + 1;
		len += 4 * (ni+3);
	      }
	    else
	      { lswitch *sw = (lswitch*) &cp[len];
		int np = sw -> np;
		len += 4 * (2*np + 2);
	      }
	    return len;
	  }
      }
  }

static int getwd(uchar *cp, int &k)
  { int n;
    for (int i = 0; i <= 4; i++) n = (n << 8) | cp[k++];
    return n;
  }

