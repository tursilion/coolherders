/****************************************/
/* Cool Herders                         */
/* A demented muse by Brendan Wiese     */
/* http://starbow.org                   */
/* Programmed by Mike Brent             */
/* http://harmlesslion.com              */
/* Copyright 2000-2005 Brendan Wiese &  */
/* Mike Brent.                          */
/* menu.h                               */
/****************************************/

// this code is awful full of copy and paste - didn't have enough time :/

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
#include "sound.h"
#include "font.h"
#include "control.h"
#include "rand.h"
#include "levels.h"
#include "vmu_logo.h"
#include "musicgallery.h"
#include "imagegallery.h"
#include "storymode.h"
#include "cheat.h"

// rg's credits
extern void credzMain(void);
// png handler
extern void flush_png_level_cache();
extern int StoryModeTotalScore;
extern int nContinues;
extern int gIsHardStory;
extern int SheepAnimationFrame[4][4];

// world names
char ShortWorldName[MAX_LEVELS+2][16] = {
	"New Zealand",
	"CandyLand",
	"Haunted House",
	"Toy Factory",
	"Disco",
	"Waterworks",
	"Heaven",
	"Hell",
	"none",
	"Random"
};

// variables used for game options
struct gamestruct gGame;
struct gameoptions gOptions;

int lastbuttons[4] = { CONT_START, CONT_START, CONT_START, CONT_START };
int continuelevel=0;
int hardcontinuelevel=0;

#define MENUFULL	0xFFB0B0FF
#define MENUMID		0xBFB0B0FF
#define MENUON		INT_PACK_COLOR(bright, 176, 176, 255)
#define MENUDIM		0x80B0B0FF
#define MENUOFF		0x80B0B0FF

// options menu - keep these in order and add/remove will be easy
enum {
	mTIMER,
	mROUNDS,
	mWINBY,
	mPOWERUPS,
	mCPUSKILL,
	mCPUMULTI,
	mGHOSTSHEEP,
	mSHEEPSPEED,
	mNIGHTMODE,
	mSOUNDVOL,
	mMUSICVOL,
	mAUTOSAVE,
	mSAVE,
	mLOAD,
	mEXIT
};

#define BRIGHT(x,y) ((x)==(y))?'*':'~',((x)==(y))?'*':'~'
#define NOTBRIGHT(x,y) ((x)!=(y))?'*':'~',((x)!=(y))?'*':'~'

extern pvr_ptr_t txr_controlwork;
extern pvr_ptr_t disc_txr, txr_sheep;
extern pvr_ptr_t txr_levela[4], txr_levelb[4], txr_levelc[4];
extern pvr_ptr_t txr_herder[4], txr_misc[4];
extern int inDemo;
extern unsigned int myJiffies;
extern int nFrames;
extern int colr[4], colg[4], colb[4];
extern sfxhnd_t snd_sheep[6];
extern int level, stage;
extern int gReturnToMenu;

char *title_cache_a=NULL, *title_cache_b=NULL;
char *playersel_cache_a=NULL, *playersel_cache_b=NULL;
char *worldsel_cache_a=NULL, *worldsel_cache_b=NULL;
pvr_ptr_t pLevs[4]= { NULL, NULL, NULL, NULL };

volatile struct HERDQUEUE {
	int fHerderLoaded;
	char szFileName[64];
} HerderLoadQueue[5];	// HerderLoadQueue[5] is the thread continue flag (exit on 0)

char szMenuText[][18] = {
	"Multiplayer",
	"Story Mode",
	"Story Continue",
	"Omake",
	"How to Play",
	"High Scores",
	"Options",
	"Image Gallery",
	"Music Player",
	"Credits",
	"Demo",		// this one is secret - hold left trigger while moving down from previous to see it
};

char szColors[4][8] = {
	"rd",
	"yl",
	"bl",
	"pu",
};

// All characters besides Zeus must be earned (points or automatic?)
// See enum herdTypes in menu.h
// Players flash white anytime their power is activated (if possible?)
char szNames[13][8] = {		//				Special power
	"zeus",		// Zeus						Automatic Lightning powerup every 10 seconds (flash on powerup)
	"herd",		// Basic herder				Invisible to sheep (flash when sheep should have seen him but doesn't)
	"cand",		// Candy Striper			
	"nh-5",		// NH-5 robot minion		Not affected by conveyor belts, short stun time (flash on conveyor or when stunned)
	"danc",		// Backup dancer			Can steal powerups from other players by knocking them free - once every 10s (flash when powerup freed)
	"zomb",		// Zombie herder			All zapped herders lose all speed ups (flash when speed ups removed)

	// Alternates! (x+6)
	"thal",		// Thalia					Automatic speed powerup every 10 seconds (flash on powerup)
	"iskr",		// Iskur					Start with full lightning and speed (flash on level start)
	"angl",		// Angel herder				Causes destructibles to randomly respawn (every 10s) (flash when respawn)
	"wolf",		// Wolf						Forces sheep into ghost mode (flash on level start)
	"godd",		// God of Dance				Sheep are attracted to him every 10 seconds (flash on effect)
	"devl",		// Devil herder				Lightning not stopped by walls (flash when shot goes through wall)

	// unused
	"",
};

int WindowX[4] = { 32, 30, 424, 430 };
int WindowY[4] = { 51, 196, 48, 196 };
int StartOffX[4] = { 16, 24, 24, 14 };
int StartOffY[4] = { 34, 38, 38, 36 };
int ContOffX[4] = {  7, 18, 18,  7 };
int ContOffY[4] = { 32, 34, 34, 30 };

int MarkerX[13] = { 236, 193, 220, 343, 386, 348, 236, 193, 220, 343, 386, 348, 292 };
int MarkerY[13] = { 59,  165, 267, 51,  153, 256, 59,  165, 267, 51,  153, 256, 159 };

// offsets per character tile per window (easier than trying to tune the art perfectly)
int FaceOffsetX[4][12] = {
//	zeus	herd	cand	nh-5	danc	zomb	thal	iskr	angl	wolf	godd	devl
{	1,		-2,		-10,	2,		-1,		-1,		10,		0,		-2,		0,		-1,		-6		},
{	16,		2,		-2,		5,		7,		5,		20,		12,		5,		12,		10,		-1		},
{	17,		6,		-2,		6,		9,		5,		18,		15,		6,		14,		14,		0		},
{	2,		-5,		-12,	-3,		-4,		-5,		9,		-2,		-4,		1,		1,		-5		},
};

int FaceOffsetY[4][12] = {
//	zeus	herd	cand	nh-5	danc	zomb	thal	iskr	angl	wolf	godd	devl
{	-2,		-1,		0,		0,		0,		-1,		0,		1,		-4,		-4,		-2,		0		},
{	0,		-3,		-2,		0,		6,		0,		0,		0,		0,		-2,		2,		0		},
{	2,		0,		2,		1,		2,		1,		1,		3,		0,		-2,		3,		2		},
{	0,		-1,		0,		-1,		1,		0,		-3,		2,		-5,		-8,		3,		1		},
};

//   0 3
//   164
//   2 5
int PlayerMap[13][4] = {		// maps directions from each ID in each direction up,right,down,left
	{	-1, 3, 1,-1	},		// 0
	{	 0, 12,2,-1	},		// 1
	{	 1, 5,-1,-1	},		// 2
	{	-1,-1, 4, 0	},		// 3
	{	 3,-1, 5,12	},		// 4
	{	 4,-1,-1, 2	},		// 5

	// Alternate char states (only used on return when player was shifted)
	{	-1, 3, 1,-1	},		// 6
	{	 0, 12,2,-1	},		// 7
	{	 1, 5,-1,-1	},		// 8
	{	-1,-1, 4, 0	},		// 9
	{	 3,-1, 5,12	},		// 10
	{	 4,-1,-1, 2	},		// 11

	// Random
	{	 0, 4, 5, 1	}		// 12
};

#define FACE_WIDTH 179
#define FACE_HEIGHT 146

// selects the next menu item on the main menu
// this will eventually include checking things like
// permissions on earned options and the like
int RollCredits(int nCurrent, int nDir, int nSecretEnable) {
	nCurrent+=nDir;

	if (nCurrent < MENU_MULTIPLAYER) nCurrent=MENU_CREDITS;
	
	if (nSecretEnable) {
		if (nCurrent > MENU_DEMO) nCurrent=MENU_MULTIPLAYER;
	} else {
		if (nCurrent > MENU_CREDITS) nCurrent=MENU_MULTIPLAYER;				
	}

	// check for unlockable items)
	switch (nCurrent) {
	case MENU_CREDITS:
		if (gGame.UnlockFlags & ENABLE_END_CREDS) {
			break;
		}
		return RollCredits(nCurrent, nDir, nSecretEnable);

	case MENU_MUSIC:
		if (gGame.UnlockFlags & ENABLE_MUSIC_GAL) {
			break;
		}
		return RollCredits(nCurrent, nDir, nSecretEnable);

	case MENU_GALLERY:
		if (gGame.UnlockFlags & ENABLE_IMAGE_GAL) {
			break;
		}
		return RollCredits(nCurrent, nDir, nSecretEnable);

	case MENU_CONTINUE:
		if ((continuelevel > 1)||(hardcontinuelevel > 1)) {
			break;
		}
		return RollCredits(nCurrent, nDir, nSecretEnable);
	
	case MENU_OMAKE:
		if (gGame.UnlockFlags & ENABLE_OMAKE) {
			break;
		}
		return RollCredits(nCurrent, nDir, nSecretEnable);

	// No default case - fall through for anything else
	}

	return nCurrent;
}

// sets gIsHardStory to 1 or 0 depending on story mode
int QueryDifficulty() {
	int fContinue, idx;
	cont_state_t * st;

	gIsHardStory=0;
	// check if hard mode is opened
	if (0 == (gGame.UnlockFlags&ENABLE_HARD_MODE)) {
		// not available, return with default
		return 0;
	}

	fContinue=1;
	while (fContinue) {
		// now to get it onscreen
		BeginScene();
		pvr_list_begin(PVR_LIST_OP_POLY);

		// Opaque polygons
		gGfxDarken=200;
		SortFullPicture(disc_txr, txr_levela[0], 1023.9f);		// title page
		gGfxDarken=0;

		pvr_list_finish();

		pvr_list_begin(PVR_LIST_TR_POLY);
		// Transparent polygons

		CenterDrawFontZ(1026.0f, 0, 170, DEFAULT_COLOR, "Story Mode");
		DrawFont(0, 0, -1, DEFAULT_COLOR, "");
		CenterDrawFontZ(1026.0f, 0, -1, DEFAULT_COLOR, "Select Difficulty");
		DrawFont(0, 0, -1, DEFAULT_COLOR, "");
		CenterDrawFontZ(1026.0f, 0, -1, DEFAULT_COLOR, "Normal    Hard ");
		
		// the above string starts at X=200 and each char is 16 wide, Y=282 and 28 high
		SortRect(1025.0f, 196+(gIsHardStory?148:0), 254, 296+(gIsHardStory?148:0), 281, INT_PACK_COLOR(200,0,0,255), INT_PACK_COLOR(200,0,0,255));

		pvr_list_finish();
		pvr_scene_end;

		if (gReturnToMenu) {
			sound_stop();
			return -1;
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

			if (st->buttons & lastbuttons[idx]) {
				continue;
			}
			lastbuttons[idx]=st->buttons;

			if (st->buttons & (CONT_START|CONT_A)) {
				// need to remember who pressed it
				gHumanPlayer=idx;
				fContinue=0;
				break;
			}

			if (st->buttons & CONT_DPAD_LEFT) {
				gIsHardStory=0;
			}

			if (st->buttons & CONT_DPAD_RIGHT) {
				gIsHardStory=1;
			}

			if (st->buttons & CONT_B) {
				fContinue=0;
				return -1;
			}
		}

		if (gReturnToMenu) {
			return -1;
		}
	}

	return 0;
}

