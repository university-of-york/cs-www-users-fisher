/* jtp - Compiler - Java bytecode to Transputer assembly code
   A.J. Fisher	 June 1997 */

#include <stdio.h>
#include <string.h>
#include "jtp.h"
#include "jcodes.h"
#include "cftags.h"

enum formtype { f_field, f_method };

static FILE *infile;
static word *constpool;
static uchar *consttags;
static char *thisclass, *superclass;
static ushort classbits;

static void checkheader(), readconstants(), resolveconstants();
static void readclassinfo(), readinterfaces();
static formvec *readform(formtype);
static void analysesig(char*, form*), rdattributes(), praccess(uint);
static int readword(), readshort(), readbyte();
static void readbytes(void*, int), skipbytes(int);


global Class *readclass(char *fn)
  { Class *cl;
    infile = fopen(fn, "r");
    if (infile != NULL)
      { checkheader(); readconstants(); resolveconstants();
	readclassinfo(); readinterfaces();
	formvec *fields = readform(f_field);
	formvec *methods = readform(f_method);
	rdattributes();
	if (options & opt_v) putchar('\n');
	cl = new Class(thisclass, superclass, constpool, consttags, fields, methods, classbits);
	fclose(infile);
      }
    else
      { warn("can't open class file %s", fn);
	cl = NULL;
      }
    return cl;
  }

static void checkheader()
  { unless (readword() == 0xcafebabe) giveup("bad magic");
    int min = readshort(); int maj = readshort();
    if (options & opt_v) printf("Version:     %d.%d\n", maj, min);
  }

static void readconstants()
  { int nc = readshort();
    constpool = new word[nc];
    consttags = new uchar[nc];
    constpool[0].n = nc;
    int n = 1;
    while (n < nc)
      { int tag = readbyte();
	consttags[n] = tag;
	switch (tag)
	  { default:
		giveup("bad tag %d in constant pool", tag);

	    case Constant_Utf8:
	      { int len = readshort();
		char *s = new char[len+1];
		readbytes(s, len);
		s[len] = '\0';
		constpool[n++].s = s;
		break;
	      }

	    case Constant_Unicode:
	      { int len = readshort();
		ushort *v = new ushort[len+1];
		v[0] = len;
		readbytes(&v[1], 2*len);	/* len shorts */
		constpool[n++].u = v;
		break;
	      }

	    case Constant_Integer:
	    case Constant_Float:
		constpool[n++].n = readword();
		break;

	    case Constant_Long:
	    case Constant_Double:
		constpool[n++].n = readword();
		consttags[n] = 0;	/* occupies 2 slots */
		constpool[n++].n = readword();
		break;

	    case Constant_Class:
	    case Constant_String:
		constpool[n++].n = readshort(); /* name_index, string_index */
		break;

	    case Constant_Fieldref:
	    case Constant_Methodref:
	    case Constant_InterfaceMethodref:
	    case Constant_NameAndType:
		constpool[n++].n = readword();	/* 2 shorts */
		break;
	  }
      }
  }

static void resolveconstants()
  { for (int i = 1; i < constpool[0].n; i++)
      { switch (consttags[i])
	  { case Constant_Class:
	    case Constant_String:
	      { int ix = constpool[i].n;
		constpool[i].s = lookupstring(constpool[ix].s);
		break;
	      }
	  }
      }
    for (int i = 1; i < constpool[0].n; i++)
      { switch (consttags[i])
	  { case Constant_Fieldref:
	    case Constant_Methodref:
	    case Constant_InterfaceMethodref:
	      { int cix = (constpool[i].n >> 16) & 0xffff;
		int j = constpool[i].n & 0xffff;
		int nix = (constpool[j].n >> 16) & 0xffff;
		int six = constpool[j].n & 0xffff;
		fullname *fn = new fullname;
		fn -> cl = lookupstring(constpool[cix].s);    /* class name	      */
		fn -> id = lookupstring(constpool[nix].s);    /* field or method name */
		fn -> sig = trsig(constpool[six].s);	      /* signature	      */
		constpool[i].f = fn;
		break;
	      }
	  }
      }
  }

static void readclassinfo()
  { classbits = readshort();
    int tcl = readshort();
    int scl = readshort();
    thisclass = lookupstring(constpool[tcl].s);
    superclass = (scl == 0) ? NULL : lookupstring(constpool[scl].s);
    if (options & opt_v)
      { printf("Access:      "); praccess(classbits); putchar('\n');
	printf("Class:       %s\n", thisclass);
	unless (superclass == NULL) printf("Extends:     %s\n", superclass);
      }
  }

