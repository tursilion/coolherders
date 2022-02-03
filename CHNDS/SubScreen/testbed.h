/****************************************/
/* Cool Herders                         */
/* http://harmlesslion.com              */
/* Copyright 2009 HarmlessLion LLC      */
/* testbed.h                            */
/****************************************/

#define LEVELXSIZE 21
#define LEVELYSIZE 16

#include "..\ch\kosemulation.h"
#include "..\ch\sprite.h"
#include "..\ch\cool.h"
#include "..\ch\levels.h"
#include "..\ch\sheep.h"

#define MAX_HERDER 4

#define TESTBED_MODE

void TestBed_InitNitro(void);
void TestBed_InitLevelData(void);
void TestBed_InitSheepData(void);
void TestBed_InitHerderData(void);

void TestBed_Init(void);

#ifdef TESTBED_MODE
#if 0
struct _leveldata {
	int isPassable;
};
typedef struct SPRITE {
	int x, y;
} SPRITE;

typedef struct _sheep{
SPRITE spr; // Sprite
int type; // 0-none, 1-normal, 2-running at 2x speed (last second before normal), 3-running at 3x speed
} SheepStruct;
 
typedef struct _herder{
SPRITE spr; // Sprite
int type; // Player type - bitmasked now - see the TYPE_xxx defines
} HerdStruct;
extern struct _sheep SheepData[MAX_SHEEP];
extern struct _herder herder[MAX_HERDER];
#endif

extern struct _leveldata LevelData[LEVELYSIZE][LEVELXSIZE];

#endif