// Main menu on the title page
// Returns item selected: (uses enum now, these values may be wrong)
// -1 - Cancel, Return to title
//	0 - Multiplayer
//	1 - Story Mode
//  2 - Story Mode Continue
//	3 - Options
//	4 - Image Gallery
//  5 - Music Player
//  6 - Demo mode (timeout)
int doMenu() {
	int nOption;
	int idx;
	int fContinue;
	int IdleFrames=0;
	int32 allbtns;
	cont_state_t * st;

#ifdef DEMO_BUILD
		return 0;	// multiplayer
#endif

	gHumanPlayer=-1;
	fContinue=1;
	nOption=1;		// story mode
	if ((continuelevel > 1)||(hardcontinuelevel > 1)) {
		nOption=2;	// story continue
	}

	// We expect the title is still loaded in video memory
	/* update the known controller list - this loop knows each port */
	for (idx=0; idx<4; idx++) {
		int u;

		// default value if all else fails
		lastbuttons[idx]=CONT_START;
		
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

		if (NULL == ControllerState[idx]) {
			continue;
		}

		// this is the entry function - cache the existing controller state on first pass
		st = maple_dev_status(ControllerState[idx]);
		if (NULL == st) {
//			debug("maple_dev_status returned NULL!!\n");
			continue;
		}

		/* If we failed the read, then don't process it. This happens when controls are */
		/* unplugged for one frame (or so?) */
		if ((st->buttons & CONT_DPAD_UP) && (st->buttons & CONT_DPAD_DOWN)) {
			/* Up and down at once? That's crazy! ;) */
			continue;
		}

		/* otherwise, cache it */
		lastbuttons[idx]=st->buttons;
	}
	
	while (fContinue) {
		// now to get it onscreen
		BeginScene();
		pvr_list_begin(PVR_LIST_OP_POLY);

		// Opaque polygons
		SortFullPicture(disc_txr, txr_levela[0], 1023.9f);		// title page

		pvr_list_finish();

		pvr_list_begin(PVR_LIST_TR_POLY);
		// Transparent polygons

		// draw three menu options
		idx=RollCredits(nOption, -1, 0);
		
		DrawFont(1, MENUX, MENUY, MENUDIM, szMenuText[idx]);
		DrawFont(1, MENUX, -1, MENUFULL, szMenuText[nOption]);
		idx=RollCredits(nOption, 1, 0);

		DrawFont(1, MENUX, -1, MENUDIM, szMenuText[idx]);

		pvr_list_finish();
		pvr_scene_end;

		if (gReturnToMenu) {
			sound_stop();
			return -1;
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

		allbtns=0;
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

			if (st->joyy < -JOYDEAD) st->buttons |= CONT_DPAD_UP;
			if (st->joyy > JOYDEAD)  st->buttons |= CONT_DPAD_DOWN;

			allbtns|=st->buttons;

			if (st->buttons & lastbuttons[idx]) {
				continue;
			}
			lastbuttons[idx]=st->buttons;

			if (st->buttons & (CONT_START|CONT_A)) {
				// need to remember who pressed it
				gHumanPlayer=idx;
				fContinue=0;
				break;
			}

			if (st->buttons & CONT_DPAD_UP) {
				nOption=RollCredits(nOption, -1, 0);
			}

			if (st->buttons & CONT_DPAD_DOWN) {
				nOption=RollCredits(nOption, 1, (st->ltrig>127));
			}

			if (st->buttons & CONT_B) {
				fContinue=0;
				nOption=MENU_CANCEL;
				break;
			}
		}

		if (allbtns == 0) {
			IdleFrames++;
			if (IdleFrames > 1920) {
				// timeout - go to demo
				return MENU_DEMO;
			}
		}
	}

	return nOption;
}

// This will become the options menu someday instead.
// Always returns 0
int doOptionsMenu() {
	int option;
	int idx;
	int bright;
	char buf[64];
	cont_state_t * st=NULL;

	idx=0;
	option=0;
	bright=191;

	// Load the world backdrop
	load_png_block("gfx/Menu/World1.png", disc_txr, 0);
	load_png_block("gfx/Menu/World2.png", txr_misc[0], 0);

	while (1) {
		/* check for music refresh */
		if (!musicisplaying()) {
			sound_restart(SONG_TITLE, 1);		/* we can let it repeat automatically here */
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
			// Cheat and map analog to the DPAD
			if (st->joyy < -JOYDEAD) st->buttons |= CONT_DPAD_UP;
			if (st->joyy > JOYDEAD)  st->buttons |= CONT_DPAD_DOWN;
			if (st->joyx > JOYDEAD)  st->buttons |= CONT_DPAD_RIGHT;
			if (st->joyx < -JOYDEAD)  st->buttons |= CONT_DPAD_LEFT;

			// Check buttons
			if (0 == (lastbuttons[idx] & st->buttons)) {
				lastbuttons[idx]=st->buttons;
	
				if (st->buttons & CONT_DPAD_UP) {
					option--;
					if (option<0) option=14;
					switch (option) {
					case mPOWERUPS:
						if (gGame.UnlockFlags & DISABLE_POWERUPS) {
							continue;
						}
						break;

					case mGHOSTSHEEP:
						if (gGame.UnlockFlags & ENABLE_GHOST_CTL) {
							continue;
						}
						break;

					case mNIGHTMODE:
						if (gGame.UnlockFlags & ENABLE_NIGHT_MDE) {
							continue;
						}
						break;
					
					default:
						continue;
					}

					// invalid option, just decrement counter
					// This works so long as we never have two unlockables beside each other ;)
					option--;
					if (option<0) option=14;
					continue;
				}

				if (st->buttons & CONT_DPAD_DOWN) {
					option++;
					if (option>14) option=0;
					switch (option) {
					case mPOWERUPS:
						if (gGame.UnlockFlags & DISABLE_POWERUPS) {
							continue;
						}
						break;

					case mGHOSTSHEEP:
						if (gGame.UnlockFlags & ENABLE_GHOST_CTL) {
							continue;
						}
						break;

					case mNIGHTMODE:
						if (gGame.UnlockFlags & ENABLE_NIGHT_MDE) {
							continue;
						}
						break;
					
					default:
						continue;
					}
					// invalid option, just increment counter
					// This works so long as we never have two unlockables beside each other ;)
					option++;
					if (option>14) option=0;
					continue;
				}

				if (st->buttons & (CONT_A|CONT_DPAD_LEFT|CONT_DPAD_RIGHT)) {
					int nDir=1;
					if (st->buttons & CONT_DPAD_LEFT) {
						nDir=-1;
					}
					if (st->buttons & CONT_A) {
						// don't change values on A
						nDir=0;
					}

					switch (option) {
					case mTIMER:	// Timer 90/60/30/0
						gGame.Options.Timer-=nDir*30;
						if (gGame.Options.Timer > 90) gGame.Options.Timer=0;
						if (gGame.Options.Timer < 0) gGame.Options.Timer=90;
						break;

					case mROUNDS:	// Rounds 1/2/3
						gGame.Options.Rounds+=nDir;
						if (gGame.Options.Rounds < 1) gGame.Options.Rounds=3;
						if (gGame.Options.Rounds > 3) gGame.Options.Rounds=1;
						break;

					case mWINBY:	// Win By Sheep/Score (1/0)
						gGame.Options.Win=!gGame.Options.Win;
						break;

					case mPOWERUPS:	// Powerups 1/0
						gGame.Options.Powers=!gGame.Options.Powers;
						break;

					case mCPUSKILL:	// CPU Skill 0/1/2
						gGame.Options.Skill+=nDir;
						if (gGame.Options.Skill < 0) gGame.Options.Skill=2;
						if (gGame.Options.Skill > 2) gGame.Options.Skill=0;
						break;

					case mCPUMULTI:	// CPU Multiplay 1/0
						gGame.CPU=!gGame.CPU;
						break;

					case mGHOSTSHEEP:	// Ghost Sheep Normal/Never/Always (0/1/2)
						gGame.Options.GhostMaze+=nDir;
						if (gGame.Options.GhostMaze < 0) gGame.Options.GhostMaze=2;
						if (gGame.Options.GhostMaze > 2) gGame.Options.GhostMaze=0;
						break;

					case mSHEEPSPEED:	// Sheep Speed -1/0/1
						gGame.Options.SheepSpeed+=nDir;
						if (gGame.Options.SheepSpeed < -1) gGame.Options.SheepSpeed=1;
						if (gGame.Options.SheepSpeed > 1) gGame.Options.SheepSpeed=-1;
						break;

					case mNIGHTMODE:	// Night Mode 0/1
						if (gGame.Options.NightMode&0x8000) {
							gGame.Options.NightMode&=0xff;
						} else {
							gGame.Options.NightMode|=0x8000;
							if (0x8000 == gGame.Options.NightMode) {
								gGame.Options.NightMode|=NIGHT_DARKEN;
							}
						}
						break;

					case mSOUNDVOL:	// Sound Volume 0-9
						gGame.SVol+=nDir;
						if (gGame.SVol < 0) gGame.SVol=0;	// no wrap
						if (gGame.SVol > 9) gGame.SVol=9;
						set_sound_volume(-1, gGame.SVol*28);
						sound_effect(snd_sheep[ZAPPEDA+ch_rand()%3], SHEEPVOL);
						break;

					case mMUSICVOL:// Music Volume 0-9
						gGame.MVol+=nDir;
						if (gGame.MVol < 0) gGame.MVol=0;	// no wrap
						if (gGame.MVol > 9) gGame.MVol=9;
						set_sound_volume(gGame.MVol*28, -1);
						break;

					case mAUTOSAVE:// Autosave 1/0
						gGame.AutoSave=!gGame.AutoSave;
						break;

					case mSAVE:// Save
						if (0 == (st->buttons & (CONT_DPAD_LEFT|CONT_DPAD_RIGHT))) {
							doVMUSave(0);
							lastbuttons[idx]|=CONT_START;
						}
						break;

					case mLOAD:// Load
						if (0 == (st->buttons & (CONT_DPAD_LEFT|CONT_DPAD_RIGHT))) {
							doVMULoad(0);
							lastbuttons[idx]|=CONT_START;
						}
						break;

					case mEXIT:// Exit
						if (0 == (st->buttons & (CONT_DPAD_LEFT|CONT_DPAD_RIGHT))) {
							debug("Exitting menu because Exit selected: %08x\n", st->buttons);
						}
						return 0;
					}
				} else {
					if (st->buttons & CONT_START) {
						// start with no DPAD or A means exit cleanly. B is ignored to save confusion
						debug("Exitting menu because START pressed: %08x\n", st->buttons);
						return 0;
					}
				}
			}
		}

		BeginScene();
		
		pvr_list_begin(PVR_LIST_OP_POLY);
			gGfxDarken=128;	
			SortFullPicture(disc_txr, txr_misc[0], 1000.0f);
			gGfxDarken=0;
		pvr_list_finish();

		pvr_list_begin(PVR_LIST_TR_POLY);

		// Transparent polygons
		bright+=2;
		if (bright>255) bright=191;

		CenterDrawFont(1, 24, MENUMID, "Cool Options");

		sprintf(buf, "Timer        : %c90%c %c60%c %c30%c %cInfinite%c", BRIGHT(gGame.Options.Timer,90), BRIGHT(gGame.Options.Timer,60), BRIGHT(gGame.Options.Timer,30), BRIGHT(gGame.Options.Timer,0));  
		DrawFontMenu(0, 64, -1, option==mTIMER ? MENUON : MENUOFF, buf);
		sprintf(buf, "Match Wins   : %c1%c %c2%c %c3%c", BRIGHT(gGame.Options.Rounds,1), BRIGHT(gGame.Options.Rounds,2), BRIGHT(gGame.Options.Rounds,3));
		DrawFontMenu(0, 64, -1, option==mROUNDS ? MENUON : MENUOFF, buf);
		sprintf(buf, "Win by       : %cSheep%c %cScore%c", BRIGHT(gGame.Options.Win, 1), BRIGHT(gGame.Options.Win, 0));
		DrawFontMenu(0, 64, -1, option==mWINBY ? MENUON : MENUOFF, buf);
		if (gGame.UnlockFlags & DISABLE_POWERUPS) {
			sprintf(buf, "PowerUps     : %cOn%c %cOff%c", BRIGHT(gGame.Options.Powers, 1), BRIGHT(gGame.Options.Powers, 0));
			DrawFontMenu(0, 64, -1, option==mPOWERUPS ? MENUON : MENUOFF, buf);
		}
		sprintf(buf, "CPU Skill    : %cLost%c %cNormal%c %cDogged%c", BRIGHT(gGame.Options.Skill, 0), BRIGHT(gGame.Options.Skill, 1), BRIGHT(gGame.Options.Skill, 2));
		DrawFontMenu(0, 64, -1, option==mCPUSKILL ? MENUON : MENUOFF, buf);
		sprintf(buf, "CPU Multiplay: %cOn%c %cOff%c", BRIGHT(gGame.CPU, 1), BRIGHT(gGame.CPU, 0));
		DrawFontMenu(0, 64, -1, option==mCPUMULTI ? MENUON : MENUOFF, buf);
		if (gGame.UnlockFlags & ENABLE_GHOST_CTL) {
			sprintf(buf, "Ghost Sheep  : %cNormal%c %cNever%c %cAlways%c", BRIGHT(gGame.Options.GhostMaze, 0), BRIGHT(gGame.Options.GhostMaze, 1), BRIGHT(gGame.Options.GhostMaze, 2));
			DrawFontMenu(0, 64, -1, option==mGHOSTSHEEP ? MENUON : MENUOFF, buf);
		}
		sprintf(buf, "Sheep Speed  : %cSlow%c %cNormal%c %cFast%c", BRIGHT(gGame.Options.SheepSpeed, -1), BRIGHT(gGame.Options.SheepSpeed, 0), BRIGHT(gGame.Options.SheepSpeed, 1));
		DrawFontMenu(0, 64, -1, option==mSHEEPSPEED ? MENUON : MENUOFF, buf);
		if (gGame.UnlockFlags & ENABLE_NIGHT_MDE) {
			if (gGame.Options.NightMode&0x8000) {
				strcpy(buf, "Night Mode   : *On* ~Off~");
			} else {
				strcpy(buf, "Night Mode   : ~On~ *Off*");
			}
			DrawFontMenu(0, 64, -1, option==mNIGHTMODE ? MENUON : MENUOFF, buf);
		}
		sprintf(buf, "Sound Volume : ----------");
		buf[gGame.SVol+15]='@';
		DrawFontMenu(0, 64, -1, option==mSOUNDVOL ? MENUON : MENUOFF, buf);
		sprintf(buf, "Music Volume : ----------");
		buf[gGame.MVol+15]='@';
		DrawFontMenu(0, 64, -1, option==mMUSICVOL ? MENUON : MENUOFF, buf);
		sprintf(buf, "Autosave     : %cYes%c  %cNo%c", BRIGHT(gGame.AutoSave, 1), BRIGHT(gGame.AutoSave, 0));
		DrawFontMenu(0, 64, -1, option==mAUTOSAVE ? MENUON : MENUOFF, buf);
		DrawFontMenu(0, 64, -1, option==mSAVE ? MENUON : MENUOFF, "Save");
		DrawFontMenu(0, 64, -1, option==mLOAD ? MENUON : MENUOFF, "Load");
		DrawFontMenu(0, 64, -1, option==mEXIT ? MENUON : MENUOFF, "Exit");

		DrawFontMenu(0, 0, -1, MENUON, "");
		CenterDrawFont(0, -1, MENUMID, "%2005 HarmlessLion");
		CenterDrawFont(0, -1, MENUMID, "www.harmlesslion.com");
		
		if (st->ltrig >= 127) {
			CenterDrawFont(0, -1, MENUMID, "Build " __DATE__);
		} else if (st->rtrig >= 127) {
			sprintf(buf, "Number of plays: %d", gGame.NumPlays);
			CenterDrawFont(0, -1, MENUMID, buf);
		}

		pvr_list_finish();
		pvr_scene_end;

		if (gReturnToMenu) {
			sound_stop();
			break;
		}
	}

	debug("Exitting menu because Fell out of loop\n");
	return 0;
}

