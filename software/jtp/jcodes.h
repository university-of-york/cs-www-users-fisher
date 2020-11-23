enum jcode
  { j_nop = 0, j_aconstnull, j_iconstm1, j_iconst0, j_iconst1, j_iconst2, j_iconst3, j_iconst4, j_iconst5,
    j_lconst0, j_lconst1, j_fconst0, j_fconst1, j_fconst2, j_dconst0, j_dconst1,
    j_bipush, j_sipush, j_ldc1, j_ldc2, j_ldc2w, j_iload, j_lload, j_fload, j_dload, j_aload,
    j_iload0, j_iload1, j_iload2, j_iload3, j_lload0, j_lload1, j_lload2, j_lload3,
    j_fload0, j_fload1, j_fload2, j_fload3, j_dload0, j_dload1, j_dload2, j_dload3,
    j_aload0, j_aload1, j_aload2, j_aload3, j_iaload, j_laload, j_faload, j_daload, j_aaload,
    j_baload, j_caload, j_saload, j_istore, j_lstore, j_fstore, j_dstore, j_astore,
    j_istore0, j_istore1, j_istore2, j_istore3, j_lstore0, j_lstore1, j_lstore2, j_lstore3,
    j_fstore0, j_fstore1, j_fstore2, j_fstore3, j_dstore0, j_dstore1, j_dstore2, j_dstore3,
    j_astore0, j_astore1, j_astore2, j_astore3, j_iastore, j_lastore, j_fastore, j_dastore, j_aastore, j_bastore,
    j_castore, j_sastore, j_pop, j_pop2, j_dup, j_dupx1, j_dupx2, j_dup2, j_dup2x1, j_dup2x2, j_swap,
    j_iadd, j_ladd, j_fadd, j_dadd, j_isub, j_lsub, j_fsub, j_dsub, j_imul, j_lmul, j_fmul, j_dmul,
    j_idiv, j_ldiv, j_fdiv, j_ddiv, j_irem, j_lrem, j_frem, j_drem, j_ineg, j_lneg, j_fneg, j_dneg,
    j_ishl, j_lshl, j_ishr, j_lshr, j_iushr, j_lushr, j_iand, j_land, j_ior, j_lor, j_ixor, j_lxor,
    j_iinc, j_i2l, j_i2f, j_i2d, j_l2i, j_l2f, j_l2d, j_f2i, j_f2l, j_f2d, j_d2i, j_d2l, j_d2f,
    j_int2byte, j_int2char, j_int2short, j_lcmp, j_fcmpl, j_fcmpg, j_dcmpl, j_dcmpg,
    j_ifeq, j_ifne, j_iflt, j_ifge, j_ifgt, j_ifle,
    j_ificmpeq, j_ificmpne, j_ificmplt, j_ificmpge, j_ificmpgt, j_ificmple, j_ifacmpeq, j_ifacmpne,
    j_goto, j_jsr, j_ret, j_tableswitch, j_lookupswitch, j_ireturn, j_lreturn, j_freturn, j_dreturn, j_areturn, j_return,
    j_getstatic, j_putstatic, j_getfield, j_putfield,
    j_callvirtual, j_callnonvirtual, j_callstatic, j_callinterface, j_186, j_new,
    j_newarray, j_anewarray, j_arraylength, j_athrow, j_checkcast, j_instanceof, j_monitorenter, j_monitorexit,
    j_wide, j_multianewarray, j_ifnull, j_ifnonnull, j_gotow, j_jsrw, j_breakpoint,
    j_203, j_204, j_205, j_206, j_207, j_208, j_retw,
    j_end = 255,
  };

enum typecode
  { ty_boolean = 4, ty_char, ty_float, ty_double, ty_byte, ty_short, ty_int, ty_long,
  };

