/****************************************/
/* Cool Herders                         */
/* A demented muse by Brendan Wiese     */
/* http://starbow.org                   */
/* Programmed by Mike Brent             */
/* http://harmlesslion.com              */
/* Copyright 2000-2005 Brendan Wiese &  */
/* Mike Brent.                          */
/* muscgallery.c                        */
/****************************************/
// a simple ogg player

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

struct musicFile {
	char szName[64];
	char szFilename[64];
};

struct musicArtist {
	char szName[32];
	char szNote[256];
	struct musicFile *pFile;
};

#define NUMUSICS 3

struct musicFile LiandrinFiles[] = {
	{	"Cool Herders Title",					"Bonus/Music/Liandrin/coolherders.ogg"	},
	{	"JakoShadows",							"Bonus/Music/Liandrin/end.ogg"			},
	{	"Swingin Sheep",						"Bonus/Music/Liandrin/swinginsheep.ogg" },
	{	"",										""										}
};

struct musicFile TenzuFiles[] = {
	{	"Candy Land",							"music/candyland.ogg"					},
	{	"Credits",								"music/credits.ogg"						},
	{	"Haunted House",						"music/haunted_house.ogg"				},
	{	"Heaven",								"music/heaven_final.ogg"				},
	{	"Hell",									"music/hell.ogg"						},
	{	"New Zealand",							"music/nz2.ogg"							},
	{	"Title",								"music/title.ogg"						},

	{	"",										""										}
};	

struct musicFile Tenzu2Files[] = {
	{	"Demo Tune",							"music/Tenzu_Demo.ogg"					},
	{	"Toy Factory",							"music/toyfactory.ogg"					},
	{	"Waterworks",							"music/waterworks.ogg"					},
	{	"Disco",								"music/disco.ogg"						},
	{	"Alternate Intro",						"music/introfinal.ogg"		},
	{	"Win Jingle",							"music/winjingle.ogg"					},

	{	"",										""										}
};

struct musicArtist sArtist[] = {
	{	
		"Sean Connell",
//		 1234567890123456789012345678901234
		"Sean provided the original catchy\n"
		"tunes for the DCTonic demo of Cool\n"
		"Herders. They are provided here\n"
		"for your enjoyment!",
		LiandrinFiles
	},

	{
		"Doug \"Tenzu\" Ritchie p1of2",
//		 1234567890123456789012345678901234
		"Tenzu composed an absolutely\n"
		"wonderful soundtrack for the game\n"
		"which exceeded our expectations!\n",
		TenzuFiles
	},

	{
		"Doug \"Tenzu\" Ritchie p2of2",
//		 1234567890123456789012345678901234
		"Tenzu composed an absolutely\n"
		"wonderful soundtrack for the game\n"
		"which exceeded our expectations!\n",
		Tenzu2Files
	},

	{	"", "", NULL }
};

int titletop=480, titledirection=0;
int fStopThread=0, fNewSong=0;
int nSongArtist, nSongNum;
int nPlayArtist, nPlayNum;
int nIdle;
kthread_t * thd_hnd;

// This thread's purpose is to handle random play, starting new music
// and to handle scrolling the music title up/down on change
// exits when fStopThread is set to true
void MusicThread(void *pDat) {
	static int nLocalArtist=-1, nLocalNum=-1;

	while (!fStopThread) {
		// if not random mode, then the other threads set these values
		if (!musicisplaying()) {
			// need to choose a new random song
newsong:
			nPlayArtist=ch_rand()%NUMUSICS;
			nPlayNum=0;
			while (sArtist[nPlayArtist].pFile[nPlayNum].szName[0]) {
				nPlayNum++;
			}
			nPlayNum=ch_rand()%nPlayNum;
			if ((nPlayArtist==nLocalArtist) && (nPlayNum == nLocalNum)) goto newsong;
			fNewSong=1;
			// if controller is 10 seconds idle, switch menu to current song
			if (nIdle > 600) {
				nSongArtist=nPlayArtist;
				nSongNum=nPlayNum;
			}
		}

		if (fNewSong) {
			// new song - start it up
			sound_start(sArtist[nPlayArtist].pFile[nPlayNum].szFilename, 0);
			nLocalArtist=nPlayArtist;
			nLocalNum=nPlayNum;

			titledirection=-1;		
			
			fNewSong=0;
		}

		if (titledirection) {
			titletop+=titledirection;
			if (titletop<=400) {
				titledirection=1;
			}
			if (titletop>=480) {
				titledirection=0;
			}
		}

		if ((fStopThread)||(gReturnToMenu)) {
			break;
		}

		thd_sleep(15);
	}

	sound_stop();
}

