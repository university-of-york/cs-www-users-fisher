{1. Verbs with V1 complements}

BEGIN
   V1[NFORM M]: V[CS, H], << N, >> VI[CS]
   V1[NFORM M]: V[CS, H], << N, >> ADV2, VI[CS]
   << V1[NFORM M, SLASH N]: V[CS, H], N[NULL], VI[CS] >>
   << V1[NFORM M, SLASH N]: V[CS, H], N[NULL], ADV2, VI[CS] >>
WITH
   M,V,N,VI: NORM, V0[SUBCAT 13], EMPTY, V1[VFORM INF];
	     NORM, V0[SUBCAT 15], EMPTY, V1[VFORM INF];
	     NORM, V0[SUBCAT 16], P2[PFORM to], V1[VFORM INF];
	     there, V0[SUBCAT 16], P2[PFORM to], V1[VFORM INF];
	     NORM, V0[SUBCAT 188], N2[CONJ NIL, NOM -, POSS -], V1[VFORM INF]
END

{2. Verbs with V2 complements}

BEGIN
   V1[NFORM N]: V[H], << P, >> SCOMP
   << V1[NFORM N, SLASH P]: V[H], P[NULL], SCOMP >>
WITH
 N,V,P,SCOMP: NORM, V0[SUBCAT 9], P2[PFORM to], V2[COMP that, VFORM FIN];
	      NORM, V0[SUBCAT 9], EMPTY, V2[COMP that, VFORM FIN, R -];
	      NORM, V0[SUBCAT 10], EMPTY, V2[COMP that, VFORM BSE, R -];
	      NORM, V0[SUBCAT 11], P2[PFORM of], V2[COMP that, VFORM BSE, R -];
	      NORM, V0[SUBCAT 14], EMPTY, V2[VFORM INF, INV -, R -];
	      NORM, V0[SUBCAT 19], EMPTY, V2[CONJ NIL, VFORM FIN, INV -, R -];
	      it, V0[SUBCAT 222], N2[CONJ NIL, NOM -, POSS -], V2[COMP that, VFORM FIN, R -];
	      it, V0[SUBCAT 223], P2[PFORM to], V2[VFORM FIN, INV -, R -];
	      it, V0[SUBCAT 223], EMPTY, V2[VFORM FIN, INV -, R -]
END

