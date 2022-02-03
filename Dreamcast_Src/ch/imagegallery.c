/****************************************/
/* Cool Herders                         */
/* A demented muse by Brendan Wiese     */
/* http://starbow.org                   */
/* Programmed by Mike Brent             */
/* http://harmlesslion.com              */
/* Copyright 2000-2005 Brendan Wiese &  */
/* Mike Brent.                          */
/* imagegallery.h                       */
/****************************************/

#include <stdio.h>
#include <malloc.h>

#ifdef WIN32
#include "kosemulation.h"
#else
#include <kos.h>
#include <png/png.h>
#endif

#include "sprite.h"
#include "cool.h"
#include "menu.h"
#include "control.h"
#include "font.h"
#include "sound.h"
#include "rand.h"

extern int lastbuttons[4];
extern pvr_ptr_t disc_txr;
extern pvr_ptr_t txr_misc[4];
extern int nFrames;
extern int gReturnToMenu;

char szText[][34] = {
//		 1234567890123456789012345678901234
		"Animation framework for herders",
		"VMU icons from the demo",
		"Concept for Bobbi Candy Striper",
		"Concept for Bobby - unused",
		"Concept for hell fireball",
		"Concept for Iskur",
		"Concept for player select",
		"Splash screen from demos",
		"Starbow logo for Binky!",
		"Concept for Thalia",
		"Thalia concept art",
		"Original Title page",
		"Concept for Iskur",
		"Sprite concept for Thalia",
		"Angel Herder full rez",
		"Backup Dancer full rez",
		"Bobbi Candy Striper full rez",
		"Basic Herder full rez",
		"Concept for Devil",
		"Devil full rez",
		"God of Dance full rez",
		"Iskur full rez",
		"NH-5 full rez",
		"Concept for Zeus",
		"Thalia full rez",
		"Zombie full rez",
		"Hades Wolf full rez",
		"Zeus full rez",
		"Evil Morgana the Fluffy",
		"Full size original Story art",
		"Gift sheep to core developers",
		"Splash screen from demos",
		"The prettiest sheep in the world",
		"Tursi sprites from first release",
		"Unused disco ball",
		"Logo collection"
};

