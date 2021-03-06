<use ~/a4-11on7.Fmt
<ft NCSR.11 NCSI.11 C.11
<tabs 4n 8n 12n
<ls 2

<tr `` x*   <tr '' x:	<tr fi x.   <tr fl x/
<tr -- x1	   \; en dash
<tr --- xP	   \; em dash
<tr _ h3p	   \; narrow space
<tr $ x2	   \; dagger
<tr % x'	   \; section
<tr ... x<	   \; Three Dots

<tr +-	fS.11x1	   \; plus or minus
<tr 'oo' fS.11x%   \; infinity
<tr /= fS.11x9	   \; not equal
<tr \<= fS.11x#	   \; less than or equal
<tr \>= fS.11x3	   \; greater than or equal
<tr \< fS.11c<	   \; less than
<tr \> fS.11c>	   \; greater than

<tr /a fS.11ca	   \; Gk alpha
<tr /b fS.11cb	   \; Gk beta
<tr /t fS.11ct	   \; Gk theta
<tr /p fS.11cp	   \; Gk tau
<tr 'w' fS.11cw	   \; Gk omega

<tr |1 fS.15xl	   \; L brace top
<tr |2 fS.15xm	   \; L brace middle
<tr |3 fS.15xn	   \; L brace bottom
<tr || fS.15xo	   \; brace bar

<def PARA < \2 \h 2m ... \2 >
<def SUBS < <ft NCSI.11 - - \4 \arg >ft \2 ... >
<def SECT < \need 5 <ft NCSB.11 CBO.11 CB.11 \4 \arg >ft \2 ... >
<def CHAP < \page <ft NCSB.15 - - <ce \arg >ce >ft \5 ... >
<def TABL < \2 ... \2 >
<def EQN < \3 <in 2m <eqn <eqnce <ls 1 ... >ls >eqnce >eqn >in \3 >

<text

<CHAP < Raised Cosine Filters >

{ A ^r^a^i^s^e^d ^c^o^s^i^n^e ^f^i^l^t^e^r is a low-pass filter which is commonly used for
pulse shaping in data transmission systems (e.g. modems). }

{ Let /t be the
duration of a baud, so the baud rate is 1//t symbols per second.  The
frequency response of a raised cosine filter is symmetrical about 0_Hz,
and is divided into three parts:

<tabs 4m 6m 20m
<EQN
< \t |1 \t /t \t for_  0 _<=_ ^f _<=_ {{ 1 -- /b over 2/t }} \g >
< \t || >
< \t || >
< \t || >
< \t >
< ^X(^f) _= \t |2 \t {{ /t over 2 }} lpar 1 + cos lpar {{ /p/t over /b }}
lpar ^f -- {{ 1 -- /b over 2/t }} rpar rpar rpar \t for _ {{ 1 -- /b over
2/t }} _<=_ ^f _<=_ {{ 1 + /b over 2/t }} \g (1) >
< \t >
< \t || >
< \t || >
< \t || >
< \t |3 \t 0 \t for _ ^f _>=_ {{ 1 + /b over 2/t }} \g >
>EQN
>tabs }

{ The parameter /b is known as the ^r^o^l^l^-^o^f^f ^f^a^c^t^o^r or ^e^x^c^e^s^s ^b^a^n^d^w^i^d^t^h.
/b lies between 0 and 1. }

{ The minimum bandwidth needed to transmit a pulse train at 1//t symbols
per second is /a_=_1_/_(2/t)_Hz.  For example, if the baud rate is 2400
and the carrier frequency is 1800, the necessary bandwidth is from 600
to 3000_Hz, and the minimum low-pass filter bandwidth is 1200_Hz.  This
requires a filter with a ``brick-wall'' (square) low-pass response.
That's what you get if you specify /b_=_0.  A ``brick-wall'' filter has
a ``sinc'' impulse response with slowly decaying tails, and is difficult
to implement and generally unsatisfactory. }

{ Non-zero values of /b specify a filter with a less square frequency
response, and a more manageable impulse response.  The rule is that the
filter response (or, alternatively, the transmitted spectrum) stretches
from 0_Hz to (1_+_/b)_/a_Hz.  That's why /b is called the excess
bandwidth:  it specifies the amount by which the filter bandwidth
exceeds the minimum necessary to transmit a pulse train with the
speci-fied baud rate. }

{ The time-domain (impulse) response of a raised cosine filter, obtained
by taking the inverse Fourier transform of the spectrum, can be shown to
be:

<EQN
< ^x(^t) _=_ lpar sinc {{ /p^t over /t }} rpar _ lpar {{ cos /p/b^t / /t
over 1 -- (2/b^t / /t)/2 }} rpar \g (2) >
>EQN }

{ In a modem, the raised cosine frequency response is divided equally
between the transmitter and the receiver.  Each contains an identical
filter whose magnitude response is given by the square root of
eqn._(1), normalized so that the maximum remains /t.  This is called a
^r^o^o^t ^r^a^i^s^e^d ^c^o^s^i^n^e filter. }

{ The #m#k#s#h#a#p#e program (which is called by the ``mkfilter'' web page)
designs a raised cosine or root raised cosine filter by first using
eqn._(1) to compute the frequency response, then performing an inverse
fast Fourier transform to compute the time-domain (impulse) response.  }

\3

< A.J. Fisher >
< #h#t#t#p#:#/#/#w#w#w#-#u#s#e#r#s#.#c#s#.#y#o#r#k#.#a#c#.#u#k#/#~#f#i#s#h#e#r \g 30 Aug 1999 >

>CHAP

