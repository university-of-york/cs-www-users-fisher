<use ~/a4-11on14.fmt
<ft NCSR.11 NCSI.11 C.11
<tabs 4n 8n 12n

<tr `` x*   <tr '' x:	<tr fi x.   <tr fl x/
<tr -- x1	   \; en dash
<tr --- xP	   \; em dash
<tr $ x2	   \; dagger
<tr % x'	   \; section
<tr => fS.11x^	   \; right double arrow
<tr @ fNCSR.11x7   \; bullet
<tr [[ c{	   \; C left brace
<tr ]] c}	   \; C right brace

<foot 1 1 < \g -- # -- \g >

<def PARA < \1 \need 2 \h 1m ... \1 >
<def SUBS < <ft NCSI.11 - - \2 \need 4 \arg >ft \1 ... >
<def COMD < <ft NCSB.11 CBO.11 CB.11 \2 \need 4 \arg >ft \1 ... >
<def SECT < <ft NCSB.11 CBO.11 CB.11 \2 \need 4 \arg >ft \1 ... >
<def CHAP < \page <ft NCSB.11 - - <ce \arg >ce >ft \2 ... >
<def TOME < \page \line 13 <ft NCSB.24 - - <ce \arg >ce >ft \page ... >
<def PROG < <ft C.11 NCSI.11 NCSR.11
	    <-fi <-ju <ws ... \1 >ws >-ju >-fi >ft >
<def TABL < \1 ... \1 >
<def REF  < \need 3 \1 ... \1 >
<def RIB1 < <ls 1 <fn 3 < \t > < --------------------------- >
	    <rin 0i <ft NCSR.9 NCSI.9 C.9 ... >ft >rin >fn >ls >
<def RIB2 < <ls 1 <fn 4 < \t > < --------------------------- >
	    <rin 0i <ft NCSR.9 NCSI.9 C.9 ... >ft >rin >fn >ls >
<def RIB3 < <ls 1 <fn 5 < \t > < --------------------------- >
	    <rin 0i <ft NCSR.9 NCSI.9 C.9 ... >ft >rin >fn >ls >

<text

