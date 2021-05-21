/* generate .gif circuit diagram
   AJF	 September 1999 */

/* Uses the "gd" gif manipulation library from
   Quest Protein Database Center   <http://siva.cshl.org/gd/gd.html> */

#include <stdio.h>
#include <string.h>

extern "C" {
    #include <gd.h>
    #include <gdfonts.h>
};

#include "solvecct.h"

#define BORDER	 40
#define HUGE	 1000000

#define VERT	 0
#define HORIZ	 1

static gdImage *image;
static int black, posx, posy;

static void drawcomp(component*);
static void draw_wire(component*), draw_resistor(component*), draw_capacitor(component*), draw_inductor(component*);
static void draw_isource(component*), draw_vsource(component*), draw_vmeter(component*);
static void draw_opamp(component*);
static void drawlabel(component*), drawnodenums(component*);

static void drawline(int, int, int, int);
static void drawarc(int, int, int, int, int, int);
static void drawint(int, int, int), drawtext(char*, int, int);


global void drawcircuit(component **cvec, int nc, char *fn)
  { /* work out extent of circuit */
    int minx = +HUGE, miny = +HUGE, maxx = -HUGE, maxy = -HUGE;
    for (int i = 0; i < nc; i++)
      { component *c = cvec[i];
	int px = c -> px, py = c -> py;
	int dx = (c -> orient == VERT) ? 0 : c -> length;
	int dy = (c -> orient == VERT) ? c -> length : 0;
	if (px < minx) minx = px;
	if (py < miny) miny = py;
	if (px+dx > maxx) maxx = px+dx;
	if (py+dy > maxy) maxy = py+dy;
      }
    /* shift all components to top left */
    for (int i = 0; i < nc; i++)
      { component *c = cvec[i];
	c -> px -= minx;
	c -> py -= miny;
      }
    /* allocate image just big enough */
    if (maxx <= minx || maxy <= miny) giveup("Bug! bad image size");
    image = gdImageCreate(maxx-minx + 2*BORDER, maxy-miny + 2*BORDER);
    gdImageColorAllocate(image, 255, 255, 255); /* white background */
    black = gdImageColorAllocate(image, 0, 0, 0);
    /* draw  the circuit */
    for (int i = 0; i < nc; i++)
      { component *c = cvec[i];
	posx = c -> px; posy = c -> py;
	drawcomp(c);
	drawlabel(c);
	drawnodenums(c);
      }
    /* write it out */
    FILE *fi = fopen(fn, "w");
    if (fi == NULL) giveup("can't create %s", fn);
    gdImageGif(image, fi);
    fclose(fi);
    gdImageDestroy(image);
  }

static void drawcomp(component *c)
  { switch (c -> type)
      { case 0:
	    draw_wire(c);
	    break;

	case 1:
	    draw_resistor(c);
	    break;

	case 2:
	    draw_capacitor(c);
	    break;

	case 3:
	    draw_inductor(c);
	    break;

	case 4:
	    draw_isource(c);
	    break;

	case 5:
	    draw_vsource(c);
	    break;

	case 6:
	    draw_vmeter(c);
	    break;

	case 7:	  case 8:
	    draw_opamp(c);
	    break;
      }
  }

static void draw_wire(component *c)
  { int dx = (c -> orient == VERT) ? 0 : c -> length;
    int dy = (c -> orient == VERT) ? c -> length : 0;
    drawline(0, 0, dx, dy);
  }

static void draw_resistor(component *c)
  { switch (c -> orient)
      { case VERT:
	    drawline(0, 0, 0, 20);
	    drawline(0, 20, 5, 25);
	    drawline(5, 25, -5, 35);
	    drawline(-5, 35, 5, 45);
	    drawline(5, 45, -5, 55);
	    drawline(-5, 55, 0, 60);
	    drawline(0, 60, 0, 80);
	    break;

	case HORIZ:
	    drawline(0, 0, 20, 0);
	    drawline(20, 0, 25, 5);
	    drawline(25, 5, 35, -5);
	    drawline(35, -5, 45, 5);
	    drawline(45, 5, 55, -5);
	    drawline(55, -5, 60, 0);
	    drawline(60, 0, 80, 0);
	    break;
      }
  }

