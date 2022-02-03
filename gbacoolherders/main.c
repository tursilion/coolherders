// Cool Herders - second try
#include "tursigb.h"
#include "gsm/random.h"
#include "gsm/gsm_short.h"
#include "tursi/tursipic.h"
#include "binky/binky.h"

extern const u16 splash_gfx[];
extern const u16 notice_gfx[];
extern const u16 notice_map[];
extern const u16 notice_pal[];
extern const char pTitle[];

int ShowTitle();
int ShowNotice();

int main(void)
{
    int idx, ret;
    
    // fade out in case there's anything there
    REG_COLEY=0;
    REG_BLDMOD= SOURCE_BG0 | SOURCE_BG1 | SOURCE_BG2 | SOURCE_BG3 | DARKEN;
    for (idx=0; idx<17; idx++) {
        REG_COLEY=(u16)idx;
        vsync();
    }

again:
    // First, do the Tursi Intro
    ShowTursi();
    // Now Binky
    ShowBinky();
    
    // That done, let's allow some interrupts
    // Set up the video chip - set vertical blank interrupt
    REG_DISPSTAT = VBLANK_IRQ;
    // Set up timer1 (for the GSM playback) and VBLANK
    REG_IE = INT_TIMER1 | INT_VBLANK;
    REG_IME = 1;    // Enable interrupts

    // Show Title (returns true if start was pressed)
    ret=ShowTitle();
    
    // Turn off interrupts
    REG_DISPSTAT = 0;
    REG_IE = 0;
    REG_IME = 0;

    // Loop forever
    goto again;
    
    return 0;
}

// Expects that screen is faded to black on entry
// Returns a non-zero value if Start was pressed
int ShowTitle() {
    // Display our 16-bit title pic and play the title music until we time out or a key is pressed
    s16 idx, flag;
    
    // turn on the sound chip
    REG_SOUNDCNT_X = SND_ENABLED;
    // we don’t want to mess with sound channels 1-4
    REG_SOUNDCNT_L = 0;

    // Audio engine setup
	gsm_init(pTitle);       // Prepare the GSM decoder - defaults to 16khz
    DEEMPHASIS=23000;
    process_decode();       // load the first buffer
    process_decode();       // load the second buffer

    AudioOn();

    // Start detected flag
    flag=0;
    
    // Main loop, repeat till music ends or key pressed
    while ((!flag)&&(!nGSMError)) {
        // Ensure screen is black
        REG_BLDMOD= SOURCE_BG0 | SOURCE_BG1 | SOURCE_BG2 | SOURCE_BG3 | DARKEN;
        REG_COLEY=17;

        // Set up the video - 16-bit bitmapped
        REG_DISPCNT= MODE3 | BG2_ENABLE;
        REG_DISPSTAT = 0;

        // Load our data while the screen's dark
        // Perhaps a 16-bit title is a waste of memory...? but it looks good :)
        fastcopy16(0x06000000, splash_gfx, 32768);
        fastcopy16(0x06000000+32768, splash_gfx+16384, 32768);
        fastcopy16(0x06000000+65536, splash_gfx+32768, 11264);

        // Fade it in
        for (idx=16; idx>=0; idx--) {
            REG_COLEY=idx;
            vsync();
        }

        // wait
        for (idx=0; idx<250; idx++) {
            if ((REG_KEYINPUT & (BTN_START))!=(BTN_START)) {
                REG_COLEY=0xf;
                flag=1;
                break;
            }

            if (nGSMError) {
                // Music is over
                break;
            }
            
            vsync();
        }
        
        // Fade it out
        for (idx=0; idx<17; idx++) {
            REG_COLEY=idx;
            vsync();
        }

        if ((!flag)&&(!nGSMError)) {
            if (ShowNotice()) {
                flag=1;
                break;
            }
        }
    }

    REG_COLEY=17;
    AudioOff();
    
    return flag;
}

// Expects that screen is faded to black on entry
int ShowNotice() {
    int idx, flag;
    // Display our 16-color notice pic for a few seconds before looping

    // Ensure screen is black
    REG_BLDMOD= SOURCE_BG0 | SOURCE_BG1 | SOURCE_BG2 | SOURCE_BG3 | DARKEN;
    REG_COLEY=17;

    // Set up the video - 4-bit tiled
    REG_DISPCNT= MODE0 | BG0_ENABLE;
    REG_DISPSTAT = 0;
    REG_BG0CNT = MAP_SIZE_1x1 | TILE_DAT_BANK0 | (6 << TILE_MAP_BANK_SHIFT);
    
    // Load our data while the screen's dark
    fastcopy16(0x06000000, notice_gfx, 11168);
    fastcopy16(0x06003000, notice_map, 1280);
    fastcopy16(0x05000000, notice_pal, 32);

    // Fade it in
    for (idx=16; idx>=0; idx--) {
        REG_COLEY=idx;
        vsync();
    }

    flag=0;
    
    // Main loop, repeat till done or key pressed
    for (idx=0; idx<250; idx++) {
        if ((REG_KEYINPUT & (BTN_START))!=(BTN_START)) {
            REG_COLEY=0xf;
            flag=1;
            break;
        }
        vsync();
    }

    // Fade it out
    for (idx=0; idx<17; idx++) {
        REG_COLEY=idx;
        vsync();
    }

    REG_COLEY=17;
    return flag;
}


