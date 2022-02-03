/****************************************/
/* Cool Herders                         */
/* http://harmlesslion.com              */
/* Copyright 2009 HarmlessLion LLC      */
/* menu.c                               */
/****************************************/

// this code is awful full of copy and paste

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <nitro/os.h>		// NITRO

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
#include "menu.h"

#include "wireless/chwh.h"
#include "wireless/chwireless.h"

#include "waterworksmini.h"
#include "special.h"
#include "sheep.h"

// new minimal credits
extern void credzMain(int reload);
// png handler
extern int StoryModeTotalScore;
extern int nContinues;
extern int SheepAnimationFrame[4][4];
extern const int DanceFrames[44];
// special interface for disco ball
extern int playeridx;
extern void HandleTrey();
extern void ProcessTrey(int who, int idx, int nCountdown);
extern SpecialStruct specialDat[4][NUM_SPECIALS];
extern int nSoundGroups[13];

// owner data
extern OSOwnerInfo OwnerInformation;	// NITRO: for TWL make OSOwnerInfoEx

// variables used for game options
struct gamestruct gGame;
struct gameoptions gOptions;
struct multiplay_options gMPStuff;

unsigned int lastbuttons[4] = { CONT_START, CONT_START, CONT_START, CONT_START };
char buf[128];		// work buffer
int nCol=0, nCol2;	// for the high score table
int gIsContinuingStory=0;

extern pvr_ptr_t txr_sheep;
extern pvr_ptr_t txr_level;
extern pvr_ptr_t txr_herder[6], txr_misc,pal_misc;
extern pvr_ptr_t pal_sprites, pal_sheep, pal_level, pal_herder[6];
extern pvr_ptr_t pal_tmp[4];

extern TXRMAP txrmap_sheep[];
extern int inDemo;
extern unsigned int myJiffies;
extern int nFrames;
extern int level, stage;

#define PACKRGB(r,g,b) 	((u16)(((b>>3)<<10)|((g>>3)<<5)|(r>>3)))
u16 colrgb[5] = {	PACKRGB(255,128,128), PACKRGB(255,204,0), PACKRGB(0,153,255), PACKRGB(255,128,255), 0x7FFF };

// txr_level is large enough for four of these level screens at 256x178
pvr_ptr_t pLevs[4]= { INVALID_PTR, INVALID_PTR, INVALID_PTR, INVALID_PTR };
pvr_ptr_t pPals[4]= { INVALID_PTR, INVALID_PTR, INVALID_PTR, INVALID_PTR };

// All characters besides Zeus must be earned
// See enum herdTypes in menu.h
char szNames[16][8] = {	
	"zeus",		// Zeus					
	"herd",		// Basic herder			
	"cand",		// Candy Striper			
	"nh-5",		// NH-5 robot minion	
	"danc",		// Backup dancer		
	"zomb",		// Zombie herder		

	// Alternates! (x+6)
	"thal",		// Thalia				
	"iskr",		// Iskur				
	"angl",		// Angel herder			
	"wolf",		// Wolf					
	"trey",		// God of Dance			
	"devl",		// Devil herder			

	// bonus
	"shep",		// Chrys
	"zfro",		// Afro Zeus
	"sfro",		// Afro Chrys

	// unused
	"",
};

// these must be 10 chars or less
char CharacterNames[15][11] = {
	"Zeus",
	"Classic",
	"Candy",
	"NH-5",
	"Dancer",
	"Zombie",
	"Thalia",
	"Iskur",
	"Angel",
	"Hades",
	"Trey Volta",
	"The Demon",
	"Chrys",
	"Afro Zeus",
	"Afro Chrys"
};

// these are the strings of text for each herder with quotes (first 12)
char *haveUherd[12] = {
	"Everybody knows\nI was born to\ndo this, and my\nLIGHTNING SHOCK\nwill make sure\nthey remember.",
	"My opponents\nthink all I\ndo is herd.\nThey will come\nto fear my\nKIWI CALL.",
	"A GIANT GUMBALL\nor two will\ncrush anyone\nwho isn't \nsweet enough\nto let me win.",
	"SHEEP GATHERING\nINITIATED.     \n* COMPETITION  \nPROTOCOL *     \nOPERATION:     \nGIFT WRAPPING  ",
	"Wooly feet\ndance to the\nbeat ...\nUnder the\nthrall of my\nDISCO BALL.",
	"...\n\n...sheeeeeps...\n\n...",
	"I don't know\nmuch about\nherding, but\nmy POWER BEAM\nshould come in\nhandy...",
	"Those who do\nnot respect my\nFLASH FLOOD\nwill be left\nin my wake.",
	"Born of the\nflock, return\nto the flock.\nTheir way will\nbe lit by my\nHOLY LIGHT.",
	"The POWER OF\nTHE MOON will\nfuel my hunger\nto win...\nAnd sheep.\nmmm... Sheep.",
	"Dance! Boogie!\nGet Down!\nMy GIANT DISCO\nBALL will make\nthis battle as\ngroovy as I am.",
	"DOOM!\nDOOM!\nThis is the\nfate of the\nvictims of my\nDEMON BLAST!"
};

// The sound group to load in relation to a given character
//extern int nSoundGroups[];

// Handles the rendering for a simple menu
// set the high word to the preferred menu option if you want that
// $oooommmmm - oooo is option (default 0), mmmm is menu number
// CallBack is called every frame if not NULL
int doMenu(int nMenuNumber, void (*CallBack)()) {
	static int nLastMenu=-99;
	int nOption;
	
	MenuScr_InitMenu(nMenuNumber);
	nMenuNumber&=0xffff;
	
	// handle top screen for menus
	debug("Menu #%d\n", nMenuNumber);
	
	if (nLastMenu != nMenuNumber) 
	{
		nLastMenu = nMenuNumber;
		switch (nMenuNumber) 
		{
			default:	// not a menu that uses the top image
				HandleTopMenuView(SCROLL_TARG_NONE);
				break;
				
			case MENU_MAIN:			// main menu
			case MENU_PRESSSTART:	// 'press start' screen
				// okay to use the bitflag with no secondary target
				HandleTopMenuView(SCROLL_TARG_MAIN);
				break;
				
			case MENU_STORY:		// story mode
				HandleTopMenuView(SCROLL_TARG_WEST);
				break;
				
			case MENU_MULTIPLAYER:	// multiplayer
				HandleTopMenuView(SCROLL_TARG_EAST);
				break;
				
			case MENU_EXTRAS:		// extras
			case MENU_BONUS:		// also uses the same screen
				HandleTopMenuView(SCROLL_TARG_NORTH);
				break;
				
			case MENU_OPTIONS:		// options
				HandleTopMenuView(SCROLL_TARG_SOUTH);
				break;
		}
	}
	
	while (!MenuScr_DrawFrame(&nOption)) 
	{	
		// handles the 3d side and end of frame code
		if (!HandleTopMenuView(0)) {
			// otherwise, use the safe 2d end of frame
			pvr_scene_finish_2d();
		}
		
		if (NULL != CallBack) {
			// let the caller do some work
			CallBack();
		}
	}
	
	sound_effect_system(SND_CLICK, SHEEPVOL);
	
	MenuScr_CloseMenu(0);

	return nOption;
}

// this is used by the error screens below
void PrepareError() {
	MenuScr_CloseMenu(0);		// no effect if nothing open

	for (int idx=0; idx<4; idx++) {
		ControllerState[idx]=eContNone;
	}
	ControllerState[gHumanPlayer]=eContLocal;

	MenuScr_Init();
	*(u16*)(HW_DB_BG_PLTT+258) = 0x0000;	// set palette to black
	*(u16*)(HW_DB_BG_PLTT+264) = 0x0000;	// set color 132
	MenuScr_InitMenu(MENU_STORY_TEXT);
}

