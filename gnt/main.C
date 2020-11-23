#include <stdio.h>
#include <string.h>
#include <new.h>
#include <libcgi.h>

#include "gnt.h"

#define GNTPROG	 "http://www-users.cs.york.ac.uk/~fisher/cgi-bin/gnt"
#define LOCKFILE "/www/usr/fisher/gnt/gnt.lck"

static int occurnum, totoccs, numoccs;

global bool hadtitle;

static void newhandler(), printheader(), logaccess();
static void appends(char*, int&, char*, word = NULL);
static void lockdb(), unlockdb();
static void dothing(), dochapter();
static Context decodeid(char*);
static void redirect(char*), usage_error(char*);
static void dosearch();
static void displaychap(Context), printchaptitle(Context);
static void printdetails(Context);
static void printwordtable(int);
static void printnextform();
static void genchartable();


global int main(int argc, char **argv)
  { ftimes ft = getftimes();
    hadtitle = false;
    set_new_handler(newhandler);
    printheader();
    getentries();
    logaccess();
    lockdb();
    openbinfile();
    dothing();
    closebinfile();
    ftimes dt = getftimes() - ft;
    printf("<small><i>Time: &nbsp; %.3f s cpu, %.3f s real</i></small>\n", dt.ct, dt.rt);
    exit(0);
  }

static void newhandler()
  { hfatal("No room!");
  }

static void printheader()
  { printf("Content-type: text/html\n\n");
    printf("<HTML>\n");
    printf("<body bgcolor=#ffffff>\n");
  }

static void logaccess()
  { char vec[MAXSTR+1]; int p = 0;
    if (isset("id"))
      { appends(vec, p, "id=%s", getval("id"));
	if (isset("xy"))
	  { char *coords = getval("xy");
	    if (*coords == '?') coords++;
	    appends(vec, p, "(%s)", coords);
	  }
      }
    else if (isset("sword1"))
      { char *s1 = getval("sword1"), *s2 = getval("sword2");
	appends(vec, p, "wd=%s", s1);
	if (s2[0] != '\0') appends(vec, p, ":%s", s2);
      }
    else appends(vec, p, "???");
    logweb("gnt", vec);
  }

static void appends(char *vec, int &p, char *fmt, word p1)
  { sprintf(&vec[p], fmt, p1);
    until (vec[p] == '\0') p++;
  }

static void lockdb()
  { bool ok = lock_database(LOCKFILE);
    unless (ok) hfatal("Sorry, database is busy. Please try again.");
    atexit(unlockdb);
  }

static void unlockdb()
  { unlock_database(LOCKFILE);
  }

static void dothing()
  { if (isset("id")) dochapter();
    else if (isset("sword1")) dosearch();
    else if (isset("genchars")) genchartable();
    else usage_error("error 1");
  }

static void dochapter()
  { Context ctxt = decodeid(getval("id"));  /* sets ctxt .bk .ch .vn .wn */
    if (ctxt.bk > 0 && ctxt.ch > 0)
      { if (ctxt.vn > 0 && ctxt.wn < 0 && isset("xy"))
	  { char *coords = getval("xy"); point clickpt;
	    if (sscanf(coords, "?%d,%d", &clickpt.x, &clickpt.y) != 2) usage_error("bad coordinates");
	    locateword(clickpt, ctxt);		/* sets ctxt .wn */
	    if (ctxt.wn < 0) hfatal("You've clicked between two words. Press ``Back'' and try again.");
	    char url[MAXSTR+1]; sprintf(url, "%s?id=%02d%02d%02d%02d#h", GNTPROG, ctxt.bk, ctxt.ch, ctxt.vn, ctxt.wn);
	    redirect(url);
	  }
	else if ((ctxt.vn <= 0 && ctxt.wn < 0) || (ctxt.vn > 0 && ctxt.wn >= 0))
	  { printchaptitle(ctxt);
	    displaychap(ctxt);
	  }
	else usage_error("error 2");
      }
    else usage_error("error 3");
  }

static Context decodeid(char *id)
  { Context ctxt;
    int ni = sscanf(id, "%02d%02d%02d%02d", &ctxt.bk, &ctxt.ch, &ctxt.vn, &ctxt.wn);
    if (ni == 0) { ctxt.bk = 0; ni++; }
    if (ni == 1) { ctxt.ch = 0; ni++; }
    if (ni == 2) { ctxt.vn = 0; ni++; }
    if (ni == 3) { ctxt.wn = -1; ni++; }
    return ctxt;
  }

static void redirect(char *url)
  { discard_output(); hadtitle = false;
    printf("Content-type: text/html\n");
    printf("Status: 301 Redirection\n");
    printf("Location: %s\n\n", url);
    printf("<HTML>\n");
    prtitle("Automatic Redirection");
    prheading("Automatic Redirection", 1);
    printf("You are redirected <a href=%s>here</a>. <br>\n", url);
  }

static void usage_error(char *msg)
  { // system("{ date; echo; env; echo; echo ==========; echo; } >> /www/usr/fisher/gnt/Debug 2>/dev/null");
    char temp[MAXSTR+1]; sprintf(temp, "Usage error! (%s)", msg);
    prtitle(temp); prheading(temp, 3);
    printf("Sorry, your browser has supplied incorrect values to the GNT program. <p>\n");
    printf("<ul>\n");
    printf("   <li> If you are using Netscape on a Macintosh, this is a Known Bug in your browser:\n");
    printf("        please try a different computer. <p>\n");
    printf("   <li> If you came here <i>via</i> a search engine, blame the search engine,\n");
    printf("        and try the correct link below.<p>\n");
    printf("</ul>\n");
    printf("The Greek New Testament home page is <a href=http://www-users.cs.york.ac.uk/~fisher/gnt>here</a>. <p>\n");
    exit(0);
  }

