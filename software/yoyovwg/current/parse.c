#include "vwg.h"
#include <stdio.h>

#define MAXLINE	   256

static struct dictnode *sentence[MAXSENTLEN+1];
static struct dictnode *predictword;
static char line[MAXLINE+1];
static int lineptr;
static struct state *topstate;
static struct tree *toptree;

extern FILE *pinfile, *poutfile;

extern struct state *parsesentence();			/* from earley	*/
extern printstates(), printrules();			/* from earley	*/
extern struct tree *makeftree();			/* from ftree	*/
extern printftree();					/* from ftree	*/
extern struct dictnode *lookupword();			/* from common	*/
extern wrtime();					/* from common	*/
extern freezeheap(), resetheap(), wrheap();		/* from heap	*/


global parse()
  { bool gotline, doneparse;
    freezeheap();
    wrstats("Start of run");
    predictword = NULL;
    doneparse = false;
    gotline = readline();
    while (gotline)
      { if (line[0] == '.')
	  { if (doneparse || (line[1] == 'p' && line[2] == 'w')) dotcommand();
	    else fprintf(poutfile, "Type a sentence first!\n");
	    fprintf(poutfile, ".\n");
	  }
	else
	  { resetheap();
	    lookupsentence();	/* sets up sentence */
	    freezeheap();	/* in case there were any new (undef'd) terminals */
	    topstate = parsesentence(sentence, predictword);
	    toptree = makeftree(topstate);
	    doneparse = true;
	  }
	fflush(poutfile);
	gotline = readline();
      }
    wrstats("End of run");
    resetheap();
  }

static bool readline()
  { int ch; bool got;
    if (isatty(pinfile -> _file)) fprintf(stderr, "\nParse: ");
    ch = getc(pinfile);
    if (ch == EOF) got = false;
    else
      { int nc = 0;
	until (ch == '\n' || ch == EOF || nc >= MAXLINE)
	  { line[nc++] = ch;
	    ch = getc(pinfile);
	  }
	unless (ch == '\n' || ch == EOF) printf("\n*** line too long!\n");
	until (ch == '\n' || ch == EOF) ch = getc(pinfile);
	line[nc++] = '\0';
	wrstats("Parsing: %s", line);
	got = true;
      }
    return got;
  }

static wrstats(s, p1) char *s; word p1;
  { if (isatty(pinfile -> _file))
      { printf("\n*** "); wrheap(); putchar(' ');
	wrtime(); printf("   ");
	printf(s, p1); putchar('\n');
      }
  }

static lookupsentence()
  { int n; bool got;
    char tok[MAXNAMELEN+1];
    lineptr = 0; n = 0;
    got = nexttoken(tok);
    while (got && n < MAXSENTLEN)
      { sentence[n++] = lookupword(tok, true);
	got = nexttoken(tok);
      }
    if (got) printf("Sentence is too long!\n");
    sentence[n++] = NULL;
  }

static bool nexttoken(tok) char *tok;	/* Read next token, prefering longer tokens to shorter ones */
  { int nc; bool got;
    readtoken(tok);
    nc = strlen(tok);
    if (nc > 0)
      { while (nc > 0 && !foundterm(tok))
	  { tok[--nc] = '\0';
	    lineptr--;
	  }
	if (nc == 0)
	  { /* token not in dictionary as a terminal; re-read the full token */
	    readtoken(tok);
	  }
	got = true;
      }
    else got = false; /* end of line */
    return got;
  }

static readtoken(tok) char *tok;
  { int nc = 0;
    while (line[lineptr] == ' ' || line[lineptr] == '\t') lineptr++;
    until (line[lineptr] == ' ' || line[lineptr] == '\t' || line[lineptr] == '\0' || nc >= MAXNAMELEN)
      tok[nc++] = line[lineptr++];
    unless (line[lineptr] == ' ' || line[lineptr] == '\t' || line[lineptr] == '\0')
      printf("Token is too long!\n");
    tok[nc++] = '\0';
  }

static bool foundterm(tok) char *tok;
  { struct dictnode *x = lookupword(tok, false);
    return (x != NULL) && (x -> flg & f_term);
  }

static dotcommand()
  { if (line[1] == 'p')
      { switch (line[2])
	  { default:
		badcmd();
		break;

	    case 'w':
	      { int k = 3;
		while (line[k] == ' ' || line[k] == '\t') k++;
		if (line[k] == '\0') predictword = NULL;
		else
		  { predictword = lookupword(&line[k], true);
		    freezeheap();
		  }
		break;
	      }

	    case 's':
		printstates();
		break;

	    case 'r':
		printrules();
		break;

	    case 't':
		printftree(toptree, (line[3] == 'x'));
		break;
	  }
      }
    else badcmd();
  }

static badcmd()
  { fprintf(poutfile, "I don't understand.\n");
    fprintf(poutfile, "Cmds are:  .pw    set predict word\n");
    fprintf(poutfile, "           .ps    print states\n");
    fprintf(poutfile, "           .pr    print rules\n");
    fprintf(poutfile, "           .pt    print tree\n");
  }

