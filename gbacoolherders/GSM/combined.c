//
// (C) 2004 Mike Brent aka Tursi aka HarmlessLion.com
// This software is provided AS-IS. No warranty
// express or implied is provided.
//
// This notice defines the entire license for this code.
// All rights not explicity granted here are reserved by the
// author.
//
// You may redistribute this software provided the original
// archive is UNCHANGED and a link back to my web page,
// http://harmlesslion.com, is provided as the author's site.
// It is acceptable to link directly to a subpage at harmlesslion.com
// provided that page offers a URL for that purpose
//
// Source code, if available, is provided for educational purposes
// only. You are welcome to read it, learn from it, mock
// it, and hack it up - for your own use only.
//
// Please contact me before distributing derived works or
// ports so that we may work out terms. I don't mind people
// using my code but it's been outright stolen before. In all
// cases the code must maintain credit to the original author(s).
//
// -COMMERCIAL USE- Contact me first. I didn't make
// any money off it - why should you? ;) If you just learned
// something from this, then go ahead. If you just pinched
// a routine or two, let me know, I'll probably just ask
// for credit. If you want to derive a commercial tool
// or use large portions, we need to talk. ;)
//
// If this, itself, is a derived work from someone else's code,
// then their original copyrights and licenses are left intact
// and in full force.
//
// http://harmlesslion.com - visit the web page for contact info
//
/* Derived by Tursi from code by:
 * Copyright 1992 by Jutta Degener and Carsten Bormann, Technische
 * Universitaet Berlin. THERE IS ABSOLUTELY NO WARRANTY FOR THIS SOFTWARE.
 * Note: Tons of code has been ripped out for speed - this may not work
 * well on samples other than the one it's been tested with ;)
 */

#include <string.h>		/* for memory funcs  */
#include <malloc.h>     /* for malloc in gsm_create */
#include "..\tursigb.h"
#include "gsm_short.h"
#include "random.h"

/* private.h */
typedef short				word;		/* 16 bit signed int	*/
typedef long 				longword;	/* 32 bit signed int	*/

typedef unsigned short		uword;		/* unsigned word	*/
typedef unsigned long		ulongword;	/* unsigned longword	*/

//#define GSM_TIMING

struct gsm_state {
	word		dp0[ 280 ];
	word		LARpp[2][8]; 	/*                              */
	word		j;				/*                              */
	word		nrp; /* 40 */	/* long_term.c, synthesis	*/
	word		v[9];			/* short_term.c, synthesis	*/
	word		msr;			/* decoder.c,	Postprocessing	*/
} sGSM;

signed char AudioBuf[160*2+10] __attribute__ ((__aligned__(32)));    // two buffers long, plus slack
volatile int nNextBlock;        // Next audio block, used for sync with the music
const char *pDat;               // Audio data pointer for process_decode()
int nBank=160;                  // Bank number for process_decode() (start offset so bank 0 is filled first)
uword AudioSwitch=0;            // Until enabled, this keeps the player off
int nGSMError=0;                // Set to 1 if decode fails

int DEEMPHASIS=DEFAULTDEEMPHASIS;

#ifdef GSM_TIMING
    int nGSMDecodeTime;
    int nGSMStopLine;
    int nGSMStartLine;
#endif

#define	MIN_WORD	(-32767 - 1)
#define	MAX_WORD	  32767

#define	MIN_LONGWORD	(-2147483647 - 1)
#define	MAX_LONGWORD	  2147483647

#define	SASR(x, by)	((x) >> (by))

#define GSM_MULT_R(a, b) /* word a, word b, !(a == b == MIN_WORD) */	\
	(SASR( ((longword)(a) * (longword)(b)), 15 ))

#define	GSM_ADD(a, b)	\
    ((a)+(b))

# define GSM_SUB(a, b)	\
    ((a)-(b))

#define GSM_ASL(a, n) \
    ((a)<<(n))

#define GSM_ASR(a, n) \
    ((a)>>(n))

extern void CODE_IN_IWRAM FastIntDivide (s32 Numer, s32 Denom, s32 *Result, s32 *Remainder);

void Gsm_Decoder(unsigned char *f) CODE_IN_IWRAM;
void Gsm_Long_Term_Synthesis_Filtering(word Ncr, word bcr, word *erp, word *drp) CODE_IN_IWRAM;
void Gsm_RPE_Decoding(word xmaxcr, word Mcr, word *xMcr, word *erp) CODE_IN_IWRAM;
void Gsm_Short_Term_Synthesis_Filter(word *LARcr, word *drp) CODE_IN_IWRAM;
void Postprocessing(unsigned char *f) CODE_IN_IWRAM;
void RPE_grid_positioning(word Mc, word *xMp, word *ep) CODE_IN_IWRAM;
void APCM_quantization_xmaxc_to_exp_mant(word xmaxc, word *exp_out, word *mant_out) CODE_IN_IWRAM;
void APCM_inverse_quantization(word *xMc, word mant, word exp, word *xMp) CODE_IN_IWRAM;
void Decoding_of_the_coded_Log_Area_Ratios(word *LARc, word *LARpp) CODE_IN_IWRAM;
void Coefficients_0_12(word *LARpp_j_1, word *LARpp_j, word *LARp) CODE_IN_IWRAM;
void Coefficients_13_26(word *LARpp_j_1, word *LARpp_j, word *LARp) CODE_IN_IWRAM;
void Coefficients_27_39(word *LARpp_j_1, word *LARpp_j, word *LARp) CODE_IN_IWRAM;
void Coefficients_40_159(word *LARpp_j, word * LARp) CODE_IN_IWRAM;
void LARp_to_rp(word * LARp) CODE_IN_IWRAM;
void Short_term_synthesis_filtering(word *rrp, int k, word *wt, word *sr) CODE_IN_IWRAM;

