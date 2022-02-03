/****************************************/
/* Cool Herders                         */
/* A demented muse by Brendan Wiese     */
/* http://starbow.org                   */
/* Programmed by Mike Brent             */
/* http://harmlesslion.com              */
/* Copyright 2000-2005 Brendan Wiese &  */
/* Mike Brent.                          */
/* vmu_logo.c                           */
/****************************************/
/* Adapted from code by Dan Potter, FoF */
/* and the GOAT libmenu                 */
/****************************************/

#ifndef WIN32
#include <kos.h>

#include "tursivmu.h"
#include "sprite.h"
#include "font.h"
#include "control.h"
#include "menu.h"
#include "cool.h"

// icon data
#include "sheepicon.h"
#include "herdicon.h"

#define VMU_MESSAGE_Y 190

// VMU tracking
maple_device_t *gVMUUsed;
// sort reset support
extern int gReturnToMenu;

/* in cool.c - don't want to include all of cool.h here */
void debug(char *str, ...);
extern unsigned int myJiffies;
extern int nFrames;
extern int gDontCheckLid;
extern pvr_ptr_t txr_sheep;
extern void SortSheepAndTitle();
/* sound.h */
void set_sound_volume(int nMusic, int nSound);

// local thread helpers
static semaphore_t * quit_sem;
static kthread_t * thd_hnd;
static struct gamestruct LastSavedGame={ 0 };

// VMU data stuff
#define VMUFILENAME "COOLHRDR"

// icons appear to be 32x32x4bits, user provided palette

vmu_pkg_t pkgsheep = {
	"Cool Herders",				// desc_short[20]	this one is on bottom in the VMU editor
	"All options unlocked!",	// desc_long[36]	this one is on top in the VMU editor
	VMUFILENAME,				// app_id[20]
	1,							// icon_cnt
	0,							// icon_anim_speed
	VMUPKG_EC_NONE,				// eyecatch_type
	sizeof(struct gamestruct),	// data_len
	// icon palette (16 x ARGB4444)
	{ 0xfdac,0x0f0f,0xfece,0xfa9a,0xffcf,0xffdf,0xfede,0xfdcd,0xfbab,0xff00,0xffff,0xfbbb,0xfaaa,0xfa9a,0xf333,0xf000 },
	Sheepicon,					// icon_data
	NULL,						// eyecatch_data (wtf is an eyecatch??)
	NULL						// data (filled in later)
};

vmu_pkg_t pkgherd = {
	"Cool Herders",				// desc_short[20]
	"Electric sheep action!",	// desc_long[36] 
	VMUFILENAME,				// app_id[20]
	1,							// icon_cnt
	0,							// icon_anim_speed
	VMUPKG_EC_NONE,				// eyecatch_type
	sizeof(struct gamestruct),	// data_len
	// icon palette (16 x ARGB4444)
	{ 0xf000,0xffff,0xf933,0xf622,0xf444,0x0f0f,0xff3f,0xff4f,0xf537,0xfaaa,0xf555,0xffef,0xffcc,0xffdc,0xf311,0xffff },
	Herdicon,					// icon_data
	NULL,						// eyecatch_data (wtf is an eyecatch??)
	NULL						// data (filled in later)
};

// This must be used instead of MAPLE_FOREACH because for some reason
// VMU devices never get __dev->status_valid set (told ya so! ;) )
#define VMU_FOREACH_BEGIN(TYPE) \
do { \
	maple_device_t  * __dev; \
	int     __i = 0; \
	while ( (__dev = maple_enum_type(__i, TYPE)) ) { \
		do {

#define VMU_FOREACH_END() \
		} while(0); \
		__i++; \
	} \
} while(0);

