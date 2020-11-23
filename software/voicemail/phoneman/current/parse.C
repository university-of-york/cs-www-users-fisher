/* phoneman -- telephone manager   A.J. Fisher	 March 1996
   parse -- parse script */

#include <stdio.h>
#include "phoneman.h"

#define REC_DURATION	40	/* dflt max record duration is 40 secs */

static FILE *scriptfile;
static int linenum, ch;
static bool anyerrors;
static symbol symb;
static char *item;

struct resvdwd { char *wd; symbol sy; };

static resvdwd resvdwds[] =
  { { "chdir",   s_chdir   },
    { "error",   s_error   },
    { "print",   s_print   },
    { "speak",   s_speak   },
    { "record",  s_record  },
    { "seize",   s_seize   },
    { "release", s_release },
    { "wtring",  s_wtring  },
    { "sleep",   s_sleep   },
    { "loop",    s_loop    },
    { "accept",  s_accept  },
    { "fax",     s_fax     },
    { "run",     s_run     },
    { "connect", s_connect },
    { "end",     s_end     },
    { NULL },			/* stopper */
  };

static pnode *parsesequence(), *parsestmt();
static void checkfor(symbol), nextsymb(), string();
static void identifier();
static char *substp(char*), *symbformat(symbol), *charformat(int);
static void scripterror(char*, word = 0, word = 0);
static void nextchar();


global pnode *parsescript(char *fn)
  { scriptfile = fopen(fn, "r");
    if (scriptfile == NULL) giveup("can't open %s", fn);
    anyerrors = false; linenum = 0; ch = '\n';
    item = "";  /* always points to a valid string */
    nextchar(); nextsymb();
    while (symb == s_chdir)	/* do "chdir" at parse time, so file access checks work */
      { nextsymb();
	int code = chdir(item);
	unless (code == 0) scripterror("chdir to %s failed", item);
	checkfor(s_string);
      }
    pnode *x = parsesequence();
    checkfor(s_eof);
    fclose(scriptfile);
    if (anyerrors) exit(2);
    return x;
  }

static pnode *parsesequence()
  { pnode *x = NULL;
    until (symb == s_eof || symb == s_end || symb == s_number)
      { pnode *y = parsestmt();
	x = (x == NULL) ? y : new pnode(s_seq, x, y);
      }
    return x;
  }

static pnode *parsestmt()
  { top:
    switch (symb)
      { default:
	    scripterror("%s found where statement expected", symbformat(symb));
	    nextsymb();
	    goto top;

	case s_error:
	    nextsymb();
	    return new pnode(s_accept);	 /* "error" is signalled as empty "accept" */

	case s_print:
	  { char *s = NULL;
	    nextsymb();
	    if (symb == s_string)
	      { s = item;
		nextsymb();
	      }
	    return new pnode(s_print, s);
	  }

	case s_speak:	case s_record:
	  { symbol sy = symb;
	    nextsymb();
	    char *s = item;
	    if (sy == s_speak)
	      { unless (exists(s)) scripterror("can't access file %s", s);
	      }
	    checkfor(s_string);
	    pnode *x = new pnode(sy, s);
	    if (symb == s_number && ch != ':')	/* careful! */
	      { x -> num = atoi(item);
		nextsymb();
	      }
	    else x -> num = (sy == s_speak) ? 1 : REC_DURATION;
	    return x;
	  }

	case s_seize:	case s_release: case s_wtring:
	  { symbol sy = symb;
	    nextsymb();
	    return new pnode(sy);
	  }

	case s_sleep:
	  { nextsymb();
	    pnode *x = new pnode(s_sleep, atoi(item));
	    checkfor(s_number);
	    return x;
	  }

	case s_loop:
	  { nextsymb();
	    pnode *x = parsesequence();
	    checkfor(s_end);
	    if (x == NULL) scripterror("body of loop is empty");
	    return new pnode(s_loop, x);
	  }

	case s_accept:
	  { nextsymb();
	    pnode *x = NULL; int maxd = 0;
	    while (symb == s_number || symb == s_fax)
	      { char *num = (symb == s_number) ? item : "f";
		int nd = strlen(num); if (nd > maxd) maxd = nd;
		nextsymb(); checkfor(s_colon);
		pnode *y = new pnode(s_branch, parsesequence());
		y -> str1 = num;
		x = (x == NULL) ? y : new pnode(s_or, x, y);
	      }
	    checkfor(s_end);
	    x = new pnode(s_accept, x);
	    x -> num = maxd;
	    return x;
	  }

	case s_run:
	  { nextsymb();
	    char *s = item;
	    checkfor(s_string);
	    pnode *x = new pnode(s_run, s);
	    if (symb == s_redirect)
	      { nextsymb();
		x -> str2 = item;
		checkfor(s_string);
	      }
	    return x;
	  }

	case s_connect:
	  { nextsymb();
	    char *s = item;
	    checkfor(s_string);
	    return new pnode(s_connect, s);
	  }
      }
  }

