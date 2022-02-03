/****************************************/
/* Cool Herders                         */
/* A demented muse by Brendan Wiese     */
/* http://starbow.org                   */
/* Programmed by Mike Brent             */
/* http://harmlesslion.com              */
/* Copyright 2000-2005 Brendan Wiese &  */
/* Mike Brent.                          */
/* cool.c                               */
/****************************************/

/* Ported to KOS 1.1.x by Dan Potter */

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>

#ifdef WIN32
#include "kosemulation.h"
#else
#include <kos.h>
#include <arch/arch.h>
#include <png/png.h>
#endif

#include "sprite.h"
#include "cool.h"
#include "sound.h"
#include "rand.h"
#include "levels.h"
#include "collide.h"
#include "sheep.h"
#include "font.h"
#include "score.h"
#include "menu.h"
#include "control.h"
#include "sound.h"
#include "audioramdisk.h"
#include "vmu_logo.h"
#include "pathfind.h"
#include "storymode.h"
#include "cheat.h"

#ifdef GOAT_DEMO_BUILD
#include "/home/tursi/Dreamcast/libmenu/libmenu.h"
#endif

void do_screen();
void load_data();
void DoSplashes();

/* game variables */
int nFrames;
int level, stage;
int nContinues;
unsigned int nGameFrames;
unsigned int myJiffies=0;
int gAlwaysWinStoryMode=0;
int gAlwaysWinChallenges=0;
int StoryModeTotalScore;
int isMultiplayer;
int gReturnToMenu=0, gReturned=1, gAtMain=0;
int gDontCheckLid=0;	// set to not check CD lid during VMU saves
int gIsHardStory=0;
int gOnlyPlayStoryMode=0;
pvr_ptr_t txr_sprites=NULL, txr_winner[4]={NULL, NULL, NULL, NULL};
pvr_ptr_t txr_levela[4]={NULL, NULL, NULL, NULL}, txr_levelb[4]={NULL, NULL, NULL, NULL}, txr_levelc[4]={NULL, NULL, NULL, NULL};
#ifdef WIN32
// non-WIN32 aliases these to the above
pvr_ptr_t txr512_levela=NULL, txr512_levelb=NULL, txr512_levelc=NULL, txr512_misc=NULL;
#endif
pvr_ptr_t txr_misc[4]={NULL, NULL, NULL, NULL};
pvr_ptr_t txr_sheep=NULL, txr_smfont=NULL, txr_lgfont=NULL;
pvr_ptr_t txr_readygopause=NULL, txr_herder[4]={NULL, NULL, NULL, NULL};
pvr_ptr_t txr_controlwork=NULL, disc_txr=NULL;
extern char *title_cache_a, *title_cache_b;
extern char *playersel_cache_a, *playersel_cache_b;
extern char *worldsel_cache_a, *worldsel_cache_b;
extern int fGhostSheep;
extern int nCacheHits, nCacheMisses;
extern char StoryIntroText[][25];
extern int StoryModeTotalScore;
extern int SheepSpeed;
extern int lastbuttons[4];

char	szLastStoryCmd[16];
char    *cpu_sprites=NULL, *cpu_lg_font=NULL;
int		nBgAnimA, nBgAnimB, nBgAnimC;
int		nBgSpeedA, nBgSpeedB, nBgSpeedC;
int		nBgCountA, nBgCountB, nBgCountC;
int		idx, inDemo;
int		sndCountdown;
int	    nTimeLeft;
int		sFx;
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
	"RED",
	"BUL",
};

extern char szColors[4][8];
extern char szNames[13][8];

/* for CD Door check */
static int vbl_hnd;

HerdStruct herder[4] ;
char cheatstring[11];	// note: 10 plus nul termination

/*****************/

#ifdef USE_ROMDISK
// not using ROMDISK anymore - but this is how you do activate it
extern uint8 romdisk[];
KOS_INIT_ROMDISK(romdisk);
#endif

KOS_INIT_FLAGS(INIT_DEFAULT | INIT_MALLOCSTATS);

/******************/
/* Error Handling */
/******************/

#ifndef WIN32
#ifndef USE_PC
static void irqHandler(irq_t source, irq_context_t * context) {
	debug("Unhandled IRQ: Source: %d  Context: 0x%p\n", (int)source, context);
#ifdef GOAT_DEMO_BUILD
	goat_exit();
#else
	arch_exit();
#endif
}
#endif

/* This is our quick checker that looks for an open tray at bad times,
   and updates the actual disc status. (thanks Dan) */
static void cdtVbl(uint32 code) {
	int rv, dt;
	int cd_status;

	if (gDontCheckLid) return;

	rv = cdrom_get_status((int *)&cd_status, &dt);
	if (rv >= 0) {
		if (cd_status == CD_STATUS_OPEN) {
			debug("cdtVbl: detected CD tray open - exitting\n");
#ifdef GOAT_DEMO_BUILD
			goat_exit();
#else
			arch_exit();
#endif
		}
	}
}
#endif

/* Disable this function in a release build to prevent debug from printing */
void debug(char *str, ...) {
#ifndef FINALBUILD
	char buf[1024];
	va_list va;

	va_start(va, str);
	vsprintf(buf, str, va);
	va_end(va);
	printf("[%06d.%02d] %s", myJiffies/50, (myJiffies%50)*2, buf);		// approximate runtime in seconds
#endif
}

#ifdef GOAT_DEMO_BUILD
void print_table(score_table_t * t) {
	int i;

	printf("\nprinting score table:\n");
	printf("game_id = %08lx\n", t->game_id);
	printf("settings = %08lx %08lx\n", t->settings[0], t->settings[1]);
	printf("play_cnt = %d\n", t->play_cnt);
	printf("bitfield = %02x\n", t->bitfield);
	printf("score_cnt = %d\n", t->score_cnt);

	printf("--------------------------------------------\n");
	printf("Name    |Score   |Diff    |Level   |Time\n");
	for (i=0; i<t->score_cnt; i++) {
		printf("%-8s|%08lx|%08lx|%08lx|%08lx\n",
			t->entries[i].name,
			t->entries[i].score,
			t->entries[i].difficulty,
			t->entries[i].level_reached,
			t->entries[i].time_lasted);
	}
	printf("--------------------------------------------\n\n");
}
#endif

// This function is now called when A+B+X+Y+START is pressed, to return to menu or exit game
// gReturnToMenu must be checked in all loops
void BackToMenu(uint8 addr, uint32 btns) {
	gReturnToMenu=25;
	if (!gAtMain) {
		gReturned=0;
	}
}

// this function picks a random level, aware of the unlock flags
int GetRandomLevel() {
	static int nLastRet=-1;
	int ret=-1;

	// HACKY DON'T CHECK IN - force either heaven or hell, or a challenge stage
	ret=ch_rand()%2;
	switch (ret) {
	case 0: ret=LEVEL_HEAVEN; break;
	case 1: ret=LEVEL_HELL; break;
	}
	return ret;
#if 0
	// doesn't include heaven and hell
	do {
		ret=ch_rand()%(MAX_LEVELS-2);
	} while (ret == nLastRet);

	// The others are in the unlock flags struct
	while (0 == (gGame.UnlockFlags & (1<<(ret+(gIsHardStory?SHIFT_HARD_STAGE:SHIFT_EARNED_STAGE))))) {
		ret--;
		if (ret < 0) {
			// why is New Zealand locked??
			debug("Odd, NZ is locked!! Selecting it anyway.\n");
			ret=0;
			break;
		}
	}

	nLastRet=ret;
	return ret;
#endif
}

/*****************/