static void dumpsavegame() {
	int idx;

	debug("Magic: 0x%08x\n", gGame.nMagic);
	debug("Timer: %d\n", gGame.Options.Timer);
	debug("Rounds: %d\n", gGame.Options.Rounds);
	debug("Win: %s\n", gGame.Options.Win?"Sheep":"Score");
	debug("Powers: %s\n", gGame.Options.Powers?"On":"Off");
	debug("NightMode: 0x%04x\n", gGame.Options.NightMode);
	debug("Sound Vol: %d\n", gGame.SVol);
	debug("Music Vol: %d\n", gGame.MVol);
	debug("CPU Plays: %s\n", gGame.CPU?"On":"Off");
	debug("CPU Skill: %d\n", gGame.Options.Skill);
	debug("Ghosts: %s\n", gGame.Options.GhostMaze?gGame.Options.GhostMaze==1?"Never":"Always":"Normal");
	debug("Sheep Speed: %d\n", gGame.Options.SheepSpeed);
	debug("Autosave: %s\n", gGame.AutoSave?"On":"Off");
	debug("Total Games: %d\n", gGame.NumPlays);
	debug("Unlock Flags: 0x%08x\n", gGame.UnlockFlags);
	debug("High Scores:\n");
	for (idx=0; idx<10; idx++) {
		debug("%s - %d\n", gGame.HighName[idx], gGame.HighScore[idx]);
	}
	for (idx=0; idx<5; idx++) {
		debug("Challenge %d - %d\n", idx, (int)gGame.ChallengeScore[idx]);
	}
}

// Draw logos on VMUs 1-4, if present
// Also ticks off the 50hz jiffies for me
static void vmuThreadproc(void *p) {
	int nCount=0;
	int nTicks=0;
	int nOldVMU[4]={0,0,0,0};
	unsigned char mixbuf[192];

	debug("vmuThreadproc: started\n");

	do {
		maple_device_t * dev;
		int i,j,u;

		for (i=0; i<4; i++) {
			for (u=0; u<MAPLE_UNIT_COUNT; u++) {
				dev = maple_enum_dev(i, u);
				if (dev != NULL && dev->info.functions & MAPLE_FUNC_LCD) {
					break;
				}
			}
			if (u>=MAPLE_UNIT_COUNT) {
				nOldVMU[i]=0;
			} else {
				if (herder[i].nIcon != nOldVMU[i]) {
					if (herder[i].nIcon!=0) {
						memcpy(mixbuf, char_icon[herder[i].nIcon&0xf], 192);
						for (j=0; j<7; j++) { 
							mixbuf[j*6]=number_icon[i][j];
						}
						VMU_FOREACH_BEGIN(MAPLE_FUNC_LCD);
							if (__dev->port == i) {
								vmu_draw_lcd(__dev, mixbuf);
							}
						VMU_FOREACH_END();
					}
					nOldVMU[i]=herder[i].nIcon;
				}
			}
		}

		
		myJiffies++;

		nTicks++;
		if (nTicks > 90) {
			nTicks=0;

			// Note: MAPLE_FOREACH_BEGIN(MAPLE_FUNC_LCD, void *, st)
			// just hangs waiting for __dev->status_valid to become true, and it never seems to
			// <actually, this is fixed now, VMUs are always ready ;) >
			i = 0;
			while ( (dev = maple_enum_type(i++, MAPLE_FUNC_LCD)) ) {
				if (0==herder[dev->port].nIcon) {
					nCount++;
					if (nCount>=NUM_LOGOS) nCount=0;
					// Besides, we don't really care if it fails
					// However, we DO care if it doesn't have an LCD. The only test I have today
					// is that the standby and max power variables are very large on the gameshark -
					// which hangs the bus if you ever try to write to the VMU. So, as a workaround,
					// we won't write to the VMU if those values are off. I need to test with more
					// memory units. So far:
					//						Standby		MaxPower
					// Sega VMU				124			130
					// Gameshark			44718		44718

					if (dev->info.max_power < 150) {
						// hacky test for actual VMU and not gameshark
						// More test units required.
						memcpy(mixbuf, vmu_icon[nCount], 192);
						for (j=0; j<7; j++) {
							mixbuf[j*6]=number_icon[dev->port][j];
						}
						vmu_draw_lcd(dev, mixbuf);
						// don't spike the CPU when drawing LCDs, take your time
						thd_sleep(19);
						myJiffies++;
					} else {
						debug("Skipping odd LCD (gameshark?) %c:%c - Power: %d/%d\n", dev->port+'A', dev->unit+'0', dev->info.standby_power, dev->info.max_power);
						// don't use this as an LCD
						dev->info.functions &= ~MAPLE_FUNC_LCD;
					}

					//debug("VMU Device: %c:%c: %s - %s - %08x - %08x,%08x,%08x - %d - %d - %d - %d\n", 
					//	dev->port+'A', dev->unit+'0', dev->info.product_name, dev->info.product_license, dev->info.functions,
					//	dev->info.function_data[0],dev->info.function_data[1],dev->info.function_data[2], dev->info.area_code,
					//	dev->info.connector_direction, dev->info.standby_power, dev->info.max_power);
				}
			}
		}
	} while (sem_wait_timed(quit_sem, 19) == -1);

	debug("vmuThreadproc: finished\n");
}

