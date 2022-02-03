/****************************************/
/* Cool Herders                         */
/* A demented muse by Brendan Wiese     */
/* http://starbow.org                   */
/* Programmed by Mike Brent             */
/* http://harmlesslion.com              */
/* Copyright 2000-2005 Brendan Wiese &  */
/* Mike Brent.                          */
/* disclaimer.c                         */
/****************************************/

/* Ported to KOS 1.1.x by Dan Potter */

/* 
   Boot screens and loader for Cool Herders
*/

#include <stdio.h>
#include <malloc.h>

#ifdef WIN32
#include "kosemulation.h"
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <io.h>
#else
#include <kos.h>
#include <png/png.h>
#endif

#include "sprite.h"
#include "cool.h"
#include "sheep.h"
#include "sound.h"
#include "audioramdisk.h"
#include "font.h"
#include "vmu_logo.h"

#ifdef DEMO_BUILD
	// smaller to account for music copied in
	#define RAMDISK_CACHE_SIZE 9*1024*1024
#else
	// limit to 9.5MB - hopefully enough. Ran out of memory at 10.0MB.
	#define RAMDISK_CACHE_SIZE 9*1024*1024+512*1024
#endif

// SPU RAM 1.5MB
#define SRAMDISK_CACHE_SIZE 1024*1024+512*1024

static int nRamUsed=0, nSRamUsed=0;		// track how much we feed to the RAMdisk
static int nOldLev=-1;					// track one previous level

void load_data(void *p);
void doLogoFade(char *szImg, char *szSnd);
int  PreAllocPVR();
void ShowBlack();

extern pvr_ptr_t txr_sprites, txr_winner[4];
extern pvr_ptr_t txr_levela[4], txr_levelb[4], txr_levelc[4];
extern pvr_ptr_t txr_sheep, txr_smfont, txr_lgfont;
extern pvr_ptr_t txr_readygopause, txr_herder[4];
extern pvr_ptr_t txr_misc[4];
extern pvr_ptr_t txr_controlwork;
extern pvr_ptr_t disc_txr;
extern char *cpu_sprites, *cpu_lg_font;
extern char *title_cache_a, *title_cache_b;
extern char *playersel_cache_a, *playersel_cache_b;
extern char *worldsel_cache_a, *worldsel_cache_b;
extern unsigned int myJiffies;
extern int nFrames;
extern int gReturnToMenu;

int nCacheHits=0, nCacheMisses=0;

/* Just a misc place to put this - shared by various screens and the credits code too */
void SortSheepAndTitle() {
	addPageAlpha(txr_sheep, 460, 300, 112, 112, 254, 254, DEFAULT_COLOR, 1003.0f);
	addPageAlphaStretch(txr_smfont, 20, 20, 339, 169, 0, 140, 254, 254, DEFAULT_COLOR, 1003.0f);
}

/* Builds a little disclaimer texture (easier than doing a bunch of font polygons) */
/* We'll use this for dynamic textures throughout */
void disc_texture() {
	//   Derived from: KOS code by Dan Potter
	int	i, x, y = (480 - 11*24)/2;
	char	*msg[] = {
		"Now Loading:  Cool Herders",
		"",
		"Content \xA9 2005 HarmlessLion",
		"Game Design by Brendan 'Binky' Wiese",
		"Programming by Mike 'Tursi' Brent",
		"Music by Doug 'Tenzu' Ritchie",
		"Sound FX by Flossie, she was", 
		"the Prettiest Sheep in the World",
		"Thanks to Yak, Kevv, Marjan and Kiki",
		"",
		"http://www.harmlesslion.com"
	};

	/* Allocate texture space */
	if (NULL == disc_txr) {
		disc_txr = ALLOC_IMAGE(512, 512);
	}
	CLEAR_IMAGE(disc_txr, 512, 512);
	
	/* Draw a message into it */
	for (i=0; i<11; i++) {
		x = 256 - (strlen(msg[i]) * 12)/2;
		DRAW_TEXT(disc_txr, x, y, msg[i]);
		DRAW_TEXT(disc_txr, x+1, y, msg[i]);
		y += 25;
	}

	/* Kick the PVR chip or KOS lib or whatever causes that first timeout error... */
	/* No wait for PVR here on purpose */
	pvr_scene_begin();
	pvr_list_begin(PVR_LIST_OP_POLY);
	pvr_list_finish();
	pvr_scene_end;
}

