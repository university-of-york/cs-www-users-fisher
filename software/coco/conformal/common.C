#include <stdio.h>
#include "hdr.h"

#define word int    /* or char* or ... */

class stats
  { double mindx, maxdx, mindy, maxdy, maxdh;
    bool any;

public:

    stats()
      { mindx = mindy = +1e10;
	maxdx = maxdy = maxdh = -1e10;
	any = false;
      }

    void update(double dx, double dy)
      { if (dx < mindx) mindx = dx;
	if (dy < mindy) mindy = dy;
	if (dx > maxdx) maxdx = dx;
	if (dy > maxdy) maxdy = dy;
	double dh = hypot(dx, dy);
	if (dh > maxdh) maxdh = dh;
	any = true;
      }

    ~stats()
      { if (any)
	  { putchar('\n');
	    printf("dx %.4f .. %.4f   dy %.4f .. %.4f   dh %.4f\n", mindx, maxdx, mindy, maxdy, maxdh);
	  }
      }
  };

static stats stats;


global void printline(pco *pc, cco *mycc, cco *pjcc)
  { printf("lon %8.4f lat %8.4f   ", pc -> lon, pc -> lat);
    printf("my x,y %13.4f %13.4f   pj x,y %13.4f %13.4f   ", mycc -> x, mycc -> y, pjcc -> x, pjcc -> y);
    double dx = (mycc -> x) - (pjcc -> x), dy = (mycc -> y) - (pjcc -> y);
    printf("dx %8.4f   dy %8.4f\n", dx, dy);
    stats.update(dx, dy);
  }

global void giveup(char *msg ...)
  { word *args = (word*) &msg;
    fprintf(stderr, msg, args[1]); putc('\n', stderr);
    exit(1);
  }