void vmuInit() {
	quit_sem = sem_create(0);
	thd_hnd = thd_create(vmuThreadproc, NULL);
}

void vmuShutdown() {
	sem_signal(quit_sem);
	thd_wait(thd_hnd);
	sem_destroy(quit_sem);
}

// Pop up the standard disclaimer about removing VMUs
// Don't assume any graphics are loaded (except fonts)
void PutUpVMUWarning() {
	BeginScene();
	
	// Opaque polygons
	pvr_list_begin(PVR_LIST_OP_POLY);
	pvr_list_finish();
	
	// transparent polygons
	pvr_list_begin(PVR_LIST_TR_POLY);
	
	// sheepie!
	SortSheepAndTitle();

	SortRect(1000.0f, 0,0, 640,480, INT_PACK_COLOR(255,128,255,128), INT_PACK_COLOR(255,0,255,0));
	CenterDrawFontBackgroundBreaksZ(1002.0f, 0, VMU_MESSAGE_Y, DEFAULT_COLOR, INT_PACK_COLOR(128,32,64,32), "Accessing VMU\nDo not insert or remove\nVMUs or controllers");

	pvr_list_finish();
	// draw it
	pvr_scene_end;
}

void ShowVMUSuccess(char *sz, int isAutoSave) {
	int nCnt=1800/16;	// ms/frame time

	debug("VMU OK: %s\n", sz);

	if (!isAutoSave) {
		while (isStartPressed());
	}

	for (;;) {
		BeginScene();
		
		// Opaque polygons
		pvr_list_begin(PVR_LIST_OP_POLY);
		pvr_list_finish();
		
		// transparent polygons
		pvr_list_begin(PVR_LIST_TR_POLY);
		
		// sheepie!
		SortSheepAndTitle();

		SortRollingRect(1000.0f, INT_PACK_COLOR(255,128,255,128), INT_PACK_COLOR(255,0,255,0));
		CenterDrawFontBackgroundBreaksZ(1002.0f, 0, VMU_MESSAGE_Y, DEFAULT_COLOR, INT_PACK_COLOR(128,32,64,32), sz);
		if (!isAutoSave) {
			CenterDrawFontZ(1002.0f, 0, 400, DEFAULT_COLOR, "Press Start"); 
		}

		pvr_list_finish();
		// draw it
		pvr_scene_end;

		if (isAutoSave) {
			nCnt--;
			if (nCnt < 1) {
				break;
			}
		}

		if ((isStartPressed()) || (gReturnToMenu)) {
			break;
		}
	}
}

void ShowVMUFailure(char *sz) {
	debug("VMU failed: %s\n", sz);

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

		SortRollingRect(1000.0f, INT_PACK_COLOR(255,255,128,128), INT_PACK_COLOR(255,255,0,0));
		CenterDrawFontBackgroundBreaksZ(1002.0f, 0, VMU_MESSAGE_Y, DEFAULT_COLOR, INT_PACK_COLOR(128,64,32,32), sz);
		CenterDrawFontZ(1002.0f, 0, 400, DEFAULT_COLOR, "Press Start"); 

		pvr_list_finish();
		// draw it
		pvr_scene_end;

		if ((isStartPressed()) || (gReturnToMenu)) {
			break;
		}
	}
}

