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

extern pvr_ptr_t txr_misc,pal_misc;
#define MAX_BAG_DROP_SPEED 12

static const int backgroundGraphics[48] = 
{
	4, 9, 9, 9, 9, 9, 9, 4,
	1, 2, 2, 2, 2, 2, 2, 3,
	4, 9, 9, 9, 9, 9, 9 ,4,
	4, 9, 9, 9, 9, 9, 9 ,4,
	7, 6, 6, 6, 6, 6, 6, 5,
	8, 8, 8, 8, 8, 8, 8, 8
};

typedef enum 
{
	iskurStand,
	iskurLeft,
	iskurLeftHop,
	iskurRight,
	iskurRightHop,
	iskurFakeStand,
	iskurFakeThrowStand,
	iskurFakeThrowArms,
	iskurThrowStand,
	iskurThrowArms,
	iskurThrowDrop,
} IskurState;

static const int iskurStateTile[11] = 
{
	19, 25, 26, 23, 24, 19, 19, 20, 19, 20, 21	
};

static const int zeusStateTile[10] = {
	31, 27, 28, 29, 30, 36, 32, 33, 34, 35
};

static const int bucketStateTile[10] = {
	38, 37, 38, 37, 38, 40, 39, 40, 39, 40	
};

TXRMAP watertexmap[50];
SPRITE backgroundSprites[48];
SPRITE sheepSprites[18];
SPRITE bagSprites[20]; // Sprites to hold falling bags
int nCaughtBags;
BOOL bSheepLeft;

static IskurState iskursState; // What state is Iskur in right now
static IskurState iskursCachedState; // A state cache for various uses
static int iskurStateFrames; // How many more frames will Iskur be in this state
static SPRITE iskurSprite; // The sprite that holds Iskur
static SPRITE iskurArmsSprite; // The sprite that holds Iskur's arms for drop
static SPRITE zeusSprite;
static SPRITE bucketSprite;
static int iskurSpeed;
static int remainingNumberOfBagsToDrop;
static int delayBetweenDropRuns;
static int resetNumberOfBagsToDrop;
static int runSpeedDuringBagDrop;
static int numberOfRunsCompleted;
static int bagDropSpeed;
static int zeusRunStep;
static int zeusDirection;
static int zeusLastBadTouch;
static int zeusAnimationFrames;
static int nSheepLeft;	// used to detect end of game - sheep are moved away each miss
static int throwDelay;