/* Sort the text buffer into the display list */
void disc_display(int fade) {
	pvr_vertex_t	v;
	uint32 c;

	if (fade < 0.0f) fade = 0.0f;

	c=min(max((int)(fade)-gGfxDarken, 0), 255);

	SortHeader(PVR_LIST_OP_POLY, PVR_TXRFMT_RGB565 | PVR_TXRFMT_NONTWIDDLED, 512, 512, disc_txr);

	/* - -
	   + - */
	v.flags = PVR_CMD_VERTEX;
	v.x = 64.0f;
	v.y = 480.0f;
	v.z = 1.0f;
	v.u = 0.0f; v.v = 480.0f/512.0f;
	v.argb = INT_PACK_COLOR(255, c, c, c);
	v.oargb = 0;
	pvr_prim(&v, sizeof(v));

	/* + -
	   - - */
	v.y = 0.0f;
	v.v = 0.0f;
	pvr_prim(&v, sizeof(v));

	/* - -
	   - + */
	v.x = 640.0f - 64.0f;
	v.y = 480.0f;
	v.u = 1.0f; v.v = 480.0f / 512.0f;
	pvr_prim(&v, sizeof(v));

	/* - +
	   - - */
	v.flags = PVR_CMD_VERTEX_EOL;
	v.y = 0.0f;
	v.v = 0.0f;
	pvr_prim(&v, sizeof(v));
}

void do_screen() {
	int i;
	kthread_t * thd_hnd;

	/* Pre-allocate the static video buffers to reduce heap fragmentation */
	i=PreAllocPVR();

	/* Setup the texture */
	disc_texture();

	if (i==0) {		/* If nothing is loaded yet */
		/* Set another thread off on the loading, cause this text isn't THAT important */
		thd_hnd = thd_create(load_data, NULL);

		/* Fade in the disclaimer */
		for (i=0; i<256; i+=3) {
			BeginScene();
			
			pvr_list_begin(PVR_LIST_OP_POLY);
			disc_display(i);
			pvr_list_finish();

			pvr_scene_end;
		}

		/* wait for the data load to complete */
		thd_wait(thd_hnd);

		/* Fade out the disclaimer */
		for (i=255; i>=0; i-=3) {
			BeginScene();
			
			pvr_list_begin(PVR_LIST_OP_POLY);
			disc_display(i);
			pvr_list_finish();

			pvr_scene_end;
		}

		// Try to autoload save file
		doVMULoad(1);
	}

	// These logos must be 512x512 images */
	doLogoFade("gfx/Misc/HarmlessLogoSized.png", NULL);
	CLEAR_IMAGE(disc_txr, 512, 512);

	doLogoFade("gfx/Misc/copper.png", "/ram/copper.ogg");
	CLEAR_IMAGE(disc_txr, 512, 512);
}

// helper function to delete any level files from the RAMdisk to free up RAM
void flush_png_level_cache() {
	int a, b;
	// wipe the old level from ram
	// we sorta cheat and assume the filename and sizes here
	if (-1 != nOldLev) {
		debug("Clearing old level %c from RAMdisk\n", nOldLev);
		for (a='a'; a<'d'; a++) {
			for (b='1'; b<'5'; b++) {
				char buf2[128];
				sprintf(buf2, "/ram/level%c%c%c.png", nOldLev, a, b);
				if (-1 != fs_unlink(buf2)) {
					debug(" - Removed %s\n", buf2);
					nRamUsed-=256*256*2;	//256x256 tile page
				}
			}
		}
		nOldLev=-1;
	}
}

