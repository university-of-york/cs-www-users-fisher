/* Alpha document preparation system
   main module
   AJF	 December 1987 */

#include "alpha.h"
#include <stdio.h>
#include <signal.h>

#define MAXFNAMES  10			/* max. filenames on cmd line */
#define MAXCLDEFS  10			/* max. -D options	      */

extern initspawn(), spawn();		/* from /usr/fisher/lib/spawn.a */

extern readstyle();			/* from readstyle */
extern initeditbook(), processfile();	/* from edit	  */
extern initfmtbook(), tidyfmtbook();	/* from format	  */
extern bool setnumber();		/* from common	  */
extern char *lookupword();		/* from common	  */
extern initdict();			/* from common	  */

extern char *getenv();

global bool pmode, epsflag;
global int firstpage, minpage, maxpage, cldeftop;
global char *cldefs[MAXCLDEFS];
global FILE *editin, *menuin, *codeout, *messout;

static char *fnames[MAXFNAMES];		/* filename(s) of ".al" file(s) */
static int numfnames;			/* number of ".al" files        */

extern int pagewidth, pageheight;	/* style variables, defined in style file, from readstyle */


global main(argc, argv) int argc; char *argv[];
  { messout = stderr; /* just in case */
    initspawn("alpha");
    initdict();
    readcmdline(argv);
    initeditbook();
    initfmtbook();
    processbook();
    tidyfmtbook();
    exit(0);
  }

static readcmdline(argv) char *argv[];
  { int ap = 0; bool minmaxset;
    numfnames = 0;
    if (argv[ap] != NULL) ap++; /* skip program name */
    while (numfnames < MAXFNAMES && argv[ap] != NULL && argv[ap][0] != '-')
      fnames[numfnames++] = argv[ap++];
    if (argv[ap] != NULL && argv[ap][0] != '-') a_giveup("too many input files!");
    if (numfnames == 0) fnames[numfnames++] = NULL; /* means use std input */
    pmode = epsflag = minmaxset = false;
    firstpage = 1; cldeftop = 0;
    until (argv[ap] == NULL)
      { if (seq(argv[ap],"-p"))
	  { pmode = true; /* generate PostScript for printing */
	    ap++;
	  }
	else if (seq(argv[ap],"-e"))
	  { epsflag = true; /* generate "eps" (with special "%!" comment and BoundingBox) */
	    ap++;
	  }
	else if (seq(argv[ap],"-fp") && (argv[ap+1] != NULL))
	  { ap++;
	    firstpage = atoi(argv[ap++]);
	  }
	else if (seq(argv[ap],"-pg") && (argv[ap+1] != NULL))
	  { ap++;
	    minpage = atoi(argv[ap++]);
	    maxpage = (argv[ap] != NULL && argv[ap][0] != '-') ? atoi(argv[ap++]) : minpage;
	    minmaxset = true;
	  }
	else if (argv[ap][0] == '-' && argv[ap][1] == 'D')
	  { if (cldeftop >= MAXCLDEFS) a_giveup("too many -D options!");
	    cldefs[cldeftop++] = lookupword(&argv[ap][2]);
	    ap++;
	  }
	else usage();
      }
    if (!pmode && (numfnames > 1 || fnames[0] == NULL)) usage();
    unless (minmaxset)
      { minpage = firstpage;
	maxpage = MAXINT;
      }
    minpage = minpage-firstpage;	/* make minpage, maxpage zero-offset */
    maxpage = (maxpage-firstpage)+1;	/* so (maxpage-minpage) is num. of pages to print */
  }

static usage()
  { fprintf(stderr, "Usage: alpha fn          [-Dopt ...] [-fp n]\n");
    fprintf(stderr, "       alpha [fn ...] -p [-Dopt ...] [-fp n] [-pg m [n]] [-e]\n");
    exit(1);
  }

static processbook()
  { int k;
    for (k=0; k < numfnames; k++)
      { char *fn = fnames[k];
	readstyle(fn);
	if (pmode)
	  { /* print mode; doesn't run in windows; default to standard input */
	    if (fn != NULL)
	      { editin = fopen(fn, "r");
		if (editin == NULL) a_giveup("can't open \"%s\"", fn);
	      }
	    else editin = stdin;
	    menuin = codeout = NULL;
	    messout = stderr;
	  }
	else
	  { /* window (edit) mode; start window processes */
	    int wid, hgt; char *editor;
	    wid = (pagewidth+500)/1000 + 10;
	    hgt = (pageheight+500)/1000 + 24;
	    editor = getenv("ALPHA_EDITOR");
	    if (editor == NULL) editor = "/usr/fisher/bin/wordy";
	    if (access(editor,1) < 0) a_giveup("can't access editor \"%s\"", editor);
	    signal(SIGPIPE, SIG_IGN);  /* ignore pipe signals */
	    signal(SIGINT, SIG_IGN);   /* ignore interrupts, but retain default action in children */
	    spawn("/usr/fisher/lib/alpha/windows", "windows %d %d %s %s %f %f %f %f",
		  wid, hgt, editor, fn, &editin, &menuin, &codeout, &messout);
	  }
	processfile();	/* updates firstpage in preparation for next file, if any */
	closefiles();
      }
  }

static closefiles()
  { unless (editin == stdin) fclose(editin);
    unless (menuin == NULL) fclose(menuin);
    unless (codeout == NULL) fclose(codeout);
    unless (messout == stderr) fclose(messout);
  }