// ask the user about where to save
// 1 for yes, 0 for no
int AskVMUUse(maple_device_t *pDev, char *szReq) {
	char szMsg[64];
	int nRet=-2;

	if (szReq[0]=='*') {
		sprintf(szMsg, "Okay to %s file\non VMU %c:%c?", szReq+1, pDev->port+'A', pDev->unit+'0');
	} else {
		sprintf(szMsg, "Okay to %s file\non VMU %c:%c?", szReq, pDev->port+'A', pDev->unit+'0');
	}
	debug("Ask user: %s\n", szMsg);

	while (nRet < 0) {
		BeginScene();
		
		// Opaque polygons
		pvr_list_begin(PVR_LIST_OP_POLY);
		pvr_list_finish();
		
		// transparent polygons
		pvr_list_begin(PVR_LIST_TR_POLY);
		
		// sheepie!
		SortSheepAndTitle();

		SortRollingRect(1000.0f, INT_PACK_COLOR(255,128,128,255), INT_PACK_COLOR(255,0,0,255));
		CenterDrawFontBackgroundBreaksZ(1002.0f, 0, VMU_MESSAGE_Y, DEFAULT_COLOR, INT_PACK_COLOR(128,32,32,64), szMsg);
		if (szReq[0]=='*') {
			CenterDrawFontBreaksZ(1002.0f, 0, 400, DEFAULT_COLOR, "Press A for yes or B\nto skip this autosave");
		} else {
			CenterDrawFontBreaksZ(1002.0f, 0, 400, DEFAULT_COLOR, "Press A for yes or B\nto find another VMU");
		}

		pvr_list_finish();
		// draw it
		pvr_scene_end;

		MAPLE_FOREACH_BEGIN(MAPLE_FUNC_CONTROLLER, cont_state_t, st);
			if (NULL == st) {
				continue;
			}
			if (nRet == -2) {
				// make sure no buttons are being held
				if (0 == st->buttons) {
					nRet=-1;
				} 
			} else {
				if (st->buttons & CONT_A) {
					nRet=1;
					debug("User said YES\n");
					goto exitscan5;
				}
				if (st->buttons & CONT_B) {
					nRet=0;
					debug("User said NO\n");
					goto exitscan5;
				}
			}
		MAPLE_FOREACH_END();
exitscan5: ;
	}

	return nRet;
}

// pass the buffer read from the memory card
int DecodeSaveFile(uint8 *pBuf, int isAuto) {
	vmu_pkg_t pkg;
	if (vmu_pkg_parse(pBuf, &pkg) < 0) {
		ShowVMUFailure("Load failed\nGame data appears corrupt.");
	} else {
		// ah! we seem to have got it!
		if ((pkg.data_len==sizeof(struct gamestruct)) && (((struct gamestruct*)pkg.data)->nMagic == SAVEMAGIC)) {
			char buf[64];
			memcpy(&gGame, pkg.data, sizeof(struct gamestruct));
			sprintf(buf, "Load successful\nfrom VMU %c:%c", gVMUUsed->port+'A', gVMUUsed->unit+'0');
			// load volumes from config
			set_sound_volume(gGame.MVol*28, gGame.SVol*28);
			ShowVMUSuccess(buf, isAuto);
			// debug the save state
			dumpsavegame();
			// mark this the last save
			memcpy(&LastSavedGame, &gGame, sizeof(struct gamestruct));
			return 1;
		} else {
			ShowVMUFailure("Load failed\nGame data may be corrupt.");
		}
	}
	return 0;
}

