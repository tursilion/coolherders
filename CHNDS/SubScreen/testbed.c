/****************************************/
/* Cool Herders                         */
/* http://harmlesslion.com              */
/* Copyright 2009 HarmlessLion LLC      */
/* testbed.c                            */
/****************************************/

#include "testbed.h"

#ifdef TESTBED_MODE
#if 0
struct _leveldata xLevelData[LEVELYSIZE][LEVELXSIZE];
struct _sheep SheepData[30];
struct _herder herder[4];

const u8 TempLevelData[LEVELYSIZE][LEVELXSIZE] = {
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,1,1,1,1,1,0,1,1,1,1,1,0,1,1,1,1,1,0,0,
	0,0,1,0,0,0,1,0,1,0,0,0,1,0,1,0,0,0,1,0,0,
	0,0,1,0,1,1,1,0,1,1,1,1,1,0,1,1,1,0,1,0,0,
	0,0,1,0,1,0,1,1,1,0,1,0,1,1,1,0,1,0,1,0,0,
	0,0,1,1,1,0,1,0,0,0,1,0,0,0,1,0,1,1,1,0,0,
	0,0,0,0,1,0,1,1,1,1,1,1,1,1,1,0,1,0,0,0,0,
	0,0,1,1,1,0,1,0,0,0,1,0,0,0,1,0,1,1,1,0,0,
	0,0,1,0,1,0,1,1,1,0,1,0,1,1,1,0,1,0,1,0,0,
	0,0,1,0,1,1,1,0,1,1,1,1,1,0,1,1,1,0,1,0,0,
	0,0,1,0,0,0,1,0,1,0,0,0,1,0,1,0,0,0,1,0,0,
	0,0,1,1,1,1,1,0,1,1,1,1,1,0,1,1,1,1,1,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};
#endif

void TestBed_InitNitro(void){
//	OS_Init();
//	GX_Init();
//	GXS_DispOn();	// leave off till initialized fully
}

void TestBed_InitLevelData(void){
//	int nTempX, nTempY;
	
//	for (nTempY = 0; nTempY < LEVELYSIZE; nTempY++) {
//		for (nTempX = 0; nTempX < LEVELXSIZE; nTempX++) {
//			xLevelData[nTempY][nTempX].isPassable = TempLevelData[nTempY][nTempX];
//		}
//	}
}

void TestBed_InitSheepData(void){
#if 0
	int nTempSheep;
	
	for (nTempSheep = 0; nTempSheep < 30; nTempSheep++) {
		SheepData[nTempSheep].type = 0;
	}
	SheepData[0].type = 1;
	SheepData[0].spr.x = 128;
	SheepData[0].spr.y = 96;
	SheepData[1].type = 1;
	SheepData[1].spr.x = 512;
	SheepData[1].spr.y = 416;
#endif
}

void TestBed_InitHerderData(void){
#if 0
	int nTempHerder;
	
	for (nTempHerder = 0; nTempHerder < 4; nTempHerder++) {
		herder[nTempHerder].type = 0x8000;
	}
	herder[0].type = 0x0000;
	herder[0].spr.x = 64;
	herder[0].spr.y = 96;
	herder[1].type = 0x4000;
	herder[1].spr.x = 576;
	herder[1].spr.y = 416;
	herder[2].type = 0x4000;
	herder[2].spr.x = 476;
	herder[2].spr.y = 316;
	herder[3].type = 0x4000;
	herder[3].spr.x = 376;
	herder[3].spr.y = 216;
#endif
}

#endif
void TestBed_Init(void) {
#ifdef TESTBED_MODE
	TestBed_InitNitro();
	TestBed_InitLevelData();
	TestBed_InitSheepData();
	TestBed_InitHerderData();
#endif
}

