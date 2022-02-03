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


// Show Tursi, lightning bolt, harmlesslion notice
// Note that it leaves the display in mode 2 with stuff on BG2,
// and that the blending mode is set to darken BG2, fully dark.

#include <malloc.h>
#include <string.h>
#include "../tursigb.h"
#include "../gsm/gsm_short.h"

extern const char pThunder[];       // Sound effect
extern const u16 tp1_Palette[];     // shared palette
extern const u8 tp1_Map[];          // Image 1
extern const u8 tp2_Map[];          // Image 2
extern const u16 presented_gfx[2864];  // Presented by screen
extern const u16 presented_map[];   // Presented by map
extern const u16 presented_pal[];   // Presented by palette

#define VidBuf1 (u16*)0x6000000
#define VidBuf1b (u16*)0x6004b00
#define VidBuf2 (u16*)0x600A000
#define VidBuf2b (u16*)0x600eb00
#define PalBuf  (u16*)0x5000000
#define ShowPage1() REG_DISPCNT=REG_DISPCNT&0xffef; REG_COLEY=2;
#define ShowPage2() REG_DISPCNT=REG_DISPCNT|0x0010; REG_COLEY=0;

// This function doesn't use interrupts, to avoid stealing your interrupt pointers
// Ensure the screen is faded to black before you call this
void ShowTursi() {
    int idx, nState;
    register int fframe;

    // turn on the sound chip
    REG_SOUNDCNT_X = SND_ENABLED;
    // we don’t want to mess with sound channels 1-4
    REG_SOUNDCNT_L = 0;

    // Audio engine setup
	gsm_init(pThunder);     // Prepare the GSM decoder - defaults to 16khz
    process_decode();       // load the first buffer
    process_decode();       // load the second buffer

    // Ensure screen is black
    REG_BLDMOD= SOURCE_BG0 | SOURCE_BG1 | SOURCE_BG2 | SOURCE_BG3 | DARKEN;
    REG_COLEY=17;
    // Set up the video and two buffers - 8-bit bitmapped mode
    REG_DISPCNT= MODE4 | BG2_ENABLE;
    REG_DISPSTAT = 0;
    
    // Load our data while the screen's dark
    fastcopy16(PalBuf, tp1_Palette, 512);
    fastcopy16(VidBuf1, tp2_Map, 19200);        // Split up the 38k of data (max 32k)
    fastcopy16(VidBuf1b, tp2_Map+19200, 19200);
    fastcopy16(VidBuf2, tp1_Map, 19200);
    fastcopy16(VidBuf2b, tp1_Map+19200, 19200);

    // fade in *almost* fully
    REG_BLDMOD= SOURCE_BG2 | DARKEN;
    REG_COLEY=17;
    for (idx=32; idx>=5; idx--) {
        REG_COLEY=(u16)idx>>1;
        vsync();
    }

    // Reduce deemphasis to prevent clipping
    DEEMPHASIS=27000;
    AudioOn();

    nState=0;
    idx=32;
    fframe=false;
    for (;;) {
        if (REG_TM1D == (0xffff-159)) {
            process_decode();
        }

        if (nGSMError) {
            REG_COLEY=0xf;
            break;
        }
        if ((REG_KEYINPUT & (BTN_A|BTN_B|BTN_SELECT|BTN_START))!=(BTN_A|BTN_B|BTN_SELECT|BTN_START)) {
            REG_COLEY=0xf;
            break;
        }

        if (VCOUNT >= 159) {
            if (!fframe) {
                fframe=true;
                
                switch (nState) {
                    case 0:
                    switch (nNextBlock) {
                        // Many of these cases are doubled because we're doing the 'timing' on
                        // the cheap, but trying to sync a 100hz playback to a 60hz refresh.
                        // It's possible to miss one block (and I saw it happen), but it's not
                        // possible to miss two in a row, so we double up all except near the
                        // end where we can't :) It's harmless to show the same page twice,
                        // in the cases where that is hit, so we're win-win and save another
                        // variable/comparison.
                        case 100: ShowPage2(); break;
                        case 101: ShowPage2(); break;
                        
                        case 140: ShowPage1(); break;
                        case 141: ShowPage1(); break;

                        case 145: ShowPage2(); break;
                        case 146: ShowPage2(); break;

                        case 150: ShowPage1(); break;
                        case 151: ShowPage1(); break;

                        case 155: ShowPage2(); break;
                        case 156: ShowPage2(); break;

                        case 160: ShowPage1(); break;
                        case 161: ShowPage1(); break;

                        case 165: ShowPage2(); break;
                        case 166: ShowPage2(); break;

                        case 168: ShowPage1(); break;
                        case 169: ShowPage1(); break;

                        case 170: ShowPage2(); break;
                        case 171: ShowPage2(); break;

                        case 172: ShowPage1(); break;
                        case 173: ShowPage1(); break;

                        // This is the flicker that we don't have an extra timeslot for
                        // but no biggie if we miss it.
                        case 174: ShowPage2(); break;
                        case 175: ShowPage1(); break;
                        case 176: ShowPage1(); break;

                        case 300: idx=5; nState=1; break;
                        case 301: idx=5; nState=1; break;
                    }
                    break;

                    case 1:
                        // When this fade ends, fade in the 'presented by Tursi, www.harmlesslion.com'
                        // That stays up till the sample ends, then we fade IT out
                        if (idx<32) {
                            REG_COLEY=(u16)(idx>>1);
                            idx++;
                            if (idx==32) {
                                nState=2;
                                idx=0;
                            }
                        }
                        break;
// TODO: Crashing if we let the whole intro run. If we skip steps 2 and 3, we don't crash
// Sound is temporarily at 44khz as it finds it faster. Some kind of race condition on
// interrupts or timers?
                    case 2:
                        // load the palette
                        fastcopy16(BG_PALETTE, presented_pal, 32);
                        nState=3;
                        break;

                    case 3:
                        // a bit more work here - load the gfx into tile memory
                        // screen is dark and the palette is set, should be invisible
                        fastcopy16(0x6000000, presented_gfx, sizeof(presented_gfx));
                        nState=4;
                        break;

                    case 4:
                        // now copy in the 32x20 map: (note: file was MANUALLY padded from 30x20)
                        fastcopy16(0x6006000, presented_map, 1280);
                        nState=5;
                        break;

                    case 5:
                        // Finally, change the graphics mode. We might as well stay on BG2 - 16 color, 256x256
                        REG_BG2CNT=(0x0c << TILE_MAP_BANK_SHIFT);
                        REG_DISPCNT=MODE0 | BG2_ENABLE;
                        nState=6;
                        idx=31;
                        break;

                    case 6:
                        // Yay! Now we can fade in!
                        if (idx>0) {
                            REG_COLEY=(u16)idx>>1;
                            idx--;
                            if (idx<0) {
                                nState=7;
                        }
                        break;

                    case 7:
                        // Now we just wait for the audio to end
                        break;
                    }
                }
            }
        } else {
            fframe=false;
        }
    }

    AudioOff();
    
    // Fade out and exit
    REG_COLEY=0;
    REG_BLDMOD= SOURCE_BG2 | DARKEN;
    for (idx=0; idx<17; idx++) {
        REG_COLEY=(u16)idx;
        vsync();
    }
}