int main(int argc, char *argv[]) {
	int idx;
	int StartPressed;
	int nStartOK;
	int fLoadTitle=0;
	int musicstoppedat=0;
	int givemusicslack;
	//pvr_init_defaults();
	pvr_init_params_t params = {
		/* Enable lists with size 16 (16 what?) */
		  // Opaque       Qpaque Mod     Translucent     Trans Mod      Punchthrough
		{ PVR_BINSIZE_16, PVR_BINSIZE_0, PVR_BINSIZE_16, PVR_BINSIZE_0, PVR_BINSIZE_0 },

		/* Vertex buffer size 256K (default 512) */
		256*1024,

		/* No DMA */
		0
	};
#ifdef GOAT_DEMO_BUILD
	int i,j;
#endif

#ifdef WIN32
	debug("Initializing Cool Herders " __DATE__ " for Windows...\n");
#else 
	debug("Initializing Cool Herders " __DATE__ " for KOS 1.3.x...\n");
#endif

	// First things first we initialize our globals to defaults! :)
	gGame.Options.Timer=60;			// 0,30,60,90
	gGame.Options.Rounds=2;			// 1,2,3
	gGame.Options.Win=0;			// 0=score, 1=sheep
	gGame.Options.Powers=1;			// 0=no powerups, 1=powerups
	gGame.Options.Skill=1;			// 0=Lost, 1=Normal, 2=Persistent
	gGame.Options.GhostMaze=0;		// 0=normal (haunted only), 1=never, 2=always
	gGame.Options.SheepSpeed=0;		// -1=slower, 0=normal, 1=faster
	gGame.Options.NightMode=NIGHT_DARKEN;	// 0=obsolete default, 1-254=amount to darken by, 0x8000 must be set to be on
	gGame.SVol=9;			// 0-9
	gGame.MVol=9;			// 0-9
	gGame.CPU=1;			// 0=no CPU in multiplayer, 1=CPU takes empty slots in multiplayer
	gGame.AutoSave=0;		// 0=no autosave, 1=autosave
	gGame.NumPlays=0;		// Number of times played (useful with autosave)
	gGame.UnlockFlags=UNLOCK_UNUSED|(1<<SHIFT_EARNED_STAGE);	// bitfields for unlockables (unused bits are set to 1, and level 0 is unlocked)

	gVMUUsed=NULL;			// VMU last used for load/save (NULL=none)
	for (idx=0; idx<10; idx++) {	// high scores - 0 is highest!
		strcpy(gGame.HighName[idx], DefaultHigh[idx]);	// 3 initials & NUL
		gGame.HighScore[idx]=(10-idx)*5000;				// Actual score obtained
	}
	for (idx=0; idx<5; idx++) {
		gGame.ChallengeScore[idx]=0;	// best scores in the challenge levels
	}
	strcpy(cheatstring, "cheat_here");

#ifdef GOAT_DEMO_BUILD
	// do some stuff for the GOAT Menu system
	debug("goat_init() returned %d\n", goat_init());
	debug("goat_menu_present() returned %d\n", goat_menu_present());
	debug("goat_50hz() returned %d\n", goat_50hz());

	score_table_t *pScore=NULL;

	printf("goat_save_erase():\n");
	goat_save_erase();

	pScore=goat_load_score_table(0);
	if (NULL == pScore) {
		debug("Got NULL for pScore\n");

		pScore = (score_table_t *)malloc(SCORE_TABLE_SIZE_CNT(5));
		pScore->settings[0] = 0xdeadbeef;
		pScore->settings[1] = 0xfeedc0de;
		pScore->play_cnt = 512;
		pScore->bitfield = SCORE_NAME | SCORE_SCORE | SCORE_DIFF | SCORE_LEVEL | SCORE_TIME;
		pScore->score_cnt = 5;

		for (i=0; i<5; i++) {
			sprintf(pScore->entries[i].name, "Entry%d", i);
			pScore->entries[i].score = i*10;
			pScore->entries[i].difficulty = i;
			pScore->entries[i].level_reached = i+1;
			pScore->entries[i].time_lasted = i*100;
		}

		// Pound it hard to see if we get any failures...
		for (i=0; i<10; i++) {
			for (j=0; j<5; j++) {
				pScore->entries[j].score = ch_rand();
				pScore->entries[j].difficulty = ch_rand();
				pScore->entries[j].level_reached = ch_rand();
				pScore->entries[j].time_lasted = ch_rand();
			}
			int rv = goat_save_score_table(i*4, pScore);
			printf("goat_save_score_table(%d, pScore) = %d\n", i*4, rv);
			if (rv < 0) {
				perror("save");
			}
		}
	}

	debug("Got a score table!\n");
	print_table(pScore);

	if (goat_save_score_table(0, pScore) < 0) {
		debug("Got some kind of error resaving it.\n");
	} else {
		debug("Saved it successfully, too!\n");
	}
	free(pScore);

	goat_exit();
#endif

// If running under Windows, assume current directory is correct
#ifndef WIN32
#ifdef USE_PC
	fs_chdir("/pc/home/tursi/Dreamcast/kos/ch/romdisk");
#else
	// Set up exception handlers
	irq_set_handler(EXC_UNHANDLED_EXC, irqHandler);
	irq_set_handler(EXC_DOUBLE_FAULT, irqHandler);

	// Set default folder
	fs_chdir("/cd");
#endif

#ifdef FINALBUILD
	// exit to menu in finalbuild mode
#ifndef GOAT_DEMO_BUILD
	arch_set_exit_path(ARCH_EXIT_MENU);		
#endif
#endif
	
	// Set up CDROM door check on the VBlank
	vbl_hnd = vblank_handler_add(cdtVbl);
#endif

	pvr_init(&params);

	// Callback to exit when pressed on any controller
	cont_btn_callback(0, CONT_START | CONT_A | CONT_B | CONT_X | CONT_Y, BackToMenu);

	memset(herder, 0, sizeof(herder));

	// Put something on the VMUs
	vmuInit();

LoopDemo:
	if (gGame.AutoSave) {	
		doVMUSave(1);
	}

	// remove player icons from VMUs
	for (idx=0; idx<4; idx++) {
		// Zero out the herders struct
		memset(&herder[idx], 0, sizeof(HerdStruct));

		herder[idx].nIcon=0;
	}

	// if there's any music playing, can it
	sound_stop();

	// Display initial copyright screen
	do_screen();

	inDemo=0;
	sndCountdown=0;

	/* We don't loop the music because we only want to play it once anyway */
	sound_start(SONG_TITLE, 0);

	/* make sure Start is released before we proceed */
	nStartOK=false;
	fLoadTitle=true;
	debug("\nWaiting for START...\n");
	nFrames=0;
	musicstoppedat=0;
	givemusicslack=0;

	while(1) {
		// remove player icons from VMUs (has to be in multiple spots due to multiple paths)
		for (idx=0; idx<4; idx++) {
			herder[idx].nIcon=0;
		}

		gAtMain=1;
		gGfxDarken=0;

		if (fLoadTitle) {
			if ((gGame.NumPlays >= 10) && (0 == (gGame.UnlockFlags&DISABLE_POWERUPS))) {
				DisablePowerups();
				givemusicslack=1;
			}
			if ((gGame.NumPlays >= 20) && (0 == (gGame.UnlockFlags&ENABLE_MUSIC_GAL))) {
				EnableMusicGallery();
				givemusicslack=1;
			}
			if ((gGame.NumPlays >= 30) && (0 == (gGame.UnlockFlags&ENABLE_GHOST_CTL))) {
				EnableGhostSheep();
				givemusicslack=1;
			}
			if ((gGame.NumPlays >= 40) && (0 == (gGame.UnlockFlags&ENABLE_IMAGE_GAL))) {
				EnableImageGallery();
				givemusicslack=1;
			}
			if ((gGame.NumPlays >= 50) && (0 == (gGame.UnlockFlags&ENABLE_NIGHT_MDE))) {
				EnableNightMode();
				givemusicslack=1;
			}

			if (gGame.AutoSave) {	
				if (doVMUSave(1)) {
					givemusicslack=1;
				}
			}
			
			/* copy the title page into the level textures for display */
			/* these are now assumed cached in the ramdisk */
			load_png_block("gfx/Misc/newtitleA.png", disc_txr, 0);
			load_png_block("gfx/Misc/newtitleB.png", txr_levela[0], 0);
			fLoadTitle=false;
		}

		if ((!musicisplaying()) && (musicstoppedat==0)) {
			if (!givemusicslack) {
				musicstoppedat=nFrames;
				debug("Music stopped at %d\n", musicstoppedat);
			} else {
				givemusicslack--;
				sound_start(SONG_TITLE, 0);
			}
		}

		// now to get it onscreen
		BeginScene();
		pvr_list_begin(PVR_LIST_OP_POLY);

		// Opaque polygons
		SortFullPicture(disc_txr, txr_levela[0], 1023.9f);	// title page

		pvr_list_finish();

		pvr_list_begin(PVR_LIST_TR_POLY);
		// Transparent polygons

		if ((nFrames>>6)&1) {
#ifdef DEMO_BUILD
			DrawFont(1, MENUX, MENUY+40, 0xFF8080FF, "Demo - Press Start");
#else
			DrawFont(1, MENUX, MENUY+40, 0xFF8080FF, "Press Start");
#endif
		}

		pvr_list_finish();
		pvr_scene_end;

		// Nothing to return to here
		if (gReturnToMenu) {
			// this gives us a primitive debounce for cheap or old controllers
			if (gReturned>25) {
				sound_stop();
				GoByeBye(0,0);
			} else {
				gReturnToMenu=0;
			}
		}

		StartPressed = 0;
		// isStartPressed will set gReturned for us when X+Y are released
		if (CONT_START == isStartPressed()) {
			// Seed random number generator
			ch_srand(myJiffies);
			StartPressed=1;
		}

		/* Not allowed to press start again till it's been released! */
		if ((!StartPressed)&&(gReturned)) {
			nStartOK=true;
		} else {
			if (nStartOK == false) {
				StartPressed=0;
			}
		}

		// check for cheats
		if ((StartPressed)&&(DoCheat(cheatstring))) {
			strcpy(cheatstring, "AAAAAAAAAA");
			StartPressed=0;
			break;
		}

		if (StartPressed) {
			gAtMain=0;

			debug("Leaving title page...\n");

			fLoadTitle=true;

			if (HandleMenus()) {
				/* Menu requested exit */
				break;
			}

			/* reset frame count to prevent demo ;) */			
			nFrames = 1;
			/* restart title music */
			sound_start(SONG_TITLE, 0);
		}

		//debug("nFrames = %d\n", nFrames);
		// Activate demo after a certain time
		if ((musicstoppedat)&&(nFrames-musicstoppedat > 75)) {
			gAtMain=0;

#ifdef DEMO_BUILD
			// this demo, we allow levels 1,2,4, but we'll allow any stage
			level=ch_rand()%3+1;
			if (level==3) level=4;
#else
			level=GetRandomLevel();
#endif
			stage=ch_rand()%3;
			inDemo=1;
		}

		if (inDemo) {
			gAtMain=0;

			debug("Starting demo...\n");
			fLoadTitle=true;
			for (idx=0; idx<4; idx++) {
				herder[idx].type=PLAY_COMPUTER|(ch_rand()%12);
			}
			Game(1);
			nFrames=0;
			musicstoppedat=0;

			if (!gReturnToMenu) {
				ShowHighScores(20*60+255);		// show for up to 20 seconds plus fade
			}

#ifdef DEMO_BUILD
			break;
#else
			sound_start(SONG_TITLE, 0);
#endif
			inDemo=0;
		}
	}

	fLoadTitle=true;
	goto LoopDemo;

	/* Not reached */
#ifdef GOAT_DEMO_BUILD
	goat_exit();
#endif
	return 0;
}

// Displays a 512x480 image from a 512x512 texture, stretching it to fit
void AddPicture(pvr_ptr_t txr_addr, float z) {
	pvr_vertex_t	vert;
	uint32 c;

	c=min(max(0xff-gGfxDarken, 0), 255);

	SortHeader(PVR_LIST_OP_POLY, PVR_TXRFMT_ARGB1555, 512, 512, txr_addr);

	vert.flags = PVR_CMD_VERTEX;
	vert.x = 0;
	vert.y = 480;
	vert.z = z;
	vert.u = 0.0f;
	vert.v = 0.9375f;
	vert.argb = INT_PACK_COLOR(0xff, c, c, c);
	vert.oargb = 0;
	pvr_prim(&vert, sizeof(vert));

	vert.y = 0;
	vert.v = 0.0f;
	pvr_prim(&vert, sizeof(vert));

	vert.x = 640;
	vert.y = 480;
	vert.u = 1.0f;
	vert.v = 0.9375f;
	pvr_prim(&vert, sizeof(vert));

	vert.flags = PVR_CMD_VERTEX_EOL;
	vert.y = 0;
	vert.v = 0.0f;
	pvr_prim(&vert, sizeof(vert));
}

// This function is called when the magic A+B+X+Y+START is pressed to exit everything
// Used to print PVR stats, but they appear to be absolute junk. :/
void GoByeBye(uint8 addr, uint32 btns) {
	if (nCacheMisses == 0) {
		debug("Shutting down. No cache misses\n");
	} else {
		debug("Shutting down. Cache hits=%d, Misses=%d, Percentage=%d%%\n", nCacheHits, nCacheMisses, nCacheHits*100/(nCacheMisses+nCacheHits));
	}
	
#ifndef WIN32
	vblank_handler_remove(vbl_hnd);
#endif
	
	// shutdown our hacky ramdisk
	spu_ramdisk_shutdown();

	vmuShutdown();
#ifdef GOAT_DEMO_BUILD
	goat_exit();
#else
	arch_exit();
#endif
}

// Display the blank screen with 'loading' in the middle
void ShowLoading() {
	// Get the 'loading' prompt up
	BeginScene();
	
	// Opaque polygons
	pvr_list_begin(PVR_LIST_OP_POLY);
	pvr_list_finish();
	
	// transparent polygons
	pvr_list_begin(PVR_LIST_TR_POLY);
	addPage(txr_sprites, 0, 208, 144, 250);
	pvr_list_finish();

	// draw it
	pvr_scene_end;
}

void ShowBlack() {
	// Draw a black screen to prevent visible texture glitches
	BeginScene();
	
	// Opaque polygons
	pvr_list_begin(PVR_LIST_OP_POLY);
	pvr_list_finish();
	
	// transparent polygons
	pvr_list_begin(PVR_LIST_TR_POLY);
	pvr_list_finish();

	// draw it
	pvr_scene_end;
}

// helper thread for start of level
// When a level is selected, the level previews fade out while the main tile grows 
// to 100%. When it's finished scaling, the word 'Loading' is displayed.
// This code must happen in a separate thread outside of this function, however, so that
// it happens *while* the level is loaded.
// pDat points to a multiline string to display. 
void scale_gfx(void *pDat) {
	for (idx=0; idx<200; idx+=10) {
		// Start drawing the frame
		BeginScene();
		pvr_list_begin(PVR_LIST_OP_POLY);
		// Opaque polygons
		
		// Scale selected from 225x200 to 450x400, in txr512_misc
		stretchLarge2(txr512_misc, 207-idx/2, 50, 432+idx/2, 250+idx, 0, 0, 449, 398, 1024.0f, INT_PACK_ALPHA(255));

		pvr_list_finish();

		pvr_list_begin(PVR_LIST_TR_POLY);
		// Transparent polygons

		if (NULL == pDat) {
			// the current level's 3 level previews are shown at the bottom, fading out quickly
			stretchLarge2(txr512_misc, 138, 300, 251, 399, 0, 400, 127, 499, 1025.0f, INT_PACK_ALPHA(max((stage==0?255:124)-idx*2,0)));
			stretchLarge2(txr512_misc, 263, 300, 376, 399, 128, 400, 255, 499, 1025.0f, INT_PACK_ALPHA(max((stage==1?255:124)-idx*2,0)));
			stretchLarge2(txr512_misc, 388, 300, 501, 399, 256, 400, 383, 499, 1025.0f, INT_PACK_ALPHA(max((stage==2?255:124)-idx*2,0)));
		}
		
		if (idx+10 >= 200) {
			// last frame, show the loading banner 
			addPage(txr_sprites, 0, 208, 144, 250);
		}

		if (NULL == pDat) {
			// Draw the main picture
			SortFullPicture(disc_txr, txr_controlwork, 1023.9f);
		} else {
			CenterDrawFontBackgroundBreaksZ(1026.0f, 0, 64, DEFAULT_COLOR, INT_PACK_COLOR(128,64,64,255), (char*)pDat);
		}

		pvr_list_finish();
		pvr_scene_end;

		if (gReturnToMenu) {
			sound_stop();
			break;
		}
	}
	// pause so the screen is visible
	thd_sleep(1000);
}