// Background character sprite loading - uses the global HerderLoadQueue
// and txr_herder[0-3]
void HerderLoadThread(void *pdat) {
	int idx;

	debug("HerderLoadQueue started...\n");

	while (HerderLoadQueue[4].fHerderLoaded) {
		for (idx=0; idx<4; idx++) {
			if (!HerderLoadQueue[idx].fHerderLoaded) {
				if ('\0' != HerderLoadQueue[idx].szFileName[0]) {
					debug("Player %d loading %s\n", idx, HerderLoadQueue[idx].szFileName);
					load_png_block((char*)HerderLoadQueue[idx].szFileName, txr_herder[idx], 0);
					debug("Player %d load complete.\n", idx);
					HerderLoadQueue[idx].fHerderLoaded=1;
				}
			}
		}

		thd_sleep(50);
	}

	debug("HerderLoadQueue exitting...\n");
}

// Player select system
// if Recall is 1, reuse the cached settings for who selected what
int doPlayerSelect(int nRecall) {
	// Our main screen is loaded in CPU RAM
	// Select1.png		512x512		disc_txr
	// Select2.png		256x256		txr_misc[0]
	// Our other resources are loaded into the RAMdisk:
	// <name>_head.png	256x256		(12) txr_levela[0-3], txr_levelb[0-3], txr_levelc[0-3]
	// Woodgrain.png	256x256		txr_misc[1]
	// Sheep.png		256x256		txr_sheep
	// Herder...?		256x256		(4) txr_herder[0-3]
	// 
	// Two helpful arrays:
	//	char szColors[4][8]
	//	char szNames[13][8]	// first 6 are normal, second are alts (last is terminator)

	// Pull both triggers when not selected to enter face adjust mode (disabled)
	int idx, fade;
	char buf[128];
	int fContinue;
	kthread_t * thd_hnd;
	SPRITE sheep;
	int nExitCountdown=60;
	int nAdjustMode=0;
	int SelectOffset[4];
	int LockFlag[4];
	static int nType[4]={-1,-1,-1,-1};
	
	// Convenient pointers
	pvr_ptr_t pHeads[12] = {
		txr_levela[0], txr_levela[1], txr_levela[2], txr_levela[3],
		txr_levelb[0], txr_levelb[1], txr_levelb[2], txr_levelb[3],
		txr_levelc[0], txr_levelc[1], txr_levelc[2], txr_levelc[3],
	};
	pvr_ptr_t pWood=txr_misc[1];
	pvr_ptr_t pSelect1=disc_txr;
	pvr_ptr_t pSelect2=txr_misc[0];

	// Load our stuff into video RAM. This should be fast enough
	// that we don't need a special loading screen.
	int nOld=myJiffies;
	debug("Loading player select gfx...\n");

	/* copy the player select into the level textures for display */
	load_png_block("gfx/Menu/Select1.png", pSelect1, 0);
	load_png_block("gfx/Menu/Select2.png", pSelect2, 0);

	// Load the player's faces	
	for (idx=0; idx<12; idx++) {
		// these should be in the RAMdisk now
		sprintf(buf, "gfx/Players/%s/%s_head.png", szNames[idx], szNames[idx]);
		load_png_block(buf, pHeads[idx], 0);
	}
	// This should be in the RAMdisk too
	load_png_block("gfx/Players/Woodgrain.png", pWood, 0);
	debug("Done loading player select in %d ticks, entering loop.\n", myJiffies-nOld);

	// Initialize
	for (idx=0; idx<4; idx++) {
		if ((nRecall==0)||(nType[idx]==-1)) {
			if (idx == gHumanPlayer) {
				// since this guy pressed start, pre-select him
				herder[idx].type=idx;
				herder[idx].flags=1;
			} else {
				herder[idx].type=PLAY_UNSET;
				herder[idx].flags=0;		// 0 = waiting, 1 = open, 2 = locked
			}
		} else {
			herder[idx].type=nType[idx];
			herder[idx].flags=1;		// 0 = waiting, 1 = open, 2 = locked
		}
		herder[idx].score=192;		// used for flashing
		SelectOffset[idx]=0;
		LockFlag[idx]=0;
	}

	for (idx=0; idx<5; idx++) {
		HerderLoadQueue[idx].fHerderLoaded=1;
		strcpy((char*)HerderLoadQueue[idx].szFileName, "");
	}
	thd_hnd = thd_create(HerderLoadThread, NULL);

	sheep.alpha=200;
	sheep.is3D=0;
	sheep.tilenumber=1;
	sheep.txr_addr=txr_sheep;
	sheep.x=0;
	sheep.y=0;
	sheep.xd=0;
	sheep.yd=0;
	sheep.z=1025.0f;

	fade=255;
	fContinue=1;
	// Start main loop
	while ((fContinue)||(nExitCountdown)) {

		if (!fContinue) {
			nExitCountdown--;
			if (nExitCountdown < 1) {
				break;
			}
		}
		
		/* update the known controller list - this loop knows each port */
		if (fContinue) {
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
					herder[idx].type=PLAY_UNSET;
					herder[idx].flags=0;
				}
			}
		}
		
		// Start drawing the frame
		BeginScene();
		pvr_list_begin(PVR_LIST_OP_POLY);
		// Opaque polygons
		pvr_list_finish();

		pvr_list_begin(PVR_LIST_TR_POLY);
		// Transparent polygons

		if (fade > 0) {
			SortRect(1025.0f, 0, 0, 640, 480, INT_PACK_ALPHA(fade), INT_PACK_ALPHA(fade));
			fade-=20;
		}

		// Now draw each herder's indicator
		for (idx=0; idx<4; idx++) {
			uint32 col=INT_PACK_COLOR(255, colr[idx], colg[idx], colb[idx]);
			if (NULL == ControllerState[idx]) {
				DrawFont(0, WindowX[idx]+ContOffX[idx], WindowY[idx]+ContOffY[idx]+16, col,  "    No");
				DrawFont(0, WindowX[idx]+ContOffX[idx], -1, col,                             "Controller");
			} else {
				int x, tmp;

				switch (herder[idx].type&PLAY_MASK) {
				case PLAY_UNSET:
					DrawFont(0, WindowX[idx]+StartOffX[idx], WindowY[idx]+StartOffY[idx], col,"  Press");
					DrawFont(0, WindowX[idx]+StartOffX[idx], -1, col,                         "  Start");
					DrawFont(0, WindowX[idx]+StartOffX[idx], -1, col,                         " to Join");
					break;

				case PLAY_COMPUTER:
					// Not used right now
					DrawFont(0, WindowX[idx]+ContOffX[idx], WindowY[idx]+ContOffY[idx], col, " COMPUTER");
					break;

				case PLAY_NONE:
					// Not used right now
					DrawFont(0, WindowX[idx]+ContOffX[idx], WindowY[idx]+ContOffY[idx], col, "  NOBODY");
					break;

				default:
					// Must be a player!
					if (!LockFlag[idx]) {
						tmp=herder[idx].type;
						if (tmp != 12) {
							x=tmp+SelectOffset[idx];
						} else {
							x=(nFrames/4)%(6+SelectOffset[idx]);
						}
						herder[idx].nIcon=x|0x80;
						addPage2(pHeads[x], WindowX[idx]+FaceOffsetX[idx][x], WindowY[idx]+FaceOffsetY[idx][x], 0, 0, FACE_WIDTH, FACE_HEIGHT, INT_PACK_COLOR(0xff, herder[idx].score, herder[idx].score, herder[idx].score), 1022.0f);
						if (nAdjustMode) {
							sprintf(buf, "%d, %d", FaceOffsetX[idx][x], FaceOffsetY[idx][x]);
							DrawFont(0, 0, 0, INT_PACK_COLOR(255, 255, 255, 255), buf);
						}
					} else {
						DrawFont(0, WindowX[idx]+ContOffX[idx], WindowY[idx]+ContOffY[idx], col, "  LOCKED");
					}
					break;
				}
			}

			// Draw the back of the sign
			addPage2(pWood, WindowX[idx], WindowY[idx], 0, 0, FACE_WIDTH, FACE_HEIGHT, DEFAULT_COLOR, 1020.0f);

			// Draw the sheepie!
			if (herder[idx].flags > 0) {
				int tmp;
				tmp=herder[idx].type;

				if ((herder[idx].flags == 1) || (!HerderLoadQueue[idx].fHerderLoaded)) {
					sheep.x=(float)MarkerX[tmp]+(idx*8);
					sheep.y=(float)MarkerY[tmp]+(idx*8);
					sheep.txr_addr=txr_sheep;
					sheep.alpha=200;
					if (herder[idx].flags == 1) {
						sheep.tilenumber=SheepAnimationFrame[1][((nFrames+idx)>>2)%4];
					} else {
						sheep.tilenumber=1;
					}
					SortSpriteTinted(&sheep, colr[idx], colg[idx], colb[idx]);
				} else {
					// He's a player! Draw that instead if it's loaded!
					sheep.x=(float)MarkerX[tmp]+(idx*8);
					sheep.y=(float)MarkerY[tmp]+(idx*8);
					sheep.alpha=255;
					sheep.txr_addr=txr_herder[idx];
					sheep.tilenumber=21;
					SortSprite(&sheep);
				}
			}

			if (herder[idx].score > 192) herder[idx].score-=4;
		}

		// Draw the main picture
		SortFullPicture(pSelect1, pSelect2, 1023.9f);

		pvr_list_finish();
		pvr_scene_end;

		if (gReturnToMenu) {
			sound_stop();
			/* end the data load thread */
			HerderLoadQueue[4].fHerderLoaded=0;
			thd_wait(thd_hnd);
			return -1;
		}

		if (fContinue) {
			for (idx=0; idx<4; idx++) {
				cont_state_t * st;

				if (NULL == ControllerState[idx]) {
					herder[idx].type=PLAY_UNSET;
					herder[idx].flags=0;
					continue;
				}
			
				st = maple_dev_status(ControllerState[idx]);
				if (NULL == st) {
//					debug("maple_dev_status returned NULL!!\n");
					continue;
				}

				/* If we failed the read, then don't process it. This happens when controls are */
				/* unplugged for one frame (or so?) */
				if ((st->buttons & CONT_DPAD_UP) && (st->buttons & CONT_DPAD_DOWN)) {
					/* Up and down at once? That's crazy! ;) */
					continue;
				}

				if (nAdjustMode==0) {
					// Cheat and map analog up/down to the DPAD
					if (st->joyy < -JOYDEAD) st->buttons |= CONT_DPAD_UP;
					if (st->joyy > JOYDEAD)  st->buttons |= CONT_DPAD_DOWN;
					if (st->joyx < -JOYDEAD) st->buttons |= CONT_DPAD_LEFT;
					if (st->joyx > JOYDEAD)  st->buttons |= CONT_DPAD_RIGHT;
//					if ((herder[idx].type&PLAY_MASK) == PLAY_UNSET) {
//						if ((st->ltrig > 120) && (st->rtrig > 120)) nAdjustMode=1;
//					}
				} else {
					int tmp,x;
					// adjust face offsets with analog
					tmp=herder[idx].type;
					if (tmp != 12) {
						x=tmp+SelectOffset[idx];
					} else {
						x=(nFrames/4)%(6+SelectOffset[idx]);
					}
					if (st->joyy < -JOYDEAD) FaceOffsetY[idx][x]--;
					if (st->joyy > JOYDEAD)  FaceOffsetY[idx][x]++;
					if (st->joyx < -JOYDEAD) FaceOffsetX[idx][x]--;
					if (st->joyx > JOYDEAD)  FaceOffsetX[idx][x]++;
				}

				if (0==(lastbuttons[idx] & st->buttons)) {
					if (herder[idx].flags == 1) {
						int d=-1, tmp;
						if (st->buttons & CONT_DPAD_DOWN) {
							d=2;
						}
						if (st->buttons & CONT_DPAD_LEFT) {
							d=3;
						}
						if (st->buttons & CONT_DPAD_UP) {
							d=0;
						}
						if (st->buttons & CONT_DPAD_RIGHT) {
							d=1;
						}
						tmp=herder[idx].type;
						if ((d != -1) && (-1 != PlayerMap[tmp][d])) {
							herder[idx].type=PlayerMap[tmp][d];
						}
					}
				}

				// Update select offset
				if (herder[idx].flags == 1) {
					SelectOffset[idx]=0;
					LockFlag[idx]=0;

					if ((st->ltrig > 120) || (st->rtrig > 120)) {
						// Is this character unlocked?
						if (herder[idx].type == 12) {
							SelectOffset[idx]=6;
						} else {
							int x;
							if (herder[idx].type>5) herder[idx].type-=6;
							x=1<<(herder[idx].type+SHIFT_EARNED_PLAYER);
							if (gGame.UnlockFlags & x) {
								SelectOffset[idx]=6;
							} else {
								LockFlag[idx]=1;
							}
						}
					}
				}

				if ((st->buttons&(CONT_X|CONT_Y))==(CONT_X|CONT_Y)) {
					// user requested options menu
					ShowBlack();
					doOptionsMenu();
					// reload the graphics that the options menu overwrote
					ShowBlack();
					load_png_block("gfx/Menu/Select1.png", pSelect1, 0);
					load_png_block("gfx/Menu/Select2.png", pSelect2, 0);
					// and now we can continue as if nothing happened
				}
	
				// Check buttons
				if (0==(lastbuttons[idx] & st->buttons)) {
					lastbuttons[idx]=st->buttons;

					if ((st->buttons & (CONT_START|CONT_A)) && (0 == LockFlag[idx])) {
						if (herder[idx].flags == 0) {
							if ((herder[idx].type&PLAY_MASK) == PLAY_UNSET) {
								herder[idx].type=idx;
								herder[idx].flags=1;
							}
						} else {
							if (herder[idx].flags == 1) {
								int i2;
								char *pherd;

								sound_effect(snd_sheep[ZAPPEDA+ch_rand()%3], SHEEPVOL);

								herder[idx].score=255;
								herder[idx].flags=2;
								if (herder[idx].type == 12) {
									do {
										herder[idx].type=ch_rand()%(6+SelectOffset[idx]);
									} while ((herder[idx].type > 5)&&((gGame.UnlockFlags&(1<<(herder[idx].type-6+SHIFT_EARNED_PLAYER)))==0));
								} else {
									herder[idx].type+=SelectOffset[idx];
								}
								SelectOffset[idx]=0;

								// Background load this sprite
								pherd=szNames[herder[idx].type];
								sprintf((char*)HerderLoadQueue[idx].szFileName, "gfx/Players/%s/%s_%s.png", pherd, szColors[idx], pherd);
								HerderLoadQueue[idx].fHerderLoaded=0;

								// Check if we're done
								fContinue=false;
								for (i2=0; i2<4; i2++) {
									if (herder[i2].flags==1) {
										fContinue=true;
									}
								}
							}
						}
					}

					if (st->buttons & CONT_B) {
						int i2;

						if (herder[idx].type >= 6) {
							herder[idx].type-=6;
						}
						
						switch (herder[idx].flags) {
						case 2:
							herder[idx].flags=1;
							break;

						case 1:
							herder[idx].flags=0;
							herder[idx].type=PLAY_UNSET;
							break;

						case 0:
							fContinue=false;
							for (i2=0; i2<4; i2++) {
								if (herder[i2].flags != 0) {
									fContinue=true;
								}
							}
							if (!fContinue) {
								// abort
								ShowBlack();
								/* end the data load thread */
								HerderLoadQueue[4].fHerderLoaded=0;
								thd_wait(thd_hnd);
								return -1;
							}
							break;
						}
					}
				}
			}
		}
	}

	for (idx=0; idx<4; idx++) {
		if ((herder[idx].type & PLAY_MASK)==PLAY_HUMAN) {
			nType[idx]=herder[idx].type;
		} else {
			nType[idx]=-1;
		}
	}

	// Do fixups on unset!
	for (idx=0; idx<4; idx++) {
		if ((herder[idx].type&PLAY_MASK) == PLAY_UNSET) {
			if (gGame.CPU) {
				herder[idx].type=PLAY_COMPUTER|(ch_rand()%12);
			} else {
				herder[idx].type=PLAY_NONE;
			}
		}
	}

	// This checks if no humans were selected.. I think it's not possible anymore??
	// But no harm in it.
	for (idx=0; idx<4; idx++) {
		if ((herder[idx].type&PLAY_MASK)!=PLAY_COMPUTER) break;
	}
	if (idx >= 4) {
		inDemo=1;
	}

	/* end the data load thread */
	HerderLoadQueue[4].fHerderLoaded=0;
	thd_wait(thd_hnd);

	return 0;
}