word LARc[8], Nc[4], Mc[4], bc[4], xmaxc[4], xmc[13*4];
gsm_signal	work[160];

// These tables are deliberably NOT const so they will be loaded in RAM
/* ************ Tables ****************** */
/*  Table 4.1  Quantization of the Log.-Area Ratios
 */
/* i 		     1      2      3        4      5      6        7       8 */
word gsm_A[8]   = {20480, 20480, 20480,  20480,  13964,  15360,   8534,  9036};
word gsm_B[8]   = {    0,     0,  2048,  -2560,     94,  -1792,   -341, -1144} ;
word gsm_MIC[8] = { -32,   -32,   -16,    -16,     -8,     -8,     -4,    -4 } ;
word gsm_MAC[8] = {  31,    31,    15,     15,      7,      7,      3,     3 } ;

/*  Table 4.2  Tabulation  of 1/A[1..8]
 */
word gsm_INVA[8]={ 13107, 13107,  13107, 13107,  19223, 17476,  31454, 29708 } ;

/*   Table 4.3a  Decision level of the LTP gain quantizer
 */
/*  bc		      0	        1	  2	     3			*/
word gsm_DLB[4] = {  6554,    16384,	26214,	   32767	} ;

/*   Table 4.3b   Quantization levels of the LTP gain quantizer
 */
/* bc		      0          1        2          3			*/
word gsm_QLB[4] = {  3277,    11469,	21299,	   32767	} ;

/*   Table 4.4	 Coefficients of the weighting filter
 */
/* i		    0      1   2    3   4      5      6     7   8   9    10  */
word gsm_H[11] = {-134, -374, 0, 2054, 5741, 8192, 5741, 2054, 0, -374, -134 } ;

/*   Table 4.5 	 Normalized inverse mantissa used to compute xM/xmax 
 */
/* i		 	0        1    2      3      4      5     6      7   */
word gsm_NRFAC[8] = { 29128, 26215, 23832, 21846, 20165, 18725, 17476, 16384 } ;

/*   Table 4.6	 Normalized direct mantissa used to compute xM/xmax
 */
/* i                  0      1       2      3      4      5      6      7   */
word gsm_FAC[8]	= { 18431, 20479, 22527, 24575, 26623, 28671, 30719, 32767 } ;

/* ************* Implementation ***************** */
/* Sets up the GB system as well, prepares timer 0 and 1, and Direct Sound A */
/* Timer 1 will be allowed to interrupt when turned on, but of course you */
/* need to enable those interrupts and handle them - calling process_decode is enough */
gsm gsm_init (const char *pSample)
{
    // Set the de-emphasis to our adjusted default
    DEEMPHASIS=DEFAULTDEEMPHASIS;

    // Empty the buffer
    memset(AudioBuf, 0, sizeof(AudioBuf));
    
    // make sure Timers are off
    REG_TM0CNT = 0;
    REG_TM1CNT = 0;
    // make sure DMA channel 1 is turned off
    REG_DMA1CNT_H = 0;

	memset(&sGSM, 0, sizeof(sGSM));
	sGSM.nrp = 40;
	AudioSwitch=0;
	nGSMError=0;
	
    nNextBlock=0;       // Next audio block, used for sync with the music
    pDat=pSample;       // Audio data pointer for process_decode()
    nBank=160;          // Bank number for process_decode() (start offset so bank 0 is filled first)

    // enable and reset Direct Sound channel A, at full volume, using
    // Timer 0 to determine frequency
    // GSM player thus uses DSA, TM0 and TM1. It sets interrupts on TM1 for empty buffer
    REG_SOUNDCNT_H = SND_OUTPUT_RATIO_100 | // 100% sound output
                     DSA_OUTPUT_RATIO_100 | // 100% direct sound A output
                     DSA_OUTPUT_TO_BOTH |   // output Direct Sound A to both right and left speakers
                     DSA_TIMER0 |           // use timer 0 to determine the playback frequency of Direct Sound A
                     DSA_FIFO_RESET;        // reset the FIFO for Direct Sound A

    // Default to 16khz
    SetAudioFreq(FREQUENCY_16);
    // timer 1 will cascade on timer 0 and enable it to trigger every time we finish a buffer
    REG_TM1D   = BLOCKSIZE_T;
    // init the DMA transfer
    REG_DMA1SAD = (u32)AudioBuf;
    REG_DMA1DAD = (u32)REG_FIFO_A;

    // Enable both timers, timer 1 first
    // Timer 1 throws an interrupt to indicate it's time to
    // refill the buffer. We don't need to worry about the
    // end of the song, because the GSM decoder will simply
    // fail at the end of the data and fill the buffers with
    // silence.
    REG_TM1CNT = TIMER_CASCADE|TIMER_ENABLED|TIMER_INTERRUPT;
    REG_TM0CNT = TIMER_ENABLED;
    // Hold off on starting audio till we're told to

	return &sGSM;
}