// Load the save game from the VMU
void doVMULoad(int isAuto) {
	uint8 *pBuf=NULL;
	int bufsize=-1;

	PutUpVMUWarning();

	debug("Loading game from VMU\n");
	if (NULL != gVMUUsed) {
		// Verify VMU is still valid - set to NULL if not
		int nFound=0;

		VMU_FOREACH_BEGIN(MAPLE_FUNC_MEMCARD);
			if (__dev == gVMUUsed) {
				nFound=1;
				goto exitscan6;
			}
		VMU_FOREACH_END();
exitscan6:
		if (!nFound) {
			gVMUUsed=NULL;
		}
	}
	if (NULL != gVMUUsed) {
		// load the save file from the remembered VMU
		// Set to NULL if not found.
		bufsize=-1;

		if (vmufs_read(gVMUUsed, VMUFILENAME, (void**)&pBuf, &bufsize) < 0) {
			// error trying to load file - prolly doesn't exist.
			gVMUUsed=NULL;
		} else {
			DecodeSaveFile(pBuf, isAuto);
		}
	}
	if (NULL == gVMUUsed) {
		bufsize=-1;
		// search all VMUs for the save file. Set to
		// correct dev if found.
		VMU_FOREACH_BEGIN(MAPLE_FUNC_MEMCARD);
			if (vmufs_read(__dev, VMUFILENAME, (void**)&pBuf, &bufsize) < 0) {
				// error trying to load save file - prolly doesn't exist.
				continue;
			} else {
				// ah! we seem to have got it!
				gVMUUsed=__dev;
				if (DecodeSaveFile(pBuf, isAuto)) {
					goto skiploop;
				}

			}
		VMU_FOREACH_END();
skiploop: ;
	}
	if (!isAuto) {
		if (NULL == gVMUUsed) {
			// if this is STILL true, then we couldn't find it at all
			ShowVMUFailure("Failed to find game data!\nCheck you have the right VMU!");
		}
	}

	if (NULL != pBuf) {
		free(pBuf);
	}
}

