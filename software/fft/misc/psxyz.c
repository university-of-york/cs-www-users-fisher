#include <stdio.h>

#define MAXDATA 40000
#define PATHLIMIT 200

#define INUM 40
#define JNUM 800

main (argc, argv) int argc; char **argv;
  { static double data[MAXDATA];
    int ndata = 0;
    double val, min, max, xscale, yscale; int i;
    int code = scanf (" %lf", &val);
    while ((ndata < MAXDATA) && (code>0))
      { data[ndata] = val;
	if (ndata == 0) { min = val; max = val; }
	if (val<min) min = val;
	if (val>max) max = val;
	ndata++; code = scanf (" %lf", &val);
      }
    if (code>0) fprintf (stderr, "plotit: warning; data ignored\n");
    xscale = 720.0 / (JNUM-1);
    yscale = 10.0 / (max-min);
    printf ("/M { moveto } def\n");
    printf ("/L { lineto } def\n");
    printf ("-90 rotate -780 60 translate\n");
    argc--; argv++;
    if (argc>0)
      { printf ("/NewCenturySchlbk-Roman findfont 12 scalefont setfont\n");
	printf ("490 450 moveto (%s) show\n", *argv);
      }
    for (i=0; i<INUM; i++)
      { int j;
	printf ("newpath\n");
	for (j=0; j<JNUM; j++)
	  { double x = j*xscale;
	    double y = (data[JNUM*i+j]-min)*yscale;
	    printf ("%d %lf ", (int)x, 12.5*i+y);
	    putchar (j==0 ? 'M' : 'L');
	    putchar ('\n');
	  }
	printf ("stroke\n");
      }
    printf ("showpage\n");
    fprintf (stderr, "ndata %d   X range 0 %d   y range %lf %lf\n", ndata, ndata-1, min, max);
  }