static void initAllSprites (void)
{
	iskurSprite.txr_addr = txr_misc;
	iskurSprite.pal_addr = pal_misc;
	iskurSprite.x = 120;
	iskurSprite.y = 0;
	iskurSprite.alpha = 31;
	iskurSprite.is3D = 0;
	iskurSprite.z = FX16_CONST(-1);
	iskurSprite.tilenumber = iskurStateTile[iskursState] - 1;
	iskurSprite.nDepth = DEPTH_256x256x8;
	
	iskurArmsSprite.txr_addr = txr_misc;
	iskurArmsSprite.pal_addr = pal_misc;
	iskurArmsSprite.x = 120;
	iskurArmsSprite.y = 32;
	iskurArmsSprite.alpha = 31;
	iskurArmsSprite.is3D = 0;
	iskurArmsSprite.z = FX16_CONST(-1);
	iskurArmsSprite.tilenumber = 21;
	iskurArmsSprite.nDepth = DEPTH_256x256x8;

	zeusSprite.txr_addr = txr_misc;
	zeusSprite.pal_addr = pal_misc;
	zeusSprite.x = 120;
	zeusSprite.y = 160;
	zeusSprite.alpha = 31;
	zeusSprite.is3D = 0;
	zeusSprite.z = FX16_CONST(-0.75);
	zeusSprite.tilenumber = 30;
	zeusSprite.nDepth = DEPTH_256x256x8;
	
	bucketSprite.txr_addr = txr_misc;
	bucketSprite.pal_addr = pal_misc;
	bucketSprite.x = 120;
	bucketSprite.y = 128;
	bucketSprite.alpha = 31;
	bucketSprite.is3D = 0;
	bucketSprite.z = FX16_CONST(-0.75);
	bucketSprite.tilenumber = 37;
	bucketSprite.nDepth = DEPTH_256x256x8;
	
	for (int spriteCounter = 0; spriteCounter < 48; spriteCounter++) 
	{
		backgroundSprites[spriteCounter].txr_addr = txr_misc;
		backgroundSprites[spriteCounter].pal_addr = pal_misc;
		backgroundSprites[spriteCounter].x = (spriteCounter % 8) * 32;
		backgroundSprites[spriteCounter].y = (spriteCounter / 8) * 32;
		backgroundSprites[spriteCounter].alpha = 31;
		backgroundSprites[spriteCounter].is3D = 0;
		backgroundSprites[spriteCounter].z = FX16_CONST(0);
		backgroundSprites[spriteCounter].tilenumber = backgroundGraphics[spriteCounter] - 1;
		backgroundSprites[spriteCounter].nDepth = DEPTH_256x256x8;
	}

	for (int spriteCounter = 0; spriteCounter < 18; spriteCounter++) 
	{
		sheepSprites[spriteCounter].txr_addr = txr_misc;
		sheepSprites[spriteCounter].pal_addr = pal_misc;
		sheepSprites[spriteCounter].x = 40 + spriteCounter * 6 + (rand() % 20);
		sheepSprites[spriteCounter].y = 165;
		sheepSprites[spriteCounter].xd = ((spriteCounter % 3) + 1) * ((rand() % 2) ? 1 : -1); // Assign speed as +- 1, 2, 3
		sheepSprites[spriteCounter].yd = spriteCounter % 16;
		sheepSprites[spriteCounter].alpha = 31;
		sheepSprites[spriteCounter].is3D = 0;
		sheepSprites[spriteCounter].z = FX16_CONST(-0.5 - (spriteCounter * 0.025)); // Assign to z of -0.5 or less
		sheepSprites[spriteCounter].tilenumber = 11;
		sheepSprites[spriteCounter].nDepth = DEPTH_256x256x8;
	}

	for (int spriteCounter = 0; spriteCounter < 20; spriteCounter++) 
	{
		bagSprites[spriteCounter].txr_addr = txr_misc;
		bagSprites[spriteCounter].pal_addr = pal_misc;
		bagSprites[spriteCounter].x = 0;
		bagSprites[spriteCounter].y = 32;
		bagSprites[spriteCounter].xd = 0;
		bagSprites[spriteCounter].yd = 0;
		bagSprites[spriteCounter].alpha = 31;
		bagSprites[spriteCounter].is3D = 0;
		bagSprites[spriteCounter].z = FX16_CONST(-0.74); 
		bagSprites[spriteCounter].tilenumber = 0;
		bagSprites[spriteCounter].nDepth = DEPTH_256x256x8;
	}
}

static bool moveIskurLeft(bool needHop) 
{
	if (iskurSprite.x <= 30) 
	{
		iskursState = iskurRightHop;
		return true;
	}
	else 
	{
		iskurSprite.x -= iskurSpeed;
		if ((iskurStateFrames % 4) == 0)
		{
			if (needHop) 
			{
				iskursState = iskurLeftHop;
			}
			else 
			{
				iskursState = iskurLeft;
			}
			return true;
		}
	}
	return false;
}

static bool moveIskurRight(bool needHop) 
{
	if (iskurSprite.x >= 190) 
	{
		iskursState = iskurLeftHop;
		return true;
	}
	else 
	{
		iskurSprite.x += iskurSpeed;
		if ((iskurStateFrames % 4) == 0)
		{
			if (needHop) 
			{
				iskursState = iskurRightHop;
			}
			else 
			{
				iskursState = iskurRight;
			}
			return true;
		}
	}
	return false;
}

