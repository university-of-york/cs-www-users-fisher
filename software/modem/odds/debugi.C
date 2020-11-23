static void debug_impulse()
  { static float rxbuf[15000];
    for (int i=0; i < 15000; i++)
      { Audio -> write((i == 1000) ? 0x200000 : 0);
	rxbuf[i] = (float) Audio -> read();
      }
    FILE *fi = fopen("debugi.grap", "w");
    if (fi == NULL) giveup("can't create debugi.grap");
    float ymax = -1.0;
    for (int i=0; i < 15000; i++)
      { float y = fabsf(rxbuf[i]);
	if (y > ymax) ymax = y;
      }
    int m = (int) (100.0 + log10f(ymax)) - 100;	    /* round downwards */
    float ym1 = powf(10.0, m); int nt = 10;
    float ym = ym1;
    while (ymax > ym)
      { ym = 2.0*ym1;
	if (ymax > ym) { ym = 3.0*ym1; nt = 6; }
	if (ymax > ym) { ym = 5.0*ym1; nt = 10; }
	if (ymax > ym) { ym1 *= 10.0; ym = ym1; }
      }
    for (int i=0; i < 15000; i++)
      { if (i%500 == 0)
	  { fprintf(fi, ".sp 0.5i\ni=%d\n.br\n", i);
	    fprintf(fi, ".G1 8i\n");
	    fprintf(fi, "ticks left from %g to %g by %g\n", -ym, +ym, ym/nt);
	    fprintf(fi, "new solid\n");
	  }
	fprintf(fi, "%d %g\n", i, rxbuf[i]);
	if (i%500 == 499) fprintf(fi, ".G2\n.bp\n");
      }
    fclose(fi);
    giveup("debug I exit");
  }

