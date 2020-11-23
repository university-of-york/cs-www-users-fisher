      PROGRAM MERCATOR PROJECTION  ii

*     CONVERTS UK GRID REFERENCE TO LATITUDE AND LONGITUDE
*     ACCURATE TO 0.002 Secs.
*     copyright C P Swan 1989

      IMPLICIT DOUBLE PRECISION (A-Z)
      INTEGER J
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
1000  FORMAT('	 DEGS = 'I4)
1001  FORMAT('	 MINS = 'I4)
1002  FORMAT('	 SECS = 'F7.3)
1003  FORMAT(A1)
1004  FORMAT(' EASTING	= ',$)
1005  FORMAT(' NORTHING = ',$)
1     WRITE(*,*)
      WRITE(*,*)'INPUT GRID REFERENCE '
      WRITE(*,1004)
      READ(*,*)EAST
      WRITE(*,1005)
      READ(*,*)NORTH
      LAT=55*DTOR
      CHANGE=3*DTOR
      DO 100 J=1,50
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
      IF (I .LT. NORTH) LAT=LAT+CHANGE
      IF (I .GT. NORTH) LAT=LAT-CHANGE
      CHANGE=CHANGE/2

100   CONTINUE

      RV=A/((1-(E*(S**2)))**.5)
*     WRITE(*,*)'RV ',RV
      RP=(RV*(1-E))/(1-E*(S**2))
*     WRITE(*,*)'RP ',RP
      H =(RV/RP)-1
*     WRITE(*,*)'H ',H

      FS=(F**2)*SSEC
      VII=(T*1E12)/(2*RV*RP*FS)
*      WRITE(*,*)'VII ',VII
      VIII=(T*(5+(3*(T**2))+H-(9*(T**2)*H))*1E24)/(24*RP*(RV**3)*FS)
*      WRITE(*,*)'VIII ',VIII
      IX=(T*(61+(90*(T**2))+(45*(T**4)))*1E36)/(720*RP*(RV**5)*FS)
*      WRITE(*,*)'IX ',IX
      Q=(EAST-400E3)*1E-6
*      WRITE(*,*)'Q ',Q
      LATDEGS=((LAT*3600)/DTOR)-((Q**2)*VII)+((Q**4)*VIII)-((Q**6)*IX)
      LATDEGS=LATDEGS/3600
*      WRITE(*,*)'LATDEGS ',LATDEGS
      LATMINS=(LATDEGS-INT(LATDEGS))*60
*      WRITE(*,*)'LATMINS ',LATMINS
      LATSECS=(LATMINS-INT(LATMINS))*60
*      WRITE(*,*)'LATSECS ',LATSECS

      X=1E6/(C*RV*SSEC*F)
*      WRITE(*,*)'X ',X
      XI=(((RV/RP)+(2*(T**2)))*1E18)/(C*6*(RV**3)*SSEC*(F**3))
*      WRITE(*,*)'XI ',XI
      XII=((5+(28*(T**2))+(24*(T**4)))*1E30)/(C*120*(RV**5)*SSEC*F)
*      WRITE(*,*)'XII ',XII
      LONG=(-7200+(Q*X)-((Q**3)*XI)+((Q**5)*XII))
      LONGD=LONG/3600
      LONGM=(LONGD-INT(LONGD))*60
      LONGS=(LONGM-INT(LONGM))*60
*      WRITE(*,*)'LONGD ',LONGD
*      WRITE(*,*)'LONGM ',LONGM
*      WRITE(*,*)'LONGS ',LONGS

      WRITE(*,*)
      WRITE(*,*)'LATITUDE'
      WRITE(*,1000)INT(LATDEGS)
      WRITE(*,1001)INT(LATMINS)
      WRITE(*,1002)LATSECS
      WRITE(*,*)
      WRITE(*,*)'LONGITUDE'
      IF (LONGD .LT. 0) THEN
	WRITE(*,*)' WEST'
	LONGD=ABS(LONGD)
	LONGM=ABS(LONGM)
	LONGS=ABS(LONGS)
       ELSE
	WRITE(*,*)' EAST'
       ENDIF
      WRITE(*,1000)INT(LONGD)
      WRITE(*,1001)INT(LONGM)
      WRITE(*,1002)LONGS
      WRITE(*,*)
3000  WRITE(*,3001)
3001  FORMAT(' TYPE Q TO QUIT ANY OTHER CHARACTER TO RE-RUN.  '$)
      READ(*,1003)CHAR
      IF (CHAR .NE. 'Q' .AND. CHAR .NE. 'q') GOTO 1


      END