// stage select animation helper functions
// background loader for level tiles into pLevs[3]
void GfxLoadingThread(void *vName) {
	char *szName=(char*)vName;

	if (szName[0] == '\0') {
		CLEAR_IMAGE(pLevs[3], 512, 512);
	} else {
		load_png_block(szName, pLevs[3], 0);
	}
}

// scroll params
#define SCROLLSPEED 10
#define XCPIX 0.234375f
#define YCPIX 0.209375f
#define ACPIX 0.4f
// These are a place where we'll use different code for DC and WIN32.
// DC treats these as memory addresses, but in Win32 they're handles.
// Only Windows requires this last rotation. Since the memory copies are
// expensive on the Dreamcast, we'll ifdef them. It's just a handle in Win32
// but we can't afford to lose it. (To work with both PC and DC we'd need
// another 512x512 buffer, which we can't afford, and another 512k memory
// copy, which is unnecessary overhead in code that should be quick
// (Well, we already copy 1.5mb where we could round-robin, but
// there's no point adding to that just for Windows))
void ScrollLeft(pvr_ptr_t pLevs[], int nCurrentLev) {
#ifdef WIN32
	pvr_ptr_t hTmp;
#endif
	char buf[128];
	kthread_t * thd_hnd;
	float xs[4], ys[4], al[4];
	int idx, idx2, offset;

	// start loading the tile we need
	if (nCurrentLev+2 > MAX_LEVELS-3) {
		strcpy(buf, "");
	} else {
		sprintf(buf, "gfx/Menu/level%d.png", nCurrentLev+2);
	}
	thd_hnd = thd_create(GfxLoadingThread, buf);

	// Set up current sizes of the pics (including off screen)
	xs[0]=150; xs[1]=225; xs[2]=150; xs[3]=75;
	ys[0]=133; ys[1]=200; ys[2]=133; ys[3]=66;
	al[0]=128; al[1]=255; al[2]=128; al[3]=0;

	// two phase scroll - scroll till we can't see #0, then load #3, then finish the scroll
	// the smaller ones are 133 pixels tall at their current size
	// first 75 pixels to clear the first image (can probably manage a few more if needed
	// to give the loader time to finish, but they should be cached and thus fast)
	offset=0;
	for (idx=0; idx<250/SCROLLSPEED; idx++) {
		offset-=SCROLLSPEED;	// scroll speed per frame
		for (idx2=0; idx2<2; idx2++) {
			xs[idx2]-=(float)SCROLLSPEED*XCPIX;
			ys[idx2]-=(float)SCROLLSPEED*YCPIX;
			al[idx2]-=(float)SCROLLSPEED*ACPIX;
			if (al[idx2]<0) al[idx2]=0;
		}
		for (idx2=2; idx2<4; idx2++) {
			xs[idx2]+=(float)SCROLLSPEED*XCPIX;
			ys[idx2]+=(float)SCROLLSPEED*YCPIX;
			al[idx2]+=(float)SCROLLSPEED*ACPIX;
			if (al[idx2]>255) al[idx2]=255;
		}

		BeginScene();
		pvr_list_begin(PVR_LIST_OP_POLY);
		pvr_list_finish();

		pvr_list_begin(PVR_LIST_TR_POLY);
		// Transparent polygons
		stretchLarge2(pLevs[0], 0+offset-(xs[0]/2), 200-ys[0]/2, 0+offset+xs[0]/2, 200+ys[0]/2, 0, 0, 449, 398, 1024.0f, INT_PACK_ALPHA((int)al[0]));
		stretchLarge2(pLevs[1], 320+offset-(xs[1]/2), 200-ys[1]/2, 320+offset+xs[1]/2, 200+ys[1]/2, 0, 0, 449, 398, 1024.0f, INT_PACK_ALPHA((int)al[1]));
		stretchLarge2(pLevs[2], 640+offset-(xs[2]/2), 200-ys[2]/2, 640+offset+xs[2]/2, 200+ys[2]/2, 0, 0, 449, 398, 1024.0f, INT_PACK_ALPHA((int)al[2]));

		// Draw the main picture
		SortFullPicture(disc_txr, txr_controlwork, 1023.9f);
		pvr_list_finish();
		pvr_scene_end;
	}

	// We have to have the next frame loaded now - from now on display 1-3 instead of 0-2
	thd_wait(thd_hnd);

	// now move the rest of the way
	for (idx=0; idx<70/SCROLLSPEED; idx++) {
		offset-=SCROLLSPEED;	// scroll speed per frame
		for (idx2=0; idx2<2; idx2++) {
			xs[idx2]-=(float)SCROLLSPEED*XCPIX;
			ys[idx2]-=(float)SCROLLSPEED*YCPIX;
			al[idx2]-=(float)SCROLLSPEED*ACPIX;
			if (al[idx2]<0) al[idx2]=0;
		}
		for (idx2=2; idx2<4; idx2++) {
			xs[idx2]+=(float)SCROLLSPEED*XCPIX;
			ys[idx2]+=(float)SCROLLSPEED*YCPIX;
			al[idx2]+=(float)SCROLLSPEED*ACPIX;
			if (al[idx2]>255) al[idx2]=255;
		}

		BeginScene();
		pvr_list_begin(PVR_LIST_OP_POLY);
		pvr_list_finish();

		pvr_list_begin(PVR_LIST_TR_POLY);
		// Transparent polygons
		stretchLarge2(pLevs[1], 320+offset-(xs[1]/2), 200-ys[1]/2, 320+offset+xs[1]/2, 200+ys[1]/2, 0, 0, 449, 398, 1024.0f, INT_PACK_ALPHA((int)al[1]));
		stretchLarge2(pLevs[2], 640+offset-(xs[2]/2), 200-ys[2]/2, 640+offset+xs[2]/2, 200+ys[2]/2, 0, 0, 449, 398, 1024.0f, INT_PACK_ALPHA((int)al[2]));
		if (buf[0] != '\0') {
			stretchLarge2(pLevs[3], 960+offset-(xs[3]/2), 200-ys[3]/2, 960+offset+xs[3]/2, 200+ys[3]/2, 0, 0, 449, 398, 1024.0f, INT_PACK_ALPHA((int)al[3]));
		}

		// Draw the main picture
		SortFullPicture(disc_txr, txr_controlwork, 1023.9f);
		pvr_list_finish();
		pvr_scene_end;
	}

#ifdef WIN32
	hTmp=pLevs[0];
#endif
	PVR_MEMCPY(pLevs[0], pLevs[1], PAGE512);
	PVR_MEMCPY(pLevs[1], pLevs[2], PAGE512);
	PVR_MEMCPY(pLevs[2], pLevs[3], PAGE512);
#ifdef WIN32
	// this is assuming the data type, but keeps consistency with how we saved it
	pLevs[3]=hTmp;
#endif
}