// Just gonna put this here -- too lazy to put it elsewhere :)
// fn = filename
// whereto = destination pvr address
// save = any combination of PNG_RETURN_RGB and PNG_NOCACHE or PNG_CACHE_LVL
//		PNG_RETURN_RGB returns a char * to the address of the raw RGB data (15 bit) (disables cache)
//		PNG_NOCACHE means do not cache in the RAMdisk (we may still look there)
//		PNG_CACHE_LVL means this is a level tile and can be cached temporarily
//		(Default is to cache permanently if possible and free the raw RGB data)
char* load_png_block_mask(char *fn, pvr_ptr_t whereto, int save, unsigned int mask) {
	kos_img_t	img;
	int nOldFrames, nThisLev=-1;
	file_t fp;
	char buf[128], *p, *p2;
	ssize_t total, left, r;
	uint8* out;

	// quicker exit during level loading
	if ((gReturnToMenu)&&(mask&PNG_CACHE_LVL)) {
		return NULL;
	}

	// Get the number on the level if set as such
	p=strrchr(fn, '/');		// use this again
	if (NULL != p) {
		p2=p+1;
	} else {
		p2=fn;
	}
	if (save&PNG_CACHE_LVL) {
		// the strings are levelxyz.png, we want 'x'
		nThisLev=p2[5];
	}
	
#ifndef WIN32
	// Don't bother trying if we're told to save the buffer - it's cached elsewhere and isn't wanted twiddled
	if (!(save&PNG_RETURN_RGB)) {
		// Make up a RAMdisk filename and test that first
		strcpy(buf, "/ram/");
		strcat(buf, p2);

		fp=fs_open(buf, O_RDONLY);
		if (fp != FILEHND_INVALID) {
			left = fs_total(fp);
			total = 0;
			out = (uint8 *)whereto;		// this is really a 16 bit buffer!
					
			while (left > 0) {
				r = fs_read(fp, out, left);
				if (r <= 0) {
					break;
				}
				left -= r;
				total += r;
				out += r;
			}
			fs_close(fp);
			debug("Loaded %d bytes for %s from RAMdisk instead of CD.\n", total, buf);
			nCacheHits++;
			return NULL;
		}

		// That didn't work? Try the sramdisk
		strcpy(buf, "/sram/");
		strcat(buf, p2);

		fp=fs_open(buf, O_RDONLY);
		if (fp != FILEHND_INVALID) {
			left = fs_total(fp);
			total = 0;
			out = (uint8 *)whereto;		// this is really a 16 bit buffer!
					
			while (left > 0) {
				r = fs_read(fp, out, left);
				if (r <= 0) {
					break;
				}
				left -= r;
				total += r;
				out += r;
			}
			fs_close(fp);
			debug("Loaded %d bytes for %s from SRAMdisk instead of CD.\n", total, buf);
			nCacheHits++;
			return NULL;
		}

		if (!(save&PNG_NOCACHE)) {
			nCacheMisses++;	
		}
	}
#endif

	// do the full load, then
	nOldFrames=myJiffies;
	debug("Loading %s\n", fn);
	if (png_to_img(fn, mask, &img) < 0) {
		debug("Can't load texture '%s' from file\n", fn);
		return NULL;
	}
	debug("Loaded in %d ticks. Now processing...\n", myJiffies-nOldFrames);
	nOldFrames=myJiffies;

	pvr_txr_load_kimg(&img, whereto, 0);

	debug("Done in %d ticks.\n", myJiffies-nOldFrames);
	
	if (save&PNG_RETURN_RGB) {
		return (char*)(img.data);
	} else {
#ifndef WIN32
		// cache this puppy in the RAMdisk in case we load it again later
		if (!(save&PNG_NOCACHE)) {
			if ((save&PNG_CACHE_LVL)&&(nOldLev != -1)) {
				// if this is a new level then free the old one
				if (nThisLev!=nOldLev) {
					flush_png_level_cache();
				}
			}
			if (nRamUsed+img.byte_count <= RAMDISK_CACHE_SIZE) {
				if (save&PNG_CACHE_LVL) {
					if (nThisLev!=nOldLev) {
						nOldLev=nThisLev;
					}
				}
				strcpy(buf, "/ram/");
				strcat(buf, p2);

				fp=fs_open(buf, O_WRONLY);
				if (fp == FILEHND_INVALID) {
					debug("Failed to write %s to RAMdisk, cause %d\n", buf, fp);
				} else {
					nRamUsed+=img.byte_count;

					debug("Writing %d bytes for %s to RAMdisk (%d left after)...\n", img.byte_count, buf, RAMDISK_CACHE_SIZE-nRamUsed);

					left = img.byte_count;
					total = 0;
					out = (uint8 *)whereto;	// this is really a 16 bit buffer!
					
					while (left > 0) {
						r = fs_write(fp, out, left);
						if (r <= 0) {
							break;
						}
						left -= r;
						total += r;
						out += r;
					}
					fs_close(fp);
				}
			} else if ((nSRamUsed+img.byte_count <= SRAMDISK_CACHE_SIZE)&&(!(save&PNG_CACHE_LVL))) {
				// try the SRAMdisk, but not for levels, because we never delete from here
				strcpy(buf, "/sram/");
				strcat(buf, p2);

				fp=fs_open(buf, O_WRONLY);
				if (fp == FILEHND_INVALID) {
					debug("Failed to write %s to SRAMdisk, cause %d\n", buf, fp);
				} else {
					nSRamUsed+=img.byte_count;

					debug("Writing %d bytes for %s to SRAMdisk (%d left after)...\n", img.byte_count, buf, SRAMDISK_CACHE_SIZE-nSRamUsed);

					left = img.byte_count;
					total = 0;
					out = (uint8 *)whereto;	// this is really a 16 bit buffer!
					
					while (left > 0) {
						r = fs_write(fp, out, left);
						if (r <= 0) {
							break;
						}
						left -= r;
						total += r;
						out += r;
					}
					fs_close(fp);
				}
			}
		}
#endif
	}

	kos_img_free(&img, 0);
	return NULL;
}

