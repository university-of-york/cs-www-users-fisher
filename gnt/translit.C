#include <stdio.h>
#include <string.h>

#include "gnt.h"

static void prepass(char*, char*), lg_translit(char*, char*), gl_translit(char*, char*);
static void appends(char*, int&, char*);


global char *latin_to_greek(char *lats)
  { char temps[MAXSTR+1], grks[MAXSTR+1];
    if (charin('(',lats) || charin(')',lats)) strcpy(temps, lats);	/* explicit breathing */
    else prepass(lats, temps);
    lg_translit(temps, grks);
    return copystring(grks);
  }

inline bool islatvow(char ch) { return (ch != '\0') && charin(ch, "aeiouw^"); }

static void prepass(char *vec1, char *vec2)
  { int j = 0, k = 0;
    switch (vec1[0])
      { default:
	    strcpy(&vec2[k], &vec1[j]);
	    break;

	case 'a':   case 'e':	case 'i':   case 'o':	case 'u':   case 'w':
	    while (islatvow(vec1[j])) vec2[k++] = vec1[j++];
	    vec2[k++] = ')';
	    strcpy(&vec2[k], &vec1[j]);
	    break;

	case 'h':
	    j++;
	    if (vec1[j] == 'r')
	      { j++; vec2[k++] = 'r';
	      }
	    else
	      { while (islatvow(vec1[j])) vec2[k++] = vec1[j++];
	      }
	    vec2[k++] = '(';	/* rough breathing */
	    strcpy(&vec2[k], &vec1[j]);
	    break;

	case 'A':   case 'E':	case 'I':   case 'O':	case 'U':   case 'W':
	    vec2[k++] = ')';	/* soft breathing */
	    strcpy(&vec2[k], &vec1[j]);
	    break;

	case 'H':
	    j++;
	    vec2[k++] = '(';	/* rough breathing */
	    strcpy(&vec2[k], &vec1[j]);
	    if (vec2[k] >= 'a' && vec2[k] <= 'z') vec2[k] += ('A'-'a');
	    break;

	case 'r':
	    /* be generous - insert a rough breathing */
	    j++;
	    vec2[k++] = 'r'; vec2[k++] = '(';
	    if (vec1[j] == 'h') j++;
	    strcpy(&vec2[k], &vec1[j]);
	    break;

	case 'R':
	    /* be generous - insert a rough breathing */
	    j++;
	    vec2[k++] = '('; vec2[k++] = 'R';
	    if (vec1[j] == 'h') j++;
	    strcpy(&vec2[k], &vec1[j]);
	    break;
      }
  }

