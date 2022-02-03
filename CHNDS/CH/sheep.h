/****************************************/
/* Cool Herders                         */
/* http://harmlesslion.com              */
/* Copyright 2009 HarmlessLion LLC      */
/* sheep.h                              */
/****************************************/

#define SHEEPFRAMES 4

// Tile Numbers
#define NORMALSHEEPSPEED 3
#define GHOSTSHEEPSPEED 2

// alpha values for sheep
#define SHEEP_ALPHA_NORMAL 	31
#define SHEEP_ALPHA_GHOST	17
#define SHEEP_ALPHA_INVINC	12

typedef struct _sheep{
	SPRITE spr;			// Sprite
	int destx, desty; 	// Destination - used for computer play
	int type;			// 0-none, 1-normal, 2-running at 2x speed (last second before normal), 3-running at 3x speed, negative=taken, but animating for being warped up
	int stuntime;		// if Stunned, for how much longer?
	int animframe;		// animation frame counter
	int maxframes;		// number of animation frames
	int range;			// invincibility countdown
	int oldx;
	int oldy;			// Trails, and used by the computer to detect being stuck
	int recaptured;		// Recapture double points time left
	int invincible;		// set when invicible
} SheepStruct;

extern unsigned int nGameFrames;
extern SheepStruct sheep[MAX_SHEEP];
extern pvr_ptr_t txr_sheep;
extern TXRMAP txrmap_sheep[];
extern int SheepAnimationFrame[4][4];

void initSheep();
int RestartSheep(int x, int y, int xstep, int ystep);
void moveSheep();

