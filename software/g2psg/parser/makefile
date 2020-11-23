# GPSG parsing system	A.J. Fisher   July 1986
# C version   AJF   August 1988
# makefile

bin=/usr/fisher/bin
lib=/usr/fisher/lib

all:			$(bin)/buildpar $(bin)/metagee $(bin)/g2psg $(lib)/g2psg.emf $(lib)/parser.o

$(bin)/buildpar:	buildpar
			cp buildpar $(bin)/buildpar
			chmod a+rx $(bin)/buildpar

$(bin)/metagee:		metagee.c
			cc -O metagee.c
			mv a.out $(bin)/metagee
			chmod a+x $(bin)/metagee

$(bin)/g2psg:		base.o readgr.o starters.o maketab.o prerror.o common.o
			cc base.o readgr.o starters.o maketab.o prerror.o common.o
			mv a.out $(bin)/g2psg
			chmod a+x $(bin)/g2psg

base.o:			g2psg.h base.c
			cc -c -O base.c

readgr.o:		g2psg.h readgr.c
			cc -c -O readgr.c

starters.o:		g2psg.h starters.c
			cc -c -O starters.c

maketab.o:		g2psg.h maketab.c
			cc -c -O maketab.c

prerror.o:		g2psg.h prerror.c
			cc -c -O prerror.c

common.o:		g2psg.h common.c
			cc -c -O common.c

$(lib)/g2psg.emf:	emf.text trans
			trans
			chmod a+r $(lib)/g2psg.emf

trans:			trans.c
			cc -O trans.c
			mv a.out trans

$(lib)/parser.o:	parser.c
			cc -c -O parser.c
			mv parser.o $(lib)/parser.o
			chmod a+r $(lib)/parser.o

clean:;			rm -f $(bin)/buildpar $(bin)/metagee $(bin)/g2psg $(lib)/g2psg.emf $(lib)/parser.o trans *.o