int CODE_IN_IWRAM gsm_decode (gsm_byte * c, unsigned char * target)
{
    /* Shift the constants, so the compiler can do it */
    if (((*c) & (0x0F<<4)) != GSM_MAGIC<<4) return -1;

	LARc[0]  = (*c++ & 0xF) << 2;		/* 1 */
	LARc[0] |= (*c >> 6) & 0x3;
	LARc[1]  = *c++ & 0x3F;
	LARc[2]  = (*c >> 3) & 0x1F;
	LARc[3]  = (*c++ & 0x7) << 2;
	LARc[3] |= (*c >> 6) & 0x3;
	LARc[4]  = (*c >> 2) & 0xF;
	LARc[5]  = (*c++ & 0x3) << 2;
	LARc[5] |= (*c >> 6) & 0x3;
	LARc[6]  = (*c >> 3) & 0x7;
	LARc[7]  = *c++ & 0x7;
	Nc[0]  = (*c >> 1) & 0x7F;
	bc[0]  = (*c++ & 0x1) << 1;
	bc[0] |= (*c >> 7) & 0x1;
	Mc[0]  = (*c >> 5) & 0x3;
	xmaxc[0]  = (*c++ & 0x1F) << 1;
	xmaxc[0] |= (*c >> 7) & 0x1;
	xmc[0]  = (*c >> 4) & 0x7;
	xmc[1]  = (*c >> 1) & 0x7;
	xmc[2]  = (*c++ & 0x1) << 2;
	xmc[2] |= (*c >> 6) & 0x3;
	xmc[3]  = (*c >> 3) & 0x7;
	xmc[4]  = *c++ & 0x7;
	xmc[5]  = (*c >> 5) & 0x7;
	xmc[6]  = (*c >> 2) & 0x7;
	xmc[7]  = (*c++ & 0x3) << 1;		/* 10 */
	xmc[7] |= (*c >> 7) & 0x1;
	xmc[8]  = (*c >> 4) & 0x7;
	xmc[9]  = (*c >> 1) & 0x7;
	xmc[10]  = (*c++ & 0x1) << 2;
	xmc[10] |= (*c >> 6) & 0x3;
	xmc[11]  = (*c >> 3) & 0x7;
	xmc[12]  = *c++ & 0x7;
	Nc[1]  = (*c >> 1) & 0x7F;
	bc[1]  = (*c++ & 0x1) << 1;
	bc[1] |= (*c >> 7) & 0x1;
	Mc[1]  = (*c >> 5) & 0x3;
	xmaxc[1]  = (*c++ & 0x1F) << 1;
	xmaxc[1] |= (*c >> 7) & 0x1;
	xmc[13]  = (*c >> 4) & 0x7;
	xmc[14]  = (*c >> 1) & 0x7;
	xmc[15]  = (*c++ & 0x1) << 2;
	xmc[15] |= (*c >> 6) & 0x3;
	xmc[16]  = (*c >> 3) & 0x7;
	xmc[17]  = *c++ & 0x7;
	xmc[18]  = (*c >> 5) & 0x7;
	xmc[19]  = (*c >> 2) & 0x7;
	xmc[20]  = (*c++ & 0x3) << 1;
	xmc[20] |= (*c >> 7) & 0x1;
	xmc[21]  = (*c >> 4) & 0x7;
	xmc[22]  = (*c >> 1) & 0x7;
	xmc[23]  = (*c++ & 0x1) << 2;
	xmc[23] |= (*c >> 6) & 0x3;
	xmc[24]  = (*c >> 3) & 0x7;
	xmc[25]  = *c++ & 0x7;
	Nc[2]  = (*c >> 1) & 0x7F;
	bc[2]  = (*c++ & 0x1) << 1;		/* 20 */
	bc[2] |= (*c >> 7) & 0x1;
	Mc[2]  = (*c >> 5) & 0x3;
	xmaxc[2]  = (*c++ & 0x1F) << 1;
	xmaxc[2] |= (*c >> 7) & 0x1;
	xmc[26]  = (*c >> 4) & 0x7;
	xmc[27]  = (*c >> 1) & 0x7;
	xmc[28]  = (*c++ & 0x1) << 2;
	xmc[28] |= (*c >> 6) & 0x3;
	xmc[29]  = (*c >> 3) & 0x7;
	xmc[30]  = *c++ & 0x7;
	xmc[31]  = (*c >> 5) & 0x7;
	xmc[32]  = (*c >> 2) & 0x7;
	xmc[33]  = (*c++ & 0x3) << 1;
	xmc[33] |= (*c >> 7) & 0x1;
	xmc[34]  = (*c >> 4) & 0x7;
	xmc[35]  = (*c >> 1) & 0x7;
	xmc[36]  = (*c++ & 0x1) << 2;
	xmc[36] |= (*c >> 6) & 0x3;
	xmc[37]  = (*c >> 3) & 0x7;
	xmc[38]  = *c++ & 0x7;
	Nc[3]  = (*c >> 1) & 0x7F;
	bc[3]  = (*c++ & 0x1) << 1;
	bc[3] |= (*c >> 7) & 0x1;
	Mc[3]  = (*c >> 5) & 0x3;
	xmaxc[3]  = (*c++ & 0x1F) << 1;
	xmaxc[3] |= (*c >> 7) & 0x1;
	xmc[39]  = (*c >> 4) & 0x7;
	xmc[40]  = (*c >> 1) & 0x7;
	xmc[41]  = (*c++ & 0x1) << 2;
	xmc[41] |= (*c >> 6) & 0x3;
	xmc[42]  = (*c >> 3) & 0x7;
	xmc[43]  = *c++ & 0x7;			/* 30  */
	xmc[44]  = (*c >> 5) & 0x7;
	xmc[45]  = (*c >> 2) & 0x7;
	xmc[46]  = (*c++ & 0x3) << 1;
	xmc[46] |= (*c >> 7) & 0x1;
	xmc[47]  = (*c >> 4) & 0x7;
	xmc[48]  = (*c >> 1) & 0x7;
	xmc[49]  = (*c++ & 0x1) << 2;
	xmc[49] |= (*c >> 6) & 0x3;
	xmc[50]  = (*c >> 3) & 0x7;
	xmc[51]  = *c & 0x7;			/* 33 */

	Gsm_Decoder(target);

	return 0;
}

