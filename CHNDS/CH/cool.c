/****************************************/
/* Cool Herders                         */
/* http://harmlesslion.com              */
/* Copyright 2009 HarmlessLion LLC      */
/* cool.c                               */
/****************************************/

/* Ported to KOS 1.1.x by Dan Potter */
 
#include <nitro.h>		// NITRO
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <nitro/os.h>	// NITRO

#include "kosemulation.h"

#include "sprite.h"
#include "cool.h"
#include "sound.h"
#include "rand.h"
#include "levels.h"
#include "collide.h"
#include "sheep.h"
#include "font.h"
#include "menu.h"
#include "control.h"
#include "sound.h"
#include "vmu_logo.h"
#include "pathfind.h"
#include "storymode.h"
#include "mapscr_manager.h"
#include "menuscr_manager.h"
#include "menu.h"
#include "special.h"
#include "wireless\chwireless.h"
#include "wireless\chwh.h"
#include "..\minigames\waterworksmini.h"
#include "..\minigames\candymini.h"

void load_data();
void DoSplashes();
void scale_gfx(char *szFile, char *pDat);
int AddPush(int idx, int gridx, int gridy);
void HandleMultiplayer();

/* game variables */
int nFrames;
int level, stage;
int nContinues;
unsigned int nGameFrames;
volatile unsigned int myJiffies=0;
int StoryModeTotalScore;
int isMultiplayer;		// 0 = no, story mode. 1 = yes, solo. 2 = yes, wireless
int gDontCheckLid=0;	// set to not check DS lid during game saves
int g_DestructiblesLeft;
int nMainScrollTarget, nScrollOffX, nScrollOffY;
int nText2DY=0;
int nTotalSheep;
int gStageSpecialEffect;
int gBonusWinFlag;
int nHowToPlayFlags = 0;
int nHowToPlayTime = 0;
int nHowToPlayPhase = 0;
pvr_ptr_t txr_sprites=INVALID_PTR;
pvr_ptr_t txr_level=INVALID_PTR;
OSOwnerInfo OwnerInformation;		// NITRO (for TWL, make OSOwnerInfoEx)

pvr_ptr_t txr_misc=INVALID_PTR, pal_misc=INVALID_PTR;
pvr_ptr_t pal_tmp[4] = { INVALID_PTR, INVALID_PTR, INVALID_PTR, INVALID_PTR };
pvr_ptr_t txr_sheep=INVALID_PTR;
pvr_ptr_t txr_herder[6]={INVALID_PTR, INVALID_PTR, INVALID_PTR, INVALID_PTR, INVALID_PTR, INVALID_PTR};
TXRMAP txrmap_sheep[50];
TXRMAP txrmap_sprites[35];
pvr_ptr_t pal_sprites=INVALID_PTR, pal_sheep=INVALID_PTR, pal_level=INVALID_PTR, pal_herder[6]={INVALID_PTR, INVALID_PTR, INVALID_PTR, INVALID_PTR, INVALID_PTR, INVALID_PTR};

extern int gStageSpecialEffect;
extern char StoryIntroText[][25];
extern int StoryModeTotalScore;
extern int SheepSpeed;
extern unsigned int lastbuttons[4];
extern int nFrameSpeed;
extern SpecialStruct specialDat[4][8];
extern int sSysState;

char	bSpecialDim=0;
char	szLastStoryCmd[16];
int		nBgAnimA, nBgAnimB, nBgAnimC;
int		nBgSpeedA, nBgSpeedB, nBgSpeedC;
int		nBgCountA, nBgCountB, nBgCountC;
int		idx, inDemo;
int	    nTimeLeft;
fx16	sFx;
int		fPaused;
int		nGameCountdown=-1;

static int HerderAnimSeq[]={ 0, 0, 1, 2, 2, 3 };

char DefaultHigh[10][4] = {
	"BAA",
	"TNK",
	"BNK",
	"TRS",
	"FOX",
	"RBT",
	"SYL",
	"MAR",
	"FLO",
	"SSI",
};

// the big font has no digits...
char *szRound[] = {
	"OVERTIME",		// 0 (used for > 9)
	"ROUND ONE",
	"ROUND TWO",
	"ROUND THREE",
	"ROUND FOUR",
	"ROUND FIVE",
	"ROUND SIX",
	"ROUND SEVEN",
	"ROUND EIGHT",
	"ROUND NINE"
};

// offset targets (main is always 0,0)
int nScrollOffTargets[10] = {
	0,0,
	0,-192,
	-256,0,
	0,192,
	256,0
};

extern char szNames[13][8];
extern int nSoundGroups[13];

// NOTE: This and pal_herder[] have 6 entries for the creditz function,
// EVERYWHERE in the game 4 are assumed.
HerdStruct herder[6] ;

// menu.c
extern u16 colrgb[4];

// static
int RunGame(int inMultiplayer);

/*****************/

/******************/
/* Error Handling */
/******************/

/* Disable this function in a release build to prevent debug from printing */
void debug(char *str, ...) {
#if !defined(SDK_FINALROM)
	char buf[256];
	va_list va;

	va_start(va, str);
	STD_TVSNPrintf(buf, 256, str, va);
	va_end(va);
	OS_TPrintf("[%06d.%02d] %s", myJiffies/60, (myJiffies%60)*1000/60, buf);		// approximate runtime in seconds
	if (NULL == strchr(str, '\n')) OS_TPrintf("\n");
#endif
}

// this handles color cycling the text on the top line for the special
// similar to the hi score rotation but for the top only, and faster
extern int nNextY;
void SpecialHIntr() {
	static int nCol=0;
	
	// this is called twice per frame for the top 16 pixels
	// so that we can set the text color dynamically
	s32 val = GX_GetVCountEqVal();
	if (val == 0) {
		// don't change color during how to play
		if (nHowToPlayPhase == 0) {
			// top of screen - set a color
			*(u16*)(HW_PLTT+30) = colrgb[nCol>>3];	// set color (don't flicker too fast!)
			nCol++;
			if (nCol >= 32) {
				nCol=0;
			}
			// next int after this text row
			GX_SetVCountEqVal(val+16);
		} 
	} else {
		// after the first line, reset to white
		*(u16*)(HW_PLTT+30) = 0x7fff;
		GX_SetVCountEqVal(0);	
	}
}

// this draws the top picture during the menu, and allows the menu system to update it
// returns 0 if it did not process the frame (meaning no end of frame occurred)
int HandleTopMenuView(int change) {
	if (change) {
		if ((nMainScrollTarget&SCROLL_TARG_NONE) == change) {
			// same target, don't change
			return 0;
		}
		if (change != SCROLL_TARG_MAIN) {
			if ((nScrollOffX == nScrollOffTargets[change<<1]) && (nScrollOffY == nScrollOffTargets[(change<<1)+1])) {
				// zero it to clear any previous 'none'
				nMainScrollTarget=0;
				// we are already there, no change
				return 0;
			}
		}
		nMainScrollTarget=change | SCROLL_TARG_MAIN;
		return 0;
	}
	
	if ((nMainScrollTarget&SCROLL_TARG_NONE) == SCROLL_TARG_NONE) {
		return 0;
	}

	// do we have a target for the upper window?
	// movement is 16 pixels at a time, that should never overshoot 256 or 192
	if (nMainScrollTarget) {
		if (nMainScrollTarget & SCROLL_TARG_MAIN) {
			// always go back to main first
			nScrollOffX += sgn(nScrollOffTargets[0]-nScrollOffX)<<4;
			nScrollOffY += sgn(nScrollOffTargets[1]-nScrollOffY)<<4;
			if ((nScrollOffX == nScrollOffTargets[0]) && (nScrollOffY == nScrollOffTargets[1])) {
				// got there, clear the flag
				nMainScrollTarget &= ~SCROLL_TARG_MAIN;
				// clear the 2d screen
				Clear2D();
				nText2DY=0;	// prevent accumulated errors
				// set the new text, if there is one
				switch (nMainScrollTarget) {
					case SCROLL_TARG_NORTH:
						WriteFont2D(24, 0, "Extras");
						break;
    
					case SCROLL_TARG_SOUTH:
						WriteFont2D(24, 0, "Options");
						break;
    
					case SCROLL_TARG_EAST:
						WriteFont2D(24, 0, "Battle Mode");
						break;
    
					case SCROLL_TARG_WEST:
						WriteFont2D(24, 0, "Story Mode");
						break;
				}
			}
			// also scroll the 2d screen
			if (nText2DY > 0) {
				nText2DY-=2;
				SetOffset2D(0, nText2DY);
			}
		} else {
			// to side image
			nScrollOffX += sgn(nScrollOffTargets[nMainScrollTarget<<1]-nScrollOffX)<<4;
			nScrollOffY += sgn(nScrollOffTargets[(nMainScrollTarget<<1)+1]-nScrollOffY)<<4;
			if ((nScrollOffX == nScrollOffTargets[nMainScrollTarget<<1]) && (nScrollOffY == nScrollOffTargets[(nMainScrollTarget<<1)+1])) {
				// got there, clear the target
				nMainScrollTarget = 0;
			}
			// also scroll the 2d screen
			if (nText2DY < 18) {
				nText2DY+=2;
				SetOffset2D(0, nText2DY);
			}
		}
	}
	
	// now to get it onscreen
	pvr_scene_begin();
	// draw top menu image here, with transition
	SortMenuPictureX(0,0,31, nScrollOffX, nScrollOffY);
	pvr_scene_finish();
	
	return 1;
}


// this function picks a random level, aware of the unlock flags
int GetRandomLevel() {
	static int nLastRet=-1;
	int ret=-1;

	// Allows all stages
	do {
		ret=(int)(rand()%(MAX_LEVELS));
	} while (ret == nLastRet);

	nLastRet=ret;
	return ret;
}

/*****************/

int main(int /*argc*/, char */*argv*/[]) {
	int idx;
	int StartPressed;
	int fLoadTitle=0;
	int givemusicslack;
	int nOption;
	
	debug("Initializing Cool Herders " __DATE__ " for NDS...\n");

	/* Pre-allocate the static video buffers to reduce heap fragmentation */
	pvr_init_defaults();
	
	// get the owner info that we care about
	OS_GetOwnerInfo(&OwnerInformation);		// NITRO: for TWL make OS_GetOwnerInfoEx
	OwnerInformation.nickNameLength = ReparseName((unsigned char*)OwnerInformation.nickName, OwnerInformation.nickNameLength);
	debug("Owner nickname: '%s'", OwnerInformation.nickName);
	debug("Owner color: %d", OwnerInformation.favoriteColor);

	// First things first we initialize our globals to defaults! :)
	gGame.Options.Timer=60;			// 0,30,60,90
	gGame.Options.Rounds=1;			// 1,2,3
	gGame.Options.Powers=POW_POWERUPS | POW_SPECIALS;
	gGame.Options.Skill=1;			// 0=Lost, 1=Normal, 2=Persistent
	gGame.Options.SheepSpeed=0;		// -1=slower, 0=normal, 1=faster
	gGame.SVol=8;			// 0-8
	gGame.MVol=8;			// 0-8
	gGame.Options.CPU=1;	// 0=no CPU in multiplayer, 1=CPU takes empty slots in multiplayer
	gGame.AutoSave=0;		// 0=no autosave, 1=autosave
	gGame.SaveSlot=0;
	gGame.continuelevel=LEVEL_NZ;	// first level to continue on (last level reached)
	
	memset(&gMPStuff, 0, sizeof(gMPStuff));
	
	for (idx=0; idx<10; idx++) {	// high scores - 0 is highest!
		strcpy(gGame.HighName[idx], DefaultHigh[idx]);	// 3 initials & NUL
		gGame.HighScore[idx]=(10-idx)*2500;				// Actual score obtained
	}
	for (idx=0; idx<5; idx++) {
		gGame.ChallengeScore[idx]=0;	// best scores in the challenge levels
		strcpy(gGame.ChallengeName[idx], DefaultHigh[idx]);	// 3 initials & NUL
	}

	MenuScr_Init();

	// Set up the save system
	vmuInit();
	
LoopDemo:
	if (gGame.AutoSave) {
		doVMUSave(1, gGame.SaveSlot);
	}

	// Zero out the herders struct
	for (idx=0; idx<4; idx++) {
		memset(&herder[idx], 0, sizeof(HerdStruct));
	} 

	// if there's any music playing, can it
	sound_stop();

	// Display initial copyright screen
	doLogoFade("gfx/Misc/HarmlessLogoSized.bmp", "gfx/Misc/Nintendo.bmp");
	
	// and the WIP screen
	DisableControlsTillReleased();
	doLogoFade("gfx/Misc/WIP.bmp", "gfx/Misc/WIP2.bmp");

	// set subscreen back to 256 color
	// change background to 16-color
	G2S_SetBG0Control(GX_BG_SCRSIZE_TEXT_256x256, GX_BG_COLORMODE_256, GX_BG_SCRBASE_0x0800, GX_BG_CHARBASE_0x00000, GX_BG_EXTPLTT_01);
	ShowBlack();	// sets some subscreen states
	
	GXS_SetVisiblePlane(0);
	GXS_SetMasterBrightness(0);

	inDemo=0;

	/* We loop the music because there are too many menus to manually loop */
	/* However, we want to be notified when it hits the end, so we check that */
	sound_start(SONG_TITLE, 1);

	/* make sure Start is released before we proceed */
	DisableControlsTillReleased();
	fLoadTitle=true;
	debug("Waiting for START...\n");
	nFrames=0;
	givemusicslack=0;
	gStageSpecialEffect = 0;
	
	MenuScr_Init();
	MenuScr_InitMenu(MENU_PRESSSTART);
	OS_WaitVBlankIntr();

	while(1) {
		if (fLoadTitle) {
			// hide screen while loading to prevent glitches
			GX_SetVisiblePlane(0);
			
			if (gGame.AutoSave) {
				if (doVMUSave(1, gGame.SaveSlot)) {
					givemusicslack=1;
				}
			}
			 
			/* title page needs ALL of 3D VRAM (512k!). So nothing 3d is valid */
			/* after this is loaded. We just use TXR and PAL addresses 0 */
			/* and hah! Even more fun, we don't have enough CPU RAM to load it */
			/* all at once. We just split the image into 3 for now rather than */
			/* mess with loading directly to VRAM. Need 128k free for this. */
			load_bmp_block_mask("gfx/Misc/mainmenu1.bmp", NULL, 0, 0, -1);
			load_bmp_block_mask("gfx/Misc/mainmenu2.bmp", NULL, 128*1024, 0, -1);
			load_bmp_block_mask("gfx/Misc/mainmenu3.bmp", NULL, 256*1024, 0, -1);
			fLoadTitle=false;
			
			// Reset the music counter, and make sure it's playing
			sound_start(SONG_TITLE, 1);
			
			// used to handle the menu scrolling
			nMainScrollTarget = 0;
			nScrollOffX=0;
			nScrollOffY=0;
			Clear2D();
			
			// TODO: HACK FOR DEMO
			WriteFont2D(22, 0, "WORK IN PROGRESS");
			
			// turn it all back on
			GX_SetVisiblePlane(GX_PLANEMASK_BG0 | GX_PLANEMASK_BG1 | GX_PLANEMASK_BG2);
		}

		// handles frame end
		HandleTopMenuView(0);

		StartPressed = 0;
		if (CONT_START == isStartPressed()) {
			// Seed random number generator
			ch_srand(myJiffies);
			StartPressed=1;
			DisableControlsTillReleased();
		}

		nOption = 0;
		if (((MenuScr_DrawFrame(&nOption)) || (StartPressed)) && (CONT_A != isStartPressed())) {
			if (0 == nOption) 
			{
				debug("Leaving title page...\n");

				sound_effect_system(SND_CLICK, SHEEPVOL);
	
				MenuScr_CloseMenu(0);

				fLoadTitle=true;
				
				HandleMainMenus();
				
				debug("Back from menu system\n");
				
				if (!inDemo) {
					// on return from main menu, come back to the Press Start screen
					MenuScr_Init();
					MenuScr_InitMenu(MENU_PRESSSTART);
	
					/* reset frame count to prevent demo ;) */			
					nFrames = 1;
					nNumberLoops = 0;
				}
			}
		}

		// Activate demo after a certain time
		// no need for post-music pause on this one, since demo does not
		// occur from the interactive menus
		if ((nNumberLoops > 0) || (inDemo)) {
			if ((givemusicslack) && (!inDemo)) {
				givemusicslack--;
				nNumberLoops = 0;		// allow one more iteration
			} else {
				level=GetRandomLevel();
				stage=(int)(rand()%3);
				inDemo=1;

				MenuScr_ResetMenu();
			
				debug("Starting demo...\n");
				fLoadTitle=true;
				for (idx=0; idx<4; idx++) {
					herder[idx].type=(PLAY_COMPUTER|(rand()%12));
				}
				Game(1);
				nFrames=0;

				ShowHighScores(20*60+255);		// show for up to 20 seconds plus fade

				MenuScr_Init();
				MenuScr_InitMenu(MENU_PRESSSTART);

				sound_start(SONG_TITLE, 1);
				inDemo=0;
			}
		}
	}

	fLoadTitle=true;
	goto LoopDemo;

	/* Not reached */
	return 0;
}

void ShowBlack() {
	// try and clear things up during blank
	OS_WaitVBlankIntr();

	// also shut down the subscreen
	GXS_SetVisiblePlane(0);
	GXS_SetMasterBrightness(0);	// reset brightness
	Clear2D();
	
	// Draw a black screen to prevent visible texture glitches
	pvr_scene_begin();
	// draw it
	pvr_scene_finish();
	// safe to reset brightness now
	GX_SetMasterBrightness(0);
	// but reset the global 3d darken
	Set3DDarken(0);
}

