// Fourier transform program   AJF   August 1986

EXT PROC $( readtoken; opensio; writes; newline
	    seq; cvtsn; heap
	    readr; writer
	    atan; sin; cos; sqrt $)

EXT VAR $( error; input; output $)

STATIC $( ndata; type; circle $)

MANIFEST $( re=0; im=1
	    maxdata=4096 $)

MANIFEST $( t.real; t.imag; t.compl; t.power $)


LET b_start() BE
     $( LET data = VEC maxdata-1
	opensio(@input,@output)
	readcmdline()
	initcircle()
	readdata(data)
	fft(data,ndata)
	writedata(data)
     $)

AND readcmdline() BE
     $( LET tok = ?
	ndata := 0; type := t.compl
	tok := readtoken()
	UNTIL tok=0 DO
	     $( TEST tok seq "-r" THEN type := t.real
		OR TEST tok seq "-i" THEN type := t.imag
		OR TEST tok seq "-z" THEN type := t.compl
		OR TEST tok seq "-p" THEN type := t.power
		OR   $( ndata := cvtsn(tok)
			UNLESS 1 LE ndata LE maxdata
			  DO error("ndata out of range")
		     $)
		tok := readtoken()
	     $)
	IF ndata=0 DO error("Usage: fft [-rizp] ndata")
     $)

AND initcircle() BE
     $( LET twopi = atan(FLOAT 1) #* FLOAT 8
	circle := heap(ndata-1)
	FOR i=0 TO ndata-1 DO
	     $( LET x = twopi #* FLOAT i #/ FLOAT ndata
		LET z = complex()
		re%z := cos(x); im%z := #- sin(x)
		circle%i := z
	     $)
     $)

AND readdata(data) BE
     $( FOR i=0 TO ndata-1 DO
	     $( LET z = complex()
		re%z := readr()
		im%z := FLOAT 0
		data%i := z
	     $)
     $)

AND fft(data,n) BE IF n>1 DO
     $( LET h = n/2
	LET even = VEC maxdata-1
	AND odd = VEC maxdata-1
	FOR i=0 TO h-1 DO
	     $( even%i := data%(2*i)
		odd%i := data%(2*i+1)
	     $)
	fft(even,h); fft(odd,h)
	FOR i=0 TO h-1 DO
	     $( LET t = ndata/n
		data%i := even%i cplus (circle%(i*t) ctimes odd%i)
		data%(h+i) := even%i cplus (circle%((h+i)*t) ctimes odd%i)
	     $)
     $)

AND writedata(data) BE
     $( LET h = ndata/2
	IF type=t.real \/ type=t.compl DO
	  FOR i=0 TO h-1 DO
	     $( LET z = data%i
		writer(re%z); newline()
	     $)
	IF type=t.imag \/ type=t.compl DO
	  FOR i=0 TO h-1 DO
	     $( LET z = data%i
		writer(im%z); newline()
	     $)
	IF type=t.power DO
	  FOR i=0 TO h-1 DO
	     $( LET z = data%i
		writer(sqrt(re%z #* re%z #+ im%z #* im%z)); newline()
	     $)
     $)

AND cplus(z1,z2) = VALOF
     $( LET z = complex()
	re%z := re%z1 #+ re%z2
	im%z := im%z1 #+ im%z2
	RESULTIS z
     $)

AND ctimes(z1,z2) = VALOF
     $( LET z = complex()
	re%z := (re%z1 #* re%z2) #- (im%z1 #* im%z2)
	im%z := (im%z1 #* re%z2) #+ (re%z1 #* im%z2)
	RESULTIS z
     $)

AND complex() = heap(im)

