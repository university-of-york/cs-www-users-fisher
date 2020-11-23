/* Alpha document preparation system
   header file
   AJF	 December 1987 */

#define global
#define forward
#define false	    0
#define true	    1
#define until(x)    while (!(x))
#define unless(x)   if (!(x))

#define min(a,b)    ((a) < (b) ? (a) : (b))
#define max(a,b)    ((a) > (b) ? (a) : (b))

typedef unsigned char  uchar;
typedef unsigned short ushort;
typedef unsigned int   uint;
typedef int	       word;		/* same size as int, char* */
typedef int	       bool;

#define MAXSTRLEN   256			/* max. length of string/line/word etc.		*/
#define BPW	    32			/* bytes per word				*/
#define MAXFONTS    96			/* max. num. of fonts (multiple of BPW)		*/
#define MAXSUBST    10			/* max. num. of "c." or "x." in transliteration */
#define MAXINT	    1000000		/* a Very Big Number				*/

#define NUMTFONTS    2				/* fonts 0, 1 reserved for translit chars	*/
#define ERROR_TCHAR  ((NUMTFONTS << 8) + '?')	/* '?' in some font or other			*/

#define GLUECH	    037
#define SUPERGLUECH 036

/* Tranliterations are either ligatures or "proper" (font-zero) transliterations.
   A font-zero translit. is denoted by its character value (one byte) which
   specifies its position in font zero.	 A ligature has a dynamically-bound
   font; it is represented by up to 3 chars (e.g. letter+accent) OR'd with LIGBIT. */

#define LIGBIT	    (1 << 24)

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

typedef (*proc)();

enum nodetype	/* node types */
  { s_stop, s_bol, s_eol, s_word,
    s_ubegin, s_begin, s_uend, s_end
  };

struct node	/* text record */
  { enum nodetype typ;
    char *wrd;
    struct node *fwd, *bwd, *slk, *unm;
    int spb, spn, skp, vof, gln;
    bool edt;
  };

enum outpop	/* outproc operations */
  { o_bobook, o_eobook, o_bofile, o_eofile, o_bopage, o_eopage, o_boline, o_eoline,
    o_clcbar, o_deffont, o_selfont, o_deftrlit, o_char
  };

struct bbox { int llx, lly, urx, ury }; /* PostScript BoundingBox; dims. in millipoints */

enum trtype	/* transliteration types */
  { t_char, t_bitmap, t_pscript, t_unkn
  };

struct trlit
  { enum trtype type;	   /* type of trlit: t_char, t_bitmap, t_pscript */
    int wid, hgt;	   /* width, height of compound mark on page	 */
    char *fn;		   /* for BM & PS:    name of BM or PS file	 */
    int scale;		   /* for BM & PS:    scale factor		 */
    struct bbox bbox;	   /* for PostScript: BoundingBox (millipoints)	 */
    int nsu;		   /* for chars:      number of substitutions	 */
    int subst[MAXSUBST];   /* for chars:      vector of substitutions	 */
  };

/* memory allocator */
#define heap(n,t) (t*) cheap((n)*sizeof(t))
forward char *cheap();

#define seq(s1,s2) (strcmp(s1,s2) == 0)