// scale out a level splash page
// pDat points to a multiline string to display (or NULL for none)
void scale_gfx(char *szFile, char *pDat) {
	int nOption;
	
	// Load the background graphics
	load_bmp_block_mask(szFile, NULL, txr_misc, pal_misc, -1);
	
	// uses the same block as story mode
	MapScr_ShowBlack();
	MenuScr_Init();
	Clear2D();
	MenuScr_InitMenu(MENU_STAGEINTRO_TEXT);	
	if (NULL != pDat) {
		MenuScr_UpdateMenuString(0, pDat);
	}

	for (idx=0; idx<=64; idx+=4) {
		// Start drawing the frame
		pvr_scene_begin(); 

		// in this new version the image is a full 256x192, so we can fill the screen with it
		// we start at about 50%, just like before (128x96 - 256x192)
		scaleimage(txr_misc, pal_misc, 64-idx, 48-(idx*3/4), 191+idx, 143+(idx*3/4), 31);
 
 		if (NULL != pDat) {
			// Draw the text cloud
			MenuScr_DrawFrame(&nOption);
 		}

		pvr_scene_finish();	
	}
	// pause so the screen is visible
	thd_sleep(1000);
	MenuScr_CloseMenu(1);
}

// Helper to deal with velocities on push tiles
// returns non-zero if we added any push
int AddPush(int idx, int gridx, int gridy) {
	int ret=0;
	// check the spot we're moving FROM for any push and add it in unless walking across it
	switch (LevelData[gridy][gridx].nPush) {
		case 1:	
			if (herder[idx].spr.xd==0) {
				herder[idx].spr.yd--; 
				ret=1;
			}
			break;

		case 2:	
			if (herder[idx].spr.yd==0) {
				herder[idx].spr.xd++;
				ret=2;
			}
			break;

		case 3: 
			if (herder[idx].spr.xd==0) {
				herder[idx].spr.yd++; 
				ret=3;
			}
			break;

		case 4:	
			if (herder[idx].spr.yd==0) {
				herder[idx].spr.xd--; 
				ret=4;
			}
			break;
	}
	return ret;
}

// does a little game over routine
// draw Zeus standing there onscreen, and have all the sheep jump out and
// run off screen, fading out.
// This function does rely on the sheep and player being previously initialized by a game
// Return 0 if it really is Game Over, or 1 if the level should be retried
int doGameOver() {
	char str[9]="AAAAAAAA";
	int nDarken;

	sound_stop();
	Clear2D();

	debug("Game Over\n");

	// get the background picture
	load_bmp_block_mask("gfx/Menu/World.bmp", NULL, txr_misc, pal_misc, -1);
	
	// recenter the screen
	SetOffset(0,0);

	// Move Zeus into position
	herder[gHumanPlayer].spr.x=112;
	herder[gHumanPlayer].spr.y=152;
	herder[gHumanPlayer].spr.tilenumber=DOWN_STAND_SPRITE;
	herder[gHumanPlayer].spr.z=(fx16)3296;

	// Now init all the sheep
	for (idx=0; idx<MAX_SHEEP; idx++) {
		sheep[idx].spr.x=herder[gHumanPlayer].spr.x;
		sheep[idx].spr.y=herder[gHumanPlayer].spr.y;
		sheep[idx].spr.z=(fx16)3588;
		sheep[idx].spr.xd=(int)(rand()%6)-3;
		sheep[idx].spr.yd=-1*(int)(rand()%(idx+1))-1;
		if (sheep[idx].spr.yd < -16) sheep[idx].spr.yd=-16;
		sheep[idx].spr.tilenumber=SheepAnimationFrame[(sheep[idx].spr.xd<0)?0:2][0];
		sheep[idx].animframe=0;
		sheep[idx].spr.alpha=31;
		sheep[idx].spr.is3D=true;
		sheep[idx].type=1;
	}

	// Check if the player wants to retry, if they have continues left
	if (nContinues) {
		char buf[64];
		int nResp=1;
		int nOption=0;
		
		// Set up the string
		sprintf(buf, "You failed!\n%d continues left\nTry Again?", nContinues);

		// first frame, so it's set before the menu draws
		pvr_scene_begin();
		// draw the background
		SortFullPictureX(GX_TEXFMT_PLTT256, txr_misc, pal_misc, 16);
		// Finally, sort our poor beleagured hero... 
		SortSprite(&herder[gHumanPlayer].spr, herder[gHumanPlayer].map, POLY_HERD);
		// draw it
		pvr_scene_finish();		

		// This is okay, since if they say no the remainder are thrown away anyway
		nContinues--;

continuemenu:
		sound_start(SONG_STORY, 1);

		// the pause menu has 'continue', 'save', 'quit' that we can reuse
		MenuScr_Init(); // First, we need to convert the secondary display to Menu mode
		MenuScr_InitMenu(MENU_PAUSE); // Then we need to tell it to begin drawing the pause menu

		while (!MenuScr_DrawFrame(&nOption)) {
			u16 btns;	// for the cheat code only, the menu subsystem handles the rest of it
			
			// this doesn't need to be in the loop, except for the cheat code
			CenterWriteFontBreak2D(2, 15, buf);
			
			pvr_scene_begin();
			
			// draw the background
			SortFullPictureX(GX_TEXFMT_PLTT256, txr_misc, pal_misc, 16);

			// Finally, sort our poor beleagured hero... 
			SortSprite(&herder[gHumanPlayer].spr, herder[gHumanPlayer].map, POLY_HERD);

			// draw it
			pvr_scene_finish();

			// Handle the player's input - up/down/left/right
			btns = GetController(gHumanPlayer);
				
			// wait for old keys to be released
			if (btns&lastbuttons[gHumanPlayer]) continue;
				
			// save old keys
			lastbuttons[gHumanPlayer]=btns;
				
			if (btns == CONT_DPAD_UP) {
				memmove(str, str+1, 8);
				str[7]='U';
				str[8]='\0';
			}
			if (btns == CONT_DPAD_DOWN) {
				memmove(str, str+1, 8);
				str[7]='D';
				str[8]='\0';
			}

			if (btns & CONT_DPAD_RIGHT) {
				nResp=0;
				memmove(str, str+1, 8);
				str[7]='R';
				str[8]='\0';
			}
			if (btns & CONT_DPAD_LEFT) {
				nResp=1;
				memmove(str, str+1, 8);
				str[7]='L';
				str[8]='\0';
			}

			if (strcmp(str, "UUDDLRLR")==0) {
				nContinues++;
				strcpy(str, "AAAAAAAA");
				sprintf(buf, "You failed!\n%d continues left\nTry Again?", nContinues);
			}
		}

		sound_stop();
		
		// close the menu
		MenuScr_CloseMenu(0);
		
		switch (nOption) {
			case MENU_CONTINUE:	
				return 1;		// retry the level (we already decremented the continues)
				
			case MENU_QUIT:
				break;			// user is done, fall through into code below
				
			case MENU_SAVE:
				doVMUSave(0, 0);
				goto continuemenu;
				
			default:
				goto continuemenu;
		}
	}

	// Now start the music, and the animation!
	sound_start(SONG_GAMEOVER, 0);
	
	// Draw the Game Over text
	Clear2D();
	CenterWriteFontBreak2D(4, 15, "GAME  OVER");
 
	for (;;) {
		int nSheepLeft;
		
		pvr_scene_begin();
		
		SortFullPictureX(GX_TEXFMT_PLTT256, txr_misc, pal_misc, 16);
	
		// Next, sort the sheep
		nSheepLeft=0;
		for (idx=0; idx<MAX_SHEEP; idx++) {
			if (sheep[idx].type > 0) {
				// move them here
				if (sheep[idx].spr.y < herder[gHumanPlayer].spr.y) {
					// apply gravity
					sheep[idx].spr.yd++;
				}

				sheep[idx].spr.x+=sheep[idx].spr.xd;
				sheep[idx].spr.y+=sheep[idx].spr.yd;

				if ((sheep[idx].spr.x > 238) || (sheep[idx].spr.x < 18)) {
					sheep[idx].type=0;
					continue;
				}
				if (sheep[idx].spr.x > 175) {
					sheep[idx].spr.alpha=(238-sheep[idx].spr.x)>>2;
				}
				if (sheep[idx].spr.x < 81) {
					sheep[idx].spr.alpha=(sheep[idx].spr.x-18)>>2;
				}

				if (sheep[idx].spr.y > herder[gHumanPlayer].spr.y) {
					// Hits the ground running!
					if (sheep[idx].spr.xd < 0) {
						sheep[idx].spr.xd-=10;
					} else {
						sheep[idx].spr.xd+=10;
					}
					sheep[idx].spr.y=herder[gHumanPlayer].spr.y;
					sheep[idx].spr.yd=0;
				}
				
				if (nFrames%4 == 0) {
					sheep[idx].animframe++;
					if (sheep[idx].animframe >= sheep[idx].maxframes) {
						sheep[idx].animframe=0;
					}
					sheep[idx].spr.tilenumber=SheepAnimationFrame[(sheep[idx].spr.xd<0)?0:2][sheep[idx].animframe];
				}

				SortSprite(&sheep[idx].spr, txrmap_sheep, POLY_SHEEP);
				nSheepLeft++;
			}
		}

		// Finally, sort our poor beleagured hero... or loser since it's game over
		SortSprite(&herder[gHumanPlayer].spr, herder[gHumanPlayer].map, POLY_HERD);

		// draw it
		pvr_scene_finish();

		// Wait for the music to end and all sheep to leave before exitting
		if ((nSheepLeft == 0) && (!musicisplaying())) {
			break;
		}
	}

	// quick fade out
	for (nDarken=0; nDarken<16; nDarken++) {
		GX_SetMasterBrightness(-nDarken);
		GXS_SetMasterBrightness(-nDarken);
	
		pvr_scene_begin();
		SortFullPictureX(GX_TEXFMT_PLTT256, txr_misc, pal_misc, 16);
		
		// Finally, sort our poor beleagured hero... or loser since it's game over
		SortSprite(&herder[gHumanPlayer].spr, herder[gHumanPlayer].map, POLY_HERD);

		pvr_scene_finish();
	}
	
	// reset everything to black
	ShowBlack();

	return 0;
}

// game wrapper to set up and clean up stuff neatly
int Game(int inMultiplayer) {
	int ret;
	
	// clear text layer
	Clear2D();
	SetOffset2D(4,0);
	
	// set up text color cycle interrupt
	GX_SetVCountEqVal(0);		// top of screen first
	OS_SetIrqFunction(OS_IE_V_COUNT, SpecialHIntr);
	OS_EnableIrqMask(OS_IE_V_BLANK | OS_IE_V_COUNT);
	GX_VCountEqIntr(true);	
	
	// now do the game function
	ret=RunGame(inMultiplayer);
	
	// now clean up
	isMultiplayer=0;				// now important due to frame speed hacks
	Clear2D();
	// stop interrupt and reset palette
	GX_VCountEqIntr(false);			// this is probably already done by end of frame, but need this for demo and quit options
	OS_DisableIrqMask(OS_IE_V_COUNT);
	OS_WaitVBlankIntr();			// wait a frame to be sure it stopped
	*(u16*)(HW_PLTT+30) = 0x7fff;	// set palette to white
	// and fix the windowing
   	G2_SetBlendBrightness(GX_BLEND_PLANEMASK_BG2, -9); // just the shadow layer
	GX_SetVisibleWnd(GX_WNDMASK_NONE);	// turn off the window
	return ret;
}

// wrapper to start the appropriate minigame, if there is one
void PlayMinigame(int nLevel) {
	switch (nLevel) {
		case LEVEL_CANDY:		// candyland minigame
			PlayCandylandMinigame();
			break;
		
		case LEVEL_WATER:		// waterworks minigame
			PlayWaterworksMinigame();
			break;
	}
}

// uses the globals to put some text at the top of the screen
extern int nNextY;		// from font.c
void UpdateHowToPlay() {
	char *p = NULL;
	char odd=0;		// set to 1 if the text is an odd number of characters wide (for half-width text offset)
	
	if (!bSpecialDim) {
		if ((nHowToPlayPhase == 9) && (nHowToPlayFlags&HOW_TO_PLAY_TIME)) {
			gStageSpecialEffect|=STAGE_EFFECT_NO_SHEEP;
		}

		if (nHowToPlayTime > 0) {
			nHowToPlayTime--;
		} else {
			if (nHowToPlayFlags & (1<<(nHowToPlayPhase-1))) {
				// times up and the flag is set, move on to the next phase
				nHowToPlayPhase++;		// the last bit if never set, so this never increments too far
				nHowToPlayTime = HOW_TO_PLAY_COUNTDOWN;
				Clear2D();
			}
		}

		switch (nHowToPlayPhase) {
			case 1:
				p="Use the D-Pad to move"; 
				odd=1;
				break;
			
			case 2:	
				p="Press A or B to fire lightning"; 
				break;
				
			case 3: 
				p="Use lightning to destroy crates";
				odd=1; 
				if (nHowToPlayTime == HOW_TO_PLAY_COUNTDOWN) {
					// we just changed mode
					gStageSpecialEffect|=STAGE_EFFECT_PWRUP_ALWAYS_BOOT; 
				}
				break;
				
			case 4: 
				p="Boots increase your speed";
				odd=1; 
				// putting this here ensures you will always have at least one bolt to proceed with
				if (nHowToPlayTime == HOW_TO_PLAY_COUNTDOWN) {
					// we just changed mode
					nHowToPlayFlags &= ~HOW_TO_PLAY_CRATES;
				}
				break;
				
			case 5: 
				p="Use lightning to destroy crates";
				odd=1; 
				if (nHowToPlayTime == HOW_TO_PLAY_COUNTDOWN) {
					// we just changed mode
					gStageSpecialEffect&=~STAGE_EFFECT_PWRUP_ALWAYS_BOOT; 
					gStageSpecialEffect|=STAGE_EFFECT_PWRUP_ALWAYS_BOLT; 
				}
				break;

			case 6: 
				p="Bolts increase your lightning";
				odd=1; 
				break;
				
			case 7: 
				p="Rub touch screen or hold L or\nR button while standing still\nto charge special attack";
				odd=1;
				if (nHowToPlayTime == HOW_TO_PLAY_COUNTDOWN) {
					// we just changed mode
					gStageSpecialEffect&=~STAGE_EFFECT_PWRUP_ALWAYS_BOLT;
				}
				break;
				
			case 8:
				// we can accidentally erase the default message
				p="A+B for LIGHTNING SHOCK";
				odd=1;
				break;
				
			case 9:
				// note: this phase is specially handled above - if the number changes, fix that ;)
				p="Capture sheep from the maze";
				odd=1;
				if (nHowToPlayTime == HOW_TO_PLAY_COUNTDOWN) {
					// we just changed mode
					gStageSpecialEffect&=~STAGE_EFFECT_NO_SHEEP;	// we turn this back on once sheep are released
					nHowToPlayFlags &= ~HOW_TO_PLAY_TIME;
				}
				break;
				
			case 10:
				p="Zap herders to free their sheep";
				odd=1;
				if (nHowToPlayTime == HOW_TO_PLAY_COUNTDOWN) {
					// we just changed mode
					gStageSpecialEffect|=STAGE_EFFECT_NO_SHEEP;		// turn it back on so the level can't be prematurely finished
					gStageSpecialEffect&=~STAGE_EFFECT_COMPUTER_FREEZE;
				}
				break;
				
			case 11:
				p="New sheep are also released\nover time";
				odd=1;
				if (nHowToPlayTime == HOW_TO_PLAY_COUNTDOWN) {
					// we just changed mode
					gStageSpecialEffect &= ~STAGE_EFFECT_NO_SHEEP;
					nHowToPlayFlags &= ~ HOW_TO_PLAY_TIME;
				}
				break;
				
			case 12:
				p="When all sheep are captured,\nor time runs out, the stage\nis over. Most sheep wins!";
				if (nHowToPlayTime == HOW_TO_PLAY_COUNTDOWN) {
					// we just changed mode
					gStageSpecialEffect&=~STAGE_EFFECT_FREEZE_TIMER;
				}
				break;
		}
		if (NULL != p) {
			CenterWriteFontBreak2D(0, 16, p);
			G2_SetWnd0Position(1, 1, 255, nNextY<<3);	// 1 pixel border on each side
			GX_SetVisibleWnd(GX_WNDMASK_W0);	// turn on the window
		   	G2_SetBlendBrightness(GX_BLEND_PLANEMASK_BG0|GX_BLEND_PLANEMASK_BG2, -9); // darken both the 3D screen and the shadow layer
		   	if (odd) {
		   		// half a character offset (Clear2D resets this)
		   		SetOffset2D(4,0);
		   	}
		}
	}
}

// draws the background tiles for the stage end
// SPRITE p should already be set up for the tile to draw
// draws the tiles specifically half-dark
extern TXRMAP LevelMap[300];
void DrawEndOfStageBackground(SPRITE *p) {
	int x,y;
	
	myG3_Color(10,10,10);
	
	for (x=0; x<256; x+=GRIDSIZE) {
		for (y=0; y<192; y+=GRIDSIZE) {
			p->x=x;
			p->y=y;
			SortSprite(p, LevelMap, POLY_BG);
		}
	}
	
	myG3_Color(31, 31, 31);
}

