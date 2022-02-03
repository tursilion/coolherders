/****************************************/
/* Cool Herders                         */
/* A demented muse by Brendan Wiese     */
/* http://starbow.org                   */
/* Programmed by Mike Brent             */
/* http://harmlesslion.com              */
/* Copyright 2000-2005 Brendan Wiese &  */
/* Mike Brent.                          */
/* cool.h                               */
/****************************************/

/* Ported to KOS 1.1.x by Dan Potter */
/* Thanks Dan! */
#ifndef FILE_COOL_H
#define FILE_COOL_H
#endif

// Uncomment this to use the PC filesystem. To use CD, comment it out and
// use 'make elf' to make a raw elf and convert it to a scrambled 1ST_READ.BIN
// Be sure to 'make clean' when you change it :)
//#define USE_PC

// To make a demo version, uncomment this. The demo version has a few extra features
// like copying the title music to a RAMdisk, running a short slideshow, and timing out pause
// Be sure you make clean!
//#define DEMO_BUILD

// Enable this define for a final build (turns off debugging and returns to BIOS)
//#define FINALBUILD

// Enable this define for very spammy computer pathfinding debug
//#define PATHFIND_DEBUG

// Enable this for the PC-based snapshot code to be included (use keyboard space bar)
#define ENABLE_SNAPSHOTS

// Enable this and the above for automatic snapshots every 3 mins or so
//#define ENABLE_AUTOMATIC_SNAPSHOTS

// flags for the load_png_block caching in disclaimer.c
#define PNG_RETURN_RGB 0x01
#define PNG_NOCACHE    0x02
#define PNG_CACHE_LVL  0x04

// Misc other stuff
#define PROGRESSX 400
#define PROGRESSY 370

#define MAX_SHEEP 29
#define HISTORY 120		// max sheep * 4 (4 is the spacing of sheep in the queue)

// These are bitmasks now
#define PLAY_MASK		0xC000
#define PLAY_HUMAN		0x0000
#define PLAY_COMPUTER	0x4000
#define PLAY_NONE		0x8000
#define PLAY_UNSET		0xC000
// additional player bitmasks
#define PLAY_SPECIAL_MASK 0x0300
#define PLAY_SPECIAL_CLONE 0x0100	// iskur's clones
#define PLAY_SPECIAL_POWER 0x0200	// powered up player

// Player character
#define PLAY_CHAR_MASK 0x000f

#define HERDERMAXSPEED 6

#define MAXLIGHTNING 5

#define HERDERSPEED 3

#define JOYDEAD 64

#define NIGHT_DARKEN 224
// default color makes everything a nice shadow
#define NIGHT_COLOR DEFAULT_COLOR
#define NIGHT_Z 511.9f
#define NIGHT_TL 80
#define NIGHT_BR (NIGHT_TL+32)

#ifdef DEMO_BUILD
// Time in ticks (50fps)
#define MAXIMUM_PAUSE_TIME 1500
#endif

// these are used to make the WIN32 version work
#ifdef WIN32
extern pvr_ptr_t txr512_levela, txr512_levelb, txr512_levelc, txr512_misc;
#define PVR_MEMCPY(to,from,size) to=from
#else
// fake 512x512 textures alias to the others
#define txr512_levela txr_levela[0]
#define txr512_levelb txr_levelb[0]
#define txr512_levelc txr_levelc[0]
#define txr512_misc txr_misc[0]
#define PVR_MEMCPY(to,from,size) memcpy(to,from,size)
#endif

// Helper
#ifndef abs
#define abs(x) ((x)<0 ? -(x) : (x))
#endif
#ifndef true
#define true (1)
#endif
#ifndef false
#define false (0)
#endif

// Centering the playfield
#define PLAYFIELDXOFF (-16)	
#define PLAYFIELDYOFF (-16)

// This structure is used for herders and sheep alike (not all fields apply to sheep)
typedef struct _herder{
	SPRITE spr;				// Sprite
	int32 destx, desty;		// Destination - used for computer play
	int32 originx,originy;	// origin - 1 axis only, used for player movement u-turns
	int32 oldx[HISTORY+1];
	int32 oldy[HISTORY+1];	// Trails, and used by the computer to detect being stuck
	int32 oldidx;			// position in the trail
	int32 type;				// Player type - bitmasked now - see the TYPE_xxx defines
	int32 score;			// Score
	int32 wins;				// Number of wins

	// Path for each CPU, x,y; x,y; x,y; etc
	char path[50*2];
	int pathIdx;

	// vmu icons
	int nIcon;

	// scoring stuff
	uint32 CPUStatus;	// Used in the CPU 'intelligence'
	int    nIdleCount;	// used by CPU and players
	uint32 stuntime;	// if Stunned, for how much longer?
	uint32 animframe;	// animation frame counter
	uint32 maxframes;	// number of animation frames
	uint32 sheep;		// How many sheep
	uint32 maxsheep;	// Maximum sheep ever captured
	uint32 recaptured;	// Maximum sheep recaptured
	uint32 range;		// range on the electric staff - for sheep, invincibility countdown
	uint32 maxrange;	// used to charge up the staff
	int32  speed;		// how fast do they move?
	uint32 charge;		// charge level (10 units = 1 range)
	uint32 lostcount;	// how many times we fell below max
	uint32 losttime;	// total time spent below max (divide to get average time below max)
	uint32 zapped;		// how many times we were zapped
	uint32 backstab;	// how many rear attacks were made
	uint32 totalzap;	// total times fired
	uint32 berserker;	// countdown timer for non-firing point
	uint32 flags;		// various bitflags for scoring
	uint32 sheeplost;	// number of sheep lost (including relost)
	uint32 idletime;	// number of frames not moving
	uint32 lasthitby;	// herder who last zapped us
	uint32 lasthittime;	// time since last hit
	uint32 punchingbag;	// times hit as a punching bag (hit, then hit again by someone else)
	uint32 lasthit;		// Last player we hit
	uint32 powerups;	// Number of powerups collected
	uint32 h_attacks;	// Number of times attacked herders
	uint32 s_attacks;	// Number of times attacked sheep
	uint32 c_attacks;	// Number of times attacked crates
} HerdStruct, *pHerdStruct;

extern pvr_ptr_t txr_sprites;
extern HerdStruct herder[4];

void GoByeBye(uint8 addr, uint32 btns);
int  main(int argc, char *argv[]);
void AddPicture(pvr_ptr_t txr_addr, float z);
int GetFinalWinner();
int  Game(int isMultiplayer);
void updateoldpos(pHerdStruct);
char *load_png_block_mask(char *fn, pvr_ptr_t whereto, int fSave, unsigned int mask);
int  isStartPressed();
void debug(char *str, ...);
void ShowLoading();
void ShowBlack();
int GetRandomLevel();
int doGameOver();

// compatibility
#define load_png_block(fn,where,save) load_png_block_mask(fn,where,save,PNG_MASK_ALPHA)

#ifndef min
	#define min(a,b) ((a)<(b)?(a):(b))
#endif
#ifndef max
	#define max(a,b) ((a)>(b)?(a):(b))
#endif


