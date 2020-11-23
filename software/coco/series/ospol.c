/* ospol -- OS-to-polar coordinate conversion
   Driver program
   A.J. Fisher	 July 1995 */

#include <stdio.h>
#include <hdr.h>


global main(argc, argv) int argc; char *argv[];
  { struct cco cco; struct pco pco;
    unless (argc == 3 || argc == 4) usage();
    getcco(argc, argv, &cco);
    os_to_polar(&cco, &pco);
    printf("    "); printcco(&cco); putchar('\n');
    printf("--> "); printpco(&pco, "OSGB36"); putchar('\n');
    exit(0);
  }

static usage()
  { fprintf(stderr, "Usage: ospol east north\n");
    fprintf(stderr, "       ospol XX eeeee nnnnn\n");
    fprintf(stderr, "See README for details of input and output formats.\n");
    exit(1);
  }