// Helper to deal with velocities on push tiles
// returns non-zero if we added any push
int AddPush(int idx, int gridx, int gridy) {
	int ret=0;
	// check the spot we're moving FROM for any push and add it in unless walking across it
//	if (nFrames&1) {	// every other frame is broken
		// every other frame only, otherwise it's too fast for
		// a non-upgraded herder! :)
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
//	}
	return ret;
}

// does a little game over routine
// draw Zeus standing there onscreen, and have all 29 sheep jump out and
// run off screen, fading out.
// This function does rely on the sheep and player being previously initialized by a game
// Return 0 if it really is Game Over, or 1 if the level should be retried
int doGameOver() {
	int bgX=0;
	char str[9]="AAAAAAAA";

	sound_stop();

	debug("Game Over\n");

	// get the background picture
	load_png_block("gfx/Menu/World1.png", disc_txr, 0);
	load_png_block("gfx/Menu/World2.png", txr_misc[0], 0);

	// Make sure the lights are on
	gGfxDarken=0;

	// Move Zeus into position
	herder[gHumanPlayer].spr.x=320;
	herder[gHumanPlayer].spr.y=300;
	herder[gHumanPlayer].spr.tilenumber=21;
	herder[gHumanPlayer].spr.z=200.0f;

	// Now init all the sheep
	for (idx=0; idx<MAX_SHEEP; idx++) {
		sheep[idx].spr.x=320;
		sheep[idx].spr.y=300;
		sheep[idx].spr.z=127.0f;
		sheep[idx].spr.xd=(ch_rand()%6)-3;
		sheep[idx].spr.yd=-1*(int)(ch_rand()%(idx+1))-3;
		if (sheep[idx].spr.yd < -16) sheep[idx].spr.yd=-16;
		sheep[idx].spr.tilenumber=sheep[idx].spr.xd<0?0:2;
		sheep[idx].animframe=0;
		sheep[idx].spr.alpha=255;
		sheep[idx].spr.is3D=true;
		sheep[idx].type=1;
	}

	// Check if the player wants to retry, if they have continues left
	if (nContinues) {
		char buf[64];
		int nResp=1;
		int nAck=1;
		
		sound_start(SONG_STORY, 1);

		// Set up the string
		sprintf(buf, "You failed!\n%d continues left\nTry Again?", nContinues);

		// This is okay, since if they say no the remainder are thrown away anyway
		nContinues--;

		while (nAck) {
			BeginScene();
			
			// Opaque polygons
			pvr_list_begin(PVR_LIST_OP_POLY);
			
			gGfxDarken=128;
			SortFullPictureX(disc_txr, txr_misc[0], 100.0f, bgX);
			gGfxDarken=0;
			bgX++;
			if (bgX>640) bgX=-639;

			pvr_list_finish();
			
			// transparent polygons
			pvr_list_begin(PVR_LIST_TR_POLY);
		
			// Draw the continue text
			CenterDrawFontBreaksZ(300.0f, 0, 100, DEFAULT_COLOR, buf);
			// Now draw the yes and no
			DrawFontBackgroundZ(300.0f, 0, 256, 190, DEFAULT_COLOR, nResp==1?INT_PACK_COLOR(255,255,0,0):INT_PACK_COLOR(0,0,0,0), "Yes");
			DrawFontBackgroundZ(300.0f, 0, 352, 190, DEFAULT_COLOR, nResp==0?INT_PACK_COLOR(255,255,0,0):INT_PACK_COLOR(0,0,0,0), "No");

			// Finally, sort our poor beleagured hero... 
			SortSprite(&herder[gHumanPlayer].spr);

			pvr_list_finish();

			// draw it
			pvr_scene_end;

			// Handle the player's input - left right and A/Start
			MAPLE_FOREACH_BEGIN(MAPLE_FUNC_CONTROLLER, cont_state_t, st);
				if (NULL == st) {
					continue;
				}
				
				if (st->joyy < -JOYDEAD) st->buttons |= CONT_DPAD_UP;
				if (st->joyy > JOYDEAD)  st->buttons |= CONT_DPAD_DOWN;
				if (st->joyx < -JOYDEAD) st->buttons |= CONT_DPAD_LEFT;
				if (st->joyx > JOYDEAD)  st->buttons |= CONT_DPAD_RIGHT;

				// wait for old keys to be released
				if (st->buttons&lastbuttons[__i]) continue;
				
				// save old keys
				lastbuttons[__i]=st->buttons;
				
				// ignore Start *AND* A (same as the left/right check elsewhere
				if ((st->buttons&CONT_START)&&(st->buttons&CONT_A)) {
					continue;
				}
				
				if (st->buttons == CONT_DPAD_UP) {
					memmove(str, str+1, 8);
					str[7]='U';
					str[8]='\0';
				}
				if (st->buttons == CONT_DPAD_DOWN) {
					memmove(str, str+1, 8);
					str[7]='D';
					str[8]='\0';
				}

				if (st->buttons & CONT_DPAD_RIGHT) {
					nResp=0;
					memmove(str, str+1, 8);
					str[7]='R';
					str[8]='\0';
				}
				if (st->buttons & CONT_DPAD_LEFT) {
					nResp=1;
					memmove(str, str+1, 8);
					str[7]='L';
					str[8]='\0';
				}
				if (st->buttons & (CONT_START|CONT_A)) {
					nAck=0;
					break;
				}
			MAPLE_FOREACH_END();

			if (strcmp(str, "UUDDLRLR")==0) {
				nContinues++;
				strcpy(str, "AAAAAAAA");
				sprintf(buf, "You failed!\n%d continues left\nTry Again?", nContinues);
			}

			if (gReturnToMenu) {
				sound_stop();
				return 0;		// cause '1' would mean continue
			}
		}

		sound_stop();

		if (nResp) {
			return 1;
		}
	}

	// Now start the music, and the animation!
	sound_start(SONG_GAMEOVER, 0);

	for (;;) {
		int nSheepLeft;
		
		BeginScene();
		
		// Opaque polygons
		pvr_list_begin(PVR_LIST_OP_POLY);
		
		gGfxDarken=128;
		SortFullPictureX(disc_txr, txr_misc[0], 100.0f, bgX);
		gGfxDarken=0;
		bgX++;
		if (bgX>640) bgX=-639;

		pvr_list_finish();
		
		// transparent polygons
		pvr_list_begin(PVR_LIST_TR_POLY);
	
		// First, two rectangles in the foreground handle fading effects for the sheep
//		SortRectHoriz(256.0f, 0, 268, 260, 479, INT_PACK_COLOR(255,0,0,0), INT_PACK_COLOR(0,0,0,0));
//		SortRectHoriz(256.0f, 380, 268, 639, 479, INT_PACK_COLOR(0,0,0,0), INT_PACK_COLOR(255,0,0,0));

		// Next, sort the sheep
		nSheepLeft=0;
		for (idx=0; idx<MAX_SHEEP; idx++) {
			if (sheep[idx].type) {
				// move them here
				if (sheep[idx].spr.y < 300) {
					// apply gravity
					sheep[idx].spr.yd++;
				}

				sheep[idx].spr.x+=sheep[idx].spr.xd;
				sheep[idx].spr.y+=sheep[idx].spr.yd;

				if ((sheep[idx].spr.x > 600) || (sheep[idx].spr.x < 40)) {
					sheep[idx].type=0;
					continue;
				}
				if (sheep[idx].spr.x > 345) {
					sheep[idx].spr.alpha=600-sheep[idx].spr.x;
				}
				if (sheep[idx].spr.x < 295) {
					sheep[idx].spr.alpha=sheep[idx].spr.x-40;
				}

				if (sheep[idx].spr.y > 300) {
					// Hits the ground running!
					if (sheep[idx].spr.xd < 0) {
						sheep[idx].spr.xd-=10;
					} else {
						sheep[idx].spr.xd+=10;
					}
					sheep[idx].spr.y=300;
					sheep[idx].spr.yd=0;
				}
				
				if (nFrames%4 == 0) {
					sheep[idx].animframe++;
					if (sheep[idx].animframe >= sheep[idx].maxframes) {
						sheep[idx].animframe=0;
					}
					sheep[idx].spr.tilenumber=SheepAnimationFrame[(sheep[idx].spr.xd<0)?0:2][sheep[idx].animframe];
				}

				SortSprite(&sheep[idx].spr);
				nSheepLeft++;
			}
		}

		// Draw the Game Over text
		CenterDrawFontZ(300.0f, 1, 100, DEFAULT_COLOR, "GAME OVER");

		// Finally, sort our poor beleagured hero... or loser since it's game over
		SortSprite(&herder[gHumanPlayer].spr);

		pvr_list_finish();

		// draw it
		pvr_scene_end;

		// Wait for the music to end and all sheep to leave before exitting
		if ((nSheepLeft == 0) && (!musicisplaying())) {
			break;
		}
	}

	// quick fade out
	for (gGfxDarken=0; gGfxDarken<255; gGfxDarken+=8) {
		BeginScene();
		
		// Opaque polygons
		pvr_list_begin(PVR_LIST_OP_POLY);
		pvr_list_finish();
		
		// transparent polygons
		pvr_list_begin(PVR_LIST_TR_POLY);

		// Draw the Game Over text
		CenterDrawFontZ(300.0f, 1, 100, DEFAULT_COLOR, "GAME OVER");

		// Finally, sort our poor beleagured hero... or loser since it's game over
		SortSprite(&herder[gHumanPlayer].spr);

		pvr_list_finish();
		pvr_scene_end;
	}

	gGfxDarken=0;

	return 0;
}

// returns 0-3 for winner, -1 for none yet, takes gOptions.Rounds into account
int GetFinalWinner() {
	int idx, cnt; 

	// Now, the actual game is going to be gGame.Rounds wins
	cnt=gOptions.Rounds;
	debug("Looking for someone who's got %d wins\n", cnt);
	
	// To eliminate ties being obvious, check the human player first in story mode, and let him win ;)
	if (!isMultiplayer) {
		if (herder[gHumanPlayer].wins >= cnt) {
			return gHumanPlayer;
		}
	}

	// and only one player can be that far!
	for (idx=0; idx<4; idx++) {
		if ((herder[idx].type&PLAY_MASK) != PLAY_NONE) {
			debug("Herder %d has %d\n", idx, (int)herder[idx].wins);
			if (herder[idx].wins >= cnt) {
				break;
			}
		}
	}
	if (idx >= 4) {
		return -1;
	} else {
		if ((!isMultiplayer)&&(gAlwaysWinStoryMode)) {
			debug("CHEATING...\n");
			return gHumanPlayer;
		}

		return idx;
	}
}

