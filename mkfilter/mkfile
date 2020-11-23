BUILTINS =

cgibin = /york/www/usr/fisher/cgi-bin
libdir = /usr/fisher/linuxlib
objs   = mkfscript.o

$cgibin/mkfscript:	$objs $libdir/libcgi.a
			olinux gcc $objs -L$libdir -lcgi -lm
			mv a.out $cgibin/mkfscript

%.o:			%.C $libdir/libcgi.h
			olinux gcc -O -c -I$libdir $stem.C

clean:
			rm -f $objs