// always returns 0
// We can't do gallery and music player together because we can't
// stream music while loading pictures. Oh well. 
int doMusicPlayer() {
	int idx;
	cont_state_t * st;
	int artidx;
	uint32 nbuts;
	int bgX=0;

	// combined music player/text viewer/image viewer
	// Keys:
	//		ltrig or rtrig - new random song
	//		dpad - move cursor
	//		A - play music
	//      Y - view current song
	//		Start - exit
	fStopThread=0;
	fNewSong=0;
	nSongArtist=1;
	nSongNum=6;
	nPlayArtist=nSongArtist;
	nPlayNum=nSongNum;
	nIdle=0;
	thd_hnd = thd_create(MusicThread, NULL);

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
	
	// Load the world backdrop
	load_png_block("gfx/Menu/World1.png", disc_txr, 0);
	load_png_block("gfx/Menu/World2.png", txr_misc[0], 0);

	while (!fStopThread) {
		// now to get it onscreen
		BeginScene();
		pvr_list_begin(PVR_LIST_OP_POLY);

		// Opaque polygons
		// Backdrop
		gGfxDarken=128;
		SortFullPictureX(disc_txr, txr_misc[0], 1000.0f, bgX);
		gGfxDarken=0;
		bgX++;
		if (bgX>640) bgX=-639;

		pvr_list_finish();

		pvr_list_begin(PVR_LIST_TR_POLY);
		// Transparent polygons

		switch ((nFrames/90)%7) {
		case 0:	CenterDrawFontZ(1006.0f, 1, 40, DEFAULT_COLOR, "Select Music"); break;
		case 1: CenterDrawFontZ(1006.0f, 1, 40, DEFAULT_COLOR, "L&R to change artist"); break;
		case 2: CenterDrawFontZ(1006.0f, 1, 40, DEFAULT_COLOR, "U&D to change music"); break;
		case 3: CenterDrawFontZ(1006.0f, 1, 40, DEFAULT_COLOR, "A to select"); break;
		case 4: CenterDrawFontZ(1006.0f, 1, 40, DEFAULT_COLOR, "Trigger for random"); break;
		case 5: CenterDrawFontZ(1006.0f, 1, 40, DEFAULT_COLOR, "Y to view current"); break;
		case 6: CenterDrawFontZ(1006.0f, 1, 40, DEFAULT_COLOR, "Start to exit"); break;
		}

		CenterDrawFontZ(1006.0f, 1, -1, DEFAULT_COLOR, "");
		artidx=0;
		CenterDrawFontZ(1006.0f, 0, -1, DEFAULT_COLOR, sArtist[nSongArtist].szName);
		CenterDrawFontZ(1006.0f, 0, -1, DEFAULT_COLOR, "");
		while (sArtist[nSongArtist].pFile[artidx].szName[0]) {
			CenterDrawFontBackgroundZ(1006.0f, 0, -1, DEFAULT_COLOR, (nSongNum==artidx)?INT_PACK_COLOR(128, 224, 0, 0):0, sArtist[nSongArtist].pFile[artidx].szName);
			artidx++;
		}
		CenterDrawFontZ(1006.0f, 1, -1, DEFAULT_COLOR, "");
		CenterDrawFontBreaksZ(1006.0f, 0, -1, INT_PACK_COLOR(224,255,255,255), sArtist[nSongArtist].szNote);

		// Layer 2 - Current music tune 1008.0
		SortRect(1008.0f, 0, titletop, 639, titletop+100, INT_PACK_COLOR(128, 0, 0, 224), INT_PACK_COLOR(128, 0, 0, 224));
		CenterDrawFontZ(1008.2f, 0, titletop+4, DEFAULT_COLOR, sArtist[nPlayArtist].szName);
		CenterDrawFontZ(1008.2f, 0, -1, DEFAULT_COLOR, sArtist[nPlayArtist].pFile[nPlayNum].szName);

		// Done drawing
		pvr_list_finish();
		pvr_scene_end;

		if (gReturnToMenu) {
			sound_stop();
			break;
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

		nbuts=0;
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

			nbuts|=st->buttons;

			if (st->buttons & lastbuttons[idx]) {
				continue;
			}
			lastbuttons[idx]=st->buttons;

			// exit gallery
			if (st->buttons & CONT_START) {
				fStopThread=1;
				break;
			}

			if (st->buttons & CONT_Z) {
				// next tune (if random), otherwise stops tune
				// force the menu to jump by faking idle ;)
				nbuts=0;
				nIdle=601;
				sound_stop();
				continue;
			}

			if (st->buttons & CONT_Y) {
				// redisplay current song name
				titledirection=-1;
				nSongArtist=nPlayArtist;
				nSongNum=nPlayNum;
				continue;
			}

			if (st->buttons & CONT_DPAD_LEFT) {
				nSongArtist--;
				if (nSongArtist < 0) nSongArtist=NUMUSICS-1;
				nSongNum=0;
				continue;
			}
			if (st->buttons & CONT_DPAD_RIGHT) {
				nSongArtist++;
				if (nSongArtist >= NUMUSICS) nSongArtist=0;
				nSongNum=0;
				continue;
			}
			if (st->buttons & CONT_DPAD_DOWN) {
				nSongNum++;
				if ('\0' == sArtist[nSongArtist].pFile[nSongNum].szName[0]) {
					nSongNum=0;
				}
				continue;
			}
			if (st->buttons & CONT_DPAD_UP) {
				nSongNum--;
				if (nSongNum < 0) {
					nSongNum=0;
					while (sArtist[nSongArtist].pFile[nSongNum].szName[0]) {
						nSongNum++;
					}
					nSongNum--;
				}
				continue;
			}
			if (st->buttons & CONT_A) {
				// start this song
				nPlayArtist=nSongArtist;
				nPlayNum=nSongNum;
				fNewSong=1;
			}
		}

		if (0 == nbuts) {
			nIdle++;
		} else {
			nIdle=0;
		}
	}

	thd_wait(thd_hnd);
	sound_stop();
	return 0;
}


