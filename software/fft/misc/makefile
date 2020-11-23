.SUFFIXES:

all:		ave ifreq psxyz sun3d sunanim lpf abs

ave:		ave.c
		cc -O ave.c -lm
		mv a.out ave

ifreq:		ifreq.c
		cc -O ifreq.c -lm
		mv a.out ifreq

psxyz:		psxyz.c
		cc -O psxyz.c -lm
		mv a.out psxyz

sun3d:		sun3d.c
		cc -O sun3d.c -lm -lSun -lpixrect
		mv a.out sun3d

sunanim:	sunanim.c
		cc -O sunanim.c -lm -lSun -lpixrect
		mv a.out sunanim

lpf:		lpf.c
		cc -O lpf.c
		mv a.out lpf

abs:		abs.c
		cc -O abs.c
		mv a.out abs

