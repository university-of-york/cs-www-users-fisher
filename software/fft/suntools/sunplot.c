#include <stdio.h>
#include <sun.h>

#define MAXDATA 40000

#define bool  char
#define false ((char) 0)
#define true  ((char) 1)

#define seq(s1,s2) (strcmp(s1,s2) == 0)

extern double log();


main (argc, argv) int argc; char **argv;
  { static double data[MAXDATA];
    int ndata = 0;
    double val, min, max, xscale, yscale; int code, i, APPLIC;
    bool logopt = false;
    if (argc >= 2)
      { if (argc == 2 && seq(argv[1],"-log")) logopt = true;
	else
	  { fprintf(stderr, "Usage: sunplot [-log]\n");
	    exit(1);
	  }
      }
    InitDisplay ();
    SetDisplayRect (Rect(300,0,1100,600));
    APPLIC = DefineInput (0); /* standard input */
    InitDevices (APPLIC | KBD | MOUSE);
    SetNamestripe ("sunplot");
    Show ();
    code = scanf (" %lf", &val);
    while ((ndata < MAXDATA) && (code>0))
      { if (logopt)
	  { if (val < 0.1e-6)
	      { fprintf(stderr, "sunplot: warning; setting log(%f) to %f\n", val, min);
		val = min;
	      }
	    else val = log(val);
	  }
	data[ndata] = val;
	if (ndata == 0) { min = val; max = val; }
	if (val<min) min = val;
	if (val>max) max = val;
	ndata++; code = scanf (" %lf", &val);
      }
    if (code>0) fprintf (stderr, "plotit: warning; data ignored\n");
    xscale = 750.0 / (ndata-1);
    yscale = 550.0 / (max-min);
    move (Pt(25,575)); line (Pt(775,575), F_SET);
    for (i=0; i<ndata; i++)
      { double x = i*xscale;
	move (Pt(25+(int)x,582)); line (Pt(25+(int)x,575), F_SET);
      }
    for (i=0; i<ndata; i++)
      { double x = i*xscale;
	double y = (data[i]-min)*yscale;
	if (i==0) move (Pt(25+(int)x,575-(int)y)); else line (Pt(25+(int)x,575-(int)y), F_SET);
      }
    fprintf (stderr, "ndata %d   X range 0 %d   Y range %lf %lf\n", ndata, ndata-1, min, max);
    converse (xscale, 400.0/ndata);
  }

converse (xscale, fac) double xscale, fac;
  { bool running = true;
    while (running)
      { int w = Wait (KBD | MOUSE);
	if (w & KBD)
	  { if (kbdchar() >= 0) running = false;
	  }
	if (w & MOUSE)
	  { if (mouse.buttons != 0)
	      { stringf (Display, mouse.p, defont, F_STORE, "%lf Hz", (mouse.p.x-25)/xscale*fac);
		while (mouse.buttons != 0) Wait (MOUSE);
	      }
	  }
      }
  }

