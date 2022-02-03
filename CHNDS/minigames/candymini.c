#include <stdio.h>
#include <string.h>

#include "kosemulation.h"

#include "sprite.h"
#include "cool.h"
#include "menu.h"
#include "sound.h"
#include "font.h"
#include "control.h"
#include "rand.h"
#include "levels.h"
#include "vmu_logo.h"
#include "musicgallery.h"
#include "storymode.h"
#include "mapscr_manager.h"
#include "menuscr_manager.h"

#define MAX_SHEEP_RUN_SPEED 8
#define COOKIE_START_CHOMP_TIME 60
#define SHEEP_COUNTDOWN 15

static const int backgroundGraphics[48] = 
{
	1, 2, 3, 1, 2, 3, 1, 2,
	2, 3, 1, 2, 3, 1, 2, 3,
	3, 1, 2, 3, 1, 2, 3, 1,
	1, 2, 3, 1, 2, 3, 1, 2,
	2, 3, 1, 2, 3, 1, 2, 3,
	3, 1, 2, 3, 1, 2, 3, 1
};

static const int cookieGraphics[18] = 
{
	4,12,14,
	5,14,14,
	6,14,14,
	7,14,14,
	8,13,14,
	9,10,11
};

// translation lookup - for each tile index, gives tile to
// change to after a chew completes. If it's the same value,
// the tile is already transparent. Tiles are offset by 1
static const int cookieChew[] = {
	  1, 2, 3,25,17,
	 19,21,23, 9,29,
	 31,15,26,32,16,
	 16,18,18,20,20,
	 22,22,24,24,25,
	 27,27,28,28,30,
	 30,33,33
};
static const int sheepRun[] = {
	36, 37, 38, 39, 40
};
static const int sheepGraze[] = {
	34, 35
};

extern pvr_ptr_t txr_misc,pal_misc;
extern TXRMAP watertexmap[50];
extern SPRITE backgroundSprites[48];
extern SPRITE sheepSprites[18];
extern SPRITE bagSprites[20];		// used for cookie
extern int nCaughtBags;				// used for score
static int nLastTouch = 0;
static BOOL bGameOver;
static int nGameSpeed;
static int nSheepAnim[18];
static int sheepCountdown = SHEEP_COUNTDOWN;		// max frames with no sheep

static void initAllSprites (void)
{
	for (int spriteCounter = 0; spriteCounter < 48; spriteCounter++) 
	{
		backgroundSprites[spriteCounter].txr_addr = txr_misc;
		backgroundSprites[spriteCounter].pal_addr = pal_misc;
		backgroundSprites[spriteCounter].x = (spriteCounter % 8) * 32;
		backgroundSprites[spriteCounter].y = (spriteCounter / 8) * 32;
		backgroundSprites[spriteCounter].alpha = 31;
		backgroundSprites[spriteCounter].is3D = 1;
		backgroundSprites[spriteCounter].z = 2044;
		backgroundSprites[spriteCounter].tilenumber = backgroundGraphics[spriteCounter] - 1;
		backgroundSprites[spriteCounter].nDepth = DEPTH_256x256x8;
		// under the cookie, background graphics are flat
		if (spriteCounter%8 >= 5) {
			backgroundSprites[spriteCounter].is3D = 0;
			backgroundSprites[spriteCounter].z = 2048;
		}
	}

	for (int spriteCounter = 0; spriteCounter < 18; spriteCounter++) 
	{
		sheepSprites[spriteCounter].txr_addr = txr_misc;
		sheepSprites[spriteCounter].pal_addr = pal_misc;
		sheepSprites[spriteCounter].x = -32;
		sheepSprites[spriteCounter].y = 200;// off screen	
		sheepSprites[spriteCounter].xd = 0;	// forward speed counter
		sheepSprites[spriteCounter].yd = 0;	// used when they drop!	
		sheepSprites[spriteCounter].alpha = 31;
		sheepSprites[spriteCounter].is3D = 1;
		sheepSprites[spriteCounter].z = 2028;
		sheepSprites[spriteCounter].tilenumber = 35;
		sheepSprites[spriteCounter].nDepth = DEPTH_256x256x8;
		nSheepAnim[spriteCounter] = 0;
	}
	
	// the cookie itself
	for (int spriteCounter = 0; spriteCounter<18; spriteCounter++) {
		bagSprites[spriteCounter].txr_addr = txr_misc;
		bagSprites[spriteCounter].pal_addr = pal_misc;
		bagSprites[spriteCounter].x = ((spriteCounter % 3) + 5)  * 32;;
		bagSprites[spriteCounter].y = (spriteCounter / 3) * 32 - 8;	// nudge the cookie up to hide the background ridges
		bagSprites[spriteCounter].xd = 0;
		bagSprites[spriteCounter].yd = COOKIE_START_CHOMP_TIME;		// how long a piece has to live
		bagSprites[spriteCounter].alpha = 31;
		bagSprites[spriteCounter].is3D = 1;
		bagSprites[spriteCounter].z = 2048;
		bagSprites[spriteCounter].tilenumber = cookieGraphics[spriteCounter]-1;
		bagSprites[spriteCounter].nDepth = DEPTH_256x256x8;
	}
	
	bagSprites[0].yd = 1;		// no time to live, really, for the tiny corner pieces
	bagSprites[15].yd= 1;
	nLastTouch = 0;				// touch pad not touched yet (doesn't matter if it is for the first frame)
	nCaughtBags = 0;			// no score
	bGameOver = FALSE;
	nGameSpeed = 1;
}