static void handleThrowRoutine (void) {
	iskursCachedState = iskursState;
	if (remainingNumberOfBagsToDrop == 0) {
		// handles difficulty increase (do it before a run, not after, though the variable name is misleading now)
		numberOfRunsCompleted++;
		if ((numberOfRunsCompleted % 3) == 0) {
			resetNumberOfBagsToDrop++;
			if (delayBetweenDropRuns > 29) {
				delayBetweenDropRuns -= 20;
			}
		}
		if ((numberOfRunsCompleted % 4) == 0) {
			if (runSpeedDuringBagDrop < 12) {	// eventually faster than zeus, slightly
				runSpeedDuringBagDrop++;
				if (bagDropSpeed < MAX_BAG_DROP_SPEED) {
					bagDropSpeed++;
				}
			}
			if (throwDelay > 1) throwDelay--;
		}
		// sets up this run
		remainingNumberOfBagsToDrop = resetNumberOfBagsToDrop;
		iskurSpeed = runSpeedDuringBagDrop;
	}
	iskursState = iskurThrowStand;
	iskurStateFrames = 3;
}

static void handleEndThrow (void) {
	remainingNumberOfBagsToDrop--;
	if (remainingNumberOfBagsToDrop == 0) {
		iskurStateFrames = delayBetweenDropRuns;
		iskursState = iskursCachedState;
		iskurSpeed = 1;
	} else {
		iskurStateFrames = 10;
		if ((rand() % 10) < 2) {
			if ((iskursCachedState == iskurLeft) || (iskursCachedState == iskurLeftHop)) {
				iskursState = iskurRight;	
			} else {
				iskursState = iskurLeft;	
			}
		} else {
			iskursState = iskursCachedState;
		}
	}
}

static void addABag(void) {
	for (int spriteCounter = 0; spriteCounter < 20; spriteCounter++) {
		if (bagSprites[spriteCounter].tilenumber == 0) {
			bagSprites[spriteCounter].y = 32;
			bagSprites[spriteCounter].x = iskurSprite.x;
			bagSprites[spriteCounter].tilenumber = 9;
			bagSprites[spriteCounter].xd = 0;
			bagSprites[spriteCounter].is3D = 0;
			break;
		}
	}
}

static void handleIskur (void)
{
	iskurStateFrames --;
	
	switch (iskursState)
	{
		case iskurStand:
			if (iskurStateFrames == 0)
			{
				iskurStateFrames = delayBetweenDropRuns;
				iskursState = iskurLeft;
			}
			break;
		case iskurLeft:
			moveIskurLeft(true);
			if (iskurStateFrames == 0) 
			{
				handleThrowRoutine();
			}
			break;
		case iskurLeftHop:
			moveIskurLeft(false);
			if (iskurStateFrames == 0) 
			{
				handleThrowRoutine();
			}
			break;
		case iskurRight:
			moveIskurRight(true);
			if (iskurStateFrames == 0) 
			{
				handleThrowRoutine();
			}
			break;
		case iskurRightHop:
			moveIskurRight(false);
			if (iskurStateFrames == 0) 
			{
				handleThrowRoutine();
			}
			break;
		case iskurFakeStand:
			if (iskurStateFrames == 0)
			{
				iskurStateFrames = 120;
				iskursState = iskursCachedState;
			}
			break;
		case iskurFakeThrowStand:
			if (iskurStateFrames == 0)
			{
				iskurStateFrames = 15;
				iskursState = iskurFakeThrowArms;
			}
			break;
		case iskurFakeThrowArms:
			if (iskurStateFrames == 0)
			{
				iskurStateFrames = 120;
				iskursState = iskursCachedState;
			}
			break;
		case iskurThrowStand:
			if (iskurStateFrames == 0)
			{
				iskurStateFrames = throwDelay;
				iskursState = iskurThrowArms;
			}
			break;
		case iskurThrowArms:
			if (iskurStateFrames == 0)
			{
				iskurArmsSprite.x = iskurSprite.x;
				iskurStateFrames = throwDelay;
				addABag();
				iskursState = iskurThrowDrop;
			}
			break;
		case iskurThrowDrop:
			if (iskurStateFrames == 0)
			{
				handleEndThrow();
			}
			break;
	}
	iskurSprite.tilenumber = iskurStateTile[iskursState] - 1;
}