// nOrder must be an array of 4 ints to receive the sorted order of the herders
// this is also used by the map screen code. Returns how many sheep the winner
// has (so ties can be detected later)
int GetPlayerSortedOrder(int *pIn) {
	int nOrder[4] = { 0, 1, 2, 3 };		// static array so it sorts less often

	// find the highest count. We can also check for ties. In the case of a counting tie, computer players lose to humans.
	// simple bubbly sort
	for (;;) {
		int idx, tmp;
		for (idx=0; idx<3; idx++) {
			// this checks for a tie against computer and human needing a fix
			if ((herder[nOrder[idx]].sheep == herder[nOrder[idx+1]].sheep) && ((herder[nOrder[idx]].type&PLAY_MASK)==PLAY_COMPUTER) && ((herder[nOrder[idx+1]].type&PLAY_MASK)==PLAY_HUMAN)) {
				// swap them
				tmp=nOrder[idx];
				nOrder[idx]=nOrder[idx+1];
				nOrder[idx+1]=tmp;
				break;
			}
			// this checks for non-players ranking higher than players
			if (((herder[nOrder[idx]].type&PLAY_MASK)==PLAY_NONE) && ((herder[nOrder[idx+1]].type&PLAY_MASK)!=PLAY_NONE)) {
				// make sure non-players have no sheep (can happen due to a network game dropout, caused a lockup)
				herder[nOrder[idx]].sheep = 0;
				// swap them
				tmp=nOrder[idx];
				nOrder[idx]=nOrder[idx+1];
				nOrder[idx+1]=tmp;
				break;
			}
			// this checks for outright order being wrong
			if (herder[nOrder[idx]].sheep < herder[nOrder[idx+1]].sheep) {
				// swap them
				tmp=nOrder[idx];
				nOrder[idx]=nOrder[idx+1];
				nOrder[idx+1]=tmp;
				break;
			}
		}				
		if (idx >= 3) {
			// we got through the list without making any changes, so we are done
			break;
		}
		// else we continue the for loop
	}
	
	memcpy(pIn, nOrder, sizeof(int)*4);
	return herder[nOrder[0]].sheep;
}

// does the graphics for end of stage
// on entry, the 3D graphics should be blacked out
// and the text layer has "time over" or "stage complete" centered on it
// music should be stopped. Returns 1 if gHumanPlayer was a winner, else 0
#define WIN_MUSIC_END 4600
int HandleEndOfStageRanking() {
	u16* pOut;
	int idx;
	SPRITE bgspr;
	char buf[32];
	int ret = 0;
	char *pReasonTxt = NULL;		// a reason instead of a sort order for losing

	// stop the horizontal blank interrupt, we don't need color changes here	
	GX_VCountEqIntr(false);
	OS_DisableIrqMask(OS_IE_V_COUNT);
	OS_WaitVBlankIntr();			// wait a frame to be sure it stopped
	gLocalFrame = 0;				// sync is less important now
	
	// first thing we'll do now is scroll the text layer upwards so that 'time over' is at the top, around line 2 (it's at 11)
	for (idx=0; idx<10*8; idx+=4) {
		pvr_scene_begin();
		// no 3D to draw
		pvr_scene_finish();
		// but now we are synced with the vblank
		SetOffset2D(0, idx);
	}
	
	if (!isMultiplayer) {
		StoryModeTotalScore+=herder[gHumanPlayer].score;
	}
	
	if ((!isMultiplayer)&&(level == LEVEL_WATER)&&(gStageSpecialEffect&STAGE_EFFECT_SPECIAL_TO_WIN)) {
		// this is Iskur's clone stage, you have to special to win
		if (gBonusWinFlag) {
			for (;;) {
				// wait for the music to end
				long p = musicisplaying();
				if ((!p)||(p>WIN_MUSIC_END)) break;
				pvr_scene_begin();
				// no 3D to draw
				pvr_scene_finish();
			}
			// just let it scroll right off the screen, then exit
			for (idx=10*8+4; idx<14*8; idx+=4) {
				pvr_scene_begin();
				// no 3D to draw
				pvr_scene_finish();
				// but now we are synced with the vblank
				SetOffset2D(0, idx);
			}
			Clear2D();
			// return that the player won
			return 1;
		} else {
			// we lost, tell the user why, but don't do the rest
			if (nTimeLeft <= 0) {
				pReasonTxt = "You failed to hit the real\nIskur with a special attack\nbefore time ran out! Watch for\nthe solid Iskur and attack!";
			} else {
				pReasonTxt = "You failed to hit the real\nIskur with a special attack\nbefore all sheep were caught!\nWatch for the solid Iskur\nand attack!";
			}
		}
	}
	if ((!isMultiplayer)&&(level == LEVEL_UNDERWORLD)&&(gStageSpecialEffect&STAGE_EFFECT_CLEAR_SHEEP_TO_WIN)) {
		// this is the Demon's stage, you have to get all sheep to win
		if (!gBonusWinFlag) {
			pReasonTxt = "You must collect ALL of the\nsheep to defeat the demon! It\nmay require multiple specials\nto make him release all the\nsheep. Remember that ghost\nsheep must be stunned to be\ncaptured!";
		}
	}
	
	// clear the subscreen - player's attention should be on the moving words on the top screen now anyway
	TweakSubscreenMapToBackground();	

	// end of loop, quickly move the text to the right part of the screen and reset the offsets
	pOut=(u16*)((u32)G2_GetBG1ScrPtr() + ((1<<5)<<1));	// copy to row 1 and 2 from row 11 and 12
	for (idx=0; idx<32; idx++) {
		*(pOut+idx) = *(pOut+32*10+idx);
		*(pOut+32+idx) = *(pOut+32*11+idx);
	}
	WriteFont2D(11, 0, "                                ");
	SetOffset2D(0,0);
	SetOffset(0,0);		// zero the 3D offset too
	
	// figure out what tile to use for the background
	switch (level) {
		default:
		case LEVEL_NZ: 		bgspr.tilenumber = 0*100 + 9; break;
		case LEVEL_CANDY:	bgspr.tilenumber = 1*100 + 10; break;
		case LEVEL_HAUNTED:	bgspr.tilenumber = 0*100 + 7; break;
		case LEVEL_TOY:		bgspr.tilenumber = 1*100 + 10; break;
		case LEVEL_DISCO:	bgspr.tilenumber = 1*100 + 1*25 + 18; break;
		case LEVEL_WATER:	bgspr.tilenumber = 1*100 + 16; break;
		case LEVEL_UNDERWORLD:bgspr.tilenumber = 1*100 + 20; break;
	}
	// and set up important defaults
	bgspr.xd=0;
	bgspr.yd=0;
	bgspr.z=(fx16)2050;
	bgspr.alpha=31;
	bgspr.nDepth=DEPTH_512x512x8;
	bgspr.txr_addr=txr_level;
	bgspr.pal_addr=pal_level;
	bgspr.is3D=false;
	
	// Now fade in a new background (to half brightness, but this resets the 3D engine brightness
	for (idx=31; idx>=0; idx-=3) {
		Set3DDarken(idx);
		pvr_scene_begin();
			DrawEndOfStageBackground(&bgspr);
		pvr_scene_finish();
	}
	Set3DDarken(0);

	// work out who won, so that we can display the final ranking (hopefully it matches the subscreen...)
	int nWinCount, nWins;
	int nOrder[4] = { 0,1,2,3 };
	
	nWinCount = GetPlayerSortedOrder(nOrder);
	
	if (NULL == pReasonTxt) {
		// Now we know finally and for certain who won, so we can display that!
		// we count it up from the bottom!
		nWins=0;
		for (idx=3; idx>=0; idx--) {
			int nTmp = nOrder[idx];
			
			if ((herder[nTmp].type&PLAY_MASK) == PLAY_NETWORKDROP) {
				herder[nTmp].type = PLAY_NONE;
			}
			if ((herder[nTmp].type&PLAY_MASK) == PLAY_NONE) continue;
			
			if (herder[nTmp].sheep == nWinCount) {
				sprintf(buf, "WINNER:    %10.10s %-2.2d sheep", herder[nTmp].name, herder[nTmp].sheep);
				herder[nTmp].wins++;
				nWins++;		// so we can detect a tie
			} else {
				sprintf(buf, "#%-5d:    %10.10s %-2.2d sheep", idx+1, herder[nTmp].name, herder[nTmp].sheep);
			}
			WriteFont2D(idx*4+5, 1, buf);
			
			herder[nTmp].spr.tilenumber = 41;	// forward facing
			herder[nTmp].spr.x=64;
			herder[nTmp].spr.y=40+32*idx;
			
			// brief delay and sprite display
			for (int cnt=0; cnt<15; cnt++) {
				pvr_scene_begin();
					DrawEndOfStageBackground(&bgspr);
					for (int i2=0; i2<4; i2++) {
						if (i2<idx) continue;
						if ((herder[nOrder[i2]].type&PLAY_MASK) == PLAY_NETWORKDROP) {
							herder[nOrder[i2]].type = PLAY_NONE;
						}
						if ((herder[nOrder[i2]].type&PLAY_MASK) == PLAY_NONE) continue;
						SortSprite(&herder[nOrder[i2]].spr, herder[nOrder[i2]].map, POLY_HERD);
					}
				pvr_scene_finish();
			}
		}
		
		// finally, display the 'duh' moment at the bottom
		if (herder[gHumanPlayer].sheep == nWinCount) {
			if (nWins > 1) {
				WriteFont2D(21, 12, "You tied");
			} else {
				WriteFont2D(21, 12, "You WIN!");
			}
			ret = 1;
		} else {
			WriteFont2D(21, 12, "Nice try!");
			ret = 0;
		}
	} else {
		CenterWriteFontBreak2D(6, 16, pReasonTxt);
		ret = 0;
	}
		
	// wait for the music to end
	for (;;) {
		long p = musicisplaying();
		if ((!p)||(p>WIN_MUSIC_END)) break;
		pvr_scene_begin();
			DrawEndOfStageBackground(&bgspr);
			if (NULL == pReasonTxt) {
				for (int i2=0; i2<4; i2++) {
					if ((herder[nOrder[i2]].type&PLAY_MASK) == PLAY_NETWORKDROP) {
						herder[nOrder[i2]].type = PLAY_NONE;
					}
					if ((herder[nOrder[i2]].type&PLAY_MASK) == PLAY_NONE) continue;
					SortSprite(&herder[nOrder[i2]].spr, herder[nOrder[i2]].map, POLY_HERD);
				}
			}
		pvr_scene_finish();
	}

	// for the sound effect, change the winners' poses to the end of dance (even though we only play one voice)
	for (idx=0; idx<4; idx++) {
		if (herder[idx].sheep == nWinCount) {
			herder[idx].spr.tilenumber = 34;
		}
	}
	
	// say the winner's catch phrase before waiting for input
	// Chrys doesn't have one, so she can play a sheep sound
	if ((!isMultiplayer)&&(gStageSpecialEffect&(STAGE_EFFECT_CLEAR_SHEEP_TO_WIN|STAGE_EFFECT_SPECIAL_TO_WIN))) {
		if (gBonusWinFlag) {
			// make sure the human voice is played (we know it's not Chrys)
			sound_effect_player(gHumanPlayer, SND_ZEUSVICTORY1 + (ch_rand()%MAXSND_VICTORY), PLAYERVOL);
		} else {
			// make sure the other voice is played!
			idx=0;
			while ((idx == gHumanPlayer)||((herder[idx].type&PLAY_MASK)!=PLAY_COMPUTER)) idx++;
			sound_effect_player(idx, SND_ZEUSVICTORY1 + (ch_rand()%MAXSND_VICTORY), PLAYERVOL);
		}
	} else {
		// normal win voice case
		if (((herder[nOrder[0]].type&PLAY_CHAR_MASK) == HERD_CHRYS) || ((herder[nOrder[0]].type&PLAY_CHAR_MASK) == HERD_AFROCHRYS)) {
			sound_effect_system(SND_SHEEP1 + FREED, PLAYERVOL);
		} else {
			sound_effect_player(nOrder[0], SND_ZEUSVICTORY1 + (ch_rand()%MAXSND_VICTORY), PLAYERVOL);
		}
	}
	
	// note on the subscreen that we can continue now (note x,y, not r,c - APIs by different coders FTW!)
	// of course, Start also works. We can also put a little extra information here as needed.
	if ((!isMultiplayer)&&(level == LEVEL_NZ)&&(ret==0)) {
		MapScr_DrawTextXY816(1,  8, "Remember, after training you");
		MapScr_DrawTextXY816(1, 10, "must place FIRST to advance.");
		MapScr_DrawTextXY816(6, 14, "Press A to continue.");
		ret = 1;		// allow it as a win so we can advance anyway
	} else if (NULL != pReasonTxt) {
		MapScr_DrawTextXY816(10, 10, "Stage Failed!");
		MapScr_DrawTextXY816(6, 13, "Press A to continue.");
	} else {
		MapScr_DrawTextXY816(6, 11, "Press A to continue.");
	}
	
	MapScr_FlushTextOnly();	
	
	// and with all that, just hold here until we are clicked off this screen
	DisableControlsTillReleased();
	
	for (;;) {
		pvr_scene_begin();
			DrawEndOfStageBackground(&bgspr);
			if (NULL == pReasonTxt) {
				for (int i2=0; i2<4; i2++) {
					if ((herder[nOrder[i2]].type&PLAY_MASK) == PLAY_NETWORKDROP) {
						herder[nOrder[i2]].type = PLAY_NONE;
					}
					if ((herder[nOrder[i2]].type&PLAY_MASK) == PLAY_NONE) continue;
					SortSprite(&herder[nOrder[i2]].spr, herder[nOrder[i2]].map, POLY_HERD);
				}
			}
		pvr_scene_finish();
		if (isMultiplayer==2) {
			gLocalFrame = 0;
			// check for broken network
			if (sSysState != WH_SYSSTATE_DATASHARING) break;
			// carry on
			GetController(gHumanPlayer);
			// check for start or A from anyone!
			if (gNetworkControl[0]&(CONT_START|CONT_A)) break;
			if (gNetworkControl[1]&(CONT_START|CONT_A)) break;
			if (gNetworkControl[2]&(CONT_START|CONT_A)) break;
			if (gNetworkControl[3]&(CONT_START|CONT_A)) break;
		}

		// in case of network issue, an emergency exit 
		// we'll pick up the network failure on the next screen
		// also needed in the non-network case ;)
		if (isStartPressed()) break;
	}
	
	return ret;
}