// Zeus isn't onscreen, but that's okay
// no DPAD input for this game
static void handleZeus(void) {
	char tmp[16];
	TPData tpData;
	
	if (TP_CheckBusy(TP_REQUEST_COMMAND_FLAG_SAMPLING) == 0) {
		TP_RequestSamplingAsync();
	}

	if (TP_WaitCalibratedResult(&tpData) != 0) {
		// try again next frame
		return;
	}
	if (tpData.touch != TP_TOUCH_ON) {
		nLastTouch = 0;
		return;
	}
	if ((tpData.validity & TP_VALIDITY_INVALID_X) || (tpData.validity & TP_VALIDITY_INVALID_Y)) {
		return;
	}
	if (nLastTouch) {
		return;
	}
	nLastTouch = 1;		// one tap, one sheep!
	
	// see if we got any sheep!
	// stop on the first one (so bunched up sheep require multiple taps!)
	for (int idx = 0; idx < 18; idx++) {
		if (sheepSprites[idx].yd) continue;
		if (sheepSprites[idx].x < -16) continue;
		if ((abs(tpData.x-sheepSprites[idx].x) <= 32) && (abs(tpData.y-sheepSprites[idx].y) <= 32)) {
			sheepSprites[idx].yd = -11;		// odd number so we don't increment to 0
			nCaughtBags++;
			sound_effect_system(SND_SHEEP1+(ZAPPEDA+ch_rand()%3), SHEEPVOL);
			if (nCaughtBags%5 == 0) nGameSpeed++;
			sprintf(tmp, "%d", nCaughtBags);
			WriteFont2D(22, 0, tmp);
			break;
		}
	}
}

static void RenderSprites() {
	// this is just for the background image
	for (int spriteCounter = 0; spriteCounter < 48; spriteCounter++) 
	{
		SortSprite(&backgroundSprites[spriteCounter], watertexmap, 0);			
	}
	for (int spriteCounter = 0; spriteCounter < 18; spriteCounter++) 
	{
		SortSprite(&sheepSprites[spriteCounter], watertexmap, 0);			
	}
	for (int spriteCounter = 0; spriteCounter < 18; spriteCounter++) 
	{
		SortSprite(&bagSprites[spriteCounter], watertexmap, 0);	
	}
}