static void draw_capacitor(component *c)
  { switch (c -> orient)
      { case VERT:
	    drawline(0, 0, 0, 35);
	    drawline(-10, 35, 10, 35);
	    drawline(-10, 45, 10, 45);
	    drawline(0, 45, 0, 80);
	    break;

	case HORIZ:
	    drawline(0, 0, 35, 0);
	    drawline(35, -10, 35, 10);
	    drawline(45, -10, 45, 10);
	    drawline(45, 0, 80, 0);
	    break;
      }
  }

static void draw_inductor(component *c)
  { switch (c -> orient)
      { case VERT:
	  { drawline(0, 0, 0, 20);
	    int y = 20;
	    for (int i = 0; i < 4; i++)
	      { drawarc(0, y+5, 10, 10, 270, 450);
		y += 10;
	      }
	    drawline(0, 60, 0, 80);
	    break;
	  }

	case HORIZ:
	  { drawline(0, 0, 20, 0);
	    int x = 20;
	    for (int i = 0; i < 4; i++)
	      { drawarc(x+5, 0, 10, 10, 180, 360);
		x += 10;
	      }
	    drawline(60, 0, 80, 0);
	    break;
	  }
      }
  }

static void draw_isource(component *c)
  { switch (c -> orient)
      { case VERT:
	    drawline(0, 0, 0, 27);
	    drawarc(0, 35, 16, 16, 0, 360);
	    drawarc(0, 45, 16, 16, 0, 360);
	    drawline(0, 53, 0, 80);
	    break;

	case HORIZ:
	    drawline(0, 0, 27, 0);
	    drawarc(35, 0, 16, 16, 0, 360);
	    drawarc(45, 0, 16, 16, 0, 360);
	    drawline(53, 0, 80, 0);
	    break;
      }
  }

static void draw_vsource(component *c)
  { switch (c -> orient)
      { case VERT:
	    drawline(0, 0, 0, 32);
	    drawarc(0, 40, 16, 16, 0, 360);
	    drawline(0, 48, 0, 80);
	    break;

	case HORIZ:
	    drawline(0, 0, 32, 0);
	    drawarc(40, 0, 16, 16, 0, 360);
	    drawline(48, 0, 80, 0);
	    break;
      }
  }

static void draw_vmeter(component *c)
  { draw_vsource(c);
    switch (c -> orient)
      { case VERT:
	    drawtext("M", -2, 40);
	    break;

	case HORIZ:
	    drawtext("M", 38, 0);
	    break;
      }
  }

static void draw_opamp(component *c)
  { drawline(20, -10, 0, -10);	  /* top input */
    drawline(20, 10, 0, 10);	  /* bot input */
    drawline(60, 0, 80, 0);	  /* output    */
    drawline(20, -20, 20, 20);
    drawline(20, 20, 60, 0);
    drawline(60, 0, 20, -20);
    if (c -> type == 7)
      { drawtext("+", 24, -10);
	drawtext("-", 24, +10);
      }
    else
      { drawtext("-", 24, -10);
	drawtext("+", 24, +10);
      }
  }

static void drawlabel(component *c)
  { char *s = c -> name;
    unless (s == NULL)
      { switch (c -> orient)
	  { case VERT:
		drawtext(s, 13, 40);
		break;

	    case HORIZ:
	      { int wid = strlen(s) * (gdFontSmall -> w);
		drawtext(s, 40 - wid/2, (c -> type == 2) ? -17 : -12);	/* draw value a bit higher on horiz cap */
		break;
	      }
	  }
      }
  }

static void drawnodenums(component *c)
  { unless (c -> type == 7 || c -> type == 8)	/* skip for opamps (non-standard connections) */
      { int dx = (c -> orient == VERT) ? 0 : c -> length;
	int dy = (c -> orient == VERT) ? c -> length : 0;
	drawint(c -> n1, 3, -5);
	drawint(c -> n2, dx+3, dy-5);
      }
  }

static void drawline(int x1, int y1, int x2, int y2)
  { gdImageLine(image, BORDER+posx+x1, BORDER+posy+y1, BORDER+posx+x2, BORDER+posy+y2, black);
  }

static void drawarc(int x, int y, int w, int h, int th1, int th2)
  { gdImageArc(image, BORDER+posx+x, BORDER+posy+y, w, h, th1, th2, black);
  }

static void drawint(int n, int x, int y)
  { char vec[32]; sprintf(vec, "%d", n);
    drawtext(vec, x, y);
  }

static void drawtext(char *s, int x, int y)
  { gdImageString(image, gdFontSmall, BORDER+posx+x, BORDER+posy+y-5, s, black);
  }

