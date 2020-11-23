/* Alpha document preparation system
   header file
   AJF	 December 1987 */

#include <stdio.h>		/* req'd for def'n of FILE etc.	      */
#include <X11/Xlib.h>		/* req'd by gfxlib		      */
#include <gfxlib.h>		/* req'd for def'n of struct fishfont */

#define global
#define until(x)    while (!(x))
#define unless(x)   if (!(x))

typedef unsigned char  uchar;
typedef unsigned short ushort;
typedef unsigned int   uint;

union word
  { word(int ix)   { i = ix; }
    word(void *px) { p = px; }
    int i; void *p;
  };

extern fishfont *fontinfo[];				    /* from common */

struct Char						    /* a Character in a Font	*/
  { Char(short f, ushort c) { ft = f; ch = c; }
    Char() { }
    int &width() { return fontinfo[ft] -> advance[ch].x; }  /* width of char, in millipixels */
    short ft;						    /* index into fontinfo	*/
    ushort ch;						    /* the character		*/
  };

#define MAXSTRLEN      256		/* max. length of string/line/word etc.		*/
#define BPW	       32		/* bytes per word				*/
#define MAXFONTS       96		/* max. num. of fonts (multiple of BPW)		*/
#define MAXSUBST       10		/* max. num. of "c." or "x." in transliteration */
#define MAXINT	       1000000		/* a Very Big Number				*/

#define NUMTFONTS      2			/* fonts 0, 1 reserved for translit chars	*/
#define ERROR_TCHAR    Char(NUMTFONTS, '?')	/* '?' in some font or other			*/

#define LIGUPCH	       001
#define LIGDNCH	       002
#define GLUECH	       037  /* this value req'd by wshell */
#define SUPERGLUECH    036

/* indices into fonts[]: */
#define Roman	    0	   /* Roman font				    */
#define Italic	    1	   /* Italic font (esc ^)			    */
#define Bold	    2	   /* Bold font (esc #)				    */
#define Symbol	    3	   /* symbol font, used for lpar, rpar in eqn mode  */
#define Romix	    4	   /* Roman font used for superscripts & subscripts */
#define Italix	    5	   /* Italic font used for ditto		    */
#define Boldix	    6	   /* Bold font used for ditto (unused at present)  */
#define Symbix	    7	   /* Symbol ditto (unused at present)		    */

#define INDEX	    4	   /* offset to index fonts    */
#define NUMFP	    8	   /* number of font positions */

/* option flags for setnumber: */
#define n_signok    0x01   /* leading sign allowed			      */
#define n_rel	    0x02   /* leading sign means relative to previous setting */
#define n_upciok    0x04   /* units "upci" allopwed                           */
#define n_nmvok	    0x10   /* units "nmv" (which depend on style file & fonts) allowed */

#define n_unitsok   (n_upciok | n_nmvok)

enum nodetype	/* node types */
  { s_stop, s_bol, s_eol, s_word,
    s_ubegin, s_begin, s_uend, s_end,
  };

struct node	/* text record */
  { node(nodetype, char*, int);		/* constructor */
    node() { }				/* constructor */
    static void* operator new(int);	/* special-purpose memory allocator for nodes */
    static void operator delete(void*); /* ditto de-allocator */
    nodetype typ;
    char *wrd;
    node *fwd, *bwd, *slk, *unm;
    int spb, spn, skp, vof, gln;
    bool edt;
  };

enum outpop	/* outproc operations */
  { o_bobook, o_eobook, o_bofile, o_eofile, o_bopage, o_eopage, o_boline, o_eoline,
    o_clcbar, o_deffont, o_selfont, o_deftrlit, o_char,
  };

struct bbox { int llx, lly, urx, ury; }; /* PostScript BoundingBox; dims. in millipoints */

enum trtype	/* transliteration types */
  { t_char, t_bitmap, t_pscript, t_unkn,
  };

struct trlit
  { trtype type;	   /* type of trlit: t_char, t_bitmap, t_pscript */
    int wid, hgt;	   /* width, height of compound mark on page	 */
    char *fn;		   /* for BM & PS:    name of BM or PS file	 */
    int scale;		   /* for BM & PS:    scale factor		 */
    bbox bb;		   /* for PostScript: BoundingBox (millipoints)	 */
    int nsu;		   /* for chars:      number of substitutions	 */
    Char subst[MAXSUBST];  /* for chars:      vector of substitutions	 */
  };

typedef void (*proc)();
typedef void (*oproc)(outpop, word = 0, word = 0, word = 0);	/* "outproc" proc type */

extern int
    /* style variables, defined in style file, from readstyle */
    version, pagewidth, pageheight, ps_tlx, ps_tly, bodywidth, leftmargin, mingluewidth,
    linesperpage, linesperbody, topmargin, headposn, footposn,
    vertspacing, indexspacing, indexmag;

extern FILE *editin, *menuin, *codeout, *messout;		/* from main	  */
extern int minpage, maxpage, firstpage, cldeftop;		/* from main	  */
extern bool pmode, epsflag;					/* from main	  */
extern char *cldefs[];						/* from main	  */
extern oproc outproc;						/* from edit	  */
extern int pvpage;						/* from format	  */
extern bool anotherpass;					/* from format	  */
extern int fonts[];						/* from format	  */
extern char *stylefn;						/* from readstyle */

extern void readstyle(char*);					/* from readstyle */

extern void processfile();					/* from edit	  */

extern void initfmtbook(), tidyfmtbook();			/* from format	  */
extern void initfmtfile(), tidyfmtfile();			/* from format	  */
extern int formatpage(node*, int);				/* from format	  */

extern Char tabtranslit(char, int);				/* from translit  */
extern Char *symtranslit(char*);				/* from translit  */
extern Char boxtranslit(int, int, int);				/* from translit  */
extern void inittranslit(), tidytranslit();			/* from translit  */

extern void ps_outproc(outpop, word = 0, word = 0, word = 0);	/* from psout	  */
extern void scr_outproc(outpop, word = 0, word = 0, word = 0);	/* from scrout	  */

extern bool setnumber(char*, int&, uint);			/* from common	  */
extern char *lookupword(char*);					/* from common	  */
extern void initfonts();					/* from common	  */
extern void splitfontname(char*, char*, int&);			/* from common	  */
extern int lookupfont(char*, int);				/* from common	  */

extern void giveup(char*, word = 0, word = 0);			/* from giveup	  */

extern "C"
  { proc set_new_handler(proc);
    char *getenv(char*), strcat(char*, char*);
    int strncmp(char*, char*, int);
    int putenv(char*), atoi(char*), access(char*, uint);
    int select(int, uint*, uint*, uint*, int[]);
    void unlink(char*);
  }

inline int min(int a, int b)	    { return (a < b) ? a : b; }
inline int max(int a, int b)	    { return (a > b) ? a : b; }
inline bool seq(char *s1, char *s2) { return strcmp(s1,s2) == 0; }

inline bool testbit(uint *m, int k)
  { int i = k/BPW, j = k%BPW;
    return (m[i] & (1 << j));
  }

inline void setbit(uint *m, int k)
  { int i = k/BPW, j = k%BPW;
    m[i] |= (1 << j);
  }

