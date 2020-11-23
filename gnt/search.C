#include <stdio.h>
#include <string.h>
#include <libcgi.h>

#include "gnt.h"

#define HASHSIZE    16384
#define BUCKETSIZE  9

enum Smode { Any, Infl, Base };

struct SearchParams
  { SearchParams()
      { smode = Any; swords = NULL;
	mask = morph = book = 0;
      }
    ~SearchParams() { delete swords; }
    Smode smode;
    IntVec *swords;
    uint mask, morph;
    int book;
  };

static SearchParams *sparams1 = NULL, *sparams2 = NULL;	    /* init'ed to catch errors */

static SearchParams *getsparams(int);
static void notinlexicon(char*, char*);
static IntVec *lookupword(char*, bool);
static char *unaccentword(char*);
static int unaccent(int);
static ContextVec *matchall(SearchParams*);
static ContextVec *matchone(int, Smode, int, uint, uint);
static ContextVec *mergectxts(ContextVec*, ContextVec*);
static ContextVec *combinenear(ContextVec*, ContextVec*);
static void prsparams(SearchParams*);


global ContextVec *performsearch()
  { sparams1 = getsparams(1);
    parsemorph(sparams1 -> morph, sparams1 -> mask);  /* sets morph, mask */
    if (sparams1 -> smode == Any && sparams1 -> mask == 0)
      hfatal("Please enter either a search word or a grammatical category (or both).");
    sparams2 = getsparams(2);
    if (isset("book"))
      { int bk = atoi(getval("book"));
	sparams1 -> book = sparams2 -> book = bk;
      }
    ContextVec *cvec = matchall(sparams1);
    if (sparams2 -> smode != Any)
      { ContextVec *cvec2 = matchall(sparams2);
	cvec = combinenear(cvec, cvec2);
      }
    return cvec;
  }

static SearchParams *getsparams(int n)
  { SearchParams *sp = new SearchParams;
    char key[16];
    sprintf(key, "sword%d", n);
    char *lsw = getval(key);
    if (lsw[0] != '\0')
      { char *gsw = latin_to_greek(lsw);
	sp -> swords = lookupword(gsw, false);
	if (sp -> swords -> num == 0)
	  { notinlexicon(key, gsw);
	    exit(0);
	  }
	sprintf(key, "smode%d", n);
	char *sm = getval(key);
	sp -> smode = (Smode) atoi(sm); /* Infl or Base */
      }
    return sp;
  }

static void notinlexicon(char *key, char *gsw)
  { prtitle("Not In Lexicon");
    printf("<a name=h>\n");
    printf("<img src=//www.york.ac.uk/icons/at-work.gif align=left>\n");
    printf("<b> <font size=+1>\n");
    printf("   Experimental Word Correction Facility: <br>\n");
    printf("   no bug reports yet please!\n");
    printf("</font> </b>\n");
    printf("<br clear=all> <p>\n");
    printf("%s <br>\n", grksimage(gsw));
    printf("That word is not in the lexicon.\n");
    IntVec *vec = lookupword(gsw, true);    /* find all entries in the same hash bucket */
    if (vec -> num > 0)
      { printf("Please either\n");
	printf("<ul>\n");
	printf("   <li> select one of the similar Greek words from the list below and click the ``Go!'' button, or\n");
	printf("   <li> press ``back'' and try again.\n");
	printf("</ul>\n");
	printf("<form action=gnt#h method=POST>\n");
	for (int i = 0; i < numentries; i++)
	  { entry *e = &entries[i];
	    unless (seq(e -> nam, key))
	      printf("   <input type=hidden name=%s value=\"%s\">\n", e -> nam, e -> val);
	  }
	printf("   <table>\n");
	for (int i = 0; i < vec -> num; i++)
	  { char *xgsw = getword(vec -> vec[i]);
	    char *xlsw = greek_to_latin(xgsw);
	    printf("      <tr>\n");
	    printf("         <td> %d.\n", i+1);
	    printf("         <td> <input type=radio name=%s value=\"%s\" %s>\n", key, xlsw, (i == 0 ? "checked" : ""));
	    printf("         <td> %s\n", xlsw);
	    printf("         <td> %s\n", wordimage(vec -> vec[i]));
	  }
	printf("   </table>\n");
	printf("   <input type=submit value=\"Go!\">\n");
	printf("</form>\n");
      }
    else
      { printf("I can find no similar Greek words.  Please press ``back'' and try again. <br>\n");
      }
  }

static IntVec *lookupword(char *s, bool all)
  { IntVec *ivec = new IntVec;
    /* hash function, same as in mkbinary */
    int hix = 0;
    for (int i = 0; s[i] != '\0'; i++) hix = (hix * 33119 + 37087 + (uchar) s[i]) & 0x7fffffff;
    hix &= (HASHSIZE-1);
    int ptr = get3(19) + (hix * BUCKETSIZE * 3);
    for (int i = 0; i < BUCKETSIZE; i++)
      { int wdo = get3(ptr);
	if (wdo == 0) break;	/* not found */
	if (all || seq(unaccentword(getword(wdo)), s)) ivec -> add(wdo);
	ptr += 3;
      }
    return ivec;
  }

static char *unaccentword(char *s)
  { char v[MAXSTR+1];
    int j = 0, k = 0;
    until (s[j] == '\0')
      { int ch = unaccent((uchar) s[j++]);
	if (ch >= 0) v[k++] = ch;
      }
    v[k] = '\0';
    return copystring(v);
  }