word	erp[40], wt[160];
void CODE_IN_IWRAM Gsm_Decoder(unsigned char *f)
{
	int		j, k, idx;
	word*	drp = sGSM.dp0 + 120;

	word *LARcr=LARc;
    word *Ncr=Nc;
    word *bcr=bc;
    word *Mcr=Mc;
    word *xmaxcr=xmaxc;
    word *xMcr=xmc;

	/* Calculate j offset outside of this loop */
	idx = 0;
	for (j=0; j <= 3; j++, xmaxcr++, bcr++, Ncr++, Mcr++, xMcr += 13) {

		Gsm_RPE_Decoding(*xmaxcr, *Mcr, xMcr, erp );
        Gsm_Long_Term_Synthesis_Filtering(*Ncr, *bcr, erp, drp );

		for (k = 0; k <= 39; k++) wt[ idx++ ] =  drp[ k ];
	}

	Gsm_Short_Term_Synthesis_Filter(LARcr, wt);
	Postprocessing(f);
}

word	xMp[ 13 ];
void CODE_IN_IWRAM Gsm_RPE_Decoding(	word 		xmaxcr,
						word		Mcr,
						word		* xMcr,		/* [0..12], 3 bits 	IN	*/
						word		* erp		/* [0..39]			OUT */
					)
{
	word	exp, mant;

	APCM_quantization_xmaxc_to_exp_mant( xmaxcr, &exp, &mant );
	APCM_inverse_quantization( xMcr, mant, exp, xMp );
	RPE_grid_positioning( Mcr, xMp, erp );
}

void CODE_IN_IWRAM APCM_quantization_xmaxc_to_exp_mant (	word	xmaxc,		/* IN 	*/
											word	* exp_out,	/* OUT	*/
											word	* mant_out 	/* OUT  */
										)
{
	word	exp, mant;
    static const word outmant[8] = { 7, 7, 3, 7, 1, 3, 5, 7 };
    static const word outexp[8]  = { 0, 4, 3, 3, 2, 2, 2, 2 };
	/* Compute exponent and mantissa of the decoded version of xmaxc
	 */

 #if 0
	exp = 0;
	if (xmaxc > 15) exp = SASR(xmaxc, 3) - 1;
	mant = xmaxc - (exp << 3);

	if (mant == 0) {
		exp  = -4;
		mant = 7;
	}
	else {
		while (mant <= 7) {
			mant = mant << 1 | 1;
			exp--;
		}
		mant -= 8;
	}
	*exp_out  = exp;
	*mant_out = mant;
#endif

	if (xmaxc > 15) {
      exp = SASR(xmaxc, 3) - 1;
    } else {
      exp = 0;
    }
    
	mant = xmaxc - (exp << 3);
	
    /* where mant must be from 0 - 7, we can use a table instead of a loop */
    if (mant <= 7) {
      *mant_out=outmant[mant];    /* do we need to mask this? */
      if (mant == 0) {
          exp = -4;
      } else {
          exp-=outexp[mant];
      }
    } else {
      mant -= 8;
      *mant_out = mant;
    }

	*exp_out  = exp;
}

void CODE_IN_IWRAM APCM_inverse_quantization (	word	* xMc,	/* [0..12]			IN 	*/
									word		mant,
									word		exp,
									word	* xMp	/* [0..12]			OUT 	*/
								)
/* 
 *  This part is for decoding the RPE sequence of coded xMc[0..12]
 *  samples to obtain the xMp[0..12] array.  Table 4.6 is used to get
 *  the mantissa of xmaxc (FAC[0..7]).
 */
{
	int	i;
	word	temp, temp1, temp2, temp3, temp4;

	temp1 = gsm_FAC[ mant ];	/* see 4.2-15 for mant */
	temp2 = GSM_SUB( 6, exp );	/* see 4.2-15 for exp  */
	temp4 = GSM_SUB( temp2, 1 );	/* broken out by Tursi for the macro */
	temp3 = GSM_ASL( 1, temp4 );

	for (i = 13; i>0; i--) {
		temp = (*xMc++ << 1) - 7;	/* restore sign   */

		temp <<= 12;				/* 16 bit signed  */
		temp = GSM_MULT_R( temp1, temp );
		temp = GSM_ADD( temp, temp3 );
		*xMp++ = GSM_ASR( temp, temp2 );
	}
}

void CODE_IN_IWRAM RPE_grid_positioning	(	word			Mc,		/* grid position	IN	*/
								word	* xMp,	/* [0..12]			IN	*/
								word	* ep	/* [0..39]			OUT	*/
							)
/*
 *  This procedure computes the reconstructed long term residual signal
 *  ep[0..39] for the LTP analysis filter.  The inputs are the Mc
 *  which is the grid position selection and the xMp[0..12] decoded
 *  RPE samples which are upsampled by a factor of 3 by inserting zero
 *  values.
 */
{
	/* Tursi's version */
	/* (seems to work better, got a few extra cycles) */
	switch (Mc) {
		case 3: *ep++ = 0;
		case 2: *ep++ = 0;
		case 1: *ep++ = 0;
		case 0: *ep++ = *xMp++;
	}
	#if 0
    int i;
	for (i=12; i; i--) {
		*ep++ = 0;
		*ep++ = 0;
		*ep++ = *xMp++;
	}
	#endif
	/* manual loop unroll */
		*ep++ = 0;
		*ep++ = 0;
		*ep++ = *xMp++;
		*ep++ = 0;
		*ep++ = 0;
		*ep++ = *xMp++;
		*ep++ = 0;
		*ep++ = 0;
		*ep++ = *xMp++;
		*ep++ = 0;
		*ep++ = 0;
		*ep++ = *xMp++;
		*ep++ = 0;
		*ep++ = 0;
		*ep++ = *xMp++;
		*ep++ = 0;
		*ep++ = 0;
		*ep++ = *xMp++;
		*ep++ = 0;
		*ep++ = 0;
		*ep++ = *xMp++;
		*ep++ = 0;
		*ep++ = 0;
		*ep++ = *xMp++;
		*ep++ = 0;
		*ep++ = 0;
		*ep++ = *xMp++;
		*ep++ = 0;
		*ep++ = 0;
		*ep++ = *xMp++;
		*ep++ = 0;
		*ep++ = 0;
		*ep++ = *xMp++;
		*ep++ = 0;
		*ep++ = 0;
		*ep++ = *xMp++;
	/* End Tursi's version */

	/* This makes sure we write out however many padding bytes we need to reach 40 */
	while (++Mc < 4) *ep++ = 0;
}

