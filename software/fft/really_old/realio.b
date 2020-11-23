ENT PROC $( readr; writer $)

EXT PROC $( readc; writec; writen $)

EXT VAR $( input $)

MANIFEST $( endstreamch $)

GET "/usr/fisher/lib/fstreamio.get"


LET readr() = VALOF
     $( LET x = FLOAT 0
	AND sg = FLOAT 1
	AND dp = 0
	AND haddp = FALSE
	AND ch = ?
	ch := readc() REPEATUNTIL
	  ch='+' \/ ch='-' \/ ('0' LE ch LE '9') \/ ch='.' \/ ch=endstreamch
	IF ch='+' \/ ch='-' DO
	     $( IF ch='-' DO sg := FLOAT -1
		ch := readc()
	     $)
	WHILE ('0' LE ch LE '9') \/ (ch='.' & NOT haddp) DO
	     $( TEST ch='.' THEN haddp := TRUE
		OR   $( x := (x #* FLOAT 10) #+ FLOAT (ch-'0')
			IF haddp DO dp := dp+1
		     $)
		ch := readc()
	     $)
	IF ch='e' DO
	     $( LET exp,esg = 0,1
		ch := readc()
		IF ch='+' \/ ch='-' DO
		     $( IF ch='-' DO esg := -1
			ch := readc()
		     $)
		WHILE ('0' LE ch LE '9') DO
		     $( exp := exp*10 + (ch-'0')
			ch := readc()
		     $)
		dp := dp - esg*exp
	     $)
	FOR i=1 TO dp DO x := x #/ FLOAT 10
	FOR i=1 TO -dp DO x := x #* FLOAT 10
	input%ftrm := ch
	RESULTIS sg #* x
     $)

AND writer(x) BE
     $( LET exp = 0
	IF x #< FLOAT 0 DO
	     $( writec('-')
		x := #- x
	     $)
	IF x #> FLOAT 0 DO
	     $( WHILE x #< FLOAT 1 DO
		     $( x := x #* FLOAT 10
			exp := exp-1
		     $)
		WHILE x #GE FLOAT 10 DO
		     $( x := x #/ FLOAT 10
			exp := exp+1
		     $)
	     $)
	FOR i=1 TO 6 DO
	     $( LET n = FIX x
		writec('0'+n)
		IF i=1 DO writec('.')
		x := (x #- FLOAT n) #* FLOAT 10
	     $)
	writec('e'); writen(exp)
     $)
