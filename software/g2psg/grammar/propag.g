{ Head Feature Convention }

BEGIN
   FCR [H] & [M x *] => [x *]	    { TRICKLE * }
WITH
   x: ADV; AUX; FREE; GEN; INV; NFORM; NOM; PAST; PER;
      PFORM; PLU; POST; PRD; PRO; REGPAS; REGPAST; SUBCAT; TEMP; VFORM
END

BEGIN
   FCR [H] & [x *] => [M x *]	    { PERCOLATE * }
WITH
   x: FREE; GEN; MORPH; NFORM; PER; PFORM; PLU; POSS; RE; TEMP; VFORM
END

{ Foot Feature Principle }

BEGIN
   FCR [x *] => [M x *]		    { PERCOLATE }
WITH
   x: SLASH; WH; WHMOR
END

{ Control Agreement Principle }

BEGIN
   FCR [CS] & [x *] => [M AGR x *]
WITH
   x: {APF;} GEN; NFORM; PER; PLU
END

{ FOOT [MORPH, RE, SLASH, WH]			???
  CONTROL [APF, GEN, NFORM, PER, PLU]		??? }