// Main game routine
// Return 0 to continue, or 1 to exit
int Game(int inMultiplayer) {
	int x, y, idx=0, idx2, idx3;
	int speedloop, round;
	int gridx, gridy;
	int xstep=0, ystep=0;
	SPRITE spr ;
	int exitflag, miscflag=0;
	int lightning;
	uint32 currentx=0, currenty=0;
	uint32 origx=0, origy=0; 
	int nCountdown;
	int nWinner;
	int nFirstStrike;
	int nCurrentWinner, nCurrentLoser;
	char buf[64], buf2[32];
	int nStoryPhase=0;
	kthread_t * thd_hnd;

	// set the global
	isMultiplayer=inMultiplayer;

	// suppress warning, don't really need them initialized yet
	x=0; y=0;

	if (inMultiplayer != -1) {	// -1 is omake mode
		// Don't play music and read the disc at the same time :)
		// (actually, it's amazing how close it comes to working!)
		sound_stop();

		// Load default sets
		memcpy(&gOptions, &gGame.Options, sizeof(struct gameoptions));

		// now get the match ready
		if (!isMultiplayer) {
			// If we're in single player mode here, then we need to track that
			// and run the story, too. This function being so nasty already,
			// we're just going to hack it in.
			nStoryPhase=FindPhase(level);	// each level has multiple phases
			StoryModeTotalScore=0;	// start with no points yet
			gOptions.Rounds=2;
		} else {
			// it IS multiplayer
			gOptions.Rounds=gGame.Options.Rounds;
		}
	}
	
	// only used in multiplayer mode
	round=0;
	strcpy(szLastStoryCmd, "");
	// Reset the win count, too
	for (idx=0; idx<4; idx++) {
		herder[idx].wins=0;
	}

	if (isMultiplayer==0) {
		// scale out the first level information
		// need to put gfx in place, just like in demo mode
		debug("Copying stage %d to misc texture...\n", level);
		sprintf(buf, "gfx/Menu/level%d.png", level);
		load_png_block(buf, txr512_misc, 0);	// load texture
		CLEAR_IMAGE(disc_txr, 512, 512);
		CLEAR_IMAGE(txr_controlwork, 256, 256);
		CLEAR_IMAGE(txr512_levela, 512, 512);
		CLEAR_IMAGE(txr512_levelb, 512, 512);
		CLEAR_IMAGE(txr512_levelc, 512, 512);

		// scale out the intro to this stage - flag no level display (not a thread here!)
		scale_gfx(StoryIntroText[level]);
		// and wait a little longer for presentation
		thd_sleep(1000);
	}

StoryModeLoop:
	nGameFrames=0;
	thd_hnd=NULL;
	nWinner=-1;
	nFirstStrike=0;
	nCountdown=-1;
	lightning=0;
	gStoryModeSpecialFlags=0;
	
	if (!isMultiplayer) {
		// in story mode - work out what it is we need to do
		if ('\0' == szLastStoryCmd[0]) {
			// start background beat
			sound_start(SONG_STORY, 1);

			// Now loop
			while ('\0' == szLastStoryCmd[0]) {
				// do next story panel
				strcpy(szLastStoryCmd, doStory(&nStoryPhase));
StoryModeContinue:
				switch (szLastStoryCmd[0]) {
				case '\0':	break;	// loop
				
				case 'C':	
					level=szLastStoryCmd[1]-'1';
					stage=3;
					gOptions.Rounds=1;
					break;	// challenge mode Cx
				
				case 'S':	
					level=szLastStoryCmd[1]-'1';
					gOptions.Rounds=szLastStoryCmd[2]-'0';
					gOptions.Rounds/=2;
					gOptions.Rounds++;
					if (szLastStoryCmd[3]) {
						stage=szLastStoryCmd[3]-'1';
					} else {
						stage=0;
					}
					gOptions.Win=1;
					break;	// play for sheep Sx
				
				case 'P':	
					level=szLastStoryCmd[1]-'1';
					gOptions.Rounds=szLastStoryCmd[2]-'0';
					gOptions.Rounds/=2;
					gOptions.Rounds++;
					if (szLastStoryCmd[3]) {
						stage=szLastStoryCmd[3]-'1';
					} else {
						stage=0;
					}
					gOptions.Win=0;
					break;	// play for points Px

				case 'B':	
					debug("Returned B?\n"); 
					szLastStoryCmd[0]='\0';
					break;	// shouldn't happen here

				case 'Q':	
					// trigger end credits to run on exit!
					return 0xbbaa;
					break;	// end of game

				case 'E':	// end of level
					if (gIsHardStory) {
						switch (level) {
						case LEVEL_CANDY:
							ShowUnlockMessage("You've unlocked\nCANDYLAND\n- Hard mode -", 0x00200000, 1);
							break;
						case LEVEL_HAUNTED:
							ShowUnlockMessage("You've unlocked\nHAUNTED HOUSE\n- Hard mode -", 0x00400000, 1);
							break;
						case LEVEL_TOY:
							ShowUnlockMessage("You've unlocked\nTHALIA'S TOY FACTORY\n- Hard mode -", 0x00800000, 1);
							break;
						case LEVEL_DISCO:
							ShowUnlockMessage("You've unlocked\nDISCO\n- Hard mode -", 0x01000000, 1);
							break;
						case LEVEL_WATER:	// this one is never hit
							ShowUnlockMessage("You've unlocked\nSTIX WATERWORKS\n- Hard mode -", 0x02000000, 1);
							break;
						// no other levels unlock anything
						}
					} else {
						switch (level) {
						case LEVEL_CANDY:
							ShowUnlockMessage("You've unlocked\nCANDYLAND", 0x00000080, 1);
							break;
						case LEVEL_HAUNTED:
							ShowUnlockMessage("You've unlocked\nHAUNTED HOUSE", 0x000000100, 1);
							break;
						case LEVEL_TOY:
							ShowUnlockMessage("You've unlocked\nTHALIA'S TOY FACTORY", 0x000000200, 1);
							break;
						case LEVEL_DISCO:
							ShowUnlockMessage("You've unlocked\nDISCO", 0x000000400, 1);
							break;
						case LEVEL_WATER:	// this one is never hit
							ShowUnlockMessage("You've unlocked\nSTIX WATERWORKS", 0x000000800, 1);
							break;
						// no other levels unlock anything
						}
					}
					
					// update for new level
					level++;
					stage=0;

					// need to put gfx in place, just like in demo mode
					debug("Copying stage %d to misc texture...\n", level);
					sprintf(buf, "gfx/Menu/level%d.png", level);
					load_png_block(buf, txr512_misc, 0);	// load texture
					CLEAR_IMAGE(disc_txr, 512, 512);
					CLEAR_IMAGE(txr_controlwork, 256, 256);
					CLEAR_IMAGE(txr512_levela, 512, 512);
					CLEAR_IMAGE(txr512_levelb, 512, 512);
					CLEAR_IMAGE(txr512_levelc, 512, 512);

					// scale out the intro to this stage - flag no level display (not a thread here!)
					scale_gfx(StoryIntroText[level]);
					// and wait a little longer for presentation
					thd_sleep(1000);
					// and loop
					szLastStoryCmd[0]='\0';
					break;
				}

				if (gReturnToMenu) {
					sound_stop();
					return 1;
				}
			}
			if (gReturnToMenu) {
				sound_stop();
				return 1;
			}

			// fade out drums
			for (idx=gGame.MVol*28; idx>0; idx-=28) {
				set_sound_volume(idx, -1);
				thd_sleep(20);
			}
			sound_stop();
			thd_sleep(50);
			set_sound_volume(gGame.MVol*28, -1);
		}

		if (gOnlyPlayStoryMode) {
			szLastStoryCmd[0]='\0';
			goto StoryModeLoop;
		}

		debug("Going to do game mode %s\n", szLastStoryCmd);
		// else just run the next stage
	} else {
		if ((round > 0)||(-1 == isMultiplayer)) {
			// same sort of deal - we need to ensure the graphics are right for the scaling
			// need to put gfx in place, just like in demo mode
			debug("Copying stage %d to misc texture...\n", level);
			sprintf(buf, "gfx/Menu/level%d.png", level);
			load_png_block(buf, txr512_misc, 0);	// load texture
			CLEAR_IMAGE(disc_txr, 512, 512);
			CLEAR_IMAGE(txr_controlwork, 256, 256);
			CLEAR_IMAGE(txr512_levela, 512, 512);
			CLEAR_IMAGE(txr512_levelb, 512, 512);
			CLEAR_IMAGE(txr512_levelc, 512, 512);
		}
	}
 
	// Game begin
#ifdef DEMO_BUILD
	{		// always in demo build ;)
#else
	if (inDemo) {
#endif
		// in a demo build, we have not had the level select up, so we need to copy some
		// data into the right places
		debug("Copying stage %d to misc texture...\n", level);
		sprintf(buf, "gfx/Menu/level%d.png", level);
		load_png_block(buf, txr512_misc, 0);	// load texture
		CLEAR_IMAGE(disc_txr, 512, 512);
		CLEAR_IMAGE(txr_controlwork, 256, 256);
		CLEAR_IMAGE(txr512_levela, 512, 512);
		CLEAR_IMAGE(txr512_levelb, 512, 512);
		CLEAR_IMAGE(txr512_levelc, 512, 512);
		// start thread and flag demo mode (no text, but no level previews either)
		thd_hnd = thd_create(scale_gfx, "DEMO");
	} else {
		if (isMultiplayer) {
			// When a level is selected, the level previews fade out while the main tile grows 
			// to 100%. This code must happen in a separate thread outside of this function, however, so that
			// it happens *while* the level is loaded.
			if (-1 == isMultiplayer) {
				sprintf(buf2, "Omake!");
				thd_hnd = thd_create(scale_gfx, buf2);
			} else {
				if (round > 0) {
					sprintf(buf2, "Round %d", round+1);
					thd_hnd = thd_create(scale_gfx, buf2);
				} else {
					// when round is 1, we just came from the level select
					thd_hnd = thd_create(scale_gfx, NULL);
				}
			}
		} else {
			if (stage == 3) {
				sprintf(buf2, "Challenge Stage!");
			} else {
				sprintf(buf2, "Round %d", stage+1);
			}
			thd_hnd = thd_create(scale_gfx, buf2);
		}
	}

	if (gReturnToMenu) {
		if (NULL != thd_hnd) {
			thd_wait(thd_hnd);
			thd_hnd=NULL;
		}
		sound_stop();
		return 1;
	}

	// Now perform initialization
	for (idx=0; idx<4; idx++) {
		char *pherd;

		idx2=herder[idx].type;		// save the type
		idx3=herder[idx].wins;		// and the wins
		memset(&herder[idx], 0, sizeof(HerdStruct));
		herder[idx].type=idx2;
		if ((herder[idx].type&PLAY_MASK) == PLAY_HUMAN) {
			herder[idx].nIcon=(herder[idx].type&PLAY_CHAR_MASK)|0x80;	// 0x80 means it's valid, reduces racing
		} else {
			herder[idx].nIcon=0;
		}
		
		herder[idx].wins=idx3;
		if (gOptions.Powers) {
			herder[idx].speed=HERDERSPEED;
			herder[idx].range=2;		// Work around some pixel-level bugs when this is only 1
			if ((!isMultiplayer)&&(gIsHardStory)) {
				if ((herder[idx].type&PLAY_MASK) == PLAY_COMPUTER) {
					// increase the computer's speed and range in hard mode story - just by 1
					// They're also more persistent and the sheep are faster
					herder[idx].speed++;
					herder[idx].range++;
				}
			}
		} else {
			debug("Disabled powerups! Setting to full speed.\n");
			herder[idx].speed=HERDERMAXSPEED;
			herder[idx].range=MAXLIGHTNING;
			herder[idx].flags|=FLAG_POWERUP;
		}
		herder[idx].spr.txr_addr=txr_herder[idx];
		herder[idx].spr.tilenumber=21;	// static down
		herder[idx].spr.alpha=255;
		if ((herder[idx].type&PLAY_SPECIAL_MASK) == PLAY_SPECIAL_CLONE) {
			// Iskur's clones are semi-transparent and almost fully-powered up
			herder[idx].spr.alpha=192;
			herder[idx].speed=HERDERMAXSPEED-2;
			herder[idx].range=MAXLIGHTNING-2;
			herder[idx].flags|=FLAG_POWERUP;
		} else {
			if ((herder[idx].type&PLAY_SPECIAL_MASK) == PLAY_SPECIAL_POWER) {
				debug("Player %d has special powers! Setting to full speed.\n", idx);
				// Powered up players have everything
				herder[idx].speed=HERDERMAXSPEED;
				herder[idx].range=MAXLIGHTNING;
				herder[idx].flags|=FLAG_POWERUP;
			}
		}
		herder[idx].spr.x=(idx<2 ? GRIDSIZE+GRIDSIZE+PLAYFIELDXOFF : (GRIDSIZE*(LEVELXSIZE-3))+PLAYFIELDXOFF);
		herder[idx].spr.y=(idx%2==0 ? (3*GRIDSIZE)+PLAYFIELDYOFF : (GRIDSIZE*(LEVELYSIZE-3))+PLAYFIELDYOFF);
		herder[idx].spr.z=517.0;
		herder[idx].spr.xd=(idx<2 ? 1 : -1);
		herder[idx].spr.yd=0;
		herder[idx].spr.is3D=false;
		herder[idx].maxframes=6;
		herder[idx].berserker=60;
		herder[idx].lasthitby=0xff;
		herder[idx].lasthit=0xff;
		for (idx2=0; idx2<HISTORY; idx2++) {
			herder[idx].oldx[idx2]=herder[idx].spr.x;
			herder[idx].oldy[idx2]=herder[idx].spr.y;
		}
		herder[idx].oldidx=0;
		memset(herder[idx].path, 0xff, sizeof(herder[idx].path));
		herder[idx].pathIdx=0;
		herder[idx].nIdleCount=0;

		// Load the appropriate herder tileset
		if ((herder[idx].type&PLAY_MASK) != PLAY_NONE) {
			pherd=szNames[herder[idx].type&PLAY_CHAR_MASK];
			sprintf(buf, "gfx/Players/%s/%s_%s.png", pherd, szColors[idx], pherd);
			debug("Player %d loading %s -> %s\n", idx, buf, (herder[idx].type&PLAY_MASK)==PLAY_COMPUTER?"Computer":"Human");
			load_png_block(buf, txr_herder[idx], 0);
		}

		if (gReturnToMenu) {
			if (NULL != thd_hnd) {
				thd_wait(thd_hnd);
				thd_hnd=NULL;
			}
			sound_stop();
			return 1;
		}
	}
	
	// Load Hades if needed
	if (gStoryModeSpecialFlags&EFFECT_LOOKING_FOR_HADES) {
		idx=0;
		if (gHumanPlayer==0) {
			idx=1;
		}
		load_png_block("gfx/Players/wolf/bl_wolf.png", txr_herder[idx], 0);
	}

	if (gReturnToMenu) {
		if (NULL != thd_hnd) {
			thd_wait(thd_hnd);
			thd_hnd=NULL;
		}
		sound_stop();
		return 1;
	}

	// Load the desired level
	LoadLevel(level, stage);

	// Make sure the animation thread is done
	if (NULL != thd_hnd) {
		thd_wait(thd_hnd);
		thd_hnd=NULL;
	}

	// Reload the work in progress tileset
//	load_png_block("gfx/Misc/controlwork.png", txr_controlwork, PNG_NOCACHE);

	// Set the Timer 
	if (gOptions.Timer == -1) {
		// counting up for a bonus round!
		nTimeLeft=0;
	} else {
		if (gOptions.Timer == 0) {
			nTimeLeft=99;
		} else {
			nTimeLeft=gOptions.Timer;
		}
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
	initScoreBar();

	/* draw level */
	exitflag=0;
	fPaused=0;
	nGameCountdown=100;

	// sFx counts a big black triangle that sweeps across to the right
	// this is it's bottom-left point
	sFx=-480;	// Ends at +640
	while (1) {

		if (gOptions.NightMode&0x8000) {
			gGfxDarken=gOptions.NightMode&0xff;
		}

		if (DrawLevel()) {
			// scene is finished, loop (currently means we're paused)
#ifdef DEMO_BUILD
			static int nPauseTime=0;
			if (fPaused) {

				nPauseTime++;
				if (nPauseTime > MAXIMUM_PAUSE_TIME) {
					inDemo=1;
					break;
				}
			} else {
				nPauseTime=0;
			}
#endif
			if (gReturnToMenu) {
				sound_stop();
				return 1;
			}
			continue;
		}
		
		// (Transparent polygons)
		DrawScoreBar((fPaused>0), nTimeLeft);

		// Put the players into the collision buffer
		// Sheep are handled separately
		for (idx=0; idx<4; idx++) {
			if ((herder[idx].type&PLAY_MASK) == PLAY_NONE) continue;
			drawbuffer(herder[idx].spr.x, herder[idx].spr.y, TYPE_PLAY);
		}

		// quick scan for the winning/losing flags
		nCurrentWinner=0xff;
		idx2=0;
		nCurrentLoser=0xff;
		idx3=0xff;
		// Find min and max
		for (idx=0; idx<4; idx++) {
			if ((herder[idx].type&PLAY_MASK) == PLAY_NONE) continue;

			if (herder[idx].sheep > idx2) {
				idx2=herder[idx].sheep;
				nCurrentWinner=idx;
			}
			if (herder[idx].sheep < idx3) {
				idx3=herder[idx].sheep;
				nCurrentLoser=idx;
			}
		}
		// now make sure they are absolute
		for (idx=0; idx<4; idx++) {
			if ((herder[idx].type&PLAY_MASK) == PLAY_NONE) continue;

			if ((nCurrentWinner != 0xff) && (idx != nCurrentWinner)) {
				if (herder[idx].sheep == idx2) {
					// More than one current winner
					nCurrentWinner=0xff;
				}
			}
			if ((nCurrentLoser != 0xff) && (idx != nCurrentLoser)) {
				if (herder[idx].sheep == idx3) {
					// More than one current loser
					nCurrentLoser=0xff;
				}
			}
		}

		for (idx=0; idx<4; idx++) {
			uint32 oldtile;

			if ((herder[idx].type&PLAY_MASK) == PLAY_NONE) continue;

			if (herder[idx].sheep < herder[idx].maxsheep) {
				herder[idx].losttime++;
			}
			if (herder[idx].lasthittime) {
				herder[idx].lasthittime--;
			}
			if ((nCurrentWinner != 0xff)&&(nCurrentWinner != idx)) {
				herder[idx].flags|=FLAG_NOTWINNING;
			}
			if ((nCurrentLoser != 0xff)&&(nCurrentLoser != idx)) {
				herder[idx].flags|=FLAG_NOTLOSING;
			}

			// First erase self from collision buffer!
			drawbuffer(herder[idx].spr.x, herder[idx].spr.y, TYPE_NONE);

			for (speedloop=0; speedloop<(herder[idx].stuntime?4:1); speedloop++) {
				lightning=0;
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
				oldtile=herder[idx].spr.tilenumber%5;

				if (herder[idx].stuntime==0) {
					switch (herder[idx].type&PLAY_MASK) {
					case PLAY_HUMAN:	{
						cont_state_t * st;

						updateoldpos(&herder[idx]);

						if (NULL != ControllerState[idx]) {

							st = maple_dev_status(ControllerState[idx]);
							if (NULL == st) {
								// detect unplugged single player
								if ((!isMultiplayer) && (idx == gHumanPlayer)) {
									// Make sure we're paused (after we're paused we won't make it this far)
									if (fPaused == 0) {
										// When we PRESS Start, we set it to idx+2 to track the release
										fPaused=idx+2;
										// And this updates the state so 'Paused' appears
										nGameCountdown=52;
									}
								}

//								debug("maple_dev_status returned NULL!!\n");
								break;
							}

							// map analog to digital
							if (st->joyy < -JOYDEAD) st->buttons|=CONT_DPAD_UP;
							if (st->joyy > JOYDEAD) st->buttons|=CONT_DPAD_DOWN;
							if (st->joyx < -JOYDEAD) st->buttons|=CONT_DPAD_LEFT;
							if (st->joyx > JOYDEAD) st->buttons|=CONT_DPAD_RIGHT;

							if ((st->ltrig >=127) && (st->rtrig >=127)) {
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

									if (st->buttons & CONT_DPAD_UP) {
										miscflag=1;
										herder[idx].spr.tilenumber=SHEEPUP;
										if (!checkblock(origx, origy-GRIDSIZE, TYPE_WALL|TYPE_BOX)) {
											herder[idx].spr.yd=-herder[idx].speed;
										}
									}
									if (st->buttons & CONT_DPAD_DOWN) {
										miscflag=1;
										herder[idx].spr.tilenumber=SHEEPDOWN;
										if (!checkblock(origx, origy+GRIDSIZE, TYPE_WALL|TYPE_BOX)) {
											herder[idx].spr.yd=herder[idx].speed;
										}
									}
									if (st->buttons & CONT_DPAD_LEFT) {
										miscflag=1;
										herder[idx].spr.tilenumber=SHEEPLEFT;
										if (!checkblock(origx-GRIDSIZE, origy, TYPE_WALL|TYPE_BOX)) {
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
									if (st->buttons & CONT_DPAD_RIGHT) {
										miscflag=1;
										herder[idx].spr.tilenumber=SHEEPRIGHT;
										if (!checkblock(origx+GRIDSIZE, origy, TYPE_WALL|TYPE_BOX)) {
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
										st->buttons &= ~(CONT_DPAD_LEFT|CONT_DPAD_RIGHT);
									} else {
										// we were moving vertical
										st->buttons &= ~(CONT_DPAD_UP|CONT_DPAD_DOWN);
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
									if (checkblock(currentx+herder[idx].spr.xd, currenty+herder[idx].spr.yd, TYPE_WALL|TYPE_BOX)) {
										herder[idx].spr.xd=0;
										herder[idx].spr.yd=0;
									}
								}

								if (miscflag) {
									// set moved flag
									herder[idx].nIdleCount=0;
									herder[idx].flags|=FLAG_MOVED;
									// verify legal move - don't check against players here
									if (checkblock(currentx+herder[idx].spr.xd, currenty+herder[idx].spr.yd, TYPE_WALL|TYPE_BOX)) {
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
									if (st->buttons & CONT_DPAD_UP) {
										herder[idx].nIdleCount=0;
										herder[idx].flags|=FLAG_MOVED;
										miscflag=1;
										herder[idx].spr.tilenumber=SHEEPUP;
										herder[idx].spr.yd=-herder[idx].speed;
										idx2=herder[idx].desty;
										herder[idx].desty=herder[idx].originy;
										herder[idx].originy=idx2;
										AddPush(idx, gridx, gridy);
									} else {
										herder[idx].spr.tilenumber=SHEEPDOWN;
										if ((st->buttons & CONT_DPAD_DOWN)||((gStoryModeSpecialFlags&EFFECT_SHEEP_RIDE_CONVEYORS)&&(st->buttons&(CONT_DPAD_LEFT|CONT_DPAD_RIGHT)))) {
											herder[idx].nIdleCount=0;
											herder[idx].flags|=FLAG_MOVED;
											herder[idx].spr.yd=herder[idx].speed;
											AddPush(idx, gridx, gridy);
										}
									}
								}
								if ((herder[idx].spr.xd==0) && (herder[idx].spr.yd <= 0)) {
									if (st->buttons & CONT_DPAD_DOWN) {
										miscflag=1;
										herder[idx].spr.tilenumber=SHEEPDOWN;
										herder[idx].spr.yd=herder[idx].speed;
										idx2=herder[idx].desty;
										herder[idx].desty=herder[idx].originy;
										herder[idx].originy=idx2;
										AddPush(idx, gridx, gridy);
									} else {
										herder[idx].spr.tilenumber=SHEEPUP;
										if ((st->buttons & CONT_DPAD_UP)||((gStoryModeSpecialFlags&EFFECT_SHEEP_RIDE_CONVEYORS)&&(st->buttons&(CONT_DPAD_LEFT|CONT_DPAD_RIGHT)))) {
											herder[idx].nIdleCount=0;
											herder[idx].flags|=FLAG_MOVED;
											herder[idx].spr.yd=-herder[idx].speed;
											AddPush(idx, gridx, gridy);
										}
									}
								}
								if ((herder[idx].spr.xd >= 0) && (herder[idx].spr.yd==0)) {
									if (st->buttons & CONT_DPAD_LEFT) {
										miscflag=1;
										herder[idx].spr.tilenumber=SHEEPLEFT;
										herder[idx].spr.xd=-herder[idx].speed;
										idx2=herder[idx].destx;
										herder[idx].destx=herder[idx].originx;
										herder[idx].originx=idx2;
										AddPush(idx, gridx, gridy);
									} else {
										herder[idx].spr.tilenumber=SHEEPRIGHT;
										if ((st->buttons & CONT_DPAD_RIGHT)||((gStoryModeSpecialFlags&EFFECT_SHEEP_RIDE_CONVEYORS)&&(st->buttons&(CONT_DPAD_UP|CONT_DPAD_DOWN)))) {
											herder[idx].nIdleCount=0;
											herder[idx].flags|=FLAG_MOVED;
											herder[idx].spr.xd=herder[idx].speed;
											AddPush(idx, gridx, gridy);
										}
									}
								}
								if ((herder[idx].spr.xd <= 0) && (herder[idx].spr.yd==0)) {
									if (st->buttons & CONT_DPAD_RIGHT) {
										miscflag=1;
										herder[idx].spr.tilenumber=SHEEPRIGHT;
										herder[idx].spr.xd=herder[idx].speed;
										idx2=herder[idx].destx;
										herder[idx].destx=herder[idx].originx;
										herder[idx].originx=idx2;
										AddPush(idx, gridx, gridy);
									} else {
										herder[idx].spr.tilenumber=SHEEPLEFT;
										if ((st->buttons & CONT_DPAD_LEFT)||((gStoryModeSpecialFlags&EFFECT_SHEEP_RIDE_CONVEYORS)&&(st->buttons&(CONT_DPAD_UP|CONT_DPAD_DOWN)))) {
											herder[idx].nIdleCount=0;
											herder[idx].flags|=FLAG_MOVED;
											herder[idx].spr.xd=-herder[idx].speed;
											AddPush(idx, gridx, gridy);
										}
									}
								}
								if ((herder[idx].spr.xd == 0) && (herder[idx].spr.yd==0)) {
									herder[idx].spr.tilenumber=oldtile;
								}

								if (miscflag) {
									// this does mean you could uturn on a grid boundary and not get caught ;)
									herder[idx].flags|=FLAG_UTURN;
								}

								// now either way, reset it for the animation below
								if (herder[idx].nIdleCount) {
									miscflag=0;
								} else {
									miscflag=1;
								}
							}

							// always check for buttons and pause!
							if (st->buttons & (CONT_A | CONT_B | CONT_X | CONT_Y)) {
								if (herder[idx].charge>=10) {
									lightning=1;			// flag to do lightning after we move
								}
							} else {
								herder[idx].charge++;
								// Adjust for maximum range
								if (herder[idx].charge > herder[idx].range*10+9) {
									herder[idx].charge=herder[idx].range*10+9;
								}

							}

							if ((st->buttons & CONT_START) && (!(st->buttons&CONT_A))) {
								// pause
								if (fPaused==0) {
									// When we PRESS Start, we set it to idx+2 to track the release
									fPaused=idx+2;
									// And this updates the state so 'Paused' appears
									nGameCountdown=52;
								}
							} else {
								// When Start is pressed again to unpause, we set fPaused to this value
								// so that we can release the pause, when the button is released
								if (-(idx+1)==fPaused) {
									fPaused=0;
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
								herder[idx].spr.tilenumber+=HerderAnimSeq[herder[idx].animframe]*5;
							} else {
								// stationary tile
								herder[idx].spr.tilenumber=oldtile+20;	// set to idle frame
							}
							break;
						} else {
							debug("Controller status returned NULL\n");
						}
					}

					case PLAY_COMPUTER:		/* computer - no controller */
						// Computer never stands still
						herder[idx].flags|=FLAG_MOVED;

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
										herder[idx].c_attacks++;
									} else if (LevelData[gridy+1][gridx].nDestructible == 1) {
										LevelData[gridy+1][gridx].nDestructible=2;
										herder[idx].c_attacks++;
									} else if (LevelData[gridy][gridx-1].nDestructible == 1) {
										LevelData[gridy][gridx-1].nDestructible=2;
										herder[idx].c_attacks++;
									} else if (LevelData[gridy][gridx+1].nDestructible == 1) {
										LevelData[gridy][gridx+1].nDestructible=2;
										herder[idx].c_attacks++;
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
									int sh=(herder[idx].CPUStatus&0x7f00)>>8;
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
									int shx=herder[idx].CPUStatus>>8;
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
								int sh=ch_rand()%MAX_SHEEP;
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
										herder[idx].CPUStatus=xx<<8;		// upper byte is sheep # or player #
										break;
									}
								}
								if (fl == 0) {
									// crap - no such sheep. Pick on a player, then?
									if (gOptions.Skill != 0) {		// only if CPU skill is not set to 'lost'
										sh=idx;
										while (sh == idx) {
											sh=ch_rand()%5+(2-gOptions.Skill);	// Chance of not chasing a player, we need some random
										}									// to break out of deadlocks. Also more random at weaker skills
										if (sh < 4) {
											while (sh != idx) {
												// prefer human targets ;)
												if ((herder[sh].type & PLAY_MASK) == PLAY_HUMAN) break;
												sh++;
												if (sh > 3) sh=0;
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
										herder[idx].destx=(ch_rand()%20) * GRIDSIZE;
										herder[idx].desty=(ch_rand()%15) * GRIDSIZE;
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
										herder[idx].CPUStatus=0x8000 | (sh<<8);	// player number with high bit set
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
							if (checkblock(currentx+herder[idx].spr.xd, currenty+herder[idx].spr.yd, TYPE_WALL|TYPE_BOX)) {
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
							herder[idx].spr.tilenumber+=HerderAnimSeq[herder[idx].animframe]*5;
						} else {
							// stationary tile
							herder[idx].spr.tilenumber=oldtile+20;
						}

						if ((currentx == origx) && (currenty == origy)) {
							// cache the value 
							herder[idx].originx=currentx;
							herder[idx].originy=currenty;
						}

						// check for u-turn
						if ((oldtile==SHEEPUP)&&(herder[idx].spr.tilenumber==SHEEPDOWN)) herder[idx].flags|=FLAG_UTURN;
						if ((oldtile==SHEEPDOWN)&&(herder[idx].spr.tilenumber==SHEEPUP)) herder[idx].flags|=FLAG_UTURN;
						if ((oldtile==SHEEPLEFT)&&(herder[idx].spr.tilenumber==SHEEPRIGHT)) herder[idx].flags|=FLAG_UTURN;
						if ((oldtile==SHEEPRIGHT)&&(herder[idx].spr.tilenumber==SHEEPLEFT)) herder[idx].flags|=FLAG_UTURN;
					
						// Lightning
						if ((ch_rand()%10>8)&&(herder[idx].charge>10)) {
							lightning=1;
						} else {
							herder[idx].charge++;
							if (herder[idx].charge > herder[idx].range*10+9) {
								herder[idx].charge=herder[idx].range*10+9;
							}
						}
						break;

					default:	/* no action - probably an error */
						break;
					}
				} else {
					// stunned (no conveyer belt in here)
					// we can super cheat and move 2 pixels at a time, cause we loop 8 times now!
					updateoldpos(&herder[idx]);		// we still do this to keep sheep attached! (this will bunch up the sheep)
					herder[idx].stuntime--;
					
					// calculate movement
					herder[idx].spr.xd=herder[idx].destx-currentx;
					herder[idx].spr.yd=herder[idx].desty-currenty;

					// stunned 2 pix per frame to make offsets reliable (unless on odd offset already)
					if (herder[idx].spr.xd>1) {
						if (herder[idx].spr.x&1) {
							herder[idx].spr.xd=2;
						} else {
							herder[idx].spr.xd=4;						
						}
					}
					if (herder[idx].spr.xd<-1) {
						if (herder[idx].spr.x&1) {
							herder[idx].spr.xd=-2;
						} else {
							herder[idx].spr.xd=-4;
						}
					}
					if (herder[idx].spr.yd>1) {
						if (herder[idx].spr.y&1) {
							herder[idx].spr.yd=2;
						} else {
							herder[idx].spr.yd=4;
						}
					}
					if (herder[idx].spr.yd<-1) {
						if (herder[idx].spr.y&1) {
							herder[idx].spr.yd=-2;
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
						}
						herder[idx].originx=origx;
						herder[idx].originy=origy;
					}

					// check - if we bump into something, or stun expires, we need to disable the stun 
					// and just reset to the last valid square (nobody should be able to keep up with us)
					if ((herder[idx].stuntime <= 0) || (checkblock(currentx+herder[idx].spr.xd, currenty+herder[idx].spr.yd, TYPE_WALL|TYPE_BOX|TYPE_PLAY))) {
						herder[idx].stuntime=0;
						herder[idx].spr.xd=0;
						herder[idx].spr.yd=0;
						// retarget a computer
						if ((herder[idx].type&PLAY_MASK) == PLAY_COMPUTER) {
#ifdef PATHFIND_DEBUG
							debug("Herder %d was stunned - retargetting.\n", idx);
#endif
							herder[idx].pathIdx=100; // off the end
						}
						// make sure we're on a legal grid position now
						if (checkblock(currentx, currenty, TYPE_WALL|TYPE_BOX|TYPE_PLAY)) {
							// just force us back to the last valid position then
							herder[idx].spr.x=herder[idx].originx;
							herder[idx].spr.y=herder[idx].originy;
						}
					}

#ifdef PATHFIND_DEBUG
					debug("Herder %d - stuntime %d, speed=%d,%d\n", idx, herder[idx].stuntime, herder[idx].spr.xd, herder[idx].spr.yd);
#endif
				}

				// If we aren't moving, then make sure we're grid aligned
				if ((herder[idx].spr.xd==0) && (herder[idx].spr.yd==0)) {
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
					if (!checkblock(xstep, herder[idx].spr.y, TYPE_PLAY)) {
						// no player, everything else should be okay, so we can move
						herder[idx].spr.x=xstep;
					}
				}
				if (herder[idx].spr.xd < 0) {
					xstep=herder[idx].spr.x+((herder[idx].spr.xd-(nFrames&1))/2);
					if (xstep < herder[idx].destx) {
						xstep=herder[idx].destx;
					}
					if (!checkblock(xstep, herder[idx].spr.y, TYPE_PLAY)) {
						// no player, everything else should be okay, so we can move
						herder[idx].spr.x=xstep;
					}
				}

				if (herder[idx].spr.yd > 0) {
					ystep=herder[idx].spr.y+((herder[idx].spr.yd+(nFrames&1))/2);
					if (ystep > herder[idx].desty) {
						ystep=herder[idx].desty;
					}
					if (!checkblock(herder[idx].spr.x, ystep, TYPE_PLAY)) {
						// no player, everything else should be okay, so we can move
						herder[idx].spr.y=ystep;
					}
				}
				if (herder[idx].spr.yd < 0) {
					ystep=herder[idx].spr.y+((herder[idx].spr.yd-(nFrames&1))/2);
					if (ystep < herder[idx].desty) {
						ystep=herder[idx].desty;
					}
					if (!checkblock(herder[idx].spr.x, ystep, TYPE_PLAY)) {
						// no player, everything else should be okay, so we can move
						herder[idx].spr.y=ystep;
					}
				}

				// Now recalculate a few values for the lightning code
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
				oldtile=herder[idx].spr.tilenumber%5;
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
					herder[idx].range++;
					if (herder[idx].range > MAXLIGHTNING) {
						herder[idx].range=MAXLIGHTNING;
						herder[idx].score+=50;
					}
					herder[idx].flags|=FLAG_POWERUP;
					herder[idx].powerups++;
					herder[idx].score+=50;
				}

				if (LevelData[gridy][gridx].nPowerup==2) {
					// speed powerup
					LevelData[gridy][gridx].nPowerup=0;
					herder[idx].speed++;
					if (herder[idx].speed > HERDERMAXSPEED) {
						herder[idx].speed=HERDERMAXSPEED;
						herder[idx].score+=50;
					}
					herder[idx].flags|=FLAG_POWERUP;
					herder[idx].powerups++;
					herder[idx].score+=50;
				}
				
				if (gStoryModeSpecialFlags&EFFECT_LOOKING_FOR_HADES) {
					if (LevelData[gridy][gridx].nPowerup==4) {
						// found Hades
						LevelData[gridy][gridx].nPowerup=0;
						herder[idx].flags|=FLAG_CHALLENGE_COMPLETE;
					}
				}

				SortSprite(&herder[idx].spr);

				if (gOptions.NightMode&0x8000) {
					SortLight(NIGHT_Z, herder[idx].spr.x-NIGHT_TL, herder[idx].spr.y-NIGHT_TL, herder[idx].spr.x+NIGHT_BR, herder[idx].spr.y+NIGHT_BR, NIGHT_COLOR);
				}

				spr.txr_addr=txr_sheep;
				spr.is3D=false;
				spr.z=514.0f;
				x=herder[idx].oldidx-4;
				if (x<0) x+=HISTORY;
				if (x>0) {
					y=x-1;
				} else {
					y=HISTORY-1;
				}
				for (idx2=0; idx2<min(HISTORY/4,herder[idx].sheep); idx2++) {
					spr.tilenumber=SHEEPUP;
					if (herder[idx].oldy[y]<herder[idx].oldy[x]) spr.tilenumber=SHEEPDOWN;
					if (herder[idx].oldx[y]>herder[idx].oldx[x]) spr.tilenumber=SHEEPLEFT;
					if (herder[idx].oldx[y]<herder[idx].oldx[x]) spr.tilenumber=SHEEPRIGHT;
					spr.x=herder[idx].oldx[x];
					spr.y=herder[idx].oldy[x];
					spr.tilenumber=SheepAnimationFrame[spr.tilenumber][(nGameFrames>>2)%SHEEPFRAMES];
					if (fGhostSheep) {
						spr.alpha=128;
					} else {
						spr.alpha=255;
					}
					SortSprite(&spr);
					x-=4;
					if (x<0) x+=HISTORY;
					if (x>0) {
						y=x-1;
					} else {
						y=HISTORY-1;
					}
				}

				if (!lightning) {
					if (herder[idx].berserker) {
						herder[idx].berserker--;
					}
				}

				miscflag=(nGameFrames>>2)&0x01;	// every 4 frames
				while (lightning) {		// this is a while instead of an IF so we can break early
					if ((xstep==0)&&(ystep==0)) {
						// no action if no direction
						break;
					}

					// Fire lightning in the current direction
					// Lightning goes until it hits a wall, not affected by players or sheep
					x=currentx+1;
					y=currenty+1;

					spr.z=515.2f;
					spr.alpha=229;
					spr.is3D=false;
					spr.txr_addr=txr_sprites;
					switch (oldtile) {
					case SHEEPUP:	spr.tilenumber=15+miscflag; break;
					case SHEEPDOWN:	spr.tilenumber=(15+miscflag)|SPR_VFLIP; break;
					case SHEEPLEFT: spr.tilenumber=10+miscflag; break;
					case SHEEPRIGHT:spr.tilenumber=(10+miscflag)|SPR_HFLIP; break;
					}

					for (idx2=herder[idx].charge/10; idx2>0; idx2--) {
						int tmpx, tmpy;

						x+=xstep;
						y+=ystep;
						
						if (idx2==1) {
							// use the lightning tip sprite instead
							spr.tilenumber+=2;
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
						SortSprite(&spr);

						// Fading destructible wall - stop on it
						if (LevelData[tmpy][tmpx].nDestructible > 1) {
							break;
						}

						// Check against destructible walls, and nuke them :) (and stop the beam)
						if (LevelData[tmpy][tmpx].nDestructible == 1) {
							LevelData[tmpy][tmpx].nDestructible=2;
							herder[idx].c_attacks++;
							herder[idx].score+=10;
							break;
						}

						if (!(gStoryModeSpecialFlags&EFFECT_SHEEP_IGNORE_LIGHTNING)) {
							// Check for stunned sheep
							for (idx3=0; idx3<MAX_SHEEP; idx3++) {
								int x,y;
								if (sheep[idx3].stuntime > 0) {
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
										// got this sheep :)
										herder[idx].score+=10;
										sheep[idx3].stuntime=60;	// 1 second stun time
										if ((!sndCountdown)&&(nCountdown==-1)) {
											sound_effect(snd_sheep[ZAPPEDA+ch_rand()%3], SHEEPVOL);
											sndCountdown=SNDDELAY;
										}
										herder[idx].flags|=FLAG_ATTACKED;
										herder[idx].s_attacks++;
									}
								}
							}
						}

						// How about other herders?
						for (idx3=0; idx3<4; idx3++) {
							int x,y;
							if ((idx3!=idx)&&((herder[idx3].type&PLAY_MASK)!=PLAY_NONE)) {	// don't check ourselves or non-players
								// skip if already stunned
								if (herder[idx3].stuntime>0) continue;
								// else, check for collision
								x=herder[idx3].spr.x;
								x-=spr.x;
								y=herder[idx3].spr.y;
								y-=spr.y;
								if ((abs(x)<28)&&(abs(y)<28)) {
									int tmpX,tmpY;
									// got this guy :)
									herder[idx].score+=10;
									herder[idx].flags|=FLAG_ATTACKED;
									herder[idx].h_attacks++;
									herder[idx3].stuntime=15*2;	// 1/8 second stun time
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
									}
									// last chance - try to knock back if moving
									// This is a bug - it's checking the current and orig pos of the person who
									// is FIRING the lightning. But it works well enough so I'm going to leave it.
									if ((herder[idx3].stuntime == 0) && ((currentx != origx)||(currenty!=origy))) {
										herder[idx3].destx=herder[idx3].originx;
										herder[idx3].desty=herder[idx3].originy;
										herder[idx3].stuntime=15*2;	// 1/8 second stun time
									}
									herder[idx3].zapped++;
									herder[idx3].flags|=FLAG_VICTIM;
									if (!nFirstStrike) {
										nFirstStrike=1;
										herder[idx].flags|=FLAG_FIRSTSTRIKE;
									}
									if (idx != herder[idx3].lasthitby) {
										if (herder[idx3].lasthittime) {
											herder[idx3].punchingbag++;
										}
										herder[idx3].lasthitby=idx;
										herder[idx3].lasthittime=60;
									}
									if (herder[idx].lasthit == 0xff) {
										herder[idx].lasthit = idx3;
										herder[idx].flags|=FLAG_STALKER;
									} else {
										if (idx3 != herder[idx].lasthit) {
											herder[idx].lasthit&=~FLAG_STALKER;
										}
									}
									if ((herder[idx].lasthitby == idx3) && (herder[idx].lasthittime)) {
										herder[idx].flags|=FLAG_REVENGE;
									}

									// lose a sheep, if he has one, and the level isn't already over
									if ((herder[idx3].sheep>0)&&(nCountdown==-1)) {
										int d1, d2;
										d1=oldtile;
										d2=herder[idx3].spr.tilenumber%5;
										if (d1==d2) {
											herder[idx].backstab++;
										}

										if (herder[idx3].sheep == herder[idx3].maxsheep) {
											// Increment number of times fell below max
											herder[idx3].lostcount++;
										}
										
										herder[idx3].sheep--;
										herder[idx3].sheeplost++;
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
									// exit the loop and break the lightning
									idx2=0;
									break;
								}
							}
						}
					}

					// countdown the charge
					herder[idx].charge--;
					herder[idx].totalzap++;
					// reset berserker, unless it's expired
					if (herder[idx].berserker) {
						herder[idx].berserker=60;
					}

					break;		// this must be here to exit the lightning while! (no looping)
				}	// while
			}	// speed loop
			// put this player into the buffer only once per frame
			drawbuffer(currentx, currenty, TYPE_PLAY);
		}	// idx loop

		if (gStoryModeSpecialFlags&EFFECT_SHEEP_RIDE_CONVEYORS) {
			moveSheepConveyor();
		} else {
			moveSheep();
		}

		// Check to see if any sheep were captured, and
		// check for end of level 
		miscflag=0;	
		if (-1 == nCountdown) {
			for (idx=0; idx<MAX_SHEEP; idx++) {
				if (sheep[idx].type>0) {
					miscflag=1;
					if (!sheep[idx].invincible) {
						for (idx2=0; idx2<4; idx2++) {
							if ((herder[idx2].type&PLAY_MASK) == PLAY_NONE) continue;

							if (herder[idx2].stuntime==0) {		// no sleepy herders
								x=(int)sheep[idx].spr.x;
								x-=herder[idx2].spr.x;
								if (x<0) x=x*(-1);
								y=(int)sheep[idx].spr.y;
								y-=herder[idx2].spr.y;
								if (y<0) y=y*(-1);
								if ((x<28)&&(y<28)) {
									if (gStoryModeSpecialFlags&EFFECT_SHEEP_STUN_PLAYER) {
										if (herder[idx2].stuntime<1) {
											// got BY this sheep
											int tmpX,tmpY;
											herder[idx2].stuntime=15*2;	// 1/8 second stun time
											// find a destination grid location
											tmpX=((herder[idx2].spr.x-PLAYFIELDXOFF)+GRIDSIZE/2)/GRIDSIZE;
											tmpY=((herder[idx2].spr.y-PLAYFIELDYOFF)+GRIDSIZE/2)/GRIDSIZE;
											if (LevelData[tmpY][tmpX].isPassable) {
												tmpX-=xstep/GRIDSIZE;
												tmpY-=ystep/GRIDSIZE;
												if (LevelData[tmpY][tmpX].isPassable) {
													tmpX-=xstep/GRIDSIZE;
													tmpY-=ystep/GRIDSIZE;
													if (!LevelData[tmpY][tmpX].isPassable) {
														// just back up one, we can't slide two units back
														tmpX+=xstep/GRIDSIZE;
														tmpY+=ystep/GRIDSIZE;
													}
												} else {
													// if we can't slide even one unit, then we just don't worry about it
													herder[idx2].stuntime=0;
												}
												if (herder[idx2].stuntime) {
													herder[idx2].destx=tmpX*GRIDSIZE+PLAYFIELDXOFF;
													herder[idx2].desty=tmpY*GRIDSIZE+PLAYFIELDYOFF;
												}
											} else {
												// not sure how this happened - never mind the stun part
												herder[idx2].stuntime=0;
											}
											if (herder[idx2].stuntime == 0) {
												herder[idx2].destx=herder[idx2].originx;
												herder[idx2].desty=herder[idx2].originy;
												herder[idx2].stuntime=15*2;	// 1/8 second stun time
											}

											herder[idx2].zapped++;
											herder[idx2].flags|=FLAG_VICTIM;

											if (!sndCountdown) {
												sound_effect(snd_sheep[CAUGHT], SHEEPVOL);
												sndCountdown=SNDDELAY;
											}

											// set the sheep invincible so we can both get away
											sheep[idx].invincible=1;
											sheep[idx].type=2;
											sheep[idx].range=30;
										}
									} else {
										// got this sheep :)
										sheep[idx].type=0;
										herder[idx2].sheep++;
										if (gStoryModeSpecialFlags&EFFECT_SHEEP_RIDE_CONVEYORS) {
											if (herder[idx2].sheep&1) {
												SheepSpeed++;
												if ((SheepSpeed&3)==0) {
													if (nBgSpeedA > 1) {
														nBgSpeedA--;
													}
													if (nBgSpeedB > 1) {
														nBgSpeedB--;
													}
												}
											}
										}
										for (idx3=0; idx3<4; idx3++) {
											if ((herder[idx3].type&PLAY_MASK)==PLAY_COMPUTER) {
												int n;
												// got a sheep, everyone check their target
												n=herder[idx3].CPUStatus>>8;
												if ((n&0x80)==0) {
													if (sheep[n].type == 0) {
#ifdef PATHFIND_DEBUG
														debug("Herder %d sheep was captured by %d - retargetting.\n", idx3, idx2);
#endif
														herder[idx3].pathIdx=100;
													}
												}
											}
										}
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
										if (!sndCountdown) {
											sound_effect(snd_sheep[CAUGHT], SHEEPVOL);
											sndCountdown=SNDDELAY;
										}
										break;
									}
								}
							}
						}
					}
				}
			}
		}
		
		// flag override if looking for/found Hades
		if (gStoryModeSpecialFlags&EFFECT_LOOKING_FOR_HADES) {
			if (herder[gHumanPlayer].flags&FLAG_CHALLENGE_COMPLETE) {
				miscflag=0;
			} else {
				miscflag=1;
			}
		}
		// Also if End_if_zapped is on (we can just check the stuntime)
		if (gStoryModeSpecialFlags&EFFECT_END_IF_ZAPPED) {
			if (herder[gHumanPlayer].stuntime > 0) {
				miscflag=0;
			} else {
				miscflag=1;
			}
		}
		// Finally, if it's the conveyor belts, we need to override that too
		if (gStoryModeSpecialFlags&EFFECT_SHEEP_RIDE_CONVEYORS){
			if (herder[gHumanPlayer].flags&FLAG_CHALLENGE_COMPLETE) {
				miscflag=0;
			} else {
				miscflag=1;
			}
		}

		// a miscflag of 1 means to continue, a miscflag of 0 means to end the level

		if (-1!=nCountdown) {
			addPage(txr_winner[nWinner-1], 0, 0, 255, 255);
			if (nCountdown > 0) {
				nCountdown--;
			}
		}

		pvr_list_finish();
		pvr_scene_end;
		
		if (gOptions.NightMode&0x8000) {
			gGfxDarken=0;
		}

		if (gReturnToMenu) {
			// exit game
			sound_stop();
			return 1;
		}

		if (nGameFrames % 60 == 0) {
			if ((gOptions.Timer==-1) && (nCountdown==-1)) {
				if (nTimeLeft < 99) {
					nTimeLeft++;
					if (szLastStoryCmd[0] == 'C') {
						if (gStoryModeSpecialFlags&EFFECT_LOOKING_FOR_HADES) {
							// then we need a max time there...
							if (nTimeLeft > atoi(&szLastStoryCmd[2])) {
								miscflag=0;		// end the level
							}
						} else {
							if (((nTimeLeft&0x03) == 0) && (gStoryModeSpecialFlags&EFFECT_GRADUAL_SPEEDUP)) {
								idx=0;
								if (idx==gHumanPlayer) idx++;
								if (herder[idx].speed < HERDERMAXSPEED) {
									herder[idx].speed++;
								}
								idx++; if (idx==gHumanPlayer) idx++;
								if (herder[idx].speed < HERDERMAXSPEED) {
									herder[idx].speed++;
								}
								idx++; if (idx==gHumanPlayer) idx++;
								if (herder[idx].speed < HERDERMAXSPEED) {
									herder[idx].speed++;
								}
							}
							if (nTimeLeft >= 99) {
								miscflag=0;
							}
						}
					}
				}
			} else if ((gOptions.Timer > 0) && (nTimeLeft > 0) && (nCountdown==-1)) {
				nTimeLeft--;
#ifdef DEMO_BUILD
				if (nTimeLeft <= gOptions.Timer-5) {	// 5 seconds in
					// Check for any unused human players, and computerize them
					for (idx=0; idx<4; idx++) {
						if ((herder[idx].type&PLAY_MASK) == PLAY_NONE) continue;

						if (!(herder[idx].flags&FLAG_MOVED)) {
							herder[idx].type|=PLAY_COMPUTER;
						}
					}
				}
#endif
				if (nTimeLeft == 0) {
					miscflag=0;		// level has ended
				}
			}
		}

		if (sndCountdown) sndCountdown--;

		if ((nCountdown==-1)&&(miscflag==0)) {
			// do level over!
			// Who won?
			x=0;
			for (idx=0; idx<4; idx++) {
				if ((herder[idx].type&PLAY_MASK) == PLAY_NONE) continue;

				if (herder[idx].sheep>x) {
					y=idx;
					x=herder[idx].sheep;
				}
			}
			// There may be no clear winner!
			for (idx=0; idx<4; idx++) {
				if ((herder[idx].type&PLAY_MASK) == PLAY_NONE) continue;

				if (idx == y) continue;
				if (herder[idx].sheep == herder[y].sheep) {
					// There's a tie, so no clear first place
					y=-2;
					break;
				}
			}
			nWinner=y+1;		// this is winner 1-4 (this is a pure sheep count)
			nCountdown=0;		// prepare to exit

			if (gOptions.NightMode&0x8000) {
				gGfxDarken=gOptions.NightMode&0xff;
			}

			nWinner=HandleBonuses(nWinner, nTimeLeft);	// this returns real winner 0-3, or -1
														// Not the same as GetFinalWinner(), which
														// only returns a winner if all rounds are complete
			if (gReturnToMenu) {
				sound_stop();
				return 1;
			}

			if (szLastStoryCmd[0] == 'C') {
				idx=nWinner;
				goto failure_test;
			}
		}
		// done the bonus tallying

		if (inDemo) {
			if (isStartPressed()) {
				exitflag = 1;
			}
		}

		if ((nCountdown==0)&&(!musicisplaying())) {
			// end of a cycle, wait for end of music
			break;
		}

		if (exitflag==-1) {
#ifdef DEMO_BUILD
			if (inDemo) DoSplashes();
#endif
			inDemo=0;
			return 1;
		}
		if (exitflag==1) {
			break;
		}
	}

	if (!isMultiplayer) {
		stage++;
		if (szLastStoryCmd[0] != 'C') {
			if (herder[gHumanPlayer].wins == 1) {
				// completed first stage, this is only valid for Iskur
				if ((level == LEVEL_WATER)&&(stage==1)) {
					ShowUnlockMessage("You've unlocked\nISKUR\nalt for basic herder", 0x00000002, 1);
				}
			}
			// check the current win status so we know whether to move on
			idx=GetFinalWinner();
		} else {
			// Check if the human won the challenge
			// See if the human won the challenge
			idx=nWinner;

			// Test minimum requirements to unlock characters
			if (idx == gHumanPlayer) {
				switch (level) {
					case LEVEL_HAUNTED:
						if (nTimeLeft < 10) {
							ShowUnlockMessage("You've unlocked\nHADES WOLF\nalt for NH-5", 0x00000008, 1);
						}
						break;
					case LEVEL_TOY:
						if (herder[gHumanPlayer].sheep >= 20) {
							ShowUnlockMessage("You've unlocked\nTHALIA\nalt for Zeus", 0x00000001, 1);
						}
						break;
					case LEVEL_DISCO:
						if (nTimeLeft >= 10) {
							ShowUnlockMessage("You've unlocked\nGOD OF DANCE\nalt for dancer", 0x00000010, 1);
						}
						break;
				}
			}
		}

		// if less than 3 stages and no winner yet (but ignore stage 4 as that's incremented from challenge stage)
		if ((idx==-1)&&(stage != 3)) {
			goto StoryModeLoop;
		}

failure_test:
		// handle cheats
		if ((!isMultiplayer)&&(gAlwaysWinChallenges)&&(szLastStoryCmd[0]=='C')) {
			idx=gHumanPlayer;
		}
		// if we weren't the winner after all that, then game over
		if (idx != gHumanPlayer) {
			if (doGameOver()) {
				// Player is using up a continue!
				// Reset the win count, too
				for (idx=0; idx<4; idx++) {
					herder[idx].wins=0;
				}
				// some vars we missed resetting
				// Note that we don't mess with the story mode flags! :)
				nGameFrames=0;
				thd_hnd=NULL;
				nWinner=-1;
				nFirstStrike=0;
				nCountdown=-1;
				lightning=0;

				debug("Copying stage %d to misc texture...\n", level);
				sprintf(buf, "gfx/Menu/level%d.png", level);
				load_png_block(buf, txr512_misc, 0);	// load texture
				CLEAR_IMAGE(disc_txr, 512, 512);
				CLEAR_IMAGE(txr_controlwork, 256, 256);
				CLEAR_IMAGE(txr512_levela, 512, 512);
				CLEAR_IMAGE(txr512_levelb, 512, 512);
				CLEAR_IMAGE(txr512_levelc, 512, 512);
				
				goto StoryModeContinue;
			}

			if (gReturnToMenu) {
				return 1;
			}
			herder[gHumanPlayer].score=StoryModeTotalScore;
			AddHighScore(gHumanPlayer);
			return 0;
		}

		// Reset the win count, too
		for (idx=0; idx<4; idx++) {
			herder[idx].wins=0;
		}

		// before we go any further, this special code unlocks the waterworks
		// after you beat the points match with Iskur
		if (!strcmp("P63", szLastStoryCmd)) {
			// we've clearly beat waterworks
			if (gIsHardStory) {
				ShowUnlockMessage("You've unlocked\nSTIX WATERWORKS\n- Hard mode -", 0x02000000, 1);	
			} else {
				ShowUnlockMessage("You've unlocked\nSTIX WATERWORKS", 0x000000800, 1);	
			}
		}

		// set the command so the code above
		// properly detects the next level
		szLastStoryCmd[0]='\0';

		if (gReturnToMenu) {
			sound_stop();
			return 1;
		}

		goto StoryModeLoop;
	} else {
		// it's a multiplayer game, honor gRounds
		round++;
		idx=GetFinalWinner();	// returns 0-3 for winner, 4 or more for none yet

		if ((inDemo)||(gReturnToMenu)) {
			sound_stop();
			return 1;
		}

		if (idx == -1) {
			goto StoryModeLoop;
		}

		debug("Herder %d has the victory!\n", idx);

	}

#ifdef DEMO_BUILD
	if (inDemo) DoSplashes();
#endif
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