pvr_ptr_t pic_load(char *fn) {
	kos_img_t	img;
	pvr_ptr_t	addr;

	if (png_to_img(fn, PNG_MASK_ALPHA, &img) < 0) {
		debug("Can't load texture '%s' from file\n", fn);
		return NULL;
	}

	addr = ALLOC_IMAGE(img.w, img.h);
	pvr_txr_load_kimg(&img, addr, 0);
	kos_img_free(&img, 0);

	return addr;
}

/* allocate PVR RAM for images - we do it now before allocating */
/* the large temporary buffers for the intro screens to reduce  */
/* heap fragmentation (ie: all free space should be at the end) */
/* Returns 0 normally. If it returns 1, memory was already allocated */
/* Note: This plus disc_txr FILLS the video memory */
int PreAllocPVR() {
	int idx;

	if (txr_sprites != NULL) {
		return 1;
	}

	debug("PreAllocPVR...\n");

	pvr_mem_reset();

	txr_sprites=ALLOC_IMAGE(256, 256);
	
	// Be warned - most of the code assumes that arrays of 256x256
	// buffers are arranged one right after the other - so the ALLOC_IMAGE
	// calls must never be multi-threaded or random access (ie: no frees)
	for (idx=0; idx<4; idx++) {
		txr_winner[idx]=ALLOC_IMAGE(256, 256);
	}

	txr_sheep=ALLOC_IMAGE(256, 256);
	txr_smfont=ALLOC_IMAGE(256, 256);
	txr_lgfont=ALLOC_IMAGE(256, 256);
	txr_readygopause=ALLOC_IMAGE(256, 256);
	txr_controlwork=ALLOC_IMAGE(256, 256);

	/* background tile buffers - multiple loops because I assume the memory layout of these guys */
	for (idx=0; idx<4; idx++) {
		txr_levela[idx]=ALLOC_IMAGE(256, 256);
	}
	for (idx=0; idx<4; idx++) {
		txr_levelb[idx]=ALLOC_IMAGE(256, 256);
	}
	for (idx=0; idx<4; idx++) {
		txr_levelc[idx]=ALLOC_IMAGE(256, 256);
	}
	for (idx=0; idx<4; idx++) {
		txr_herder[idx]=ALLOC_IMAGE(256, 256);
	}
	for (idx=0; idx<4; idx++) {
		txr_misc[idx]=ALLOC_IMAGE(256, 256);
	}

#ifdef WIN32
	// on the dreamcast, these are just pointed at the base
	// memory for each array. On Windows, they are BITMAP structures
	// so must be reallocated. 
	txr512_levela=ALLOC_IMAGE(512,512);
	txr512_levelb=ALLOC_IMAGE(512,512);
	txr512_levelc=ALLOC_IMAGE(512,512);
	txr512_misc=ALLOC_IMAGE(512,512);
#endif
	
	pvr_mem_stats();

	/* we'll also allocate some CPU memory buffers */
	/* This code is not going to work on the PC! But time... */
	title_cache_a=malloc(PAGE512);
	title_cache_b=malloc(PAGE256);
	playersel_cache_a=malloc(PAGE512);
	playersel_cache_b=malloc(PAGE256);
	worldsel_cache_a=malloc(PAGE512);
	worldsel_cache_b=malloc(PAGE256);

	return 0;
}