static void lg_translit(char *lats, char *grks)
  { int j = 0, k = 0;
    until (lats[j] == '\0')
      { int ch = lats[j++];
	switch (ch)
	  { default:
		hfatal("Bad Latin char `%c'.  Check transliteration.", ch);

	    case 'a':	case 'b':   case 'd':	case 'f':   case 'g':
	    case 'k':	case 'l':   case 'm':	case 'n':   case 'q':
	    case 'r':	case 'u':   case 'w':	case 'x':   case 'z':
	      { char *tr = "\200b.d.fg...klmn..qr..\205.\206x.z";
		grks[k++] = tr[ch-'a'];
		break;
	      }

	    case 'c':
		if (lats[j] == 'h') { grks[k++] = 'c'; j++; }
		else grks[k++] = 'k';	/* be generous */
		break;

	    case 'e':
		if (lats[j] == '^') { grks[k++] = 0202; j++; }
		else grks[k++] = 0201;
		break;

	    case 'i':
		if (lats[j] == '^') { grks[k++] = 0202; j++; }
		else grks[k++] = 0203;
		break;

	    case 'o':
		if (lats[j] == '^') { grks[k++] = 0206; j++; }
		else grks[k++] = 0204;
		break;

	    case 'p':
		if (lats[j] == 'h') { grks[k++] = 'f'; j++; }
		else if (lats[j] == 's') { grks[k++] = 'y'; j++; }
		else grks[k++] = 'p';
		break;

	    case 's':
		grks[k++] = (lats[j] == '\0') ? 'V' : 's';
		break;

	    case 't':
		if (lats[j] == 'h') { grks[k++] = 'q'; j++; }
		else grks[k++] = 't';
		break;

	    case 'A':	case 'B':   case 'D':	case 'F':   case 'G':
	    case 'K':	case 'L':   case 'M':	case 'N':   case 'Q':
	    case 'R':	case 'S':   case 'U':	case 'W':   case 'X':
	    case 'Z':
	      { char *tr = "AB.D.FG...KLMN..QRS.U.WX.Z";
		grks[k++] = tr[ch-'A'];
		break;
	      }

	    case 'C':
		if (lats[j] == 'h') { grks[k++] = 'C'; j++; }
		else grks[k++] = 'K';	/* be generous */
		break;

	    case 'E':
		if (lats[j] == '^') { grks[k++] = 'H'; j++; }
		else grks[k++] = 'E';
		break;

	    case 'I':
		if (lats[j] == '^') { grks[k++] = 'H'; j++; }
		else grks[k++] = 'I';
		break;

	    case 'O':
		if (lats[j] == '^') { grks[k++] = 'W'; j++; }
		else grks[k++] = 'O';
		break;

	    case 'P':
		if (lats[j] == 'h') { grks[k++] = 'F'; j++; }
		else if (lats[j] == 's') { grks[k++] = 'Y'; j++; }
		else grks[k++] = 'P';
		break;

	    case 'T':
		if (lats[j] == 'h') { grks[k++] = 'Q'; j++; }
		else grks[k++] = 'T';
		break;

	    case '0':	case '1':   case '2':	case '3':   case '4':	case '5':
	    case '6':	case '7':   case '8':	case '9':   case '.':	case '\'':
		grks[k++] = ch;
		break;

	    case ')':	case '(':
		if (k > 0)
		  { if ((uchar) grks[k-1] >= 0200 && (uchar) grks[k-1] <= 0206)
		      grks[k-1] += (ch == ')' ? 10 : 20);
		    else if ((uchar) grks[k-1] == 0162 && ch == '(') grks[k-1] = 0166;
		  }
		else grks[k++] = ch;
		break;
	  }
      }
    grks[k] = '\0';
  }

global char *greek_to_latin(char *grks)
  { char lats[MAXSTR+1];
    gl_translit(grks, lats);
    return copystring(lats);
  }

static void gl_translit(char *grks, char *lats)
  { int j = 0, k = 0;
    until (grks[j] == '\0')
      { int ch = (uchar) grks[j++];
	if ((ch >= '0' && ch <= '9') || (ch == '.')) lats[k++] = ch;
	else if (ch >= 64 && ch < 128)
	  { char *tr[] =
	      { "?", "A", "B", "Ch", "D", "E", "F", "G", "E^", "I", "?", "K", "L", "M", "N", "O",
		"P", "Th", "R", "S", "T", "U", "s", "O^", "X", "Ps", "Z", "?", "?", "?", "?", "?",
		"?", "a", "b", "ch", "d", "e", "f", "g", "e^", "i", "?", "k", "l", "m", "n", "o",
		"p", "th", "r", "s", "t", "u", "rh", "o^", "x", "ps", "z", "?", "?", "?", "?", "?",
	      };
	    appends(lats, k, tr[ch-64]);
	  }
	else if (ch >= VOWELS && ch < VOWELS+120)
	  { ch = (ch-VOWELS) % 30;
	    char *tr[] = { "a", "e", "e^", "i", "o", "u", "o^", "a|", "e^|", "o^|" };
	    appends(lats, k, tr[ch%10]);
	    if (ch >= 10 && ch < 20) lats[k++] = ')';
	    if (ch >= 20) lats[k++] = '(';
	  }
	else lats[k++] = '?';
      }
    lats[k] = '\0';
  }

static void appends(char *vec, int &p, char *s)
  { strcpy(&vec[p], s);
    until (vec[p] == '\0') p++;
  }

