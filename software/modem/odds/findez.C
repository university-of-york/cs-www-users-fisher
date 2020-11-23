static uchar ctab4[] =
  { 0xf, 0x1, 0x3, 0xd,
    0x5, 0x8, 0xa, 0x7,
    0x6, 0xb, 0x9, 0x4,
    0xc, 0x2, 0x0, 0xe,
  };

static void findez(complex z, complex &ez)
  { int r = 0;
    r = (r << 1) | (z.re > +2.0 || z.re < -2.0);
    r = (r << 1) | (z.im > +2.0 || z.im < -2.0);
    r = (r << 1) | (z.re > 0.0);
    r = (r << 1) | (z.im > 0.0);
    r = ctab4[r];	/* get 4-bit constellation posn */
    ez = ztab4[r];	/* quantized version of z */
  }
