/* Alpha document preparation system
   transliteration routines
   AJF	 January 1993 */

#include "alpha.h"
#include "/usr/fisher/lib/fishfont.h"
#include "/usr/fisher/lib/sunlib.h"
#include <stdio.h>

#define MINTCHAR     0060   /* start at '0' to avoid problems with '-' etc. */
#define MAXTCHAR     0377   /* highest char allowed for translits	    */

extern struct bitmap *ReadBitmap(); /* from -lsunlib */
extern splitfontname();		    /* from common   */
extern int lookupfont();	    /* from common   */
extern bool setnumber();	    /* from common   */
extern char *lookupword();	    /* from common   */

extern int indexspacing;	    /* style variable, from readstyle */

extern FILE *messout;		    /* from alpha  */
extern bool pmode;		    /* from alpha  */
extern struct fishfont *fontinfo[]; /* from common */
extern proc outproc;		    /* from common */

static char *trtab[MAXTCHAR-MINTCHAR+1];
static int mypid, numtrs;
static char *wptr;

enum ftype
  { ft_bitmap, ft_pscript, ft_unkn, ft_nonex
  };

struct magic
  { uint mmin, mmax;
    enum ftype ft;
  };

static struct magic magictab[] =	/* table for deciding file type */
  { { 0x2f2a2046, 0x2f2a2046, ft_bitmap	 },    /* "/* Format_version"      */
    { 0x6d70725f, 0x6d70725f, ft_bitmap	 },    /* "mpr_static"             */
    { 0x4465636c, 0x4465636c, ft_bitmap	 },    /* "DeclareBitmap"          */
    { 0xa7590000, 0xa7590000, ft_bitmap	 },    /* binary xor'd		   */
    { 0x50340a23, 0x50340a23, ft_bitmap	 },    /* "P4\n#" (Portable Pixmap)*/
    { 0x25210000, 0x2521ffff, ft_pscript },    /* "%!"    (PostScript)     */
    { 0x00000000, 0xffffffff, ft_unkn	 },    /* stopper		   */
  };

forward char *getfname();
forward enum ftype filetype();


global inittranslit()	/* called from format module */
  { mypid = getpid();
    numtrs = 0;
  }

global tidytranslit()
  { char cmd[MAXSTRLEN+1];
    sprintf(cmd, "/bin/rm -f /tmp/%05d.*", mypid);
    system(cmd);
  }

global int tabtranslit(c, d) char c; int d;
  { char v[MAXSTRLEN+1];
    v[0] = c; /* 'h' or 'v' */
    sprintf(&v[1], "%d", d); /* distance in millipixels */
    return lookuptranslit(lookupword(v));
  }

global int symtranslit(w) char *w;
  { int tc;
    if (w[0] == 'c' || w[0] == 'x')
      { /* ligature (dynamically-bound font) */
	tc = makeligature(w);
      }
    else
      { /* transliteration (statically-bound font) */
	tc = lookuptranslit(w);
      }
    return tc;
  }

global int boxtranslit(wid, hgt) int wid, hgt;
  { char fn[MAXSTRLEN+1], trv[MAXSTRLEN+1];
    wid = (wid+500)/1000; hgt = (hgt+500)/1000; /* cvt to points */
    sprintf(fn, "/tmp/%05d.B%03d%03d", mypid, wid, hgt);
    if (access(fn, 4) < 0)
      { /* file does not exist; make it */
	char cmd[MAXSTRLEN+1]; int code;
	sprintf(cmd, "/usr/fisher/bin/mkbox -%c %d %d >%s", (pmode ? 'p' : 'b'), wid, hgt, fn);
	code = system(cmd);
	if (code != 0) fprintf(messout, "Can't make box!\n");
      }
    sprintf(trv, "%c\"%s\"", (pmode ? 'p' : 'b'), fn);
    return lookuptranslit(lookupword(trv));
  }

static int makeligature(w) char *w;
  { /* max. 3 chars in ligature */
    int tc = 0, k = 0;
    while (k < 6 && (w[k] == 'c' || w[k] == 'x') && (w[k+1] != '\0'))
      { tc = (tc << 8) | (w[k+1] & 0xff);
	if (w[k] == 'x') tc |= 0x80;
	k += 2;
      }
    unless (w[k] == '\0') fprintf(messout, "Bad ligature \"%s\"\n", w);
    while (k < 6) { tc <<= 8; k += 2; }
    return (tc | LIGBIT);
  }

