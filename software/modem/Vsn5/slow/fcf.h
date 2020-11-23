/* Modem for MIPS   AJF	  January 1995
   Defns of facsimile control field bytes (T.30) */

#define DIS	0x01	/* digital identification signal       */
#define CSI	0x02	/* called subscriber identification    */
#define NSF	0x04	/* non-standard facilities	       */
#define CFR	0x21	/* confirmation to receive	       */
#define FTT	0x22	/* failure to train		       */
#define MCF	0x31	/* message confirmation		       */
#define RTN	0x32	/* retrain negative		       */
#define RTP	0x33	/* retrain positive		       */
#define XCN	0x5f	/* disconnect (from remote)	       */
#define DCS	0xc1	/* digital command signal	       */
#define TSI	0xc2	/* transmitting subscriber information */
#define DCN	0xdf	/* disconnect (to remote)	       */
#define MPS	0xf2	/* multipage signal		       */
#define EOP	0xf4	/* end of procedures		       */