static void handleSheep() {
	bGameOver = FALSE;
	
	// first, should we bring out a new sheep?
	if (((ch_rand()%150) <= nGameSpeed)||(--sheepCountdown < 1)) {
		for (int spriteCounter = 0; spriteCounter < 18; spriteCounter++) {
			if (sheepSprites[spriteCounter].y >= 200) {
				// this one is free!
				sheepSprites[spriteCounter].x = -24;
				sheepSprites[spriteCounter].y = (ch_rand()%6)*32;
				sheepSprites[spriteCounter].yd = 0;
				sheepSprites[spriteCounter].xd = (ch_rand()%nGameSpeed)+1;
				if (sheepSprites[spriteCounter].xd > 16) sheepSprites[spriteCounter].xd = 16;
				sheepSprites[spriteCounter].tilenumber = 35;
				nSheepAnim[spriteCounter] = 10-sheepSprites[spriteCounter].xd;
				if (nSheepAnim[spriteCounter] < 2) nSheepAnim[spriteCounter] = 2;
				break;
			}
		}
	}
	
	// now, we should move the sheep we have	
	for (int spriteCounter = 0; spriteCounter < 18; spriteCounter++) 
	{
		unsigned int nCookieX, nCookieY, nCookieOk;
	
		// check if valid
		if (sheepSprites[spriteCounter].y >= 200) {
			continue;
		}
		
		// reset timer if we have a sheep
		sheepCountdown = SHEEP_COUNTDOWN;	
		
		// check if the sheep is on a cookie piece
		nCookieX = (sheepSprites[spriteCounter].x+16)/32;
		nCookieY = sheepSprites[spriteCounter].y/32;
		int nOffset = (nCookieX-5)+(nCookieY*3);
		if ((nCookieX >= 5) && (nCookieY < 6)) {
			if (cookieChew[bagSprites[nOffset].tilenumber]-1 == bagSprites[nOffset].tilenumber) {
				nCookieOk = 1;		// passable
			} else {
				nCookieOk = 0;
			}
		} else {
			nCookieOk = 1;			// not a cookie piece
		}
			
		if ((sheepSprites[spriteCounter].yd == 0) && (!nCookieOk)) {
			// we need to chew on this cookie
			if (bagSprites[nOffset].yd > 0) bagSprites[nOffset].yd--;
			if (bagSprites[nOffset].yd == 0) {
				bagSprites[nOffset].yd = COOKIE_START_CHOMP_TIME;
				bagSprites[nOffset].tilenumber = cookieChew[bagSprites[nOffset].tilenumber]-1;
				sheepSprites[spriteCounter].x += min(sheepSprites[spriteCounter].xd, 8);
			}
			nSheepAnim[spriteCounter]--;
			if ((nSheepAnim[spriteCounter]&0xff) == 0) {
				int nNext = ((nSheepAnim[spriteCounter]&0xff00)>>8)+1;
				if (nNext > 1) nNext = 0;
				int nSpeed = (10-sheepSprites[spriteCounter].xd);
				if (nSpeed < 2) nSpeed = 2;
				nSheepAnim[spriteCounter] = (nNext<<8) | nSpeed;
				sheepSprites[spriteCounter].tilenumber = 33 + nNext;
			}
		} else {
			// nope, so move them
			sheepSprites[spriteCounter].x += sheepSprites[spriteCounter].xd;
			if (sheepSprites[spriteCounter].yd == 0) {
				// sheep is active, running towards cookie. Did they make it to the end?
				if (sheepSprites[spriteCounter].x>=224) {
					sheepSprites[spriteCounter].x=224;
					bGameOver = TRUE;
				}
			} else {
				// sheep is just falling off the screen
				sheepSprites[spriteCounter].y += sheepSprites[spriteCounter].yd;
				sheepSprites[spriteCounter].yd += 2;		// increment by 2 so we don't hit zero
			}
			nSheepAnim[spriteCounter]--;
			if ((nSheepAnim[spriteCounter]&0xff) == 0) {
				int nNext = ((nSheepAnim[spriteCounter]&0xff00)>>8)+1;
				if (nNext > 4) nNext = 0;
				int nSpeed = (10-sheepSprites[spriteCounter].xd);
				if (nSpeed < 2) nSpeed = 2;
				nSheepAnim[spriteCounter] = (nNext<<8) | nSpeed;
				sheepSprites[spriteCounter].tilenumber = 35 + nNext;
			}
		}
	}
}

