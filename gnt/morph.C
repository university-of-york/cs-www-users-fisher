#include <stdio.h>
#include <string.h>
#include <libcgi.h>

#include "gnt.h"

#define NUMCLASSES 10

static char *classtab[NUMCLASSES] =
  { "Major1", "Major2", "Person", "Tense", "Voice",
    "Mood", "Case", "Number", "Gender", "Degree",
  };

static char *valuetab[] =
  { "", "noun", "adjective", "pronoun", "conjunction", "particle", "interjection", "?7",
	"preposition", "adverb", "verb", "?11", "?12", "?13", "?14", "?15",
    "", "article", "demonstrative", "interrogative/indefinite", "personal/possessive", "relative", "?6", "?7",
    "", "1st", "2nd", "3rd",
    "", "present", "imperfect", "future", "aorist", "perfect", "pluperfect", "?7",
    "", "active", "middle", "passive",
    "", "indicative", "imperative", "subjunctive", "optative", "infinitive", "participle", "?7",
    "", "nominative", "genitive", "dative", "accusative", "vocative", "?6", "?7",
    "", "singular", "dual", "plural",
    "", "masculine", "feminine", "neuter",
    "", "comparative", "superlative", "?3",
  };

static int fltab[NUMCLASSES] = { 4, 3, 2, 3, 2, 3, 3, 2, 2, 2 };    /* num. of bits for each field */


global void writemorph(uint m)
  { int p = 0;
    for (int i = 0; i < NUMCLASSES; i++)
      { int nb = fltab[i];
	uint size = 1 << nb;
	uint val = m & (size-1);
	char *cs = classtab[i], *vs = valuetab[p+val];
	if (*vs != '\0') printf("<tr> <td> %s: </td> <td> %s </td> </tr>\n", cs, vs);
	p += size; m >>= nb;
      }
  }

global void parsemorph(uint &morph, uint &mask)
  { morph = mask = 0;
    for (int i = NUMCLASSES-1; i >= 0; i--)
      { int nb = fltab[i];
	morph <<= nb; mask <<= nb;
	int m = atoi(getval(classtab[i]));
	if (m > 0)
	  { uint size = 1 << nb;
	    morph |= m;
	    mask |= (size-1);
	  }
      }
  }