// Simple image and text viewer - always returns 0
int doImageGallery() {
	int x, y, nText=255;
	int fStopThread;
	int nMode, idx;
	cont_state_t * st;
	float top=0, left=0, bottom=0, right=0;
	float tspeed=0, lspeed=0, bspeed=0, rspeed=0;

	// can't play music while loading images
	sound_stop();
	// disc_txr will contain the loaded image, txr512_misc will contain the currenly displayed image
	CLEAR_IMAGE(txr512_misc, 512, 512);

	// current selection
	x=0;
	y=0;

	// Pop up the loading screen
	ShowLoading();

	// Load our selection screen, caching it
	load_png_block("Bonus/gfx/index.png", disc_txr, PNG_NOCACHE);

	// main loop
	fStopThread=0;
	nMode=0;
	while (!fStopThread) {
		// now to get it onscreen
		BeginScene();
		pvr_list_begin(PVR_LIST_OP_POLY);

		// Opaque polygons
		// Backdrop
		gGfxDarken=100;
		addPageLarge(disc_txr, 64, 0, 0, 0, 511, 479, DEFAULT_COLOR, 1000.0f);
		gGfxDarken=0;
		pvr_list_finish();

		pvr_list_begin(PVR_LIST_TR_POLY);
		// Transparent polygons
		// on top of that, at the appropriate place, the current selection
		addPageLarge(disc_txr, 64+16+x*80, y*80, 16+x*80, y*80, 16+x*80+79, y*80+79, INT_PACK_ALPHA(255-(nFrames%30)*3), 1002.0f);

		if (nMode == 2) {
			int nDone=1;
			// draw a growing version of the loaded picture
			if (left>64.0f) {
				left-=lspeed;
				nDone=0;
				if (left < 64.0f) {
					left=64.0f;
				}
			}
			if (top>0.0f) {
				top-=tspeed;
				nDone=0;
				if (top < 0.0f) {
					top=0.0f;
				}
			}
			if (right<575.0f) {
				right+=rspeed;
				nDone=0;
				if (right > 575.0f) {
					right=575.0f;
				}
			}
			if (bottom<480.0f) {
				bottom+=bspeed;
				nDone=0;
				if (bottom > 480.0f) {
					bottom=480.0f;
				}
			}
			if (nDone) {
				nMode=3;
			}
		}
		if ((nMode==2) || (nMode==3)) {
			stretchLarge2(txr512_misc, (int)left, (int)top, (int)right, (int)bottom, 0, 0, 512, 480, 1002.0f, DEFAULT_COLOR);
		}

		// reminder text that stays out of the way
		switch (nMode) {
		case 0:
			CenterDrawFontBackgroundZ(1001.5f, 0, y>=3?48:368, DEFAULT_COLOR, INT_PACK_COLOR(128, 0, 0, 224), szText[y*6+x]);
			CenterDrawFontBackgroundZ(1001.5f, 0, -1, DEFAULT_COLOR, INT_PACK_COLOR(128, 0, 0, 224), " Press A to select ");
			CenterDrawFontBackgroundZ(1001.5f, 0, -1, DEFAULT_COLOR, INT_PACK_COLOR(128, 0, 0, 224), "Press Start to exit");
			break;

		case 1:
			CenterDrawFontBackgroundZ(1001.5f, 0, y>=3?48:368, DEFAULT_COLOR, INT_PACK_COLOR(128, 0, 0, 224), "Loading...");		
			break;

		case 2:
		case 3:
			CenterDrawFontBackgroundZ(1002.5f, 0, y>=3?48:368, INT_PACK_ALPHA(nText), INT_PACK_COLOR(nText/2, 0, 0, 224), szText[y*6+x]);
			CenterDrawFontBackgroundZ(1002.5f, 0, -1, INT_PACK_ALPHA(nText), INT_PACK_COLOR(nText/2, 0, 0, 224), "Press A to exit view");
		}

		if ((nText>0)&&(nMode == 3)) {
			nText--;
		}

		// Done drawing
		pvr_list_finish();
		pvr_scene_end;

		if (gReturnToMenu) {
			sound_stop();
			return 0;
		}

		if (nMode == 1) {
			// pause here to load the image
			char buf[128];
			sprintf(buf, "Bonus/gfx/img%d.png", y*6+x);
			load_png_block(buf, txr512_misc, PNG_NOCACHE);
			nMode=2;
		}

		/* update the known controller list - this loop knows each port */
		for (idx=0; idx<4; idx++) {
			int u;
			for (u=0; u<MAPLE_UNIT_COUNT; u++) {
				ControllerState[idx]=maple_enum_dev(idx, u);
				if ((ControllerState[idx] != NULL) && (ControllerState[idx]->info.functions & MAPLE_FUNC_CONTROLLER)) {
					break;
				}
			}
			if (u>=MAPLE_UNIT_COUNT) {
				// Not found
				ControllerState[idx]=NULL;
			}
		}

		for (idx=0; idx<4; idx++) {
			if (NULL == ControllerState[idx]) continue;

			st = maple_dev_status(ControllerState[idx]);
			if (NULL == st) {
//				debug("maple_dev_status returned NULL!!\n");
				continue;
			}

			/* If we failed the read, then don't process it. This happens when controls are */
			/* unplugged for one frame (or so?) */
			if ((st->buttons & CONT_DPAD_UP) && (st->buttons & CONT_DPAD_DOWN)) {
				/* Up and down at once? That's crazy! ;) */
				continue;
			}

			if (st->joyx < -JOYDEAD) st->buttons |= CONT_DPAD_LEFT;
			if (st->joyx > JOYDEAD)  st->buttons |= CONT_DPAD_RIGHT;
			if (st->joyy < -JOYDEAD) st->buttons |= CONT_DPAD_UP;
			if (st->joyy > JOYDEAD)  st->buttons |= CONT_DPAD_DOWN;
			// 'Z' doesn't exist on the standard DC controller
			if ((st->ltrig >= 127)||(st->rtrig >= 127)) st->buttons|=CONT_Z;

			if (st->buttons & lastbuttons[idx]) {
				continue;
			}
			lastbuttons[idx]=st->buttons;

			// exit gallery
			if (st->buttons & CONT_START) {
				if (nMode > 0) {
					nMode=0;
				} else {
					fStopThread=1;
				}
				break;
			}

			if (nMode == 0) {
				// picture selection mode 
				if (st->buttons & CONT_DPAD_LEFT) {
					x--;
					if (x<0) x=5;
					continue;
				}
				if (st->buttons & CONT_DPAD_RIGHT) {
					x++;
					if (x>5) x=0;
					continue;
				}
				if (st->buttons & CONT_DPAD_DOWN) {
					y++;
					if (y>5) y=0;
					continue;
				}
				if (st->buttons & CONT_DPAD_UP) {
					y--;
					if (y<0) y=5;
					continue;
				}
				if (st->buttons & CONT_Z) {
					int ox=x;
					int oy=y;
					// random
					while ((ox==x)&&(oy==y)) {
						x=ch_rand()%6;
						y=ch_rand()%6;
					}
				}
			}
			if (nMode == 3) {
				if (st->buttons & CONT_Z) {
					nText=0;
				}
			}

			if (st->buttons & CONT_A) {
				// display this picture, or not
				if (nMode > 0) {
					nMode=0;
				} else {
					// start to display this picture
					top=y*80;
					left=64+16+x*80;
					bottom=top+79;
					right=left+79;
					tspeed=(top-0.0f)/60.0f;
					lspeed=(left-64.0f)/60.0f;
					rspeed=(575.0f-right)/60.0f;
					bspeed=(480.0f-bottom)/60.0f;
					nText=255;
					nMode=1;
				}
			}
		}
	}

	return 0;
}
