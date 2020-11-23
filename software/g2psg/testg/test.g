GROUP [RS N2]
GROUP [LS N2]
GROUP [NULL [+, -]]
GROUP [PLU [+, -]]

FCR [LS *] => [RS *] # +[M LS *]
FCR [RS *] => [LS *] # +[M RS *]
FCR [M PLU *] => +[PLU *]     { PLU trickles }

Utt: S [PLU -]		      { ought to trickle! }

S:  V2
S:  N2[NULL -], V2[RS N2]     { the telephone Carol tests }
V2: N2[NULL -], V1	      { Carol tests the telephone }
V1: V0, N2		      { tests the telephone	  }

N2: "Carol"
N2: "the telephone"
V0[PLU -]: "tests"
V0[PLU +]: "test"

N2[LS N2, NULL +]: GAP

