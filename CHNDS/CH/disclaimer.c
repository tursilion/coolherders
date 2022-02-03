/****************************************/
/* Cool Herders                         */
/* http://harmlesslion.com              */
/* Copyright 2009 HarmlessLion LLC      */
/* disclaimer.c                         */
/****************************************/

/* Ported to KOS 1.1.x by Dan Potter */

/* 
   Boot screens and loader for Cool Herders
*/

#include <stdio.h>
 
#include "kosemulation.h"
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>

#include "sprite.h"
#include "cool.h"
#include "sheep.h"
#include "sound.h"
#include "font.h"
#include "vmu_logo.h"

void load_data();
void doLogoFade(char *topscr, char *btmscr);
int  PreAllocPVR();
void ShowBlack();
void LoadFixedGfx2DA();

extern pvr_ptr_t pal_tmp[4];
extern pvr_ptr_t txr_sprites;
extern pvr_ptr_t txr_level;
extern pvr_ptr_t txr_sheep;
extern pvr_ptr_t txr_herder[6];
extern pvr_ptr_t txr_misc, pal_misc;
extern pvr_ptr_t pal_sprites, pal_sheep, pal_level, pal_herder[6];
extern TXRMAP txrmap_sheep[];
extern TXRMAP txrmap_sprites[];
extern unsigned int myJiffies;
extern int nFrames;

// mapscr_manager.c
void MapScr_EndMap();
 
// Just gonna put this here -- too lazy to put it elsewhere :)
// fn = filename
// pf = separate palette filename, or NULL to use fn
// whereto = destination texture address
// palette = destination palette address
// herd = herder to copy to for mapscr, or -1 for none, or -2 for special 2D load hack
// WARNING: Do not specify herd of 0-3 unless there is enough palette space for 3 palettes!
void load_bmp_block_mask(char *fn, char *pf, pvr_ptr_t whereto, pvr_ptr_t palette, int herd) {
	kos_img_t	img;
	unsigned int nOldFrames;

	// do the full load, then
	nOldFrames=myJiffies;
	debug("Loading %s\n", fn);
 	if (bmp_to_img(fn, pf, &img) < 0) {
		debug("Can't load texture '%s' from file\n", fn);
		return;
	}
	debug("Loaded in %d ticks. Now processing...\n", myJiffies-nOldFrames);
	nOldFrames=myJiffies;

	pvr_txr_load_kimg(&img, whereto, palette, herd);

	debug("Done in %d ticks.\n", myJiffies-nOldFrames);
	
	kos_img_free(&img);
	
	return;
}

/* allocate PVR RAM for images - we do it now before allocating */
/* the large temporary buffers for the intro screens to reduce  */
/* heap fragmentation (ie: all free space should be at the end) */
/* Returns 0 normally. If it returns 1, memory was already allocated */
int PreAllocPVR() {
	int idx;

	if (txr_sprites != INVALID_PTR) {
		return 1;
	}
	
	debug("PreAllocPVR...\n");

	pvr_mem_reset();

	// we should have room for 16 256x256 pages at 4bit (levels are 8bit though)
	// these are the ones we think we will need
	txr_sheep=ALLOC_IMAGE(256, 256, SIZE_4BIT);
	pal_sheep=ALLOC_PAL(SIZE_4BIT);

	txr_sprites=ALLOC_IMAGE(256, 256, SIZE_4BIT);
	pal_sprites=ALLOC_PAL(SIZE_4BIT);
	
	// The levels are all 512x512x8 bit, one page each
	txr_level=ALLOC_IMAGE(512, 512, SIZE_8BIT);
	pal_level=ALLOC_PAL(SIZE_8BIT);
	
	txr_misc=ALLOC_IMAGE(256, 256, SIZE_8BIT);
	pal_misc=ALLOC_PAL(SIZE_8BIT);
	
	// there are /6/ herder blocks just to support the end creditz
	// but there's only enough vram to allocate 4 - the others overlap level data
	for (idx=0; idx<4; idx++) {
		txr_herder[idx]=ALLOC_IMAGE(256, 256, SIZE_4BIT);
		// Herders get 3 palettes - the loaded one, one where color 12 is forced to white,
		// and one where color 5 is forced to white. These alternates are used for Chrys and
		// Hades' specials. While this is a little wasteful, we had lots of palette RAM free.
		pal_herder[idx]=ALLOC_PAL(SIZE_4BITx3);
	}
	txr_herder[4] = txr_level;
	pal_herder[4] = pal_level;
	txr_herder[5] = txr_level+32768;			// 256x256x4bit, txr_level is 512x512x8bit, can hold 8 of these!
	pal_herder[5] = ALLOC_PAL(SIZE_4BITx3);		// we have enough palette space free
	
	// we have lots of palette space, so grab some
	// extra palette banks for menus
	pal_tmp[0]=ALLOC_PAL(SIZE_8BIT);
	pal_tmp[1]=ALLOC_PAL(SIZE_8BIT);
	pal_tmp[2]=ALLOC_PAL(SIZE_8BIT);
	pal_tmp[3]=ALLOC_PAL(SIZE_8BIT);
	
	// the above consumes ALL texture RAM though very little palette RAM
	
	pvr_mem_stats();

	return 0;
}

