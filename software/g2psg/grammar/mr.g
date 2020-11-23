{REFLEXIVE RULES NEEDED FOR 701 SUBCAT}
{Metarules}

{Passive metarule}
{Generates active and passive verb phrases for passivising subcategories}

BEGIN
   V1[NFORM NORM, PAS -]: V0[H, AUX -, SUBCAT S], N2[CONJ NIL, RE -, NOM -, POSS -] <<, P >>
	  {active verb phrase: 'eat the cat'}
   V1[NFORM NORM, PAS -]: V0[CS, H, SUBCAT S], N2[CS, CONJ NIL, RE +, NOM -, POSS -] <<, P >>
	  {reflexive active verb phrase: 'eat himself'}
   V1[NFORM NORM, PAS -, SLASH N2[RE -]]: V0[H, SUBCAT S], N2[NULL] <<, P >>
   V1[NFORM NORM, PAS -, SLASH N2[RE +]]: V0[CS, H, SUBCAT S], N2[CS, NULL] <<, P >>
	  {SLASHed active verb phrase: 'what did she eat -?'}
   V1[NFORM NORM, VFORM PAS]: V0[H, SUBCAT S] <<, P >>
	  {passive verb phrase, with no by-phrase: 'eaten'}
WITH
   S,P : 2, EMPTY;
	 {eaten}
	 3, P2[PFORM to];
	 {given to Carol}
	 5, N2[CONJ NIL, NOM -, POSS -];
	 {given	 Carol}
	 6, P2[PFORM NORM];
	 {put on the table}
	 602, P2[PFORM off];
	 {switched off}
	 602, P2[PFORM on];
	 {switched on}
	 8, V2[VFORM FIN, TOP, INV -, R -];
	 {persuaded (that) it was a good idea}
	 17,V1[VFORM INF, TOP];
	 {believed to be happy}
	 18, V1[VFORM INF, TOP];
	 {persuadeed to dial}
	 {TOP prevents double extraction: *who did Ruth persuade _ to dial _ }
	 199, V1[VFORM INF, TOP];
	 {told to dial}
	 199, P2[PFORM about];
	 {told about Paul}
	 199, V2[VFORM FIN, TOP, INV -, R -]
	 {told (that) we were going}
END

{SLASH P2 rules for these subcategories}
V1[NFORM NORM, PAS -, SLASH P2[PFORM to]]: V0[H, SUBCAT 3], N2[CONJ NIL, NOM -, POSS -, RE -], P2[NULL]
V1[NFORM NORM, PAS -, SLASH P2[PFORM to]]: V0[CS, H, SUBCAT 3], N2[CS, CONJ NIL, RE +], P2[NULL]
V1[NFORM NORM, PAS -, SLASH P2[PFORM NORM]]: V0[H, SUBCAT 6], N2[CONJ NIL, POSS -, RE -], P2[NULL]
V1[NFORM NORM, PAS -, SLASH P2[PFORM NORM]]: V0[CS, H, SUBCAT 6], N2[CS, CONJ NIL, RE +], P2[NULL]


{Subject auxiliary metarule 1 (SAI 1)}
{The complement of the verb agrees with it in control features, and thus also
with the subject of the sentence}

BEGIN
   V1[AUX +]: V[CS, H], X[CS]
	  {Verb phrase: was eaten by Erik}
	  {Only the first, finite aux. verb can invert with the subj}
   V2[CONJ NIL, INV +, VFORM FIN, R -]: V[CS, H, AUX +], N2[CS, CONJ NIL], X[CS]
	  {Sentence: was she eaten by Erik}
WITH
   V,X: V0[SUBCAT 701, NFORM NORM], V1[VFORM PAS];
	{was dialed}
	V0[SUBCAT 701], V1[VFORM INF];
	{was to go}
	V0[SUBCAT 701], V1[VFORM PRP];
	{was dialing the number}
	V0[SUBCAT 702], V1[VFORM BSE];
	{should eat}
	V0[SUBCAT 703], V1[AUX -, VFORM BSE];
	{does eat}
	V0[SUBCAT 704], V1[VFORM PSP];
	{has dialed the number}
	V0[SUBCAT 705], V1[VFORM INF, AUX +]
	{ought to dial the number}
END

{Subject auxiliary metarule 2 (SAI 2)}
{No agreement between verb and complement}

