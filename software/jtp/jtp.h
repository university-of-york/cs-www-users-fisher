/* jtp - Compiler - Java bytecode to Transputer assembly code
   A.J. Fisher	 June 1997 */

#define global
#define unless(x)   if(!(x))
#define until(x)    while(!(x))

#define MINT	    (1 << 31)
#define MAXSTRING   512

#define acc_private	    0x002
#define acc_static	    0x008
#define acc_final	    0x010
#define acc_synchronized    0x020
#define acc_abstract	    0x400

typedef unsigned int uint;
typedef unsigned short ushort;
typedef unsigned char uchar;
typedef void (*proc)();

struct fullname
  { char *cl;	/* class name		*/
    char *id;	/* field or method name */
    char *sig;	/* signature		*/
  };

union word	/* for error msgs, constant pool, etc */
  { word() { }
    word(int nx)	{ n = nx; }
    word(char *sx)	{ s = sx; }
    word(ushort *ux)	{ u = ux; }
    word(fullname *fx)	{ f = fx; }
    word(word *wx)	{ w = wx; }
    int n; char *s; ushort *u; fullname *f; word *w;
  };

struct form			/* field or method */
  { fullname *fn;
    ushort acc;			/* access bits */
    int addr;			/* for fields: offset (non-static) or labno (static) */
    uchar *code;		/* for methods: code vector */
    int fps, locs;		/* for methods: stack usage */
    char rtype;			/* (result) type character */
  };

struct formvec
  { /* growable vector */
    formvec(int = 20);	 /* parameter is initial size */
    ~formvec();
    void add(form*);
    formvec *clone();
    int max, num;
    form **vec;
  };

struct Class
  { Class(char *nam, char *snam, word *pool, uchar *tags, formvec *flds, formvec *mthds, ushort a)
      { name = nam;
	super = snam;
	constpool = pool;
	consttags = tags;
	fields = flds;
	methods = mthds;
	acc = a;
      }
    char *name, *super;
    word *constpool;
    uchar *consttags;
    formvec *fields;
    formvec *methods;
    ushort acc;
    formvec *mtab;
    uchar bits;	    /* used by gentail */
    int size;
    uint refbm;	    /* reference bitmask */
  };

enum addrmode
  { m_none, m_lab, m_loc, m_name, m_num, m_str, m_astr, m_float, m_afloat, m_double, m_adouble, m_wksp,
  };

struct lvinfo
  { lvinfo(int nf, int nl, int na) { fps = nf; locs = nl; aps = na; }
    int fps;		/* num. of formal parameters */
    int locs;		/* num. of local variables   */
    int aps;		/* num. of actual parameters */
  };

struct tinstr
  { tinstr(int op, int m1 = m_none, word p1 = 0, int m2 = m_none, word p2 = 0)
      { this -> op = op;
	this -> m1 = m1;
	this -> p1 = p1;
	this -> m2 = m2;
	this -> p2 = p2;
      }
    uchar op;
    int m1; word p1;
    int m2; word p2;
    lvinfo *lvi;		/* local variable info (t_entry) */
    int jlen;			/* for pc-relative instructions (used by assemble) */
  };

struct tprog
  { tprog();
    ~tprog();
    void gen(int, int = m_none, word = 0, int = m_none, word = 0);
    void append(tprog*);
    tinstr *last();	/* returns most recent tinstr generated */
    tinstr **code;
    int clen;
  };

struct tswitch
  { /* format of j_tableswitch instruction */
    int dflt;
    int lo, hi;
    int vec[0]; /* extensible */
  };

struct lswitch
  { /* format of j_lookupswitch instruction */
    int dflt;
    int np;
    int vec[0]; /* extensible */
  };

struct fprog
  { fprog();
    ~fprog();
    void gen(uchar);
    uchar *code;
    int clen;
  };

enum Itype { ty_class, ty_assembly, ty_unkn, ty_badfile };

struct Item
  { /* command line item : either a .class or a .s */
    Itype ty;
    char *fn;	    /* filename */
    Class *cl;
    tprog *tp;
  };

#define opt_l	1	/* -l : list output	 */
#define opt_v	2	/* -v : verbose output	 */
#define opt_u	4	/* -w : warn if not used */

extern uint options;
extern bool anyerrors;		/* from main */
extern Item items[];		/* from main */
extern int labno, numitems;	/* from main */

extern "C"
  { proc set_new_handler(proc);
    void *realloc(void*, int);
  }

extern Class *readclass(char*);					/* from readclass   */
extern void allocfields();					/* from allocfields */
extern void initopcodes();					/* from readassem   */
extern tprog *readassembly(char*);				/* from readassem   */
extern tprog *compileclass(Class*);				/* from compile	  */
extern void translate(form*, tprog*, word*, uchar*);		/* from translate */
extern void peephole(tprog*);					/* from peephole  */
extern void optimize(tprog*, tprog*);				/* from optimize  */
extern void gentail(tprog*);					/* from gentail	  */
extern void fixconstants(tprog*);				/* from fixconsts */
extern void assemble(tprog*, fprog*);				/* from assemble  */
extern void jlist(uchar*, word*, uchar*);			/* from jlist	  */
extern char *jopstring(int);					/* from jlist	  */
extern int jlength(uchar*);					/* from jlist	  */
extern void tlist(tinstr*);					/* from tlist	  */
extern char *topstring(int), *addrfmt(int);			/* from tlist	  */

extern void findlabels(form*);					/* from findlabs  */
extern int labelat(uchar*);					/* from findlabs  */

extern float bitsfp(uint);					/* from common	  */
extern double bitsdp(uint, uint);				/* from common	  */

extern Class *lookupclass(char*);				/* from common	  */
extern char *mkname(fullname*);					/* from common	  */
extern char *trsig(char*);					/* from common	  */
extern int objectsize(char);					/* from common	  */
extern char *lookupstring(char*), *copystr(char*);		/* from common	  */
extern void giveup(char*, word = 0, word = 0, word = 0);	/* from common	  */
extern void warn(char*, word = 0, word = 0, word = 0);		/* from common	  */

inline bool seq(char *s1, char *s2) { return strcmp(s1,s2) == 0; }

