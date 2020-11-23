	.verstamp	3 18
	.option	pic2
	.rdata	
	.align	2
	.align	0
$$9:
	.ascii	"lat %2d lon %+2d   \X00"
	.rdata	
	.align	2
	.align	0
$$10:
	.ascii	"my x,y %12.4f %12.4f   pj x,y %12.4f %12.4f   \X00"
	.rdata	
	.align	2
	.align	0
$$12:
	.ascii	"dx %8.4f   dy %8.4f   \X00"
	.rdata	
	.align	2
	.align	0
$$17:
	.ascii	"dx %.4f .. %.4f   dy %.4f .. %.4f   \X00"
	.rdata	
	.align	2
	.align	0
$$18:
	.ascii	"dh %.4f\X0A\X00"
	.rdata	
	.align	3
	.align	0
etab1:
	.double	-1.0681152343750000e-02:1
	.double	-1.9531250000000000e-02:1
	.double	-4.6875000000000000e-02:1
	.double	-2.5000000000000000e-01:1
	.double	1.:1
	.rdata	
	.align	3
	.align	0
etab2:
	.double	-1.0681152343750000e-02:1
	.double	-1.9531250000000000e-02:1
	.double	-4.6875000000000000e-02:1
	.double	.75:1
	.rdata	
	.align	3
	.align	0
etab3:
	.double	-7.1207682291666670e-03:1
	.double	-1.3020833333333332e-02:1
	.double	.46875:1
	.rdata	
	.align	3
	.align	0
etab4:
	.double	-5.6966145833333330e-03:1
	.double	.36458333333333333333:1
	.rdata	
	.align	3
	.align	0
etab5:
	.double	.3076171875:1
	.rdata	
	.align	3
	.align	0
xtab1:
	.double	-1.0000000000000000e+00:1
	.double	1.7900000000000000e+02:1
	.double	4.7900000000000000e+02:1
	.double	6.1000000000000000e+01:1
	.rdata	
	.align	3
	.align	0
xtab2:
	.double	1.0000000000000000e+00:1
	.double	-1.8000000000000000e+01:1
	.double	5.0000000000000000e+00:1
	.rdata	
	.align	3
	.align	0
xtab3:
	.double	-5.8000000000000000e+01:1
	.double	1.4000000000000000e+01:1
	.rdata	
	.align	3
	.align	0
ytab1:
	.double	-1.0000000000000000e+00:1
	.double	5.4300000000000000e+02:1
	.double	-3.1110000000000000e+03:1
	.double	1.3850000000000000e+03:1
	.rdata	
	.align	3
	.align	0
ytab2:
	.double	1.0000000000000000e+00:1
	.double	-5.8000000000000000e+01:1
	.double	6.1000000000000000e+01:1
	.rdata	
	.align	3
	.align	0
ytab3:
	.double	-3.3000000000000000e+02:1
	.double	2.7000000000000000e+02:1
	.rdata	
	.align	2
	.align	0
$$44:
	.ascii	"echo '%.4f %.4f' | proj +proj=tmerc +ellps=airy +lat_0=%.0f +lon_0=%.0f +k_0=%.9f +x_0=%.0f +y_0=%.0f\X00"
	.rdata	
	.align	2
	.align	0
$$46:
	.ascii	"r\X00"
	.rdata	
	.align	2
	.align	0
$$48:
	.ascii	"popen failed\X00"
	.rdata	
	.align	2
	.align	0
$$50:
	.ascii	"%lg %lg\X0A\X00"
	.rdata	
	.align	2
	.align	0
$$52:
	.ascii	"fscanf error, ni = %d\X00"
	.extern	__us_rsthread_stdio 4
	.extern	__iob 1600
	.lcomm	enpars 40
	.text	
	.align	2
	.file	2 "testpolos.c"
	.globl	main
	.loc	2 43
 #  43	  { int ilat, ilon;
	.ent	main 2