// display a fatal wireless error to the user, suggest reboot, etc
void doWirelessError() {
	int nOption;
	
	debug("FATAL WIRELESS ERROR OCCURRED. OI.");
	
	// shut it down
	WH_Finalize();
	
	PrepareError();
	MenuScr_UpdateMenuString(0, "A wireless error occurred.\nPlease try again. If the\nproblem persists, please\nrestart your game system.\nPress A to continue.");
	while (!MenuScr_DrawFrame(&nOption)) {
		pvr_scene_finish_2d();
	}
	MenuScr_CloseMenu(0);
}

// display a loss of network other than wireless error
void doLostWireless() {
	int nOption;
	
	debug("WIRELESS COMMS HAVE BEEN LOST.");

	// shut it down
	WH_Finalize();

	PrepareError();
	MenuScr_UpdateMenuString(0, "Communications were lost.\nPlease try again. If the\nproblem persists, please\nrestart your game system.\nPress A to continue.");
	while (!MenuScr_DrawFrame(&nOption)) {
		pvr_scene_finish_2d();
	}
	MenuScr_CloseMenu(0);
}

// Host is deliberately ending the game
void doHostQuit() {
	int nOption;
	
	debug("Host has quit the game.");

	// shut it down
	WH_Finalize();

	PrepareError();
	MenuScr_UpdateMenuString(0, "The host has closed the game.\nYou will return to the menu.\nPress A to continue.");
	while (!MenuScr_DrawFrame(&nOption)) {
		pvr_scene_finish_2d();
	}
	MenuScr_CloseMenu(0);
}

// Does the join multiplayer select menu
// returns -1 to cancel or the game number being joined
int doJoinMultiMenu() {
	int nOption;
	
redoEverything:	
	// turns on wireless
	if (!InitializeChild()) {
		// wireless failed to initialize
		EndWireless();		// does this leave things okay even if the stack never came up?
		doWirelessError();
		return -1;
	}

redoJoinMenu:
	MenuScr_InitMenu(MENU_STAGEINTRO_TEXT);	
	MenuScr_UpdateMenuString(0, "scanning");
	memset(&gMPStuff, 0, sizeof(gMPStuff));
	
	// force a fixed number of scans to build the list
	for (int idx=0; idx<=48; idx++) {
		// Draw the text cloud
		MenuScr_DrawFrame(&nOption);

		pvr_scene_finish_2d();
		
		if (!ScanForHostGames()) {
			// network error occured
			EndWireless();
			doWirelessError();
			return -1;
		}
	}
	
	MenuScr_CloseMenu(0);
	MenuScr_InitMenu(MENU_JOIN_MULTIPLAYER);

	// now run the menu
	while (!MenuScr_DrawFrame(&nOption)) {
		pvr_scene_finish_2d();
	}
	
	if (nOption == MENU_JOIN_SCAN) {
		// scan
		MenuScr_CloseMenu(0);
		goto redoJoinMenu;
	}
	
	sound_effect_system(SND_CLICK, SHEEPVOL);
	MenuScr_CloseMenu(0);

	if (!EndScanning()) {
		EndWireless();
		doWirelessError();
		return -1;
	}
	
	if (nOption > -1) {
		// actually join the game, if we can
		MenuScr_InitMenu(MENU_STAGEINTRO_TEXT);	
		MenuScr_UpdateMenuString(0, "connecting");

		// just enough frames to animate the bubble up
		for (int idx=0; idx<14; idx++) {
			int nDummy=0;
			
			// Draw the text cloud
			MenuScr_DrawFrame(&nDummy);
			pvr_scene_finish_2d();
		}
		
		// this just starts the scan, so it's failure is an error
		if (!WH_ChildConnectAuto(WH_CONNECTMODE_DS_CHILD, gMPStuff.MPGames[nOption].MAC, 0)) {
	    	debug("WH_ChildConnectAuto failed.");
			EndWireless();
			doWirelessError();
			return -1;
		}
		
		// now wait and see if we connect
		if ((!WaitForDataSharing()) || (WH_GetSystemState() != WH_SYSSTATE_DATASHARING)) {
			// hopefully wireless is not down, the host just went away
			// note we could get more information if we wanted to decipher the state/error code
			EndWireless();
			MenuScr_CloseMenu(0);
			MenuScr_InitMenu(MENU_STORY_TEXT);	
			*(u16*)(HW_DB_BG_PLTT+258) = 0x0000;	// set palette to black
			*(u16*)(HW_DB_BG_PLTT+264) = 0x0000;	// set color 132
			MenuScr_UpdateMenuString(0, "Could not connect\nto the host.\nPress A to continue.");
			while (!MenuScr_DrawFrame(&nOption)) {
				pvr_scene_finish_2d();
			}
			MenuScr_CloseMenu(0);
			goto redoEverything;
	    }
		
		MenuScr_CloseMenu(0);
	}

	return nOption;
}

// This draws the options menu
// Always returns 0
int doOptionsMenu() {
	int nOption;

	MenuScr_InitMenu(MENU_OPTIONS);
	HandleTopMenuView(SCROLL_TARG_SOUTH);
	
	MenuScr_DoOptions(1);

	while (!MenuScr_DrawFrame(&nOption)) 
	{
		if(!HandleTopMenuView(0)) {
			// We aren't using 3d, of course, but there is some centralized code here that should be
			// run every frame.
			pvr_scene_finish_2d();
		}
	}
	
	sound_effect_system(SND_CLICK, SHEEPVOL);
	
	if (-1 != nOption) {
		// Less than ideal, but it gets the point across.  When this is not -1 (Back), the user wants to
		// keep their options.
		debug("Saving options...\n");
		gGame.AutoSave = MenuScr_CurrentOptions.bAutosave;
		gGame.SVol = MenuScr_CurrentOptions.nSoundVol;
		gGame.MVol = MenuScr_CurrentOptions.nMusicVol;
		if (!gGame.AutoSave) gGame.SaveSlot=0;
	}
	MenuScr_CloseMenu(0);
	MenuScr_DoOptions(0);

	if ((-1 != nOption) && (gGame.AutoSave)) {
		doVMUSave(1,gGame.SaveSlot);
		sound_start(SONG_TITLE, 1);		
	}

	// fix mixer either way	
	set_sound_volume(gGame.MVol, gGame.SVol);

	debug("Exitting options menu because Fell out of loop\n");
	return 0;
}

// This draws the battle options menu
// Always returns 0. Used by both multiplayer host and
// solo multiplayer. To make the cancel work, this version
// modifies gOptions and saves to gGame.Options
int doBattleOptionsMenu(void (*CallBack)()) {
	int nOption;

	MenuScr_InitMenu(MENU_OPTIONS);
	MenuScr_DoBattleOptions(1);
	memcpy(&gOptions, &gGame.Options, sizeof(gOptions));

	// waits for input
	while (!MenuScr_DrawFrame(&nOption)) 
	{
		pvr_scene_finish_2d();
		if (NULL != CallBack) CallBack();
	}
	
	sound_effect_system(SND_CLICK, SHEEPVOL);
	
	if (-1 != nOption) {
		// Less than ideal, but it gets the point across.  When this is not -1 (Back), the user wants to
		// keep their options.
		debug("Saving options...\n");
		memcpy(&gGame.Options, &gOptions, sizeof(gOptions));
		// only autosave this in solo mode
		if (!gMPStuff.fHosting) {
			if (gGame.AutoSave) {
				doVMUSave(1, gGame.SaveSlot);
				sound_start(SONG_TITLE, 1);		
			}
		}
	}
	MenuScr_CloseMenu(1);
	while (MenuScr_DrawMenuItems()) {
		pvr_scene_finish_2d();
		if (NULL != CallBack) CallBack();
	}
	MenuScr_DoBattleOptions(0);

	debug("Exitting battle options menu because Fell out of loop\n");
	return 0;
}