static void handleBags(void) {
	for (int spriteCounter = 0; spriteCounter < 20; spriteCounter++) {
		if (bagSprites[spriteCounter].tilenumber == 9) {
			// falling bag
			bagSprites[spriteCounter].y += bagDropSpeed;
			bagSprites[spriteCounter].x += bagSprites[spriteCounter].xd;
			bagSprites[spriteCounter].is3D += bagSprites[spriteCounter].xd;
			if (bagSprites[spriteCounter].y >= 160) {
				bagSprites[spriteCounter].tilenumber = 40;
				bagSprites[spriteCounter].yd = 10;
				bagSprites[spriteCounter].y += 15;
				bagSprites[spriteCounter].is3D = 0;
				// lose sheep
				if (nSheepLeft > 0) {
					nSheepLeft -= 3;
					if (nSheepLeft >= 0) sheepSprites[nSheepLeft].yd=-20;		// 'explode' sheep upwards, they'll then drop off screen
					if (nSheepLeft >= -1) sheepSprites[nSheepLeft+1].yd=-20;	// 'explode' sheep upwards, they'll then drop off screen
					if (nSheepLeft >= -2) sheepSprites[nSheepLeft+2].yd=-20;	// 'explode' sheep upwards, they'll then drop off screen
					sound_effect_system(SND_SHEEP1+(ZAPPEDA+rand()%3), SHEEPVOL);
				} 
			} else if ((bagSprites[spriteCounter].y >= bucketSprite.y-10) && (bagSprites[spriteCounter].y < bucketSprite.y-2)) {
				if ((bagSprites[spriteCounter].x >= bucketSprite.x-24) && (bagSprites[spriteCounter].x <= bucketSprite.x+24)) {
					if ((bagSprites[spriteCounter].x >= bucketSprite.x-8) && (bagSprites[spriteCounter].x <= bucketSprite.x+8)) {
						char tmp[16];
					
						// caught it!
						nCaughtBags++;
						sprintf(tmp, "%d", nCaughtBags);
						WriteFont2D(22, 0, tmp);
						// same animation as the ground splash, though shorter and hidden behind the bucket
						bagSprites[spriteCounter].tilenumber = 40;
						bagSprites[spriteCounter].yd = 5;
						bagSprites[spriteCounter].y += 15;
						bagSprites[spriteCounter].is3D = 0;
					} else {
						// near miss - bounce the bag off it
						if (bagSprites[spriteCounter].x > bucketSprite.x) {
							bagSprites[spriteCounter].xd=3;
						} else {
							bagSprites[spriteCounter].xd=-3;
						}
					}
				}
			}
		} else if (bagSprites[spriteCounter].tilenumber == 40) {
			// splash sprite delay
			bagSprites[spriteCounter].yd --;
			if (bagSprites[spriteCounter].yd == 0) {
				bagSprites[spriteCounter].tilenumber = 0;
			}
		}
	}
}

