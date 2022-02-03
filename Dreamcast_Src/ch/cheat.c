/****************************************/
/* Cool Herders                         */
/* A demented muse by Brendan Wiese     */
/* http://starbow.org                   */
/* Programmed by Mike Brent             */
/* http://harmlesslion.com              */
/* Copyright 2000-2005 Brendan Wiese &  */
/* Mike Brent.                          */
/* cheat.c                              */
/****************************************/

#include <stdio.h>
#include <malloc.h>

#ifdef WIN32
#include "kosemulation.h"
#else
#include <kos.h>
#include <arch/arch.h>
#include <png/png.h>
#endif

#include "sprite.h"
#include "levels.h"
#include "collide.h"
#include "score.h"
#include "cool.h"
#include "rand.h"
#include "control.h"
#include "sound.h"
#include "menu.h"		// for globals
#include "font.h"
#include "vmu_logo.h"

extern int gAYBText;
extern int nFrames;
extern int gAlwaysWinStoryMode;
extern int gAlwaysWinChallenges;
// sort reset support
extern int gReturnToMenu;
extern int gOnlyPlayStoryMode;
extern pvr_ptr_t txr_sheep;
extern void SortSheepAndTitle();

#define MESSAGE_Y 190

// Show a message
void ShowUnlockMessage(char *sz, unsigned int bits, int CheckAutoSave) {
	debug("Unlock: %s - 0x%08x\n", sz, bits);

	if ((bits == 0) || ((gGame.UnlockFlags&bits) != bits)) {
		gGame.UnlockFlags|=bits;

		while (isStartPressed());

		for (;;) {
			BeginScene();
			
			// Opaque polygons
			pvr_list_begin(PVR_LIST_OP_POLY);
			pvr_list_finish();
			
			// transparent polygons
			pvr_list_begin(PVR_LIST_TR_POLY);

			// sheepie!
			SortSheepAndTitle();

			// boring stuff!
			SortRollingRect(1000.0f, INT_PACK_COLOR(255,128,128,255), INT_PACK_COLOR(255,0,0,255));
			CenterDrawFontBackgroundBreaksZ(1002.0f, 0, MESSAGE_Y, DEFAULT_COLOR, INT_PACK_COLOR(128,32,32,64), sz);
			CenterDrawFontZ(1002.0f, 0, 400, DEFAULT_COLOR, "Press Start"); 

			pvr_list_finish();
			// draw it
			pvr_scene_end;

			if ((isStartPressed())||(gReturnToMenu)) {
				break;
			}
		}

		if (CheckAutoSave) {
			if (gGame.AutoSave) {
				doVMUSave(1);
			}
		}
	} else {
		debug("Already unlocked.\n");
	}
}

// nothing just yet
void Message() {
	ShowUnlockMessage("Extra greetz to\nMetafox, Bard, Kevv\n \nEVERYTHING ENABLED", 0xffffffff, 0);
}

void AllPlayers() {
	ShowUnlockMessage("All hidden players\nunlocked!", 0x0000003f, 0);
}

void EnableHardMode() {
	ShowUnlockMessage("Hard story mode\nunlocked!", 0x00080000, 0);
}

void AllWorlds() {
	if ((gGame.UnlockFlags&0xfc0)==0xfc0) {
		ShowUnlockMessage("All hard worlds\nunlocked!",   0x03f80000, 0);
	} else {
		ShowUnlockMessage("All normal worlds\nunlocked!", 0x00000fc0, 0);
	}
}

void DisablePowerups() {
	ShowUnlockMessage("Disable Powerups\nnow available in\noptions menu!", DISABLE_POWERUPS, 0);
}

void EnableAYB() {
	if (gAYBText) {
		ShowUnlockMessage("Alternate Story\ndisabled.", 0, 0);
		gAYBText=0;
	} else {
		ShowUnlockMessage("Alternate story\ntext now active -\nlevel 1 only", 0, 0);
		gAYBText=1;
	}
}

void EnableAlwaysWin() {
	if (gAlwaysWinStoryMode) {
		ShowUnlockMessage("Always Win\ndisabled.", 0, 0);
		gAlwaysWinStoryMode=0;
	} else {
		ShowUnlockMessage("Always Win\nnow active", 0, 0);
		gAlwaysWinStoryMode=1;
	}
}

void EnableAlwaysWinChallenges() {
	if (gAlwaysWinChallenges) {
		ShowUnlockMessage("Always Win Challenges\ndisabled.", 0, 0);
		gAlwaysWinChallenges=0;
	} else {
		ShowUnlockMessage("Always Win Challenges\nnow active", 0, 0);
		gAlwaysWinChallenges=1;
	}
}

void EnableStoryModeOnly() {
	if (gOnlyPlayStoryMode) {
		ShowUnlockMessage("Story text only\ndisabled.", 0, 0);
		gOnlyPlayStoryMode=0;
	} else {
		ShowUnlockMessage("Story text only\nnow active", 0, 0);
		gOnlyPlayStoryMode=1;
	}
}

void EnableMusicGallery() {
	ShowUnlockMessage("Music Gallery\nunlocked!", ENABLE_MUSIC_GAL, 0);
}

void EnableGhostSheep() {
	ShowUnlockMessage("Ghost Sheep control\nnow available on\noptions menu!", ENABLE_GHOST_CTL, 0);
}

void EnableImageGallery() {
	ShowUnlockMessage("Image gallery\nunlocked!", ENABLE_IMAGE_GAL, 0);
}

void EnableNightMode() {
	ShowUnlockMessage("Night mode\nnow availabe on\noptions menu!", ENABLE_NIGHT_MDE, 0);
}

void EnableEndCredits() {
	ShowUnlockMessage("End Credits\nnow available on\nmain menu!", ENABLE_END_CREDS, 0);
}

void EnableOmake() {
	ShowUnlockMessage("Omake\nnow available on\nmain menu!", ENABLE_OMAKE, 0);
}

// Handle cheating - return 1 if cheat was processed
int DoCheat(char *szString) {
	// should be UUDDLRLRBA, but we made the string too short. That's okay.
//	if (!strcmp(szString, "UDDLRLRBA")) {
//		Message();
//		return 1;
//	}

	if (!strcmp(szString, "AAAABLURB")) {
		// all players
		AllPlayers();
		return 1;
	}

	if (!strcmp(szString, "AAAAARRAY")) {
		// all players
		AllWorlds();
		return 1;
	}

	if (!strcmp(szString, "AAALUXURY")) {
		DisablePowerups();
		return 1;
	}

	if (!strcmp(szString, "AAAAYBAYB")) {
		EnableAYB();
		return 1;
	}

	if (!strcmp(szString, "AAABALLAD")) {
		EnableMusicGallery();
		return 1;
	}

	if (!strcmp(szString, "AAAARADAR")) {
		EnableGhostSheep();
		return 1;
	}

	if (!strcmp(szString, "AAAABULLY")) {
		EnableImageGallery();
		return 1;
	}
	
	if (!strcmp(szString, "AARURALLY")) {
		EnableNightMode();
		return 1;
	}

	if (!strcmp(szString, "AADURABLY")) {
		EnableEndCredits();
		return 1;
	}

	if (!strcmp(szString, "AAAAABABY")) {
		EnableAlwaysWin();
		return 1;
	}

	if (!strcmp(szString, "ABABYBABY")) {
		EnableAlwaysWinChallenges();
		return 1;
	}

	if (!strcmp(szString, "AAAAYABAX")) {
		EnableOmake();
		return 1;
	}

	if (!strcmp(szString, "AAAAYABBA")) {
		EnableStoryModeOnly();
		return 1;
	}

	return 0;
}
