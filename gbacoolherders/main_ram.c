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

#include "tursigb.h"
#include "gsm\gsm_short.h"

volatile u32 nFrame=0;

// Do an integer division using GBA BIOS fast division SWI code.
//  by Jeff F., 2002-Apr-27
// May not work correctly with vars on the stack?
void CODE_IN_IWRAM FastIntDivide (s32 Numer, s32 Denom, s32 *Result, s32 *Remainder)
{
   asm volatile
      (
      " mov   r0,%2   \n"
      " mov   r1,%3   \n"
      " swi   0x60000       \n"     // NOTE!!!!! Put 6 here for Thumb C Compiler mode.
      " ldr   r2,%0   \n"           //           Put 0x60000 there for ARM C Compiler mode.
      " str   r0,[r2] \n"
      " ldr   r2,%1   \n"
      " str   r1,[r2] \n"
      : "=m" (Result), "=m" (Remainder) // Outputs
      : "r" (Numer), "r" (Denom)        // Inputs
      : "r0","r1","r2","r3"             // Regs crushed & smushed
      );
}

// Handle a simple interrupt
void CODE_IN_IWRAM InterruptProcess(void) {
    register u32 tmp=REG_IF;

    // disable interrupts
    REG_IME = 0;

    if (tmp & INT_VBLANK) {
        nFrame++;
   	}
   	
    /* acknowledge to BIOS, for IntrWait() */
//    *(volatile unsigned short *)0x03fffff8 |= tmp;

    // clear *all* flagged ints except timer 1, which we check below (writing a 1 clears that bit)
    // When we clear the bits, those ints are allowed to trigger again   	
    REG_IF = ~INT_TIMER1;

    // Re-enable interrupts
    REG_IME = 1;

    // This one can take a while - we'll allow other ints now
	if (tmp & INT_TIMER1) {
        process_decode();
        // clear the interrupt only after we're done. If we were too slow,
        // we may have missed interrupts!
        REG_IF=INT_TIMER1;  // writing zeros is ignored
    }
}

