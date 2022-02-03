/****************************************/
/* Cool Herders                         */
/* http://harmlesslion.com              */
/* Copyright 2011 HarmlessLion LLC      */
/* cool.h                               */
/****************************************/

// NOTE: FINALBUILD replaced with the NDS value: #if !defined(SDK_FINALROM)

//Cool Herders TWL-KUHE
//Header files are on the FTP!  Here's the file info:
//Filename: KUHE.exe
//Password: h3rd3r5
//Here's the GGID you requested:
//0x00401349

// Enable this define for very spammy computer pathfinding debug
//#define PATHFIND_DEBUG

// menu scroll targets - used to control scroll
// main is a bitflag to indicate we have to scroll back to main first
#define SCROLL_TARG_MAIN	0x80
#define SCROLL_TARG_NORTH	1
#define SCROLL_TARG_EAST	2
#define SCROLL_TARG_SOUTH	3
#define SCROLL_TARG_WEST	4
// disables processing
#define SCROLL_TARG_NONE	0x7f

// These are bitmasks now
#define PLAY_MASK		0xC000
#define PLAY_HUMAN		0x0000
#define PLAY_COMPUTER	0x4000
#define PLAY_NONE		0x8000
#define PLAY_NETWORKDROP 0xC000

// additional player bitmasks
#define PLAY_SPECIAL_MASK 0x0300
#define PLAY_SPECIAL_CLONE 0x0100	// iskur's clones
#define PLAY_SPECIAL_POWER 0x0200	// powered up player

// Player character
#define PLAY_CHAR_MASK 0x000f

// some maximums and defaults
#define MAX_SHEEP 35	// total sheep to come out in the level
#define FIRST_SHEEP 5	// release five at first, then 4 more every so often
#define HISTORY 120		// max sheep * 4 (4 is the spacing of sheep in the queue)
#define HERDERMAXSPEED 6
#define MAXLIGHTNING 5
#define HERDERSPEED 3
#define MAXSPECIAL 160
#define ADDSPECIAL 5
 
// Stage special effects (similar to the old story mode effects)
#define STAGE_EFFECT_SUGAR_RUSH			0x0001
#define STAGE_EFFECT_BOXED_SHEEP		0x0002
#define STAGE_EFFECT_NO_PLAYER_SPECIAL	0x0004
#define STAGE_EFFECT_SPECIAL_TO_WIN		0x0008
#define STAGE_EFFECT_CLEAR_SHEEP_TO_WIN	0x0010
#define STAGE_EFFECT_GHOST_SHEEP		0x0020
#define STAGE_EFFECT_PWRUP_ALWAYS_BOOT	0x0040
#define STAGE_EFFECT_PWRUP_ALWAYS_BOLT	0x0080
#define STAGE_EFFECT_NO_SHEEP			0x0100		// affects two places! start of level and timed release
#define STAGE_EFFECT_COMPUTER_FREEZE	0x0200
#define STAGE_EFFECT_NO_COMPUTER_ATTACK	0x0400		// affects lightning and specials
#define STAGE_EFFECT_FREEZE_TIMER		0x0800

// flags to help out the How To Play mode
#define HOW_TO_PLAY_MOVE	0x0001
#define HOW_TO_PLAY_FIRE	0x0002
#define HOW_TO_PLAY_CRATES	0x0014
#define HOW_TO_PLAY_BOOTS	0x0008
//efine HOW_TO_PLAY_CRATES2 0x0010	// merged with above
#define HOW_TO_PLAY_BOLTS	0x0020
#define HOW_TO_PLAY_CHARGE	0x0040
#define HOW_TO_PLAY_SPECIAL	0x0080
#define HOW_TO_PLAY_CAPTURE	0x0100
#define HOW_TO_PLAY_ZAP		0x0200
#define HOW_TO_PLAY_TIME	0x0400
#define HOW_TO_PLAY_LAST1	0x0800		// never set this one
#define HOW_TO_PLAY_COUNTDOWN	30*3
extern int nHowToPlayFlags;

// number of lost frames before a child gives up on the parent
#define MAX_LOST_FRAMES 100

// Helper
//#ifndef abs
//#define abs(x) ((x)<0 ? -(x) : (x))
//#endif
#ifndef true
#define true (1)
#endif
#ifndef false
#define false (0)
#endif

#ifndef min
	#define min(a,b) ((a)<(b)?(a):(b))
#endif
#ifndef max
	#define max(a,b) ((a)>(b)?(a):(b))
#endif

#ifndef sgn
	#define sgn(a) ((a) > 0 ? (1) : (a) == 0 ? 0 : -1)
#endif

// Centering the playfield
#define PLAYFIELDXOFF (-16)	
#define PLAYFIELDYOFF (-16)

// This structure is used for herders and sheep alike (not all fields apply to sheep)
typedef struct _herder{
	SPRITE spr;				// Sprite
	TXRMAP map[50];			// sprite map
	int destx, desty;		// Destination - used for computer play
	int originx,originy;	// origin - 1 axis only, used for player movement u-turns
	int oldx[HISTORY+1];
	int oldy[HISTORY+1];	// Trails, and used by the computer to detect being stuck
	int oldidx;				// position in the trail
	uint32 type;			// Player type - bitmasked now - see the TYPE_xxx defines
	unsigned char color;	// player color number (0-15)
	int score;				// Score
	int wins;				// Number of wins

	// user information (based on owner info, but reformatted to 10 ASCII-ish chars)
	unsigned char name[11];

	// Path for each CPU, x,y; x,y; x,y; etc
	char path[50*2];
	int pathIdx;

	uint32 CPUStatus;	// Used in the CPU 'intelligence'
	int nIdleCount;	// used by CPU and players
	int stuntime;	// if Stunned, for how much longer?
	int stunframe;	// special frame for being stunned
	int animframe;	// animation frame counter
	int maxframes;	// number of animation frames
	int sheep;		// How many sheep
	int ghostsheep;	// How many of those are ghosts?
	
	int maxsheep;	// Maximum sheep ever captured
	int recaptured;	// Maximum sheep recaptured
	int range;		// range on the electric staff
	int charge;		// charge level (16 units = 1 range)
	int speed;		// how fast do they move?
	
	int special;	// current special level (0-160 - ready when > 140)
	u16 spec_x;		// used for touch pad or shoulder buttons
	u16 spec_y;		// when charging up the special
	int specialFreeze;// when greater than zero, we are frozen for a special animation
} HerdStruct, *pHerdStruct;

extern pvr_ptr_t txr_sprites;
extern HerdStruct herder[6];

int  main(int argc, char *argv[]);
int  GetFinalWinner();
int  Game(int isMultiplayer);
void updateoldpos(pHerdStruct);
void load_bmp_block_mask(char *fn, char *pf, pvr_ptr_t whereto, pvr_ptr_t palette, int herd);
int  isStartPressed();
void debug(char *str, ...);
void ShowBlack();
int  GetRandomLevel();
int  doGameOver();
void ReloadMenu(char *psz);
int  HandleTopMenuView(int change);
void HandleMultiplayer();
void AIRetarget(int who);
int  GetPlayerSortedOrder(int *pArray);
void PlayMinigame(int nLevel);

// disclaimer.c
void doLogoFade(char *topscr, char *btmscr);