static void handleZeus(void) {
	TPData tpData;
	u16 btns;
	
	if (TP_CheckBusy(TP_REQUEST_COMMAND_FLAG_SAMPLING) == 0) {
		TP_RequestSamplingAsync();
	}

	// also read the dpad for people who don't like the touch screen
	// we do it here to let the touch pad work
	btns = GetController(gHumanPlayer);
	if (btns&CONT_DPAD_LEFT) {
		zeusLastBadTouch = zeusSprite.x+16 - 8;
		if (zeusLastBadTouch < 8) {
			zeusLastBadTouch = 8;
		}
	}
	if (btns&CONT_DPAD_RIGHT) {
		zeusLastBadTouch = zeusSprite.x+16 + 8;
		if (zeusLastBadTouch > 247) {
			zeusLastBadTouch = 247;
		}
	}
	
	if (TP_WaitCalibratedResult(&tpData) == 0) {
		if ((!(tpData.validity & TP_VALIDITY_INVALID_X)) && (tpData.touch == TP_TOUCH_ON)) {
			// If the X value is NOT invalid, then Zeus got a 'bad touch' which he runs towards
			zeusLastBadTouch = tpData.x;
		}
	}

	int zeusDistance = abs((zeusSprite.x + 16) - zeusLastBadTouch);
	// If the user is touching, then run. min distance of 3 pixels to avoid jitter
	if (zeusDistance > 3) {
		zeusAnimationFrames = ((zeusAnimationFrames + 1) % 16);
		zeusRunStep = (zeusAnimationFrames / 4) + 1;
		if (zeusRunStep == 0) {
			zeusRunStep = 1;
		}
		if (zeusLastBadTouch < zeusSprite.x+16) {
			zeusDirection = 0;
			zeusSprite.x -= (zeusDistance > 10) ? 10 : zeusDistance;
			bucketSprite.x -= (zeusDistance > 10) ? 10 : zeusDistance;
		} else {
			zeusDirection = 1;
			zeusSprite.x += (zeusDistance > 10) ? 10 : zeusDistance;
			bucketSprite.x += (zeusDistance > 10) ? 10 : zeusDistance;
		}
	} else {
		// If the user stops touching, stop running
		zeusRunStep = 0;
	}
	
	zeusSprite.tilenumber = zeusStateTile[(zeusDirection * 5) + zeusRunStep] - 1;
	bucketSprite.tilenumber = bucketStateTile[(zeusDirection * 5) + zeusRunStep] - 1;
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
	for (int spriteCounter = 0; spriteCounter < 20; spriteCounter++) 
	{
		if (bagSprites[spriteCounter].tilenumber != 0) {
			SortSpriteRotated(&bagSprites[spriteCounter], watertexmap, 0);			
		}
	}
	SortSprite(&iskurSprite, watertexmap, 0);
	if (iskursState == iskurThrowDrop) {
		SortSprite(&iskurArmsSprite, watertexmap, 0);
	}
	SortSprite(&zeusSprite, watertexmap, 0);
	SortSprite(&bucketSprite, watertexmap, 0);
}

static void handleSheep() {
	bSheepLeft = FALSE;
	for (int spriteCounter = 0; spriteCounter < 18; spriteCounter++) 
	{
		if (sheepSprites[spriteCounter].y < 200) bSheepLeft = TRUE;	// end of minigame detect (after all sheep are blown off)
		
		sheepSprites[spriteCounter].x += sheepSprites[spriteCounter].xd;
		if (sheepSprites[spriteCounter].yd >= 0) {
			// normal operation
			sheepSprites[spriteCounter].yd = (sheepSprites[spriteCounter].yd + 1) % 16;
			if ((sheepSprites[spriteCounter].x <= 30) && (sheepSprites[spriteCounter].xd < 0)) 
			{
				sheepSprites[spriteCounter].xd = -sheepSprites[spriteCounter].xd;
			}
			if ((sheepSprites[spriteCounter].x >= 200) && (sheepSprites[spriteCounter].xd > 0)) 
			{
				sheepSprites[spriteCounter].xd = -sheepSprites[spriteCounter].xd;	
			}
			if (sheepSprites[spriteCounter].xd < 0) 
			{
				sheepSprites[spriteCounter].tilenumber = 10 + (sheepSprites[spriteCounter].yd / 4);	
			}
			else
			{
				sheepSprites[spriteCounter].tilenumber = 14 + (sheepSprites[spriteCounter].yd / 4);	
			}
		} else {
			// while yd is negative, we are doing the explode animation. yd gets 10 added to it for a movement value
			// tilenumber doesn't animate anymore (yd was used for that)
			sheepSprites[spriteCounter].y+=sheepSprites[spriteCounter].yd+10;
			if (sheepSprites[spriteCounter].yd < -1) sheepSprites[spriteCounter].yd++;
			if (sheepSprites[spriteCounter].y >= 200) {
				// go back to normal operation, just offscreen
				sheepSprites[spriteCounter].yd = 0;
			}
		}
	}
}

