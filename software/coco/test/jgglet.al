<use ~/a4-11on14.fmt
<ft H.11 HO.11 HB.11
<tabs 4n 8n 12n

<tr `` x*   <tr '' x:	<tr fi x.   <tr fl x/
<tr -- x1	   \; en dash
<tr --- xP	   \; em dash
<tr @ fNCSR.11x7   \; bullet
<tr _ h3p	   \; narrow space
<tr /I fS.15xr	   \; integral
<tr /= fS.11x9	   \; not equal
<tr /o fS.11x0	   \; degrees

<tr /a fS.11ca	   \; Gk alpha
<tr /l fS.11cl	   \; Gk lambda
<tr /ph fS.11cj	   \; Gk phi
<tr /th fS.11cq	   \; Gk theta

<def PARA < \1 \need 2 \h 1m ... \1 >
<def SUBS < <ft HO.11 - - \2 \need 4 \arg >ft \1 ... >
<def SECT < <ft HB.11 - - \2 \need 4 \arg >ft \1 ... >
<def CHAP < \page <ft HB.11 - - <ce \arg >ce >ft \2 ... >
<def TOME < \page \line 13 <ft HB.24 - - <ce \arg >ce >ft \page ... >
<def PROG < <ft C.11 HO.11 H.11
	    <-fi <-ju <ws ... \1 >ws >-ju >-fi >ft >
<def EQN < \1 ... \1 >

<text

< \tr i"/usr/fisher/mipslib/letters/dh" >

\10

<ft HB.18 - - <ce
Facsimile message to: \h 1m Dr J.G. Griffiths \1
< 2 pages >
>ce >ft

\6

<in 5.0i
< 8th May, 1995 >
>in

\3

< Dear John, >

{ You might remember that you jotted down some equations for me for
computing the Mercator projection from an oblate spheroid, i.e.
given latitude /ph and longitude /th, compute easting ^x and northing ^y. }

{ I've been using these equations in a modified form for a couple of years
with success.  The modification is this:  I approximated the elliptic
integral (which gives ^y in terms of /th and /ph) by (1+/a)_^r^T, where /a
is a small positive constant.  This was satisfactory because I was
working with a radionavigation system that covered a small area, so the
approx-imation was not too far out. }

{ I am now working with Loran-C, and the deficiencies of the
approximation are now apparent.	 I'm now using trapezoidal numerical
integration to evaluate the elliptic integral.	That is not the problem.
My problem is that I have found what appear
to be errors in your results.  The following assumes geographic
coordinates, ^v^i^z^. longitude /th_=_0 on the central meridian (the line of
contact between Mercator's cylinder and the earth), and /ph_=_0 on the
equator. }

{ Your equation for ^y is

<EQN
< \t ^y _=_ /I\0'T (1 -- ^e/2 cos/2 ^t)/1///2 d^t	  \g (1) >
>EQN

This gives the wrong results (out by a kilometre or so over the whole of
the U.K.), compared with ``official'' Ordnance Survey coordinates.  A
correct equation is

<EQN
< \t ^y _=_ /I\0'T (1 -- ^e/2) (1 -- ^e/2 sin/2 ^t)/-/-/3///2 d^t    \g (2) >
>EQN

The limits of integration are from 0 to /ph, in both cases, when
/th_=_0. }

{ The derivation of the ``correct'' equation is as follows.  If ^s is the
path length along an ellipsoid, then d^s_=_^r_d/th, where ^r is the radius
of curvature at that point -- not the distance to the centre of the
spheroid, which is what you seem to be assuming.  Integrating gives
eqn. 2.	 I wonder if you'd like to comment on this? }

{ I'm in need of some advice.  As I mentioned, I've succeeded in
calculating ^y in terms of /ph when /th_=_0, using my eqn. (2).
Unfortunately my eqn. (2) doesn't work for /th_/=_0,
and your eqn. for ^x in terms of /th and /ph is wrong almost everywhere!
These errors have sprung to light only recently because I am now
treating as ``serious'' a distance error of ~1_m over an area the size
of the U.K.  I need a set of correct equations which give ^x and ^y in
terms of /th and /ph, assuming an oblate spheroidal earth. }

{ Of course all this must have been worked out in the 19th century; but
I can find no appropriate text-book in our meagre library.  It is
possible that I am mis-applying your equations:	 they assume that the
latitude of the Mercator origin (the parallel along which ^y is
independent of longitude /th) is the equator; in fact in the Ordnance
Survey projection it is 49/o N.	 I have of course attempted to take this
into account, but I am still getting the wrong results. }

{ Do you have any comments on this?  Can you provide a reference to a
good text-book, or a corrected set of equations? }

{ I had a splendid time yesterday in Duffield, playing in a ``scratch''
performance of Shostakovich 10 (nicknamed ^T^h^e ^U^n^p^l^a^y^a^b^l^e on the oboe
part!). }

\1

< Best regards to you & your family, >
\4
< Tony >

\1

< ------------------------------------------------------------ >

\1

< Your equations: >

\1

< /th = longitude, 0 = central meridian >
< /ph = latitude, 0 = equator >
< ^a = major (equatorial) radius of earth >

\1

<EQN
< \t  /l _=_ (cos/2 /ph cos/2 /th + sin/2 /ph)/-/-/1///2	\g >
>EQN

<EQN
< \t ^x _=_ /l ^a cos /ph sin /th				\g >
>EQN

<EQN
< \t ^T _=_ sin/-/-/1 (/l sin /ph)				\g >
>EQN

<EQN
< \t ^y _=_ /I\0'T (1 -- ^e/2 cos/2 ^t)/1///2 d^t			\g >
>EQN