static void readinterfaces()
  { int n = readshort();
    for (int i = 0; i < n; i++)
      { int ix = readshort();
	if ((ix != 0) && (options & opt_v)) printf("Implements:  %s\n", constpool[ix].s);
      }
  }

static formvec *readform(formtype f)	/* read field or method */
  { int n = readshort();
    formvec *fv = new formvec(n);
    for (int i = 0; i < n; i++)
      { form *x = new form;
	x -> acc = readshort();
	if (f == f_method)
	  { /* all private & static methods are implicitly final */
	    if (x -> acc & (acc_private | acc_static)) x -> acc |= acc_final;
	  }
	x -> code = NULL;
	fullname *fn = new fullname;
	fn -> cl = thisclass;
	int nix = readshort();
	int six = readshort();
	fn -> id = lookupstring(constpool[nix].s);
	fn -> sig = trsig(constpool[six].s);
	if (options & opt_v)
	  { printf("%s      ", (f == f_field) ? "Field: " : "Method:");
	    printf("acc="); praccess(x -> acc); putchar(' ');
	    printf("cl=%s id=%s sig=%s ", fn -> cl, fn -> id, fn -> sig);
	  }
	analysesig(fn -> sig, x);	/* sets fps, rtype */
	if (options & opt_v) printf("rtype=%c ", x -> rtype);
	if (f == f_method)
	  { unless (x -> acc & acc_static) x -> fps++;	 /* count "this" formal */
	    if (options & opt_v) printf("fps=%d ", x -> fps);
	    if (x -> acc & acc_synchronized) warn("``%s'': sorry, can't handle synchronized methods", mkname(fn));
	  }
	int acnt = readshort();
	for (int j = 0; j < acnt; j++)
	  { int ax = readshort();
	    char *anam = constpool[ax].s;
	    if (options & opt_v) printf("attr=%s ", anam);
	    int alen = readword();
	    if (seq(anam,"Code"))
	      { int stk = readshort(); int locs = readshort(); int clen = readword();
		x -> locs = locs;	/* num. of fps + locals */
		if (options & opt_v) printf("stk=%d locs=%d clen=%d ", stk, locs, clen);
		uchar *cvec = new uchar[clen+1];
		readbytes(cvec, clen);
		cvec[clen] = j_end;
		x -> code = cvec;
		int elen = readshort();
		if (options & opt_v) printf("elen=%d ", elen);
		skipbytes(8*elen);	/* discard exception table */
		rdattributes(); /* read & discard additional attributes */
	      }
	    else
	      { /* unknown attribute, ignore */
		skipbytes(alen);
	      }
	  }
	if (options & opt_v) putchar('\n');
	x -> fn = fn;
	fv -> add(x);
      }
    return fv;
  }

static void analysesig(char *sig, form *x)
  { /* analyse the signature; set fps, rtype */
    x -> fps = 0;
    x -> rtype = '?';
    int k = 0;
    if (sig[k] == '(')	/* method? */
      { k++;
	until (sig[k] == ')' || sig[k] == '\0')
	  { x -> fps += objectsize(sig[k]);
	    while (sig[k] == 'L' || sig[k] == 'R')
	      { if (sig[k++] == 'L')
		  { until (sig[k] == ';' || sig[k] == '\0') k++;
		  }
	      }
	    unless (sig[k] == '\0') k++;
	  }
	if (sig[k] == ')') k++;
      }
    unless (sig[k] == '\0') x -> rtype = sig[k];
  }

static void rdattributes()
  { int acnt = readshort();
    for (int j = 0; j < acnt; j++)
      { readshort();	/* attribute_name */
	int alen = readword();
	skipbytes(alen);
      }
  }

static void praccess(uint acc)
  { static char *strs[] =
      { "public", "private", "protected", "static",
	"final", "synchronized", "volatile", "transient",
	"native", "interface", "abstract", NULL,
      };
    int n = 0, np = 0; uint a = acc;
    until (a == 0 || strs[n] == NULL)
      { if (a & 1)
	  { if (np++ > 0) putchar(',');
	    fputs(strs[n], stdout);
	  }
	a >>= 1; n++;
      }
    unless (a == 0) giveup("unknown access bits: %08x", acc);
  }

static int readword()
  { int n;
    for (int i = 0; i < 4; i++) n = (n << 8) | readbyte();
    return n;
  }

static int readshort()
  { int c1 = readbyte();
    int c2 = readbyte();
    return (c1 << 8) + c2;
  }

static int readbyte()
  { int ch = getc(infile);
    if (ch < 0) giveup("premature eof!");
  }

static void readbytes(void *ptr, int nb)
  { int ni = fread(ptr, 1, nb, infile);
    unless (ni == nb) giveup("premature eof!");
  }

static void skipbytes(int nb)
  { fseek(infile, nb, 1);
  }

