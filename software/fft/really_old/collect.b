EXT PROC $( findinput; findfunit; finderrout; endread; endwrite
	    inputwaiting; readbyte; writec; writef; newline
	    readtoken; cvtsn $)

EXT VAR $( output; input; error; exitstatus $)


LET b_start() BE
     $( LET tok,ns = ?,?
	error := myerror
	tok := readtoken()
	IF tok=0 DO error("Usage: collect num")
	ns := cvtsn(tok)
	input := findinput("/dev/ttyb")
	output := findfunit(1)
	WHILE inputwaiting() DO readbyte()
	FOR i=1 TO ns DO
	     $( LET b = readbyte()
		IF (b & #343) NE #340 DO error("Pattern error #%3O",b)
		writec((b & #010) NE 0 -> '1', '0')
		writec('*S')
		IF (i REM 32) = 0 DO newline()
	     $)
	IF (ns REM 32) NE 0 DO newline()
	endread(); endwrite()
     $)

AND myerror(s,p1) BE
     $( output := finderrout()
	writef(s,p1); newline()
	exitstatus := 1
	FINISH
     $)

