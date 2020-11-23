/* Modem for MIPS   AJF	  January 1995
   Send a document (T.4), in Group 3 format on stdin */

#include <stdio.h>
#include "modem.h"

#define SEGSIZE	 96000	  /* was 64000 */
#define MAXLINES 2400
#define VERSION	 1

static uchar *docpage;
static int pagebits, pagenum, ch;

static void formaterror();


global void initdoc()
  { int vsn, xres, yres;
    int ni = scanf("!<Group 3> %d %d %d\n", &vsn, &xres, &yres);
    unless (ni == 3 && vsn == VERSION && xres == 200 && (yres == 100 || yres == 200)) formaterror();
    if (yres == 200) options |= opt_H;
    pagenum = 0;
    docpage = NULL;
    ch = getchar(); /* read 1 char ahead */
    morepages = (ch >= 0);
  }

global void readdocpage()
  { /* Read a single page */
    unless (morepages) giveup("Bug! readdocpage");
    unless (docpage == NULL) delete docpage;
    docpage = new uchar[SEGSIZE];
    int *blvec = new int[MAXLINES];
    pagenum++; pagebits = 0;
    uint prevbits = ~0; int nbytes = 0, neols = 0, blnum = 0;
    until (nbytes >= SEGSIZE || neols >= 6)
      { uchar n;
	if (ch < 0) formaterror();
	docpage[nbytes++] = n = ch;
	for (int j=0; j < 8 && neols < 6; j++)
	  { prevbits = (prevbits << 1) | (n >> 7);
	    n <<= 1; pagebits++;
	    /* 7 consecutive EOLs mark end of page; note that 2 consec. EOLs never occur in body of page */
	    if ((prevbits & 0xffffff) == 0x001001) neols++;
	    if ((prevbits & 0x1fffffff) == 0x09b35001)	/* 1728 white + 0 white + EOL, i.e. blank line */
	      { if (blnum >= MAXLINES) giveup("Page %d: too long!", pagenum);
		blvec[blnum++] = pagebits;   /* keep track of ends of blank lines */
	      }
	  }
	ch = getchar();
      }
    if (neols < 6) giveup("Page %d: too big!", pagenum);
    /* delete RTC signal (6 EOLs), and trailing blank lines */
    pagebits -= 72;
    while (blnum > 0 && blvec[blnum-1] == pagebits) { pagebits -= 29; blnum--; }
    nbytes = (pagebits+7) / 8;
    infomsg("Page %2d: %5d bytes, %6d bits", pagenum, nbytes, pagebits);
    docpage = (uchar*) realloc(docpage, nbytes);    /* reduce size; always succeeds */
    delete blvec;
    morepages = (ch >= 0);
  }

static void formaterror()
  { giveup("Input is not in Group 3 format");
  }

global void senddocpage()
  { if (options & opt_v) fprintf(stderr, ">>> Page %d\n", pagenum);
    uchar *ptr = docpage;
    int nb = 0;
    int lb = scanbits; /* don't pad first EOL */
    uint prevbits = ~0;
    while (nb < pagebits)
      { uchar n = *(ptr++);
	for (int j=0; j < 8 && nb < pagebits; j++)
	  { uchar bit = n >> 7;
	    prevbits = (prevbits << 1) | bit;
	    if ((prevbits & 0xfff) == 0x001)	/* EOL */
	      { while (lb++ < scanbits) putbit(0);  /* pad scan line to achieve min. scan time */
		lb = 0;
	      }
	    putbit(bit);
	    lb++; nb++; n <<= 1;
	  }
      }
    /* send RTC (6 further EOLs) plus 2 more "for luck", to flush tribit buffer */
    for (int i=0; i < 8; i++)
      { for (int j=0; j < 11; j++) putbit(0);
	putbit(1);
      }
  }