void ScrollRight(pvr_ptr_t pLevs[], int nCurrentLev) {
#ifdef WIN32
	pvr_ptr_t hTmp;
#endif
	char buf[128];
	kthread_t * thd_hnd;
	float xs[4], ys[4], al[4];
	int idx, idx2, offset;

	// start loading the tile we need
	if (nCurrentLev-2 < 0) {
		strcpy(buf, "");
	} else {
		sprintf(buf, "gfx/Menu/level%d.png", nCurrentLev-2);
	}
	thd_hnd = thd_create(GfxLoadingThread, buf);

	// Set up current sizes of the pics (including off screen)
	xs[0]=150; xs[1]=225; xs[2]=150; xs[3]=75;
	ys[0]=133; ys[1]=200; ys[2]=133; ys[3]=66;
	al[0]=128; al[1]=255; al[2]=128; al[3]=0;

	// two phase scroll - scroll till we can't see #0, then load #3, then finish the scroll
	// the smaller ones are 133 pixels tall at their current size
	// first 75 pixels to clear the first image (can probably manage a few more if needed
	// to give the loader time to finish, but they should be cached and thus fast)
	offset=0;
	for (idx=0; idx<250/SCROLLSPEED; idx++) {
		offset+=SCROLLSPEED;	// scroll speed per frame
		for (idx2=0; idx2<4; idx2+=3) {
			xs[idx2]+=(float)SCROLLSPEED*XCPIX;
			ys[idx2]+=(float)SCROLLSPEED*YCPIX;
			al[idx2]+=(float)SCROLLSPEED*ACPIX;
			if (al[idx2]>255) al[idx2]=255;
		}
		for (idx2=1; idx2<3; idx2++) {
			xs[idx2]-=(float)SCROLLSPEED*XCPIX;
			ys[idx2]-=(float)SCROLLSPEED*YCPIX;
			al[idx2]-=(float)SCROLLSPEED*ACPIX;
			if (al[idx2]<0) al[idx2]=0;
		}

		BeginScene();
		pvr_list_begin(PVR_LIST_OP_POLY);
		pvr_list_finish();

		pvr_list_begin(PVR_LIST_TR_POLY);
		// Transparent polygons
		stretchLarge2(pLevs[0], 0+offset-(xs[0]/2), 200-ys[0]/2, 0+offset+xs[0]/2, 200+ys[0]/2, 0, 0, 449, 398, 1024.0f, INT_PACK_ALPHA((int)al[0]));
		stretchLarge2(pLevs[1], 320+offset-(xs[1]/2), 200-ys[1]/2, 320+offset+xs[1]/2, 200+ys[1]/2, 0, 0, 449, 398, 1024.0f, INT_PACK_ALPHA((int)al[1]));
		stretchLarge2(pLevs[2], 640+offset-(xs[2]/2), 200-ys[2]/2, 640+offset+xs[2]/2, 200+ys[2]/2, 0, 0, 449, 398, 1024.0f, INT_PACK_ALPHA((int)al[2]));

		// Draw the main picture
		SortFullPicture(disc_txr, txr_controlwork, 1023.9f);
		pvr_list_finish();
		pvr_scene_end;
	}

	// We have to have the next frame loaded now - from now on display 3,0,1 instead of 0-2
	thd_wait(thd_hnd);

	// now move the rest of the way
	for (idx=0; idx<70/SCROLLSPEED; idx++) {
		offset+=SCROLLSPEED;	// scroll speed per frame
		for (idx2=0; idx2<4; idx2+=3) {
			xs[idx2]+=(float)SCROLLSPEED*XCPIX;
			ys[idx2]+=(float)SCROLLSPEED*YCPIX;
			al[idx2]+=(float)SCROLLSPEED*ACPIX;
			if (al[idx2]>255) al[idx2]=255;
		}
		for (idx2=1; idx2<3; idx2++) {
			xs[idx2]-=(float)SCROLLSPEED*XCPIX;
			ys[idx2]-=(float)SCROLLSPEED*YCPIX;
			al[idx2]-=(float)SCROLLSPEED*ACPIX;
			if (al[idx2]<0) al[idx2]=0;
		}

		BeginScene();
		pvr_list_begin(PVR_LIST_OP_POLY);
		pvr_list_finish();

		pvr_list_begin(PVR_LIST_TR_POLY);
		// Transparent polygons
		if (buf[0] != '\0') {
			stretchLarge2(pLevs[3], -320+offset-(xs[3]/2), 200-ys[3]/2, -320+offset+xs[3]/2, 200+ys[3]/2, 0, 0, 449, 398, 1024.0f, INT_PACK_ALPHA((int)al[3]));
		}
		stretchLarge2(pLevs[0], 0+offset-(xs[0]/2), 200-ys[0]/2, 0+offset+xs[0]/2, 200+ys[0]/2, 0, 0, 449, 398, 1024.0f, INT_PACK_ALPHA((int)al[0]));
		stretchLarge2(pLevs[1], 320+offset-(xs[1]/2), 200-ys[1]/2, 320+offset+xs[1]/2, 200+ys[1]/2, 0, 0, 449, 398, 1024.0f, INT_PACK_ALPHA((int)al[1]));

		// Draw the main picture
		SortFullPicture(disc_txr, txr_controlwork, 1023.9f);
		pvr_list_finish();
		pvr_scene_end;
	}

#ifdef WIN32
	hTmp=pLevs[2];
#endif
	PVR_MEMCPY(pLevs[2], pLevs[1], PAGE512);
	PVR_MEMCPY(pLevs[1], pLevs[0], PAGE512);
	PVR_MEMCPY(pLevs[0], pLevs[3], PAGE512);
#ifdef WIN32
	// this is assuming the data type, but keeps consistency with how we saved it
	pLevs[3]=hTmp;
#endif
}

// these heaven/hell scrolls are bonus levels, so a bit less fancy
// we'll actually load the level first then do a full scroll, no bg task
void ScrollToHell(pvr_ptr_t pLevs[], int nCurrentLev) {
	char buf[128];
	int idx, offset;

	sprintf(buf, "gfx/Menu/level%d.png", LEVEL_HELL);
	load_png_block(buf, pLevs[3], 0);

	offset=0;
	for (idx=0; idx<480/SCROLLSPEED; idx++) {
		offset+=SCROLLSPEED;	// scroll speed per frame
		BeginScene();
		pvr_list_begin(PVR_LIST_OP_POLY);
		pvr_list_finish();

		pvr_list_begin(PVR_LIST_TR_POLY);
		// Transparent polygons
		stretchLarge2(pLevs[3], 207, 580-offset, 432, 780-offset, 0, 0, 449, 398, 1024.0f, INT_PACK_ALPHA(min(255,128+offset/3)));
		stretchLarge2(pLevs[1], 207, 100-offset, 432, 300-offset, 0, 0, 449, 398, 1024.0f, INT_PACK_ALPHA(max(0,255-offset/3)));
		// These two are off to the sides and shown at 33%. 
		if (nCurrentLev-1 >= 0) {
			stretchLarge2(pLevs[0], -75, 133-offset, 75, 266-offset, 0, 0, 449, 398, 1024.0f, INT_PACK_ALPHA(128));
		}
		if (nCurrentLev+1 <= MAX_LEVELS-3) {
			stretchLarge2(pLevs[2], 565, 133-offset, 715, 266-offset, 0, 0, 449, 398, 1024.0f, INT_PACK_ALPHA(128));
		}

		// Draw the main picture
		SortFullPicture(disc_txr, txr_controlwork, 1023.9f);
		pvr_list_finish();
		pvr_scene_end;
	}

	ShowUnlockMessage("You've unlocked\nDEVIL\nalt for zombie", 0x00000020, 1);
}

void ScrollFromHell(pvr_ptr_t pLevs[], int nCurrentLev) {
	int idx, offset;

	offset=480;
	for (idx=0; idx<480/SCROLLSPEED; idx++) {
		offset-=SCROLLSPEED;	// scroll speed per frame
		BeginScene();
		pvr_list_begin(PVR_LIST_OP_POLY);
		pvr_list_finish();

		pvr_list_begin(PVR_LIST_TR_POLY);
		// Transparent polygons
		stretchLarge2(pLevs[3], 207, 580-offset, 432, 780-offset, 0, 0, 449, 398, 1024.0f, INT_PACK_ALPHA(min(255,128+offset/3)));
		stretchLarge2(pLevs[1], 207, 100-offset, 432, 300-offset, 0, 0, 449, 398, 1024.0f, INT_PACK_ALPHA(max(0,255-offset/3)));
		// These two are off to the sides and shown at 33%. Cache all of them while the menu
		if (nCurrentLev-1 >= 0) {
			stretchLarge2(pLevs[0], -75, 133-offset, 75, 266-offset, 0, 0, 449, 398, 1024.0f, INT_PACK_ALPHA(128));
		}
		if (nCurrentLev+1 <= MAX_LEVELS-3) {
			stretchLarge2(pLevs[2], 565, 133-offset, 715, 266-offset, 0, 0, 449, 398, 1024.0f, INT_PACK_ALPHA(128));
		}

		// Draw the main picture
		SortFullPicture(disc_txr, txr_controlwork, 1023.9f);
		pvr_list_finish();
		pvr_scene_end;
	}
}

void ScrollToHeaven(pvr_ptr_t pLevs[], int nCurrentLev) {
	char buf[128];
	int idx, offset;

	sprintf(buf, "gfx/Menu/level%d.png", LEVEL_HEAVEN);
	load_png_block(buf, pLevs[3], 0);

	offset=0;
	for (idx=0; idx<480/SCROLLSPEED; idx++) {
		offset+=SCROLLSPEED;	// scroll speed per frame
		BeginScene();
		pvr_list_begin(PVR_LIST_OP_POLY);
		pvr_list_finish();

		pvr_list_begin(PVR_LIST_TR_POLY);
		// Transparent polygons
		stretchLarge2(pLevs[3], 207, -380+offset, 432, -180+offset, 0, 0, 449, 398, 1024.0f, INT_PACK_ALPHA(min(255,128+offset/3)));
		stretchLarge2(pLevs[1], 207, 100+offset, 432, 300+offset, 0, 0, 449, 398, 1024.0f, INT_PACK_ALPHA(max(0,255-offset/3)));
		// These two are off to the sides and shown at 33%. Cache all of them while the menu
		if (nCurrentLev-1 >= 0) {
			stretchLarge2(pLevs[0], -75, 133+offset, 75, 266+offset, 0, 0, 449, 398, 1024.0f, INT_PACK_ALPHA(128));
		}
		if (nCurrentLev+1 <= MAX_LEVELS-3) {
			stretchLarge2(pLevs[2], 565, 133+offset, 715, 266+offset, 0, 0, 449, 398, 1024.0f, INT_PACK_ALPHA(128));
		}

		// Draw the main picture
		SortFullPicture(disc_txr, txr_controlwork, 1023.9f);
		pvr_list_finish();
		pvr_scene_end;
	}

	ShowUnlockMessage("You've unlocked\nANGEL\nalt for Candi", 0x00000004, 1);
}

int ScrollToRandom(pvr_ptr_t pLevs[], int nCurrentLev) {
	int idx, offset;
	int level;
	char buf[128];

	level=GetRandomLevel();
	sprintf(buf, "gfx/Menu/level%d.png", level);
	load_png_block(buf, pLevs[3], 0);

	offset=0;
	for (idx=0; idx<430/SCROLLSPEED; idx++) {
		offset+=SCROLLSPEED;	// scroll speed per frame
		BeginScene();
		pvr_list_begin(PVR_LIST_OP_POLY);
		pvr_list_finish();

		pvr_list_begin(PVR_LIST_TR_POLY);
		// Transparent polygons

		stretchLarge2(pLevs[3], 207, -380+offset, 432, -180+offset, 0, 0, 449, 398, 1024.0f, INT_PACK_ALPHA(min(255,128+offset/3)));
		stretchLarge2(pLevs[1], 207, 100+offset, 432, 300+offset, 0, 0, 449, 398, 1024.0f, INT_PACK_ALPHA(max(0,255-offset/3)));
		// These two are off to the sides and shown at 33%. Cache all of them while the menu
		if (nCurrentLev-1 >= 0) {
			stretchLarge2(pLevs[0], -75, 133+offset, 75, 266+offset, 0, 0, 449, 398, 1024.0f, INT_PACK_ALPHA(128));
		}
		if (nCurrentLev+1 <= MAX_LEVELS-3) {
			stretchLarge2(pLevs[2], 565, 133+offset, 715, 266+offset, 0, 0, 449, 398, 1024.0f, INT_PACK_ALPHA(128));
		}

		// Draw the main picture
		SortFullPicture(disc_txr, txr_controlwork, 1023.9f);
		pvr_list_finish();
		pvr_scene_end;
	}

	sprintf(buf, "gfx/Menu/level%d.png", level);
	load_png_block(buf, pLevs[1], 0);
	if (level > 0) {
		sprintf(buf, "gfx/Menu/level%d.png", level-1);
		load_png_block(buf, pLevs[0], 0);
	} else {
		CLEAR_IMAGE(pLevs[0], 512, 512);
	}
	if (level < MAX_LEVELS-2) {
		sprintf(buf, "gfx/Menu/level%d.png", level+1);
		load_png_block(buf, pLevs[2], 0);
	} else {
		CLEAR_IMAGE(pLevs[2], 512, 512);
	}

	return level;
}

