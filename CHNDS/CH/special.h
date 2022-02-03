/****************************************/
/* Cool Herders                         */
/* http://harmlesslion.com              */
/* Copyright 2009 HarmlessLion LLC      */
/* special.h                            */
/****************************************/

#define NUM_SPECIALS 8

enum 
{
	SP_NONE = 0,
	SP_DISCOBALL,		// backup dancer's small disco ball
	SP_GUMBALL,			// candy striper's giant gumball
	SP_KIWI,			// classic herder's Kiwi bird
	SP_FLOOD,			// Iskur's flash flood
	SP_RIBBON,			// NH-5's ribbon wrap
	SP_BEAM,			// Thalia's power beam
	SP_GIANTDISCO,		// Trey's giant disco ball
	SP_LIGHTNING,		// Zeus's lightning blast
	SP_ANGELLIGHT,		// Angel's light
	SP_DEMONBLAST,		// demon blast wave
	SP_WOLF,			// Hades power of the wolf
	SP_ZOMBIE			// Zombie power
};

typedef struct _special {
	SPRITE spr;			// Sprite
	int destx, desty; 	// Destination - used for kiwis
	int type;			// See enum above
	int animframe;		// animation frame counter
	int misccnt;		// miscellaneous counter
	int oldx;
	int oldy;			// Trails, and used by kiwi to detect being stuck
} SpecialStruct;


void InitSpecials();
int HandleSpecial(int idx, int xstep, int ystep, int currentx, int currenty, int oldtile);
int ProcessSpecials(int who, int nCountdown);
int GetDanceFrame();
void DrawZombieLightning(SPRITE spr, int who);