main:
	.option	O1
	.set	 noreorder
	.cpload	$25
	.set	 reorder
	subu	$sp, 176
	sw	$31, 52($sp)
	.cprestore	48
	sw	$4, 176($sp)
	sw	$5, 180($sp)
	.mask	0x90000000, -124
	.frame	$sp, 176, $31
	.loc	2 43
	.loc	2 44
 #  44	    double mindx = +1e10, maxdx = -1e10;
	li.d	$f4, 1e10
	s.d	$f4, 160($sp)
	.loc	2 44
	li.d	$f6, -1.0000000000000000e+10
	s.d	$f6, 152($sp)
	.loc	2 45
 #  45	    double mindy = +1e10, maxdy = -1e10;
	li.d	$f8, 1e10
	s.d	$f8, 144($sp)
	.loc	2 45
	li.d	$f10, -1.0000000000000000e+10
	s.d	$f10, 136($sp)
	.loc	2 46
 #  46	    double maxdh = -1e10;
	li.d	$f16, -1.0000000000000000e+10
	s.d	$f16, 128($sp)
	.loc	2 47
 #  47	    makeenpars();
	.livereg	0x0000000E,0x00000000
	jal	makeenpars
	.loc	2 49
 #  48	    /* for (ilat = 49; ilat <= 49; ilat++) */
 #  49	    for (ilat = 49; ilat <= 61; ilat++)
	li	$14, 49
	sw	$14, 172($sp)
$32:
	.loc	2 50
 #  50	      { for (ilon = -6; ilon <= +2; ilon++)
	.loc	2 50
	li	$15, -6
	sw	$15, 168($sp)
$33:
	.loc	2 52
 #  51		/* for (ilon = -2; ilon <= -2; ilon++) */
 #  52		  { struct pco pco; struct cco mycco, pjcco; double dx, dy, dh;
	.loc	2 53
 #  53		    pco.lon = (double) ilon;
	lw	$24, 168($sp)
	mtc1	$24, $f18
	cvt.d.w	$f4, $f18
	s.d	$f4, 112($sp)
	.loc	2 54
 #  54		    pco.lat = (double) ilat;
	lw	$25, 172($sp)
	mtc1	$25, $f6
	cvt.d.w	$f8, $f6
	s.d	$f8, 120($sp)
	.loc	2 55
 #  55		    my_convert(&pco, &mycco);
	addu	$4, $sp, 112
	addu	$5, $sp, 96
	.livereg	0x0C00000E,0x00000000
	jal	my_convert
	.loc	2 56
 #  56		    pj_convert(&pco, &pjcco);
	addu	$4, $sp, 112
	addu	$5, $sp, 80
	.livereg	0x0C00000E,0x00000000
	jal	pj_convert
	.loc	2 57
 #  57		    printf("lat %2d lon %+2d   ", ilat, ilon);
	la	$4, $$9
	lw	$5, 172($sp)
	lw	$6, 168($sp)
	.livereg	0x0E00000E,0x00000000
	jal	printf
	.loc	2 58
 #  58		    printf("my x,y %12.4f %12.4f   pj x,y %12.4f %12.4f   ", mycco.x, mycco.y, pjcco.x, pjcco.y);
	la	$4, $$10
	lw	$6, 96($sp)
	lw	$7, 100($sp)
	l.d	$f10, 104($sp)
	s.d	$f10, 16($sp)
	l.d	$f16, 80($sp)
	s.d	$f16, 24($sp)
	l.d	$f18, 88($sp)
	s.d	$f18, 32($sp)
	.livereg	0x0B00000E,0x00000000
	jal	printf
	.loc	2 59
 #  59		    dx = mycco.x - pjcco.x; dy = mycco.y - pjcco.y; dh = hypot(dx, dy);
	l.d	$f4, 96($sp)
	l.d	$f6, 80($sp)
	sub.d	$f8, $f4, $f6
	s.d	$f8, 72($sp)
	.loc	2 59
	l.d	$f10, 104($sp)
	l.d	$f16, 88($sp)
	sub.d	$f18, $f10, $f16
	s.d	$f18, 64($sp)
	.loc	2 59
	mov.d	$f12, $f8
	mov.d	$f14, $f18
	.livereg	0x0000000E,0x000F0000
	jal	hypot
	s.d	$f0, 56($sp)
	.loc	2 60
 #  60		    printf("dx %8.4f   dy %8.4f   ", dx, dy);
	la	$4, $$12
	lw	$6, 72($sp)
	lw	$7, 76($sp)
	l.d	$f4, 64($sp)
	s.d	$f4, 16($sp)
	.livereg	0x0B00000E,0x00000000
	jal	printf
	.loc	2 61
 #  61		    putchar('\n');
	lw	$8, __us_rsthread_stdio
	beq	$8, 0, $34
	li	$4, 10
	la	$5, __iob
	addu	$5, $5, 16
	.livereg	0x0C00000E,0x00000000
	jal	__semputc
	b	$36
$34:
	la	$9, __iob
	lw	$10, 16($9)
	addu	$11, $10, -1
	sw	$11, 16($9)
	la	$12, __iob
	lw	$13, 16($12)
	bge	$13, 0, $35
	li	$4, 10
	addu	$5, $12, 16
	.livereg	0x0C00000E,0x00000000
	jal	__flsbuf
	b	$36