// also used for random
void ScrollFromHeaven(pvr_ptr_t pLevs[], int nCurrentLev) {
	int idx, offset;

	offset=480;
	for (idx=0; idx<480/SCROLLSPEED; idx++) {
		offset-=SCROLLSPEED;	// scroll speed per frame
		BeginScene();
		pvr_list_begin(PVR_LIST_OP_POLY);
		pvr_list_finish();

		pvr_list_begin(PVR_LIST_TR_POLY);
		// Transparent polygons
		stretchLarge2(pLevs[3], 207, -380+offset, 432, -180+offset, 0, 0, 449, 398, 1024.0f, INT_PACK_ALPHA(min(255,128+offset/3)));
		stretchLarge2(pLevs[1], 207, 100+offset, 432, 300+offset, 0, 0, 449, 398, 1024.0f, INT_PACK_ALPHA(max(0,255-offset/3)));
		// These two are off to the sides and shown at 33%. Cache all of them while the menu
		if (nCurrentLev-1 >= 0) {
			stretchLarge2(pLevs[0], -75, 133+offset, 75, 266+offset, 0, 0, 449, 398, 1024.0f, INT_PACK_ALPHA(128));
		}
		if (nCurrentLev+1 <= MAX_LEVELS-3) {
			stretchLarge2(pLevs[2], 565, 133+offset, 715, 266+offset, 0, 0, 449, 398, 1024.0f, INT_PACK_ALPHA(128));
		}

		// Draw the main picture
		SortFullPicture(disc_txr, txr_controlwork, 1023.9f);
		pvr_list_finish();
		pvr_scene_end;
	}
}

// Dir may be +1 or -1, or 2 for the level-only select mode
void ScrollStages(int nDir, int nStageSelectExtra, int nCurrentLevel) {
	// After selecting one, the others disappear. The main image moves upwards, and the 
	// level thumbnails expand upwards to 100% (128x100) size. They are displayed dimmed except for
	// the currently selected one. The currently selected tile should be copied into
	// a known location (probably txr_misc) for use by the main loader after this function
	int start,end,step;
	int idx, nCurrent;

	if (nStageSelectExtra) {
		nCurrent=3;
	} else {
		nCurrent=1;
	}

	if (nDir > 0) {
		start=0;
		end=50;
		step=SCROLLSPEED/2;
	} else {
		start=50;
		end=0;
		step=-SCROLLSPEED/2;
	}
	for (idx=start; nDir>0?(idx<end):(idx>end); idx+=step) {
		// Start drawing the frame
		BeginScene();
		pvr_list_begin(PVR_LIST_OP_POLY);
		// Opaque polygons
		
		// Show currently selected at 50% of 450x400, move it upwards
		// The other two are off to the sides and shown at 33% and are faded out
		stretchLarge2(pLevs[nCurrent], 207, 100-idx, 432, 300-idx, 0, 0, 449, 398, 1024.0f, INT_PACK_ALPHA(255));

		pvr_list_finish();

		pvr_list_begin(PVR_LIST_TR_POLY);
		// Transparent polygons

		// These two are off to the sides and shown at 33%, and fade out
		if ((!nStageSelectExtra) && (nCurrentLevel > 0)) {
			stretchLarge2(pLevs[0], -75, 133, 75, 266, 0, 0, 449, 398, 1024.0f, INT_PACK_ALPHA(max(0,128-(idx*3))));
		}
		if ((!nStageSelectExtra) && (nCurrentLevel < MAX_LEVELS-3)) {
			stretchLarge2(pLevs[2], 565, 133, 715, 266, 0, 0, 449, 398, 1024.0f, INT_PACK_ALPHA(max(0,128-(idx*3))));
		}

		if (nDir != 2) {
			// the current level's 3 level previews are shown at the bottom, scaling up and tweaking transparency
			stretchLarge2(pLevs[nCurrent], 188-idx, 350-idx, 251, 399, 0, 400, 127, 499, 1025.0f, INT_PACK_ALPHA(224-idx*2));
			stretchLarge2(pLevs[nCurrent], 288-idx/2, 350-idx, 351+idx/2, 399, 128, 400, 255, 499, 1025.0f, INT_PACK_ALPHA(224+idx/2));
			stretchLarge2(pLevs[nCurrent], 388, 350-idx, 451+idx, 399, 256, 400, 383, 499, 1025.0f, INT_PACK_ALPHA(224-idx*2));
		}
		
		// Draw the main picture
		if (nDir == 2) {
			gGfxDarken=idx*5;
		}
		SortFullPicture(disc_txr, txr_controlwork, 1023.9f);
		gGfxDarken=0;

		pvr_list_finish();
		pvr_scene_end;
	}
}