/* Note: Background thread! */
extern int gHumanPlayer;
void load_data(void *p) {
	int idx;
	char buf[256];

	// prepare the sound system, cause our hacky ramdisk will use it, too :)
	sound_init();
	// Attach our hacky ramdisk
	spu_ramdisk_init();

	// Now we can start loading graphics
	cpu_sprites=load_png_block("gfx/Misc/sprites.png", txr_sprites, PNG_RETURN_RGB);// static
	
	for (idx=0; idx<4; idx++) {
		sprintf(buf, "gfx/Misc/p%d_win.png", idx+1);
		load_png_block(buf, txr_winner[idx], PNG_NOCACHE);							// static
	}

	// the magic sheep loads as 1555 (MASK_ALPHA), except the bottom right corner
	// >=112 on both axis, that loads as 4444 (FULL_ALPHA)
	load_png_block_mask("gfx/Misc/sheep.png", txr_sheep, PNG_NOCACHE, PNG_MAGIC_SHEEP);

	// the magic sheep worked so well, magic font will do the same with the cutoff being
	// >=140 on the Y axis ;)
	load_png_block_mask("gfx/Misc/font2.png", txr_smfont, PNG_NOCACHE, PNG_MAGIC_FONT);	/* save these in CPU memory */
	cpu_lg_font=load_png_block("gfx/Misc/font1.png", txr_lgfont, PNG_RETURN_RGB);
	load_png_block("gfx/Misc/readygopause.png", txr_readygopause, PNG_NOCACHE);		// static

#if 0
// Hack code to test ending
	if (!fs_copy(SONG_LOSE, SONG_GAMEOVER)) {
		debug("Failed to copy end.off\n");
	}
	extern int nContinues;
	nContinues=3;
	for (idx=0; idx<MAX_SHEEP; idx++) {
		memset(&sheep[idx], 0, sizeof(SheepStruct));
		sheep[idx].spr.txr_addr=txr_sheep;
		sheep[idx].spr.tilenumber=16;
		sheep[idx].spr.z=516.0;
		sheep[idx].spr.xd=1;
		sheep[idx].spr.yd=0;
		sheep[idx].type=2;	// active
		sheep[idx].invincible=1;
		sheep[idx].maxframes=SHEEPFRAMES;
		sheep[idx].range=240;	// start with 4 seconds of invincibility
	}
	idx=0;
	memset(&herder[idx], 0, sizeof(HerdStruct));
	herder[idx].type=PLAY_HUMAN;
	herder[idx].wins=0;
	herder[idx].spr.txr_addr=txr_herder[idx];
	herder[idx].spr.tilenumber=21;	// static down
	herder[idx].spr.alpha=255;
	herder[idx].spr.z=517.0;
	herder[idx].spr.xd=(idx<2 ? 1 : -1);
	herder[idx].spr.yd=0;
	herder[idx].spr.is3D=false;
	herder[idx].maxframes=6;
	herder[idx].berserker=60;
	herder[idx].lasthitby=0xff;
	herder[idx].lasthit=0xff;
	// Load the appropriate herder tileset
	if ((herder[idx].type&PLAY_MASK) != PLAY_NONE) {
		char buf[128];
		sprintf(buf, "gfx/Players/zeus/yl_zeus.png");
		debug("Player %d loading %s -> %s\n", idx, buf, (herder[idx].type&PLAY_MASK)==PLAY_COMPUTER?"Computer":"Human");
		load_png_block(buf, txr_herder[idx], 0);
	}
	gHumanPlayer=0;
	for (;;) {
		doGameOver();
		while (!isStartPressed());
		if (CONT_A==isStartPressed()) break;
	}
	arch_exit();
	return;
//// end hack
#endif

	/* rely on the load_png caching code to cache these now */
	load_png_block("gfx/Misc/newtitleA.png", txr_levela[0], 0);
	load_png_block("gfx/Misc/newtitleB.png", txr_levela[0], 0);

	load_png_block("gfx/Menu/Select1.png", txr_levela[0], 0);
	load_png_block("gfx/Menu/Select2.png", txr_levela[0], 0);

	load_png_block("gfx/Menu/World1.png", txr_levela[0], 0);
	load_png_block("gfx/Menu/World2.png", txr_levela[0], 0);

	/* copy various things to the RAM disk */
#ifndef WIN32
	if (!fs_copy(SONG_COPPER, "/ram/copper.ogg")) {
		debug("Failed to copy Copper.ogg\n");
	}
	if (!fs_copy(SONG_END, SONG_WIN)) {
		debug("Failed to copy WinJingle.ogg\n");
	}
	if (!fs_copy(SONG_LOOP, SONG_STORY)) {
		debug("Failed to copy loop.ogg\n");
	}
	if (!fs_copy(SONG_LOSE, SONG_GAMEOVER)) {
		debug("Failed to copy end.off\n");
	}

#ifndef DEMO_BUILD
	// This is just a bit of precaching for the player select
	// we discard it, but the load_png_block function will copy
	// them to the RAMdisk, all twiddled and ready to go
	load_png_block("gfx/Players/angl/angl_head.png", txr_misc[0], 0);
	load_png_block("gfx/Players/cand/cand_head.png", txr_misc[0], 0);
	load_png_block("gfx/Players/danc/danc_head.png", txr_misc[0], 0);
	load_png_block("gfx/Players/devl/devl_head.png", txr_misc[0], 0);
	load_png_block("gfx/Players/godd/godd_head.png", txr_misc[0], 0);
	load_png_block("gfx/Players/herd/herd_head.png", txr_misc[0], 0);
	load_png_block("gfx/Players/iskr/iskr_head.png", txr_misc[0], 0);
	load_png_block("gfx/Players/nh-5/nh-5_head.png", txr_misc[0], 0);
	load_png_block("gfx/Players/thal/thal_head.png", txr_misc[0], 0);
	load_png_block("gfx/Players/wolf/wolf_head.png", txr_misc[0], 0);
	load_png_block("gfx/Players/zeus/zeus_head.png", txr_misc[0], 0);
	load_png_block("gfx/Players/zomb/zomb_head.png", txr_misc[0], 0);
	load_png_block("gfx/Players/Woodgrain.png", txr_misc[0], 0);
#endif
	// And these precache Foxx's level select tiles - LOTS of RAM used
	// for these (512x512), but we want the menus to load quickly!
	load_png_block("gfx/Menu/level0.png", txr_misc[0], 0);
	load_png_block("gfx/Menu/level1.png", txr_misc[0], 0);
	load_png_block("gfx/Menu/level2.png", txr_misc[0], 0);
	load_png_block("gfx/Menu/level3.png", txr_misc[0], 0);
	load_png_block("gfx/Menu/level4.png", txr_misc[0], 0);
	load_png_block("gfx/Menu/level5.png", txr_misc[0], 0);
	load_png_block("gfx/Menu/level6.png", txr_misc[0], 0);
	load_png_block("gfx/Menu/level7.png", txr_misc[0], 0);

 #ifdef DEMO_BUILD
	if (!fs_copy(SONG_TITLE, SONG_RAM)) {
		debug("Failed to copy CoolHerders.ogg\n");
	}
 #endif
 #endif
}