BEGIN
   V1[AUX +, INV -]: V[CS, H], X <<, C >>
	  {Verb phrase: was happy}
	  {Only the first, finite aux. verb can invert with the subj}
   V1[AUX +, INV -]: V[H], NOT, X <<, C >>
	  {negated verb phrase - was not happy}
   V2[CONJ NIL, INV +, VFORM FIN, R -]: V[CS, H, AUX +], Z[CS], X <<, C >>
	  {Question - was she happy}
   V2[CONJ NIL, INV +, VFORM FIN, R -, NEG +]: V[CS, H, AUX +], Z[CS], NOT, X <<, C >>
	  {negated question - was she not happy}
WITH
 V,Z,X,C: V0[SUBCAT 701, NFORM there], N2[NFORM there], N2[CONJ NIL, PRD +, POSS -, NOM -], EMPTY;
	  {was a computer}
	  V0[SUBCAT 701, NFORM there], N2[NFORM there], N2[CONJ NIL, PRD +, POSS -, NOM -], V1[VFORM INF];
	  {was a computer to be put in the office}
	  V0[SUBCAT 701, NFORM there], N2[NFORM there], N2[CONJ NIL, PRD +, POSS -, NOM -], V1[VFORM PRP];
	  {was a computer being put in the office}
	  V0[SUBCAT 701, NFORM there], N2[NFORM there], N2[CONJ NIL, PRD +, POSS -, NOM -], P2[PFORM NORM];
	  {was a computer in the office}
	  V0[SUBCAT 701, NFORM NORM], N2[CONJ NIL, NOM +], N2[CONJ NIL, PRD +, POSS -, NOM -], EMPTY;
	  {was a computer}
	  V0[SUBCAT 701, NFORM NORM], N2[CONJ NIL, NOM +], P2[PRD +, PFORM NORM], EMPTY;
	  {was underneath}
	  V0[SUBCAT 701, NFORM NORM], N2[CONJ NIL, NOM +], P2[PRD +, PFORM NORM], V1[VFORM INF];
	  {in the office to be tested}
	  V0[SUBCAT 701, NFORM NORM], N2[CONJ NIL, NOM +], A2[PRD +], EMPTY;
	  {was right}
	  V0[SUBCAT 701, NFORM S], N2[CONJ NIL, NOM +], A2[PRD +, SUBCAT 404], EMPTY
	  {was apparent to us}
END


{Slash rules for these subcategories}
V2[INV +, SLASH N2, R -]: V0[CS, H, AUX +, SUBCAT 701], N2[CS, CONJ NIL, NOM +], N2[NULL]
	{(who) were you [t]?}
V2[INV +, SLASH N2, R -]: V0[CS, H, AUX +, SUBCAT 701], N2[CS, NFORM there], N2[NULL]
	{(what) was there [t]?}
V1[AUX +, SLASH N2]: V0[H, SUBCAT 701], N2[NULL]
	{(a cat) she was [t]}

V2[INV +, AUX +, R -, SLASH P2]: V0[H, SUBCAT 701], N2[CONJ NIL, NFORM NORM], P2[NULL, PFORM NORM]
	{in the office is the telephone}
V2[INV +, AUX +, R -, SLASH A2]: V0[H, SUBCAT 701], N2[CONJ NIL, NFORM NORM], A2[NULL]
	{how are you}
V1[AUX +, SLASH P2]: V0[H, SUBCAT 701], P2[NULL, PFORM NORM]
	{in the office he was}

V1[AUX +, NFORM there, SLASH N2]: V0[H, SUBCAT 701], N2[NULL], V1[VFORM INF]
V1[AUX +, NFORM there, SLASH N2]: V0[H, SUBCAT 701], N2[NULL], P2[PFORM NORM]
V1[AUX +, NFORM there, SLASH N2]: V0[H, SUBCAT 701], N2[NULL], V1[VFORM PSP]
	{an engineer there was to work, working, in the office}

V2[INV +, R -, SLASH N2]: V0[CS, H, SUBCAT 701, AUX +], N2[CS, NFORM there], N2[NULL], V1[VFORM INF]
V2[INV +, R -, SLASH N2]: V0[CS, H, SUBCAT 701, AUX +], N2[CS, NFORM there], N2[NULL], V1[VFORM PRP]
V2[INV +, R -, SLASH N2]: V0[CS, H, SUBCAT 701, AUX +], N2[CS, NFORM there], N2[NULL], P2[PFORM NORM]
	{who was there eating in the office, to eat, in the office}

