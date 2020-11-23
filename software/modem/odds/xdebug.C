static void writedebug()
  { FILE *fi = fopen("debug.grap", "w");
    if (fi == NULL) giveup("debug can't create");
    until (timenow%2400 == 0) debugvec[timenow++] = debug(0.0, 0.0, 0.0);
    for (int j=0; j < 3; j++)
      { for (int i=72000; i < timenow; i++)
	  { if (i%2400 == 0)
	      { fprintf(fi, ".sp 0.5i\nj=%d   i=%d\n.br\n", j, i);
		fprintf(fi, ".G1 8i\n");
		static float ticks[3] = { 3e-4, 3e-2, 1e-4 }, tstep[3] = { 1e-4, 5e-3, 2e-5 };
		fprintf(fi, "ticks left from %g to %g by %g\n", -ticks[j], +ticks[j], tstep[j]);
		fprintf(fi, "new solid\n");
	      }
	    fprintf(fi, "%d %g\n", i, debugvec[i].vals[j]);
	    if (i%2400 == 2399) fprintf(fi, ".G2\n.bp\n");
	  }
      }
    fclose(fi);
  }