void CODE_IN_IWRAM Gsm_Long_Term_Synthesis_Filtering	(	word				Ncr,
											word				bcr,
											word		* erp,	   /* [0..39]		  	 IN */
											word		* drp	   /* [-120..-1] IN, [-120..40] OUT */
										)
/*
 *  This procedure uses the bcr and Ncr parameter to realize the
 *  long term synthesis filtering.  The decoding of bcr needs
 *  table 4.3b.
 */
{
	int 		k;
	word				brp, drpp, Nr;

	/*  Check the limits of Nr.
	 */
	Nr = Ncr < 40 || Ncr > 120 ? sGSM.nrp : Ncr;
	sGSM.nrp = Nr;
    
	/*  Decoding of the LTP gain bcr
	 */
	brp = gsm_QLB[ bcr ];

	/*  Computation of the reconstructed short term residual 
	 *  signal drp[0..39]
	 */
	for (k = 0; k <= 39; k++) {
		drpp   = GSM_MULT_R( brp, drp[ k - Nr ] );
		drp[k] = GSM_ADD( erp[k], drpp );
	}

	/*
	 *  Update of the reconstructed short term residual signal
	 *  drp[ -1..-120 ]
	 */
	for (k = 0; k <= 119; k++) drp[ -120 + k ] = drp[ -80 + k ];
}

word		LARp[8];
void CODE_IN_IWRAM Gsm_Short_Term_Synthesis_Filter	(	word	* LARcr,	/* received log area ratios [0..7]	IN  */
											word	* wt		/* received d [0..159]				IN  */
										)
{
	word		* LARpp_j	= sGSM.LARpp[ sGSM.j     ];
	word		* LARpp_j_1	= sGSM.LARpp[ sGSM.j ^=1 ];
	word        *s          = work;

	Decoding_of_the_coded_Log_Area_Ratios( LARcr, LARpp_j );

	Coefficients_0_12( LARpp_j_1, LARpp_j, LARp );
	LARp_to_rp( LARp );
	Short_term_synthesis_filtering( LARp, 13, wt, s );

	Coefficients_13_26( LARpp_j_1, LARpp_j, LARp);
	LARp_to_rp( LARp );
	Short_term_synthesis_filtering(LARp, 14, wt + 13, s + 13 );

	Coefficients_27_39( LARpp_j_1, LARpp_j, LARp);
	LARp_to_rp( LARp );
	Short_term_synthesis_filtering(LARp, 13, wt + 27, s + 27 );

	Coefficients_40_159( LARpp_j, LARp );
	LARp_to_rp( LARp );
	Short_term_synthesis_filtering(LARp, 120, wt + 40, s + 40);
}

void CODE_IN_IWRAM Decoding_of_the_coded_Log_Area_Ratios	(	word	* LARc,		/* coded log area ratio	[0..7] 	IN	*/
														word	* LARpp		/* out: decoded ..			*/
                                            )
{
	word	temp1;

	/*  This procedure requires for efficient implementation
	 *  two tables.
 	 *
	 *  INVA[1..8] = integer( (32768 * 8) / real_A[1..8])
	 *  MIC[1..8]  = minimum value of the LARc[1..8]
	 */

	/*  Compute the LARpp[1..8]
	 */

#define	STEP( B, MIC, INVA )	\
		temp1    = GSM_ADD( *LARc++, MIC ) << 10;	\
		temp1    = GSM_SUB( temp1, B << 1 );		\
		temp1    = GSM_MULT_R( INVA, temp1 );		\
		*LARpp++ = GSM_ADD( temp1, temp1 );

	STEP(      0,  -32,  13107 );
	STEP(      0,  -32,  13107 );
	STEP(   2048,  -16,  13107 );
	STEP(  -2560,  -16,  13107 );

	STEP(     94,   -8,  19223 );
	STEP(  -1792,   -8,  17476 );
	STEP(   -341,   -4,  31454 );
	STEP(  -1144,   -4,  29708 );

	/* NOTE: the addition of *MIC is used to restore
	 * 	 the sign of *LARc.
	 */
}

/*
 *  Within each frame of 160 analyzed speech samples the short term
 *  analysis and synthesis filters operate with four different sets of
 *  coefficients, derived from the previous set of decoded LARs(LARpp(j-1))
 *  and the actual set of decoded LARs (LARpp(j))
 *
 * (Initial value: LARpp(j-1)[1..8] = 0.)
 */

