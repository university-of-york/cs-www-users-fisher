static void printderivs()
  { int j;
    for (j=0; j < NDERIVS; j++)
      { printf("Deriv %2d: linterm %14.6e\n", j, linterm[j]);
	if (j & 1) prcoeffs("B:", bvec[j]); else prcoeffs("A:", avec[j]);
	putchar('\n');
      }
  }

static void printabgd()
  { prcoeffs('A', alpha);
    prcoeffs('B', beta);
    prcoeffs('G', gamma);
    prcoeffs('D', delta);
  }

static void printabgd()
  { prcoeffs("AD", alphadelta);
  }

static void prcoeffs(char *s, double vec[])
  { int i;
    for (i=0; i < NHARMS; i++)
      { if (i%8 == 0) { fputs(s, stdout); s = "  "; }
	printf("%14.6e", vec[i]);
	if (i%8 == 7) putchar('\n');
      }
    unless (NHARMS%8 == 0) putchar('\n');
  }


