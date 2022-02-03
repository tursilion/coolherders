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

// Show Binky, walking across the screen

#include <malloc.h>
#include <string.h>
#include "../tursigb.h"
#include "../gsm/gsm_short.h"

extern const char Ergarg[];         // Sound effect
extern const u16 binktop_gfx[];     // Binky character tiles
extern const u16 binktop_map[];     // Binky layout map
extern const u16 mutantbink_gfx[];  // Background tiles
extern const u16 mutantbink_map[];  // Background layout
extern const u16 mutantbink_pal[];  // Background palette (shared)

#define GfxBuf0 (u16*)0x600C000
#define GfxBuf1 (u16*)0x6000000
#define TileBuf0 (u16*)0x600D000
#define TileBuf1 (u16*)0x6009800
#define PalBuf  (u16*)0x5000000

void special_vsync() {
    // Prevents it from skipping if you call it twice.
    // Yes, it wastes a scanline, but you're waiting anyway!
    while (VCOUNT != 159) {
        if (REG_TM1D == (0xffff-159)) {
            u16 old=VCOUNT;
            process_decode();
            if (VCOUNT < old) return;       // missed it
        }
    }

    while (VCOUNT < 160) {
        if (REG_TM1D == (0xffff-159)) {
            u16 old=VCOUNT;
            process_decode();
            if (VCOUNT < old) return;       // missed it
        }
    }
}

// This function doesn't use interrupts, to avoid stealing your interrupt pointers
// Ensure screen is faded out before you call it!
// Leaves screen in mode 0 with BG 0 and 1 active, and screen faded to black
void ShowBinky() {
    // turn on the sound chip
    REG_SOUNDCNT_X = SND_ENABLED;
    // we don’t want to mess with sound channels 1-4
    REG_SOUNDCNT_L = 0;

    // Audio engine setup
	gsm_init(Ergarg);       // Prepare the GSM decoder - defaults to 16khz
	SetAudioFreq(FREQUENCY_11);  // change to 11khz for the sample
    process_decode();       // load the first buffer
    process_decode();       // load the second buffer

    // Ensure screen is black
    REG_BLDMOD= SOURCE_BG0 | SOURCE_BG1 | SOURCE_BG2 | SOURCE_BG3 | DARKEN;
    REG_COLEY=17;
    // Set up the video - 8-bit tiled, 2 layers
    REG_DISPCNT= MODE0 | BG0_ENABLE | BG1_ENABLE;
    REG_DISPSTAT = 0;
    // Top layer (Binky), 256 color, 256x256 pixels (32x32 tiles), char data at 0x6008000, tiles at 0x6008800
    REG_BG0CNT = PRIORITY0 | COLORS_256 | MAP_SIZE_1x1 | TILE_DAT_BANK3 | (0x1a << TILE_MAP_BANK_SHIFT);
    // Bottom layer (bg), 256 color, 256x256 pixels (32x32 tiles), char data at 0x6000000, tiles at 0x6005000
    REG_BG1CNT = PRIORITY1 | COLORS_256 | MAP_SIZE_1x1 | TILE_DAT_BANK0 | (0x13 << TILE_MAP_BANK_SHIFT);

    // Load our data while the screen's dark
    fastcopy16(PalBuf, mutantbink_pal, 512);
    fastcopy16(GfxBuf1, mutantbink_gfx, 19232);
    fastcopy16(GfxBuf1+9616, mutantbink_gfx+9616, 19232);
    fastcopy16(TileBuf1, mutantbink_map, 1280);

    fastcopy16(GfxBuf0, binktop_gfx, 19200);
    fastcopy16(TileBuf0, binktop_map, 200);

    // Setup the binky
    s16 x, y;
    int fade=32;
    int step=0;
    int flag=0;
    
    x=16;
    y=40;
    REG_BG0_HSCROLL=x;
    REG_BG0_VSCROLL=y;

    // fade in during movement
    REG_BLDMOD= SOURCE_BG0 | SOURCE_BG1 | DARKEN;
    REG_COLEY=17;

    // Main loop, repeat till off screen or key pressed
    for (;;) {
        if (fade > 0) {
            fade--;
            REG_COLEY=(u16)(fade>>1);
        }
        
        if (flag ==1 ) {
            if (REG_TM1D == (0xffff-159)) {
                process_decode();
            }
            if (nGSMError) {
                AudioOff();
            }
        }
        
        x-=2;
        y++;
        step++;
        if (step >= 10) {
            step=0;
            y=40;
        }
        REG_BG0_HSCROLL=x;
        REG_BG0_VSCROLL=y;

        if ((flag == 0) && (x < -55)) {
            flag=1;
            AudioOn();
        }
        
        if (x <= -180) {
            break;
        }
        
        if (x < -150) {
            // fade to black
            REG_COLEY=(u16)((-150-x)>>1);
        }
        
        if ((REG_KEYINPUT & (BTN_A|BTN_B|BTN_SELECT|BTN_START))!=(BTN_A|BTN_B|BTN_SELECT|BTN_START)) {
            REG_COLEY=0xf;
            break;
        }
        
        if (flag) {
            special_vsync();
            special_vsync();
        } else {
            vsync();
            vsync();
        }
    }

    REG_COLEY=17;
    AudioOff();
}

