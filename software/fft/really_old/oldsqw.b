EXT PROC $( opentty; findfunit; writef; writes; newline; readn
	    readtoken; cvtsn $)

EXT VAR $( input; output; error $)

MANIFEST $( endstreamch=-1 $)


LET b_start() BE
     $( LET mf,df,hp,k,pn,pt = ?,?,?,?,?,?
	AND tok,ndata = ?,?
	tok := readtoken()
	IF tok=0 DO error("Usage: sqw ndata")
	ndata := cvtsn(tok)
	opentty(@input,@output)
	writef("Num. data = %N*N",ndata)
	writes("Master freq:  "); mf := readn()
	writes("Desired freq: "); df := readn()
	output := findfunit(1)
	hp := (FLOAT mf #/ FLOAT df) #/ FLOAT 2 // half period
	k,pn := 0,0
	WHILE k<ndata DO
	     $( pn := pn+1           // half-period number
		pt := hp #* FLOAT pn // predicted transition time
		WHILE FLOAT k #< pt & k<ndata DO
		     $( writes((pn REM 2) = 0 -> " -1", " +1")
			k := k+1
		     $)
	     $)
	newline()
     $)
