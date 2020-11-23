/* Alpha document preparation system
   transliteration routines
   AJF	 January 1993 */

#include <errno.h>

#include "alpha.h"

#define MINTCHAR     0060			/* start at '0' to avoid problems with '-' etc. */
#define MAXTCHAR     0377			/* highest char allowed for translits		*/
#define NUMTCHARS    (MAXTCHAR-MINTCHAR+1)	/* num. of translit chars per font		*/

#ifdef mips
   #define MKBOX     "/usr/fisher/mipsbin/mkbox"
#endif

#ifdef sun
   #define MKBOX     "/usr/fisher/sunbin/mkbox"
#endif

static char *trtab[NUMTFONTS*NUMTCHARS];
static int numtrs;
static char *wptr;

enum ftype
  { ft_bitmap, ft_pscript, ft_unkn, ft_nonex,
  };

struct magic
  { uint mmin, mmax;
    enum ftype ft;
  };

static magic magictab[] =	/* table for deciding file type */
  { { 0x2f2a2046, 0x2f2a2046, ft_bitmap	 },    /* "/* Format_version"      */
    { 0x6d70725f, 0x6d70725f, ft_bitmap	 },    /* "mpr_static"             */
    { 0x4465636c, 0x4465636c, ft_bitmap	 },    /* "DeclareBitmap"          */
    { 0xa7590000, 0xa7590000, ft_bitmap	 },    /* binary xor'd		   */
    { 0x50340a23, 0x50340a23, ft_bitmap	 },    /* "P4\n#" (Portable Pixmap)*/
    { 0x25210000, 0x2521ffff, ft_pscript },    /* "%!"    (PostScript)     */
    { 0x00000000, 0xffffffff, ft_unkn	 },    /* stopper		   */
  };

struct box
  { struct box *link;
    int wid, hgt, lth;	/* in millipoints (WAS points) */
    char fn[16];
  };

static box *boxlist;

static void makebox(box*);
static void makeligature(char*, Char*);
static Char lookuptranslit(char*);
static Char index_to_tc(int);
static bool decode(trlit*, char*);
static char *getfname(char*);
static int getscale();
static void importbm(char*, trlit*), importps(char*, trlit*), cantimport(char*, char*);
static bool getbbox(trlit*);
static int rdline(FILE*, char[]);
static void setimagewidth(trlit*);
static int bmwidth(char*);
static ftype filetype(char*);


global void inittranslit()   /* called from format module */
  { numtrs = 0;
    boxlist = NULL;
  }

global void tidytranslit()
  { box *b = boxlist;
    until (b == NULL)
      { unlink(b -> fn);
	b = b -> link;
      }
  }

global Char tabtranslit(char c, int d)
  { char v[MAXSTRLEN+1];
    v[0] = c; /* 'h' or 'v' */
    sprintf(&v[1], "%d", d); /* distance in millipixels */
    return lookuptranslit(lookupword(v));
  }

global Char boxtranslit(int wid, int hgt, int lth)
  { box *b = boxlist;
    until (b == NULL || (b -> wid == wid && b -> hgt == hgt && b -> lth == lth)) b = b -> link;
    if (b == NULL)
      { /* box file does not exist; make it */
	b = new box;
	b -> wid = wid; b -> hgt = hgt; b -> lth = lth;
	makebox(b);
	b -> link = boxlist; boxlist = b; /* link into list */
      }
    char trv[MAXSTRLEN+1]; sprintf(trv, "%c\"%s\"", (pmode ? 'p' : 'b'), b -> fn);
    return lookuptranslit(lookupword(trv));
  }

static void makebox(box *b)
  { /* file does not exist; make it */
    strcpy(b -> fn, "/tmp/box.XXXXXX"); mktemp(b -> fn);
    char cmd[MAXSTRLEN+1];
    sprintf(cmd, "%s -%c %.3f %.3f %.3f >%s", MKBOX, (pmode ? 'p' : 'b'),
	    (b -> wid)/1000.0, (b -> hgt)/1000.0, (b -> lth)/1000.0, b -> fn);
    int code = system(cmd);
    if (code != 0) fprintf(messout, "Can't make box!\n");
  }