$35:
	li	$14, 10
	la	$15, __iob
	lw	$24, 20($15)
	sb	$14, 0($24)
	la	$25, __iob
	lw	$8, 20($25)
	addu	$10, $8, 1
	sw	$10, 20($25)
$36:
	.loc	2 62
 #  62		    if (dx < mindx) mindx = dx;
	l.d	$f6, 72($sp)
	l.d	$f10, 160($sp)
	c.lt.d	$f6, $f10
	bc1f	$37
	.loc	2 62
	s.d	$f6, 160($sp)
$37:
	.loc	2 63
 #  63		    if (dy < mindy) mindy = dy;
	l.d	$f16, 64($sp)
	l.d	$f8, 144($sp)
	c.lt.d	$f16, $f8
	bc1f	$38
	.loc	2 63
	s.d	$f16, 144($sp)
$38:
	.loc	2 64
 #  64		    if (dx > maxdx) maxdx = dx;
	l.d	$f18, 72($sp)
	l.d	$f4, 152($sp)
	c.lt.d	$f4, $f18
	bc1f	$39
	.loc	2 64
	s.d	$f18, 152($sp)
$39:
	.loc	2 65
 #  65		    if (dy > maxdy) maxdy = dy;
	l.d	$f10, 64($sp)
	l.d	$f6, 136($sp)
	c.lt.d	$f6, $f10
	bc1f	$40
	.loc	2 65
	s.d	$f10, 136($sp)
$40:
	.loc	2 66
 #  66		    if (dh > maxdh) maxdh = dh;
	l.d	$f8, 56($sp)
	l.d	$f16, 128($sp)
	c.lt.d	$f16, $f8
	bc1f	$41
	.loc	2 66
	s.d	$f8, 128($sp)
	.loc	2 50
 #  50	      { for (ilon = -6; ilon <= +2; ilon++)
$41:
	lw	$11, 168($sp)
	addu	$9, $11, 1
	sw	$9, 168($sp)
	ble	$9, 2, $33
	.loc	2 68
 #  68		putchar('\n');
	lw	$13, __us_rsthread_stdio
	beq	$13, 0, $42
	li	$4, 10
	la	$5, __iob
	addu	$5, $5, 16
	.livereg	0x0C00000E,0x00000000
	jal	__semputc
	b	$44
$42:
	la	$12, __iob
	lw	$15, 16($12)
	addu	$14, $15, -1
	sw	$14, 16($12)
	la	$24, __iob
	lw	$8, 16($24)
	bge	$8, 0, $43
	li	$4, 10
	addu	$5, $24, 16
	.livereg	0x0C00000E,0x00000000
	jal	__flsbuf
	b	$44
$43:
	li	$10, 10
	la	$25, __iob
	lw	$11, 20($25)
	sb	$10, 0($11)
	la	$9, __iob
	lw	$13, 20($9)
	addu	$15, $13, 1
	sw	$15, 20($9)
	.loc	2 49
 #  49	    for (ilat = 49; ilat <= 61; ilat++)
$44:
	lw	$14, 172($sp)
	addu	$12, $14, 1
	sw	$12, 172($sp)
	ble	$12, 61, $32
	.loc	2 70
 #  70	    printf("dx %.4f .. %.4f   dy %.4f .. %.4f   ", mindx, maxdx, mindy, maxdy);
	la	$4, $$17
	lw	$6, 160($sp)
	lw	$7, 164($sp)
	l.d	$f4, 152($sp)
	s.d	$f4, 16($sp)
	l.d	$f18, 144($sp)
	s.d	$f18, 24($sp)
	l.d	$f6, 136($sp)
	s.d	$f6, 32($sp)
	.livereg	0x0B00000E,0x00000000
	jal	printf
	.loc	2 71
 #  71	    printf("dh %.4f\n", maxdh);
	la	$4, $$18
	lw	$6, 128($sp)
	lw	$7, 132($sp)
	.livereg	0x0B00000E,0x00000000
	jal	printf
	.loc	2 72
 #  72	    exit(0);
	move	$4, $0
	.livereg	0x0800000E,0x00000000
	jal	exit
	.loc	2 73
 #  73	  }
	move	$2, $0
	.livereg	0x2000FF0E,0x00000FFF
	lw	$31, 52($sp)
	addu	$sp, 176
	j	$31
	.end	main
	.text	
	.align	2
	.file	2 "testpolos.c"
	.loc	2 101
 # 101	  { enpars[4] = POLY4(ECC2, etab1);
	.ent	makeenpars 2
