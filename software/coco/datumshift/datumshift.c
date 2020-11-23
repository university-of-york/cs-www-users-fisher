/* datumshift -- adjust lon/lat for datum shift
   Driver program
   A.J. Fisher	 September 1995 */

#include <stdio.h>
#include <hdr.h>


global main(argc, argv) int argc; char *argv[];
  { struct triple pco1, pco2;
    char *isys = NULL, *osys = NULL;
    bool gotpco = false;
    if (argc > 0) { argc--; argv++; } /* skip program name */
    while (argc > 0)
      { char *s = argv[0];
	if (seq(s, "-I") && argc >= 2)
	  { if (isys != NULL) usage();
	    isys = argv[1];
	    argc -= 2; argv += 2;
	  }
	else if (seq(s, "-O") && argc >= 2)
	  { if (osys != NULL) usage();
	    osys = argv[1];
	    argc -= 2; argv += 2;
	  }
	else if (argc >= 3)
	  { if (gotpco) usage();
	    gettriple(argv, &pco1);
	    gotpco = true;
	    argc -= 3; argv += 3;
	  }
	else usage();
      }
    unless (gotpco) usage();
    if (isys == NULL) isys = "WGS84";
    if (osys == NULL) osys = "WGS84";
    datumshift(&pco1, &pco2, isys, osys);
    printf("    "); printtriple(&pco1, isys); putchar('\n');
    printf("--> "); printtriple(&pco2, osys); putchar('\n');
    exit(0);
  }

static usage()
  { fprintf(stderr, "Usage: datumshift [-I xxx] [-O xxx] lon lat hgt\n");
    fprintf(stderr, "See README for details of input and output formats.\n");
    exit(1);
  }

