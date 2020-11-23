{Simulation rules for agreement between preposed P2 and V2/P2 P2}

{BEGIN
   FCR [SLASH P2[PFORM X]] => [APF X]
   FCR [PFORM X] => [APF X]
WITH
   X: NORM; to; by; of; on; in
END}


{Simulation rules for agreement between a preposed N2 and the SLASH N2, and
the subject, re. RE}

{BEGIN
   FCR [SLASH N2[RE X]] => [AGRE X]
   FCR [RE X] => [AGRE X]
WITH
   X: +; -
END}