/* Sorts the 512x512 solid logo in disc_txr (centered, top and bottom offscreen by 16 pixels each) */
/* f is the fade brightness (0=black, 1=full) */
void SortLogo(int f) {
	pvr_vertex_t	vert;
	uint32 c;

	c=min(max(f-gGfxDarken, 0), 255);

	SortHeader(PVR_LIST_OP_POLY, PVR_TXRFMT_ARGB1555, 512, 512, disc_txr);

	vert.flags = PVR_CMD_VERTEX;
	vert.x = 64;
	vert.y = 512;
	vert.z = 1.0f;
	vert.u = 0.0f;
	vert.v = 1.0f;
	vert.argb = INT_PACK_COLOR(255, c, c, c);
	vert.oargb = 0;
	pvr_prim(&vert, sizeof(vert));

	vert.y = 0;
	vert.v = 0.0f;
	pvr_prim(&vert, sizeof(vert));

	vert.x = 576;
	vert.y = 512;
	vert.u = 1.0f;
	vert.v = 1.0f;
	pvr_prim(&vert, sizeof(vert));

	vert.flags = PVR_CMD_VERTEX_EOL;
	vert.y = 0;
	vert.v = 0.0f;
	pvr_prim(&vert, sizeof(vert));
}


/* Fade in an image, play an optional sound. Can be aborted */
/* uses disc_txr, so can't use outside of the intro sequence */
void doLogoFade(char *szImg, char *szSnd) {
	int i;
	int nStartJiffies;

	// Early out
	if ((isStartPressed())||(gReturnToMenu)) {
		ShowBlack();
		return;
	}

	// Don't cache these on the RAMdisk, but don't bother saving the RAM either
    load_png_block(szImg, disc_txr, PNG_NOCACHE);
	
	// play the sound while it's loading
	if (NULL != szSnd) {
		sound_start(szSnd, 0);
	}

	/* Fade in the logo */
	for (i=0; i<=255; i+=5) {
		BeginScene();
		
		pvr_list_begin(PVR_LIST_OP_POLY);
		SortLogo(i);
		pvr_list_finish();

		pvr_scene_end;

		if (isStartPressed()) {
			ShowBlack();
			return;
		}
	}

	if (NULL != szSnd) {
		while (musicisplaying()) {
			if (isStartPressed()) {
				sound_stop();
				break;
			}
		}
	} else {
		nStartJiffies=myJiffies;
		while (myJiffies < nStartJiffies+150) {
			if (isStartPressed()) {
				break;
			}
		}
	}

	if (isStartPressed()) {
		ShowBlack();
		return;
	}

	/* Fade out the logo */
	for (i=255; i>=0; i-=5) {
		BeginScene();
		
		pvr_list_begin(PVR_LIST_OP_POLY);
		SortLogo(i);
		pvr_list_finish();

		pvr_scene_end;

		if (isStartPressed()) {
			ShowBlack();
			return;
		}
	}
}

