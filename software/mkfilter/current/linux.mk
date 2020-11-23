BUILTINS =

libdir = /usr/fisher/linuxlib
bindir = /york/www/usr/fisher/helpers

all:V:		    $bindir/mkfilter $bindir/mkshape $bindir/mkaverage $bindir/gencode $bindir/genplot

$bindir/mkfilter:   mkfilter.o complex.o
		    olinux gcc mkfilter.o complex.o -lm
		    mv a.out $bindir/mkfilter

$bindir/mkshape:    mkshape.o complex.o
		    olinux gcc mkshape.o complex.o -lm
		    mv a.out $bindir/mkshape

$bindir/mkaverage:  mkaverage.o complex.o
		    olinux gcc mkaverage.o complex.o -lm
		    mv a.out $bindir/mkaverage

$bindir/gencode:    gencode.o complex.o readdata.o
		    olinux gcc gencode.o complex.o readdata.o -lm
		    mv a.out $bindir/gencode

$bindir/genplot:    genplot.o complex.o readdata.o
		    olinux gcc genplot.o complex.o readdata.o -L$libdir -lgd -lm
		    mv a.out $bindir/genplot

%.o:		    %.C mkfilter.h complex.h
		    olinux gcc -I$libdir -O -c $stem.C

clean:
		    rm -f mkfilter.o mkshape.o mkaverage.o gencode.o genplot.o complex.o readdata.o