global Char *symtranslit(char *w)
  { Char *sub = new Char[MAXSUBST];
    int j = 0, k = 0;
    int font = -1;	/* font -1 means "copy current font" */
    while (w[j] != '\0' && k+3 < MAXSUBST)
      { switch (w[j])
	  { case 'f':
	      { /* set font for subsequent 'c/x' trs. */
		char fn[MAXSTRLEN+1]; int ps;
		j++;
		splitfontname(&w[j], fn, ps);
		font = lookupfont(fn, ps);
		while ((w[j] >= 'A' && w[j] <= 'Z') || (w[j] >= '0' && w[j] <= '9') || (w[j] == '.')) j++;
		break;
	      }

	    case 'c':	case 'x':
	      { bool rsfx = (w[j+2] == 'r');
		if (rsfx) sub[k++] = Char(0, LIGUPCH);
		sub[k++] = Char(font, (w[j] == 'c') ? w[j+1] : w[j+1] | 0x80);
		if (rsfx) sub[k++] = Char(0, LIGDNCH);
		j += (rsfx ? 3 : 2);
		break;
	      }

	    default:
		sub[k++] = lookuptranslit(&w[j]);
		until (w[j] == '\0') j++;
		break;
	  }
      }
    unless (w[j] == '\0') fprintf(messout, "Bad ligature \"%s\"\n", w);
    sub[k] = Char(0, '\0');
    return sub;
  }

static Char lookuptranslit(char *w)
  { Char tc = ERROR_TCHAR; int i = numtrs-1;
    while (i >= 0 && trtab[i] != w) i--;
    if (i >= 0) tc = index_to_tc(i);
    else if (numtrs >= NUMTFONTS*NUMTCHARS) fprintf(messout, "Translit set is full!\n");
    else
      { trlit trl;
	bool ok = decode(&trl, w);
	if (ok)
	  { tc = index_to_tc(numtrs);
	    tc.width() = trl.wid;			    /* set width in fontinfo entry of new char */
	    outproc(o_deftrlit, tc.ft, tc.ch, &trl);	    /* define the transliteration */
	    trtab[numtrs++] = w;
	  }
	else fprintf(messout, "Bad transliteration \"%s\"\n", w);
      }
    return tc;
  }

static Char index_to_tc(int n)
  { int f = 0;
    while (n >= NUMTCHARS) { f++; n -= NUMTCHARS; }  /* select font */
    return Char(f, MINTCHAR + n);
  }

static bool decode(trlit *tr, char *w)
  { bool ok = true, end = false, wset = false;
    wptr = w;
    tr -> wid = tr -> hgt = tr -> nsu = 0;
    tr -> type = t_char;
    until (end || !ok)
      { char code = *(wptr++);
	switch (code)
	  { default:
		ok = false;
		break;

	    case '\0':
		end = true;
		break;

	    case 'h':
		/* store width (may be pos. or neg.) */
		setnumber(wptr, tr -> wid, n_signok | n_unitsok);
		wset = true; end = true;
		break;

	    case 'v':
		/* store height (may be pos. or neg.) */
		setnumber(wptr, tr -> hgt, n_signok | n_unitsok);
		end = true;
		break;

	    case 'u':
		fprintf(messout, "??? up in trlit\n");
		tr -> hgt = -indexspacing; /* up a notch */
		break;

	    case 'd':
		fprintf(messout, "??? down in trlit\n");
		tr -> hgt = indexspacing; /* down a notch */
		break;

	    case 'b':	case 'p':   case 'i':
	      { /* import a bitmap (b) or PostScript (p) file */
		if (code == 'i')
		  { /* "print mode" can handle PS; else we need a bitmap */
		    if (pmode)
		      { char *fn = getfname(".ps");
			importps(fn, tr);
		      }
		    else
		      { char *fn = getfname(".bm");
			importbm(fn, tr);
		      }
		  }
		else
		  { char *fn = getfname("");
		    if (code == 'b') importbm(fn, tr);
		    else
		      { if (pmode) importps(fn, tr);
			else ok = false;
		      }
		  }
		tr -> scale = getscale();
		unless (wset)
		  { setimagewidth(tr);
		    wset = true;
		  }
		break;
	      }
	  }
      }
    return ok;
  }

static char *getfname(char *sfx)
  { char fv[MAXSTRLEN+1]; int k = 0;
    int sn = strlen(sfx);
    if (*wptr == '"') wptr++;
    until (*wptr == '"' || *wptr == '\0') fv[k++] = *(wptr++);
    if (*wptr == '"') wptr++;
    fv[k++] = '\0';
    char *fn = new char[k+sn];
    strcpy(fn, fv); strcat(fn, sfx);
    return fn;
  }

