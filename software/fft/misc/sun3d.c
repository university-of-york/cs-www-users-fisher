#include <stdio.h>
#include <sun.h>

#define MAXDATA 40000

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
    xscale = 494.0 / 255;
    yscale = 294.0 / (max-min);
    for (i=0; i<62; i++)
      { int bx = 25+(4*i), by = 281-(4*i);
	xmove (bx,by); xline (bx+494,by);
	for (j=0; j<256; j++)
	  { double x = j*xscale;
	    double y = (data[256*i+j]-min)*yscale;
	    xmove (bx+(int)x,by);
	    xline (bx+(int)x,by+(int)y);
	  }
      }
    rect (Display, Dr, F_XOR);
    fprintf (stderr, "ndata %d   X range 0 255   Y range %lf %lf   Z range 0 61\n", ndata, min, max);
    converse (xscale, 400.0/256);
  }

xmove (x, y) int x, y;
  { move (Pt ((short)x, (short)(600-y)));
  }

xline (x, y) int x, y;
  { line (Pt ((short)x, (short)(600-y)), F_SET);
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

