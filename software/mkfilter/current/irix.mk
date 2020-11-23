BUILTINS =

libdir=/york/$cputype/lib

all:V:		  $MYBIN/mkfilter $MYBIN/mkshape $MYBIN/mkaverage $MYBIN/gencode $MYBIN/genplot

$MYBIN/mkfilter:  mkfilter.o complex.o
		  gcc mkfilter.o complex.o -lm
		  mv a.out $MYBIN/mkfilter
		  chmod a+x $MYBIN/mkfilter

$MYBIN/mkshape:	  mkshape.o complex.o
		  gcc mkshape.o complex.o -lm
		  mv a.out $MYBIN/mkshape
		  chmod a+x $MYBIN/mkshape

$MYBIN/mkaverage: mkaverage.o
		  gcc mkaverage.o -lm
		  mv a.out $MYBIN/mkaverage
		  chmod a+x $MYBIN/mkaverage

$MYBIN/gencode:	  gencode.o complex.o readdata.o
		  gcc gencode.o complex.o readdata.o -lm
		  mv a.out $MYBIN/gencode
		  chmod a+x $MYBIN/gencode

$MYBIN/genplot:	  genplot.o complex.o readdata.o
		  gcc genplot.o complex.o readdata.o -L$libdir -lgd -lm
		  mv a.out $MYBIN/genplot
		  chmod a+x $MYBIN/genplot

%.o:		  %.C mkfilter.h complex.h
		  gcc -I$MYLIB -O -c $stem.C

clean:
		  rm -f mkfilter.o mkshape.o mkaverage.o gencode.o genplot.o complex.o readdata.o