// This draws the system info menu
// Always returns 0
int doSysInfoMenu() {
	int nOption;

	MenuScr_InitMenu(MENU_OPTIONS);	// reuses this frame
	
	MenuScr_DoSystemInfo(1);

	while (!MenuScr_DrawFrame(&nOption)) 
	{
		// We aren't using 3d, of course, but there is some centralized code here that should be
		// run every frame.
		pvr_scene_finish_2d();
	}
	
	sound_effect_system(SND_CLICK, SHEEPVOL);
	
	MenuScr_CloseMenu(0);

	MenuScr_DoSystemInfo(0);

	debug("Exitting menu because Fell out of loop\n");
	return 0;
}

// Player select system - return -1 for back
// if Recall is -1, reuse the cached settings for who selected what
// if Recall is >0, use the passed in value as the desired setting
// Needs to set the player in herder[].nType -- all four players
// need to have their type set as well
int doPlayerSelect(int nRecall, void (*CallBack)()) {
	static int nType = -1; 
	int nOption;
	int nLoaded = -1;	// same as default nType
	
	// check for invalid setting (secret chars)
	if (nRecall > 0) {
		nType = nRecall-1;
		nRecall = -1;
	}
	if (nType > 11) {
		nType = -1;		// unset
	}
	
	ShowBlack();
	
	// stop using the top menu screen
	HandleTopMenuView(SCROLL_TARG_NONE);

	// load the needed graphics into the herder tilesets
	// 0 = background with woodgrain(256x192x4)
	// 1 = currently loaded face
	load_bmp_block_mask("gfx/Menu/Menu_BG_combined4.bmp", NULL, txr_herder[0], pal_herder[0], -1);

	// reuses this
	MenuScr_InitMenu(MENU_OPTIONS);
	MenuScr_ResetSubScr();
	MenuScr_DoPlayerSelect(1, nRecall?nType:-1);

	// label used to handle random	
continuemenu:
	while (!MenuScr_DrawFrame(&nOption)) {
		// check for new load 
		int t = MenuScr_GetSelOption();
		if ((t>=0) && (t<=11) && (t != nLoaded)) {
			// select the image
			sprintf(buf, "gfx/Menu/%s_head.bmp", szNames[t]);	
			load_bmp_block_mask(buf, NULL, txr_herder[1], pal_herder[1], -1);
			Clear2D();
			CenterWriteFontBreak2D(6,10,haveUherd[t]);
			nLoaded=t;
		}
		if (t != 12) {
			// don't change color when on random
			u16 btns = GetController(gHumanPlayer);
		
			if ((lastbuttons[gHumanPlayer]&(CONT_R|CONT_L)) != (btns & (CONT_R|CONT_L)))	{
				if (btns & CONT_L) {
					// previous color
					OwnerInformation.favoriteColor--;
					OwnerInformation.favoriteColor&=0x0f;
					sprintf(buf, "Color:%X", OwnerInformation.favoriteColor);
					WriteFont2D(22, 24, buf);
				}
				if (btns & CONT_R) {
					OwnerInformation.favoriteColor++;
					OwnerInformation.favoriteColor&=0x0f;
					sprintf(buf, "Color:%X", OwnerInformation.favoriteColor);
					WriteFont2D(22, 24, buf);
				}
			}
			lastbuttons[gHumanPlayer]=btns;
		}
	
		pvr_scene_begin();
		// draw background image here
		SortFullPictureX(GX_TEXFMT_PLTT16, txr_herder[0], pal_herder[0], 31);

		if (-1 != nLoaded) {
			// also sort the loaded image
			DrawTexture128(txr_herder[1], pal_herder[1], 114, 32, 31);
		}
	
		pvr_scene_finish();
		
		if (NULL != CallBack) CallBack();
	}

	// check what we got	
	if (-1 != nOption) {
		// -1 means B or back was pressed
		if (nOption == 12) {
			u16 btns;
		
			// handle random - special characters are handled here too
			// hold down the shoulder buttons :)
			nOption=ch_rand()%12;

			btns = GetController(gHumanPlayer);
			lastbuttons[gHumanPlayer]=btns;

			if (btns & CONT_L) {
				// left button for Chrys
				nOption=HERD_CHRYS;
			}
			if (btns & CONT_R) {
				if (btns & CONT_L) {
					// both buttons for afro chrys
					nOption = HERD_AFROCHRYS;
				} else {
					// right button for afro zeus
					nOption = HERD_AFROZEUS;
				}
			}
			debug("Random select player %d\n", nOption);
			if (nOption < 11) {
				// random select, normal character, treat as selected
				sound_effect_system(SND_CLICK, SHEEPVOL);
				nType=nOption;
				MenuScr_SetSelOption(nOption);	// menu override
				DisableControlsTillReleased();
				goto continuemenu;
			}
		}
		
		nType = nOption;		// cache the local result
		herder[gHumanPlayer].type = nType;
		herder[gHumanPlayer].color = OwnerInformation.favoriteColor;
	}
	MenuScr_CloseMenu(1);
	while (MenuScr_DrawMenuItems()) {
		pvr_scene_finish_2d();
		if (NULL != CallBack) CallBack();
	}

	MenuScr_DoPlayerSelect(0,0);
	
	debug("Exitting menu because Fell out of loop\n");
	if (nOption == -1) {
		return -1;
	} else {
		return 0;
	}
}

// Stage select system - return -1 for backing up
// needs to set 'level'. nLevel is the maximum level to allow (NZ = 0)
// CallBack is called every frame (used for network management in multiplayer), set to NULL if not used
// if nMaxLevel is 99, then Heaven is enabled to be selected with a cheat
int doStageSelect(int nMaxLevel, void (*CallBack)()) {
	static int nCurrentLevel=0;	// static so we can remember the last choice
	int nOption;
	int nLoaded=-1;
	int nDarken = 31;			// 31 until an image is loaded

	if (nMaxLevel > LEVEL_UNDERWORLD) {
		nMaxLevel = LEVEL_UNDERWORLD;
	}

	// this can be called before player select in continue mode
	// stop using the top menu screen
	HandleTopMenuView(SCROLL_TARG_NONE);

	// reuses this
	MenuScr_InitMenu(MENU_OPTIONS);
	Clear2D();
	
	// load background
	load_bmp_block_mask("gfx/Menu/World.bmp", NULL, txr_misc, pal_misc, -1);
	
	// setup menu	
	MenuScr_DoWorldSelect(1, nCurrentLevel, nMaxLevel);
	
	while (!MenuScr_DrawFrame(&nOption)) {
		// check for new load 
		int t = MenuScr_GetSelOption();
		if ((t>=0) && (t<=7) && (t != nLoaded)) {
			// get rid of the old image so we don't see the new one load
			pvr_scene_begin();
			// draw darkened background
			SortFullPictureX(GX_TEXFMT_PLTT256, txr_misc, pal_misc, 16);
			pvr_scene_finish();
			
			// select the image
			nLoaded=t;
			sprintf(buf, "gfx/Menu/level%d.bmp", nLoaded);	
			load_bmp_block_mask(buf, NULL, txr_level, pal_level, -1);
			nDarken = 5;
		}
		
		pvr_scene_begin();
		
		// draw darkened background
		SortFullPictureX(GX_TEXFMT_PLTT256, txr_misc, pal_misc, 16);
		
		if (nLoaded >= 0) {
			if (nDarken < 31) {
				nDarken+=5;
				if (nDarken > 31) nDarken=31;
			}
			scaleimage(txr_level, pal_level, 64, 48, 191, 143, nDarken);
		}
		
		pvr_scene_finish();

		// handle callback
		if (NULL != CallBack) CallBack();
	}
	
	sound_effect_system(SND_CLICK, SHEEPVOL);
	
	if (-1 != nOption) {
		// -1 means B or back was pressed
		nCurrentLevel = nOption;		// cache the local result
		level = nCurrentLevel;
		// TODO: handle random stage select
	}
	MenuScr_CloseMenu(1);
	while (MenuScr_DrawMenuItems()) {
		if (CallBack) CallBack();
	}

	MenuScr_DoWorldSelect(0,0,0);

	debug("Exitting menu because Fell out of loop\n");

	if (-1 == nOption) {
		return -1;
	} else {
		return 0;
	}
}

