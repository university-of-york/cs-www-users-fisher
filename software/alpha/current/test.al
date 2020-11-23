<use ~/a4-11on14.Fmt
<ls 2 <in 0.4i <rin 0.4i \; formatting for JSS
<ft NCSR.11 NCSI.11 NCSB.11
<tabs 2m 4m 6m 8m
<tr `` x*   <tr '' x:	<tr fi x.   <tr fl x/
<tr -- x1	   \; en dash
<tr --- xP	   \; em dash
<tr _ h3p	   \; narrow space
<tr @ fNCSR.11x7   \; bullet
<tr # fNCSR.11xuxH \; i two dots
<tr => fS.11x^	   \; right double arrow
<tr \<= fS.11x#	   \; less than or equal
<tr $ x2	   \; dagger
<tr +- fS.11x1	   \; plus or minus
<tr /e fS.11ce	   \; Gk epsilon
<tr /z fS.11cz	   \; Gk zeta
<tr /u fS.11cm	   \; Gk mu
<tr /p fS.11cj	   \; Gk phi
<tr 'p' fS.11cp	   \; Gk pi
<tr '*' fS.11x4	   \; cross
<tr '+' fS.11xE	   \; plus in a circle
<tr '-' fH.10cOfS.11c-	\; minus in a circle
<tr " fS.11x2      \; double prime
<tr /\ fS.11xY	   \; logical and

<foot 1 1 < \g -- # -- \g >

<def PARA < \1 \need 2 \h 3m ... \1 >
<def SUBS < <ft NCSI.11 - - \2 \need 4 \arg >ft \1 ... >
<def SECT < <ft NCSB.8 - - \2 \need 4 \arg || \arg >ft \1 ... >
<def CHAP < \page <ft NCSB.15 - - <ce \arg >ce >ft \2 ... >
<def PROG < <ft C.11 - NCSR.11 <-fi <-ju <ws \;
	    \2 <rin -2i ... >rin \2 >ws >-ju >-fi >ft >
<def TABL < \2 ... \2 >
<def REF < \need 4 ... \2  >

<text

<CHAP < >

<tabs 1.1i

<TABL
< Title:   \tf A highly-stable wide-range voltage-controlled oscillator >
>TABL

\; <ce \tr b"/usr/fisher/bitmaps/yso" >ce

<TABL
< Author:  \tf Anthony J. Fisher >
>TABL

<TABL
< Address: \tf Department of Computer Science, The University of York,
	       York YO1 5DD, U.K. >
>TABL

>tabs

<tr /a caxB	   \; a acute
<tr /S cSxOr	   \; S hacek

\4

<ft NCSR.10 NCSI.10 -
< Smetana---^/^S^/^a^r^k^a from ^M^/^a ^V^l^a^s^t > \1
>ft

\4

<ft NCSR.18 NCSI.18 -
< Smetana---^/^S^/^a^r^k^a from ^M^/^a ^V^l^a^s^t > \1
>ft

\4

<ft NCSR.24 NCSI.24 -
< Smetana---^/^S^/^a^r^k^a from ^M^/^a ^V^l^a^s^t > \1
>ft

>tr >tr

>CHAP

<CHAP < A highly-stable wide-range \0 voltage-controlled oscillator >

<SECT < 1| > < Introduction >

{ There has always been a compromise between the range of frequency
adjustment possible in a voltage-controlled oscillator (VCO) and the
stability of the centre frequency with changes in temperature, supply
voltage, and other parameters.	For example, the well-known 74HC4046
device/1 has an adjustment range which extends to the limits of operation
of the device (i.e.  up to +-100% of the centre frequency), but the
stability of the centre frequency with temperature is typically as poor as
+-1500_ppm_K/-/-/1.  On the other hand, a crystal-controlled
VCO can have a frequency stability of +-0.2_ppm_K/-/-/1, but to achieve
this degree of stability, the adjustment range is typically
limited to +-0.01% of the centre frequency/2. }

{ An ideal VCO would have both a wide adjustment range and a highly
stable, repeatable frequency ^v^s^. voltage characteristic.  The oscillator
described here goes some way towards achieving both
ideals.	 It was designed for use in a phase-locked loop, in which the
absolute stability of the frequency at the extremes of the control
voltage was not important, but the centre frequency (i.e. the frequency
when the control voltage is zero) had to be tightly controlled, or else
the loop would fail to lock. }

>SECT

<SECT < 2 > < How it works >