void CODE_IN_IWRAM Coefficients_0_12	(	word * LARpp_j_1,
							word * LARpp_j,
							word * LARp
						)
{
#if 0
	int 	i;

	for (i = 1; i <= 8; i++, LARp++, LARpp_j_1++, LARpp_j++) {
		*LARp = GSM_ADD( SASR( *LARpp_j_1, 2 ), SASR( *LARpp_j, 2 ));
		*LARp = GSM_ADD( *LARp,  SASR( *LARpp_j_1, 1));
	}
#endif

	word   temp;

		temp = GSM_ADD( SASR( *LARpp_j_1, 2 ), SASR( *LARpp_j++, 2 ));
		*LARp++ = GSM_ADD( temp,  SASR( *LARpp_j_1++, 1));
		temp = GSM_ADD( SASR( *LARpp_j_1, 2 ), SASR( *LARpp_j++, 2 ));
		*LARp++ = GSM_ADD( temp,  SASR( *LARpp_j_1++, 1));
		temp = GSM_ADD( SASR( *LARpp_j_1, 2 ), SASR( *LARpp_j++, 2 ));
		*LARp++ = GSM_ADD( temp,  SASR( *LARpp_j_1++, 1));
		temp = GSM_ADD( SASR( *LARpp_j_1, 2 ), SASR( *LARpp_j++, 2 ));
		*LARp++ = GSM_ADD( temp,  SASR( *LARpp_j_1++, 1));
		temp = GSM_ADD( SASR( *LARpp_j_1, 2 ), SASR( *LARpp_j++, 2 ));
		*LARp++ = GSM_ADD( temp,  SASR( *LARpp_j_1++, 1));
		temp = GSM_ADD( SASR( *LARpp_j_1, 2 ), SASR( *LARpp_j++, 2 ));
		*LARp++ = GSM_ADD( temp,  SASR( *LARpp_j_1++, 1));
		temp = GSM_ADD( SASR( *LARpp_j_1, 2 ), SASR( *LARpp_j++, 2 ));
		*LARp++ = GSM_ADD( temp,  SASR( *LARpp_j_1++, 1));
		temp = GSM_ADD( SASR( *LARpp_j_1, 2 ), SASR( *LARpp_j++, 2 ));
		*LARp++ = GSM_ADD( temp,  SASR( *LARpp_j_1++, 1));
	
}

void CODE_IN_IWRAM Coefficients_13_26	(	word * LARpp_j_1,
							word * LARpp_j,
							word * LARp
						) 
{
    #if 0
	int i;
	for (i = 1; i <= 8; i++, LARpp_j_1++, LARpp_j++, LARp++) {
		*LARp = GSM_ADD( SASR( *LARpp_j_1, 1), SASR( *LARpp_j, 1 ));
	}
	#endif

		*LARp++ = GSM_ADD( SASR( *LARpp_j_1++, 1), SASR( *LARpp_j++, 1 ));
		*LARp++ = GSM_ADD( SASR( *LARpp_j_1++, 1), SASR( *LARpp_j++, 1 ));
		*LARp++ = GSM_ADD( SASR( *LARpp_j_1++, 1), SASR( *LARpp_j++, 1 ));
		*LARp++ = GSM_ADD( SASR( *LARpp_j_1++, 1), SASR( *LARpp_j++, 1 ));
		*LARp++ = GSM_ADD( SASR( *LARpp_j_1++, 1), SASR( *LARpp_j++, 1 ));
		*LARp++ = GSM_ADD( SASR( *LARpp_j_1++, 1), SASR( *LARpp_j++, 1 ));
		*LARp++ = GSM_ADD( SASR( *LARpp_j_1++, 1), SASR( *LARpp_j++, 1 ));
		*LARp++ = GSM_ADD( SASR( *LARpp_j_1++, 1), SASR( *LARpp_j++, 1 ));
	
}

void CODE_IN_IWRAM Coefficients_27_39	(	word * LARpp_j_1,
							word * LARpp_j,
							word * LARp
						) 
{
#if 0
	int i;
	for (i = 1; i <= 8; i++, LARpp_j_1++, LARpp_j++, LARp++) {
		*LARp = GSM_ADD( SASR( *LARpp_j_1, 2 ), SASR( *LARpp_j, 2 ));
		*LARp = GSM_ADD( *LARp, SASR( *LARpp_j, 1 ));
	}
#endif
    word temp;
    
		temp = GSM_ADD( SASR( *LARpp_j_1++, 2 ), SASR( *LARpp_j, 2 ));
		*LARp++ = GSM_ADD( temp, SASR( *LARpp_j++, 1 ));
		temp = GSM_ADD( SASR( *LARpp_j_1++, 2 ), SASR( *LARpp_j, 2 ));
		*LARp++ = GSM_ADD( temp, SASR( *LARpp_j++, 1 ));
		temp = GSM_ADD( SASR( *LARpp_j_1++, 2 ), SASR( *LARpp_j, 2 ));
		*LARp++ = GSM_ADD( temp, SASR( *LARpp_j++, 1 ));
		temp = GSM_ADD( SASR( *LARpp_j_1++, 2 ), SASR( *LARpp_j, 2 ));
		*LARp++ = GSM_ADD( temp, SASR( *LARpp_j++, 1 ));
		temp = GSM_ADD( SASR( *LARpp_j_1++, 2 ), SASR( *LARpp_j, 2 ));
		*LARp++ = GSM_ADD( temp, SASR( *LARpp_j++, 1 ));
		temp = GSM_ADD( SASR( *LARpp_j_1++, 2 ), SASR( *LARpp_j, 2 ));
		*LARp++ = GSM_ADD( temp, SASR( *LARpp_j++, 1 ));
		temp = GSM_ADD( SASR( *LARpp_j_1++, 2 ), SASR( *LARpp_j, 2 ));
		*LARp++ = GSM_ADD( temp, SASR( *LARpp_j++, 1 ));
		temp = GSM_ADD( SASR( *LARpp_j_1++, 2 ), SASR( *LARpp_j, 2 ));
		*LARp++ = GSM_ADD( temp, SASR( *LARpp_j++, 1 ));

}

void CODE_IN_IWRAM Coefficients_40_159	(	word * LARpp_j,
								word * LARp
							) 
{
#if 0
	int i;
	for (i = 1; i <= 8; i++, LARp++, LARpp_j++)
		*LARp = *LARpp_j;
#endif

    *LARp++ = *LARpp_j++;
    *LARp++ = *LARpp_j++;
    *LARp++ = *LARpp_j++;
    *LARp++ = *LARpp_j++;
    *LARp++ = *LARpp_j++;
    *LARp++ = *LARpp_j++;
    *LARp++ = *LARpp_j++;
    *LARp++ = *LARpp_j++;
}

