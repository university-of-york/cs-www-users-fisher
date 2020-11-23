EXT PROC $( opensio; finderrout; writes; writef; newline; readtoken
	    readr; writer $)

EXT VAR $( input; output; xformat $)

MANIFEST $( maxdata=4096; pathlimit=200 $)

GET "/usr/fisher/lib/fstreamio.get"


LET b_start() BE
     $( LET data = VEC maxdata-1
	AND ndata = 0
	AND ign = FALSE
	AND xscale,yscale,min,max,val,tok = ?,?,?,?,?,?
	opensio(@input,@output)
	xformat := writer
	val := readr()
	UNTIL ndata GE maxdata \/ input%ftrm=endstreamch DO
	     $( data%ndata := val
		IF ndata=0 DO min,max := val,val
		IF data%ndata #< min DO min := data%ndata
		IF data%ndata #> max DO max := data%ndata
		ndata := ndata+1
		val := readr()
	     $)
	UNLESS input%ftrm=endstreamch DO ign := TRUE
	xscale := FLOAT (10*72) #/ FLOAT (ndata-1)
	yscale := FLOAT (7*72) #/ (max #- min)
	writes("-90 rotate -780 60 translate*N")
	tok := readtoken()
	IF tok NE 0 DO
	     $( writes("/NewCenturySchlbk-Roman findfont *
		       *12 scalefont setfont*N")
		writef("490 450 moveto (%S) show*N",tok)
	     $)
	writes("newpath*N")
	writes("0 0 moveto 720 0 lineto*N")
	FOR i=0 TO ndata-1 DO
	     $( LET x = FLOAT i #* xscale
		LET ix = FIX x
		IF ((i+1) REM pathlimit) = 0 DO writes("stroke newpath*N")
		writef("%N -7 moveto %N 0 lineto*N",ix,ix)
	     $)
	writes("stroke newpath*N")
	FOR i=0 TO ndata-1 DO
	     $( LET x = FLOAT i #* xscale
		AND y = (data%i #- min) #* yscale
		writef("%N %N ", FIX x, FIX y)
		writes(i=0 -> "moveto", "lineto")
		newline()
	     $)
	writes("stroke showpage*N")
	output := finderrout()
	writef("X range 0 %N   y range %X %X*N",ndata-1,min,max)
	IF ign DO writes("Warning: data ignored*N")
     $)