static int lookuptranslit(w) char *w;
  { int tc, i;
    i = numtrs-1;
    while (i >= 0 && trtab[i] != w) i--;
    if (i >= 0) tc = MINTCHAR+i;
    else
      { struct trlit trlit; bool ok;
	if (numtrs >= MAXTCHAR-MINTCHAR+1)
	  { fprintf(messout, "Translit set is full (no problem)\n");
	    numtrs = 0;
	  }
	tc = MINTCHAR+numtrs;
	ok = decode(&trlit, w);
	if (ok)
	  { fontinfo[0] -> advance[tc].x = trlit.wid; /* set width in fontinfo entry of new char */
	    outproc(o_deftrlit, tc, &trlit);	      /* define the transliteration */
	  }
	else fprintf(messout, "Bad transliteration \"%s\"\n", w);
	trtab[numtrs++] = w;
      }
    return tc;
  }

static bool decode(tr, w) struct trlit *tr; char *w;
  { bool ok = true, end = false, wset = false;
    int font = -1;
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
		setnumber(wptr, &tr -> wid, n_signok | n_unitsok);
		wset = true; end = true;
		break;

	    case 'v':
		/* store height (may be pos. or neg.) */
		setnumber(wptr, &tr -> hgt, n_signok | n_unitsok);
		end = true;
		break;

	    case 'u':
		tr -> hgt = -indexspacing; /* up a notch */
		break;

	    case 'd':
		tr -> hgt = indexspacing; /* down a notch */
		break;

	    case 'f':
	      { /* set font for subsequent 'c/x' trs. */
		char fn[MAXSTRLEN+1]; int ps;
		splitfontname(wptr, fn, &ps);
		font = lookupfont(fn, ps);
		while ((*wptr >= 'A' && *wptr <= 'Z') || (*wptr >= '0' && *wptr <= '9') || *wptr == '.') wptr++;
		break;
	      }

	    case 'c':	case 'x':
		/* a Character in a Font */
		if (*wptr != '\0' && font >= 0 && tr -> nsu < MAXSUBST)
		  { int uc = *(wptr++) & 0377;
		    if (code == 'x') uc |= 0200;
		    tr -> subst[tr -> nsu++] = (font << 8) + uc;
		    /* set width of sequence equal to width of first character */
		    unless (wset)
		      { tr -> wid = fontinfo[font] -> advance[uc].x;
			wset = true;
		      }
		  }
		else ok = false;
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

static char *getfname(sfx) char *sfx;
  { char fv[MAXSTRLEN+1]; char *fn; int k = 0;
    int sn = strlen(sfx);
    if (*wptr == '"') wptr++;
    until (*wptr == '"' || *wptr == '\0') fv[k++] = *(wptr++);
    if (*wptr == '"') wptr++;
    fv[k++] = '\0';
    fn = heap(k+sn, char);
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

static importbm(fn, tr) char *fn; struct trlit *tr;
  { enum ftype ft = filetype(fn);
    switch (ft)
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

static importps(fn, tr) char *fn; struct trlit *tr;
  { enum ftype ft = filetype(fn);
    switch (ft)
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
	    bool ok;
	    tr -> type = t_pscript;
	    tr -> fn = fn;
	    ok = getbbox(tr);
	    unless (ok) a_giveup("Giving up!");
	    break;
	  }
      }
  }

static cantimport(fn, msg) char *fn, *msg;
  { fprintf(messout, "Can't import file \"%s\": %s\n", fn, msg);
  }

static bool getbbox(tr) struct trlit *tr;
  { bool ok = false;
    FILE *fi = fopen(tr -> fn, "r");
    if (fi != NULL)
      { char line[MAXSTRLEN]; int nc;
	nc = rdline(fi, line);
	while (nc >= 0 && !ok)
	  { if (strncmp(line, "%%BoundingBox:", 14) == 0)
	      { int *box = &tr -> bbox.llx; int p = 14; int k;
		ok = true;
		for (k=0; k<4 && ok; k++)
		  { char wrd[MAXSTRLEN+1]; bool dok;
		    int j = 0;
		    while (p < nc && (line[p] == ' ' || line[p] == '\t')) p++;
		    while (p < nc && (line[p] >= '0' && line[p] <= '9' || line[p] == '.')) wrd[j++] = line[p++];
		    wrd[j++] = 'p'; wrd[j++] = '\0';
		    dok = setnumber(wrd, &box[k], n_upciok);
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

static int rdline(fi, line) FILE *fi; char line[];
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

static setimagewidth(tr) struct trlit *tr;
  { int wid = (tr -> type == t_bitmap) ? bmwidth(tr -> fn) :
	      (tr -> type == t_pscript) ? (tr -> bbox.urx) - (tr -> bbox.llx) : 0;
    tr -> wid = wid / tr -> scale;
  }

static int bmwidth(fn) char *fn;
  { int wid = 0;
    struct bitmap *bm = ReadBitmap(fn);
    if (bm != NULL)
      { wid = bm -> wid * 1000;	  /* width in millipixels */
	Bfree(bm);
      }
    else fprintf(messout, "Can't read bitmap file \"%s\"\n", fn);
    return wid;
  }

static enum ftype filetype(fn) char *fn;
  { enum ftype ft = ft_nonex;
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