makeenpars:
	.option	O1
	.set	 noreorder
	.cpload	$25
	.set	 reorder
	.frame	$sp, 0, $31
	.loc	2 101
	.loc	2 101
	la	$14, etab1
	l.d	$f4, 0($14)
	li.d	$f6, 6.670540000123428e-3
	mul.d	$f8, $f4, $f6
	l.d	$f10, 8($14)
	add.d	$f16, $f8, $f10
	li.d	$f18, 6.670540000123428e-3
	mul.d	$f4, $f16, $f18
	l.d	$f6, 16($14)
	add.d	$f8, $f4, $f6
	li.d	$f10, 6.670540000123428e-3
	mul.d	$f16, $f8, $f10
	l.d	$f18, 24($14)
	add.d	$f4, $f16, $f18
	li.d	$f6, 6.670540000123428e-3
	mul.d	$f8, $f4, $f6
	l.d	$f10, 32($14)
	add.d	$f16, $f10, $f8
	la	$15, enpars
	s.d	$f16, 32($15)
	.loc	2 102
 # 102	    enpars[3] = ECC2 * POLY3(ECC2, etab2);
	la	$24, etab2
	l.d	$f18, 0($24)
	li.d	$f4, 6.670540000123428e-3
	mul.d	$f6, $f18, $f4
	l.d	$f10, 8($24)
	add.d	$f8, $f6, $f10
	li.d	$f16, 6.670540000123428e-3
	mul.d	$f18, $f8, $f16
	l.d	$f4, 16($24)
	add.d	$f6, $f18, $f4
	li.d	$f10, 6.670540000123428e-3
	mul.d	$f8, $f6, $f10
	l.d	$f16, 24($24)
	add.d	$f18, $f16, $f8
	li.d	$f4, 6.670540000123428e-3
	mul.d	$f6, $f18, $f4
	la	$25, enpars
	s.d	$f6, 24($25)
	.loc	2 103
 # 103	    enpars[2] = ECC4 * POLY2(ECC2, etab3);
	la	$8, etab3
	l.d	$f10, 0($8)
	li.d	$f16, 6.670540000123428e-3
	mul.d	$f8, $f10, $f16
	l.d	$f18, 8($8)
	add.d	$f4, $f8, $f18
	li.d	$f6, 6.670540000123428e-3
	mul.d	$f10, $f4, $f6
	l.d	$f16, 16($8)
	add.d	$f8, $f16, $f10
	li.d	$f18, 4.4496103893246662e-05
	mul.d	$f4, $f8, $f18
	la	$9, enpars
	s.d	$f4, 16($9)
	.loc	2 104
 # 104	    enpars[1] = ECC6 * POLY1(ECC2, etab4);
	la	$10, etab4
	l.d	$f6, 0($10)
	li.d	$f16, 6.670540000123428e-3
	mul.d	$f10, $f6, $f16
	l.d	$f8, 8($10)
	add.d	$f18, $f8, $f10
	li.d	$f4, 2.9681304086954966e-07
	mul.d	$f6, $f18, $f4
	la	$11, enpars
	s.d	$f6, 8($11)
	.loc	2 105
 # 105	    enpars[0] = ECC8 * POLY0(ECC2, etab5);
	la	$12, etab5
	l.d	$f16, 0($12)
	li.d	$f8, 1.9799032616786007e-09
	mul.d	$f10, $f16, $f8
	la	$13, enpars
	s.d	$f10, 0($13)
	.loc	2 106
 # 106	  }
	.livereg	0x2000FF0E,0x00000FFF
	j	$31
	.end	makeenpars
	.text	
	.align	2
	.file	2 "testpolos.c"
	.loc	2 117
 # 117	  { /* given lon, lat, compute OS coords x, y */
	.ent	my_convert 2
