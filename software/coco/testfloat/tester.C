#include <stdio.h>
#include "hdr.h"

extern "C" double hypot(double, double);

static void convert(char*, pco*, cco*);


global int main()
  { double mindx = +1e10, maxdx = -1e10;
    double mindy = +1e10, maxdy = -1e10;
    double maxdh = -1e10;
    for (int ilon = -6; ilon <= +2; ilon++)
      { for (int ilat = 40; ilat < 70; ilat++)
	// for (int ilat = 0; ilat < 90; ilat++)
	  { pco pco; cco mycco, dblcco; double dx, dy, dh;
	    pco.lon = (double) ilon;
	    pco.lat = (double) ilat;
	    convert(".", &pco, &mycco);                 /* float vsn */
	    convert("../series", &pco, &dblcco);        /* double ("correct") vsn */
	    printf("lat %2d lon %+2d   ", ilat, ilon);
	    printf("my x,y %12.4f %12.4f   dbl x,y %12.4f %12.4f   ", mycco.x, mycco.y, dblcco.x, dblcco.y);
	    dx = mycco.x - dblcco.x; dy = mycco.y - dblcco.y; dh = hypot(dx, dy);
	    printf("dx %8.4f   dy %8.4f   ", dx, dy);
	    putchar('\n');
	    if (dx < mindx) mindx = dx;
	    if (dy < mindy) mindy = dy;
	    if (dx > maxdx) maxdx = dx;
	    if (dy > maxdy) maxdy = dy;
	    if (dh > maxdh) maxdh = dh;
	  }
	putchar('\n');
      }
    printf("dx %.4f .. %.4f   dy %.4f .. %.4f   ", mindx, maxdx, mindy, maxdy);
    printf("dh %.4f\n", maxdh);
    return 0;
  }

static void convert(char *dir, pco *pc, cco *cc)
  { char cmd[256]; sprintf(cmd, "%s/polos %14.10f %14.10f", dir, pc -> lon, pc -> lat);
    FILE *fi = popen(cmd, "r");
    if (fi == NULL)
      { fprintf(stderr, "popen failed\n");
	exit(1);
      }
    int ni = fscanf(fi, "%*[^>]>%lf E %lf N :", &cc -> x, &cc -> y);
    pclose(fi);
    unless (ni == 2)
      { fprintf(stderr, "fscanf failed (%d)\n", ni);
	exit(1);
      }
  }