// this is called before every level
void load_data() {
	// Now we can start loading graphics
	load_bmp_block_mask("gfx/Players/shep_img.bmp", "gfx/Players/sheepWH.bmp", txr_sheep, pal_sheep, -1);
	load_map("gfx/Players/shep_map.txt", txrmap_sheep);

	load_bmp_block_mask("gfx/Misc/misc.bmp", NULL, txr_sprites, pal_sprites, -1);
	load_map("gfx/Misc/misc_map.txt", txrmap_sprites);
}

// this is called only on startup
void LoadFixedGfx2DA() {
	// these files are created in Nitro Character - they
	// are a 16 color, 16 palette dataset. PalTable is a 1024
	// entry table that contains the palette number for each
	// of the possible characters.
	if (ReadNitroPalette("gfx/Misc/fontmiscb.ncl", (void*)HW_PLTT)) {
		debug("Failed to read font palette");
	}
	// force color 0 to black (we use hot pink, but that shows up now!)
	*(unsigned short*)HW_PLTT = 0;
	// load the character set
	if (ReadNitroChar("gfx/Misc/fontmiscb.ncg", G2_GetBG1CharPtr(), PalTable)) {
		debug("Failed to read font characters");
	}
}

/* Fade in and out an image on each screen. Can be aborted */
/* uses txr_level 2d ram for the subscreen */
void doLogoFade(char *topscr, char *btmscr) {
	int i,r,c;
	u16 j;
	volatile unsigned int nStartJiffies;
	static int isDone=0;		// don't do this more than twice with WIP screens, once otherwise

	MapScr_EndMap();

	if (OS_GetResetParameter()) {
		isDone = 2;
	}

	if (isDone > 1) {
		ShowBlack();
		return;
	}
	isDone++;

	// Early out
	if (isStartPressed()) {
		ShowBlack();
		return;
	}
 
	// no subscreen or window
	GX_SetVisiblePlane(0);
	GXS_SetVisiblePlane(0);
	GXS_SetVisibleWnd(0);
	// change background to 16-color
	G2S_SetBG0Control(GX_BG_SCRSIZE_TEXT_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE_0x6000, GX_BG_CHARBASE_0x00000, GX_BG_EXTPLTT_01);
	  
	// load the graphics - 256x192
    load_bmp_block_mask(topscr, NULL, txr_level, pal_level, -1);
    // and 256x192x4 (has to fit into 32k tile RAM)
    load_bmp_block_mask(btmscr, NULL, INVALID_PTR, INVALID_PTR, -2);	// load to 2d ram
     
	// and setup BG1 with an appropriate character map
	// Nintendo image is 256x192 which is 32x24 chars
	i=0; j=0;
	for (r=0; r<24; r++) {
		for (c=0; c<32; c++) {
			*(u16*)((u32) G2S_GetBG0ScrPtr() + i) = j++;
			i+=2;
		}
	}
	
	// enable correct BG on both screens
	pvr_scene_begin();
	pvr_scene_finish();
	GXS_SetMasterBrightness(-16);
	GXS_SetVisiblePlane(GX_PLANEMASK_BG0);
	GX_SetVisiblePlane(GX_PLANEMASK_BG0);
	
	// please don't 'return' after this point, use 'goto exit'	

	/* Fade in the logo */
	for (i=1; i<=31; i++) {
		pvr_scene_begin();
		SortFullPictureX(GX_TEXFMT_PLTT256, txr_level, pal_level, i);
		pvr_scene_finish();
		GXS_SetMasterBrightness(-((32-i)>>1));

		if (isStartPressed()) {
			goto exit;
		}
	}

	nStartJiffies=myJiffies;
	while (myJiffies < nStartJiffies+75) {
		if (isStartPressed()) {
			break;
		}
	}

	if (isStartPressed()) {
		goto exit;
	}

	/* Fade out the logo */
	for (i=31; i>0; i--) {
		pvr_scene_begin();
		SortFullPictureX(GX_TEXFMT_PLTT256, txr_level, pal_level, i);
		pvr_scene_finish();
		GXS_SetMasterBrightness(-((32-i)>>1));

		if (isStartPressed()) {
			goto exit;
		}
	}

exit:
	// turn off visible screens
//	ShowBlack();	// sets some subscreen states
	nStartJiffies=myJiffies;
}