{ Figure 1 shows a block diagram of the oscillator.  It has as its core
a conventional wide-range VCO whose output frequency ^f`x is stabilized at
one particular value of ^V`i`n (^v^i^z^. ^V`i`n_=_0) by reference to a
crystal-controlled standard frequency ^f\0. }

{ Figure 2 shows the typical frequency ^v^s^. voltage characteristic of the
oscillator shown in figure 1, in which non-linearities are exaggerated.
In an ideal oscillator, the characteristic lies entirely within the first
and third quadrants of the graph.  When, because of imperfections in the
VCO core, the characteristic strays into the second or fourth quadrant, an
appropriate correction is applied to the input of the core VCO, reducing
the frequency error.  The correction is derived by logic from the output
of a frequency comparator, which produces an output ^F which is true when
^f`x_>_^f\0, and a voltage comparator, whose output ^P is true when ^V`i`n_>_0.
When the condition ^P_/\_~^F is true, the core VCO is running ``slow'', and a
positive pulse is applied at point ^Y.  When the condition ~^P_/\_^F is true,
the core VCO is running ``fast'', and a negative pulse is applied at
point ^Y.  The other two logical combinations of ^P and ^F correspond to
operation in the first or third quadrants, when both diodes are
reverse-biased. }

{ In order for this technique to work, the characteristic must enter the
second or fourth quadrants sufficiently often for the error to be reduced
to the desired level.  This can be ensured by stipulating that ^V`i`n must
pass through zero sufficiently often during normal operation of
the VCO.  The precise meaning of ``sufficiently often'' depends on the
stability of the core VCO, the leakage of the charge on ^C, and the
accuracy required; for the application for which the VCO was designed, one
transition every 100_ms or so was found to be quite enough. }

{ Assuming a perfect frequency comparator which produces a logic output ^F
equal to the instantaneous value of the Boolean expression ^f`x_>_^f\0, the
loop is a first-order one,
whose output cannot oscillate or overshoot, and the magnitude of
^f`x(0)_--_^f\0 can be made arbitrarily small (where ^f`x(0) denotes the value of
^f`x when ^V`i`n_=_0).  A perfect frequency comparator is a theoretical
impossibility, however, since the frequency of a signal is not
well defined at an instant; the best that can be done is to compare the
instantaneous phase of the two input signals ^f`x and ^f\0, by using (for
example) a type II phase comparator with a capacitor on the output (so that
its output is always at a well-defined logic level).  Such a device
functions as a frequency comparator, giving a logic output which depends
on the sign of ^f`x_--_^f\0 as required, but its output is subject to a delay
of the order of one cycle of ^f\0.  In the ^s-domain, its transfer function
has a term in 1/^s, since it is really comparing phase, not frequency, and
phase is the integral of frequency.  The loop as a whole is therefore in
reality second-order, and the output ^f`x will overshoot when a correction
is applied/3. }

{ In a normal second-order
phase-locked loop this would not matter; the output would
oscillate about the final value, but would eventually stabilize.  In this
loop, however, as soon as the output frequency overshoots, the diodes
become reverse-biased and the loop becomes open; the overshoot is not
corrected until the next zero-crossing of ^V`i`n, when the output overshoots
in the opposite direction.  Accuracy is seriously impaired. }

{ To make the circuit work, it is necessary to ensure that the loop is
heavily damped by carefully choosing the value of ^R\2.
In conventional phase-locked-loop terminology, the damping factor /z must
be large.  Unfortunately, the optimum values of /z and ^R\2 depend on
d^V`i`n_/_d^t, and there are no best values for all possible input signals.
The common rule-of-thumb for phase-locked loops of choosing ^R\2_=_^R\1_/_10
has been found to work surprisingly well in practice, however. }

>SECT

<SECT < 3 > < A practical implementation >

{ Figure 3 shows a practical implementation.  The output frequency range
is 4_MHz_+-66_kHz, for an input voltage range of +-5_V.	 The value of
|^f`x(0)_--_^f\0| is about 400_Hz, which gives an overall stability
of the centre frequency of about
+-100_ppm, almost independent of temperature and supply voltage.  Over a
temperature range of 20_K, this figure gives a stability of about
+-5_ppm_K/-/-/1, an improvement of about 300 times over the plain 74HC4046 on
which the circuit is based. }

>SECT

\5

<SECT < > < References >

<REF
< 1.  \tf Philips technical handbook IC06: High-speed CMOS logic
	  74HC/HCT family.  Philips Components Ltd (1991). >
>REF

<REF
< 2.  \tf Short-form catalogue.	 Vectron Crystal Oscillators Ltd (1992). >
>REF

<REF
< 3.  \tf F.M. Gardner, Phaselock techniques, Wiley (1966). >
>REF

>SECT

>CHAP