// Handles the sub-menu for the story mode options
// return 0 = story was completed, reset everything
// 2 - cancel, just go back one level
int HandleStoryMenu(void) 
{
	int nBlockNumber = 0;
	
	gIsContinuingStory = 0;		// helps find the right start point in the slideshow
	
	for (;;) // Keep running the menu till someone explicitly wants to exit
	{
		// zero out the herder names
		herder[0].name[0]='\0';
		herder[1].name[0]='\0';
		herder[2].name[0]='\0';
		herder[3].name[0]='\0';
	
		switch (doMenu(MENU_STORY,NULL)) {
			case MENU_CANCEL:
				debug("Leaving story menu...\n");
				return 2;
			
			case MENU_STORY_NEW:
				debug("Running story mode...\n");
				MapScr_ShowBlack();
				ShowBlack();

				level=LEVEL_NZ;
				nContinues=PLAYER_CONTINUES;

				if (0xbbaa == Game(0)) {
					// player won! Do credits!
					credzMain(1);
					herder[gHumanPlayer].score=StoryModeTotalScore;
					AddHighScore(gHumanPlayer, -1, 0);
				}

				return 0;

			case MENU_CONTINUE:		// Story Mode with continue for where we left off
				debug("Running continue...\n");

				MapScr_ShowBlack();
				ShowBlack();
				
				level=gGame.continuelevel;
				nContinues=PLAYER_CONTINUES;

				MenuScr_Init();
				if (-1 == doStageSelect(level, NULL)) {
					ReloadMenu("Story Mode");
					HandleTopMenuView(SCROLL_TARG_WEST);
					MenuScr_Init();
					continue;		// loop this menu on cancel
				}
				if (level > LEVEL_NZ) {
					gIsContinuingStory=1;
				} else {
					gIsContinuingStory=0;
				}

				if (0xbbaa == Game(0)) {
					// player won! Do credits!
					credzMain(1);
					herder[gHumanPlayer].score=StoryModeTotalScore;
					AddHighScore(gHumanPlayer, -1, 0);
				}

				return 0;
				
			case MENU_LOAD:
				doVMULoad(0);
				sound_start(SONG_TITLE, 1);
				ReloadMenu("Story Mode");
				HandleTopMenuView(SCROLL_TARG_WEST);
				MenuScr_Init();
				continue;
		}
	}
	return 0;
}

// Handles the sub-menu for the extras
void HandleExtrasMenu(void) 
{
	int nTmp=0;
	
	for (;;) {
		switch (doMenu(MENU_EXTRAS,NULL)) {
			default:
			case MENU_CANCEL:
				debug("Leaving extras menu...\n");
				return;
				
			case MENU_FILE:
				debug("Entering file menu...\n");
				nTmp=0;
				while (nTmp==0) {
					nTmp=1;		// normally one loop
					switch (doMenu(MENU_FILE,NULL)) {
						case MENU_LOAD:
							doVMULoad(0);
							sound_start(SONG_TITLE, 1);		
							break;
						case MENU_SAVE:
							doVMUSave(0,0);
							sound_start(SONG_TITLE, 1);		
							break;
						case MENU_AUTOSAVE:
							gGame.AutoSave=!gGame.AutoSave;
							nTmp=0;	// repeat
							break;
					}
				}
				break;
			
			case MENU_DEMO:
				debug("Starting demo...\n");
				inDemo=1;
				return;
				
			case MENU_BONUS:
				debug("Entering bonus menu...\n");
				doBonusMenu();
				break;
		}
	}
	
	return;
}

// Main wrapper for menuing system
// only return from here if you want to
// back out to the 'press start' screen,
// or run a demo (set inDemo to 1)
void HandleMainMenus() {
	int nMultiOption = 0;

	while (!inDemo) // Keep running the menu till someone explicitly wants to exit or demo starts
	{
		MenuScr_ResetSubScr();
		MenuScr_InitMenu(MENU_PRESSSTART);
		
		// zero out the herder names
		herder[0].name[0]='\0';
		herder[1].name[0]='\0';
		herder[2].name[0]='\0';
		herder[3].name[0]='\0';
		

		switch (doMenu(MENU_MAIN,NULL)) {
		case MENU_CANCEL:			// cancel
			sound_stop();
			debug("Leaving main menu...\n");
			return;

		case MENU_STORY:
			debug("Running story menu...\n");
			if (0 == HandleStoryMenu()) {
				debug("Story ended, restart menu.");
				inDemo=0;	// make sure?
				return;
			}
			break;

		case MENU_MULTIPLAYER:		// multiplayer
			debug("Running multiplayer...\n");
			HandleMultiplayer();
			break;

		case MENU_OPTIONS:
			doOptionsMenu();
			// options doesn't use doMenu, so we need to manually
			// set the scroll target back to main
			HandleTopMenuView(SCROLL_TARG_MAIN);
			break;

		case MENU_EXTRAS:
			debug("Running extras menu...\n");
			HandleExtrasMenu();	
			break;
			
		case MENU_SYS_INFO:		// hidden sys info (L+R+OPTIONS)
			debug("Entering sysinfo menu...\n");
			doSysInfoMenu();
			break;
		
		case MENU_DEMO:
			debug("Running demo...\n");
			inDemo=1;
			sound_stop();
			return;		
		}
	}

}

void HSIntr() {
	// this is called for every 16 pixels on the high score table
	// so that we can set the text color dynamically
	s32 val = GX_GetVCountEqVal();
	if (val == 0) {
		nCol2=nCol;						// reset for top of screen
		*(u16*)(HW_PLTT+30) = 0x7fff;	// set top line to white
		// nothing to change on subscreen
	} else {
		*(u16*)(HW_PLTT+30) = colrgb[nCol2];	// set color
		if (val == 32) {
			*(u16*)(HW_DB_BG_PLTT+130) = 0x7fff;	// set top line to white 
			*(u16*)(HW_DB_BG_PLTT+136) = 0x7fff;	// font gfx bug, also need this one
		} else {
			*(u16*)(HW_DB_BG_PLTT+130) = colrgb[nCol2];	// set color
			*(u16*)(HW_DB_BG_PLTT+136) = colrgb[nCol2];	// set color 
		}
		nCol2++;
		if (nCol2 > 3) {
			nCol2=0;
		}
	}
	
	// set next interrupt
	if (val >= 192) {
		// back to top of screen
		GX_SetVCountEqVal(0);	
	} else {
		GX_SetVCountEqVal(val+16);
	}
}

// used in high score and end-of-level to turn the lower screen into a dim map of tiles
void TweakSubscreenMapToBackground() {
	MapScr_EndMap();	// stop auto-drawing the map
	OS_WaitVBlankIntr();
 
	// this comes up after the mapscr, so tweak as needed
	// make 2 exactly like 1, but slightly offset - if we need this again make a function.
	GXS_SetVisiblePlane(GX_PLANEMASK_BG0 | GX_PLANEMASK_BG1 | GX_PLANEMASK_BG2);	// turn off sprites and layer 3
	GXS_SetVisibleWnd(GX_WNDMASK_NONE);
	G2S_SetBG2ControlText(GX_BG_SCRSIZE_TEXT_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE_0x3800, GX_BG_CHARBASE_0x00000);
	MapScr_EraseTextOnly();
	G2S_SetBG1Offset(0, 0);
	G2S_SetBG2Offset(-1,-1);
	G2S_SetBlendBrightness(GX_BLEND_PLANEMASK_BG2, -9);	// can only have one alpha effect at a time

	// darken the background palette, since we can't use hardware tricks on two planes at once (well, we sort of can, but never mind...)
	// we can assume 64 colors? (looks like it)
	for (int idx=0; idx<128; idx+=2) {
		u16 tmp = *(u16*)(HW_DB_BG_PLTT+idx);
		// this hacky exception is used when we come from the high score entry, where we already darkened the grass once
		// note it will break if the grass palette changes :)
		if ((idx == 0)&&(tmp == 0x04E3)) break;
		tmp = (tmp>>1)&0x3DEF;	// shift one bit (out of 5) and mask off the new zeros
		*(u16*)(HW_DB_BG_PLTT+idx) = tmp;
	}
}

