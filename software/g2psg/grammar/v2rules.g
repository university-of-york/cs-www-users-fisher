Utterance: U[TOP]

U: EXPRESSION
	{phatic utterance rule - "hello!"}
U: EXPRESSION, N2[CONJ NIL, RE -, PER 3]
	{phatic utterance + vocative - "hello Dave!"}
U: EXPRESSION, N2[CONJ NIL, RE -, PER 3, PLU -], V2[COMP NIL, VFORM FIN, R -, INV +, PLU -]
U: EXPRESSION, N2[CONJ NIL, RE -, PER 3, PLU +], V2[COMP NIL, VFORM FIN, R -, INV +, PLU +]
	{phatic utterance + vocative + opening question }
	{"hello Dave, how are you"}
U: V2[VFORM FIN, R -]
	{declarative and interrogative sentences}
U: EXPRESSION, V2[VFORM FIN, R -]
	{declarative and interrogative sentences}
U: V2[COMP NIL, IMP +, R -]
	{imperative sentences}

{rules for conjoined sentences}
U: V2[CONJ NIL, INV -, R -], V2[CONJ and, R -]
U: V2[CONJ NIL, INV +, R -], V2[CONJ and, INV +, R -]
U: V2[CONJ NIL, INV -, R -], V2[CONJ or, R -]
U: V2[CONJ NIL, INV +, R -], V2[CONJ or, INV +, R -]
U: V2[CONJ NIL, INV -, R -], V2[CONJ but, R -]
U: V2[CONJ NIL, INV +, R -], V2[CONJ because, INV -, R -]
U: V2[CONJ NIL, INV -, R -], V2[CONJ because, INV -, R -]
U: V2[CONJ NIL, INV -, R -], V2[CONJ although, INV -, R -]
U: V2[CONJ although, INV -, R -], V2[CONJ NIL, R -]
U: V2[CONJ NIL, R -], V2[CONJ if, INV -, R -]
U: V2[CONJ if, INV -, R -], V2[CONJ NIL, R -]
U: V2[CONJ if, INV -, R -], V2[CONJ then, R -]
U: V2[CONJ when, INV -, R -], V2[CONJ NIL, VFORM FIN, INV -, R -]
U: V2[CONJ while, INV -, R -], V2[CONJ NIL, VFORM FIN, INV -, R -]
U: V2[CONJ before, INV -, R -], V2[CONJ NIL, VFORM FIN, INV -, R -]
U: V2[CONJ NIL, INV -, R -], V2[CONJ until, VFORM FIN, INV -, R -]

{rules for expanding conjoined sentences}

V2[CONJ when, R -]: CONJ[CONJ when], V2[CONJ NIL, VFORM FIN, R -]
V2[CONJ while, R -]: CONJ[CONJ while], V2[CONJ NIL, VFORM FIN, R -]
V2[CONJ before, R -]: CONJ[CONJ before], V2[CONJ NIL, VFORM FIN, R -]
V2[CONJ until, R -]: CONJ[CONJ until], V2[CONJ NIL, VFORM FIN, R -]
V2[CONJ so, R -]: CONJ[CONJ so], V2[CONJ NIL, VFORM FIN, R -]
V2[CONJ and, VFORM FIN, R -]: CONJ[CONJ and], V2[CONJ NIL, VFORM FIN, R -]
V2[CONJ and, VFORM BSE, R -]: CONJ[CONJ and], V2[CONJ NIL, VFORM BSE, R -]
V2[CONJ or, R -]: CONJ[CONJ or], V2[CONJ NIL, R -]
V2[CONJ but, R -]: CONJ[CONJ but], V2[CONJ NIL, R -]
V2[CONJ because, R -]: CONJ[CONJ because], V2[CONJ NIL, R -]
V2[CONJ although, R -]: CONJ[CONJ although], V2[CONJ NIL, R -]
V2[CONJ if, R -]: CONJ[CONJ if], V2[CONJ NIL, R -]
V2[CONJ then, R -]: CONJ[CONJ then], V2[CONJ NIL, R -]


{Expansions of V2}

{V2: X2 V2}

V2[CONJ NIL, VFORM FIN, INV -, R -]: ADV2[POST -, TOP], V2[H, COMP NIL, R -]
	{matrix sentence rule with adverbial modifier}

V2[CONJ NIL, R -, INV -, VFORM FIN]:
   N2[CONJ NIL, TOP, NOM -, RE -], V2[H, COMP NIL, SLASH N2[RE -, NOM -], R -]
	{non-reflexive topicalised N2 - the woman, he dialed}