#ifdef DEMO_BUILD
/* Sorts the 512x512 solid logo in disc_txr (stretched, top and bottom offscreen by 16 pixels each) */
/* f is the fade brightness (0=black, 1=full) */
void SortLogoWide(int f) {
	pvr_vertex_t	vert;

	SortHeader(PVR_LIST_OP_POLY, PVR_TXRFMT_ARGB1555, 512, 512, disc_txr);

	vert.flags = PVR_CMD_VERTEX;
	vert.x = 0;
	vert.y = 512;
	vert.z = 1.0f;
	vert.u = 0.0f;
	vert.v = 1.0f;
	vert.argb = INT_PACK_COLOR(255, f, f, f);
	vert.oargb = 0;
	pvr_prim(&vert, sizeof(vert));

	vert.y = 0;
	vert.v = 0.0f;
	pvr_prim(&vert, sizeof(vert));

	vert.x = 640;
	vert.y = 512;
	vert.u = 1.0f;
	vert.v = 1.0f;
	pvr_prim(&vert, sizeof(vert));

	vert.flags = PVR_CMD_VERTEX_EOL;
	vert.y = 0;
	vert.v = 0.0f;
	pvr_prim(&vert, sizeof(vert));
}


/* Fade in an image, play an optional sound. Can be aborted */
/* uses disc_txr, so can't use outside of the intro sequence */
void doSplashFade(char *szImg) {
	int i;
	int nStartJiffies;

	// Early out
	if (isStartPressed()) {
		return;
	}

    load_png_block(szImg, disc_txr, PNG_NOCACHE);
	
	/* Fade in the logo */
	for (i=0; i<=255; i+=5) {
		BeginScene();
		
		pvr_list_begin(PVR_LIST_OP_POLY);
		SortLogoWide(i);
		pvr_list_finish();

		pvr_scene_end;

		if (isStartPressed()) {
			return;
		}
	}

	nStartJiffies=myJiffies;
	while (myJiffies < nStartJiffies+250) {
		if (isStartPressed()) {
			break;
		}
	}

	/* Fade out the logo */
	for (i=255; i>=0; i-=5) {
		BeginScene();
		
		pvr_list_begin(PVR_LIST_OP_POLY);
		SortLogoWide(i);
		pvr_list_finish();

		pvr_scene_end;

		if (isStartPressed()) {
			return;
		}
	}
}

void DoSplashes() {
	sound_stop();

	sound_start(SONG_RAM, 1);

	if (isStartPressed()) {
		sound_stop();
		return;
	}

	doSplashFade("Bonus/gfx/img7.png");

	if (isStartPressed()) {
		sound_stop();
		return;
	}

	doSplashFade("Bonus/gfx/img31.png");
	sound_stop();
}

#endif