void ShowHighScores(int nTimeout) {
	int fContinue, idx;
	char buf[64];
	
	sound_stop();
	Clear2D();
	TweakSubscreenMapToBackground();
	
	// Load the background graphics 
	load_bmp_block_mask("gfx/Menu/World.bmp", NULL, txr_misc, pal_misc, -1);

	// now we can start music
	sound_start(SONG_HISCORE, 1);	
	fContinue=nTimeout;

	// subscreen - challenge tables
	MapScr_DrawTextXY816(7, 4, "Best Challengers");
	
	sprintf(buf, "Cookie Save     %c%c%c %.3d sheep ", gGame.ChallengeName[0][0], gGame.ChallengeName[0][1], gGame.ChallengeName[0][2], gGame.ChallengeScore[0]);
	MapScr_DrawTextXY816(1, 10, buf);

	sprintf(buf, "Water Drop      %c%c%c %.3d caught", gGame.ChallengeName[4][0], gGame.ChallengeName[4][1], gGame.ChallengeName[4][2], gGame.ChallengeScore[4]);
	MapScr_DrawTextXY816(1, 14, buf);

	MapScr_FlushTextOnly();	

	// top screen, high scores
	// this writes out the high score table
	CenterWriteFont2D(0, 15, "Top Herders");
	WriteFont2D(-1, 15, " ");		// just to increment the cursor

	nCol2=nCol=0;

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
		CenterWriteFont2D(-1, 15, buf);
	}

	// now set up color change interrupts
	// Text is all color 15 in palette 0, so we can rotate that
	GX_SetVCountEqVal(0);		// top of screen first
	OS_SetIrqFunction(OS_IE_V_COUNT, HSIntr);
	OS_EnableIrqMask(OS_IE_V_BLANK | OS_IE_V_COUNT);
	GX_VCountEqIntr(true);
	
	// make sure the 3D darken is disabled
	Set3DDarken(0);

	while (fContinue > 0) {
		fContinue-=2;	// for 30fps instead of 60
		// fading out
		if (nFrames%6 == 0) {
			nCol++;
			if (nCol>3) nCol=0;
		}
		
		if (isStartPressed()) {
			if (fContinue>255) {
				fContinue=255;
			}
		}
		
		// this is just for the background image
		pvr_scene_begin();
		SortFullPictureX(GX_TEXFMT_PLTT256, txr_misc, pal_misc, 16);
		pvr_scene_finish();

		if (fContinue < 256) {
			if ((fContinue>>3) < gGame.MVol) {
				set_sound_volume(fContinue>>3, -1);
			}
			fContinue-=7;
			GX_SetMasterBrightness(-((256-fContinue)>>4));
			GXS_SetMasterBrightness(-((256-fContinue)>>4));
		}
	}
	
	sound_stop();
	Clear2D();
	
	// stop interrupt and reset palette
	GX_VCountEqIntr(false);
	OS_DisableIrqMask(OS_IE_V_COUNT);
	OS_WaitVBlankIntr();			// wait a frame to be sure it stopped
	*(u16*)(HW_PLTT+30) = 0x7fff;	// set palette to white
	*(u16*)(HW_DB_BG_PLTT+258) = 0x0000;	// set palette to black
	*(u16*)(HW_DB_BG_PLTT+264) = 0x0000;	// set color 132
	
	// reset sound volume
	set_sound_volume(gGame.MVol, -1);

	// and call the generic clear-screen/reset code
	ShowBlack();

	// don't worry about fixing the subscreen, it is about to change modes anyway
}

// update the onscreen keyboard as needed
// Letter must be in the Range 'A' - ('Z'+2)
void DrawKeyboardChar(int nLetter, int bHighlight) {
	char str[2];
	switch (nLetter) {
		case 'Z'+2:		// end character
			if (bHighlight) {
				MenuScr_DrawTextXY816(27,17, "END", 0);
			} else {
				MenuScr_DrawTextXY816(27,17, "END", 1);
			}
			break;
			
		case 'Z'+1:		// rub character
			if (bHighlight) {
				MenuScr_DrawTextXY816(23,17, "RUB", 0);
			} else {
				MenuScr_DrawTextXY816(23,17, "RUB", 1);
			}
			break;
		default:		// everything else
			str[1]='\0';
			
			if (bHighlight) {
				str[0] = nLetter;
			} else {
				str[0] = tolower(nLetter);
			}
			MenuScr_DrawTextXY1616(3+((nLetter-'A')%7)*4,2+((nLetter-'A')/7)*5, str, 0);
			break;
	}
}

