/* jtp - Compiler - Java bytecode to Transputer assembly code
   A.J. Fisher	 June 1997 */

#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include "jtp.h"

#define MAXITEMS 100

global uint options;
global bool anyerrors;
global Item items[MAXITEMS];
global int labno, numitems;

static char *outfn;

static void newhandler(), parsecmdline(char *[]), additem(char*);
static bool ends(char*, char*);
static void usage();
static void readclasses(), combineinputs(tprog*);
static void writeprog(fprog*), writeblock(void*, int, FILE*);


global void main(int argc, char *argv[])
  { set_new_handler(newhandler);
    anyerrors = false;
    parsecmdline(argv);
    labno = 0;
    initopcodes();
    readclasses();
    allocfields();
    tprog *tp = new tprog;
    combineinputs(tp);
    gentail(tp);
    fixconstants(tp);
    fprog *fp = new fprog;
    assemble(tp, fp);
    if (outfn != NULL && !anyerrors) writeprog(fp);
    delete tp; delete fp;
    exit(anyerrors ? 3 : 0);
  }

static void newhandler()
  { giveup("No room");
  }

static void parsecmdline(char *argv[])
  { options = numitems = 0; outfn = NULL;
    int ap = 0;
    unless (argv[ap] == NULL) ap++;
    until (argv[ap] == NULL)
      { char *s = argv[ap++];
	if (s[0] == '-')
	  { if (seq(s,"-o"))
	      { if (argv[ap] == NULL || outfn != NULL) usage();
		outfn = argv[ap++];
	      }
	    else
	      { s++;
		int c = *(s++);
		until (c == '\0')
		  { if (c == 'u') options |= opt_u;
		    else if (c == 'v') options |= (opt_v | opt_l);	/* -v implies -l */
		    else if (c == 'l') options |= opt_l;
		    else if (c == 'a') ;	/* ignored */
		    else usage();
		    c = *(s++);
		  }
	      }
	  }
	else additem(s);
      }
  }

static void additem(char *fn)
  { Itype ty = ends(fn,".class") ? ty_class :
	       ends(fn,".s") ? ty_assembly : ty_unkn;
    if (ty == ty_unkn)
      { DIR *dir = opendir(fn);		/* is it a directory? */
	if (dir != NULL)
	  { dirent *ent = readdir(dir);
	    until (ent == NULL)
	      { char *efn = ent -> d_name;
		unless (efn[0] == '.')
		  { if (strlen(fn) + strlen(efn) + 2 > MAXSTRING) giveup("directories nested too deeply!");
		    char buf[MAXSTRING]; sprintf(buf, "%s/%s", fn, efn);
		    additem(buf);   /* recurse */
		  }
		ent = readdir(dir);
	      }
	    closedir(dir);
	  }
	else warn("unknown file type: %s", fn);
      }
    else
      { if (numitems >= MAXITEMS) giveup("too many input files!");
	items[numitems].fn = copystr(fn);
	items[numitems].ty = ty;
	numitems++;
      }
  }

static bool ends(char *s1, char *s2)
  { int n1 = strlen(s1), n2 = strlen(s2);
    return (n1 >= n2) && seq(&s1[n1-n2], s2);
  }

static void usage()
  { fprintf(stderr, "Usage: jtp [-o fn.out] [-{uvla}] { in.class | in.s | dir } ...\n");
    exit(2);
  }

static void readclasses()
  { for (int n = 0; n < numitems; n++)
      { Item *it = &items[n];
	if (it -> ty == ty_class)
	  { it -> cl = readclass(it -> fn);
	    if (it -> cl == NULL) it -> ty = ty_badfile;
	  }
      }
  }

static void combineinputs(tprog *out)
  { /* compile all classes */
    for (int n = 0; n < numitems; n++)
      { Item *it = &items[n];
	if (it -> ty == ty_class) it -> tp = compileclass(it -> cl);
      }
    /* read and assemble all assembly modules */
    for (int n = 0; n < numitems; n++)
      { Item *it = &items[n];
	if (it -> ty == ty_assembly) it -> tp = readassembly(it -> fn);
      }
    /* append items in the correct order to output tprog */
    for (int n = 0; n < numitems; n++)
      { Item *it = &items[n];
	if (it -> ty == ty_class || it -> ty == ty_assembly) out -> append(it -> tp);
      }
  }

static void writeprog(fprog *fp)
  { FILE *ofi = fopen(outfn, "w");
    if (ofi != NULL)
      { writeblock("JavaTptr", 8, ofi);     /* magic number */
	writeblock(fp -> code, fp -> clen, ofi);
	fclose(ofi);
      }
    else warn("can't create %s", outfn);
  }

static void writeblock(void *ptr, int ni, FILE *ofi)
  { int nw = fwrite(ptr, 1, ni, ofi);
    if (nw != ni) warn("write error on output file");
  }