// Stage select system - return -1 for backing up
// pass non-zero to skip selecting a stage
int doStageSelect(int nLevelOnly) {
	// Our main screen is loaded in CPU RAM
	// World1.png		512x512		disc_txr
	// World2.png		256x256		txr_controlwork
	// buffer 1			512x512		txr_levela[0-3]
	// buffer 2			512x512		txr_levelb[0-3]
	// buffer 3			512x512		txr_levelc[0-3]
	// buffer 4			512x512		txr_misc[0-3]
	cont_state_t * st=NULL;
	int idx, stg;
	int fContinue;
	int nLastLevel=0;
	int stairway=0;					// count taps up or down
	// Indicates whether we are showing heaven or hell
	int nStageSelectExtra=0;
	static int nCurrentLevel=0;	// static so we can remember the last choice

	if (nLevelOnly > 0) {
		nCurrentLevel=nLevelOnly-1;
	}

	if (nCurrentLevel >= LEVEL_HEAVEN) {
		nStageSelectExtra=1;
	}

	// load levs array
	pLevs[0]= txr512_levela;
	pLevs[1]= txr512_misc;		// this is where we want it in the end anyway
	pLevs[2]= txr512_levelc;
	pLevs[3]= txr512_levelb;

	/* copy the player select into the level textures for display */
	load_png_block("gfx/Menu/World1.png", disc_txr, 0);
	load_png_block("gfx/Menu/World2.png", txr_controlwork, 0);

	// load three buffers at a time into pLevs (fourth is used during animation/load)
	if (nStageSelectExtra) {
		char buf[128];
		sprintf(buf, "gfx/Menu/level%d.png", nCurrentLevel);
		load_png_block(buf, pLevs[3], 0);	// reload heaven or hell
		stg=nLastLevel;						// make sure to reload the right codes below
	} else {
		stg=nCurrentLevel;					// nothing special here after all
		CLEAR_IMAGE(pLevs[3], 512, 512);
	}
	for (idx=stg-1; idx<stg+2; idx++) {
		int nPos=idx+1-stg;
		if ((nPos != 1)&&(idx>=MAX_LEVELS-2)) {
			CLEAR_IMAGE(pLevs[nPos], 512, 512);
		} else if ((idx<0)||(idx>=MAX_LEVELS)) {
			CLEAR_IMAGE(pLevs[nPos], 512, 512);
		} else {
			char buf[128];
			sprintf(buf, "gfx/Menu/level%d.png", idx);
			load_png_block(buf, pLevs[nPos], 0);
		}
	}

	// What we're going to do here is this:
	// Show three - the center one is the currently selected and is shown at 50% of 450x400
	// The other two are off to the sides and shown at 33%. Cache all of them while the menu
	// is running into RAM (make sure the precaching can be aborted if one is selected!) The 
	// precache should persist for the entire run of the game (so only the first time into
	// the menu need be slow). 50% thumbnails of the current level's 3 level previews are
	// shown at the bottom. 
	
	// After selecting one, the others disappear. The main image moves upwards, and the 
	// level thumbnails expand upwards to 100% size. They are displayed dimmed except for
	// the currently selected one. The currently selected tile should be copied into
	// a known location (probably txr_misc) for use by the main loader after this function
	
	// When a level is selected, the level previews fade out while the main tile grows 
	// to 100%. When it's finished scaling, the words 'Loading [world name]' are displayed.
	// This code must happen in a separate thread outside of this function, however, so that
	// it happens *while* the level is loaded.

startlevelselect:
	/* Scan the known controllers and update the last button state! */
	for (idx=0; idx<4; idx++) {
		if (NULL == ControllerState[idx]) {
			continue;
		}
	
		st = maple_dev_status(ControllerState[idx]);
		if (NULL == st) {
//			debug("maple_dev_status returned NULL!!\n");
			continue;
		}

		if ((st->ltrig>=127)||(st->rtrig>=127)) st->buttons|=CONT_Z;

		/* If we failed the read, then don't process it. This happens when controls are */
		/* unplugged for one frame (or so?) */
		if ((st->buttons & CONT_DPAD_UP) && (st->buttons & CONT_DPAD_DOWN)) {
			/* Up and down at once? That's crazy! ;) */
			continue;
		}

		/* otherwise, cache it */
		lastbuttons[idx]=st->buttons;
	}
	
	fContinue=1;
	// Start main loop
	while (fContinue) {
		int nCurrent, nLocked;

		nLocked=0;
		if (nCurrentLevel != LEVEL_RANDOM) {
			if ((!nStageSelectExtra) && ((gGame.UnlockFlags & (1<<(nCurrentLevel+(gIsHardStory?SHIFT_HARD_STAGE:SHIFT_EARNED_STAGE))))==0)) {
				nLocked=1;
			}
		}

		// Start drawing the frame
		BeginScene();
		pvr_list_begin(PVR_LIST_OP_POLY);
		// Opaque polygons
		
		// Work on the right tile!
		if (nStageSelectExtra) {
			nCurrent=3;
		} else {
			nCurrent=1;
		}

		// Show three - the center one is the currently selected and is shown at 50% of 450x400
		// The other two are off to the sides and shown at 33%. Cache all of them while the menu
		// this is the center solid on
		stretchLarge2(pLevs[nCurrent], 207, 100, 432, 300, 0, 0, 449, 398, 1024.0f, INT_PACK_ALPHA(255));

		// Draw the main picture
		SortFullPicture(disc_txr, txr_controlwork, 1023.9f);

		pvr_list_finish();

		pvr_list_begin(PVR_LIST_TR_POLY);
		// Transparent polygons

		// These two are off to the sides and shown at 33%. Cache all of them while the menu
		if ((!nStageSelectExtra) && (nCurrentLevel > 0)) {
			stretchLarge2(pLevs[0], -75, 133, 75, 266, 0, 0, 449, 398, 1024.0f, INT_PACK_ALPHA(128));
		}
		if ((!nStageSelectExtra) && (nCurrentLevel < MAX_LEVELS-3)) {
			stretchLarge2(pLevs[2], 565, 133, 715, 266, 0, 0, 449, 398, 1024.0f, INT_PACK_ALPHA(128));
		}

		// just a quick extra scan of the controllers - don't display the previews if
		// a user is holding left or right (eliminates a little flicker)
		if (nStageSelectExtra) {
			idx=4;
		} else {
			for (idx=0; idx<4; idx++) {
				st = maple_dev_status(ControllerState[idx]);
				if (NULL == st) {
			//	debug("maple_dev_status returned NULL!!\n");
					continue;
				}
				if ((st->buttons & (CONT_DPAD_UP|CONT_DPAD_DOWN)) == (CONT_DPAD_UP|CONT_DPAD_DOWN)) {
					continue;
				}
				if (st->joyx < -JOYDEAD) st->buttons |= CONT_DPAD_LEFT;
				if (st->joyx > JOYDEAD)  st->buttons |= CONT_DPAD_RIGHT;
				if (st->buttons & (CONT_DPAD_LEFT|CONT_DPAD_RIGHT)) {
					break;
				}
			}
		}

		if (idx>=4) {
			// Instructional text
			switch ((nFrames/90)%2) {
				case 1:
					if (!nStageSelectExtra) {
						CenterDrawFontZ(1024.1f, 0, 65, DEFAULT_COLOR, "Pull trigger for random");
						break;
					} 
					// else fall through

				case 0:
					CenterDrawFontZ(1024.1f, 0, 65, DEFAULT_COLOR, "Press A to select world");
					break;
			}

			CenterDrawFontZ(1024.1f, 0, 408, DEFAULT_COLOR, "B to go back");

			// World name
			CenterDrawFontZ(1024.1f, 0, 303, INT_PACK_COLOR(255,128,128,255), ShortWorldName[nCurrentLevel]);

			if (!nLocked) {
				if ((!nLevelOnly)&&(nCurrentLevel != LEVEL_RANDOM)) {
					// 50% thumbnails of the current level's 3 level previews are shown at the bottom (these are all at 75% trans or so)
					stretchLarge2(pLevs[nCurrent], 188, 350, 251, 399, 0, 400, 127, 499, 1025.0f, INT_PACK_ALPHA(224));
					stretchLarge2(pLevs[nCurrent], 288, 350, 351, 399, 128, 400, 255, 499, 1025.0f, INT_PACK_ALPHA(224));
					stretchLarge2(pLevs[nCurrent], 388, 350, 451, 399, 256, 400, 383, 499, 1025.0f, INT_PACK_ALPHA(224));
				}
			} else {
				CenterDrawFontBackgroundZ(1026.0f, 1, 190, DEFAULT_COLOR, INT_PACK_COLOR(255,0,0,224), "LOCKED");
			}
		} else {
			debug("Controller %d holding 0x%08x\n", idx, st->buttons);
		}
		
		pvr_list_finish();
		pvr_scene_end;

		if (gReturnToMenu) {
			sound_stop();
			return -1;
		}

		for (idx=0; idx<4; idx++) {
			if (NULL == ControllerState[idx]) {
				continue;
			}
		
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

			// Cheat and map analog up/down to the DPAD
			if (st->joyy < -JOYDEAD) st->buttons |= CONT_DPAD_UP;
			if (st->joyy > JOYDEAD)  st->buttons |= CONT_DPAD_DOWN;
			if (st->joyx < -JOYDEAD) st->buttons |= CONT_DPAD_LEFT;
			if (st->joyx > JOYDEAD)  st->buttons |= CONT_DPAD_RIGHT;
			if ((st->ltrig>=127)||(st->rtrig>=127)) st->buttons|=CONT_Z;

			// Check buttons
			if (0==(lastbuttons[idx] & st->buttons)) {
				lastbuttons[idx]=st->buttons;
	
				if (!nStageSelectExtra) {
					if ((st->buttons & CONT_DPAD_LEFT)&&(nCurrentLevel > 0)) {
						// don't cache left or right
						lastbuttons[idx]&=~CONT_DPAD_LEFT;
						ScrollRight(pLevs, nCurrentLevel);
						nCurrentLevel--;
					}
					if ((st->buttons & CONT_DPAD_RIGHT)&&(nCurrentLevel < MAX_LEVELS-3)) {	// -1 and minus heaven and hell
						// don't cache left or right
						lastbuttons[idx]&=~CONT_DPAD_RIGHT;
						ScrollLeft(pLevs, nCurrentLevel);
						nCurrentLevel++;
					}
				}
				if ((st->buttons & CONT_DPAD_UP)&&(!nLevelOnly)) {
					if (nCurrentLevel==LEVEL_HELL) {
						nCurrentLevel=nLastLevel;
						ScrollFromHell(pLevs, nCurrentLevel);
						nStageSelectExtra=0;
						stairway=0;
					} else {
						if (stairway < 0) {
							stairway=1;
						} else {
							stairway++;
							if (stairway==7) {
								ScrollToHeaven(pLevs, nCurrentLevel);
								nLastLevel=nCurrentLevel;
								nStageSelectExtra=1;
								nCurrentLevel=LEVEL_HEAVEN;
							}
						}
					}
				}
				if ((st->buttons & CONT_DPAD_DOWN)&&(!nLevelOnly)) {
					if ((nCurrentLevel==LEVEL_HEAVEN)||(nCurrentLevel == LEVEL_RANDOM)) {
						nCurrentLevel=nLastLevel;
						ScrollFromHeaven(pLevs, nCurrentLevel);
						nStageSelectExtra=0;
						stairway=0;
					} else {
						if (stairway > 0) {
							stairway=-1;
						} else {
							stairway--;
							if (stairway==-6) {
								ScrollToHell(pLevs, nCurrentLevel);
								nLastLevel=nCurrentLevel;
								nStageSelectExtra=1;
								nCurrentLevel=LEVEL_HELL;
							}
						}
					}
				}

				if ((!nLocked)&&(st->buttons & (CONT_START|CONT_A))) {
					fContinue=false;
					break;
				}

				if (((!nStageSelectExtra)) && (st->buttons&CONT_Z)) {
					// Random Level - disallows heaven and hell due to code complications
					nLastLevel=ScrollToRandom(pLevs, nCurrentLevel);

					nStageSelectExtra=1;
					nCurrentLevel=LEVEL_RANDOM;
					fContinue=false;
					break;
				}

				if (st->buttons & CONT_B) {
					// backing up
					ShowBlack();
					return -1;
				}
			}
		}
	}

	if (nLevelOnly) {
		if (nCurrentLevel == LEVEL_RANDOM) {
			nCurrentLevel=nLastLevel;
			nStageSelectExtra=0;
		} else {
			ScrollStages(2, nStageSelectExtra, nCurrentLevel);
		}
		level=nCurrentLevel;
	} else {
		// Now transition to the stage select
		if (nCurrentLevel == LEVEL_RANDOM) {
			stage=ch_rand()%3;
			nCurrentLevel=nLastLevel;
			level=nCurrentLevel;
			nStageSelectExtra=0;
		} else {
			ScrollStages(1, nStageSelectExtra, nCurrentLevel);
			stage=1;
			level=nCurrentLevel;
		}

		// now do the select
		fContinue=1;
		while (fContinue) {
			int nCurrent;

			// Start drawing the frame
			BeginScene();
			pvr_list_begin(PVR_LIST_OP_POLY);
			// Opaque polygons

			if (nStageSelectExtra) {
				nCurrent=3;
			} else {
				nCurrent=1;
			}
			
			// Show currently selected at 50% of 450x400, move it upwards
			stretchLarge2(pLevs[nCurrent], 207, 50, 432, 250, 0, 0, 449, 398, 1024.0f, INT_PACK_ALPHA(255));

			pvr_list_finish();

			pvr_list_begin(PVR_LIST_TR_POLY);
			// Transparent polygons

			// Instructional text
			CenterDrawFontZ(1024.1f, 0, 265, DEFAULT_COLOR, "Press A to select stage");
			CenterDrawFontZ(1024.1f, 0, 408, DEFAULT_COLOR, "B to go back");

			// the current level's 3 level previews are shown at the bottom, with the current most solid
			stretchLarge2(pLevs[nCurrent], 138, 300, 251, 399, 0, 400, 127, 499, 1025.0f, INT_PACK_ALPHA(stage==0?255:124));
			stretchLarge2(pLevs[nCurrent], 263, 300, 376, 399, 128, 400, 255, 499, 1025.0f, INT_PACK_ALPHA(stage==1?255:124));
			stretchLarge2(pLevs[nCurrent], 388, 300, 501, 399, 256, 400, 383, 499, 1025.0f, INT_PACK_ALPHA(stage==2?255:124));
			
			// Draw the main picture
			SortFullPicture(disc_txr, txr_controlwork, 1023.9f);

			pvr_list_finish();
			pvr_scene_end;

			if (gReturnToMenu) {
				sound_stop();
				return -1;
			}

			for (idx=0; idx<4; idx++) {
				if (NULL == ControllerState[idx]) {
					continue;
				}
			
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

				// Cheat and map analog up/down to the DPAD
				if (st->joyy < -JOYDEAD) st->buttons |= CONT_DPAD_UP;
				if (st->joyy > JOYDEAD)  st->buttons |= CONT_DPAD_DOWN;
				if (st->joyx < -JOYDEAD) st->buttons |= CONT_DPAD_LEFT;
				if (st->joyx > JOYDEAD)  st->buttons |= CONT_DPAD_RIGHT;

				// Check buttons
				if (0==(lastbuttons[idx] & st->buttons)) {
					lastbuttons[idx]=st->buttons;
					if ((st->buttons & CONT_DPAD_LEFT)&&(stage > 0)) {
						stage--;
					}
					if ((st->buttons & CONT_DPAD_RIGHT)&&(stage < 2)) {	
						stage++;
					}

					if (st->buttons & (CONT_START|CONT_A)) {
						fContinue=false;
						break;
					}

					if (st->buttons & CONT_B) {
						// backing up
						ScrollStages(-1, nStageSelectExtra, nCurrentLevel);
						goto startlevelselect;
					}
				}
			}
		}
	}

	// Copy selected tile to txr_misc (if not already there)
	if (nStageSelectExtra) {
		if (pLevs[3] != txr512_misc) {
			// this will probably always need to be copied
			debug("Copying bonus texture...\n");
			PVR_MEMCPY(txr512_misc, pLevs[3], PAGE512);
		}
	} else {
		if (pLevs[1] != txr512_misc) {
			// this should not be needed on the Dreamcast ;)
			debug("Copying selected texture...\n");
			PVR_MEMCPY(txr512_misc, pLevs[1], PAGE512);
		}
	}

	return 0;
}

// Main wrapper for menuing system
// 1 to break main loop and repeat, 0 to repeat 
// Basically, 1 returns to the title page, and 0 will
// immediately recall this function.
int HandleMenus() {
	int idx;
	int PlayerSelectRecall=0;

	for (idx=LEVEL_CANDY; idx<=LEVEL_WATER; idx++) {
		if (0 == (gGame.UnlockFlags&(1<<(SHIFT_EARNED_STAGE+idx)))) {
			break;
		}
	}
	continuelevel=idx;
	for (idx=LEVEL_NZ; idx<=LEVEL_WATER; idx++) {
		if (0 == (gGame.UnlockFlags&(1<<(SHIFT_HARD_STAGE+idx)))) {
			break;
		}
	}
	hardcontinuelevel=idx;
	gIsHardStory=0;		// default to normal mode

	if (!inDemo) {
		switch (doMenu()) {
		case MENU_CANCEL:			// cancel
			return 0;

		case MENU_MULTIPLAYER:		// multiplayer
			// We need to preload anything that needs CD access,
			// so the music is not interrupted once started.
			sound_stop();
		
			// wait for any rendering to complete
			waitPVRDone();

			// Get the whiteout onscreen (to mask loading time)
			BeginScene();
			pvr_list_begin(PVR_LIST_OP_POLY);
			// Opaque polygons
			pvr_list_finish();
			pvr_list_begin(PVR_LIST_TR_POLY);
			SortRect(1025.0f, 0, 0, 640, 480, INT_PACK_ALPHA(255), INT_PACK_ALPHA(255));
			pvr_list_finish();
			pvr_scene_end;

			// ** PLAYER SELECT **
			// Everything is in RAMdisk or CPU cache

			// ** WORLD SELECT **
			// Everything's in RAMdisk or CPU cache

			// Now that all disk access is complete, start the music
			sound_start(SONG_SELECT, 1);

#ifndef DEMO_BUILD
			// label used to loop the player select if backing out of level select
restartPlayerSelect:

			// wait for any rendering to complete
			waitPVRDone();
			if (-1 == doPlayerSelect(PlayerSelectRecall)) {
				return 0;
			}
			PlayerSelectRecall=1;
			// this label is used in multiplayer
//restartStageSelect:
			ShowBlack();
			// wait for any rendering to complete
			waitPVRDone();
			if (-1 == doStageSelect(0)) {
				goto restartPlayerSelect;
			}
#else
			// Random select players for demo build
			for (idx=0; idx<4; idx++) {
				int u;
				for (u=0; u<MAPLE_UNIT_COUNT; u++) {
					ControllerState[idx]=maple_enum_dev(idx, u);
					if ((ControllerState[idx] != NULL) && (ControllerState[idx]->info.functions & MAPLE_FUNC_CONTROLLER)) {
						break;
					}
				}
				herder[idx].type=ch_rand()%12;
				if (u>=MAPLE_UNIT_COUNT) {
					// Not found
					ControllerState[idx]=NULL;
					herder[idx].type|=PLAY_COMPUTER;
				}
			}

			level=GetRandomLevel();
			stage=ch_rand()%3;
#endif
			gGame.NumPlays++;

			if (0 == Game(1)) {
				sound_stop();
				sound_start(SONG_SELECT, 1);
				goto restartPlayerSelect;
			}
			return 1;

		case MENU_STORY:		// Story Mode
			if (-1 == QueryDifficulty()) {
				// aborted
				return 0;
			}

			level=0;
			gGame.NumPlays++;
			nContinues=PLAYER_CONTINUES;

			ShowBlack();

			if (0xbbaa == Game(0)) {
				// player won! Do credits!
				credzMain();
				if (!gReturnToMenu) {
					if (gIsHardStory) {
						EnableOmake();
					} else {
						if (gGame.UnlockFlags & ENABLE_HARD_MODE) {
							EnableEndCredits();
						} else {
							EnableHardMode();
						}
					}
					if (!gReturnToMenu) {
						herder[gHumanPlayer].score=StoryModeTotalScore;
						AddHighScore(gHumanPlayer);
					}
				}
			}
			return 0;

		case MENU_CONTINUE:		// Story Mode with continue for where we left off
			if (-1 == QueryDifficulty()) {
				// aborted
				return 0;
			}

			nContinues=PLAYER_CONTINUES;
			level=(gIsHardStory?hardcontinuelevel:continuelevel);

			ShowBlack();

			// only continue if a level is available
			if (level > 1) {
				if (-1 == doStageSelect(level)) {
					return 0;
				}
			} else {
				// only New Zealand is unlocked then
				level=0;
			}

			gGame.NumPlays++;

			if (0xbbaa == Game(0)) {
				// player won! Do credits!
				credzMain();
				if (!gReturnToMenu) {
					if (gIsHardStory) {
						EnableOmake();
					} else {
						if (gGame.UnlockFlags & ENABLE_HARD_MODE) {
							EnableEndCredits();
						} else {
							EnableHardMode();
						}
					}
					if (!gReturnToMenu) {
						herder[gHumanPlayer].score=StoryModeTotalScore;
						AddHighScore(gHumanPlayer);
					}
				}
			}
			return 0;

		case MENU_OMAKE:		// Special very hard game
			ShowBlack();
			doOmake();
			return 0;

		case MENU_HOWTOPLAY:	// do a short how to play slideshow
			sound_stop();
			flush_png_level_cache();
#ifndef _WIN32
			if (fs_copy(SONG_TITLE, RAM_TITLE)) {
				sound_start(RAM_TITLE, 1);
			}
#endif
			for (idx=-1; idx>-6; idx--) {
				int tmp=idx;
				doStory(&tmp);
				if (gReturnToMenu) {
					break;
				}
			}
			sound_stop();
			fs_unlink(RAM_TITLE);
			return 0;

		case MENU_HIGHSCORES:
			sound_stop();
			ShowHighScores(0x7fffffff);	// impossibly high so it'll never timeout
			return 0;

		case MENU_OPTIONS:		// options
			return doOptionsMenu();

		case MENU_GALLERY:		// gallery
			return doImageGallery();

		case MENU_MUSIC:		// music player
			return doMusicPlayer();

		case MENU_CREDITS:		// credits routine
			credzMain() ;
			return 0;
		}
		// No default case, fall through for demo
	}

	inDemo=1;
	level=GetRandomLevel();
	stage=ch_rand()%3;
	for (idx=0; idx<4; idx++) {
		herder[idx].type=PLAY_COMPUTER|(ch_rand()%12);
	}
	thd_sleep(100);
	return Game(1);
}

