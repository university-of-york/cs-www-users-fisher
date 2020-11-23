/* Alpha document preparation system
   main module
   AJF	 December 1987 */

#include <signal.h>
#include <spawn.h>

#include "alpha.h"

#define MAXFNAMES  10			/* max. filenames on cmd line */
#define MAXCLDEFS  10			/* max. -D options	      */

#ifdef mips
#define WINDOWS	   "/usr/fisher/mipslib/alpha/windows"
#define WORDY	   "/usr/fisher/mipsbin/wordy"
#endif

#ifdef sun
#define WINDOWS	   "/usr/fisher/sunlib/alpha/windows"
#define WORDY	   "/usr/fisher/sunbin/wordy"
#endif

global bool pmode, epsflag;
global int firstpage, minpage, maxpage, cldeftop;
global char *cldefs[MAXCLDEFS];
global FILE *editin, *menuin, *codeout, *messout;

static char *fnames[MAXFNAMES];		/* filename(s) of ".al" file(s) */
static int numfnames;			/* number of ".al" files        */

static void newhandler(), readcmdline(char*[]), usage(), processbook(), closefiles();


global int main(int argc, char *argv[])
  { set_new_handler(newhandler);
    messout = stderr; /* just in case */
    readcmdline(argv);
    initfmtbook();
    processbook();
    tidyfmtbook();
    return 0;
  }

static void newhandler()
  { giveup("No room");
  }

static void readcmdline(char *argv[])
  { int ap = 0; bool minmaxset;
    numfnames = 0;
    if (argv[ap] != NULL) ap++; /* skip program name */
    while (numfnames < MAXFNAMES && argv[ap] != NULL && argv[ap][0] != '-') fnames[numfnames++] = argv[ap++];
    if (argv[ap] != NULL && argv[ap][0] != '-') giveup("too many input files!");
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
	  { if (cldeftop >= MAXCLDEFS) giveup("too many -D options!");
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

static void usage()
  { fprintf(stderr, "Usage: alpha fn          [-Dopt ...] [-fp n]\n");
    fprintf(stderr, "       alpha [fn ...] -p [-Dopt ...] [-fp n] [-pg m [n]] [-e]\n");
    exit(1);
  }

static void processbook()
  { for (int k=0; k < numfnames; k++)
      { char *fn = fnames[k];
	readstyle(fn);
	if (pmode)
	  { /* print mode; doesn't run in windows; default to standard input */
	    if (fn != NULL)
	      { editin = fopen(fn, "r");
		if (editin == NULL) giveup("can't open \"%s\"", fn);
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
	    if (editor == NULL) editor = WORDY;
	    if (access(editor,1) < 0) giveup("can't access editor \"%s\"", editor);
	    signal(SIGPIPE, SIG_IGN);  /* ignore pipe signals */
	    signal(SIGINT, SIG_IGN);   /* ignore interrupts, but retain default action in children */
	    spawn(WINDOWS, "windows %d %d %s %s %f %f %f %f",
		  wid, hgt, editor, fn, &editin, &menuin, &codeout, &messout);
	  }
	processfile();	/* updates firstpage in preparation for next file, if any */
	closefiles();
      }
  }

static void closefiles()
  { unless (editin == stdin) fclose(editin);
    unless (menuin == NULL) fclose(menuin);
    unless (codeout == NULL) fclose(codeout);
    unless (messout == stderr) fclose(messout);
  }