void PlayCandylandMinigame() {
	int fContinue;
	int nOption = 0;
	char buf[64];
	
	sound_stop();
	Clear2D();
	GX_SetDispSelect(GX_DISP_SELECT_SUB_MAIN);
	SetOffset(0, 0);
	MenuScr_Init();
	MenuScr_InitMenu(MENU_STORY_TEXT);
	MenuScr_UpdateMenuString(0, "BONUS GAME!\n \nTap the sheep to\nprevent them from\neating the cookie!");
	 
	// Load the graphics - this is all cached
	load_bmp_block_mask("gfx/Minigames/candy_minigame_texture.bmp_final.bmp", NULL, txr_misc, pal_misc, -1);
	load_map("gfx/Minigames/candy_minigame_texture.bmp_map.txt", watertexmap);
	
	initAllSprites();
	*(u16*)(HW_DB_BG_PLTT+258) = 0;	// set color
	*(u16*)(HW_DB_BG_PLTT+264) = 0;	// set color
		
	// now we can start music
	sound_start(SONG_HISCORE, 1);	

	for (;;) {
		handleZeus();
		handleSheep();

		pvr_scene_begin();
		MenuScr_DrawFrame(&nOption);
		RenderSprites();
		pvr_scene_finish();
		
		if (bGameOver) {
			break;
		}
	}
	
	// run a little loop to tally up the score
	MenuScr_UpdateMenuString(0, "Minigame over!\n\n\n\n\n");
	fContinue = 255;
	
	// fade out the music
	while (fContinue > 0) {
		fContinue-=2;	// for 30fps instead of 60
		// fading out
		if (fContinue < 256) {
			if (fContinue < gGame.MVol) {
				set_sound_volume(fContinue, -1);
			}
			fContinue-=7;
		}

		pvr_scene_begin();
		MenuScr_DrawFrame(&nOption);
		RenderSprites();
		pvr_scene_finish();
	}
	
	sound_stop();
	// reset sound volume
	set_sound_volume(gGame.MVol, -1);

	// tally up the count
	for (int idx=0; idx<nCaughtBags; idx++) {
		sprintf(buf, "Minigame over!\n\n%d sheep stopped!\n%d points\n\n", idx, idx*20);
		MenuScr_UpdateMenuString(0, buf);
		sound_effect_system(SND_CLICK, SHEEPVOL);
		sprintf(buf, "%d       ", nCaughtBags-idx);
		WriteFont2D(22, 0, buf);
		
		pvr_scene_begin();
		MenuScr_DrawFrame(&nOption);
		RenderSprites();
		pvr_scene_finish();
	}
	
	sprintf(buf, "Minigame over!\n\n%d sheep stopped!\n%d points\n\nPress A to continue.", nCaughtBags, nCaughtBags*20);
	MenuScr_UpdateMenuString(0, buf);
	Clear2D();
	StoryModeTotalScore += nCaughtBags*20;

	if (gGame.ChallengeScore[4] < nCaughtBags) {
		GX_SetDispSelect(GX_DISP_SELECT_MAIN_SUB);
		AddHighScore(gHumanPlayer, 0, nCaughtBags);
		GX_SetMasterBrightness(-16);
		GXS_SetMasterBrightness(-16);
	} else {
		// wait for a keypress
		while (!isStartPressed()) {
			pvr_scene_begin();
			MenuScr_DrawFrame(&nOption);
			RenderSprites();
			pvr_scene_finish();
		}

		// fade out
		fContinue = 255;
		while (fContinue > 0) {
			fContinue-=2;	// for 30fps instead of 60
			// fading out
			if (fContinue < 256) {
				if ((fContinue>>3) < gGame.MVol) {
					set_sound_volume(fContinue>>3, -1);
				}
				fContinue-=7;
				GX_SetMasterBrightness(-((256-fContinue)>>4));
				GXS_SetMasterBrightness(-((256-fContinue)>>4));
			}

			pvr_scene_begin();
			MenuScr_DrawFrame(&nOption);
			RenderSprites();
			pvr_scene_finish();
		}

	}
		
	// clean up and exit
	Clear2D();
	
	// stop interrupt and reset palette
	// and call the generic clear-screen/reset code
	ShowBlack();
	sound_stop();
	set_sound_volume(gGame.MVol, -1);

	GX_SetDispSelect(GX_DISP_SELECT_MAIN_SUB);
}