static int getscale()
  { int n = 0;
    if (*wptr == '/')
      { /* scale factor */
	wptr++;
	while (*wptr >= '0' && *wptr <= '9') n = (n * 10) + (*(wptr++) - '0');
      }
    if (n == 0) n = 1;
    return n;
  }

static void importbm(char *fn, trlit *tr)
  { switch (filetype(fn))
      { default:
	    cantimport(fn, "bug!");
	    tr -> type = t_unkn;
	    break;

	case ft_nonex:
	    cantimport(fn, "no such file");
	    tr -> type = t_unkn;
	    break;

	case ft_unkn:	case ft_pscript:
	    cantimport(fn, "not a bitmap file");
	    tr -> type = t_unkn;
	    break;

	case ft_bitmap:
	    /* bitmap */
	    tr -> type = t_bitmap;
	    tr -> fn = fn;
	    break;
      }
  }

static void importps(char *fn, trlit *tr)
  { switch (filetype(fn))
      { default:
	    cantimport(fn, "bug!");
	    tr -> type = t_unkn;
	    break;

	case ft_nonex:
	    cantimport(fn, "no such file");
	    tr -> type = t_unkn;
	    break;

	case ft_unkn:	case ft_bitmap:
	    cantimport(fn, "not a PostScript file");
	    tr -> type = t_unkn;
	    break;

	case ft_pscript:
	  { /* PostScript file (pmode only) */
	    tr -> type = t_pscript;
	    tr -> fn = fn;
	    bool ok = getbbox(tr);
	    unless (ok) giveup("Giving up!");
	    break;
	  }
      }
  }

static void cantimport(char *fn, char *msg)
  { fprintf(messout, "Can't import file \"%s\": %s\n", fn, msg);
  }

static bool getbbox(trlit *tr)
  { bool ok = false;
    FILE *fi = fopen(tr -> fn, "r");
    if (fi != NULL)
      { char line[MAXSTRLEN];
	int nc = rdline(fi, line);
	while (nc >= 0 && !ok)
	  { if (strncmp(line, "%%BoundingBox:", 14) == 0)
	      { int *bp = &tr -> bb.llx; int p = 14;
		ok = true;
		for (int k=0; k < 4 && ok; k++)
		  { char wrd[MAXSTRLEN+1]; bool dok;
		    int j = 0;
		    while (p < nc && (line[p] == ' ' || line[p] == '\t')) p++;
		    while (p < nc && (line[p] >= '0' && line[p] <= '9' || line[p] == '.')) wrd[j++] = line[p++];
		    wrd[j++] = 'p'; wrd[j++] = '\0';
		    dok = setnumber(wrd, bp[k], n_upciok);
		    unless (dok) ok = false;
		  }
	      }
	    nc = rdline(fi, line);
	  }
	unless (ok) fprintf(messout, "No valid %%%%BoundingBox in file \"%s\"\n", tr -> fn);
	fclose(fi);
      }
    else fprintf(messout, "Can't read PostScript file \"%s\"\n", tr -> fn);
    return ok;
  }

static int rdline(FILE *fi, char line[])
  { int nc = 0;
    int ch = getc(fi);
    until (ch == '\n' || ch == EOF || nc >= MAXSTRLEN)
      { line[nc++] = ch;
	ch = getc(fi);
      }
    if (ch == EOF) nc = -1;
    /* we don't care if the line's too long;
       we're only looking at the first few chars anyway */
    until (ch == EOF || ch == '\n') ch = getc(fi);
    return nc;
  }

static void setimagewidth(trlit *tr)
  { int wid = (tr -> type == t_bitmap) ? bmwidth(tr -> fn) :
	      (tr -> type == t_pscript) ? (tr -> bb.urx) - (tr -> bb.llx) : 0;
    tr -> wid = wid / tr -> scale;
  }

static int bmwidth(char *fn)
  { int wid = 0;
    bitmap *bm = new bitmap(fn, 0);
    if (bm -> ok) wid = bm -> wid * 1000;   /* width in millipixels */
    else fprintf(messout, "Can't read bitmap file \"%s\"\n", fn);
    delete bm;
    return wid;
  }

static ftype filetype(char *fn)
  { ftype ft = ft_nonex;
    FILE *fi = fopen(fn, "r");
    if (fi != NULL)
      { uint magic; int nw;
	ft = ft_unkn;
	nw = fread(&magic, 4, 1, fi);
	fclose(fi);
	if (nw == 1)
	  { int k = 0;
	    until (magic >= magictab[k].mmin && magic <= magictab[k].mmax) k++;
	    ft = magictab[k].ft;
	  }
      }
    return ft;
  }