my_convert:
	.option	O1
	.set	 noreorder
	.cpload	$25
	.set	 reorder
	subu	$sp, 200
	sw	$31, 44($sp)
	.cprestore	40
	sw	$4, 200($sp)
	sw	$5, 204($sp)
	s.d	$f22, 32($sp)
	s.d	$f20, 24($sp)
	.mask	0x90000000, -156
	.fmask	0x00F00000, -168
	.frame	$sp, 200, $31
	.loc	2 117
	.loc	2 120
 # 118	    double lam, phi, sph, cph, tph, t, al, al2, n;
 # 119	    double xco[4], yco[4];
 # 120	    lam = (pc -> lon - THETA0) * RADIANS;
	lw	$14, 200($sp)
	l.d	$f4, 0($14)
	li.d	$f6, -2.0000000000000000e+00
	sub.d	$f8, $f4, $f6
	li.d	$f10, 1.7453292519943295e-02
	mul.d	$f16, $f8, $f10
	s.d	$f16, 192($sp)
	.loc	2 121
 # 121	    phi = pc -> lat * RADIANS;
	l.d	$f18, 8($14)
	li.d	$f4, 1.7453292519943295e-02
	mul.d	$f6, $f18, $f4
	s.d	$f6, 184($sp)
	.loc	2 122
 # 122	    sph = sin(phi); cph = cos(phi); tph = sph/cph; t = tph*tph;
	mov.d	$f12, $f6
	.livereg	0x0000000E,0x000C0000
	jal	sin
	s.d	$f0, 176($sp)
	.loc	2 122
	l.d	$f12, 184($sp)
	.livereg	0x0000000E,0x000C0000
	jal	cos
	s.d	$f0, 168($sp)
	.loc	2 122
	l.d	$f8, 176($sp)
	l.d	$f10, 168($sp)
	div.d	$f16, $f8, $f10
	s.d	$f16, 160($sp)
	.loc	2 122
	mul.d	$f18, $f16, $f16
	s.d	$f18, 152($sp)
	.loc	2 123
 # 123	    al = cph*lam; al2 = al*al;
	l.d	$f4, 192($sp)
	mul.d	$f6, $f10, $f4
	s.d	$f6, 144($sp)
	.loc	2 123
	mul.d	$f16, $f6, $f6
	s.d	$f16, 136($sp)
	.loc	2 124
 # 124	    al /= sqrt(1.0 - ECC2*sph*sph);
	li.d	$f18, 6.670540000123428e-3
	mul.d	$f10, $f18, $f8
	mul.d	$f4, $f10, $f8
	li.d	$f6, 1.0
	sub.d	$f12, $f6, $f4
	.livereg	0x0000000E,0x000C0000
	jal	sqrt
	mov.d	$f20, $f0
	l.d	$f16, 144($sp)
	div.d	$f18, $f16, $f20
	s.d	$f18, 144($sp)
	.loc	2 125
 # 125	    n = (ECC2/(1.0-ECC2))*cph*cph;
	li.d	$f10, 6.7153349102565194e-03
	l.d	$f8, 168($sp)
	mul.d	$f6, $f10, $f8
	mul.d	$f4, $f6, $f8
	s.d	$f4, 128($sp)
	.loc	2 127
 # 126	
 # 127	    xco[0] = POLY3(t, xtab1) / 5040;					/* coeff of al2^3 */
	la	$15, xtab1
	l.d	$f16, 0($15)
	l.d	$f18, 152($sp)
	mul.d	$f10, $f16, $f18
	l.d	$f6, 8($15)
	add.d	$f8, $f10, $f6
	mul.d	$f4, $f8, $f18
	l.d	$f16, 16($15)
	add.d	$f10, $f4, $f16
	mul.d	$f6, $f10, $f18
	l.d	$f8, 24($15)
	add.d	$f4, $f8, $f6
	li.d	$f16, 5.0400000000000000e+03
	div.d	$f10, $f4, $f16
	addu	$24, $sp, 96
	s.d	$f10, 0($24)
	.loc	2 128
 # 128	    xco[1] = (POLY2(t, xtab2) + n * POLY1(t, xtab3)) / 120;		/* coeff of al2^2 */
	la	$25, xtab3
	l.d	$f18, 0($25)
	l.d	$f8, 152($sp)
	mul.d	$f6, $f18, $f8
	l.d	$f4, 8($25)
	add.d	$f16, $f4, $f6
	l.d	$f10, 128($sp)
	mul.d	$f18, $f16, $f10
	la	$8, xtab2
	l.d	$f4, 0($8)
	mul.d	$f6, $f4, $f8
	l.d	$f16, 8($8)
	add.d	$f10, $f6, $f16
	mul.d	$f4, $f10, $f8
	l.d	$f6, 16($8)
	add.d	$f16, $f4, $f6
	add.d	$f10, $f18, $f16
	li.d	$f8, 1.2000000000000000e+02
	div.d	$f4, $f10, $f8
	addu	$9, $sp, 96
	s.d	$f4, 8($9)
	.loc	2 129
 # 129	    xco[2] = (1 - t + n) / 6;						/* coeff of al2^1 */
	li.d	$f6, 1.0000000000000000e+00
	l.d	$f18, 152($sp)
	sub.d	$f16, $f6, $f18
	l.d	$f10, 128($sp)
	add.d	$f8, $f16, $f10
	li.d	$f4, 6.0000000000000000e+00
	div.d	$f6, $f8, $f4
	addu	$10, $sp, 96
	s.d	$f6, 16($10)
	.loc	2 130
 # 130	    xco[3] = 1.0;							/* coeff of al2^0 */
	li.d	$f18, 1.0
	addu	$11, $sp, 96
	s.d	$f18, 24($11)
	.loc	2 132
 # 131	
 # 132	    cc -> x = X0 + (AIRY_A*SCALE) * al * POLY3(al2, xco);
	addu	$12, $sp, 96
	l.d	$f16, 0($12)
	l.d	$f10, 136($sp)
	mul.d	$f8, $f16, $f10
	l.d	$f4, 8($12)
	add.d	$f6, $f8, $f4
	mul.d	$f18, $f6, $f10
	l.d	$f16, 16($12)
	add.d	$f8, $f18, $f16
	mul.d	$f4, $f8, $f10
	l.d	$f6, 24($12)
	add.d	$f18, $f6, $f4
	li.d	$f16, 6.3750204789897688e+06
	l.d	$f8, 144($sp)
	mul.d	$f10, $f16, $f8
	mul.d	$f6, $f18, $f10
	li.d	$f4, 400000.0
	add.d	$f16, $f6, $f4
	lw	$13, 204($sp)
	s.d	$f16, 0($13)
	.loc	2 134
 # 133	
 # 134	    yco[0] = POLY3(t, ytab1) / 40320;					/* coeff of al2^3 */
	la	$14, ytab1
	l.d	$f8, 0($14)
	l.d	$f18, 152($sp)
	mul.d	$f10, $f8, $f18
	l.d	$f6, 8($14)
	add.d	$f4, $f10, $f6
	mul.d	$f16, $f4, $f18
	l.d	$f8, 16($14)
	add.d	$f10, $f16, $f8
	mul.d	$f6, $f10, $f18
	l.d	$f4, 24($14)
	add.d	$f16, $f4, $f6
	li.d	$f8, 4.0320000000000000e+04
	div.d	$f10, $f16, $f8
	addu	$15, $sp, 64
	s.d	$f10, 0($15)
	.loc	2 135
 # 135	    yco[1] = (POLY2(t, ytab2) + n * POLY1(t, ytab3)) / 720;		/* coeff of al2^2 */
	la	$24, ytab3
	l.d	$f18, 0($24)
	l.d	$f4, 152($sp)
	mul.d	$f6, $f18, $f4
	l.d	$f16, 8($24)
	add.d	$f8, $f16, $f6
	l.d	$f10, 128($sp)
	mul.d	$f18, $f8, $f10
	la	$25, ytab2
	l.d	$f16, 0($25)
	mul.d	$f6, $f16, $f4
	l.d	$f8, 8($25)
	add.d	$f10, $f6, $f8
	mul.d	$f16, $f10, $f4
	l.d	$f6, 16($25)
	add.d	$f8, $f16, $f6
	add.d	$f10, $f18, $f8
	li.d	$f4, 7.2000000000000000e+02
	div.d	$f16, $f10, $f4
	addu	$8, $sp, 64
	s.d	$f16, 8($8)
	.loc	2 136
 # 136	    yco[2] = (5. - t + n * (9. + 4. * n)) / 24;				/* coeff of al2^1 */
	li.d	$f6, 5.
	l.d	$f18, 152($sp)
	sub.d	$f8, $f6, $f18
	li.d	$f10, 4.
	l.d	$f4, 128($sp)
	mul.d	$f16, $f10, $f4
	li.d	$f6, 9.
	add.d	$f18, $f6, $f16
	mul.d	$f10, $f4, $f18
	add.d	$f6, $f8, $f10
	li.d	$f16, 2.4000000000000000e+01
	div.d	$f4, $f6, $f16
	addu	$9, $sp, 64
	s.d	$f4, 16($9)
	.loc	2 137
 # 137	    yco[3] = 0.5;							/* coeff of al2^0 */
	li.d	$f18, 0.5
	addu	$10, $sp, 64
	s.d	$f18, 24($10)
	.loc	2 139
 # 138	
 # 139	    cc -> y = Y0 + (AIRY_A*SCALE) * (meridist(phi) - meridist(PHI0*RADIANS) + sph*lam*al * POLY3(al2, yco));
	li.d	$f12, 8.5521133347722145e-01
	.livereg	0x0000000E,0x000C0000
	jal	meridist
	mov.d	$f22, $f0
	l.d	$f12, 184($sp)
	.livereg	0x0000000E,0x000C0000
	jal	meridist
	mov.d	$f20, $f0
	addu	$11, $sp, 64
	l.d	$f8, 0($11)
	l.d	$f10, 136($sp)
	mul.d	$f6, $f8, $f10
	l.d	$f16, 8($11)
	add.d	$f4, $f6, $f16
	mul.d	$f18, $f4, $f10
	l.d	$f8, 16($11)
	add.d	$f6, $f18, $f8
	mul.d	$f16, $f6, $f10
	l.d	$f4, 24($11)
	add.d	$f18, $f4, $f16
	l.d	$f8, 176($sp)
	l.d	$f6, 192($sp)
	mul.d	$f10, $f8, $f6
	l.d	$f4, 144($sp)
	mul.d	$f16, $f10, $f4
	mul.d	$f8, $f18, $f16
	sub.d	$f6, $f20, $f22
	add.d	$f10, $f8, $f6
	li.d	$f4, 6.3750204789897688e+06
	mul.d	$f18, $f10, $f4
	li.d	$f16, -1.0000000000000000e+05
	add.d	$f8, $f18, $f16
	lw	$12, 204($sp)
	s.d	$f8, 8($12)
	.loc	2 141
 # 140	
 # 141	  };
	.livereg	0x2000FF0E,0x00000FFF
	l.d	$f20, 24($sp)
	l.d	$f22, 32($sp)
	lw	$31, 44($sp)
	addu	$sp, 200
	j	$31
	.end	my_convert
	.text	
	.align	2
	.file	2 "testpolos.c"
	.loc	2 149
 # 149	  { /* meridional distance to latitude phi */
	.ent	meridist 2
