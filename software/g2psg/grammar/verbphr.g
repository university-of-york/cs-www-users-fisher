{Verb phrase rules not covered by advmr.g}
{Rules for verb phrase modification}

{Negation rules}
V1[VFORM PSP]: NOT, V1[H]
V1[VFORM BSE]: NOT, V1[H]
	{this must be limited, to not occuring inside V1[INF]?}
	{to not occur inside V1[INF]?}
V1[VFORM PRP]: NOT, V1[H]
V1[VFORM PAS]: NOT, V1[H]
V1[VFORM INF]: NOT, V1[H]

{Rule for adverbial phrase modification of V1's}
V1: V1[H, AUX -], ADV2
	{[AUX -] specification purely heuristic}
V1: ADV2[FREE +], V1[H]


V1[NFORM NORM]: V0[H, SUBCAT 1]
	{eats}

V1[NFORM NORM]: V0[H, SUBCAT 601], P1[SUBCAT 49]

V1[VFORM INF, AUX +]: V0[CS, H, SUBCAT 12], V1[CS, VFORM BSE]
	{to}

V1[NFORM NORM]: V0[H, SUBCAT 20], P2[PFORM of]
	{approves of Rachael}
V1[NFORM NORM, SLASH P2[PFORM of]]: V0[H, SUBCAT 20], P2[NULL]
	{Slashed version}

V1[NFORM NORM]: V0[H, SUBCAT 224], A2[PRD +]
	{she seems happy}
V1[NFORM S]: V0[H, SUBCAT 224], A2[PRD +]
