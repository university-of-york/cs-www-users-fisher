      PROGRAM MERCATOR PROJECTION

*     CONVERTS LATITUDE AND LONGITUDE TO UK GRID REFERENCE
*     ACCURATE TO 0.02m
*     copyright C P Swan 1989

      IMPLICIT DOUBLE PRECISION (A-Z)
      CHARACTER*1 CHAR
      DTOR = 3.141592654/180
      A = 6377563.394
      B = 6356256.91
      F = 0.9996012717
      E = 0.006670540000123428
      N = 0.001673220310
      MA= (1+N+(5*(N**2)/4)+(5*(N**3)/4))
      MB= (3*N+3*(N**2)+(21/8)*(N**3))
      MC= 15*((N**2)+(N**3))/8
      MD= 35*(N**3)/24
      U = 49*DTOR
      SSEC = 4.848136811E-6
1000  FORMAT('	 DEGS = '$)
1001  FORMAT('	 MINS = '$)
1002  FORMAT('	 SECS = '$)
1003  FORMAT(A1)
1004  FORMAT(' EASTING	=',F10.1)
1005  FORMAT(' NORTHING =',F10.1)
1     WRITE(*,*)
      WRITE(*,*)'INPUT LATITUDE '
      WRITE(*,1000)
      READ(*,*)LATDEGS
      WRITE(*,1001)
      READ(*,*)LATMINS
      WRITE(*,1002)
      READ(*,*)LATSECS
      LAT=(LATDEGS+(LATMINS/60)+(LATSECS/3600))*DTOR
      WRITE(*,*)
*     WRITE(*,*)' LATITUDE (RADS) ',LAT
      WRITE(*,*)'INPUT LONGITUDE'
      WRITE(*,1000)
      READ(*,*)LONGDEGS
      WRITE(*,1001)
      READ(*,*)LONGMINS
      WRITE(*,1002)
      READ(*,*)LONGSECS
      LONG=LONGDEGS*3600+LONGMINS*60+LONGSECS
      WRITE(*,*)
2000  WRITE(*,2001)
2001  FORMAT(' IS LONGITUDE [E]AST OR [W]EST ?'$)
      READ(*,1003)CHAR
      IF (CHAR .EQ. 'W' .OR. CHAR .EQ. 'w') THEN
	LONG=-LONG
      ELSE
	IF (CHAR .NE. 'e' .AND. CHAR .NE. 'E') GOTO 2000
      ENDIF
*     WRITE(*,*) ' LONGITUDE (SECS) ',LONG
      PA=LAT-U
      PB=LAT+U
      S =DSIN(LAT)
      C =DCOS(LAT)
      T =DTAN(LAT)
      SA=DSIN(PA)
      SB=DSIN(2*PA)
      SC=DSIN(3*PA)
      CA=DCOS(PB)
      CB=DCOS(2*PB)
      CC=DCOS(3*PB)
      M =B*F*(MA*PA-MB*SA*CA+MC*SB*CB-MD*SC*CC)
      I =M-100000
      RV=A/((1-(E*(S**2)))**.5)
*     WRITE(*,*)'RV ',RV
      RP=(RV*(1-E))/(1-E*(S**2))
*     WRITE(*,*)'RP ',RP
      H =(RV/RP)-1
*     WRITE(*,*)'H ',H
      II=F*RV*(SSEC**2)*S*C*0.5E8
      III=F*(RV*(SSEC**4)*S*(C**3)*(5-(T**2)+9*H)*1E16)/24
      IIIA=F*(RV*(SSEC**6)*S*(C**5)*(61-58*(T**2)+(T**4))*1E24)/720
      IV=F*RV*SSEC*C*1E4
      V=F*(RV*(SSEC**3)*(C**3)*((RV/RP)-(T**2))*1E12)/6
      VI=F*(RV*(SSEC**5)*(C**5)*
     1(5-18*(T**2)+(T**4)+14*H-58*(T**2)*H+2*H*(T**4))*1E20)/120
      P=(LONG+7200)*1E-4
      NORTH=I+II*(P**2)+III*(P**4)+IIIA*(P**6)
      EAST=400000+P*IV+(P**3)*V+(P**5)*VI
*     WRITE(*,*)'I    ',I
*     WRITE(*,*)'II   ',II
*     WRITE(*,*)'III  ',III
*     WRITE(*,*)'IIIA ',IIIA
*     WRITE(*,*)'IV   ',IV
*     WRITE(*,*)'V    ',V
*     WRITE(*,*)'VI   ',VI
*     WRITE(*,*)'P    ',P
      WRITE(*,*)
      WRITE(*,1004)EAST
      WRITE(*,1005)NORTH
      WRITE(*,*)
3000  WRITE(*,3001)
3001  FORMAT(' TYPE Q TO QUIT ANY OTHER CHARACTER TO RE-RUN. '$)
      READ(*,1003)CHAR
      IF (CHAR .NE. 'Q' .AND. CHAR .NE. 'q') GOTO 1
      END