// Save the save game to the VMU
// returns 1 if we paused to ask the user anything
int doVMUSave(int isAuto) {
	vmu_pkg_t pkg;
	int n,b,c, idx;
	int pkgsize, pkgblocks;
	maple_device_t *oldVMU;
	uint8 *pData;

	gGame.nMagic=SAVEMAGIC;

	// do we really need to save?
	if ((0 == memcmp(&gGame, &LastSavedGame, sizeof(struct gamestruct))) && (isAuto)) {
		debug("doVMUSave: Autosave - Game has not changed, skipping.\n");
		return 0;
	}

	// don't do it when we were told to return to the menu
	if (gReturnToMenu) {
		return 0;
	}

	// warn the user
	PutUpVMUWarning();
	debug("Saving game to VMU\n");

	// calculate how far we are by counting set bits in gGame.UnlockFlags
	n=0;
	b=1;
	c=0;
	for (idx=0; idx<32; idx++) {
		if (0 == (UNLOCK_UNUSED & b)) {
			c++;
			if (gGame.UnlockFlags & b) n++;
		}
		b<<=1;
	}
	// This is our percentage of complete
	n=n*100/c;

	if (n >= 100) {
		// get the happy sheep icon
		memcpy(&pkg, &pkgsheep, sizeof(vmu_pkg_t));
	} else {
		memcpy(&pkg, &pkgherd, sizeof(vmu_pkg_t));
		sprintf(pkg.desc_long, "Electric sheep action! %d%%", n);
	}
	pkg.data=(void*)&gGame;

	pData=NULL;
	if (vmu_pkg_build(&pkg, &pData, &pkgsize) < 0) {
		// failure - memory? We have nothing malloced though
		ShowVMUFailure("Failed to create save data");
		if (NULL != pData) {
			free(pData);
		}
		return 0;
	}

	pkgblocks = (pkgsize / 512) + ((pkgsize % 512) ? 1 : 0);

	// Pad the data up if needed.
	if (pkgsize % 512) {
		pData = (uint8*)realloc(pData, pkgblocks*512);
		memset(pData + pkgsize, 0, pkgblocks*512 - pkgsize);
		pkgsize = pkgblocks * 512;
	}

	debug("Saving game file %s, %d blocks\n", pkg.desc_long, pkgblocks);
	oldVMU=NULL;

	// If we have a save location, make sure it's still valid
	if (NULL != gVMUUsed) {
		debug("Checking old VMU location %c:%c\n", gVMUUsed->port+'A', gVMUUsed->unit+'0');
		if (vmufs_file_exists(gVMUUsed, VMUFILENAME) < 0) {
			// it's gone, clear it
			debug("Old save file gone, forgetting about it.\n");
			gVMUUsed=NULL;
		} else {
			// the '*' means that B will cancel, not search for another VMU. Tell the user.
			if (!AskVMUUse(gVMUUsed, "*overwrite")) {
				// If this is an autosave, just skip the save, but remember the card for next time
				if (isAuto) {
					return 1;
				}
				// otherwise, forget the card and we'll search below
				// We will take a note so we don't ask about it again ;)
				oldVMU=gVMUUsed;
				gVMUUsed=NULL;
			}
			PutUpVMUWarning();
		}

	}

	// If we don't have a save location, try to find one
	if (NULL == gVMUUsed) {
		VMU_FOREACH_BEGIN(MAPLE_FUNC_MEMCARD)
			// check existance
			if (__dev == oldVMU) continue;
			debug("Checking for file on new VMU location %c:%c\n", __dev->port+'A', __dev->unit+'0');
			if (vmufs_file_exists(__dev, VMUFILENAME) >= 0) {
				if (AskVMUUse(__dev, "overwrite")) {
					gVMUUsed=__dev;
					goto exitscan7;
				}
				PutUpVMUWarning();
			} else {
				// check free space
				debug("Checking for space on new VMU location %c:%c\n", __dev->port+'A', __dev->unit+'0');
				if (vmufs_free_blocks(__dev) >= pkgblocks) {
					if (AskVMUUse(__dev, "save")) {
						gVMUUsed=__dev;
						goto exitscan7;
					}
					PutUpVMUWarning();
				}
			}
		VMU_FOREACH_END()
exitscan7: ;
	}

	// If we still don't have a VMU, fail out
	if (NULL == gVMUUsed) {
		if (isAuto) {
			ShowVMUFailure("Game was not saved!");
		} else {
			ShowVMUFailure("No suitable VMU found!");
		}
		if (NULL != pData) {
			free(pData);
		}
		return 1;
	}

	// If we have a save location, try to use it
	PutUpVMUWarning();
	// debug stuff
	dumpsavegame();

	// Make sure that opening the lid won't corrupt the VMU 
	gDontCheckLid=1;
	
	if (vmufs_write(gVMUUsed, VMUFILENAME, pData, pkgsize, VMUFS_OVERWRITE) < 0) {
		gDontCheckLid=0;	// re-enable lid checks
		ShowVMUFailure("VMU write failed!\nVisit the Options Menu\nto try another VMU");
	} else {
		char buf[64];
		// mark this the last save
		memcpy(&LastSavedGame, &gGame, sizeof(struct gamestruct));
		gDontCheckLid=0;	// re-enable lid checks
		// tell the user
		sprintf(buf, "Save successful\non VMU %c:%c", gVMUUsed->port+'A', gVMUUsed->unit+'0');
		ShowVMUSuccess(buf, isAuto);
	}

	if (NULL != pData) {
		free(pData);
	}

	return 1;
}

#else 
// this is the Win32 version (if we ever implement it)
// yes, I've given up on the KOS emulation layer
#include <stdio.h>
#include "kosemulation.h"
#include "tursivmu.h"
#include "sprite.h"
#include "font.h"
#include "control.h"
#include "menu.h"

// icon data
#include "sheepicon.h"
#include "herdicon.h"

// VMU tracking
maple_device_t *gVMUUsed;

// nothing for now. Could do save games someday?
void vmuInit() {
}

void vmuShutdown() {
}

void doVMULoad(int isAuto) {
}

int doVMUSave(int isAuto) {
	return 0;
}
#endif
