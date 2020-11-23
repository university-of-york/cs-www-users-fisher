EXT PROC $( findfunit; writes; newline
	    readtoken; cvtsn $)

EXT VAR $( output; error $)


LET b_start() BE
     $( LET ndata,p,q = ?,?,?
	ndata := rtoknum()
	p := rtoknum()
	q := rtoknum()
	output := findfunit(1)
	FOR i=0 TO q-1 DO
	     $( LET w = walsh(p,q,i)
		FOR j=0 TO (ndata/q)-1 DO writes(w -> " +1", " -1")
	     $)
	newline()
     $)

AND rtoknum() = VALOF
     $( LET tok = readtoken()
	IF tok=0 DO error("Usage: wal ndata p q")
	RESULTIS cvtsn(tok)
     $)

AND walsh(p,q,i) = p=0 -> TRUE, VALOF
     $( LET x = p
	UNTIL ((x+1) & x) = 0 DO x := x+1
	RESULTIS walsh(p NEQV x,q,i) EQV rad(x,q,i)
     $)

AND rad(x,q,i) =
	x=1 -> i GE q/2,
	(q/(x+1) & i) = 0
