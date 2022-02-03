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
/* gsm.h */
#define DEFAULTDEEMPHASIS (28180)   /* original: 28180. Lower=tinnier, higher=more bass but more clipping */
extern int DEEMPHASIS;              /* 25380 for Sandstorm */

typedef struct gsm_state*	gsm;
typedef short				gsm_signal;		/* signed 16 bit */
typedef unsigned char		gsm_byte;
typedef gsm_byte 			gsm_frame[33];	/* 33 * 8 bits	 */

#define	GSM_MAGIC			0xD		  		/* 13 kbit/s RPE-LTP */
extern signed char AudioBuf[160*2+10];      /* two buffers long plus slack */
extern volatile int nNextBlock;             // Next audio block, used for sync with the music
extern const char *pDat;                    // Audio data pointer for process_decode()
extern int nBank;                           // Bank number for process_decode() (start offset so bank 0 is filled first)
extern int nGSMError;                       // 1 if last decode failed

gsm		gsm_init		(const char *pSample);
int		gsm_decode		(gsm_byte *, unsigned char *) CODE_IN_IWRAM;
void    process_decode  () CODE_IN_IWRAM;
void    AudioOn         ();
void    AudioOff        ();
void    SetAudioFreq    (int nFreq);

#define SetAudioFreq(nFreq) {       \
    REG_TM0D   = (nFreq);           \
}

// These values are just for history...
//#define FREQUENCY_T (0xFFFF - 1564)       // First pass - fastest possible (O2, fast switches)                [10727Hz]
//#define FREQUENCY_T (0xFFFF - 1546)       // Second pass - optimize Gsm_Decoder loop in decode.c              [10852Hz]
//#define FREQUENCY_T (0xFFFF - 1519)       // Third pass - remove redundant buffer copies, output 8 bit        [11045Hz]
//#define FREQUENCY_T (0xFFFF - 1494)       // Fourth pass - remove lib, strip code to basics, unroll loops     [11222Hz]
//#define FREQUENCY_T (0xFFFF - 724)        // Fifth pass - removal of clipping code (slightly noisier)         [23172Hz]
//#define FREQUENCY_T (0xFFFF - 383)        // Sixth pass - run from RAM, move stack arrays to global           [43812Hz]
//#define FREQUENCY_T (0xFFFF - 269)        // Seventh pass - retested (but no good on real hardware)           [62379Hz]
//#define FREQUENCY_T (0xFFFF - 350)        // Good on real hardware                                            [47942Hz]
//#define FREQUENCY_T (0xFFFF - 245)        // Eight pass - unroll short_term_synthesis_filtering (not for real)[68489Hz]
//#define FREQUENCY_T (0xFFFF - 333)        // Good on real hardware                                            [50390Hz]
// It's getting silly now, and I fear I'm hitting restrictions in the timer/hardware system. So now it's time to work on CPU use

// These are default tick counts...
#define FREQUENCY_44 (0xFFFF - 379)        // 44khz - works
#define FREQUENCY_22 (0xFFFF - 759)        // 22khz - works
#define FREQUENCY_16 (0xFFFF - 1048)       // 16khz - works
#define FREQUENCY_11 (0xFFFF - 1521)       // 11khz - works
#define FREQUENCY_8  (0xFFFF - 2047)       // 8khz  - works

// The GSM block is always 160 bytes
#define BLOCKSIZE_T (0xFFFF - 159)          // 160 byte blocks (-159 cause it triggers on overflow, not at 0xffff)