meridist:
	.option	O1
	.set	 noreorder
	.cpload	$25
	.set	 reorder
	subu	$sp, 64
	sw	$31, 28($sp)
	.cprestore	24
	s.d	$f12, 64($sp)
	.mask	0x90000000, -36
	.frame	$sp, 64, $31
	.loc	2 149
	.loc	2 151
 # 150	    double sph, cph, cs, ss;
 # 151	    sph = sin(phi); cph = cos(phi);
	l.d	$f12, 64($sp)
	.livereg	0x0000000E,0x000C0000
	jal	sin
	s.d	$f0, 56($sp)
	.loc	2 151
	l.d	$f12, 64($sp)
	.livereg	0x0000000E,0x000C0000
	jal	cos
	s.d	$f0, 48($sp)
	.loc	2 152
 # 152	    cs = cph*sph; ss = sph*sph;
	l.d	$f4, 48($sp)
	l.d	$f6, 56($sp)
	mul.d	$f8, $f4, $f6
	s.d	$f8, 40($sp)
	.loc	2 152
	mul.d	$f10, $f6, $f6
	s.d	$f10, 32($sp)
	.loc	2 153
 # 153	    return (enpars[4] * phi) - (POLY3(ss, enpars) * cs);
	la	$14, enpars
	l.d	$f16, 0($14)
	mul.d	$f18, $f16, $f10
	l.d	$f4, 8($14)
	add.d	$f6, $f18, $f4
	mul.d	$f16, $f6, $f10
	l.d	$f18, 16($14)
	add.d	$f4, $f16, $f18
	mul.d	$f6, $f4, $f10
	l.d	$f16, 24($14)
	add.d	$f18, $f16, $f6
	mul.d	$f4, $f18, $f8
	l.d	$f10, 32($14)
	l.d	$f16, 64($sp)
	mul.d	$f6, $f10, $f16
	sub.d	$f0, $f6, $f4
	.livereg	0x0000FF0E,0xF0000FFF
	lw	$31, 28($sp)
	addu	$sp, 64
	j	$31
	.end	meridist
	.text	
	.align	2
	.file	2 "testpolos.c"
	.loc	2 157
 # 154	  }
 # 155	
 # 156	static pj_convert(pc, cc) struct pco *pc; struct cco *cc;
 # 157	  { FILE *pfi; char cmd[256]; int ni;
	.ent	pj_convert 2
