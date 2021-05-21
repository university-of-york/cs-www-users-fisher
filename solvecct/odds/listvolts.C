static void listvoltages()
  { fprintf(htmlout, "<h2> Node Voltages </h2>\n");
    fprintf(htmlout, "<table border>\n");
    fprintf(htmlout, "<tr> <th> Frequency (Hz)");
    for (int j = 0; j < numvisnodes; j++) fprintf(htmlout, "<th> Node %3d (V)", j+2);
    putc('\n', htmlout);
    int ns = (minfreq == maxfreq) ? 1 : NUMFSTEPS;
    for (int fs = 0; fs < ns; fs++)
      { int i;
	double alpha = fs / (double) (NUMFSTEPS-1);
	double freq = minfreq + alpha * (maxfreq-minfreq);
	fprintf(htmlout, "<tr> <td> %11.4e Hz ", freq);
	for (i = 0; i < numvisnodes; i++)
	  { complex z = nvmat[fs][i];
	    fprintf(htmlout, "<td> %11.4e + %11.4e&nbsp;j ", z.re, z.im);
	  }
	putc('\n', htmlout);
      }
    fprintf(htmlout, "</table> <p>\n");
  }
