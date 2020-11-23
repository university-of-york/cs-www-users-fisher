{Prepositional phrase rules}

{The [NOM -] specifications, put in to ensure that the N2 inside a
prepositional phrase is not nominative, also ensure that the N2 is
[NFORM NORM], since only normal N2's can have case}

{P2 rules}
{Only [PFORM NORM] P2s can take specifiers - thus P2 complements of verb
phrases, which are [PFORM specified], will not take specifiers}

P2[PFORM NORM]: SPECP, P1[H]
	{just underneath the computer}
P2: P1[H]
	{underneath the computer}

{P1 rules}
P1: P0[H, SUBCAT 49]
	{intransitive preposition}
	{underneath}
P1: P0[H, SUBCAT 38], N2[CONJ NIL, NOM -]
	{transitive preposition}
	{underneath the computer}
P1: P0[H, SUBCAT 50], P2
	{prepositions subcategorising for a prepositional phrase}
	{from underneath the computer}
P1: P0[H, SUBCAT 39], P2[PFORM of]
	{prepositions which take a P2[of] complement}
	{out of the darkness,	 NB *out the darkness}
P1[PFORM of]: P0[H], N2[CONJ NIL, POSS +]
	{part of possessive N2}
	{a book of Connie's}
P1[PFORM of]: P0[H], V1[VFORM PRP]

{Rules for expanding slashed prepositional phrases}
P1[SLASH N2]: P0[H, SUBCAT 38], N2[NULL]
P2[SLASH P2]: P0[H, SUBCAT 50], P2[NULL]

P2[SLASH P2, NULL]: GAP