static void dosearch()
  { ContextVec *cvec = performsearch();
    occurnum = atoi(getval("occurnum"));
    totoccs = cvec -> num;
    numoccs = atoi(getval("numoccs"));
    if (numoccs < 1) numoccs = 1;
    if (occurnum < totoccs)
      { if (numoccs > 1)
	  { if (numoccs > totoccs-occurnum) numoccs = totoccs-occurnum;
	    prtitle("Search Results");
	    printf("<a name=h>\n");
	    writesearch();
	    char temp[MAXSTR+1]; sprintf(temp, "Occurrences %d - %d of %d", occurnum+1, occurnum+numoccs, totoccs);
	    prheading(temp, 3);
	    printf("<table>\n");
	    for (int i = 0; i < numoccs; i++)
	      { Context ctxt = cvec -> vec[occurnum+i];
		printf("   <tr valign=top> <td> %d. <td> %s %d ", occurnum+i+1, bookname(ctxt.bk), ctxt.ch);
		char *img = verseimage(Context(ctxt.bk, ctxt.ch, ctxt.vn, ctxt.wn));
		printf("<td><a href=gnt?id=%02d%02d%02d&xy=>%s</a></td>\n", ctxt.bk, ctxt.ch, ctxt.vn, img);
		delete img;
	      }
	    printf("</table>\n");
	    printnextform();
	  }
	else
	  { Context ctxt = cvec -> vec[occurnum];
	    printchaptitle(ctxt);
	    displaychap(ctxt);
	  }
      }
    else
      { prtitle("Search Failed");
	writesearch();
	hfatal((occurnum == 0) ? "No occurrences." : "No more occurrences.");
      }
  }

static void displaychap(Context ctxt)
  { int cho = findcho(ctxt);	/* seek to chapter */
    int nv = get1(cho);		/* num. of verses */
    for (int vn = 1; vn <= nv; vn++)
      { char *img;
	if (vn == ctxt.vn && ctxt.wn >= 0)
	  { printf("<a name=h></a>\n");
	    printf("<p> <hr noshade>\n");
	    printdetails(ctxt);
	    printf("<hr noshade> <p>\n");
	    img = verseimage(Context(ctxt.bk, ctxt.ch, vn, ctxt.wn));
	  }
	else img = verseimage(Context(ctxt.bk, ctxt.ch, vn, -1));
	printf("<a href=gnt?id=%02d%02d%02d&xy=>%s</a><br>\n", ctxt.bk, ctxt.ch, vn, img);
	delete img;
      }
    printf("<hr noshade>\n");
  }

static void printchaptitle(Context ctxt)
  { char tit[MAXSTR+1]; sprintf(tit, "%s %d", bookname(ctxt.bk), ctxt.ch);
    prtitle(tit); prheading(tit, 3);
  }

static void printdetails(Context ctxt)
  { int vso = findvso(ctxt);	/* seek to verse */
    int nwd = get1(vso);
    if (ctxt.wn < 0 || ctxt.wn >= nwd) hfatal("can't find %02d.%02d.%02d.%02d!", ctxt.bk, ctxt.ch, ctxt.vn, ctxt.wn);
    int wptr = vso + 1 + 13*ctxt.wn;
    printf("<ul> <table border>\n");
    printf("   <tr valign=top align=center>\n");
    if (isset("sword1"))
      { printf("      <td width=50%>\n"); printwordtable(wptr);
	printf("      <td width=50%>\n"); writesearch();
	printf("&nbsp; <br> Occurrence %d of %d </br>\n", occurnum+1, totoccs);
	printnextform();
      }
    else
      { printf("      <td>\n"); printwordtable(wptr);
      }
    printf("</table> </ul>\n");
  }

static void printwordtable(int wptr)
  { int x1 = get3(wptr+3);	/* inflected form  */
    int x2 = next3();		/* base form	   */
    uint m = next4();		/* morphology	   */
    printf("<b>Current word</b> <p>\n");
    printf("<table border>\n");
    printf("   <tr> <td> Inflected <br> form: </td> <td>%s</td> </tr>\n", wordimage(x1));
    printf("   <tr> <td> Base      <br> form: </td> <td>%s</td> </tr>\n", wordimage(x2));
    writemorph(m);
    printf("</table>\n");
  }

static void printnextform()
  { printf("<form method=POST>\n");
    for (int i = 0; i < numentries; i++)
      { entry *e = &entries[i];
	unless (seq(e -> nam, "occurnum"))
	  printf("   <input type=hidden name=%s value=\"%s\">\n", e -> nam, e -> val);
      }
    printf("   <input type=hidden name=occurnum value=%d>\n", occurnum+numoccs);
    printf("   <input type=submit value=\"Find Next\">\n");
    printf("</form>\n");
  }

static void genchartable()
  { /* test mode, intended for use by AJF */
    prtitle("genchars");
    printf("<table align=center>\n");
    for (int i = 0; i < 16; i++)
      { printf("   <tr>\n");
	for (int j = 0; j < 16; j++)
	  { int ch = (i << 4) + j;
	    printf("      <td>%s</td>\n", ch, charimage(ch));
	  }
      }
    printf("</table>\n");
  }

