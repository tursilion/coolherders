/****************************************/
/* Cool Herders                         */
/* http://harmlesslion.com              */
/* Copyright 2009 HarmlessLion LLC      */
/* levels.h                             */
/****************************************/

/* Screens are 21x16 */

#define LEVELXSIZE 21
#define LEVELYSIZE 16
#define LEVELBYTES (LEVELXSIZE * LEVELYSIZE * 3 + 1)

#define NUMLEVELS  (7*3)

struct _leveldata {
	int nPage;
	int is3D;
	int isPassable;
	int nTile;
	int nBaseTile;		// 0 if none, bit 0x80 indicates page 2
	int nPowerup;		// 1=lightning, 2=speed
	int nDestructible;	// 0=No, 1=solid, 2-4=animating
	int nPush;			// pushes player: 0=no, 1=up, 2=right, 3=down, 4=left
};

extern struct _leveldata LevelData[LEVELYSIZE][LEVELXSIZE];

extern char Level[NUMLEVELS][LEVELBYTES];

enum {	
	LEVEL_NZ,		// 0
	LEVEL_CANDY,
	LEVEL_HAUNTED,
	LEVEL_TOY,
	LEVEL_DISCO,
	LEVEL_WATER,
	LEVEL_UNDERWORLD,

	MAX_LEVELS,

	LEVEL_RANDOM	// deliberately AFTER MAX_LEVELS
};

int DrawLevel();
void LoadLevel(int nWorld, int nStage);
void StartLevelMusic(int nWorld);