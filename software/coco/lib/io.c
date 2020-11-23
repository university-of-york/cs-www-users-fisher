/* Routines for printing (formatting) and decoding polar & OS coords
   A.J. Fisher	 July 1995 */

#include <stdio.h>
#include "hdr.h"

/* OS 100km and 500km squares; table index first by N, then by E */
static char sqtab[5][5] =
  { { 'V', 'W', 'X', 'Y', 'Z' },
    { 'Q', 'R', 'S', 'T', 'U' },
    { 'L', 'M', 'N', 'O', 'P' },
    { 'F', 'G', 'H', 'J', 'K' },
    { 'A', 'B', 'C', 'D', 'E' },
  };

extern double fmod();
extern char *strchr();

forward double getpcoord(), dmstof(), dectof();


global gettriple(argv, tc) char *argv[]; struct triple *tc;
  { /* Decode polar coords, return a struct triple */
    tc -> lon = getpcoord(argv[0]);
    tc -> lat = getpcoord(argv[1]);
    tc -> hgt = dectof(argv[2]);
  }

global getpco(argv, pc) char *argv[]; struct pco *pc;
  { /* Decode polar coords, return a struct pco */
    pc -> lon = getpcoord(argv[0]);
    pc -> lat = getpcoord(argv[1]);
  }

static double getpcoord(s) char *s;
  { if (charcount(s,'.') > 1) badformat();
    return ((charcount(s,':') > 0) ? dmstof : dectof) (s);
  }

static int charcount(s, c) char *s; char c;
  { int n = 0, p = 0;
    until (s[p] == '\0')
      { if (s[p] == c) n++;
	p++;
      }
    return n;
  }

global getcco(argc, argv, cc) int argc; char *argv[]; struct cco *cc;
  { /* Decode Cartesian coords, return a struct cco */
    struct cco cco1, cco2;
    bool sql = false;
    unless (argc == 3 || argc == 4) badformat();
    if (argc == 4)
      { char *str = argv[1]; /* square letters */
	unless (strlen(str) == 2) badformat();
	decodesq(str[0], &cco1);
	decodesq(str[1], &cco2);
	argc--; argv++; sql = true;
      }
    cc -> x = dectof(argv[1]);
    if (sql) cc -> x += ((cco1.x - 2.0) * 5e5) + (cco2.x * 1e5);
    cc -> y = dectof(argv[2]);
    if (sql) cc -> y += ((cco1.y - 1.0) * 5e5) + (cco2.y * 1e5);
  }

static decodesq(ch, cc) char ch; struct cco *cc;
  { int esq, nsq; bool found = false;
    for (nsq=0; nsq < 5 && !found; nsq++)
      { for (esq=0; esq < 5 && !found; esq++)
	  { if (sqtab[nsq][esq] == ch) found = true;
	  }
      }
    unless (found) badformat();
    cc -> x = esq-1;
    cc -> y = nsq-1;
  }

static double dmstof(s) char *s;
  { int ni; double x1, x2, x3, ang; char junk;
    bool neg = false;
    if (s[0] == '-') { neg = true; s++; }
    ni = sscanf(s, "%lf:%lf:%lf%c", &x1, &x2, &x3, &junk);
    unless (ni == 3) badformat();
    ang = x1 + (x2 / 60.0) + (x3 / 3600.0);
    if (neg) ang = -ang;
    return ang;
  }

static double dectof(s) char *s;
  { int ni; double x; char junk;
    ni = sscanf(s, "%lf%c", &x, &junk);
    unless (ni == 1) badformat();
    return x;
  }

static badformat()
  { fprintf(stderr, "Format error in input.\n");
    fprintf(stderr, "See README for details of input and output formats.\n");
    exit(1);
  }

global printtriple(tc, datum) struct triple *tc; char *datum;
  { /* Print polar coords */
    printf("lon "); prpco(tc -> lon); putchar(' ');
    printf("lat "); prpco(tc -> lat); putchar(' ');
    printf("hgt %.3f datum %s", tc -> hgt, datum);
  }

global printpco(pc, datum) struct pco *pc; char *datum;
  { /* Print polar coords */
    printf("lon "); prpco(pc -> lon); putchar(' ');
    printf("lat "); prpco(pc -> lat); putchar(' ');
    printf("datum %s", datum);
  }

static prpco(x, str) double x; char *str;
  { printf("%.6f : ", x);
    if (x < 0.0) { putchar('-'); x = -x; }
    printf("%d:", (int) x);
    x = fmod(x, 1.0) * 60.0;
    printf("%02d:", (int) x);
    x = fmod(x, 1.0) * 60.0;
    printf("%07.4f", x);
  }

global printcco(cc) struct cco *cc;
  { /* print Cartesian coords */
    double x = cc -> x, y = cc -> y;
    printf("%.4f E %.4f N : ", x, y);   /* coords in metres */
    if ((x >= 0.0 && x < 1.0e6) && (y >= 0.0 && y < 1.5e6))
      { x /= 5e5; y /= 5e5;
	putchar(sqtab[(int)y+1][(int)x+2]);
	x = fmod(x, 1.0) * 5.0;
	y = fmod(y, 1.0) * 5.0;
	putchar(sqtab[(int)y][(int)x]);
	x = fmod(x, 1.0) * 1e5;
	y = fmod(y, 1.0) * 1e5;
	printf(" %05ld %05ld", (long) (x+0.5), (long) (y+0.5));
      }
  }