void CODE_IN_IWRAM LARp_to_rp(word * LARp)	/* [0..7] IN/OUT  */
/*
 *  The input of this procedure is the interpolated LARp[0..7] array.
 *  The reflection coefficients, rp[i], are used in the analysis
 *  filter and in the synthesis filter.
 */
{
	word		temp;

#if 0
	int 		i;
	for (i = 1; i <= 8; i++, LARp++) {
		if (*LARp < 0) {
			temp = -(*LARp);
			*LARp = - ((temp < 11059) ? temp << 1
				: ((temp < 20070) ? temp + 11059
				:  GSM_ADD( temp >> 2, 26112 )));
		} else {
			temp  = *LARp;
			*LARp =    (temp < 11059) ? temp << 1
				: ((temp < 20070) ? temp + 11059
				:  GSM_ADD( temp >> 2, 26112 ));
		}
	}
#endif

	if (*LARp < 0) {
		temp = -(*LARp);
		*LARp = - ((temp < 11059) ? temp << 1
			: ((temp < 20070) ? temp + 11059
			:  GSM_ADD( temp >> 2, 26112 )));
	} else {
		temp  = *LARp;
		*LARp =    (temp < 11059) ? temp << 1
			: ((temp < 20070) ? temp + 11059
			:  GSM_ADD( temp >> 2, 26112 ));
	}
	LARp++;
	if (*LARp < 0) {
		temp = -(*LARp);
		*LARp = - ((temp < 11059) ? temp << 1
			: ((temp < 20070) ? temp + 11059
			:  GSM_ADD( temp >> 2, 26112 )));
	} else {
		temp  = *LARp;
		*LARp =    (temp < 11059) ? temp << 1
			: ((temp < 20070) ? temp + 11059
			:  GSM_ADD( temp >> 2, 26112 ));
	}
	LARp++;
	if (*LARp < 0) {
		temp = -(*LARp);
		*LARp = - ((temp < 11059) ? temp << 1
			: ((temp < 20070) ? temp + 11059
			:  GSM_ADD( temp >> 2, 26112 )));
	} else {
		temp  = *LARp;
		*LARp =    (temp < 11059) ? temp << 1
			: ((temp < 20070) ? temp + 11059
			:  GSM_ADD( temp >> 2, 26112 ));
	}
	LARp++;
	if (*LARp < 0) {
		temp = -(*LARp);
		*LARp = - ((temp < 11059) ? temp << 1
			: ((temp < 20070) ? temp + 11059
			:  GSM_ADD( temp >> 2, 26112 )));
	} else {
		temp  = *LARp;
		*LARp =    (temp < 11059) ? temp << 1
			: ((temp < 20070) ? temp + 11059
			:  GSM_ADD( temp >> 2, 26112 ));
	}
	LARp++;
	if (*LARp < 0) {
		temp = -(*LARp);
		*LARp = - ((temp < 11059) ? temp << 1
			: ((temp < 20070) ? temp + 11059
			:  GSM_ADD( temp >> 2, 26112 )));
	} else {
		temp  = *LARp;
		*LARp =    (temp < 11059) ? temp << 1
			: ((temp < 20070) ? temp + 11059
			:  GSM_ADD( temp >> 2, 26112 ));
	}
	LARp++;
	if (*LARp < 0) {
		temp = -(*LARp);
		*LARp = - ((temp < 11059) ? temp << 1
			: ((temp < 20070) ? temp + 11059
			:  GSM_ADD( temp >> 2, 26112 )));
	} else {
		temp  = *LARp;
		*LARp =    (temp < 11059) ? temp << 1
			: ((temp < 20070) ? temp + 11059
			:  GSM_ADD( temp >> 2, 26112 ));
	}
	LARp++;
	if (*LARp < 0) {
		temp = -(*LARp);
		*LARp = - ((temp < 11059) ? temp << 1
			: ((temp < 20070) ? temp + 11059
			:  GSM_ADD( temp >> 2, 26112 )));
	} else {
		temp  = *LARp;
		*LARp =    (temp < 11059) ? temp << 1
			: ((temp < 20070) ? temp + 11059
			:  GSM_ADD( temp >> 2, 26112 ));
	}
	LARp++;
	if (*LARp < 0) {
		temp = -(*LARp);
		*LARp = - ((temp < 11059) ? temp << 1
			: ((temp < 20070) ? temp + 11059
			:  GSM_ADD( temp >> 2, 26112 )));
	} else {
		temp  = *LARp;
		*LARp =    (temp < 11059) ? temp << 1
			: ((temp < 20070) ? temp + 11059
			:  GSM_ADD( temp >> 2, 26112 ));
	}
	LARp++;
}

