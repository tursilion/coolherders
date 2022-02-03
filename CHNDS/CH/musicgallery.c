/****************************************/
/* Cool Herders                         */
/* http://harmlesslion.com              */
/* Copyright 2009 HarmlessLion LLC      */
/* muscgallery.c                        */
/****************************************/
// a simple music player

#include <stdio.h>
#include "kosemulation.h"
#include "sprite.h"
#include "cool.h"
#include "menu.h"
#include "control.h"
#include "font.h"
#include "sound.h"
#include "rand.h"
#include "menuscr_manager.h"

extern unsigned int lastbuttons[4];
extern pvr_ptr_t txr_misc,pal_misc;
extern int nFrames;

int nRandom=0;
int nLastMenu = 0;


int doMusicPlayer();
void NewMusic();
void StartRandom();

// start a random tune
void StartRandom() {
	int x;
	
	sound_stop();
	
	do {
		x=rand()%15;
	} while ((x==SONG_STORY) || (x == SONG_HEAVEN));
	
	nLastMenu = x;
	if (nLastMenu > 12) nLastMenu--;	// adjust for missing story
	if (nLastMenu > 8) nLastMenu--;		// adjust for missing heaven
	MenuScr_SetSelOption(nLastMenu);
	sound_start(x, 0);		// no repeat
}

// called by the menu once per frame
void CallBack() {
	if ((nRandom) && (!musicisplaying())) {
		StartRandom();
	}
}
	

// always returns 0
int doMusicPlayer() {
	int nMenu, nTmp;
	
	sound_stop();
	nRandom = 0;
	nLastMenu=0;
	
	// reuses this
	MenuScr_InitMenu(MENU_OPTIONS);
	MenuScr_ResetSubScr();
	MenuScr_DoMusicSelect(1);

	nMenu=MENU_MUSICPLAYER;
	nTmp=0;

	while (nMenu != -1) {
		nTmp = doMenu(nMenu|(nLastMenu<<16), CallBack);

		switch (nTmp) {
			default:
			case MENU_CANCEL:
				nMenu = -1;
				break;
				
			case 0:
			case 1:
			case 2:
			case 3:
			case 4:
			case 5:
			case 6:
			case 7:
			case 8:
			case 9:
			case 10:
			case 11:
			case 12:
				if (GetController(0) & CONT_R) {
					// secret random menu
					nRandom=1;
					StartRandom();
				} else {
					nLastMenu=nTmp;				// save selection
					
					if (nTmp >= 8) nTmp++;		// skip heaven
					if (nTmp >= 12) nTmp++;		// skip story
					
					nRandom=0;
					sound_stop();
					sound_start(nTmp, 0);	// no repeat on manual select
				}
				break;
		}
	}

	sound_stop();
	MenuScr_CloseMenu(1);
	while (MenuScr_DrawMenuItems()) {
		pvr_scene_finish_2d();
		if (NULL != CallBack) CallBack();
	}
	MenuScr_DoMusicSelect(0);		// turn it off again

	return 0;
}