V2[CONJ NIL, R -, INV -, VFORM FIN]:
   N2[CS, CONJ NIL, TOP, NOM -, RE +], V2[CS, H, COMP NIL, SLASH N2[RE +], R -]
	{reflexive topicalised N2 - herself she dialed}

V2[CONJ NIL, R -, INV +, VFORM FIN]:
   N2[CONJ NIL, TOP, WH +, WHMOR Q, NOM -, RE -], V2[H, SLASH N2[RE -], R -]
	{SLASHed question rule - who(m) did he dial}


V2[CONJ NIL, R -, INV -, VFORM FIN]: P2[CS, TOP], V2[CS, H, SLASH P2, R -]
	{topicalised P2}
	{in the office Paul put the computer}
V2[CONJ NIL, R -, INV +, VFORM FIN]: P2[CS, WH +, WHMOR Q], V2[CS, H, SLASH P2, R -]
	{SLASHed P2 question rule}
	{in what did Paul put the computer}
V2[CONJ NIL, R -, INV +]: ADV2[WH +], V2[H, R -]
	{Adverb question - for whom did Paul buy a book}
V2[CONJ NIL, R -, INV +, VFORM FIN]: A2[CS, WH +, WHMOR Q], V2[CS, H, SLASH A2, R -]
	{SLASHed A2 question rule - how is she?}


{V2: N2 V1}

	{matrix sentence rule - always finite vform}
V2[COMP NIL, VFORM FIN, INV -, R -, IMP -]: N2[CS, CONJ NIL, TOP, NOM +], V1[CS, H]
V2[COMP NIL, VFORM FIN, INV -, R -]: N2[CS, NFORM it], V1[CS, H]
V2[COMP NIL, VFORM FIN, INV -, R -]: N2[CS, NFORM there], V1[CS, H]
V2[COMP NIL, VFORM FIN, INV -, R -]: N2[CS, TOP, NFORM S], V1[CS, H]

V2[COMP NIL, VFORM INF, INV -, R -]: N2[CS, CONJ NIL, TOP, NOM -], V1[CS, H]
V2[COMP NIL, VFORM BSE, INV -, R -, IMP -]: N2[CS, CONJ NIL, TOP, NOM +], V1[CS, H]

V2[COMP NIL, IMP +]: V1[H]
	{have a happy new year}
V2[COMP NIL, IMP +]: PLEASE, V1[H]
	{Imperative with please}

{Subordinate clauses}

V2[R -, COMP that, VFORM FIN]: COMP[COMP that], V2[H, COMP NIL, INV -, R -]
V2[R -, COMP that, VFORM BSE]: COMP[COMP that], V2[H, COMP NIL, INV -, R -, IMP -]
V2[COMP whether, VFORM FIN]: COMP[COMP whether], V2[H, COMP NIL, INV -, R -]
V2[COMP if, VFORM FIN]: COMP[COMP if], V2[H, COMP NIL, INV -, R -]

{Relative clauses}

{V2[R +]: N2[CONJ NIL, WH +, WHMOR R, NOM +], V1[H, VFORM FIN]}
V2[R +]: N2[CONJ NIL, WH +, WHMOR R, NOM +], V2[COMP NIL, R -, VFORM FIN, SLASH N2[RE -]]
	{subject relative clause - (the woman) who dialed}
V2[R +]: N2[CONJ NIL, WH +, WHMOR R, NOM -], V2[VFORM FIN, SLASH N2[RE -, NOM -], R -]
	{direct object relative clause - (the woman) whom he dialed [e]}
{V2[R +]: V1[H, VFORM PAS]}
	{'whiz' deletion relatives - (a man) called Dave}
	{restrict to ditransitive or modified phrases}
V2[R -, VFORM FIN, SLASH N2[NOM +]]: N2[NULL], V1[H, TOP]
	{(the woman that) [e] dialed}

V2[R +]: COMP[COMP that], V2[COMP NIL, R -, VFORM FIN, SLASH N2[RE -]]
	{relative clause - (the woman) that he analyseed [e]}
	{(the woman) that [e] analyseed him}
V2[R +]: V2[R -, COMP NIL, VFORM FIN, SLASH N2[RE -, NOM -]]
	{(the woman) he analyseed [e]}
V2[R +]: P2[WH +, WHMOR R], V2[VFORM FIN, SLASH P2[RE -], R -]
	{oblique object relative clause - (the woman) of whom we approve [e]}

