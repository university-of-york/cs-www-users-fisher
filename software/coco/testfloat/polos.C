/* polos -- polar-to-OS coordinate conversion
   Driver program
   A.J. Fisher	 July 1995 */

#include <stdio.h>
#include "hdr.h"

static void usage();


global int main(int argc, char *argv[])
  { pco pco; cco cco;
    unless (argc == 3) usage();
    getpco(&argv[1], &pco);
    polar_to_os(&pco, &cco);
    printf("    "); printpco(&pco, "OSGB36"); putchar('\n');
    printf("--> "); printcco(&cco); putchar('\n');
    return 0;
  }

static void usage()
  { fprintf(stderr, "Usage: polos lon lat\n");
    fprintf(stderr, "See README for details of input and output formats.\n");
    exit(1);
  }