void PlayWaterworksMinigame() {
	int fContinue;
	int nOption = 0;
	char buf[64];
	
	sound_stop();
	Clear2D();
	GX_SetDispSelect(GX_DISP_SELECT_SUB_MAIN);
	SetOffset(0, 0);
	MenuScr_Init();
	MenuScr_InitMenu(MENU_STORY_TEXT);
	MenuScr_UpdateMenuString(0, "BONUS GAME!\n \nProtect the sheep\nfrom Iskur's\nwater balloons!");
	 
	// Load the graphics - this is all cached
	load_bmp_block_mask("gfx/Minigames/waterdrop_minigame_texture.bmp_final.bmp", NULL, txr_misc, pal_misc, -1);
	// NOTE: The map needed to have tiles 0, 1, 2, 3 edited to manually provide the alignment information
	// the balloon '9' was also modified manually to be an unpacked 48x48 sprite so it could be rotated
	load_map("gfx/Minigames/waterdrop_minigame_texture.bmp_map.txt", watertexmap);
	*(u16*)(HW_DB_BG_PLTT+258) = 0;	// set color
	*(u16*)(HW_DB_BG_PLTT+264) = 0;	// set color
	
	iskursState = iskurStand;
	iskurStateFrames = 60; // Stand still for 30 seconds
	initAllSprites();
	iskurSpeed = 1;
	remainingNumberOfBagsToDrop = 0;
	resetNumberOfBagsToDrop = 4;
	runSpeedDuringBagDrop = 4;
	delayBetweenDropRuns = 120;
	numberOfRunsCompleted = 0;
	bagDropSpeed = 2;
	zeusRunStep = 0;
	zeusDirection = 0;
	zeusLastBadTouch = 120;
	nSheepLeft = 18;
	nCaughtBags = 0;
	throwDelay = 3;		// in frames
		
	// now we can start music
	sound_start(SONG_HISCORE, 1);	

	for (;;) {
		handleIskur();
		handleBags();
		handleZeus();
		handleSheep();

		pvr_scene_begin();
		MenuScr_DrawFrame(&nOption);
		RenderSprites();
		pvr_scene_finish();
		
		if (!bSheepLeft) {
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
		sprintf(buf, "Minigame over!\n\n%d balloons caught!\n%d points\n\n", idx, idx*20);
		MenuScr_UpdateMenuString(0, buf);
		sound_effect_system(SND_CLICK, SHEEPVOL);
		sprintf(buf, "%d       ", nCaughtBags-idx);
		WriteFont2D(22, 0, buf);
		
		pvr_scene_begin();
		MenuScr_DrawFrame(&nOption);
		RenderSprites();
		pvr_scene_finish();
	}
	
	sprintf(buf, "Minigame over!\n\n%d balloons caught!\n%d points\n\nPress A to continue.", nCaughtBags, nCaughtBags*20);
	MenuScr_UpdateMenuString(0, buf);
	Clear2D();
	StoryModeTotalScore += nCaughtBags*20;

	if (gGame.ChallengeScore[4] < nCaughtBags) {
		GX_SetDispSelect(GX_DISP_SELECT_MAIN_SUB);
		AddHighScore(gHumanPlayer, 4, nCaughtBags);
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

