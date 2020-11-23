/* G2PSG parser generator   A.J. Fisher	  September 1990 */
/* Program for preparing error message file */

#define global
#define until(x) while (!(x))

#include <stdio.h>

#define INFN   "emf.text"
#define OUTFN  "/usr/fisher/lib/g2psg.emf"
#define RECLEN 80 /* record length */


global main()
  { char s[RECLEN+1];
    FILE *in, *out; char *code;
    in = fopen(INFN, "r");
    if (in == NULL)
      { fprintf(stderr, "trans: can't open %s\n", INFN);
	exit(1);
      }
    out = fopen(OUTFN, "w");
    if (out == NULL)
      { fprintf(stderr, "trans: can't create %s\n", OUTFN);
	exit(1);
      }
    code = fgets(s, RECLEN+1, in);
    until (code == NULL)
      { int k = 0;
	until (s[k] == '\0') putc(s[k++], out);
	while (k < RECLEN) { putc('\0', out); k++; }
	code = fgets(s, RECLEN+1, in);
      }
    fflush(out);
    if (ferror(out))
      { fprintf(stderr, "trans: error writing %s\n", OUTFN);
	exit(1);
      }
    fclose(in); fclose(out);
    exit(0);
  }

