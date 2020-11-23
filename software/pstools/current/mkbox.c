#define global
#define unless(x)   if(!(x))
#define until(x)    while(!(x))
#define uint	    unsigned int
#define bool	    int
#define false	    0
#define true	    1
#define seq(s1,s2)  (strcmp(s1,s2) == 0)

#include <stdio.h>

extern double atof();

static int nwds;


global main(argc, argv) int argc; char *argv[];
  { float wid, hgt, lth;
    unless (argc == 5) usage();
    unless (seq(argv[1],"-b") || seq(argv[1],"-p")) usage();
    wid = atof(argv[2]); hgt = atof(argv[3]); lth = atof(argv[4]);
    unless (wid >= 0.0 && hgt >= 0.0 && lth >= 0.0) usage();
    if (wid == 0.0 && hgt == 0.0)
      { fprintf(stderr, "mkbox: empty box!\n");
	exit(1);
      }
    (argv[1][1] == 'b') ? makebm(wid, hgt, lth) : makeps(wid, hgt, lth);
    exit(0);
  }

static usage()
  { fprintf(stderr, "Usage: mkbox -[bp] wid hgt lth\n");
    exit(1);
  }

static makebm(wid, hgt, lth) float wid, hgt, lth;
  { int iwid = (int) (wid+0.5), ihgt = (int) (hgt+0.5);
    if (iwid == 0) iwid = 1;
    if (ihgt == 0) ihgt = 1;
    printf("DeclareBitmap(bitmap, %d, %d, bitmap_bits)\n", iwid, ihgt);
    printf("short bitmap_bits[] =\n");
    nwds = 0;
    wrline(iwid, 1);
    if (ihgt >= 2)
      { int i;
	for (i=0; i < ihgt-2; i++) wrline(iwid, 0);
	wrline(iwid, 1);
      }
    if (nwds%8 != 0) putchar('\n');
    printf("  }\n");
  }

static wrline(iwid, bit) int iwid; uint bit;
  { if (iwid <= 16)
      { wrword((bit ? 0xffff : 0x0001) << 16-iwid | 0x8000);
      }
    else
      { wrword(bit ? 0xffff : 0x8000);
	iwid -= 16;
	while (iwid > 16)
	  { wrword(bit ? 0xffff : 0x0000);
	    iwid -= 16;
	  }
	wrword((bit ? 0xffff : 0x0001) << 16-iwid);
      }
  }

static wrword(wd) uint wd;
  { if (nwds%8 == 0) printf((nwds == 0) ? "  { " : "    ");
    printf("0x%04x, ", wd & 0xffff);
    if (++nwds%8 == 0) putchar('\n');
  }

static makeps(wid, hgt, lth) float wid, hgt, lth;
  { printf("%%! mkbox -p %.3f %.3f %.3f\n", wid, hgt, lth);
    printf("%%%%BoundingBox: 0 0 %d %d\n", (int) wid + 1, (int) hgt + 1);
    printf("%.3f setlinewidth\n", lth);
    printf("newpath 0 0 moveto ");
    if (wid == 0.0) printf("0 %.3f lineto ", hgt);      /* vert line  */
    else if (hgt == 0.0) printf("%.3f 0 lineto ", wid); /* horiz line */
    else printf("%.3f 0 lineto %.3f %.3f lineto 0 %.3f lineto closepath ", wid, wid, hgt, hgt);
    printf("stroke\n");
  }