pj_convert:
	.option	O1
	.set	 noreorder
	.cpload	$25
	.set	 reorder
	subu	$sp, 344
	sw	$31, 76($sp)
	.cprestore	72
	sw	$4, 344($sp)
	sw	$5, 348($sp)
	.mask	0x90000000, -268
	.frame	$sp, 344, $31
	.loc	2 157
	.loc	2 158
 # 158	    sprintf(cmd, "echo '%.4f %.4f' | proj +proj=tmerc +ellps=airy +lat_0=%.0f +lon_0=%.0f +k_0=%.9f +x_0=%.0f +y_0=%.0f",
	addu	$4, $sp, 84
	la	$5, $$44
	lw	$14, 344($sp)
	ld	$6, 0($14)
	l.d	$f4, 8($14)
	s.d	$f4, 16($sp)
	li.d	$f6, 49.0
	s.d	$f6, 24($sp)
	li.d	$f8, -2.0000000000000000e+00
	s.d	$f8, 32($sp)
	li.d	$f10, 0.9996012717
	s.d	$f10, 40($sp)
	li.d	$f16, 400000.0
	s.d	$f16, 48($sp)
	li.d	$f18, -1.0000000000000000e+05
	s.d	$f18, 56($sp)
	.livereg	0x0F00000E,0x00000000
	jal	sprintf
	.loc	2 160
 # 159			 pc -> lon, pc -> lat, PHI0, THETA0, SCALE, X0, Y0);
 # 160	    pfi = popen(cmd, "r");
	addu	$4, $sp, 84
	la	$5, $$46
	.livereg	0x0C00000E,0x00000000
	jal	popen
	sw	$2, 340($sp)
	.loc	2 161
 # 161	    if (pfi == NULL) giveup("popen failed");
	lw	$15, 340($sp)
	bne	$15, 0, $45
	.loc	2 161
	la	$4, $$48
	.livereg	0x0800000E,0x00000000
	jal	giveup
