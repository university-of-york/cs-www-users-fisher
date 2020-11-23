``Yo-yo'' parser for van Wijngaarden grammars

This directory contains an implementation by the author of the VWG parser
described in:

   A.J. Fisher,
   A "yo-yo" parsing algorithm for a large class of van Wijngaarden grammars,
   Acta Informatica 29 461-481 (1992)

At the prompt "Parse: " you can type a candidate sentence for parsing, or one
of the following commands:

	.pr	print rule set (strict syntax)
	.ps	print Earley state sets
	.pt	print parse tree(s)		.ptx	ditto, different format

Here is an example of the use of the parser.

----
Anthony Fisher
Dept. of Computer Science, The University of York, York YO1 5DD, U.K.
Tel.:	  +44 1904 432738 or 432722		Fax: +44 1904 432767
Internet: fisher@minster.york.ac.uk
Janet:	  fisher@uk.ac.york.minster

-------------------------------------------------------------------------------

$ cat anbncn.vwg
TALLY::		  i TALLETY.
TALLETY::	  TALLY; EMPTY.
EMPTY::		  .
LETTER::	  a; b; c.

program:	  TALLY as, TALLY bs, TALLY cs.

i TALLY LETTER s: i LETTER s, TALLY LETTER s.
i LETTER s:	  LETTER symbol.

a symbol:	  "a".
b symbol:	  "b".
c symbol:	  "c".

$ vwg anbncn.vwg

*** 2224 bytes (1/400) 00h 00m 00.1s   Start of run

Parse: a a a b b b c c c

*** 2224 bytes (1/400) 00h 00m 00.1s   Parsing: a a a b b b c c c
 a a a b b b c c c YES

Parse: .pr

*** 42948 bytes (6/400) 00h 00m 00.2s	Parsing: .pr
csymbol*: "c".
bsymbol*: "b".
asymbol*: "a".
ics*: csymbol*.
ibs*: bsymbol*.
ias*: asymbol*.
iics*: ics*, ics*.
iibs*: ibs*, ibs*.
iias*: ias*, ias*.
_top*: program*.
iiics*: ics*, iics*.
iiibs*: ibs*, iibs*.
iiias*: ias*, iias*.
program*: ias*, ibs*, ics*.
program*: iias*, iibs*, iics*.
program*: iiias*, iiibs*, iiics*.

Parse: .pt

*** 42948 bytes (6/400) 00h 00m 00.2s	Parsing: .pt
{
program*
..iiias*
....ias*
......asymbol*
........"a"
....iias*
......ias*
........asymbol*
.........."a"
......ias*
........asymbol*
.........."a"
..iiibs*
....ibs*
......bsymbol*
........"b"
....iibs*
......ibs*
........bsymbol*
.........."b"
......ibs*
........bsymbol*
.........."b"
..iiics*
....ics*
......csymbol*
........"c"
....iics*
......ics*
........csymbol*
.........."c"
......ics*
........csymbol*
.........."c"
}

Parse: ^D
*** 42948 bytes (6/400) 00h 00m 00.3s	End of run
$