// Main game routine
// Return 0 to continue, or 1 to exit
int RunGame(int inMultiplayer) {
	static u8 bSpecialUp = 0;	// whether the 'special' is displayed on the screen
	int x, y, idx, idx2, idx3;
	int speedloop;
	int gridx, gridy;
	int xstep=0, ystep=0;
	SPRITE spr ;
	int exitflag, miscflag=0;
	int lightning, special;
	int currentx=0, currenty=0;
	int origx=0, origy=0; 
	int nCountdown;
	static char buf[64], buf2[64];
	int nStoryPhase=1;
	int nLocalSetSnap = 0; // Do we want to set snap at the end of this frame?
	
	// erase text screen
	Clear2D();

	// set the global
	isMultiplayer=inMultiplayer;

	// suppress warning, don't really need them initialized yet
	x=0; y=0;

	// Don't play music and read the disc at the same time :)
	// (actually, it's amazing how close it comes to working!)
	sound_stop();

	// Load default sets
	memcpy(&gOptions, &gGame.Options, sizeof(struct gameoptions));
	gLocalFrame = 0;

	// now get the match ready
	// check what we need to set up
	if (!isMultiplayer) {
		// If we're in single player mode here, then we need to track that
		// and run the story, too. This function being so nasty already,
		// we're just going to hack it in.
		nStoryPhase=FindPhase(level);	// each level has multiple phases
		gOptions.Rounds=1;				// one round in story
		//player is allowed to override time except for infinite
		if (gOptions.Timer == 0) {
			gOptions.Timer = 60;
		}
		gOptions.Powers |= POW_SPECIALS;	// specials at least are mandatory for the story
		// player is allowed to control computer skill, sheep speed
	} else {
		// it IS multiplayer
		gOptions.Rounds=gGame.Options.Rounds;
		
		// if we are in wireless, broadcast a not ready
		if (isMultiplayer==2) {
			QueueNotReady();
			for (idx=0; idx<5; idx++) {
				// arbitrary number of frames to broadcast, ignoring loss at this point
				pvr_scene_finish_2d();
			}
		}
	}
	
	// start with no points yet (even in multiplayer clear this)
	StoryModeTotalScore=0;
	strcpy(szLastStoryCmd, "");
	// Reset the win count, too
	for (idx=0; idx<4; idx++) {
		herder[idx].wins=0;
	}

StoryModeLoop:
	nGameFrames=0;
	nCountdown=-1;
	lightning=0;
	special=0;
	// prepare to load stage-specific options
	gStageSpecialEffect = 0;
	nHowToPlayPhase = 0;		// no how to play
	MapScr_SetSpecialText("                ");

	if (!isMultiplayer) {
		gGame.continuelevel = level;	// continue however far you got
		if (gGame.AutoSave) {
			doVMUSave(1, gGame.SaveSlot);
		}		
	
		// in story mode - work out what it is we need to do
		if ('\0' == szLastStoryCmd[0]) {
			// clean up the display
			MapScr_ShowBlack();
			Clear2D();
			ShowBlack();
			// display is off, so we don't need to be 'safe'
			initStory();
			// and fix the windowing in case it was training
		   	G2_SetBlendBrightness(GX_BLEND_PLANEMASK_BG2, -9); // just the shadow layer
			GX_SetVisibleWnd(GX_WNDMASK_NONE);	// turn off the window

			// start background beat
			sound_start(SONG_STORY, 1);
			// start the menu
			MenuScr_Init();
			MenuScr_InitMenu(MENU_STORY_TEXT);	

			// Now loop
			while (('\0' == szLastStoryCmd[0]) || ('#' == szLastStoryCmd[0])) {
				// do next story panel
				strcpy(szLastStoryCmd, doStory(&nStoryPhase, szLastStoryCmd[0]));
StoryModeContinue:
				switch (szLastStoryCmd[0]) {
				case '\0':	
					break;	// loop
				
				case 'H':
					// we are going to play one phase with How To Play hints.
					level = LEVEL_NZ;		// new zealand
					gOptions.Rounds = 1;
					stage = 0;
					nHowToPlayFlags = 0;		// nothing has been done yet
					nHowToPlayTime = HOW_TO_PLAY_COUNTDOWN;	// first items minimum timeout
					nHowToPlayPhase = 1;		// first item (movement)
					gStageSpecialEffect|=STAGE_EFFECT_NO_SHEEP|STAGE_EFFECT_COMPUTER_FREEZE|STAGE_EFFECT_NO_COMPUTER_ATTACK|STAGE_EFFECT_FREEZE_TIMER;
					MapScr_SetSpecialText("    Training    ");
					break;	// play it!
				
				case 'S':	
					level=szLastStoryCmd[1]-'1';
					gOptions.Rounds=szLastStoryCmd[2]-'0';
					if (szLastStoryCmd[3]) {
						stage=szLastStoryCmd[3]-'1';
					} else {
						stage=rand()%3;
					}
					break;	// play for sheep Sx
				
				case 'Q':	
					// trigger end credits to run on exit!
					return 0xbbaa;	// end of game

				case 'E':	// end of level
					// update for new level
					level++;
					stage=0;
					// and loop
					szLastStoryCmd[0]='\0';
					break;
				}
			}


			// fade out drums while we close the text menu
			MenuScr_CloseMenu(0);
			for (idx=gGame.MVol; idx>0; idx--) {
				set_sound_volume(idx, -1);
				pvr_scene_finish_2d();
			}
			sound_stop();
			pvr_scene_finish_2d();
			set_sound_volume(gGame.MVol, -1);
		}

		debug("Going to do game mode %s\n", szLastStoryCmd);
		// else just run the next stage
	}
 
	// Game begin
	sprintf(buf, "gfx/Menu/level%d.bmp", level);
	if (inDemo) {
		// used to be a thread, now it's inline!
		scale_gfx(buf, "DEMO");
	} else {
		if (isMultiplayer) {
			scale_gfx(buf, "GET READY");
		} else {
			if (szLastStoryCmd[3] == '3') {
				scale_gfx(buf, szRound[level+2]);	// hack for Iskur and Demon's last two levels
			} else {
				scale_gfx(buf, szRound[level+1]);
			}
		}
	}

	sound_unloadlevel();
	// load miscellaneous data and start the subscreen (must come before we load the players)
	load_data();
	// Load the desired level
	LoadLevel(level, stage);
	// init the map screen
	MapScr_Init(level, 0);

	// sheep who will be in reserve (after initsheep())
	// note the first stage doesn't release any initial sheep,
	// so the TOTAL sheep will be this value (today that's
	// 24 instead of 29 sheep). That's okay for training.
	nTotalSheep = MAX_SHEEP-FIRST_SHEEP;

	// Now perform initialization
	for (idx=0; idx<4; idx++) {
		char *pherd;
		uint32 utmp;
		unsigned char utmp2;
		char szNameTmp[11];
		
		gNetworkControl[idx] = 0;	// make sure no old buttons are saved (particularly for non-players!)

		utmp=herder[idx].type;		// save the type
		idx3=herder[idx].wins;		// and the wins
		utmp2=herder[idx].color;	// and the color
		strcpy(szNameTmp, (char*)herder[idx].name);
		memset(&herder[idx], 0, sizeof(HerdStruct));
		herder[idx].type=utmp;
		herder[idx].wins=idx3;
		herder[idx].color=utmp2;
		strcpy((char*)herder[idx].name, szNameTmp);
		if (gStageSpecialEffect&STAGE_EFFECT_COMPUTER_FREEZE) {
			// each computer herder gets 1 sheep just to start in this mode
			if ((herder[idx].type & PLAY_MASK) == PLAY_COMPUTER) {
				herder[idx].sheep=1;
				nTotalSheep--;
			}
		}
		if (gOptions.Powers & POW_POWERUPS) {
			herder[idx].speed=HERDERSPEED;
			// work around slow networking
			if (isMultiplayer!=2) {
				herder[idx].speed++;
			}
			herder[idx].range=2;		// Work around some pixel-level bugs when this is only 1
		} else {
			debug("Disabled powerups! Setting to full speed.\n");
			herder[idx].speed=HERDERMAXSPEED;
			herder[idx].range=MAXLIGHTNING;
		}
		herder[idx].spr.nDepth=DEPTH_256x256x4;
		herder[idx].spr.txr_addr=txr_herder[idx];
		herder[idx].spr.pal_addr=pal_herder[idx];
		herder[idx].spr.tilenumber=DOWN_STAND_SPRITE;	// static down
		herder[idx].spr.alpha=31;
		if (herder[idx].type&PLAY_SPECIAL_CLONE) {
			// Iskur's clones are semi-transparent and almost fully-powered up
			herder[idx].spr.alpha=24;
			herder[idx].speed=HERDERMAXSPEED-2;
			herder[idx].range=MAXLIGHTNING-2;
		} else {
			if (herder[idx].type&PLAY_SPECIAL_POWER) {
				debug("Player %d has special powers! Setting to full speed.\n", idx);
				// Powered up players have everything
				herder[idx].speed=HERDERMAXSPEED;
				herder[idx].range=MAXLIGHTNING;
			}
		}
		herder[idx].spr.x=(idx<2 ? GRIDSIZE+GRIDSIZE+PLAYFIELDXOFF : (GRIDSIZE*(LEVELXSIZE-3))+PLAYFIELDXOFF);
		herder[idx].spr.y=(idx%2==0 ? (3*GRIDSIZE)+PLAYFIELDYOFF : (GRIDSIZE*(LEVELYSIZE-3))+PLAYFIELDYOFF);
		herder[idx].spr.z=(fx16)2028;
		herder[idx].spr.xd=(idx<2 ? 1 : -1);
		//herder[idx].spr.yd=0;		// memset means we don't need to set zeros
		herder[idx].spr.is3D=true;
		herder[idx].maxframes=6;
		for (idx2=0; idx2<HISTORY; idx2++) {
			herder[idx].oldx[idx2]=herder[idx].spr.x;
			herder[idx].oldy[idx2]=herder[idx].spr.y;
		} 
		//herder[idx].oldidx=0;		// memset means we don't need to set zeros
		memset(herder[idx].path, 0xff, sizeof(herder[idx].path));
		//herder[idx].pathIdx=0;		// memset means we don't need to set zeros
		//herder[idx].nIdleCount=0;		// memset means we don't need to set zeros
		//herder[idx].special=0;		// memset means we don't need to set zeros
		//herder[idx].specialFreeze=0;	// memset means we don't need to set zeros

		// check for drop
		if ((herder[idx].type&PLAY_MASK) == PLAY_NETWORKDROP) {
			doLostWireless();
			return 1;
		}

		// Load the appropriate herder tileset and sounds
		if ((herder[idx].type&PLAY_MASK) != PLAY_NONE) {
			pherd=szNames[herder[idx].type&PLAY_CHAR_MASK];
		
			// if playing in Story mode, and loading the Disco, then load
			// Afro Zeus instead (no Afro Chrys, sniff)
			if ((level == LEVEL_DISCO) && (!isMultiplayer) && (idx == 0)) {
				pherd=szNames[HERD_AFROZEUS];
			}
		
			// select the image
			sprintf(buf, "gfx/Players/%s_img.bmp", pherd);	
			// select the palette
			sprintf(buf2, "gfx/Players/%s%X.bmp", pherd, herder[idx].color);

			debug("Player %d (%s) loading %s (%s) -> %s\n", idx, herder[idx].name, buf, buf2, (herder[idx].type&PLAY_MASK)==PLAY_COMPUTER?"Computer":"Human");
			load_bmp_block_mask(buf, buf2, txr_herder[idx], pal_herder[idx], idx);
			buf[strlen(buf)-7]='\0';
			strcat(buf,"map.txt");
			load_map(buf, herder[idx].map);
			 
			sound_loadplayer(idx, nSoundGroups[herder[idx].type&PLAY_CHAR_MASK]);
			
		}
	}

	// make sure player specials are cleared
	InitSpecials();
	
	// Start level music
	StartLevelMusic(level);

	// Set the Timer 
	if (gOptions.Timer == 0) {
		nTimeLeft=99;
	} else {
		nTimeLeft=gOptions.Timer;
	}

	// Set up the background animation stuff
	// only ones that seem to matter are Disco and Toy Factory
	if (level == LEVEL_TOY) {
		nBgSpeedA=10;
	} else {
		nBgSpeedA=6;
	}
	if (level == LEVEL_DISCO) {
		nBgSpeedB=6;
	} else {
		nBgSpeedB=10;
	}
	nBgSpeedC=15;

	nBgAnimA=0;
	nBgAnimB=0;
	nBgAnimC=0;

	nBgCountA=nBgSpeedA;
	nBgCountB=nBgSpeedB;
	nBgCountC=nBgSpeedC;
	// End load level stuff

	initSheep();

	/* draw level */
	exitflag=0;
	fPaused=0;
	nGameCountdown=100;

	// sFx counts a big black parallelogram that sweeps across to the right
	// this is it's top-left point
	sFx=0;
		
	// simplified hack for checking if Zeus wins by special on applicable levels
	gBonusWinFlag = 0;

	nLocalSetSnap = 1;
	SetTimer(nTimeLeft);										// get the correct timer value up
	SetScore((u32)(herder[gHumanPlayer].score + StoryModeTotalScore));		// and score
	
	// the game loop is about to begin, if we need to, sync the network
	if (isMultiplayer == 2) {
		// waiting for everyone to get ready
		// two step process just to ensure we're in sync (kind of redundant..)
		debug("Queuing NET_CMD_READY");
		QueueReady();
		if (!WirelessWaitAllMachinesReady()) {
			doLostWireless();
			return 1;
		}

		debug("Queuing NET_CMD_GAME_READY");
		QueueGameReady();
		if (!WirelessWaitHostMachineReady()) {
			doLostWireless();
			return 1;
		}
		
		// activate network-driven controls
		for (idx=0; idx<4; idx++) {
			ControllerState[idx] = eContNetwork;
			gNetworkControl[idx] = 0;
		}
	}

	gLocalFrame = 0;		// for wireless, but init either way to avoid uninitialized variable issues
	nFrames = 0;			// all machines need to be on the same frame number!	

	// No point sending more than one, things are not actually queued up anymore
	QueueCtrl(0, gLocalFrame);	// make sure the frame index is out there and controls are zeroed
	
	while (1) {
		// center the screen on the player
		SetOffset(herder[gHumanPlayer].spr.x-112, herder[gHumanPlayer].spr.y-88);
		
		// before we render anything, see if any players are doing specials,
		// and if they are, darken the screen
		bSpecialDim = 0;
		if (herder[0].specialFreeze > 0) {
			bSpecialDim = 1;
		}
		if (herder[1].specialFreeze > 0) {
			bSpecialDim = 2;
		}
		if (herder[2].specialFreeze > 0) {
			bSpecialDim = 3;
		}
		if (herder[3].specialFreeze > 0) {
			bSpecialDim = 4;
		}
		if (bSpecialDim) {
			myG3_Color(15,15,15);
		} else {
			myG3_Color(31,31,31);
		}

		int nTmp = DrawLevel();
		if (nTmp == -99) {
			// we were asked to quit the game
			sound_stop();
			return 1;			
		} else {
			if (nTmp) {
				// scene is finished, loop (currently means we're paused)
				continue;
			}
		}

		// TODO: multiplayer frame tracking - can diff the output of multiple machines to ensure they stayed together		
//		debug("%04d - %3d,%3d - %3d,%3d - %3d,%3d - %3d,%3d\n", gLocalFrame, herder[0].spr.x, herder[0].spr.y,
//			herder[1].spr.x, herder[1].spr.y, herder[2].spr.x, herder[2].spr.y, herder[3].spr.x, herder[3].spr.y);
		
		// fire off a network signal multiple times per frame to boost the frame rate (we send very little data!)
		// this is a horrible, horrible hack. But it helps - OUTSIDE the player/sheep loop.
		if (isMultiplayer == 2) {
			SyncNetwork();
		}

		// Loop for rendering players
		for (idx=0; idx<4; idx++) {
			int oldtile;

			if ((herder[idx].type&PLAY_MASK) == PLAY_NETWORKDROP) {
				doLostWireless();
				return 1;
			}

			if ((herder[idx].type&PLAY_MASK) == PLAY_NONE) continue;
			
			// First erase self from collision buffer!
			drawbuffer(herder[idx].spr.x, herder[idx].spr.y, TYPE_NONE);
			
			if (bSpecialDim) {
				// bright if we're doing a special animation, or we're dancing to someone else's
				if ((herder[idx].specialFreeze > 0) || ((herder[idx].stuntime>0) && (herder[idx].stunframe == -1))) {
					myG3_Color(31,31,31);
				} else {
					myG3_Color(15,15,15);
				}
			}

			int nSpeedLoopSize = 1;
			if ((herder[idx].stunframe == 0) && (herder[idx].stuntime > 0)) nSpeedLoopSize=4;
			int nSpecialsTmp=-1;
			for (speedloop=0; speedloop<nSpeedLoopSize; speedloop++) {
				lightning=0;
				special=0;
				// get some default values
				// where we are
				currentx=herder[idx].spr.x;
				currenty=herder[idx].spr.y;
				// grid coordinates
				gridx=((currentx-PLAYFIELDXOFF)+GRIDSIZE/2)/GRIDSIZE;
				gridy=((currenty-PLAYFIELDYOFF)+GRIDSIZE/2)/GRIDSIZE;
				// grid aligned pixel coordinates
				origx=gridx*GRIDSIZE+PLAYFIELDXOFF;
				origy=gridy*GRIDSIZE+PLAYFIELDYOFF;
				// Work out our direction
				oldtile=herder[idx].spr.tilenumber%10;
				if (oldtile > 3) {
					// certain tiles don't follow the pattern
					switch (herder[idx].spr.tilenumber) {
						// charging frames
						case 44:	oldtile=SHEEPLEFT; break;
						case 45:	oldtile=SHEEPUP; break;
						case 46:	oldtile=SHEEPRIGHT; break;
						case 35:	oldtile=SHEEPDOWN; break;
						// deploying special
						case 6:		oldtile=SHEEPLEFT; break;
						case 16:	oldtile=SHEEPDOWN; break;
						case 26:	oldtile=SHEEPRIGHT; break;
						case 36:	oldtile=SHEEPUP; break;
						// dancing
						default:	oldtile=SHEEPDOWN; break;
					}
				}
	
				// This ensures specials are only called once, and the result is saved for the whole speedloop
				if (speedloop == 0) {
					nSpecialsTmp = ProcessSpecials(idx, nCountdown);
				}
				if (-1 != nSpecialsTmp) {
					herder[idx].spr.xd=0;
					herder[idx].spr.yd=0;
				} else if (herder[idx].stuntime==0) {
					int colltest = TYPE_WALL | TYPE_BOX;
					// Hades can run over boxes (and then destroys them)
					if ((bSpecialDim-1 == idx) && (specialDat[bSpecialDim-1][0].type == SP_WOLF)) {
						colltest = TYPE_WALL;
					}

					switch (herder[idx].type&PLAY_MASK) {
					case PLAY_HUMAN:	{
						u16 btns;
						if (TP_CheckBusy(TP_REQUEST_COMMAND_FLAG_SAMPLING) == 0) {
							// let the touch pad start reading
							// while we handle the buttons
							TP_RequestSamplingAsync();
						}

						updateoldpos(&herder[idx]);

						btns = GetNetController(idx);
							
#if !defined(SDK_FINALROM)
						if ((btns&CONT_L) && (btns&CONT_R)) {
							debug("Currentx %d\n", currentx);
							debug("Currenty %d\n", currenty);
							debug("gridx    %d\n", gridx);
							debug("gridy    %d\n", gridy);
							debug("origx    %d\n", origx);
							debug("origy    %d\n", origy);
							debug("destx    %d\n", herder[idx].destx);
							debug("desty    %d\n", herder[idx].desty);
							debug("xd       %d\n", herder[idx].spr.xd);
							debug("yd       %d\n", herder[idx].spr.yd);
							debug("------------\n");
						}
#endif

						// use this to decide whether to animate or show static
						miscflag=0;

						// check if we are grid aligned
						if ((currentx == origx) && (currenty == origy)) {
							// we are! That means we can take a new input from the user
							for (;;) {		// we may have to loop this block once
								herder[idx].spr.xd=0;
								herder[idx].spr.yd=0;
								herder[idx].originx=origx;
								herder[idx].originy=origy;
								herder[idx].spr.tilenumber=oldtile;
								herder[idx].destx=currentx;
								herder[idx].desty=currenty;

								if (btns & CONT_DPAD_UP) {
									miscflag=1;
									herder[idx].spr.tilenumber=SHEEPUP;
										
									if (!checkblock(origx, origy-GRIDSIZE, colltest)) {
										herder[idx].spr.yd=-herder[idx].speed;
									}
								}
								if (btns & CONT_DPAD_DOWN) {
									miscflag=1;
									herder[idx].spr.tilenumber=SHEEPDOWN;
									if (!checkblock(origx, origy+GRIDSIZE, colltest)) {
										herder[idx].spr.yd=herder[idx].speed;
									}
								}
								if (btns & CONT_DPAD_LEFT) {
									miscflag=1;
									herder[idx].spr.tilenumber=SHEEPLEFT;
									if (!checkblock(origx-GRIDSIZE, origy, colltest)) {
										herder[idx].spr.xd=-herder[idx].speed;
									} else {
										if (herder[idx].spr.yd) {
											// fixup tilenumber
											if (herder[idx].spr.yd > 0) {
												herder[idx].spr.tilenumber=SHEEPDOWN;
											} else {
												herder[idx].spr.tilenumber=SHEEPUP;
											}
										}
									}
								}
								if (btns & CONT_DPAD_RIGHT) {
									miscflag=1;
									herder[idx].spr.tilenumber=SHEEPRIGHT;
									if (!checkblock(origx+GRIDSIZE, origy, colltest)) {
										herder[idx].spr.xd=herder[idx].speed;
									} else {
										if (herder[idx].spr.yd) {
											// fixup tilenumber
											if (herder[idx].spr.yd > 0) {
												herder[idx].spr.tilenumber=SHEEPDOWN;
											} else {
												herder[idx].spr.tilenumber=SHEEPUP;
											}
										}
									}
								}
								if (miscflag) {
									nHowToPlayFlags |= HOW_TO_PLAY_MOVE;
								}
								if ((herder[idx].spr.xd == 0) || (herder[idx].spr.yd == 0)) {
									// we have a valid move
									break;
								}

								// if we get here, we've got a diagonal and both directions are legal.
								// Check our history and try to move perpendicular to last move. Disable
								// the move we don't want in the input buffer
								idx2=herder[idx].oldidx-1;
								if (idx2<0) idx2=HISTORY-1;
								if (herder[idx].oldy[idx2] == currenty) {
									// we were moving horizontal (or were stationary)
									btns &= ~(CONT_DPAD_LEFT|CONT_DPAD_RIGHT);
								} else {
									// we were moving vertical
									btns &= ~(CONT_DPAD_UP|CONT_DPAD_DOWN);
								}
								// now repeat the loop
							}
								
							// deal with any push tiles
							if ((AddPush(idx, gridx, gridy)) && (miscflag == 0)) {
								// we weren't moving, but the conveyer belt pushes us
								herder[idx].destx=origx;
								herder[idx].desty=origy;
								if (herder[idx].spr.xd > 0) {
									herder[idx].destx+=GRIDSIZE;
								}
								if (herder[idx].spr.xd < 0) {
									herder[idx].destx-=GRIDSIZE;
								}
								if (herder[idx].spr.yd > 0) {
									herder[idx].desty+=GRIDSIZE;
								}
								if (herder[idx].spr.yd < 0) {
									herder[idx].desty-=GRIDSIZE;
								}
								// verify legal move (except for destructibles it should be)
								if (checkblock(currentx+herder[idx].spr.xd, currenty+herder[idx].spr.yd, colltest)) {
									herder[idx].spr.xd=0;
									herder[idx].spr.yd=0;
								}
							}

							if (miscflag) {
								// set moved flag
								herder[idx].nIdleCount=0;
								// verify legal move - don't check against players here
								if (checkblock(currentx+herder[idx].spr.xd, currenty+herder[idx].spr.yd, colltest)) {
									herder[idx].spr.xd=0;
									herder[idx].spr.yd=0;
								} else {
									herder[idx].destx=origx;
									herder[idx].desty=origy;
									if (herder[idx].spr.xd > 0) {
										herder[idx].destx+=GRIDSIZE;
									}
									if (herder[idx].spr.xd < 0) {
										herder[idx].destx-=GRIDSIZE;
									}
									if (herder[idx].spr.yd > 0) {
										herder[idx].desty+=GRIDSIZE;
									}
									if (herder[idx].spr.yd < 0) {
										herder[idx].desty-=GRIDSIZE;
									}
								}
							} else {
								herder[idx].nIdleCount=1;
							}
						} else {
							// we are not grid aligned, so we need to check for reversal
							// Because of conveyer belts, we also need to check for forward
							if ((herder[idx].spr.xd==0) && (herder[idx].spr.yd >= 0)) {
								if (btns & CONT_DPAD_UP) {
									herder[idx].nIdleCount=0;
									miscflag=1;
									herder[idx].spr.tilenumber=SHEEPUP;
									herder[idx].spr.yd=-herder[idx].speed;
									idx2=herder[idx].desty;
									herder[idx].desty=herder[idx].originy;
									herder[idx].originy=idx2;
									AddPush(idx, gridx, gridy);
								} else {
									herder[idx].spr.tilenumber=SHEEPDOWN;
									if (btns & CONT_DPAD_DOWN) {
										herder[idx].nIdleCount=0;
										herder[idx].spr.yd=herder[idx].speed;
										AddPush(idx, gridx, gridy);
									}
								}
							}
							if ((herder[idx].spr.xd==0) && (herder[idx].spr.yd <= 0)) {
								if (btns & CONT_DPAD_DOWN) {
									miscflag=1;
									herder[idx].spr.tilenumber=SHEEPDOWN;
									herder[idx].spr.yd=herder[idx].speed;
									idx2=herder[idx].desty;
									herder[idx].desty=herder[idx].originy;
									herder[idx].originy=idx2;
									AddPush(idx, gridx, gridy);
								} else {
									herder[idx].spr.tilenumber=SHEEPUP;
									if (btns & CONT_DPAD_UP) {
										herder[idx].nIdleCount=0;
										herder[idx].spr.yd=-herder[idx].speed;
										AddPush(idx, gridx, gridy);
									}
								}
							}
							if ((herder[idx].spr.xd >= 0) && (herder[idx].spr.yd==0)) {
								if (btns & CONT_DPAD_LEFT) {
									miscflag=1;
									herder[idx].spr.tilenumber=SHEEPLEFT;
									herder[idx].spr.xd=-herder[idx].speed;
									idx2=herder[idx].destx;
									herder[idx].destx=herder[idx].originx;
									herder[idx].originx=idx2;
									AddPush(idx, gridx, gridy);
								} else {
									herder[idx].spr.tilenumber=SHEEPRIGHT;
									if (btns & CONT_DPAD_RIGHT) {
										herder[idx].nIdleCount=0;
										herder[idx].spr.xd=herder[idx].speed;
										AddPush(idx, gridx, gridy);
									}
								}
							}
							if ((herder[idx].spr.xd <= 0) && (herder[idx].spr.yd==0)) {
								if (btns & CONT_DPAD_RIGHT) {
									miscflag=1;
									herder[idx].spr.tilenumber=SHEEPRIGHT;
									herder[idx].spr.xd=herder[idx].speed;
									idx2=herder[idx].destx;
									herder[idx].destx=herder[idx].originx;
									herder[idx].originx=idx2;
									AddPush(idx, gridx, gridy);
								} else {
									herder[idx].spr.tilenumber=SHEEPLEFT;
									if (btns & CONT_DPAD_LEFT) {
										herder[idx].nIdleCount=0;
										herder[idx].spr.xd=-herder[idx].speed;
										AddPush(idx, gridx, gridy);
									}
								}
							}
							if ((herder[idx].spr.xd == 0) && (herder[idx].spr.yd==0)) {
								herder[idx].spr.tilenumber=oldtile;
							}

							// now either way, reset it for the animation below
							if (herder[idx].nIdleCount) {
								miscflag=0;
							} else {
								miscflag=1;
							}
						}

						// always check for buttons and pause!
						if (btns & (CONT_A | CONT_B | CONT_X | CONT_Y)) {
							nHowToPlayFlags |= HOW_TO_PLAY_FIRE;
							if ( ((btns & (CONT_A | CONT_B)) == (CONT_A | CONT_B)) && (herder[idx].special >= 140) ) {
								special=1;				// flag to do special after we move
							} else if (herder[idx].charge>=16) {
								lightning=1;			// flag to do lightning after we move
							}
						} else {
							if (((herder[idx].type & PLAY_CHAR_MASK) == HERD_CHRYS) || ((herder[idx].type & PLAY_CHAR_MASK) == HERD_AFROCHRYS)) {
								// Chrys never gets to charge lightning, either,
								// if she has a charge it is always on till it runs out
								lightning=1;
							} else {
								herder[idx].charge++;
								// Adjust for maximum range
								if (herder[idx].charge > (herder[idx].range<<4)+15) {
									herder[idx].charge=(herder[idx].range<<4)+15;
								}
							}
						}

						// for now, disable START in multiplayer
						// TODO: at least the host needs to be able to quit the game!
						if (isMultiplayer != 2) {
							if ((btns & CONT_START) && (!(btns & CONT_A))) {
								// pause
								// TODO: don't allow pause during multiplayer games
								if (fPaused==0) {
									// When we PRESS Start, we set it to 2 to track the release
									fPaused=2;
									// And this updates the state so 'Paused' appears
									nGameCountdown=52;
									
									MenuScr_Init(); // First, we need to convert the secondary display to Menu mode
									MenuScr_InitMenu(MENU_PAUSE); // Then we need to tell it to begin drawing the pause menu
								}
							} else {
								// When Start is pressed again to unpause, we set fPaused to this value
								// so that we can release the pause, when the button is released
								if (-(idx+1)==fPaused) {
									fPaused=0;
									nLocalSetSnap = 1;
								}
							}
						}

						// If we are moving at will (ie: player said to)
						if (miscflag) {
							// Do the animation
							if (nGameFrames%4 == 0) {
								herder[idx].animframe++;
								if (herder[idx].animframe >= herder[idx].maxframes) {
									herder[idx].animframe=0;
								}
							}
							herder[idx].spr.tilenumber+=HerderAnimSeq[herder[idx].animframe]*10;
						} else {
							// stationary tile
							herder[idx].spr.tilenumber=oldtile+40;	// set to idle frame
						}
							
						if (bSpecialDim-1 != idx) {
							if ((herder[idx].charge > 15) && (0 == (btns & (CONT_DPAD_LEFT|CONT_DPAD_RIGHT|CONT_DPAD_UP|CONT_DPAD_DOWN)))) {
								// Increment special gauge when lightning is above a minimum value
								// and either touch pad is rubbed or shoulders are tapped, but not moving
								// get the touch pad result (should be long enough by now)
								TPData touchdat;
								bool bCounted=false;
								// non-zero means error occurred, so ignore that
								if (0 == TP_GetCalibratedResult(&touchdat)) {
									if ((touchdat.touch == TP_TOUCH_ON) && (TP_VALIDITY_VALID == touchdat.validity)) {
										// good so far, check if it is different from last time
										// reduce accuracy to ignore jitter
										touchdat.x>>=2;
										touchdat.y>>=2;
										if ((herder[idx].spec_x != touchdat.x) || (herder[idx].spec_y != touchdat.y)) {
											herder[idx].spec_x=touchdat.x;
											herder[idx].spec_y=touchdat.y;
											if (herder[idx].special < MAXSPECIAL) {
												herder[idx].special++;
												if (herder[idx].range >= MAXLIGHTNING) {
													herder[idx].special++;
												}
												bCounted=true;
											}
										}
									}
								}
								if (!bCounted) {
									// if not touch pad, check shoulder buttons as an alternate
									// we can't count zero on the shoulders because otherwise
									// then you can hold one spot on the touch pad and they will
									// both count it up :)
									if (0 != (btns&(CONT_L|CONT_R))) {
										if (herder[idx].special < MAXSPECIAL) {
											herder[idx].special++;
											if (herder[idx].range >= MAXLIGHTNING) {
												herder[idx].special++;
											}
											bCounted=true;
										}
									}
								}
								// do the charging animation (if it's allowed)
								if (((gStageSpecialEffect&STAGE_EFFECT_NO_PLAYER_SPECIAL) == 0) && (gOptions.Powers&POW_SPECIALS) && (bCounted)) {
									sound_effect_player_throttled(idx, SND_ZEUSCHARGE1+(ch_rand()%MAXSND_CHARGE), PLAYERVOL);
									if (oldtile&1) {
										// up and down are not laid out the same as the rest
										herder[idx].spr.tilenumber=(oldtile==1 ? 35 : 45);
									} else {
										herder[idx].spr.tilenumber=oldtile+44;
									}
								}
							} else if ( (((herder[idx].type & PLAY_CHAR_MASK) == HERD_CHRYS) || ((herder[idx].type & PLAY_CHAR_MASK) == HERD_AFROCHRYS) ) && (miscflag)) {
								if (herder[idx].charge < 16) {
									// this is special - Chrys' gauge charges when she moves (static electricity!)
									// she charges faster than the others, too
									// is this the best way? We never see her charge-up frames this way...
									// eh, it works. ;)
									if (herder[idx].special < MAXSPECIAL) {
										herder[idx].special+=3;
									}
								}
							}
						}
							
						if (herder[gHumanPlayer].special < 140) {
							if (bSpecialUp) {
								Clear2D();
								bSpecialUp = 0;
							}
						} else {
							if (!bSpecialUp) {
								char *p;
							
								bSpecialUp=(herder[gHumanPlayer].type&PLAY_CHAR_MASK)+1;
								nHowToPlayFlags |= HOW_TO_PLAY_CHARGE;

								switch (bSpecialUp) {
									case 1:
									case 14:
										p="A+B for LIGHTNING SHOCK";
										break;
										
									case 2:
										p="A+B for KIWI CALL";
										break;
									
									case 3:
										p="A+B for GIANT GUMBALL";
										break;
									
									case 4:
										p="A+B for GIFT WRAPPING";
										break;
										
									case 5:
										p="A+B for DISCO BALL";
										break;
										
									case 6:
										p="A+B for BLACK LIGHTNING";
										break;
										
									case 7:
										p="A+B for POWER BEAM";
										break;
										
									case 8:
										p="A+B for FLASH FLOOD";
										break;
										
									case 9:
										p="A+B for HOLY LIGHT";
										break;
										
									case 10:
										p="A+B for POWER OF MOON";
										break;
										
									case 11:
										p="A+B for GIANT DISCO BALL";
										break;
										
									case 12:
										p="A+B for DEMON BLAST";
										break;
										
									default:
										p="A+B for STATIC STREAM";
										break;
								}
								
								CenterWriteFont2D(0, 15, p);
							}
						}
						
						// check for story mode text
						if ((nHowToPlayPhase)&&(nCountdown==-1)) {
							UpdateHowToPlay();
						}
						break;
					}

					case PLAY_COMPUTER: 		/* computer - no controller */
						// check if we're inactive
						if (gStageSpecialEffect & STAGE_EFFECT_COMPUTER_FREEZE) {
							herder[idx].spr.xd=0;
							herder[idx].spr.yd=0;
							herder[idx].stuntime=0;
							break;
						}
						
						// If we're at our destination
						if ((currentx == herder[idx].destx) && (currenty == herder[idx].desty)) {
							// at a destination
							if ((herder[idx].pathIdx < 98) && (herder[idx].path[herder[idx].pathIdx] != -1)) {
#ifdef PATHFIND_DEBUG
								debug("Herder %d at waypoint %d, moving to next waypoint.\n", idx, herder[idx].pathIdx/2);
#endif
								herder[idx].pathIdx+=2;
							}
							
							if ((herder[idx].pathIdx >= 98) || (herder[idx].path[herder[idx].pathIdx] == -1)) {
#ifdef PATHFIND_DEBUG
								debug("Herder %d at destination.\n", idx);
#endif
								herder[idx].destx=0;
							}
						}

						// no point checking for idle if we have to retarget anyway
						if (herder[idx].destx) {
							// Check - if we didn't move and update idlecomp - if too high, choose a new destination
							idx2=herder[idx].oldidx-1;
							if (idx2<0) idx2=HISTORY-1;
							if ((herder[idx].oldx[idx2]==currentx)&&(herder[idx].oldy[idx2]==currenty)) {
								int fPatient=0;

								if (herder[idx].nIdleCount > 1) {
									// As a workaround for our inability to zap crates around sharp corners,
									// see if there are any crates we can destroy that might be blocking us, too.
									if (LevelData[gridy-1][gridx].nDestructible == 1) {
										LevelData[gridy-1][gridx].nDestructible=2;
									} else if (LevelData[gridy+1][gridx].nDestructible == 1) {
										LevelData[gridy+1][gridx].nDestructible=2;
									} else if (LevelData[gridy][gridx-1].nDestructible == 1) {
										LevelData[gridy][gridx-1].nDestructible=2;
									} else if (LevelData[gridy][gridx+1].nDestructible == 1) {
										LevelData[gridy][gridx+1].nDestructible=2;
									}
								}

								// we must also be patient and wait for crates to be destroyed
								fPatient=0;
								if (LevelData[gridy-1][gridx].nDestructible > 1) {
									fPatient=1;
								}
								if (LevelData[gridy+1][gridx].nDestructible > 1) {
									fPatient=1;
								}
								if (LevelData[gridy][gridx-1].nDestructible > 1) {
									fPatient=1;
								}
								if (LevelData[gridy][gridx+1].nDestructible > 1) {
									fPatient=1;
								}
#ifdef PATHFIND_DEBUG
								if (fPatient) {
									debug("Herder %d waiting on destructible...\n", idx);
								}
#endif								
								if (!fPatient) {
									herder[idx].nIdleCount++;
									if (herder[idx].nIdleCount > 2) {
										herder[idx].nIdleCount=0;
										if (herder[idx].path[herder[idx].pathIdx] != -1) {
											// two because it's 1d array with x and y
											// This probably won't work about half the time, but y'never know.
#ifdef PATHFIND_DEBUG
											debug("Herder %d stuck seeking waypoint %d, recentering and retargetting.\n", idx, herder[idx].pathIdx/2);
#endif
											herder[idx].spr.x=origx;
											herder[idx].spr.y=origy;
											currentx=origx;
											currenty=origy;
											herder[idx].destx=0;
										}
										if (herder[idx].path[herder[idx].pathIdx] == -1) {
#ifdef PATHFIND_DEBUG
											debug("Herder %d stuck seeking final waypoint - will retarget.\n", idx);
#endif
											herder[idx].destx=0;
											herder[idx].spr.x=origx;
											herder[idx].spr.y=origy;
											currentx=origx;
											currenty=origy;
										}
									}
								}
							} else {
								// We moved, so reset the counter
								herder[idx].nIdleCount=0;
							}
						}

						updateoldpos(&herder[idx]);

						if ((herder[idx].destx==0)||(herder[idx].desty==0)) {
							// Check if we need to re-aim or select a new target
							if ((herder[idx].CPUStatus&0xff00) == 0xff00) {
								// random - we never persist on the random wander - choose new target
#ifdef PATHFIND_DEBUG
								debug("Herder %d finished with random target.\n", idx);
#endif
								herder[idx].CPUStatus=0;
							}
							// see whether we are counting down our tracking time
							if ((herder[idx].CPUStatus&0xff) > 1) {
								// yes, just decrement it
#ifdef PATHFIND_DEBUG
								debug("Herder %d continuing with existing target - time %d\n", idx, herder[idx].CPUStatus&0xff);
#endif
								herder[idx].CPUStatus--;
							} else {
								// chased long enoug - find a new target
#ifdef PATHFIND_DEBUG
								debug("Herder %d finished with target %d\n", idx, herder[idx].CPUStatus>>8);
#endif
								herder[idx].CPUStatus=0;
							}

							if (herder[idx].CPUStatus != 0) {
								// we still have a target, so we just need to reaim at it
								if (herder[idx].CPUStatus&0x8000) {
									int sh=(int)((herder[idx].CPUStatus&0x7f00)>>8);
									// it's a player, and players are always available
									// if they ever were!
									herder[idx].destx=herder[sh].spr.x;
									herder[idx].desty=herder[sh].spr.y;
#ifdef PATHFIND_DEBUG
									debug("Herder %d re-acquire player %d at %d,%d (Calling FindPath)\n", idx, sh, herder[idx].destx, herder[idx].desty);
#endif
									FindPath(idx, herder[idx].path, currentx, currenty, herder[idx].destx, herder[idx].desty);
									herder[idx].pathIdx=0;
								} else {
									int shx=(int)(herder[idx].CPUStatus>>8);
									// it's a sheep, and it may not be available anymore
									if ((sheep[shx].type > 0) && (sheep[shx].spr.x > 0)) {
										// but it is!
										herder[idx].destx=sheep[shx].spr.x;
										herder[idx].desty=sheep[shx].spr.y;
#ifdef PATHFIND_DEBUG
										debug("Herder %d re-acquire sheep %d, type %d at %d,%d (Calling FindPath)\n", idx, shx, sheep[shx].type, herder[idx].destx, herder[idx].desty);
#endif
										FindPath(idx, herder[idx].path, currentx, currenty, herder[idx].destx, herder[idx].desty);
										herder[idx].pathIdx=0;
									} else {
										// it's not.. new target required
#ifdef PATHFIND_DEBUG
										debug("Herder %d target sheep %d not available\n", idx, shx);
#endif
										herder[idx].CPUStatus=0;
									}
								}
							}

							// this can't be an else because the above code might change the target to 0
							if (herder[idx].CPUStatus == 0) {
								// new target - it doesn't have to be a legal destination,
								// as we'll pick a new one as soon as we get stuck :)
								// Move towards a sheep!
								int sh=(int)(ch_rand()%MAX_SHEEP);
								int shx, fl;
								fl=0;
								for (shx=sh; shx<sh+MAX_SHEEP/2; shx++) {
									int xx=shx%MAX_SHEEP;
									if ((sheep[xx].type>0)&&(sheep[xx].spr.x>32)&&(sheep[xx].spr.y>32)) {
										fl=1;
										herder[idx].destx=sheep[xx].spr.x;
										herder[idx].desty=sheep[xx].spr.y;
#ifdef PATHFIND_DEBUG
										debug("Herder %d selected sheep %d, type %d, at %d,%d\n", idx, xx, sheep[xx].type, herder[idx].destx, herder[idx].desty);
#endif
										herder[idx].CPUStatus=(unsigned)xx<<8;		// upper byte is sheep # or player #
										break;
									}
								}
								if (fl == 0) {
									// crap - no such sheep. Pick on a player, then?
									if (gOptions.Skill != 0) {		// only if CPU skill is not set to 'lost'
										sh=(int)(ch_rand()%5+(2-gOptions.Skill));	// Chance of not chasing a player, we need some random
										if ((sh < 4) && (sh != idx)) {
										
											// prefer human targets ;) Check other three!
											if ((herder[sh].type & PLAY_MASK) != PLAY_HUMAN) {
												sh++; if (sh>3) sh=0; if (sh == idx) { sh++; if (sh>3) sh=0; }
												if ((herder[sh].type & PLAY_MASK) != PLAY_HUMAN) {
													sh++; if (sh>3) sh=0; if (sh == idx) { sh++; if (sh>3) sh=0; }
													if ((herder[sh].type & PLAY_MASK) != PLAY_HUMAN) {
														sh=idx;	// the check below looks for this
													}
												}
											}
											
											if (sh == idx) {
												// no human targets - might be demo mode. Check for anyone else at all
												sh++;
												while (sh != idx) {
													if ((herder[sh].type&PLAY_MASK) != PLAY_NONE) break;
													sh++;
													if (sh > 3) sh=0;
												}
											}
										} else {
											// force a fall through to random
											sh=idx;
										}
									} else {
										// force a fall through to random
										sh=idx;
									}

									if ((sh > 3) || (sh == idx)) {
										// double crap - no player either? Fine. Wander aimlessly.
										herder[idx].destx=(int)((ch_rand()%20) * GRIDSIZE);
										herder[idx].desty=(int)((ch_rand()%15) * GRIDSIZE);
#ifdef PATHFIND_DEBUG
										debug("Herder %d selected random target at %d,%d.\n", idx, herder[idx].destx, herder[idx].desty);
#endif
										herder[idx].CPUStatus=0xff00;		// randomness
									} else {
										herder[idx].destx=herder[sh].spr.x;
										herder[idx].desty=herder[sh].spr.y;
#ifdef PATHFIND_DEBUG
										debug("Herder %d selected player %d at %d,%d\n", idx, sh, herder[idx].destx, herder[idx].desty);
#endif
										herder[idx].CPUStatus=(0x8000 | ((unsigned)sh<<8));	// player number with high bit set
									}
								}
#ifdef PATHFIND_DEBUG
								debug("Herder %d persistence is %d (Calling FindPath to %d,%d)\n", idx, gOptions.Skill*2+1, herder[idx].destx, herder[idx].desty);
#endif
								herder[idx].CPUStatus|=gOptions.Skill*2+1;	// lower byte is how long to chase the target
								FindPath(idx, herder[idx].path, currentx, currenty, herder[idx].destx, herder[idx].desty);
								herder[idx].pathIdx=0;
							}
						}

						// get the next destX and destY
						herder[idx].destx=herder[idx].path[herder[idx].pathIdx]*GRIDSIZE+PLAYFIELDXOFF;
						herder[idx].desty=herder[idx].path[herder[idx].pathIdx+1]*GRIDSIZE+PLAYFIELDYOFF;
						if ((herder[idx].destx<0) || (herder[idx].desty < 0)) {
							herder[idx].destx=origx;
							herder[idx].desty=origy;
						}
						
						// now get there!
						herder[idx].spr.xd=herder[idx].destx-currentx;
						herder[idx].spr.yd=herder[idx].desty-currenty;
#ifdef PATHFIND_DEBUG
						debug("Herder %d next waypoint is at %d,%d, range %d,%d\n", idx, herder[idx].destx, herder[idx].desty, herder[idx].spr.xd, herder[idx].spr.yd);
#endif
						AddPush(idx, gridx, gridy);
						
						// normally we always move, so set miscflag here
						miscflag=1;

						// check if we aren't following the path correctly
						if ((herder[idx].spr.xd==0)&&(herder[idx].spr.yd==0)) {
							// new target next loop if both are 0 (null movement - error)
#ifdef PATHFIND_DEBUG
							debug("Herder %d null movement - resetting.\n", idx);
#endif
							// we aren't moving, make darn sure we're aligned.
							// This is hacky but we'll leave it unless it's visible
							if ((currentx!=origx)||(currenty!=origy)) {
								// Not supposed to MOVE the player in this part, but this is to fix an error
								// in positioning that shouldn't happen, either.
#ifdef PATHFIND_DEBUG
								debug("Herder %d - Doing unintended fixup on static player.\n", idx);
#endif
								herder[idx].spr.x=origx;
								herder[idx].spr.y=origy;
							}
							herder[idx].pathIdx=100;
							// no movement case
							miscflag=0;
						}
						if ((herder[idx].spr.xd != 0) && (herder[idx].spr.yd != 0)) {
							// also new target if neither are 0 (diagonal movement - off track)
							// allow some slack though
							if ((abs(herder[idx].spr.xd) > 8) && (abs(herder[idx].spr.yd > 8))) {
#ifdef PATHFIND_DEBUG
								debug("Herder %d diagonal movement - resetting.\n", idx);
#endif
								// clean up for this frame's sake
								herder[idx].destx=origx;
								herder[idx].desty=origy;
								herder[idx].spr.xd=origx-currentx;
								herder[idx].spr.yd=origy-currenty;
								herder[idx].pathIdx=100; // off the end
								// no movement case
								miscflag=0;
							}
						}

						// limit it by speed
						if (herder[idx].spr.xd > herder[idx].speed) herder[idx].spr.xd=herder[idx].speed;
						if (herder[idx].spr.yd > herder[idx].speed) herder[idx].spr.yd=herder[idx].speed;
						if (herder[idx].spr.xd < -herder[idx].speed) herder[idx].spr.xd=-herder[idx].speed;
						if (herder[idx].spr.yd < -herder[idx].speed) herder[idx].spr.yd=-herder[idx].speed;
						
						// handle setting the animation frame. Since we force 4 way directions,
						// we have a simpler search
						if (herder[idx].spr.xd > 0) herder[idx].spr.tilenumber=SHEEPRIGHT; else
						if (herder[idx].spr.xd < 0) herder[idx].spr.tilenumber=SHEEPLEFT; else
						if (herder[idx].spr.yd < 0) herder[idx].spr.tilenumber=SHEEPUP; else
						if (herder[idx].spr.yd > 0) herder[idx].spr.tilenumber=SHEEPDOWN;

						if (miscflag) {
							// verify legal move - don't check against players here
							if (checkblock(currentx+herder[idx].spr.xd, currenty+herder[idx].spr.yd, colltest)) {
								herder[idx].spr.xd=0;
								herder[idx].spr.yd=0;
							}

							// Do the animation
							if (nGameFrames%4 == 0) {
								herder[idx].animframe++;
								if (herder[idx].animframe >= herder[idx].maxframes) {
									herder[idx].animframe=0;
								}
							}
							herder[idx].spr.tilenumber+=HerderAnimSeq[herder[idx].animframe]*10;
						} else {
							// stationary tile
							// TODO: this might be dancing
							herder[idx].spr.tilenumber=oldtile+40;
						}

						if ((currentx == origx) && (currenty == origy)) {
							// cache the value 
							herder[idx].originx=currentx;
							herder[idx].originy=currenty;
						}

						// Lightning
						if (((ch_rand()&0xf)>8)&&(herder[idx].charge>15)) {
							lightning=1;
						} else {
							herder[idx].charge++;
							if (herder[idx].charge > (herder[idx].range<<4)+15) {
								herder[idx].charge=(herder[idx].range<<4)+15;
							}
						}
						if ((lightning) && (herder[idx].special >= 150)) {
							special = 1;
							lightning=0;
						}
						
						if (gStageSpecialEffect & STAGE_EFFECT_NO_COMPUTER_ATTACK) {
							// cancel all that
							special=0;
							lightning=0;
							herder[idx].special=0;
						}
						break;

					default:	/* no action - probably an error */
						break;
					}
				} else {
					// stunned (no conveyer belt in here)
					// we can super cheat and move 2 pixels at a time, cause we loop 8 times now!
					updateoldpos(&herder[idx]);		// we still do this to keep sheep attached! (this will bunch up the sheep)
					if ((--herder[idx].stuntime) <= 0) {
						herder[idx].stunframe=0;
					}
					
					// calculate movement
					herder[idx].spr.xd=herder[idx].destx-currentx;
					herder[idx].spr.yd=herder[idx].desty-currenty;

					// stunned 4 pix per frame to make offsets reliable (unless on odd offset already)
					if (herder[idx].spr.xd>1) {
						if (herder[idx].spr.x&1) {
							herder[idx].spr.xd=3;
						} else {
							herder[idx].spr.xd=4;						
						}
					}
					if (herder[idx].spr.xd<-1) {
						if (herder[idx].spr.x&1) {
							herder[idx].spr.xd=-3;
						} else {
							herder[idx].spr.xd=-4;
						}
					}
					if (herder[idx].spr.yd>1) {
						if (herder[idx].spr.y&1) {
							herder[idx].spr.yd=3;
						} else {
							herder[idx].spr.yd=4;
						}
					}
					if (herder[idx].spr.yd<-1) {
						if (herder[idx].spr.y&1) {
							herder[idx].spr.yd=-3;
						} else {
							herder[idx].spr.yd=-4;
						}
					}

					if ((currentx==origx)&&(currenty==origy)) {
						// check if we should do the next tile or abort
						if (herder[idx].spr.xd > 0) {
							if (!LevelData[gridy][gridx+1].isPassable) {
								// abort it now
								herder[idx].stuntime=0;
							}
						}
						if (herder[idx].spr.xd < 0) {
							if (!LevelData[gridy][gridx-1].isPassable) {
								// abort it now
								herder[idx].stuntime=0;
							}
						}
						if (herder[idx].spr.yd < 0) {
							if (!LevelData[gridy-1][gridx].isPassable) {
								// abort it now
								herder[idx].stuntime=0;
							}
						}
						if (herder[idx].spr.yd > 0) {
							if (!LevelData[gridy+1][gridx].isPassable) {
								// abort it now
								herder[idx].stuntime=0;
							}
						}
						if (herder[idx].stuntime == 0) {
							herder[idx].spr.xd=0;
							herder[idx].spr.yd=0;
							herder[idx].destx=0;
							herder[idx].desty=0;
							herder[idx].stunframe=0;
						}
						herder[idx].originx=origx;
						herder[idx].originy=origy;
					}

					// check - if we bump into something, or stun expires, we need to disable the stun 
					// Removed TYPE_PLAY - player collisions okay
					if ((herder[idx].stuntime <= 0) || (checkblock(currentx+herder[idx].spr.xd, currenty+herder[idx].spr.yd, TYPE_WALL | TYPE_BOX))) {
						herder[idx].stuntime=0;
						herder[idx].stunframe=0;
						herder[idx].spr.xd=0;
						herder[idx].spr.yd=0;
						// retarget a computer
						if ((herder[idx].type&PLAY_MASK) == PLAY_COMPUTER) {
#ifdef PATHFIND_DEBUG
							debug("Herder %d was stunned - retargetting.\n", idx);
#endif
							herder[idx].pathIdx=100; // off the end
						}
					}

#ifdef PATHFIND_DEBUG
					debug("Herder %d - stuntime %d, speed=%d,%d\n", idx, herder[idx].stuntime, herder[idx].spr.xd, herder[idx].spr.yd);
#endif
				}

				// If we aren't moving, then make sure we're grid aligned
				if ((herder[idx].spr.xd==0) && (herder[idx].spr.yd==0) && (herder[idx].stuntime <= 0)) {
					herder[idx].spr.x=origx;
					herder[idx].spr.y=origy;
				}

				// Now actually move the player according to the xd and yd settings, stopping at destx and desty.
				// At this point we need to check for players to know legality. Anything else should be clear already.
				// Herder's actual speed is 1/2 rated speed. To deal with fractions, we add nFrame&1 before dividing by 2
				if (herder[idx].spr.xd > 0) {
					xstep=herder[idx].spr.x+((herder[idx].spr.xd+(nFrames&1))/2);
					if (xstep > herder[idx].destx) {
						xstep=herder[idx].destx;
					}
					herder[idx].spr.x=xstep;
				}
				if (herder[idx].spr.xd < 0) {
					xstep=herder[idx].spr.x+((herder[idx].spr.xd-(nFrames&1))/2);
					if (xstep < herder[idx].destx) {
						xstep=herder[idx].destx;
					}
					herder[idx].spr.x=xstep;
				}

				if (herder[idx].spr.yd > 0) {
					ystep=herder[idx].spr.y+((herder[idx].spr.yd+(nFrames&1))/2);
					if (ystep > herder[idx].desty) {
						ystep=herder[idx].desty;
					}
					herder[idx].spr.y=ystep;
				}
				if (herder[idx].spr.yd < 0) {
					ystep=herder[idx].spr.y+((herder[idx].spr.yd-(nFrames&1))/2);
					if (ystep < herder[idx].desty) {
						ystep=herder[idx].desty;
					}
					herder[idx].spr.y=ystep;
				}

				// Now recalculate a few values for the lightning and special code
				// where we are
				currentx=herder[idx].spr.x;
				currenty=herder[idx].spr.y;
				// grid coordinates
				gridx=((currentx-PLAYFIELDXOFF)+GRIDSIZE/2)/GRIDSIZE;
				gridy=((currenty-PLAYFIELDYOFF)+GRIDSIZE/2)/GRIDSIZE;
				// grid aligned pixel coordinates
				origx=gridx*GRIDSIZE+PLAYFIELDXOFF;
				origy=gridy*GRIDSIZE+PLAYFIELDYOFF;
				// get xstep and ystep from direction
				// oldtile is still valid from above, don't need to get it again
				switch (oldtile) {
				case SHEEPLEFT:
					xstep=-GRIDSIZE;
					ystep=0;
					break;
				
				case SHEEPDOWN:
					xstep=0;
					ystep=GRIDSIZE;
					break;

				case SHEEPRIGHT:
					xstep=GRIDSIZE;
					ystep=0;
					break;

				case SHEEPUP:
					xstep=0;
					ystep=-GRIDSIZE;
					break;

				default:
					// this shouldn't happen
					xstep=0;
					ystep=0;
				}

				// Check for powerup in new location
				if (LevelData[gridy][gridx].nPowerup==1) {
					// lightning powerup
					LevelData[gridy][gridx].nPowerup=0;
					nHowToPlayFlags |= HOW_TO_PLAY_BOLTS;
					herder[idx].range++;
					if (herder[idx].range > MAXLIGHTNING) {
						herder[idx].range=MAXLIGHTNING;
						herder[idx].score+=50;
					}
					herder[idx].score+=50;
				}

				if (LevelData[gridy][gridx].nPowerup==2) {
					// speed powerup
					LevelData[gridy][gridx].nPowerup=0;
					nHowToPlayFlags |= HOW_TO_PLAY_BOOTS;
					herder[idx].speed++;
					if (herder[idx].speed > HERDERMAXSPEED) {
						herder[idx].speed=HERDERMAXSPEED;
						herder[idx].score+=50;
					}
					herder[idx].score+=50;
				}
				
				// only actually draw the sprites once!
				if (0 != speedloop) {
					continue;
				}
				
				// During special, handle color animation for Hades and Chrys
				if (((nFrames%6) == 0) && (bSpecialDim-1 == idx)) {
					if (((herder[idx].type & PLAY_CHAR_MASK) == HERD_CHRYS) || ((herder[idx].type & PLAY_CHAR_MASK) == HERD_AFROCHRYS)) {
						// use alternate palette 2 for Chrys (this seems a little pointless)
						herder[idx].spr.pal_addr+=64;		// skip two 32-byte palettes
					} else if ((herder[idx].type & PLAY_CHAR_MASK) == HERD_WOLF) {
						if (herder[idx].specialFreeze <= 2) {
							// use alternate palette 1 for Hades
							herder[idx].spr.pal_addr+=32;		// skip one 32-byte palettes
						}
					} 
				} else {
					// this won't have any effect on the other characters
					herder[idx].spr.pal_addr = pal_herder[idx];		// reset to default
				}

				if (herder[idx].stunframe > 0) {
					SPRITE tmp;
					memcpy(&tmp, &herder[idx].spr, sizeof(SPRITE));
					// package or electrocution
					tmp.tilenumber = herder[idx].stunframe;
					tmp.txr_addr = txr_sprites;
					tmp.pal_addr = pal_sprites;
					tmp.z -= 4;		// put it a bit more overtop of the effect
					SortSprite(&tmp, txrmap_sprites, POLY_HERD);
				} else {
					int nOldFrame = herder[idx].spr.tilenumber;
					if (herder[idx].stunframe == -1) {
						herder[idx].spr.tilenumber = GetDanceFrame();
					}
					SortSprite(&herder[idx].spr, herder[idx].map, POLY_HERD);
					herder[idx].spr.tilenumber = nOldFrame;
				}

				// draw the trail of sheep
				spr.nDepth=DEPTH_256x256x4;
				spr.txr_addr=txr_sheep;
				spr.pal_addr=pal_sheep;
				spr.is3D=false;
				spr.z=(fx16)2040;
				x=herder[idx].oldidx-4;
				if (x<0) x+=HISTORY;
				if (x>0) {
					y=x-1;
				} else {
					y=HISTORY-1;
				}
				for (idx2=0; idx2<min(HISTORY/4,herder[idx].sheep); idx2++) {
					// ghost sheep at the end of the trail
					if (idx2 >= herder[idx].sheep-herder[idx].ghostsheep) {
						spr.alpha=SHEEP_ALPHA_GHOST;
					} else {
						spr.alpha=SHEEP_ALPHA_NORMAL;
					}
					spr.tilenumber=SHEEPUP;
					if (herder[idx].oldy[y]<herder[idx].oldy[x]) spr.tilenumber=SHEEPDOWN;
					if (herder[idx].oldx[y]>herder[idx].oldx[x]) spr.tilenumber=SHEEPLEFT;
					if (herder[idx].oldx[y]<herder[idx].oldx[x]) spr.tilenumber=SHEEPRIGHT;
					spr.x=herder[idx].oldx[x];
					spr.y=herder[idx].oldy[x];
					if ((bSpecialDim-1 == idx) && ((specialDat[idx][0].type == SP_DISCOBALL) || (specialDat[idx][0].type == SP_GIANTDISCO))) {
						spr.tilenumber = GetDanceFrame();
					} else {
						spr.tilenumber=SheepAnimationFrame[spr.tilenumber][(nGameFrames>>2)%SHEEPFRAMES];
					}
					SortSprite(&spr, txrmap_sheep, POLY_SHEEP);
					x-=4;
					if (x<0) x+=HISTORY;
					if (x>0) {
						y=x-1;
					} else {
						y=HISTORY-1;
					}
				}

				miscflag=(int)((nGameFrames>>2)&0x01);	// every 4 frames
				
				// we can not do the special if one is already in effect
				if (bSpecialDim) {
					special=0;
				}
				
				// if we are doing specials, don't do lightning
				if (special) {
					// no action if no direction
					lightning=0;
					special=0;

					if ((xstep!=0)||(ystep!=0)) {
						if (HandleSpecial(idx, xstep, ystep, currentx, currenty, oldtile)) {
							// HandleSpecial return says to just do lightning - this
							// is just for Chrys ;)
							lightning=1;
							// Chrys gets twice the normal lightning on special
							herder[idx].charge=MAXLIGHTNING<<5;
						}
						herder[idx].special=0;
					}
				}
				
				while (lightning) {		// this is a while instead of an IF so we can break early
					int tipoff;
					bool bZombieType;

					if ((bSpecialDim-1 == idx) && (specialDat[bSpecialDim-1][0].type == SP_ZOMBIE)) {
						bZombieType = true;
					} else {
						bZombieType = false;
					}

					if ((xstep==0)&&(ystep==0)) {
						// no action if no direction
						break;
					}

					// Fire lightning in the current direction
					// Lightning goes until it hits a wall, not affected by players or sheep
					x=currentx+1;
					y=currenty+1;
  
					spr.z=(fx16)2035;
					spr.alpha=28;
					spr.is3D=false;
					spr.nDepth=DEPTH_256x256x4;
					spr.txr_addr=txr_sprites;
					spr.pal_addr=pal_sprites;		// default lightning color
					switch (oldtile) {
					// LIGHTNING frame numbers
					// Note that the frames for up and left were manually added to the
					// map to be flipped versions of  down and right so that SPR_VFLIP and
					// SPR_HFLIP could be removed -- if the misc textures are reprocessed then
					// those lines need to be added again manually. This also means the Misc
					// texture's map is larger than normal, since the reflected tiles are
					// greater than 25 :)
					 
					// DOWN:  10,11  Tip: 15,16
					// RIGHT: 12,17  Tip: 13,18
					// manual edits
					// UP:    25,26  Tip: 27,28
					// LEFT:  29,30  Tip: 31,32
					case SHEEPDOWN:	spr.tilenumber=10+miscflag; tipoff=5; break;
					case SHEEPRIGHT:spr.tilenumber=12+(miscflag?5:0); tipoff=1; break;
					case SHEEPUP:	spr.tilenumber=(25+miscflag); tipoff=2; break;
					case SHEEPLEFT:	spr.tilenumber=(29+miscflag); tipoff=2; break;
					}

					for (idx2=(herder[idx].charge>>4); idx2>0; idx2--) {
						int tmpx, tmpy;

						x+=xstep;
						y+=ystep;
						
						if (idx2==1) {
							// use the lightning tip sprite instead
							spr.tilenumber+=tipoff;
						}

						spr.x=x;
						spr.y=y;

						// Remap X and Y to grid coordinates
						tmpx=(x+(GRIDSIZE/2))/GRIDSIZE;
						tmpy=(y+(GRIDSIZE/2))/GRIDSIZE;

						// Check against solid 3D walls to stop it
						if ((LevelData[tmpy][tmpx].isPassable==false) && (LevelData[tmpy][tmpx].is3D) && (LevelData[tmpy][tmpx].nDestructible!=1)) {
							break;
						}

						// Draw the sprite 
						if (bZombieType) {
							DrawZombieLightning(spr, idx);
						} else {
							SortSprite(&spr, txrmap_sprites, POLY_MISC);
							// Fading destructible wall - stop on it
							if (LevelData[tmpy][tmpx].nDestructible > 1) {
								break;
							}
						}

						// Check against destructible walls, and nuke them :) (and stop the beam)
						if (LevelData[tmpy][tmpx].nDestructible == 1) {
							LevelData[tmpy][tmpx].nDestructible=2;
							nHowToPlayFlags |= HOW_TO_PLAY_CRATES;
							herder[idx].score+=10;
							if (!bZombieType) {
								break;
							}
						}

						// Check for stunned sheep
						for (idx3=0; idx3<MAX_SHEEP; idx3++) {
							int x,y;
							if ((sheep[idx3].stuntime > 0) || (sheep[idx3].type < 1)) {
								// Can't re-zap a stunned sheep
								continue;
							}
							if (!sheep[idx3].invincible) {
								x=(int)sheep[idx3].spr.x;
								x-=spr.x;
								if (x<0) x=x*(-1);
								y=(int)sheep[idx3].spr.y;
								y-=spr.y;
								if (y<0) y=y*(-1);
								if ((x<28)&&(y<28)) {
									if (bZombieType) {
										// actually capture this sheep as a ghost (no matter what it was)
										sheep[idx3].type=-100;		// start warping up animation
										herder[idx].sheep++;
										herder[idx].ghostsheep++;	// always capture as a ghost
										AIRetarget(idx);
										if (herder[idx].sheep <= herder[idx].maxsheep) {
											if (herder[idx].sheep > herder[idx].recaptured) {
												herder[idx].score+=10;
												herder[idx].recaptured++;
											}
										} else {
											if (sheep[idx3].recaptured > 0) {
												herder[idx].score+=100;
											} else {
												herder[idx].score+=50;
											}
											herder[idx].maxsheep=herder[idx].sheep;
										}
										sound_effect_system_throttled(SND_SHEEP1+CAUGHT, SHEEPVOL>>1);
									} else {
										// just stun this sheep :)
										herder[idx].score+=10;
										if (gStageSpecialEffect&STAGE_EFFECT_SUGAR_RUSH) {
											sheep[idx].stuntime = 5;	// barely phases them!
										} else {
											sheep[idx3].stuntime=60;	// 2 seconds stun time
										}
									}
								}
							}
						}

						// How about other herders?
						for (idx3=0; idx3<4; idx3++) {
							int x,y;
							if ((idx3!=idx)&&((herder[idx3].type&PLAY_MASK)!=PLAY_NONE)) {	// don't check ourselves or non-players
								
								// skip if already stunned, or performing a special, or disabled computer player
								if (herder[idx3].stuntime>0) continue;
								if (herder[idx3].specialFreeze>0) continue;
								if (((herder[idx3].type & PLAY_MASK)==PLAY_COMPUTER) && (gStageSpecialEffect&STAGE_EFFECT_COMPUTER_FREEZE)) continue;
								
								// else, check for collision
								x=herder[idx3].spr.x;
								x-=spr.x;
								y=herder[idx3].spr.y;
								y-=spr.y;
								if ((abs(x)<28)&&(abs(y)<28)) {
									// got this guy :)
									nHowToPlayFlags |= HOW_TO_PLAY_ZAP;
									
									// play a sound effect
									if (herder[idx3].charge > 3) {
										sound_effect_player_throttled(idx3, SND_ZEUSDAMAGE1+(ch_rand()%MAXSND_DAMAGE), PLAYERVOL);
									}

									// process score and stun
									herder[idx].score+=10;
									herder[idx3].stuntime=10;	// 1/3s stun
									herder[idx3].stunframe=0;	// no locked animation

#if 0									
									int tmpX,tmpY;
									// find a destination grid location
									tmpX=((herder[idx3].spr.x-PLAYFIELDXOFF)+GRIDSIZE/2)/GRIDSIZE;
									tmpY=((herder[idx3].spr.y-PLAYFIELDYOFF)+GRIDSIZE/2)/GRIDSIZE;
									if (LevelData[tmpY][tmpX].isPassable) {
										tmpX+=xstep/GRIDSIZE;
										tmpY+=ystep/GRIDSIZE;
										if (LevelData[tmpY][tmpX].isPassable) {
											tmpX+=xstep/GRIDSIZE;
											tmpY+=ystep/GRIDSIZE;
											if (!LevelData[tmpY][tmpX].isPassable) {
												// just back up one, we can't slide two units back
												tmpX-=xstep/GRIDSIZE;
												tmpY-=ystep/GRIDSIZE;
											}
										} else {
											// if we can't slide even one unit, then we just don't worry about it
											herder[idx3].stuntime=0;
										}
										if (herder[idx3].stuntime) {
											herder[idx3].destx=tmpX*GRIDSIZE+PLAYFIELDXOFF;
											herder[idx3].desty=tmpY*GRIDSIZE+PLAYFIELDYOFF;
										}
									} else {
										// not sure how this happened - never mind the stun part
										herder[idx3].stuntime=0;
										herder[idx3].stunframe=0;
									}
#else
									// just knock him backwards, we know the direction of the lightning
									// if that differs in axis from his own direction, use that
									if ( ((xstep == 0)&&(herder[idx3].spr.xd != 0)) ||
										 ((ystep == 0)&&(herder[idx3].spr.yd != 0)) ) {
										// use the herders axis, but move away from the lightning
										if (herder[idx3].spr.xd != 0) {
											herder[idx3].destx = (herder[idx3].spr.x - spr.x) * GRIDSIZE*4;
										} else {
											herder[idx3].desty = (herder[idx3].spr.y - spr.y) * GRIDSIZE*4;
										}
									} else {
										// use the lightning movement
										herder[idx3].destx += xstep*GRIDSIZE*4;
										herder[idx3].desty += ystep*GRIDSIZE*4;
									}
#endif
																		
									// Tweak both lightning gauges. This is to
									// prevent back and forth battles and reduce the chance of
									// trapping someone
									// in this version, we make just a small tweak - since the victim's
									// special gauge is increased, the rapid attacks have a slightly
									// different aspect to them
									if (herder[idx3].charge) herder[idx3].charge--;		// victim
									if (herder[idx].charge) herder[idx].charge--;		// aggressor
									
									// Also increment the victim's special gauge, if not max'd
									if (herder[idx3].special < MAXSPECIAL) {
										herder[idx3].special+=ADDSPECIAL;
										// on ghost stages, since the special is the ONLY way to free ghosts, count up faster
										if (gStageSpecialEffect&STAGE_EFFECT_GHOST_SHEEP) {
											herder[idx3].special+=ADDSPECIAL;
										}
										if (herder[idx3].special > MAXSPECIAL) {
											herder[idx3].special=MAXSPECIAL;
										}
									}
									
									// if it's a zombie attack, and the zombie needs speed, steal it
									if (bZombieType) {
										if ((herder[idx].speed < HERDERMAXSPEED+1) && (herder[idx3].speed >= HERDERSPEED-1)) {
											herder[idx].speed++;
											herder[idx3].speed--;
										}
									}

									// lose a sheep, if he has one that isn't a ghost, and the level isn't already over
									// note the zombie special can knock ghost sheep off AND takes 2 at a time
									if (nCountdown == -1) {
										for (int tmpcnt = 0; tmpcnt < (bZombieType?2:1); tmpcnt++) {
											if ( ((bZombieType) && (herder[idx3].sheep > 0)) ||
												 (herder[idx3].sheep > herder[idx3].ghostsheep) ) {
														
												herder[idx3].sheep--;
												if (herder[idx3].ghostsheep > herder[idx3].sheep) {
													herder[idx3].ghostsheep = herder[idx3].sheep;
												}
												// find a free sheep to start
												{
													int x,y;
													// Sheep start on grid offsets
													x=((herder[idx3].spr.x-PLAYFIELDXOFF)/GRIDSIZE);
													x=x*GRIDSIZE+PLAYFIELDXOFF;
													y=((herder[idx3].spr.y-PLAYFIELDYOFF)/GRIDSIZE);
													y=y*GRIDSIZE+PLAYFIELDYOFF;
													RestartSheep(x,y,xstep,ystep);
												}
											}
										}
									}
									// exit the loop and break the lightning
									idx2=0;
									break;
								}
							}
						}
					}

					// countdown the charge
					herder[idx].charge-=2;
					break;		// this must be here to exit the lightning while! (no looping)
				}	// while
			}	// speed loop
			// put this player into the buffer only once per frame
			drawbuffer(currentx, currenty, TYPE_PLAY);
			
			// if we aren't doing specials, hackily zero them ;)
			if ((gOptions.Powers&POW_SPECIALS) == 0) {
				herder[idx].special = 0;
			}
			if ((!isMultiplayer) && (gStageSpecialEffect&STAGE_EFFECT_NO_PLAYER_SPECIAL) && (idx == gHumanPlayer)) {
				herder[idx].special = 0;
			}
		}	// idx loop

		// reset the color in case the very last player was bright
		if (bSpecialDim) {
			myG3_Color(15,15,15);
		}

		moveSheep();

		// Update stuff for the subscreen
		if (gHumanPlayer == -1) {
			debug("gHumanPlayer is set to -1, should never happen");
		} else {
			SetScore((u32)(herder[gHumanPlayer].score + StoryModeTotalScore));
			SetNumSheep((u32)herder[gHumanPlayer].sheep);
			if (herder[gHumanPlayer].charge < 16) {
				SetLightning(0);
			} else {
				SetLightning((u8)((herder[gHumanPlayer].charge-15) << 1));
			}
			if (gOptions.Powers&POW_SPECIALS) {
				SetSpecial((u8)herder[gHumanPlayer].special);
			} else {
				SetSpecial(0);
			}
		}

		if (nLocalSetSnap) 
		{
			SetSnapFlag();
			nLocalSetSnap = 0;
		}

		pvr_scene_finish();
		// handle network sync
		if (isMultiplayer == 2) {
			// wait for all players to be on the same frame
			while (!CheckNetworkSync()) {
				OS_WaitVBlankIntr();
				SyncNetwork();
				sound_frame_update();
				if (gLostFrames > MAX_LOST_FRAMES) {
					doLostWireless();
					return 0;
				}
			}
	
			// we seem happy, so we can go on to the next frame now
			gLocalFrame++;
			// before we return to the main loop, call the real controller read. This should queue everyone at the same time
			GetController(gHumanPlayer);
		}
		
		// Check to see if any sheep were captured, and
		// check for end of level (miscflag = 0 means end of level)
		miscflag=0;
		if (-1 == nCountdown) {
			for (idx=0; idx<MAX_SHEEP; idx++) {
				if (sheep[idx].type>0) {
					miscflag=1;
					if (!sheep[idx].invincible) {
						for (idx2=0; idx2<4; idx2++) {
							if ((herder[idx2].type&PLAY_MASK) == PLAY_NONE) continue;
							if (((herder[idx2].type&PLAY_MASK) == PLAY_COMPUTER) && (gStageSpecialEffect & STAGE_EFFECT_COMPUTER_FREEZE)) continue;

							if (herder[idx2].stuntime==0) {		// no sleepy herders
								x=(int)sheep[idx].spr.x;
								x-=herder[idx2].spr.x;
								if (x<0) x=x*(-1);
								y=(int)sheep[idx].spr.y;
								y-=herder[idx2].spr.y;
								if (y<0) y=y*(-1);
								if ((x<28)&&(y<28)) {
									// if the stage effect is NOT ghost sheep, or the sheep is stunned, or Hades is in his special, we can capture
									if ( ((gStageSpecialEffect&STAGE_EFFECT_GHOST_SHEEP)==0) || (sheep[idx].stuntime != 0) ||
										((bSpecialDim-1 == idx2) && (specialDat[bSpecialDim-1][0].type == SP_WOLF)) ) {
										nHowToPlayFlags |= HOW_TO_PLAY_CAPTURE;
										// got this sheep :)
										sheep[idx].type=0;
										herder[idx2].sheep++;
										// if the stage is ghost sheep, 
										//	or Hades captures during his special,
										//	or the demon captures (during story mode only)
										// make it a ghost sheep
										if (
											(gStageSpecialEffect&STAGE_EFFECT_GHOST_SHEEP) ||
											((bSpecialDim-1 == idx2) && (specialDat[bSpecialDim-1][0].type == SP_WOLF))
										   ) {
											herder[idx2].ghostsheep++;
										}
										
										AIRetarget(idx2);

										if (herder[idx2].sheep <= herder[idx2].maxsheep) {
											if (herder[idx2].sheep > herder[idx2].recaptured) {
												herder[idx2].score+=10;
												herder[idx2].recaptured++;
											}
										} else {
											if (sheep[idx].recaptured > 0) {
												herder[idx2].score+=100;
											} else {
												herder[idx2].score+=50;
											}
											herder[idx2].maxsheep=herder[idx2].sheep;
										}
										sound_effect_system_throttled(SND_SHEEP1+CAUGHT, SHEEPVOL>>1);
										break;
									}
								}
							}
						}
					}
				}
			}
		}
		
		if (nTotalSheep > 0) {
			miscflag = 1;		// can't be the end yet
		}
		if (gStageSpecialEffect&STAGE_EFFECT_CLEAR_SHEEP_TO_WIN) {
			// if we are playing for all the sheep, check if we have them (story mode only)
			if (miscflag == 0) {
				// no sheep left on the screen, no sheep left to come out, make sure the enemy has no sheep and we're set
				if (((herder[0].type&PLAY_MASK)==PLAY_COMPUTER) && (herder[0].sheep > 0)) miscflag=1;	// he has sheep, not over
				if (((herder[1].type&PLAY_MASK)==PLAY_COMPUTER) && (herder[1].sheep > 0)) miscflag=1;	// he has sheep, not over
				if (((herder[2].type&PLAY_MASK)==PLAY_COMPUTER) && (herder[2].sheep > 0)) miscflag=1;	// he has sheep, not over
				if (((herder[3].type&PLAY_MASK)==PLAY_COMPUTER) && (herder[3].sheep > 0)) miscflag=1;	// he has sheep, not over
				if (miscflag == 0) {
					// he has no sheep, so we win!
					gBonusWinFlag = 1;
				}
			}
		}
				
		// check the bonus win flag (may be set elsewhere than right above)
		// But, we delay it's effect while a special is in effect (in theory that's yours)
		if (gBonusWinFlag) {
			miscflag = 0;			// level has ended, no matter what the above says
		}
		
		// a miscflag of 1 means to continue, a miscflag of 0 means to end the level
 
		if (nGameFrames % 30 == 0) {
			if ((gOptions.Timer > 0) && (nTimeLeft > 0) && (nCountdown==-1)) {
				// don't count down the timer during a special
				if (bSpecialDim == 0) {
					if (0 == (gStageSpecialEffect&STAGE_EFFECT_FREEZE_TIMER)) {
						nTimeLeft--;
					}
				}
				SetTimer(nTimeLeft);
				if (nTimeLeft == 0) {
					miscflag=0;		// level has ended
				}
			}
		}

		if ((nCountdown==-1)&&(miscflag==0)) {
			// do level over! - display some text about why, and fade out the music/background
			Clear2D();
			bSpecialUp = 0;		// special is no longer up if it was
			herder[gHumanPlayer].special=0;	// and make sure it doesn't re-draw
			
			// and fix the windowing in case it was training
		   	G2_SetBlendBrightness(GX_BLEND_PLANEMASK_BG2, -9); // just the shadow layer
			GX_SetVisibleWnd(GX_WNDMASK_NONE);	// turn off the window
			if (nTimeLeft == 0) {
				CenterWriteFont2D(11, 16, "TIME OVER!");
			} else {
				if ((!isMultiplayer) && (gStageSpecialEffect&(STAGE_EFFECT_SPECIAL_TO_WIN|STAGE_EFFECT_CLEAR_SHEEP_TO_WIN)) && (!gBonusWinFlag)) {
					CenterWriteFont2D(11, 16, "STAGE FAILED");
				} else {
					CenterWriteFont2D(11, 16, "STAGE COMPLETE");
				}
			}
			if (!inDemo) {
				// start the win music overtop the following animations
				sound_start(SONG_WIN, FALSE);
			}
			nCountdown=31;	// current setting, counting down

			// when zapping Iskur, I want to show the animation a little bit longer, so we add an extra second to show we got him
			if ((gBonusWinFlag)&&(gStageSpecialEffect&STAGE_EFFECT_SPECIAL_TO_WIN)) {
				nCountdown+=40*4;	// countdown decrements by 4 each time
			}
		}
		
		if (nCountdown > 0) {
			nCountdown -= 4;
			if (nCountdown < 0) nCountdown=0;
			if (nCountdown <= 31) {
				// darken the 3D layer
				Set3DDarken(31-nCountdown);
			}
		}

		if (inDemo) {
			if (CONT_START == isStartPressed()) {
				exitflag = 1;
			}
		}

		if (nCountdown==0) {
			break;
		}

		if (exitflag==1) {
			break;
		}
	}
	
	// now we get here, and the main game engine is done it's loop
	// we still need to determine winners and the graphics are still up, just dark

	// if we were in demo, make sure the user isn't holding start, then get out.
	if (inDemo) {
		while (isStartPressed()) {
			;
		}
		Clear2D();
		inDemo=0;
		return 0;
	}
	
	// both story and multiplayer modes need to do the win animation, so let's take care of that first
	// all the flags we need to do that are global, so we can just dive into it.
	int bWinner = HandleEndOfStageRanking();
	
	if (!isMultiplayer) {
		// this is story mode
		// check winner was the human
		// if we weren't the winner after all that, then game over (which handles continues)
		if (!bWinner) {
			if (doGameOver()) {
				// Player is using up a continue!
				// some vars we missed resetting
				// Note that we don't mess with the story mode flags! :)
				nGameFrames=0;
				nCountdown=-1;
				lightning=0;
				special=0;

				CLEAR_IMAGE(txr_level, 512, 512, 8);
				
				goto StoryModeContinue;
			}

			// game really is over - check the high scores
			herder[gHumanPlayer].score=StoryModeTotalScore;
			AddHighScore(gHumanPlayer, -1, 0);
			return 0;
		}
		
		// if we DID win the round
		// set the command so the code above
		// properly detects the next level
		szLastStoryCmd[0]='\0';
		
		// before we loop around, check the two special cases for minigames.
		// scripting it wasn't working right. This does nothing for stages
		// without a minigame (most of them)
		if (0 == (gStageSpecialEffect&STAGE_EFFECT_SPECIAL_TO_WIN)) {
			// the flag avoids the mini-game the second time you fight Iskur
			PlayMinigame(level);
		}

		level++;		// this will be overridden, but we need it to detect the end of game
		goto StoryModeLoop;
	} else {
		// it's a multiplayer game, we just need to get back into the multiplayer menu
		// (which, depending on network or local, is just player select or lobby)
		// herder[].wins was already updated, so we can show it in the lobby
		if (inDemo) {
			debug("Exitting demo.");
			sound_stop();
			return 1;
		}
		debug("Exitting multiplayer game. Local player %s win.\n", bWinner?"did":"did not");
	}

	inDemo=0;
	return 0;
}

