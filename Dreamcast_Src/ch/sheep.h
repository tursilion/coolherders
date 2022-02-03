/****************************************/
/* Cool Herders                         */
/* A demented muse by Brendan Wiese     */
/* http://starbow.org                   */
/* Programmed by Mike Brent             */
/* http://harmlesslion.com              */
/* Copyright 2000-2005 Brendan Wiese &  */
/* Mike Brent.                          */
/* sheep.h                              */
/****************************************/

// Tile Numbers
#define SHEEPFRAMES 4
#define SHEEPUP 3
#define SHEEPDOWN 1
#define SHEEPLEFT 0
#define SHEEPRIGHT 2

#define NORMALSHEEPSPEED 3
#define GHOSTSHEEPSPEED 2

#define SHEEPHISTORY 5

typedef struct _sheep{
	SPRITE spr;			// Sprite
	int32 destx, desty; // Destination - used for computer play
	int32 type;			// 0-none, 1-normal, 2-running at 2x speed (last second before normal), 3-running at 3x speed
	uint32 stuntime;	// if Stunned, for how much longer?
	uint32 animframe;	// animation frame counter
	uint32 maxframes;	// number of animation frames
	uint32 range;		// invincibility countdown
	int32 oldx[SHEEPHISTORY+1];
	int32 oldy[SHEEPHISTORY+1];	// Trails, and used by the computer to detect being stuck
	int32 oldidx;			// position in the trail
	uint32 recaptured;	// Recapture double points time left
	int invincible;		// set when invicible
} SheepStruct;

extern unsigned int nGameFrames;
extern SheepStruct sheep[MAX_SHEEP];
extern pvr_ptr_t txr_sheep;
extern sfxhnd_t snd_sheep[6];
extern int SheepAnimationFrame[4][4];

void initSheep();
int RestartSheep(int x, int y, int xstep, int ystep);
void moveSheep();
void moveSheepConveyor();

