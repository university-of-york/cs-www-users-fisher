#define global
#define forward

#define word	  int
#define uint	  unsigned int
#define uchar	  unsigned char
#define bool	  int
#define false	  0
#define true	  1
#define unless(x) if(!(x))
#define until(x)  while(!(x))

/* memory allocator */
#define heap(n,t) (t*) cheap((n)*sizeof(t))
forward char *cheap();

#define MAXSUBSTS  10	/* max. different metanotions in hypernotion or hyperrule */
#define MAXHITEMS  200	/* max. ssms & metanotions in hypernotion		  */
#define MAXRHS	   10	/* max. hypernotions on rhs of hyperrule		  */
#define MAXNAMELEN 80	/* max. chars in a metanotion, name line, etc.		  */
#define MAXSENTLEN 50	/* max. terminals in sentence at parse-time		  */

#define HASHSIZE   509	    /* should be a prime number */

enum symbol
  { s_colon, s_dcolon, s_comma, s_semico, s_dot, s_eof, s_ssm,
    s_meta, s_term
  };

#define f_term	1	/* word is used as a terminal	*/
#define f_meta	2	/* word is used as a metanotion */

struct dictnode
  { struct dictnode *lft, *rgt, *nxt;
    uchar flg;
    struct metarhs *rhs;	/* for metanotions only	 */
    uint sta, fol;		/* for metanotions only, used in checkgr */
    char *str;
  };

struct ruleset			/* a set of hyperrules */
  { struct ruleset *lnk;
    struct hyperrule *hr;
  };


/* bits set in type field of hyperrule: */
#define type_L	 001
#define type_R	 002
#define type_LR	 003	/* == (type_L | type_R) */
#define type_T	 004

struct hyperrule
  { struct hypernotion *lhs;
    struct hyperalt *rhs;
    uchar type;			/* used by checkgr & earley */
  };

struct hyperalt
  { int rlen;			/* num. of hypernotions on rhs */
    struct hypernotion **rdef;	/* comma-separated list of hypernotions */
  };

struct metarhs			/* semico-separated list of hypernotions */
  { struct metarhs *lnk;
    struct hypernotion *hn;
  };

/* bits set in flags field of hyperrule: */
#define hn_dnull   1		/* hypernotion derives EMPTY */
#define hn_dterm   2		/* hypernotion derives terminal word */
#define hn_pospred 4		/* hypernotion is "where..." */
#define hn_negpred 8		/* hypernotion is "where..." but really means "unless..." */

struct hypernotion
  { int hlen;				/* >= 0 means hypernotion; -1 means terminal		 */
    struct hitem *hdef;			/* array of hitems, if hlen >= 0			 */
    struct ruleset *fxref, *bxref;	/* forward (Y) & backward cross-reference, if hlen >= 0	 */
    struct ruleset *xrefclo;		/* closure of fxref, if hlen >= 0			 */
    uchar flags;			/* used by checkgr					 */
    struct dictnode *term;		/* if hlen < 0						 */
    int lnum;				/* line number where hypernotion occurs			 */
  };

struct hitem
  { enum symbol sy;		/* s_ssm or s_meta   */
    int it_s;			/* if sy == s_ssm    */
    struct dictnode *it_z;	/* if sy == s_meta   */
    bool isbound;		/* used by checkgr   */
  };

