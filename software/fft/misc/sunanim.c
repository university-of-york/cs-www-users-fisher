#include <stdio.h>
#include <sun.h>

#define MAXDATA 40000
#define NUMI 31
#define NUMJ 512

#define bool  char
#define false ((char) 0)
#define true  ((char) 1)


main (argc, argv) int argc; char **argv;
  { static double data[MAXDATA];
    int ndata = 0;
    double val, min, max, xscale, yscale; int code, i, j, APPLIC;
    InitDisplay ();
    SetDisplayRect (Rect(300,0,1100,600));
    APPLIC = DefineInput (0); /* standard input */
    InitDevices (APPLIC | KBD | MOUSE);
    SetNamestripe ("sunplot");
    Show ();
    code = scanf (" %lf", &val);
    while ((ndata < MAXDATA) && (code>0))
      { data[ndata] = val;
	if (ndata == 0) { min = val; max = val; }
	if (val<min) min = val;
	if (val>max) max = val;
	ndata++; code = scanf (" %lf", &val);
      }
    if (code>0) fprintf (stderr, "plotit: warning; data ignored\n");
    xscale = 750.0 / (NUMJ-1);
    yscale = 550.0 / (max-min);
    move (Pt(25,575)); line (Pt(775,575), F_SET);
    for (j=0; j<NUMJ; j++)
      { double x = j*xscale;
	move (Pt(25+(int)x,582)); line (Pt(25+(int)x,575), F_SET);
      }
    for (i=0; i<NUMI; i++)
      { rect (Display, Rect(0,0,800,574), F_CLR);
	for (j=0; j<NUMJ; j++)
	  { double x = j*xscale;
	    double y = (data[i*NUMJ+j]-min)*yscale;
	    if (j==0) move (Pt(25+(int)x,575-(int)y)); else line (Pt(25+(int)x,575-(int)y), F_SET);
	  }
	nap (30);
      }
    fprintf (stderr, "ndata %d   X range 0 255   Y range %lf %lf    Z range 0 %d\n", ndata, min, max, NUMI-1);
    converse (xscale, 400.0/NUMJ);
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