static void checkfor(symbol sy)
  { if (symb == sy)
      { unless (symb == s_eof) nextsymb();
      }
    else scripterror("%s found where %s expected", symbformat(symb), symbformat(sy));
  }

static void nextsymb()
  { top:
    switch (ch)
      { default:
	    scripterror("ilgl char %s", charformat(ch));
	    nextchar();
	    goto top;

	case ' ':   case '\t':	case '\n':
	    nextchar();
	    goto top;

	case '/':
	    do nextchar(); until (ch == '\n' || ch == EOF);
	    goto top;

	case ':':
	    nextchar();
	    symb = s_colon;
	    break;

	case EOF:
	    symb = s_eof;
	    break;

	case '>':
	    nextchar();
	    symb = s_redirect;
	    break;

	case '\'':  case '"':
	    string();
	    break;

	case 'a':   case 'b':	case 'c':   case 'd':	case 'e':   case 'f':
	case 'g':   case 'h':	case 'i':   case 'j':	case 'k':   case 'l':
	case 'm':   case 'n':	case 'o':   case 'p':	case 'q':   case 'r':
	case 's':   case 't':	case 'u':   case 'v':	case 'w':   case 'x':
	case 'y':   case 'z':
	case '0':   case '1':	case '2':   case '3':	case '4':   case '5':
	case '6':   case '7':	case '8':   case '9':
	    identifier();
	    break;
      }
  }

static void string()
  { char str[MAXSTR+1]; int n = 0;
    int qu = ch;
    nextchar();
    until (ch == qu || ch == '\n' || ch == EOF || n >= MAXSTR)
      { str[n++] = ch;
	nextchar();
      }
    if (ch == qu) nextchar();
    else scripterror("%s found where %s expected", charformat(ch), charformat(qu));
    str[n] = '\0';
    symb = s_string;
    item = substp(copystr(str));
  }

static void identifier()
  { char vec[MAXSTR+1]; int n = 0; bool numok = true;
    while (n < MAXSTR && ((ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9')))
      { vec[n++] = ch;
	unless ((ch >= '0' && ch <= '9') || (ch == 'x')) numok = false;
	nextchar();
      }
    vec[n] = '\0';
    int k = 0;
    until (resvdwds[k].wd == NULL || seq(resvdwds[k].wd, vec)) k++;
    if (resvdwds[k].wd != NULL) symb = resvdwds[k].sy;
    else
      { unless (numok) scripterror("illegal number or unknown reserved word: %s", vec);
	symb = s_number;
	item = substp(copystr(vec));
      }
  }

static char *substp(char *v1)
  { /* subst for all vars except $num, $uid, $time, which are left until obey time */
    return substparams(v1, scripterror, NULL, NULL, NULL);
  }

static char *symbformat(symbol sy)
  { switch (sy)
      { default:	  return "???";
	case s_number:	  return "<number>";
	case s_string:	  return "<string>";
	case s_eof:	  return "<end of file>";
	case s_redirect:  return "'>'";
	case s_colon:	  return "':'";
	case s_chdir:	  return "'chdir'";

	case s_print:	  return "'print'";
	case s_speak:	  return "'speak'";
	case s_record:	  return "'record'";
	case s_seize:	  return "'seize'";
	case s_release:	  return "'release'";
	case s_wtring:	  return "'wtring'";
	case s_sleep:	  return "'sleep'";
	case s_loop:	  return "'loop'";
	case s_accept:	  return "'accept'";
	case s_fax:	  return "'fax'";
	case s_run:	  return "'run'";
	case s_connect:	  return "'connect'";
	case s_end:	  return "'end'";
      }
  }

static char *charformat(int c)
  { char *str = new char[5];
    sprintf(str, (c < 0) ? "EOF" :
		 (c >= 040 && c <= 0176) ? "'%c'" :
		 (c <= 0377) ? "\\%03o" : "???", c);
    return str;
  }

static void scripterror(char *msg, word p1, word p2)
  { fprintf(stderr, "phoneman: line %d: ", linenum);
    fprintf(stderr, msg, p1, p2); putc('\n', stderr);
    anyerrors = true;
  }

static void nextchar()
  { if (ch == '\n') linenum++;
    ch = getc(scriptfile);
  }

