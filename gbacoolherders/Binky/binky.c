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

// Crossfade from thinkbink1 to thinkbink2 (256 color version)

#include <malloc.h>
#include <string.h>
#include "../tursigb.h"

extern const u16 thinkbink1_gfx[];  // Page 1 patterns
extern const u16 thinkbink1_map[];  // Page 1 map
extern const u16 thinkbink1_pal[];  // Shared palette
extern const u16 thinkbink2_gfx[];  // Page 2 patterns
extern const u16 thinkbink2_map[];  // Page 2 map

#define GfxBuf0 (u16*)0x600C000
#define GfxBuf1 (u16*)0x6000000
#define TileBuf0 (u16*)0x6009800
#define TileBuf1 (u16*)0x6009000
#define PalBuf  (u16*)0x5000000

// This function doesn't use interrupts, to avoid stealing your interrupt pointers
// Ensure screen is faded out before you call it!
// Leaves screen in mode 0 with BG 0 and 1 active, and screen faded to black
void ShowBinky() {
    int fade, idx;
    
    // turn off the sound chip
    REG_SOUNDCNT_X = 0;
    // we don’t want to mess with sound channels 1-4
    REG_SOUNDCNT_L = 0;

    // Ensure screen is black
    REG_BLDMOD= SOURCE_BG0 | SOURCE_BG1 | SOURCE_BG2 | SOURCE_BG3 | DARKEN;
    REG_COLEY=17;
    // Set up the video - 8-bit tiled, 2 layers
    REG_DISPCNT= MODE0 | BG0_ENABLE;    /* hide bg1 till we set up the alpha | BG1_ENABLE; */
    REG_DISPSTAT = 0;
    // Bottom layer (thinkbink1), 256 color, 256x256 pixels (32x32 tiles), char data at GfxBuf0, tiles at TileBuf0
    REG_BG0CNT = PRIORITY1 | COLORS_256 | MAP_SIZE_1x1 | TILE_DAT_BANK3 | (0x13 << TILE_MAP_BANK_SHIFT);
    // Top layer (thinkbink2), 256 color, 256x256 pixels (32x32 tiles), char data at GfxBuf1, tiles at TileBuf1
    REG_BG1CNT = PRIORITY0 | COLORS_256 | MAP_SIZE_1x1 | TILE_DAT_BANK0 | (0x12 << TILE_MAP_BANK_SHIFT);

    // Load our data while the screen's dark
    fastcopy16(PalBuf, thinkbink1_pal, 512);
    fastcopy16(GfxBuf0, thinkbink1_gfx, 15552);
    fastcopy16(TileBuf0, thinkbink1_map, 1280);

    fastcopy16(GfxBuf1, thinkbink2_gfx, 18176);
    fastcopy16(GfxBuf1+9088, thinkbink2_gfx+9088, 18176);   // >32k
    fastcopy16(TileBuf1, thinkbink2_map, 1280);

    // fade in during movement
    REG_BLDMOD= SOURCE_BG0 | SOURCE_BG1 | DARKEN;
    REG_COLEY=17;
    fade=17;
    while (fade > 0) {
        fade--;
        REG_COLEY=(u16)(fade>>1);
        vsync();
    }
    
    // wait for a sec
    for (idx=0; idx<90; idx++) {
        if ((REG_KEYINPUT & (BTN_A|BTN_B|BTN_SELECT|BTN_START))!=(BTN_A|BTN_B|BTN_SELECT|BTN_START)) {
            goto getout;
        }
        vsync();
    }

    // crossfade - enable BG1 and setup the effect
    REG_BLDMOD = SOURCE_BG1 | TARGET_BG0 | ALPHA_BLEND;
    REG_COLEV = (16 << 8) | 0;                      /* target at 16, source at 0 */
    REG_DISPCNT = MODE0 | BG0_ENABLE | BG1_ENABLE;  /* activate BG1 - should be invisible */
    
    for (idx=0; idx<=16; idx++) {
        if ((REG_KEYINPUT & (BTN_A|BTN_B|BTN_SELECT|BTN_START))!=(BTN_A|BTN_B|BTN_SELECT|BTN_START)) {
            goto getout;
        }
        vsync();
        vsync();
        vsync();
        vsync();
        REG_COLEV = ((16-idx)<<8) | (idx);      /* crossfade BG1 into prominence */
    }

    // wait for a few secs
    for (idx=0; idx<300; idx++) {
        if ((REG_KEYINPUT & (BTN_A|BTN_B|BTN_SELECT|BTN_START))!=(BTN_A|BTN_B|BTN_SELECT|BTN_START)) {
            goto getout;
        }
        vsync();
    }

getout:
    REG_BLDMOD= SOURCE_BG0 | SOURCE_BG1 | SOURCE_BG2 | SOURCE_BG3 | DARKEN;
    /* fade out */
    for (idx=fade; idx <= 17; idx++) {
        REG_COLEY=idx;
        vsync();
    }
    REG_COLEY=17;
}