$45:
	.loc	2 162
 # 162	    ni = fscanf(pfi, "%lg %lg\n", &cc -> x, &cc -> y);
	lw	$4, 340($sp)
	la	$5, $$50
	lw	$24, 348($sp)
	move	$6, $24
	addu	$7, $24, 8
	.livereg	0x0F00000E,0x00000000
	jal	fscanf
	sw	$2, 80($sp)
	.loc	2 163
 # 163	    pclose(pfi);
	lw	$4, 340($sp)
	.livereg	0x0800000E,0x00000000
	jal	pclose
	.loc	2 164
 # 164	    unless (ni == 2) giveup("fscanf error, ni = %d", ni);
	lw	$25, 80($sp)
	beq	$25, 2, $46
	.loc	2 164
	la	$4, $$52
	move	$5, $25
	.livereg	0x0C00000E,0x00000000
	jal	giveup
	.loc	2 165
 # 165	  }
$46:
	.livereg	0x2000FF0E,0x00000FFF
	lw	$31, 76($sp)
	addu	$sp, 344
	j	$31
	.end	pj_convert
	.text	
	.align	2
	.file	2 "testpolos.c"
	.loc	2 168
 # 166	
 # 167	static giveup(msg, p1) char *msg; int p1;
 # 168	  { fprintf(stderr, msg, p1); putc('\n', stderr);
	.ent	giveup 2
giveup:
	.option	O1
	.set	 noreorder
	.cpload	$25
	.set	 reorder
	subu	$sp, 32
	sw	$31, 28($sp)
	.cprestore	24
	sw	$4, 32($sp)
	sw	$5, 36($sp)
	.mask	0x90000000, -4
	.frame	$sp, 32, $31
	.loc	2 168
	.loc	2 168
	la	$4, __iob
	addu	$4, $4, 32
	lw	$5, 32($sp)
	lw	$6, 36($sp)
	.livereg	0x0E00000E,0x00000000
	jal	fprintf
	.loc	2 168
	lw	$14, __us_rsthread_stdio
	beq	$14, 0, $47
	li	$4, 10
	la	$5, __iob
	addu	$5, $5, 32
	.livereg	0x0C00000E,0x00000000
	jal	__semputc
	b	$49
$47:
	la	$15, __iob
	lw	$24, 32($15)
	addu	$25, $24, -1
	sw	$25, 32($15)
	la	$8, __iob
	lw	$9, 32($8)
	bge	$9, 0, $48
	li	$4, 10
	addu	$5, $8, 32
	.livereg	0x0C00000E,0x00000000
	jal	__flsbuf
	b	$49
$48:
	li	$10, 10
	la	$11, __iob
	lw	$12, 36($11)
	sb	$10, 0($12)
	la	$13, __iob
	lw	$14, 36($13)
	addu	$24, $14, 1
	sw	$24, 36($13)
$49:
	.loc	2 169
 # 169	    exit(1);
	li	$4, 1
	.livereg	0x0800000E,0x00000000
	jal	exit
	.loc	2 170
 # 170	  }
	.livereg	0x2000FF0E,0x00000FFF
	lw	$31, 28($sp)
	addu	$sp, 32
	j	$31
	.end	giveup