// nWinner is the herder in question
// nGame is the bonus game number or -1 for main game
// nCount is the count awarded in the bonus game
// This function checks main game scores but assumes you already checked bonus games
void AddHighScore(int nWinner, int nGame, int nCount) {
	int fContinue, idx, nPos;
	char buf[64];
	int nLetter;
	char x,y,z;
	char *pInit;
	
	nPos=-1;
	if (nGame == -1) {
		for (idx=0; idx<10; idx++) {
			debug("Winner: %5d  HighX: %5d\n", herder[nWinner].score, gGame.HighScore[idx]);
			if (gGame.HighScore[idx] <= herder[nWinner].score) {
				nPos=idx;
				break;
			}
		}
		if (nPos == -1) {
			// not a high score after all
			return;
		}
		// move the other scores down
		for (idx=9; idx>=nPos+1; idx--) {
			gGame.HighScore[idx]=gGame.HighScore[idx-1];
			memcpy(gGame.HighName[idx], gGame.HighName[idx-1], 4);
		}
		pInit = &gGame.HighName[nPos][0];
	} else {
		// save off the old best name
		x=gGame.ChallengeName[nGame][0];
		y=gGame.ChallengeName[nGame][1];
		z=gGame.ChallengeName[nGame][2];
		if (x=='\0') x=' ';
		if (y=='\0') y=' ';
		if (z=='\0') z=' ';
		pInit = &gGame.ChallengeName[nGame][0];
	}
	
	sound_stop();
	Clear2D();
	MenuScr_Init();	// we need the menu fonts so that screws up the subscreen backdrop...
	// darken the background palette, since we aren't using the window overtop
	// we can assume 64 colors?
	for (int idx=0; idx<128; idx+=2) {
		u16 tmp = *(u16*)(HW_DB_BG_PLTT+idx);
		tmp = (tmp>>1)&0x3DEF;	// shift one bit (out of 5) and mask off the new zeros
		*(u16*)(HW_DB_BG_PLTT+idx) = tmp;
	}
	
	
	// Load the background graphics 
	load_bmp_block_mask("gfx/Menu/World.bmp", NULL, txr_misc, pal_misc, -1);

	// now we can start music
	sound_start(SONG_HELL, 1);	

	// top screen, high scores (one of which we are going to replace)
	// this writes out the high score table
	CenterWriteFont2D(0, 15, "You got a high score!!");
	WriteFont2D(-1, 15, " ");		// just to increment the cursor

	// init colors
	nCol2=nCol=0;
	
	if (nGame == -1) {
		// load the score
		gGame.HighScore[nPos]=herder[nWinner].score;
		for (idx=0; idx<4; idx++) {
			gGame.HighName[nPos][idx]='\0';
		}
		
		// build the screen
		for (idx=0; idx<10; idx++) {
			char a,b,c;
			a=gGame.HighName[idx][0];
			b=gGame.HighName[idx][1];
			c=gGame.HighName[idx][2];
			// no initials for the one we are replacing yet!
			if (a=='\0') a=' ';
			if (b=='\0') b=' ';
			if (c=='\0') c=' ';
			// 0 is the top score
			sprintf(buf, "%2d  %c%c%c  %6d ", idx+1, a, b, c, gGame.HighScore[idx]);
			CenterWriteFont2D(-1, 15, buf);
		}
	} else {
		if (nGame == 0) {
			CenterWriteFont2D(-1, 15, "Cookie Save");
			WriteFont2D(-1, 15, " ");		// just to increment the cursor
			sprintf(buf, "Old: %3d sheep    by %c%c%c", gGame.ChallengeScore[0], x, y, z);
			CenterWriteFont2D(-1, 15, buf);
			WriteFont2D(-1, 15, " ");		// just to increment the cursor
			sprintf(buf, "You: %3d sheep    --    ", nCount);
			CenterWriteFont2D(-1, 15, buf);
		} else {
			// must be game 4, we canned the rest
			CenterWriteFont2D(-1, 15, "Water Drop");
			WriteFont2D(-1, 15, " ");		// just to increment the cursor
			sprintf(buf, "Old: %3d balloons by %c%c%c", gGame.ChallengeScore[4], x, y, z);
			CenterWriteFont2D(-1, 15, buf);
			WriteFont2D(-1, 15, " ");		// just to increment the cursor
			sprintf(buf, "You: %3d balloons --    ", nCount);
			CenterWriteFont2D(-1, 15, buf);
		}
	}
	
	// set up a keyboard on the subscreen
	MenuScr_DrawTextXY1616(3, 2, "a b c d e f g", 0);
	MenuScr_DrawTextXY1616(3, 7, "h i j k l m n", 0);
	MenuScr_DrawTextXY1616(3,12, "o p q r s t u", 0);
	MenuScr_DrawTextXY1616(3,17, "v w x y z", 0);
	MenuScr_DrawTextXY816(23,17, "RUB END", 1);
	MenuScr_DrawTextXY816( 5,21, "Enter three initials!", 0);
	MenuScr_FlushTextOnly();

	// now set up color change interrupts
	// Text is all color 15 in palette 0, so we can rotate that
	GX_SetVCountEqVal(0);		// top of screen first
	OS_SetIrqFunction(OS_IE_V_COUNT, HSIntr);
	OS_EnableIrqMask(OS_IE_V_BLANK | OS_IE_V_COUNT);
	GX_VCountEqIntr(true);
	
	// make sure the 3D darken is disabled
	Set3DDarken(0);
	
	// here goes!
	nLetter = 'A'-1;
	idx=0;

	fContinue = 1;
	while (fContinue) {
		int btns;
		
		if (nFrames%6 == 0) {
			nCol++;
			if (nCol>3) nCol=0;
		}
		
		// this is just for the background image
		pvr_scene_begin();
		SortFullPictureX(GX_TEXFMT_PLTT256, txr_misc, pal_misc, 16);
		pvr_scene_finish();
		
		// handle controls
		btns = GetController(gHumanPlayer);
		if (btns) {
			DisableControlsTillReleased();
			if (nLetter < 'A') {
				// just activate the manual editor
				nLetter='A';
			} else {
				// erase the old highlight
				DrawKeyboardChar(nLetter, 0);
				if ((btns&CONT_DPAD_UP)&&(nLetter>='H')) nLetter-=7;
				if ((btns&CONT_DPAD_LEFT)&&(nLetter>'A')) nLetter--;
				if ((btns&CONT_DPAD_RIGHT)&&(nLetter<'Z'+2)) nLetter++;
				if ((btns&CONT_DPAD_DOWN)&&(nLetter<'V')) nLetter+=7;

				if (btns&CONT_A) {
processinput:				
					sound_effect_system(SND_SHEEP1 + FREED, PLAYERVOL);
					switch (nLetter) {
						case 'Z'+2:		// end
							fContinue = 0;
							break;
							
						case 'Z'+1:		// rub
							if (idx > 0) {
								idx--;
								if (nGame == -1) {
									// erase on the top screen
									WriteFont2D(4+nPos*2, 11+idx, " ");
								} else {
									WriteFont2D(12, 24+idx, " ");
								}
									
							}
							break;
							
						default:		// letters
							if (idx < 3) {
								pInit[idx] = nLetter;
								buf[0] = nLetter;
								buf[1] = '\0';
								if (nGame == -1) {
									WriteFont2D(4+nPos*2, 11+idx, buf);
								} else {
 									WriteFont2D(12, 24+idx, buf);
								}									
									
								idx++;
							}
							if (idx >= 3) {
								nLetter = 'Z'+2;
							}
							break;
					}
				}
			}
			// draw the selected letter
			DrawKeyboardChar(nLetter, 1);
			MenuScr_FlushTextOnly();
		} else {
			TPData touchdat;

			// how about touch screen?
			GetTouchData(&touchdat);
			if ((touchdat.touch == TP_TOUCH_ON) && (TP_VALIDITY_VALID == touchdat.validity)) {
				DisableControlsTillReleased();
				// each character is 2 chars wide, with 2 chars empty inbetween
				// height is trickier, they are 2 chars high with 3 chars empty inbetween
				touchdat.x>>=3;	// convert to characters
				touchdat.y>>=3;
				touchdat.x-=3;
				touchdat.x>>=1;	// 0-13 or so, even numbers 0-12 valid
				// cheat and factor away the extra vertical spacing
				if (touchdat.y >= 17) {
					touchdat.y -= 3;
				} else if (touchdat.y >= 12) {
					touchdat.y -= 2;
				} else if (touchdat.y >= 7) {
					touchdat.y --;
				}
				touchdat.y-=2;
				touchdat.y>>=1;	// even numbers 0-6 valid
				if ((touchdat.x&1)||(touchdat.y&1)) continue;
				// seems to be valid, so divide again
				touchdat.x>>=1;	// 0-6
				touchdat.y>>=1;	// 0-3
				DrawKeyboardChar(nLetter, 0);
				nLetter = (touchdat.y*7)+touchdat.x+'A';
				goto processinput;
			}
		}
	}
	
	if (nGame == -1) {
		gGame.HighName[nPos][3] = '\0';		// just to be safe
	} else {
		gGame.ChallengeName[nGame][3] = '\0';
		gGame.ChallengeScore[nGame] = nCount;
	}
	
	// handle autosave
	if (gGame.AutoSave) {
		doVMUSave(1, gGame.SaveSlot);
	}
	
	// on exit, we just show the high score table (unless after a challenge)
	sound_stop();
	if (nGame == -1) {
		ShowHighScores(20*60+255);		// show for up to 20 seconds plus fade
	} else {
		// stop interrupt and reset palette
		GX_VCountEqIntr(false);
		OS_DisableIrqMask(OS_IE_V_COUNT);
		OS_WaitVBlankIntr();			// wait a frame to be sure it stopped
		*(u16*)(HW_PLTT+30) = 0x7fff;	// set palette to white
		*(u16*)(HW_DB_BG_PLTT+258) = 0x0000;	// set palette to black
		*(u16*)(HW_DB_BG_PLTT+264) = 0x0000;	// set color 132
		ShowBlack();
	}
}

