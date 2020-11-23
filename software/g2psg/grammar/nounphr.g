{Rules for noun phrases}

{N2 rules}
N2[PRO -, NFORM NORM, RE -, CONJ NIL, POSS -]: DET[CS], N1[CS, H, PER 3]
	{the efficient computer}

{rules for possessive specifiers}
N2[NFORM NORM, CONJ NIL]: N2[POSS +, NFORM NORM], N1[H, PER 3]
	{John's mother}
N2[PRO -, POSS +, CONJ NIL]: N2[CS, PRO -, POSS -], posaffix[CS, H]
	{John's}

N2[PRO -, NFORM S, PER 3, CONJ NIL]: V2[R -, COMP that, VFORM FIN]
	{Sentential subject rule}
N2[PRO -, NFORM NORM, PLU +, CONJ NIL]: N1[H, PER 3]
	{bare plural common noun}
	{computers are useful}

N2[NFORM NORM]: N2[H, PRO -], P2[PFORM NORM, POST +]
	{noun phrase with prepositional phrase modifier}
N2[NFORM NORM, CONJ NIL]: N2[CS, H, WHMOR R], V2[CS, R +]
	{relative clause modifier}

{N1 rules - mostly rules for N1 modifiers}
N1: N0[H]
	{rule allowing for noun phrases without modifiers}

N1[NFORM NORM]: A2[SUBCAT 406], N1[H]
	{nounphrase with adjective modifier}
{N1[NFORM NORM]: N1[H], P2[PFORM NORM, POST +]}
	{noun phrase with prepositional phrase modifier}
N1[NFORM NORM]: N1[H], P2[PFORM of]
	{noun phrase with possessive prepostional phrase modifier}

{N1[NFORM NORM]: N1[CS, H], V2[CS, R +]}
	{relative clause modifier}

{nounphrase conjunction rules - binary coordination}
N2[NFORM NORM, PLU -, CONJ NIL]: N2[CONJ not, NFORM NORM], N2[CONJ but, NFORM NORM]
N2[NFORM NORM, PLU -, CONJ NIL]: N2[CONJ NIL, NFORM NORM], N2[CONJ or, NFORM NORM]
N2[NFORM NORM, PLU +, CONJ NIL]: N2[CONJ NIL, NFORM NORM], N2[CONJ and, NFORM NORM]
N2[NFORM NORM, PLU +, CONJ NIL]: N2[CONJ both, NFORM NORM], N2[CONJ and, NFORM NORM]
N2[NFORM NORM, CONJ NIL]: N2[CONJ either, NFORM NORM], N2[CONJ or, NFORM NORM]
N2[NFORM NORM, CONJ NIL]: N2[CONJ neither, NFORM NORM], N2[CONJ nor, NFORM NORM]

{ternary coordination}
N2[NFORM NORM, CONJ NIL]: N2[CONJ neither, NFORM NORM], N2[CONJ nor, NFORM NORM], N2[CONJ but, NFORM NORM]
N2[NFORM NORM, CONJ NIL]: N2[CONJ either, NFORM NORM], N2[CONJ or, NFORM NORM], N2[CONJ butnot, NFORM NORM]

{noun phrase conjunction - recursive coordination}
N2[NFORM NORM, PLU +, CONJ and]: N2[CONJ NIL, NFORM NORM], N2[CONJ and, NFORM NORM]
N2[NFORM NORM, PLU -, CONJ or]: N2[CONJ NIL, NFORM NORM], N2[CONJ or, NFORM NORM]

N2[CONJ both]: CONJ[CONJ both], N2[H, CONJ NIL]
N2[CONJ and]: CONJ[CONJ and], N2[H, CONJ NIL]
N2[CONJ either]: CONJ[CONJ either], N2[H, CONJ NIL]
N2[CONJ but]: CONJ[CONJ but], N2[H, CONJ NIL]
N2[CONJ butnot]: CONJ[CONJ but], N2[H, CONJ not]
N2[CONJ not]: CONJ[CONJ not], N2[H, CONJ NIL]
N2[CONJ or]: CONJ[CONJ or], N2[H, CONJ NIL]
N2[CONJ neither]: CONJ[CONJ neither], N2[H, CONJ NIL]
N2[CONJ nor]: CONJ[CONJ nor], N2[H, CONJ NIL]

{Noun phrase gap rule}
N2[CONJ NIL, SLASH N2, NULL]: GAP