void CODE_IN_IWRAM Short_term_synthesis_filtering  (
                                        word	* rrp,   /* [0..7]	    IN	*/
                                       	int	    k,	     /* k_end - k_start	*/
                                       	word	* wt,	 /* [0..k-1]	IN	*/
                                       	word	* sr	 /* [0..k-1]	OUT	*/
                                     ) 
{
	word		* v = sGSM.v;
/*	int		    i;*/
	word		sri, tmp1, tmp2;

	while (k--) {
		sri = *wt++;
#if 0
		for (i = 8; i--;) {
			tmp1 = rrp[i];
			tmp2 = v[i];
			tmp2 = GSM_MULT_R(tmp1, tmp2);
			sri  = GSM_SUB( sri, tmp2 );
            tmp1 = GSM_MULT_R(tmp1, sri);
			v[i+1] = GSM_ADD( v[i], tmp1);
		}
		*sr++ = v[0] = sri;
#else
        rrp+=7;
        v+=7;
		tmp2 = GSM_MULT_R((*(rrp)), (*v));
		sri  = GSM_SUB( sri, tmp2 );
        tmp1 = GSM_MULT_R((*(rrp--)), sri);
		*(v+1) = GSM_ADD( (*v), tmp1);
		v--;
		tmp2 = GSM_MULT_R((*(rrp)), (*v));
		sri  = GSM_SUB( sri, tmp2 );
        tmp1 = GSM_MULT_R((*(rrp--)), sri);
		*(v+1) = GSM_ADD( (*v), tmp1);
		v--;
		tmp2 = GSM_MULT_R((*(rrp)), (*v));
		sri  = GSM_SUB( sri, tmp2 );
        tmp1 = GSM_MULT_R((*(rrp--)), sri);
		*(v+1) = GSM_ADD( (*v), tmp1);
		v--;
		tmp2 = GSM_MULT_R((*(rrp)), (*v));
		sri  = GSM_SUB( sri, tmp2 );
        tmp1 = GSM_MULT_R((*(rrp--)), sri);
		*(v+1) = GSM_ADD( (*v), tmp1);
		v--;
		tmp2 = GSM_MULT_R((*(rrp)), (*v));
		sri  = GSM_SUB( sri, tmp2 );
        tmp1 = GSM_MULT_R((*(rrp--)), sri);
		*(v+1) = GSM_ADD( (*v), tmp1);
		v--;
		tmp2 = GSM_MULT_R((*(rrp)), (*v));
		sri  = GSM_SUB( sri, tmp2 );
        tmp1 = GSM_MULT_R((*(rrp--)), sri);
		*(v+1) = GSM_ADD( (*v), tmp1);
		v--;
		tmp2 = GSM_MULT_R((*(rrp)), (*v));
		sri  = GSM_SUB( sri, tmp2 );
        tmp1 = GSM_MULT_R((*(rrp--)), sri);
		*(v+1) = GSM_ADD( (*v), tmp1);
		v--;
		tmp2 = GSM_MULT_R((*(rrp)), (*v));
		sri  = GSM_SUB( sri, tmp2 );
        tmp1 = GSM_MULT_R((*(rrp)), sri);
		*(v+1) = GSM_ADD( (*v), tmp1);

		*sr++ = *v = sri;
#endif
	}
}

void CODE_IN_IWRAM Postprocessing	(unsigned char *f) 
{
	int		k;
	word		msr = sGSM.msr;
	word		tmp;
	word *s = work;

	for (k = 160; k--; ) {
		tmp = GSM_MULT_R( msr, DEEMPHASIS );   /* original: 28180. Lower=tinnier, higher=more bass but more clipping */
		msr = GSM_ADD(*(s++), tmp);  	       /* Deemphasis 	     */
        *(f++)=(char)(msr >> 7);               /* Final output - shifted to 8 bits */
    }
	sGSM.msr = msr;
}

// Fill the next audio buffer
// Currently works on 2x160byte buffers, in continuous memory
void CODE_IN_IWRAM process_decode()
{
    int idx;
    
    if (nBank) {
        nBank=0;
    } else {
        // Audio playback is always on the OTHER bank
        REG_DMA1CNT_H = 0;
        // Not necessary to rewrite this pointer so long as it hasn't been changed!
//        REG_DMA1SAD = (u32)AudioBuf;    // reset to beginning of buffer
        REG_DMA1CNT_H = AudioSwitch;
        nBank=160;
    }
    nNextBlock++;

    // Process GSM encoded audio
    // A gsm_frame is 33 unsigned bytes long
    // the resulting signal is 160 signed 16-bit words long (only high 13 bits relevant?)
    // But my hacked version outputs 8-bit words
#ifdef GSM_TIMING
    nGSMStartLine=VCOUNT;
#endif
    if (gsm_decode((gsm_byte*)pDat, &AudioBuf[nBank])) {
        // decode failed, write empty data
        for (idx=0; idx<160; idx++) {
            AudioBuf[nBank+idx]=0;
        }
        nGSMError=1;
        // Don't increment pointer any further
    } else {
        pDat+=sizeof(gsm_frame);
	}
#ifdef GSM_TIMING
    nGSMStopLine=VCOUNT;
    if (nGSMStopLine < nGSMStartLine) nGSMStopLine+=228;
    nGSMDecodeTime=nGSMStopLine-nGSMStartLine;

    // The following code is only good with the SAND2 demo
    // It reports 39-39 scanlines, or about 2.86ms per frame (on real hardware)
    // Each frame is 160 samples. At 16khz, that's 10ms of audio. Meaning that
    // currently, this code uses 28.6% of the CPU at 16khz
    // (VBA with HAM reports 45-46 lines, and VBA DirectX reports 30-31 lines)
    idx=0;
    SPR_SETY(idx, 0);
    SPR_SETX(idx, 0);
    SPR_SETTILE(idx, (((nGSMDecodeTime/100))<<1)+512);

    idx++;
    SPR_SETY(idx, 0);
    SPR_SETX(idx, 20);
    SPR_SETTILE(idx, (((nGSMDecodeTime%100/10))<<1)+512);
    
    idx++;
    SPR_SETY(idx, 0);
    SPR_SETX(idx, 40);
    SPR_SETTILE(idx, (((nGSMDecodeTime%10))<<1)+512);

#endif
}

void AudioOn() {
    AudioSwitch=ENABLE_DMA | START_ON_FIFO_EMPTY | WORD_DMA | DMA_REPEAT | DEST_REG_SAME;
    REG_DMA1CNT_H = AudioSwitch;
}

void AudioOff() {
    AudioSwitch=0;
    REG_DMA1CNT_H = AudioSwitch;
}

/* Random number stuff */
unsigned int seed;

/* Thanks to Albert Veli for this clever routine! */
void srand(unsigned int newseed)
{
	if (newseed) {
		seed=newseed;
	} else {
		seed=1;
	}
}