// handles the bonus menu and its submenus, except musicplayer
void doBonusMenu() {
	int nTmp=0;
	
	for (;;) {
		MenuScr_ResetSubScr();
		switch (doMenu(MENU_BONUS,NULL)) {
			default:
			case MENU_CANCEL:
				debug("Leaving bonus menu...\n");
				return;
				
			case MENU_MINIGAMES:
				debug("Entering minigames menu...\n");
				nTmp=MENU_MINIGAMES;
				while (nTmp != -1) {
					switch (doMenu(nTmp,NULL)) {
						case MENU_CANCEL:
							nTmp=-1;
							break;
							
						case MENU_MINI_CANDY:
							PlayMinigame(LEVEL_CANDY);
							ReloadMenu("Extras");
							HandleTopMenuView(SCROLL_TARG_NORTH);
							MenuScr_Init();
							nTmp = MENU_MINIGAMES;
							break;
							
						case MENU_MINI_WATER:
							PlayMinigame(LEVEL_WATER);
							ReloadMenu("Extras");
							HandleTopMenuView(SCROLL_TARG_NORTH);
							MenuScr_Init();
							nTmp = MENU_MINIGAMES;
							break;
					}
				}
				break;		
			
			case MENU_HIGHSCORES:
				debug("Running high scores...\n");
				sound_stop();
				Clear2D();
				ShowHighScores(0x7fffffff);	// impossibly high so it'll never timeout
				ReloadMenu("Extras");
				HandleTopMenuView(SCROLL_TARG_NORTH);
				MenuScr_Init();
				break;
			
			case MENU_MUSICPLAYER:
				debug("Running music player...\n");
				Clear2D();
				doMusicPlayer();
				ReloadMenu("Extras");
				break;
				
			case MENU_CREDITS:
				debug("Running credits...\n");
				Clear2D();
				credzMain(0) ;
				ReloadMenu("Extras");
				HandleTopMenuView(SCROLL_TARG_NORTH);
				MenuScr_Init();
				break;
		}
	}
}

// 32 chars wide
char *credzText[] = {
"Software Lead         Mike Brent",
"Art Lead           Brendan Wiese",
"Artwork     E. Elizabeth Cristie",
"                    Sophia Volpi",
"Software        Keith Henrickson",
"                      Roger Hook",
"Addl Artwork      Maurizio Terzo",
"                      Foxx Crump",
"Music               Doug Ritchie",                    
"Voices and Sfx      Roddy Toomim",
"                     Alex Aguila",
"                    Mary Ann Seo",
"                     Ange Kogutz",
"                      Pam Staten",
"                         Flossie",
"Thanks              Paul Andrews",
"                 Steve Napierski",
"                    Kirsten Ross",
"                   David Schultz",
"                      Dan Loosen",
"                       Gary Heil",
"                     Ben Shiplet",
"                     Jeff Minter",                      
"                    Dale Simpson",
};

char *credzSpace = 
"                                ";

