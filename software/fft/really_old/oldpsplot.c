#include <stdio.h>

#define MAXDATA 4096
#define PATHLIMIT 200


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
    xscale = 720.0 / (ndata-1);
    yscale = 504.0 / (max-min);
    printf ("-90 rotate -780 60 translate\n");
    argc--; argv++;
    if (argc>0)
      { printf ("/NewCenturySchlbk-Roman findfont 12 scalefont setfont\n");
	printf ("490 450 moveto (%s) show\n", *argv);
      }
    printf ("newpath\n");
    printf ("0 0 moveto 720 0 lineto\n");
    for (i=0; i<ndata; i++)
      { double x = i*xscale;
	double y = (data[i]-min)*yscale;
	printf ("%d %d ", (int)x, (int)y);
	printf (i==0 ? "moveto" : "lineto");
	putchar ('\n');
	if (i%PATHLIMIT == 0)
	  { printf("stroke newpath\n");
	    printf("%d %d moveto\n", (int)x, (int)y);
	  }
      }
    printf ("stroke showpage\n");
    fprintf (stderr, "ndata %d   X range 0 %d   y range %lf %lf\n", ndata, ndata-1, min, max);
  }