void AddHighScore(int nWinner) {
	int idx, nPos, x, y, ch;
	int nLet, fContinue;
	int nRetries=5;
	// if you change the legal string, there are a lot of magic numbers below referencing it
	char szLegal[]="ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!?.~";
	char szStr[]=
		"A B C D E F G H\n"
		"I J K L M N O P\n"
		"Q R S T U V W X\n"
		"Y Z 0 1 2 3 4 5\n"
		"6 7 8 9 ! ? . ~";
	char buf[64];

	nPos=-1;
	for (idx=0; idx<10; idx++) {
		debug("Winner: %5d  HighX: %5d\n", herder[nWinner].score, gGame.HighScore[idx]);
		if (gGame.HighScore[idx] < herder[nWinner].score) {
			nPos=idx;
			break;
		}
	}
	if (nPos == -1) {
		// not a high score after all
		return;
	}

	// reuse the Hell theme for credits
	sound_stop();
	sound_start(SONG_HELL, 1);

	// move the other scores down
	for (idx=9; idx>=nPos+1; idx--) {
		gGame.HighScore[idx]=gGame.HighScore[idx-1];
		memcpy(gGame.HighName[idx], gGame.HighName[idx-1], 4);
	}

	// Load the background graphics
	load_png_block("gfx/Menu/World1.png", disc_txr, 0);
	load_png_block("gfx/Menu/World2.png", txr_misc[0], 0);

	ch=0;
	nLet=0;
	fContinue=1;

	while (fContinue) {
		cont_state_t * st=NULL;

		// now get the new name from the user
		BeginScene();

		// Opaque polygons
		pvr_list_begin(PVR_LIST_OP_POLY);
		gGfxDarken=128;	
		SortFullPicture(disc_txr, txr_misc[0], 1000.0f);
		gGfxDarken=0;
		pvr_list_finish();

		// transparent polygons
		pvr_list_begin(PVR_LIST_TR_POLY);
		CenterDrawFontBreaksZ(1002.0f, 0, 180, DEFAULT_COLOR, szStr);

		// highlight the current letter
		x=ch%8*32;
		y=ch/8*22;
		SortRect(1001.0f, x+190, y+170, x+14+210, y+20+190, INT_PACK_COLOR(192,255,0,0), INT_PACK_COLOR(192,255,0,0));
		buf[0]=szLegal[ch];
		buf[1]='\0';
		DrawFontZ(1003.0f, 0, x+200, y+180, INT_PACK_COLOR(192,32,32,255), buf);

		strcpy(buf, "");
		for (idx=0; idx<nLet; idx++) {
			buf[idx]=gGame.HighName[nPos][idx];
		}
		buf[idx]='\0';
		DrawFontZ(1002.0f, 1, 316, 120, INT_PACK_COLOR(255, colr[nWinner], colg[nWinner], colb[nWinner]), buf);

		// add the herder directly, without the sprite overhead
		addPage2(txr_herder[nWinner], 264, 120, 54, 200, 86, 236, DEFAULT_COLOR, 1002.0f);

		// draw in his score
		sprintf(buf, "Score: %d", (int)herder[nWinner].score);
		CenterDrawFontZ(1002.0f, 0, 70, INT_PACK_COLOR(255, colr[nWinner], colg[nWinner], colb[nWinner]), buf);

		// and instructions!
		CenterDrawFontBreaksZ(1002.0f, 0, 320, DEFAULT_COLOR, "You got a high score!\n \n'A' to select\n'B' to erase");

		pvr_list_finish();

		// draw it
		pvr_scene_end;

		// get a new control input from user
		if (gReturnToMenu) {
			sound_stop();
			return;
		}

		// the code below is normally a loop ;)
		if (NULL == ControllerState[nWinner]) {
			// no controller - drop out
			fContinue=false;
			break;
		}
	
		st = maple_dev_status(ControllerState[nWinner]);
		if (NULL == st) {
//				debug("maple_dev_status returned NULL!!\n");
			nRetries--;
			if (nRetries) continue;
			// controller failed? drop out.
			fContinue=false;
			break;
		}

		/* If we failed the read, then don't process it. This happens when controls are */
		/* unplugged for one frame (or so?) */
		if ((st->buttons & CONT_DPAD_UP) && (st->buttons & CONT_DPAD_DOWN)) {
			/* Up and down at once? That's crazy! ;) */
			continue;
		}

		// Cheat and map analog up/down to the DPAD
		if (st->joyy < -JOYDEAD) st->buttons |= CONT_DPAD_UP;
		if (st->joyy > JOYDEAD)  st->buttons |= CONT_DPAD_DOWN;
		if (st->joyx < -JOYDEAD) st->buttons |= CONT_DPAD_LEFT;
		if (st->joyx > JOYDEAD)  st->buttons |= CONT_DPAD_RIGHT;

		// Check buttons
		if (0==(lastbuttons[nWinner] & st->buttons)) {
			lastbuttons[nWinner]=st->buttons;

			if ((st->buttons & CONT_DPAD_LEFT)&&(ch > 0)&&(nLet<3)) {
				ch--;
			}
			if ((st->buttons & CONT_DPAD_RIGHT)&&(ch < 39)&&(nLet<3)) {	
				ch++;
			}
			if ((st->buttons & CONT_DPAD_UP)&&(ch >= 8)&&(nLet<3)) {	
				ch-=8;
			}
			if ((st->buttons & CONT_DPAD_DOWN)&&(ch <= 31)&&(nLet<3)) {	
				ch+=8;
			}

			if (st->buttons & (CONT_START|CONT_A)) {
				if ((nLet==3)||(szLegal[ch]=='~')) {
					sound_effect(snd_sheep[ZAPPEDA+ch_rand()%3], SHEEPVOL);
					// all scores entered
					fContinue=false;
					break;
				} else {
					// otherwise it's valid
					gGame.HighName[nPos][nLet]=szLegal[ch];
					nLet++;
					sound_effect(snd_sheep[ZAPPEDA+ch_rand()%3], SHEEPVOL);
					if (nLet >= 3) {
						nLet=3;
						ch=39;
					}
				}
			}

			if (st->buttons & CONT_B) {
				// backing up
				if (nLet>0) {
					sound_effect(snd_sheep[ZAPPEDA+ch_rand()%3], SHEEPVOL);
					nLet--;
				}
			}
		}
	}

	gGame.HighScore[nPos]=herder[nWinner].score;
	for (idx=nLet; idx<4; idx++) {
		gGame.HighName[nPos][idx]='\0';
	}
	gGame.HighName[nPos][3]='\0';		// this is just to be extra safe

	sound_stop();
	if (!gReturnToMenu) {
		ShowHighScores(20*60+255);		// show for up to 20 seconds plus fade
	}
}

void ShowHighScores(int nTimeout) {
	int fContinue, idx;
	char buf[64];
//	int txrX[12]={  0, 48, 96,144,192,  0, 48, 96,144,192,192,192};
//	int txrY[12]={192,192,192,192,192,192,192,192,192,192,144,144};
//	pvr_ptr_t txrPage[12]={ txr_misc[1],txr_misc[1],txr_misc[1],txr_misc[1],txr_misc[1],
//							txr_misc[2],txr_misc[2],txr_misc[2],txr_misc[2],txr_misc[2],
//							txr_misc[1],txr_misc[2] };
	int nCol=0, nCol2;
	
	sound_stop();

	// Load the background graphics - this is all cached
	load_png_block("gfx/Menu/World1.png", disc_txr, 0);
	load_png_block("gfx/Menu/World2.png", txr_misc[0], 0);
	// load the faces which have all the characters on them
	//load_png_block("gfx/Players/angl/angl_head.png", txr_misc[1], 0);
	//load_png_block("gfx/Players/devl/devl_head.png", txr_misc[2], 0);

	// now we can start music
	sound_start(SONG_HISCORE, 1);	
	fContinue=nTimeout;

	while (fContinue > 0) {
		fContinue--;
		// fading out
		if (fContinue < 256) {
			if (fContinue < gGame.MVol*28) {
				set_sound_volume(fContinue, -1);
			}
			fContinue-=7;
		}

		if (nFrames%12 == 0) {
			nCol++;
			if (nCol>3) nCol=0;
		}

		// now display the current top herders
		BeginScene();

		// Opaque polygons
		pvr_list_begin(PVR_LIST_OP_POLY);
		gGfxDarken=128;	
		SortFullPicture(disc_txr, txr_misc[0], 1000.0f);
		gGfxDarken=0;
		pvr_list_finish();

		// transparent polygons
		pvr_list_begin(PVR_LIST_TR_POLY);
		CenterDrawFontZ(1002.0f, 0, 44, DEFAULT_COLOR, "Story Mode");
		CenterDrawFontZ(1002.0f, 1, -1, DEFAULT_COLOR, "Top Herders");
		CenterDrawFontZ(1002.0f, 0, -1, DEFAULT_COLOR, " ");		// just to increment the cursor
		nCol2=nCol;
		for (idx=0; idx<10; idx++) {
			char a,b,c;
			a=gGame.HighName[idx][0];
			b=gGame.HighName[idx][1];
			c=gGame.HighName[idx][2];
			if (a=='\0') a=' ';
			if (b=='\0') b=' ';			
			if (c=='\0') c=' ';
			// 0 is the top score
			sprintf(buf, "%2d  %c%c%c  %6d ", idx+1, a, b, c, gGame.HighScore[idx]);
			CenterDrawFontZ(1002.0f, 1, 118+idx*32, INT_PACK_COLOR(255, colr[nCol2], colg[nCol2], colb[nCol2]), buf);

			nCol2++;
			if (nCol2 > 3) {
				nCol2=0;
			}
		}
		pvr_list_finish();

		// draw it
		pvr_scene_end;

		if (isStartPressed()) {
			if (fContinue>255) {
				fContinue=255;
			}
		}

		// get a new control input from user
		if (gReturnToMenu) {
			break;
		}
	}

	sound_stop();
	// reset sound volume
	set_sound_volume(gGame.MVol*28, -1);
	gGfxDarken=0;
}