// a really minimal credits listing
// we should have 12 lines per screen, right?
// if reload is true, then we reload the story mode menu
// Note that this function is allowed to use all 6 herder objects!
// but the last two reuse the space for txr_level and txr_misc
// we get tricky in here and interleave the texture load steps in
// hopes of reducing the visible impact - the API was not really designed
// for streaming!
// Herder 0 is defined as a hidden Trey so I can use his Disco ball special
// without a bunch of redundant code. He's never drawn, instead we use
// the other 5 herder objects (1-5). That's why we needed six structs.
// Trey is #0 so I don't need to extend the SpecialDat struct.
void credzMain(int reload) {
	u16 lastbtns = 0;	// we need a local cache because isStartPressed() updates the global
	int fContinue, idx, tmp;
	int scroll = -24;
	int herdidx = 0;	// 0-14
	char buf[32], buf2[32];
	int scrollnow = 1;	// so text draws on first frame
	int danceframe = 0;
	kos_img_t img;
	FSFile fp;
	int nDataSize = 0;
	int nOffset = 0;
	int loadStep;
		
	sound_stop();
	Clear2D();
	ShowBlack();
	TweakSubscreenMapToBackground();
	
	// set up some dancing herders
	// hmm.. can we load them without hiccuping the display too much?
	// recenter the screen
	SetOffset(0,0);
	
	// clear the working image structure
	memset(&img, 0, sizeof(img));
	
	// set up the sheep (all unset for now)
	for (idx=0; idx<MAX_SHEEP; idx++) {
		sheep[idx].type=0;
	}

	// this is the only place we have 6 herders.... just to make life hacky!
	for (idx = 0; idx<6; idx++) {
		memset(&herder[idx], 0, sizeof(HerdStruct));
		herder[idx].type=PLAY_NONE;
		herder[idx].spr.nDepth=DEPTH_256x256x4;
		herder[idx].spr.x=(idx-1)*64;
		herder[idx].spr.y=152;
		herder[idx].spr.tilenumber=DOWN_STAND_SPRITE;
		herder[idx].spr.z=(fx16)3296;
		herder[idx].color = rand()&0x0f;
		herder[idx].spr.txr_addr=txr_herder[idx];
		herder[idx].spr.pal_addr=pal_herder[idx];
		herder[idx].spr.alpha=31;
		herder[idx].spr.is3D=true;

		// select the image
		sprintf(buf, "gfx/Players/%s_img.bmp", szNames[herdidx]);	
		// select the palette
		sprintf(buf2, "gfx/Players/%s%X.bmp", szNames[herdidx], herder[idx].color);
		
		// number 0 is special, we just load him to get Trey's disco ball
		if (idx == 0) {
			strcpy(buf, "gfx/Players/trey_img.bmp");
			strcpy(buf2, "gfx/Players/treyA.bmp");
			// we need him centered so the Disco ball effect is happy
			herder[idx].spr.x=112;
			herder[idx].spr.y=132;	// a little higher than normal
		}

		debug("Player %d (%s) loading %s (%s) -> %s\n", idx, herder[idx].name, buf, buf2, (herder[idx].type&PLAY_MASK)==PLAY_COMPUTER?"Computer":"Human");
		load_bmp_block_mask(buf, buf2, txr_herder[idx], pal_herder[idx], idx);
		buf[strlen(buf)-7]='\0';
		strcat(buf,"map.txt");
		load_map(buf, herder[idx].map);
		
		++herdidx;
		if (herdidx > 14) herdidx=0;
	}
	
	// don't forget to load the sheep sprites
	load_bmp_block_mask("gfx/Players/shep_img.bmp", "gfx/Players/sheepWH.bmp", txr_sheep, pal_sheep, -1);
	load_map("gfx/Players/shep_map.txt", txrmap_sheep);
	
	// load Hades' sound file for Trey, I like the howl
	sound_loadplayer(0, nSoundGroups[HERD_WOLF]);
	
	// Load the background graphics 
	load_bmp_block_mask("gfx/Menu/World.bmp", NULL, txr_misc, pal_misc, -1);

	// now we can start music
	sound_start(SONG_CREDITS, 1);	
	fContinue=18000;	// ten minutes

	// now set up color change interrupts
	// Text is all color 15 in palette 0, so we can rotate that
	GX_SetVCountEqVal(0);		// top of screen first
	OS_SetIrqFunction(OS_IE_V_COUNT, HSIntr);
	OS_EnableIrqMask(OS_IE_V_BLANK | OS_IE_V_COUNT);
	GX_VCountEqIntr(true);
	
	// make sure the 3D darken is disabled
	Set3DDarken(0);
	
	// start the disco ball effect
	playeridx = 0;
	HandleTrey();

	// now we can loop!
	while (fContinue > 0) {
		fContinue-=2;	// for 30fps instead of 60
		// fading out
		if ((nFrames%6 == 0)||(scrollnow)) {
			scrollnow=0;
			nCol++;
			if (nCol>3) {
				nCol=0;
				++scroll;
				if (scroll > 23) scroll=0;

				// subscreen - second text block
				tmp = scroll+12; 
				if (tmp > 23) tmp-=24;
				for (idx = 0; idx<12; idx++) {
					// takes X,Y
					if (tmp < 0) {
						MapScr_DrawTextXY816(0,idx*2,credzSpace);
					} else {
						MapScr_DrawTextXY816(0,idx*2,credzText[tmp]);
					}
					tmp++;
					if (tmp > 23) tmp=0;
				}

				MapScr_FlushTextOnly();	

				// top screen - first text block
				tmp = scroll;
				for (idx=0; idx<12; idx++) {
					// takes row,col
					if (tmp < 0) {
						WriteFont2D(idx*2, 0, credzSpace);
					} else {
						WriteFont2D(idx*2, 0, credzText[tmp]);
					}
					tmp++;
					if (tmp > 23) tmp=0;
				}
				
			}
		}

		// background image and dancing herders
		pvr_scene_begin();
			SortFullPictureX(GX_TEXFMT_PLTT256, txr_misc, pal_misc, 16);
			// we only render 5 - the first is just for the disco ball to work
			for (idx=1; idx<6; idx++) {
				SortSprite(&herder[idx].spr, herder[idx].map, POLY_HERD);
			}
			// update the disco ball, along with any hacks needed to keep it working
			ProcessTrey(0, 0, 99);			// magic values
			specialDat[0][0].oldx = 99;		// don't expire
			
			// get any valid sheep up and manage physics
			for (idx=0; idx<MAX_SHEEP; idx++) {
				if (sheep[idx].type > 0) {
					// move them here
					if (sheep[idx].spr.y < herder[0].spr.y) {
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
						sheep[idx].spr.alpha=(248-sheep[idx].spr.x)>>2;
					}
					if (sheep[idx].spr.x < 81) {
						sheep[idx].spr.alpha=(sheep[idx].spr.x-10)>>2;
					}

					if (sheep[idx].spr.y > herder[0].spr.y) {
						// just poof
						sheep[idx].type = 0;
					}
					
					if (nFrames%4 == 0) {
						sheep[idx].animframe++;
						if (sheep[idx].animframe >= sheep[idx].maxframes) {
							sheep[idx].animframe=0;
						}
						sheep[idx].spr.tilenumber=SheepAnimationFrame[(sheep[idx].spr.xd<0)?0:2][sheep[idx].animframe];
					}
					
					SortSprite(&sheep[idx].spr, txrmap_sheep, POLY_SHEEP);
				}
			}
		pvr_scene_finish();

		// move the herders
		if (nFrames%3 == 0) {
			++danceframe;
			if (danceframe > 43) danceframe = 0;
		}

		for (idx=1; idx<6; idx++) {
			herder[idx].spr.tilenumber = DanceFrames[danceframe];
		
			herder[idx].spr.x++;
			// position chosen partly to beat wraparound, partly to ensure two don't process at once!
			// this inline load and processing seems to work fairly smoothly - a few flickers here and there
			// in debug mode, even in release mode (weird... due to the herder loop maybe?)
			if (herder[idx].spr.x >= 290) {
				loadStep = herder[idx].spr.x - 290;
				debug("Running load step %d\n", loadStep);
			
				// we need this for a clean VDP load
				OS_WaitVBlankIntr();

				// start an incremental load of the next fellow
				if (loadStep == 0) {
					// load palette
					herder[idx].color = rand()&0x0f;
					sprintf(buf2, "gfx/Players/%s%X.bmp", szNames[herdidx], herder[idx].color);
					debug("Loading %s\n", buf2);
					FS_InitFile(&fp); 
					// handle the palette first
					if (FALSE == FS_OpenFile(&fp, buf2)) {
						debug("Could not open palette %s\n", buf2);
					} else {
						bmp_to_pal(&fp, &img);
				  		FS_CloseFile(&fp);
					}
				} else if (loadStep == 1) {
					// load pixels
					img.byte_count = 0;
					sprintf(buf, "gfx/Players/%s_img.bmp", szNames[herdidx]);	
					debug("Loading %s\n", buf);
					if (FALSE == FS_OpenFile(&fp, buf)) {
						debug("Could not open %s\n", buf);
					} else {
						bmp_to_pixels(&fp, &img);
						FS_CloseFile(&fp);
					}
				} else if (loadStep == 2) {
					// load map
					sprintf(buf, "gfx/Players/%s_map.txt", szNames[herdidx]);					
					load_map(buf, herder[idx].map);
					
					// prepare memory ranges for DMA
					DC_FlushRange(img.pal, img.fmt<<1);
					DC_FlushRange(img.data, img.byte_count);
				} else if (loadStep == 3) {
					// load palette to VDP
					GX_BeginLoadTexPltt();
						GX_LoadTexPltt(img.pal, pal_herder[idx], img.fmt<<1);
					GX_EndLoadTexPltt();
					
					// prepare to load pixels, too
					nDataSize = img.byte_count;
					nOffset = 0; 
				} else if (nDataSize > 0) {
					// load one 'safe' frame
					GX_BeginLoadTex();
						GX_LoadTex((char*)img.data+nOffset, txr_herder[idx]+nOffset, min(nDataSize, SAFE_COPY_BYTES_PER_VBLANK/2));
					GX_EndLoadTex();
					nDataSize -= SAFE_COPY_BYTES_PER_VBLANK/2;
					debug("Loaded data to %d, %d remaining", nOffset, nDataSize);
					nOffset += SAFE_COPY_BYTES_PER_VBLANK/2;
				} else if (img.data) {
					debug("Free image data.");
					// free the memory when we're done
					kos_img_free(&img);
					++herdidx;
					if (herdidx > 14) herdidx=0;
				}

				// check for wraparound
				if (herder[idx].spr.x == 304) {		// eyeballed ;) still some pop on the left, but not bad
					herder[idx].spr.x = -15;
				}
			}
		}
		
		// check for exit
		if (isStartPressed()) {
			if (fContinue>255) {
				fContinue=255;
			}
		}
		
		// check for new sheep launch
		u16 btns;
		btns = GetController(gHumanPlayer);	// calls CheckControlStillDisabled
		if (btns != lastbtns) {
			if (btns & (CONT_L|CONT_R)) {
				// gonna launch a sheep, if we have one free
				for (idx = 0; idx<MAX_SHEEP; idx++) {
					if (sheep[idx].type == 0) break;
				}
				if (idx < MAX_SHEEP) {
					// going for it! Which side?
					if (btns&CONT_L) {
						// left side
						sheep[idx].spr.x=19;		// may be switched
						sheep[idx].spr.xd=(int)(rand()%8)+8;
					} else {
						// right side
						sheep[idx].spr.x=237;		// may be switched
						sheep[idx].spr.xd=-1*((int)(rand()%8)+8);
					}

					// and set the rest of the values
					sheep[idx].spr.nDepth=DEPTH_256x256x4;
					sheep[idx].spr.txr_addr=txr_sheep;
					sheep[idx].spr.pal_addr=pal_sheep;
					sheep[idx].spr.y=120;
					sheep[idx].spr.z=(fx16)3296;
					sheep[idx].spr.yd=-1*(int)(rand()%MAX_SHEEP)-1;
					if (sheep[idx].spr.yd < -16) sheep[idx].spr.yd=-16;
					sheep[idx].spr.tilenumber=SheepAnimationFrame[(sheep[idx].spr.xd<0)?0:2][0];
					sheep[idx].animframe=0;
					sheep[idx].spr.alpha=31;
					sheep[idx].spr.is3D=true;
					sheep[idx].type=1;
					sound_effect_system(SND_SHEEP1+(ZAPPEDA+ch_rand()%3), SHEEPVOL);
				}
			}
		}
		lastbtns=btns;
		
		if (fContinue < 256) {
			if ((fContinue>>3) < gGame.MVol) {
				set_sound_volume(fContinue>>3, -1);
			}
			fContinue-=7;
			GX_SetMasterBrightness(-((256-fContinue)>>4));
			GXS_SetMasterBrightness(-((256-fContinue)>>4));
		}
	}
	
	sound_stop();
	Clear2D();
	
	// stop interrupt and reset palette
	GX_VCountEqIntr(false);
	OS_DisableIrqMask(OS_IE_V_COUNT);
	OS_WaitVBlankIntr();			// wait a frame to be sure it stopped
	*(u16*)(HW_PLTT+30) = 0x7fff;	// set palette to white
	*(u16*)(HW_DB_BG_PLTT+258) = 0x0000;	// set palette to black
	*(u16*)(HW_DB_BG_PLTT+264) = 0x0000;	// set color 132
	
	// reset sound volume
	set_sound_volume(gGame.MVol, -1);

	// and call the generic clear-screen/reset code
	ShowBlack();

	if (reload) {
		ReloadMenu("Story Mode");
		HandleTopMenuView(SCROLL_TARG_WEST);
		MenuScr_Init();
	}
}