static int unaccent(int ch)	/* remove accent from ch */
  { if (ch >= VOWELS+30 && ch < VOWELS+120) return (ch - VOWELS) % 30 + VOWELS;
    else switch (ch)
      { default:
	    return ch;

	case 0052:  case 0053:
	    return ch-2;

	case 0057:  case 0134:	case 0075:
	    return -1;
      }
  }

static ContextVec *matchall(SearchParams *sp)
  { ContextVec *cvec;
    if (sp -> smode == Any)
      { /* match grammatical category only */
	cvec = matchone(0, Any, sp -> book, sp -> mask, sp -> morph);
      }
    else
      { /* match any alternative word */
	if (sp -> swords -> num == 0) hfatal("Bug: nothing to search for");
	cvec = matchone(sp -> swords -> vec[0], sp -> smode, sp -> book, sp -> mask, sp -> morph);
	for (int i = 1; i < sp -> swords -> num; i++)
	  { ContextVec *cvec2 = matchone(sp -> swords -> vec[i], sp -> smode, sp -> book, sp -> mask, sp -> morph);
	    cvec = mergectxts(cvec, cvec2);
	  }
      }
    return cvec;
  }

inline bool mnext(int swd, Smode smo, uint mask, uint morph)
  { next3();			/* skip display form */
    int x1 = next3();		/* inflected form */
    int x2 = next3();		/* base form */
    uint m = next4();		/* morphology */
    switch (smo)
      { case Infl:
	    if (x1 != swd) return false;
	    break;

	case Base:
	    if (x2 != swd) return false;
	    break;
      }
    return (m & mask) == morph;
  }

static ContextVec *matchone(int swd, Smode smo, int book, uint mask, uint morph)
  { ContextVec *cvec = new ContextVec;
    int start = textstart(book > 0 ? book : 1);
    int stop = textstart(book > 0 ? book+1 : 99);
    seekto(start);
    while (fileptr < stop)
      { int nwd = next1();
	int p = fileptr;		    /* word offset of verse number */
	for (int i = 0; i < nwd; i++)
	  if (mnext(swd, smo, mask, morph)) cvec -> add(Context(p, 0, 0, i));
      }
    for (int i = 0; i < cvec -> num; i++)
      { Context *cx = &cvec -> vec[i];
	int ref = get3(cx -> bk + 3);	    /* shd be the "bk.ch.vn" ref for this verse */
	char *s = getword(ref);
	unless (sscanf(s, "%d.%d.%d", &cx -> bk, &cx -> ch, &cx -> vn) == 3) hfatal("bug in matchall! (%s)", s);
	delete s;
      }
    return cvec;
  }

inline int wordcomp(Context ctxt1, Context ctxt2)
  { return (ctxt1.bk != ctxt2.bk) ? (ctxt1.bk - ctxt2.bk) :
	   (ctxt1.ch != ctxt2.ch) ? (ctxt1.ch - ctxt2.ch) :
	   (ctxt1.vn != ctxt2.vn) ? (ctxt1.vn - ctxt2.vn) : (ctxt1.wn - ctxt2.wn);
  }

static ContextVec *mergectxts(ContextVec *cvec1, ContextVec *cvec2)
  { ContextVec *cvec = new ContextVec;
    Context *v1 = cvec1 -> vec, *v2 = cvec2 -> vec;
    int n1 = cvec1 -> num, n2 = cvec2 -> num;
    int p1 = 0, p2 = 0;
    while (p1 < n1 && p2 < n2)
      { int c = wordcomp(v1[p1], v2[p2]);
	if (c < 0) cvec -> add(v1[p1++]);
	if (c > 0) cvec -> add(v2[p2++]);
      }
    while (p1 < n1) cvec -> add(v1[p1++]);
    while (p2 < n2) cvec -> add(v2[p2++]);
    return cvec;
  }

inline int versecomp(Context ctxt1, Context ctxt2)
  { return (ctxt1.bk != ctxt2.bk) ? (ctxt1.bk - ctxt2.bk) :
	   (ctxt1.ch != ctxt2.ch) ? (ctxt1.ch - ctxt2.ch) : (ctxt1.vn - ctxt2.vn);
  }

static ContextVec *combinenear(ContextVec *cvec1, ContextVec *cvec2)
  { ContextVec *cvec = new ContextVec;
    Context *v1 = cvec1 -> vec, *v2 = cvec2 -> vec;
    int n1 = cvec1 -> num, n2 = cvec2 -> num;
    int p1 = 0, p2 = 0;
    while (p1 < n1)
      { int c;
	while (p2 < n2 && (c = versecomp(v2[p2], v1[p1])) < 0) p2++;
	if (c == 0) cvec -> add(v1[p1]);
	p1++;
      }
    return cvec;
  }

global void writesearch()
  { printf("<b>Search for</b> <p>\n");
    prsparams(sparams1);
    if (sparams2 -> smode != Any)
      { printf("<p> <b>occuring near</b> <p>\n");
	prsparams(sparams2);
      }
  }

static void prsparams(SearchParams *sp)
  { if (sp == NULL) hfatal("Bug! null sp");
    printf("<table border>\n");
    unless (sp -> smode == Any)
      { printf("<tr> <td> %s <br> form: </td>\n", (sp -> smode == Infl) ? "Inflected" : "Base");
	printf("<td>");
	for (int i = 0; i < sp -> swords -> num; i++) printf("%s<br>", wordimage(sp -> swords -> vec[i]));
	printf("</td>\n");
      }
    writemorph(sp -> morph);
    printf("</table>\n");
    if (sp -> book > 0) printf("<p> In book: &nbsp; %s <br>\n", bookname(sp -> book));
  }