// Add the current position to the list of old positions
// And adjust the pointer. The list is a circular buffer
// HISTORY entries long. pHerd->oldidx is the current
// position, and pHerd->oldidx-1 is the previous position
void updateoldpos(HerdStruct *pHerd) {
	pHerd->oldidx++;
	if (pHerd->oldidx>=HISTORY) pHerd->oldidx=0;

	pHerd->oldx[pHerd->oldidx]=pHerd->spr.x;
	pHerd->oldy[pHerd->oldidx]=pHerd->spr.y;
}

void ReloadMenu(char *psz) {
	// used from within the menus
	// hide the reload
	ShowBlack();
	sound_stop();
	// need to reload the title page graphics
	load_bmp_block_mask("gfx/Misc/mainmenu1.bmp", NULL, 0, 0, -1);
	load_bmp_block_mask("gfx/Misc/mainmenu2.bmp", NULL, 128*1024, 0, -1);
	load_bmp_block_mask("gfx/Misc/mainmenu3.bmp", NULL, 256*1024, 0, -1);
	// and need to set the text back up (hacky, assumptions)
	WriteFont2D(24, 0, psz);
	SetOffset2D(0,18);
	// and need to restart the music
	sound_start(SONG_TITLE, 1);
}

// Multiplayer starting from the multiplayer menu
void HandleMultiplayer() {
	int nMultiOption;
	int PlayerSelectRecall=0;
	int bKeepMenu;
	int loopWireless = 0;
		
restartMultiSelect:
	MenuScr_Init();
	memset(&gMPStuff, 0, sizeof(gMPStuff));

	bKeepMenu=true;
	while (bKeepMenu) {
		nMultiOption = doMenu(MENU_MULTIPLAYER, NULL);
		
		if (MENU_CANCEL == nMultiOption) {
			return;
		}

		switch (nMultiOption) {
		case MENU_HOST_GAME:
			// we are always player 0 as the host
			gHumanPlayer = 0;
			
			// We are hosting the game so set the flag
			gMPStuff.fHosting = 1;
			do {
				loopWireless = HandleWirelessGame(loopWireless);
			} while (loopWireless);
			gMPStuff.fHosting = 0;
			
			// now we are done, turn off wireless
			EndWireless();
			
			// and revert the controller map
			gHumanPlayer = 0;
			for (int idx=0; idx<4; idx++) {
				ControllerState[idx] = eContNone;
			}
			ControllerState[gHumanPlayer] = eContLocal;
			
			ReloadMenu("Battle Mode");
			HandleTopMenuView(SCROLL_TARG_EAST);
			MenuScr_Init();
			break;
			
		case MENU_JOIN_GAME:
			// we are not hosting so go look for a game
			gMPStuff.fHosting = 0;
			nMultiOption = doJoinMultiMenu();
			
			if (MENU_CANCEL != nMultiOption) {
				// don't leave any left-over messages
				do {
					QueueNotReady();
					loopWireless = HandleWirelessGame(loopWireless);
				} while (loopWireless);
			}
			
			// now we are done, turn off wireless
			EndWireless();

			// and revert the controller map
			gHumanPlayer = 0;
			for (int idx=0; idx<4; idx++) {
				ControllerState[idx] = eContNone;
			}
			ControllerState[gHumanPlayer] = eContLocal;

			ReloadMenu("Battle Mode");
			HandleTopMenuView(SCROLL_TARG_EAST);
			MenuScr_Init();
			break;

		case MENU_QUICKPLAY:
			// zero out the herder names
			herder[0].name[0]='\0';
			herder[1].name[0]='\0';
			herder[2].name[0]='\0';
			herder[3].name[0]='\0';
		
			bKeepMenu = false;
			break;
			
		case MENU_BATTLE_OPTIONS:
			doBattleOptionsMenu(NULL);
			break;

		default:	// includes MENU_CANCEL
			return;
		}
	}

	// Now start the music
	sound_start(SONG_SELECT, 1);
	Clear2D();

	// label used to loop the player select if backing out of level select
restartPlayerSelect:
	MapScr_ShowBlack();
	MenuScr_Init();

	if (-1 == doPlayerSelect(PlayerSelectRecall, NULL)) {
		ReloadMenu("Battle Mode");
		HandleTopMenuView(SCROLL_TARG_EAST);
		goto restartMultiSelect;
	}
	
	// set up computer players (everyone but the human)
	for (idx=0; idx<4; idx++) {
		if (idx != gHumanPlayer) {
			// this data isn't copied into the local structure yet
			if (gGame.Options.CPU) {
				int o1,o2,o3;
				herder[idx].type = PLAY_COMPUTER|(ch_rand()%12);
				herder[idx].color = ch_rand()%16;
				if (idx == 0) o1=3; else o1=0;
				if (idx == 1) o2=3; else o2=1;
				if (idx == 2) o3=3; else o3=2;
				// make sure there's no color match with the human (we don't check character here)
				while ((herder[o1].color == herder[idx].color) || (herder[o2].color == herder[idx].color) || (herder[o3].color == herder[idx].color)) {
					herder[idx].color = (herder[idx].color+1)&0x0f;
				}
			} else {
				// so.. yeah, you can play by yourself
				herder[idx].type = PLAY_NONE;
			}
		}
	}
	
	PlayerSelectRecall=-1;

	// this label is used in multiplayer
restartStageSelect:

	if (-1 == doStageSelect(99, NULL)) {		// all stages allowed
		goto restartPlayerSelect;
	}

	if (0 == Game(1)) {
		sound_stop();
		sound_start(SONG_SELECT, 1);
		goto restartPlayerSelect;
	}
}

#ifdef PATHFIND_DEBUG
void AIRetarget(int who) {
#else
void AIRetarget(int) {
#endif

	int idx;
	
	for (idx=0; idx<4; idx++) {
		if ((herder[idx].type&PLAY_MASK)==PLAY_COMPUTER) {
			int n;
			// got a sheep, everyone check their target
			n=(int)(herder[idx].CPUStatus>>8);
			if ((n&0x80)==0) {
				if (sheep[n].type <= 0) {
#ifdef PATHFIND_DEBUG
					debug("Herder %d sheep was captured by %d - retargetting.\n", idx, who);
#endif
					herder[idx].pathIdx=100;
				}
			}
		}
	}
}

static char extraStrings[] = {
	"the menu team thanks david schultz "
	"my little pony, my little pony... "
};
