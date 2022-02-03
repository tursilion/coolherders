/****************************************/
/* Cool Herders                         */
/* http://harmlesslion.com              */
/* Copyright 2009 HarmlessLion LLC      */
/* special.c                            */
/****************************************/
// see bottom of file for summary of each special (for documentation)

#include <stdio.h>
#include <string.h>
#include "kosemulation.h"
#include "sprite.h"
#include "cool.h"
#include "menu.h"
#include "sound.h"
#include "font.h"
#include "control.h"
#include "rand.h"
#include "levels.h"
#include "vmu_logo.h"
#include "musicgallery.h"
#include "storymode.h"
#include "special.h"
#include "pathfind.h"
#include "sheep.h"
#include "chwireless.h"
  
// settings for specials
#define SP_LIGHTNING_LIFETIME 28
#define NH5SPEED 4
#define THALIASPEED 2
 
// local variables to speed parsing
static int xstep, ystep, currentx, currenty, oldtile;
int playeridx;		// needed externally
extern char bSpecialDim;
extern int myJiffies;
extern int gStageSpecialEffect;
extern int gBonusWinFlag;

// Data for each player's special
SpecialStruct specialDat[4][NUM_SPECIALS];	// four players, up to NUM_SPECIALS sprites each (only zeus and the demon needs more than 4)
 
// animation frames for each special
const int DemonBlastFrames[8] = { 37, 8, 18, 28, 38, 27, 17, 7 };	// 0 = up, then clockwise
const int DemonBlastXDir[8] = { 0, 1, 2, 1, 0, -1, -2, -1 };
const int DemonBlastYDir[8] = { -2, -1, 0, 1, 2, 1, 0, -1 };

const int DiscoBallFrames[4] =  { 7,17,27,37 };

const int GiantDiscoFrames[7][4] =  {	// [frame][sprite]
	{	7,17,8,18	}, 
	{	7,17,28,18	}, 
	{	27,17,8,18	}, 
	{	7,37,8,18	}, 
	{	7,17,8,38	}, 
	{	27,17,8,38	}, 
	{	7,37,28,18	}
};
		
const int GumBallFrames[2] = { 7,17 };
const int KiwiFrames[4][2] = {
	{ 7, 8,	},				// SHEEPLEFT
	{ 37,38	},				// SHEEPDOWN
	{ 17,18	},				// SHEEPRIGHT
	{ 27,28 }				// SHEEPUP
};	
const int FloodFrames[8] = { 7, 17, 27, 37, 8, 18, 28, 38 };
const int BeamFrames[4][3][2] = {	// also good for lightning and ribbon
	// SHEEPLEFT
	{ 
		{ 9, 19 },			// first
		{ 8, 18 },			// middle
		{ 7, 17 }			// end
	},
	// SHEEPDOWN
	{
		{27, 28},			// first
		{37, 38},			// middle
		{47, 48}			// end
	},
	// SHEEPRIGHT
	{
		{7, 17},			// first
		{8, 18},			// middle
		{9, 19}				// end
	},
	// SHEEPUP
	{
		{47,48},			// first
		{37, 38},			// middle
		{27, 28}			// end
	}
};

// update Dancer, Trey, and GetDanceFrame if this changes size
// would be const, but I can't reference it then.
int DanceFrames[44] = {
	4, 4, 5, 5, 4, 4, 5, 5, 14,14,15,15,14,14,15,15,32,32,24,24,43,43,25,25,43,43,24,24,43,43,25,25,43,43,30,31,32,33,34,34,34,34,34,34
};

// mapping table from regular lightning to zombie lightning
// Original	
// DOWN:  10,11  Tip: 15,16
// RIGHT: 12,17  Tip: 13,18
// manual edits
// UP:    25,26  Tip: 27,28
// LEFT:  29,30  Tip: 31,32
// unused entries are just set to a default, they shouldn't come up
const int DarkLightningMap[33] = {
	0,0,0,0,0,0,0,0,0,0,
	17,						// down 1
	18, 					// down 2
	38,						// right 1
	39,						// right tip 1
	0,
	27,						// down tip 1
	28,						// down tip 2
	48,						// right 2
	49,						// right tip 2
	0,0,0,0,0,0,
	17,						// up 1
	18,						// up 2
	7,						// up tip 1
	8,						// up tip 2
	48,						// left 1
	38,						// left 2
	37,						// left tip 1
	47						// left tip 2
};

// return the current dance frame (assumes dancing is active)
int GetDanceFrame() {
	if ((bSpecialDim > 0) && (specialDat[bSpecialDim-1][0].animframe >= 0)) {
		return DanceFrames[specialDat[bSpecialDim-1][0].animframe];
	} else {
		// return something sane - end of the sequence
		return DanceFrames[43];
	}
}

// individual handlers -- set up whatever needs to be
// done for each character's special. In most cases this will
// be setting up the special objects for the event, setting
// the appropriate pose and timer, playing the correct sound
// effect, etc
void HandleZeus() {
	int idx, x2, y2;
	
	// Zeus' special is a massive electric blast that spreads out in front of him
	// We can use the player position to determine direction - bolts should never
	// move back towards the player on either axis but can spread otherwise
	// So here, we need to put down the first element
	// On each turn, each element chooses a position to spread a new element to
	// any valid direction as long as it is not back towards the player. When the
	// last element is filled, we instead start deleting from the start. During the
	// delete phase the player is allowed to move again.

	// Start the sound effect for lightning shock
	sound_effect_player(playeridx, SND_ZEUSPOWER+(ch_rand()%MAXSND_SPECIAL), SPECIALVOL);
	
	// zero most of the struct
	for (idx=1; idx<NUM_SPECIALS; idx++) {
		specialDat[playeridx][idx].type = SP_NONE;
	}
	// set the first object into motion
	specialDat[playeridx][0].type = SP_LIGHTNING;
	specialDat[playeridx][0].animframe = 0;
	memcpy(&specialDat[playeridx][0].spr, &herder[playeridx].spr, sizeof(SPRITE));

	specialDat[playeridx][0].spr.x += xstep;
	specialDat[playeridx][0].spr.y += ystep;
	specialDat[playeridx][0].misccnt = SP_LIGHTNING_LIFETIME*2;		// lifetime of the bolt (first one is delayed for sfx reasons)
	
	// make sure that first object is grid aligned
	x2=((specialDat[playeridx][0].spr.x-PLAYFIELDXOFF)+GRIDSIZE/2)/GRIDSIZE;
	y2=((specialDat[playeridx][0].spr.y-PLAYFIELDYOFF)+GRIDSIZE/2)/GRIDSIZE;
	specialDat[playeridx][0].spr.x = (x2*GRIDSIZE)+PLAYFIELDXOFF;
	specialDat[playeridx][0].spr.y = (y2*GRIDSIZE)+PLAYFIELDYOFF;	

	// set up animation frames
	if (xstep > 0) {
		specialDat[playeridx][0].spr.tilenumber = 9;
		herder[playeridx].spr.tilenumber = 26;
	} else if (xstep < 0) {
		specialDat[playeridx][0].spr.tilenumber = 7;
		herder[playeridx].spr.tilenumber = 6;
	} else if (ystep > 0) {
		specialDat[playeridx][0].spr.tilenumber = 47;
		herder[playeridx].spr.tilenumber = 16;
	} else {
		specialDat[playeridx][0].spr.tilenumber = 27;
		herder[playeridx].spr.tilenumber = 36;
	}
	
	herder[playeridx].specialFreeze = 10;		// we won't let this count down normally
}

// update one Zeus lightning bolt, they count down their own life and
// try to spawn a new one (with a higher index only)
// if nCountdown is not -1, don't capture or steal sheep
void ProcessZeus(int who, int idx, int nCountdown) {
	int x1,y1,x2,y2;
	int free,idx3;
	int x,y;
	
	// check life first
	if (specialDat[who][idx].misccnt > 0) {
		specialDat[who][idx].misccnt--;
	}
	if (specialDat[who][idx].misccnt <= 0) {
		// we're finished!
		specialDat[who][idx].type = SP_NONE;
		// player freeze can countdown now
		nHowToPlayFlags |= HOW_TO_PLAY_SPECIAL;
		return;
	}
	
	// otherwise, don't count it down
	herder[who].specialFreeze = 5;		// we won't let this count down normally

	// set up the special locations for testing	
	x1=specialDat[who][idx].spr.x;
	y1=specialDat[who][idx].spr.y;
	// so now is the correct time, for each valid direction 
	// we spawn only if there is a valid structure to load into
	x2=(x1-PLAYFIELDXOFF)/GRIDSIZE;
	y2=(y1-PLAYFIELDYOFF)/GRIDSIZE;

	// not done, check whether we can spawn a new bolt. We have
	// to check only once, so we check when our lifetime counts down twice
	if (specialDat[who][idx].misccnt == SP_LIGHTNING_LIFETIME - 2) {
		// find the first free object
		free=idx+1;
		while (free < NUM_SPECIALS) {
			if (specialDat[who][free].type == SP_NONE) break;
			free++;
		}
		
		// check the four directions - we never move towards the player so
		// that always rules out at least one
		if (free < NUM_SPECIALS) {
			// up
			if (y1 <= herder[who].spr.y) {
				if (isFlat(x2,y2-1)) {
					// spawn a new upwards lightning bolt
					specialDat[who][free].type = SP_LIGHTNING;
					specialDat[who][free].animframe = 0;
					memcpy(&specialDat[who][free].spr, &specialDat[who][idx].spr, sizeof(SPRITE));
					specialDat[who][free].spr.y -= GRIDSIZE;
					specialDat[who][free].misccnt = SP_LIGHTNING_LIFETIME;		// lifetime of the bolt
					specialDat[who][free].spr.tilenumber = 27;
					// this affects our animation type if we were previously a tip
					if ((specialDat[who][idx].spr.tilenumber == 27)||(specialDat[who][idx].spr.tilenumber == 28)) {
						specialDat[who][idx].spr.tilenumber = 37;
					}
					free++;
				}
			}
		}
		if (free < NUM_SPECIALS) {
			// down
			if (y1 >= herder[who].spr.y) {
				if (isFlat(x2,y2+1)) {
					// spawn a new upwards lightning bolt
					specialDat[who][free].type = SP_LIGHTNING;
					specialDat[who][free].animframe = 0;
					memcpy(&specialDat[who][free].spr, &specialDat[who][idx].spr, sizeof(SPRITE));
					specialDat[who][free].spr.y += GRIDSIZE;
					specialDat[who][free].misccnt = SP_LIGHTNING_LIFETIME;		// lifetime of the bolt
					specialDat[who][free].spr.tilenumber = 47;
					// this affects our animation type if we were previously a tip
					if ((specialDat[who][idx].spr.tilenumber == 47)||(specialDat[who][idx].spr.tilenumber == 48)) {
						specialDat[who][idx].spr.tilenumber = 37;
					}
					free++;
				}
			}
		}
		if (free < NUM_SPECIALS) {
			// right
			if (x1 >= herder[who].spr.x) {
				if (isFlat(x2+1,y2)) {
					// spawn a new upwards lightning bolt
					specialDat[who][free].type = SP_LIGHTNING;
					specialDat[who][free].animframe = 0;
					memcpy(&specialDat[who][free].spr, &specialDat[who][idx].spr, sizeof(SPRITE));
					specialDat[who][free].spr.x += GRIDSIZE;
					specialDat[who][free].misccnt = SP_LIGHTNING_LIFETIME;		// lifetime of the bolt
					specialDat[who][free].spr.tilenumber = 9;
					// this affects our animation type if we were previously a tip
					if ((specialDat[who][idx].spr.tilenumber == 9)||(specialDat[who][idx].spr.tilenumber == 19)) {
						specialDat[who][idx].spr.tilenumber = 8;
					}
					free++;
				}
			}
		}
		if (free < NUM_SPECIALS) {
			// left
			if (x1 <= herder[who].spr.x) {
				if (isFlat(x2-1,y2)) {
					// spawn a new upwards lightning bolt
					specialDat[who][free].type = SP_LIGHTNING;
					specialDat[who][free].animframe = 0;
					memcpy(&specialDat[who][free].spr, &specialDat[who][idx].spr, sizeof(SPRITE));
					specialDat[who][free].spr.x -= GRIDSIZE;
					specialDat[who][free].misccnt = SP_LIGHTNING_LIFETIME;		// lifetime of the bolt
					specialDat[who][free].spr.tilenumber = 7;
					// this affects our animation type if we were previously a tip
					if ((specialDat[who][idx].spr.tilenumber == 7)||(specialDat[who][idx].spr.tilenumber == 17)) {
						specialDat[who][idx].spr.tilenumber = 8;
					}
					free++;
				}
			}
		}
	} else {
		// destroy any destructible objects we hit
		if (LevelData[y2][x2].nDestructible == 1) {
			LevelData[y2][x2].nDestructible=2;
			herder[who].score+=10;
		}
		
		// check for sheep
		// Check for stunned sheep
		for (idx3=0; idx3<MAX_SHEEP; idx3++) {
			if ((sheep[idx3].stuntime > 0) || (sheep[idx3].type < 1)) {
				// Can't re-zap a stunned sheep
				continue;
			}
			if (!sheep[idx3].invincible) {
				x=(int)sheep[idx3].spr.x;
				x-=x1;
				if (x<0) x=x*(-1);
				y=(int)sheep[idx3].spr.y;
				y-=y1;
				if (y<0) y=y*(-1);
				if ((x<28)&&(y<28)) {
					// got this sheep :)
					herder[who].score+=10;
					sheep[idx3].stuntime=60;	// 2 second stun time
				}
			}
		}

		// How about other herders?
		for (idx3=0; idx3<4; idx3++) {
			if ((idx3!=who)&&((herder[idx3].type&PLAY_MASK)!=PLAY_NONE)) {	// don't check ourselves or non-players
				// check for collision
				x=herder[idx3].spr.x;
				x-=x1;
				y=herder[idx3].spr.y;
				y-=y1;
				if ((abs(x)<28)&&(abs(y)<28)) {
					// if a first time hit, add some special
					if (herder[idx3].stuntime == 0) {
						herder[idx3].special+=5;
						if (herder[idx3].special > MAXSPECIAL) {
							herder[idx3].special = MAXSPECIAL;
						}
					}
				
					// got this guy :) (even if stunned!)
					if (herder[idx3].stuntime > 0) {
						// to help the animation along, we cheat a bit here
						// we don't touch the frame, and we try to keep the last
						// few bits of the time untouched
						herder[idx3].stuntime |= 0x3C;
					} else {
						// first hit
						herder[idx3].stunframe = 9;		// first electrified frame
						herder[idx3].stuntime = 63;		// 2 second stun time
					}
					herder[idx3].destx = herder[idx3].spr.x;
					herder[idx3].desty = herder[idx3].spr.y;	// don't nudge him!
					herder[who].score+=10;			// will probably score multiple hits
					
					// play the other players hit sound (throttled)
					sound_effect_player_throttled(idx3, SND_ZEUSDAMAGE1+(ch_rand()%MAXSND_DAMAGE), PLAYERVOL);
					
					herder[idx3].charge = 0;	// zero the victim's lightning gauge
					
					if (nCountdown == -1) {
						// only for Zeus, there's a little bit of story mode interaction here
						// when STAGE_EFFECT_SPECIAL_TO_WIN is set, hitting any other player
						// (who is assumed to be Iskur) wins the stage.
						// when STAGE_EFFECT_CLEAR_SHEEP_TO_WIN is set, you have to remove all
						// sheep from the other herder, and pick them all up.
						if (gStageSpecialEffect&STAGE_EFFECT_SPECIAL_TO_WIN) {
							if ((herder[idx3].type&PLAY_SPECIAL_CLONE) == 0) {
								// the REAL Iskur, only
								gBonusWinFlag = 1;
							}
						}
						// lose all sheep, including ghosts, if he has any
						while (herder[idx3].sheep>0) {
							herder[idx3].sheep--;
							// find a free sheep to start
							// Sheep start on grid offsets
							x=x2*GRIDSIZE+PLAYFIELDXOFF;
							y=y2*GRIDSIZE+PLAYFIELDYOFF;
							RestartSheep(x,y,xstep,ystep);
							if (gStageSpecialEffect&STAGE_EFFECT_CLEAR_SHEEP_TO_WIN) {
								// longer stun time to give Zeus time to collect
								herder[idx3].stuntime+=30;	// one more second
							}
						}
						// lose ghost sheep too (only Zeus needs this to be a test)
						if (herder[idx3].ghostsheep > herder[idx3].sheep) {
							herder[idx3].ghostsheep = herder[idx3].sheep;
						}
					}
				}
			}
		}
	}
	
	// just to fill in the gaps, if this is a vertical sprite we should draw an extra overlapping set
	if (specialDat[who][idx].spr.tilenumber == 37) {
		// vertical! draw an extra two to pad it
		specialDat[who][idx].spr.y+=16;
		SortSprite(&specialDat[who][idx].spr, herder[who].map, POLY_SPECIAL);
		specialDat[who][idx].spr.y-=32;
		SortSprite(&specialDat[who][idx].spr, herder[who].map, POLY_SPECIAL);
		specialDat[who][idx].spr.y+=16;
	}
}

void HandleAngel() {
	int idx;
	
	// Angel's special is glowing circle of light that captures loose sheep and frees
	// captured sheep. It expands out, then contracts back down. We only need one variable
	// (used in the level draw code) to handle it.

	// Start the sound effect for holy light
	sound_effect_player(playeridx, SND_ANGELPOWER+(ch_rand()%MAXSND_SPECIAL), SPECIALVOL);
	
	// zero most of the struct
	for (idx=1; idx<NUM_SPECIALS; idx++) {
		specialDat[playeridx][idx].type = SP_NONE;
	}
	// set the first object into motion
	specialDat[playeridx][0].type = SP_ANGELLIGHT;
	specialDat[playeridx][0].misccnt = 25;	// current range
	specialDat[playeridx][0].animframe = 5;	// direction
	
	// set up animation frames
	if (xstep > 0) {
		herder[playeridx].spr.tilenumber = 26;
	} else if (xstep < 0) {
		herder[playeridx].spr.tilenumber = 6;
	} else if (ystep > 0) {
		herder[playeridx].spr.tilenumber = 16;
	} else {
		herder[playeridx].spr.tilenumber = 36;
	}
	
	herder[playeridx].specialFreeze = 10;		// we won't let this count down normally
}

// Update the circle of light, and check if anyone is caught by it
// idx will always be 0 here. if nCountdown is not -1, don't capture or steal sheep.
void ProcessAngel(int who, int idx, int nCountdown) {
	int x1,y1;
	int idx3;
	int nRange;
	int x,y;
	int nDistance;
	
	// check life first
	if (specialDat[who][0].animframe > 0) {
		if (specialDat[who][0].misccnt >= 100) {
			specialDat[who][0].animframe-=2;
		} else {
			specialDat[who][0].misccnt += specialDat[who][0].animframe;
			if (specialDat[who][0].misccnt >= 100) {
				specialDat[who][0].animframe = -specialDat[who][0].animframe;
			}
		}
	} else {
		if ((specialDat[who][0].misccnt >= 100)&&(specialDat[who][0].animframe > -5)) {
			specialDat[who][0].animframe-=2;
		} else {
			specialDat[who][0].misccnt += specialDat[who][0].animframe;
			if (specialDat[who][0].misccnt <= 16) {
				// all finished
				specialDat[who][idx].type = SP_NONE;
				return;
			}
		}
	}
		
	// otherwise, don't count down player freeze
	herder[who].specialFreeze = 5;		// we won't let this count down normally
	x1=herder[who].spr.x;
	y1=herder[who].spr.y;

	// calculate range
	nRange = specialDat[who][idx].misccnt * specialDat[who][idx].misccnt;
	
	// check for sheep
	if (nCountdown == -1) {
		for (idx3=0; idx3<MAX_SHEEP; idx3++) {
			if ((sheep[idx3].stuntime > 0) || (sheep[idx3].type < 1)) {
				// Can't collect a stunned sheep with this (otherwise you'd instantly
				// get all the sheep who were released from the other herders
				continue;
			}
			if (!sheep[idx3].invincible) {
				x=(int)sheep[idx3].spr.x;
				x-=x1;
				y=(int)sheep[idx3].spr.y;
				y-=y1;
				nDistance = (x*x) + (y*y);
				if (nDistance < nRange) {
					// got this sheep :)
					sheep[idx3].type=-100;		// start warping up animation
					herder[who].sheep++;
					if (gStageSpecialEffect&STAGE_EFFECT_GHOST_SHEEP) herder[who].ghostsheep++;
					AIRetarget(who);
					if (herder[who].sheep <= herder[who].maxsheep) {
						if (herder[who].sheep > herder[who].recaptured) {
							herder[who].score+=10;
							herder[who].recaptured++;
						}
					} else {
						if (sheep[idx3].recaptured > 0) {
							herder[who].score+=100;
						} else {
							herder[who].score+=50;
						}
						herder[who].maxsheep=herder[who].sheep;
					}
					sound_effect_system_throttled(SND_SHEEP1+CAUGHT, SHEEPVOL>>1);
				}
			}
		}
	}

	// How about other herders?
	for (idx3=0; idx3<4; idx3++) {
		if ((idx3!=who)&&((herder[idx3].type&PLAY_MASK)!=PLAY_NONE)) {	// don't check ourselves or non-players
			// check for collision
			x=herder[idx3].spr.x;
			x-=x1;
			y=herder[idx3].spr.y;
			y-=y1;
			nDistance = (x*x) + (y*y);
			if (nDistance < nRange) {
				// if a first time hit, add some special
				if (herder[idx3].stuntime == 0) {
					herder[idx3].special+=5;
					if (herder[idx3].special > MAXSPECIAL) {
						herder[idx3].special = MAXSPECIAL;
					}
				}

				// got this guy :)
				herder[idx3].stuntime = 5;		// 1/5 second stun time
				herder[idx3].stunframe = 0;		// no animation
				herder[idx3].destx = herder[idx3].spr.x;
				herder[idx3].desty = herder[idx3].spr.y;	// don't nudge him!
				herder[who].score+=10;			// will probably score multiple hits
				
				// play the other players hit sound (throttled)
				sound_effect_player_throttled(idx3, SND_ZEUSDAMAGE1+(ch_rand()%MAXSND_DAMAGE), PLAYERVOL);
				
				herder[idx3].charge = 0;	// zero the victim's lightning gauge
				
				if (nCountdown == -1) {
					// lose all sheep, if he has any
					while (herder[idx3].sheep>0) {
						herder[idx3].sheep--;
						// find a free sheep to start
						// Sheep start on grid offsets
						x=((herder[idx3].spr.x-PLAYFIELDXOFF)/GRIDSIZE);
						x=x*GRIDSIZE+PLAYFIELDXOFF;
						y=((herder[idx3].spr.y-PLAYFIELDYOFF)/GRIDSIZE);
						y=y*GRIDSIZE+PLAYFIELDYOFF;
						RestartSheep(x,y,xstep,ystep);
					}
					// lose all ghost sheep too
					herder[idx3].ghostsheep = 0;
				}
			}
		}
	}
}


void HandleCandy() {
	int idx,x2,y2;
	
	// Candy Striper's special is a giant gumball that rezzes in front of her,
	// then rolls through the maze until it can't roll any further.
	// We can use the player position to determine direction - turns should never
	// move back towards the player on either axis.
	// So here, we need to create the first and only element.
	// On each turn, the ball moves until it reaches the next junction, then chooses
	// a direction which moves away from the player who launched it.
	// When it can no longer choose a new direction, it dissolves.

	// Start the sound effect for gumdrop
	sound_effect_player(playeridx, SND_CANDYSTRIPER1POWER+(ch_rand()%MAXSND_SPECIAL), SPECIALVOL);
	
	// zero most of the struct
	for (idx=1; idx<NUM_SPECIALS; idx++) {
		specialDat[playeridx][idx].type = SP_NONE;
	}
	// set the first object into motion
	specialDat[playeridx][0].type = SP_GUMBALL;
	specialDat[playeridx][0].animframe = -1;	// rezzing
	specialDat[playeridx][0].misccnt = 10;		// rez state and direction
	memcpy(&specialDat[playeridx][0].spr, &herder[playeridx].spr, sizeof(SPRITE));
	specialDat[playeridx][0].spr.xd = 0;
	specialDat[playeridx][0].spr.yd = 0;

	for (idx=0; idx<5; idx++) {
		switch (idx) {
			case 0:		// player's choice
				break;
				
			case 1:		// up
				xstep = 0;
				ystep = -GRIDSIZE;
				break; 
				
			case 2:		// down
				xstep = 0;
				ystep = GRIDSIZE;
				break; 

			case 3:		// left
				xstep = -GRIDSIZE;
				ystep = 0;
				break; 

			case 4:		// right
				xstep = GRIDSIZE;
				ystep = 0;
				break; 
		}

		// get a start point
		specialDat[playeridx][0].spr.x += xstep;
		specialDat[playeridx][0].spr.y += ystep;
		// align position to the grid so that it's going to be able to choose a direction
		// Note if this first point isn't valid, just pick another, I guess.
		x2=((specialDat[playeridx][0].spr.x-PLAYFIELDXOFF)+GRIDSIZE/2)/GRIDSIZE;
		y2=((specialDat[playeridx][0].spr.y-PLAYFIELDYOFF)+GRIDSIZE/2)/GRIDSIZE;
		if (!isFlat(x2,y2)) {
			specialDat[playeridx][0].spr.x = herder[playeridx].spr.x;
			specialDat[playeridx][0].spr.y = herder[playeridx].spr.y;
			continue;
		}
		specialDat[playeridx][0].spr.x = (x2*GRIDSIZE)+PLAYFIELDXOFF;
		specialDat[playeridx][0].spr.y = (y2*GRIDSIZE)+PLAYFIELDYOFF;
		break;
	}
	
	// set up animation frames
	specialDat[playeridx][0].spr.tilenumber = GumBallFrames[0];
	if (xstep > 0) {
		herder[playeridx].spr.tilenumber = 26;
	} else if (xstep < 0) {
		herder[playeridx].spr.tilenumber = 6;
	} else if (ystep > 0) {
		herder[playeridx].spr.tilenumber = 16;
	} else {
		herder[playeridx].spr.tilenumber = 36;
	}
	
	herder[playeridx].specialFreeze = 10;	// temporary value
}

// update Candy Striper's gumball.
// if nCountdown is not -1, don't capture or steal sheep
void ProcessCandy(int who, int idx, int nCountdown) {
	int x1,y1,x2,y2;
	int idx3;
	int x,y;

	// are we animating it, first?
	if (specialDat[who][idx].animframe == -1) {
		// yes, work the direction
		if (specialDat[who][idx].misccnt > 0) {
			// up

			// no freeze countdown during rez frame
			herder[who].specialFreeze = 10;		// we won't let this count down normally

			specialDat[who][idx].misccnt += 10;
			if (specialDat[who][idx].misccnt >= 100) {
				specialDat[who][idx].misccnt = 100;
				specialDat[who][idx].animframe = 0;		// roll next frame
			}
			// render it here squished
			SortSpriteSquashed(&specialDat[who][idx].spr, herder[who].map, POLY_SPECIAL, 100, specialDat[who][idx].misccnt);
			return;
		}
		// else, down
		specialDat[who][idx].misccnt += 10;
		if (specialDat[who][idx].misccnt >= 0) {
			// we're done, just disable this object completely
			specialDat[who][idx].type = SP_NONE;
			return;
		}
		// render it here squished
		SortSpriteSquashed(&specialDat[who][idx].spr, herder[who].map, POLY_SPECIAL, 100, -specialDat[who][idx].misccnt);
		return;
	}

	// when the gumball is stuck, set animframe to -1 and misccnt to -100 to do the deflate animation

	// set up the special locations for testing	
	x1=specialDat[who][idx].spr.x;
	y1=specialDat[who][idx].spr.y;
	x2=((x1-PLAYFIELDXOFF)+GRIDSIZE/2)/GRIDSIZE;
	y2=((y1-PLAYFIELDYOFF)+GRIDSIZE/2)/GRIDSIZE;

	// first, determine if we are at an absolute offset which means we need to choose a direction
	if (((x2*GRIDSIZE)+PLAYFIELDXOFF == x1) && ((y2*GRIDSIZE)+PLAYFIELDYOFF == y1)) {
		// yes, we are. Choose a legal direction. There are up to three possibilities, but all must be away from the player
		specialDat[who][idx].spr.xd=0;
		specialDat[who][idx].spr.yd=0;
		// check for up
		if (y1 <= herder[who].spr.y) {
			if (isFlat(x2,y2-1)) {
				specialDat[who][idx].spr.yd=-2;
				goto accepted;
			}
		}
		// check for down
		if (y1 >= herder[who].spr.y) {
			if (isFlat(x2,y2+1)) {
				specialDat[who][idx].spr.yd=2;
				goto accepted;
			}
		}
		// check for left
		if (x1 <= herder[who].spr.x) {
			if (isFlat(x2-1,y2)) {
				specialDat[who][idx].spr.xd=-2;
				goto accepted;
			}
		}
		// check for right
		if (x1 >= herder[who].spr.x) {
			if (isFlat(x2+1,y2)) {
				specialDat[who][idx].spr.xd=2;
				goto accepted;
			}
		}
		// no direction was valid, so just end next frame
		specialDat[who][idx].animframe=-1;
		specialDat[who][idx].misccnt=-100;
		
accepted:
		// finished here
		idx3=0;		// dummy statement
	}
	
	// move the ball
	specialDat[who][idx].spr.x += specialDat[who][idx].spr.xd;
	specialDat[who][idx].spr.y += specialDat[who][idx].spr.yd;
	// animate it
	if (specialDat[who][idx].animframe > -1) {
		specialDat[who][idx].animframe++;
		if (specialDat[who][idx].animframe > 7) {
			specialDat[who][idx].animframe = 0;
		}
		specialDat[who][idx].spr.tilenumber = GumBallFrames[specialDat[who][idx].animframe>>2];
	}
	
	// sort it into the buffer (special case)
	SortSprite(&specialDat[who][idx].spr, herder[who].map, POLY_SPECIAL);
	
	// now check collisions
	// destroy any destructible objects we hit
	if (LevelData[y2][x2].nDestructible == 1) {
		LevelData[y2][x2].nDestructible=2;
		herder[who].score+=10;
	}
		
	// check for sheep
	// Check for stunned sheep
	for (idx3=0; idx3<MAX_SHEEP; idx3++) {
		if ((sheep[idx3].stuntime > 0) || (sheep[idx3].type < 1)) {
			// Can't re-zap a stunned sheep
			continue;
		}
		if (!sheep[idx3].invincible) {
			x=(int)sheep[idx3].spr.x;
			x-=x1;
			if (x<0) x=x*(-1);
			y=(int)sheep[idx3].spr.y;
			y-=y1;
			if (y<0) y=y*(-1);
			if ((x<28)&&(y<28)) {
				// got this sheep :)
				herder[who].score+=10;
				sheep[idx3].stuntime=60;	// 2 second stun time
			}
		}
	}

	// How about other herders?
	for (idx3=0; idx3<4; idx3++) {
		if ((idx3!=who)&&((herder[idx3].type&PLAY_MASK)!=PLAY_NONE)) {	// don't check ourselves or non-players
			// check for collision
			x=herder[idx3].spr.x;
			x-=x1;
			y=herder[idx3].spr.y;
			y-=y1;
			if ((abs(x)<28)&&(abs(y)<28)) {
				// if a first time hit, add some special
				if (herder[idx3].stuntime == 0) {
					herder[idx3].special+=5;
					if (herder[idx3].special > MAXSPECIAL) {
						herder[idx3].special = MAXSPECIAL;
					}
				}

				// got this guy :) (even if stunned!)
				herder[idx3].stuntime = 15;		// 1/2 second stun time
				herder[idx3].stunframe = 0;		// no animation frame
				herder[idx3].destx = herder[idx3].spr.x + specialDat[who][idx].spr.xd;
				herder[idx3].desty = herder[idx3].spr.y + specialDat[who][idx].spr.yd;	// push him along!
				herder[who].score+=10;			// will probably score multiple hits
				
				// play the other players hit sound (throttled)
				sound_effect_player_throttled(idx3, SND_ZEUSDAMAGE1+(ch_rand()%MAXSND_DAMAGE), PLAYERVOL);

				herder[idx3].charge = 0;	// zero the victim's lightning gauge
								
				if (nCountdown == -1) {
					// lose all sheep, if he has any
					while (herder[idx3].sheep>0) {
						herder[idx3].sheep--;
						// find a free sheep to start
						// Sheep start on grid offsets
						x=x2*GRIDSIZE+PLAYFIELDXOFF;
						y=y2*GRIDSIZE+PLAYFIELDYOFF;
						RestartSheep(x,y,xstep,ystep);
					}
					// lose all ghost sheep too
					herder[idx3].ghostsheep = 0;
				}
			}
		}
	}
}


void HandleDancer() {
	int idx;
	
	// Backup Dancer's special is a small disco ball that rezzes above her.
	// For a small range, it causes all sheep to dance, and all herders to lose
	// their sheep and dance along.
	// So here, we need to create the first and only element.
	// On each turn, we update the sheep animation and time the effect.
	// It warps in and out like the gumball but is overtop of the background and players

	// Start the sound effect for discoball
	sound_effect_player(playeridx, SND_BACKUPDANCERPOWER+(ch_rand()%MAXSND_SPECIAL), SPECIALVOL);
	
	// zero most of the struct
	for (idx=1; idx<NUM_SPECIALS; idx++) {
		specialDat[playeridx][idx].type = SP_NONE;
	}
	// set the first object into motion
	specialDat[playeridx][0].type = SP_DISCOBALL;
	specialDat[playeridx][0].animframe = -1;	// rezzing
	specialDat[playeridx][0].misccnt = 10;		// rez state and direction
	memcpy(&specialDat[playeridx][0].spr, &herder[playeridx].spr, sizeof(SPRITE));
	specialDat[playeridx][0].spr.xd = 0;
	specialDat[playeridx][0].spr.yd = 0;
	specialDat[playeridx][0].spr.y -= GRIDSIZE;		// nudge up above the dancer
	specialDat[playeridx][0].spr.z -= (fx16)0x60;	// overtop of all
	specialDat[playeridx][0].oldx = 90;				// duration of disco ball
	specialDat[playeridx][0].oldy = 0;				// animation frame counter
	// we cheat and use some of the others as flags just to see if we caught the other herders (just for audio today)
	specialDat[playeridx][1].misccnt=0;				// we don't bother here to check which one is us ;)
	specialDat[playeridx][2].misccnt=0;
	specialDat[playeridx][3].misccnt=0;
	specialDat[playeridx][4].misccnt=0;

	// set up animation frames
	specialDat[playeridx][0].spr.tilenumber = DiscoBallFrames[0];
	if (xstep > 0) {
		herder[playeridx].spr.tilenumber = 26;
	} else if (xstep < 0) {
		herder[playeridx].spr.tilenumber = 6;
	} else if (ystep > 0) {
		herder[playeridx].spr.tilenumber = 16;
	} else {
		herder[playeridx].spr.tilenumber = 36;
	}
	
	herder[playeridx].specialFreeze = 10;	// temporary value
}

// update Backup Dancer's disco ball.
// if nCountdown is not -1, don't capture or steal sheep
void ProcessDancer(int who, int idx, int nCountdown) {
	int x1,y1;
	int idx3;
	int nRange;
	int x,y;
	int nDistance;
	

	// are we animating it, first?
	if (specialDat[who][idx].animframe == -1) {
		// yes, work the direction
		if (specialDat[who][idx].misccnt > 0) {
			// up

			// no freeze countdown during rez frames
			herder[who].specialFreeze = 44;		// we won't let this count down normally

			specialDat[who][idx].misccnt += 10;
			if (specialDat[who][idx].misccnt >= 80) {
				specialDat[who][idx].misccnt = 80;
				specialDat[who][idx].animframe = 0;		// dance next frame
			}
			// render it here squished
			SortSpriteSquashed(&specialDat[who][idx].spr, herder[who].map, POLY_SPECIAL, 100, specialDat[who][idx].misccnt);
			return;
		}
		// else, down
		specialDat[who][idx].misccnt += 10;
		if (specialDat[who][idx].misccnt >= 0) {
			// we're done, just disable this object completely
			specialDat[who][idx].type = SP_NONE;
			return;
		}
		// render it here squished
		SortSpriteSquashed(&specialDat[who][idx].spr, herder[who].map, POLY_SPECIAL, 100, -specialDat[who][idx].misccnt);
		return;
	}

	// animation speed counter
	specialDat[playeridx][0].oldy++;
	if (specialDat[playeridx][0].oldy >= 4) {
		specialDat[playeridx][0].oldy = 0;
	}

	// backup dancer dances for one cycle - animframes of 0-43, after that he or she is free
	if (herder[who].specialFreeze > 1) {
		if (specialDat[playeridx][0].oldy != 0) {
			herder[who].specialFreeze++;	// slow the animation
		}
		herder[who].spr.tilenumber = DanceFrames[44-herder[who].specialFreeze];
	} else {
		// count down the disco ball
		specialDat[who][idx].oldx--;
		herder[who].specialFreeze = 2;	// so that it counts down to 1
		if (specialDat[who][idx].oldx < 1) {
			// animate the disco ball flattening
			specialDat[who][idx].animframe = -1;
			specialDat[who][idx].misccnt = -100;
			return;
		}
	}
			
	// animate the ball and count up the global dance frame
	if (specialDat[playeridx][0].oldy == 0) {
		specialDat[who][idx].animframe++;
		if (specialDat[who][idx].animframe >= 44) specialDat[who][idx].animframe=0;
	}
	specialDat[who][idx].spr.tilenumber = DiscoBallFrames[specialDat[who][idx].animframe & 0x03];
	SortSprite(&specialDat[who][idx].spr, herder[who].map, POLY_SPECIAL);

	// destructibles are handled in the level draw code
	
	x1=specialDat[who][idx].spr.x;
	y1=specialDat[who][idx].spr.y;
	
	// calculate range
	nRange = specialDat[who][idx].misccnt * specialDat[who][idx].misccnt;
	
	// check for sheep
	// Check for stunned sheep
	for (idx3=0; idx3<MAX_SHEEP; idx3++) {
		if ((sheep[idx3].stuntime > 0) || (sheep[idx3].type < 1)) {
			// Can't re-zap a stunned sheep
			continue;
		}
		if (!sheep[idx3].invincible) {
			x=(int)sheep[idx3].spr.x;
			x-=x1;
			y=(int)sheep[idx3].spr.y;
			y-=y1;
			nDistance = (x*x) + (y*y);
			if (nDistance < nRange) {
				// got this sheep :)
				herder[who].score+=10;
				sheep[idx3].stuntime=60;	// 2 second stun time (this is fine, all stunned sheep dance during dance specials)
			}
		}
	}

	// How about other herders?
	for (idx3=0; idx3<4; idx3++) {
		if ((idx3!=who)&&((herder[idx3].type&PLAY_MASK)!=PLAY_NONE)) {	// don't check ourselves or non-players
			// check for collision
			x=herder[idx3].spr.x;		// X is centered
			x-=x1;
			y=herder[idx3].spr.y-16;	// Y is BOTTOM based
			y-=y1;
			nDistance = (x*x) + (y*y);
			if (nDistance < nRange) {
				// if a first time hit, add some special
				if (herder[idx3].stuntime == 0) {
					herder[idx3].special+=5;
					if (herder[idx3].special > MAXSPECIAL) {
						herder[idx3].special = MAXSPECIAL;
					}
				}

				// got this guy :) (even if stunned!)
				herder[idx3].stuntime = 15;		// 1/2 second stun time
				herder[idx3].stunframe = -1;	// dancing animation frame
				herder[who].score+=10;			// will probably score multiple hits
				
				// play the other players hit sound (throttled)
				// hearing it over and over again is annoying, so we set a flag to only play it once
				if (specialDat[who][idx3+1].misccnt == 0) {
					specialDat[who][idx3+1].misccnt = 1;
					sound_effect_player_throttled(idx3, SND_ZEUSDAMAGE1+(ch_rand()%MAXSND_DAMAGE), PLAYERVOL);
				}

				herder[idx3].charge = 0;	// zero the victim's lightning gauge
								
				if (nCountdown == -1) {
					// lose all sheep, if he has any
					while (herder[idx3].sheep>0) {
						herder[idx3].sheep--;
						// find a free sheep to start
						// Sheep start on grid offsets
						x=((herder[idx3].spr.x-PLAYFIELDXOFF)/GRIDSIZE);
						x=x*GRIDSIZE+PLAYFIELDXOFF;
						y=((herder[idx3].spr.y-PLAYFIELDYOFF)/GRIDSIZE);
						y=y*GRIDSIZE+PLAYFIELDYOFF;
						RestartSheep(x,y,xstep,ystep);
					}
					// lose all ghost sheep too
					herder[idx3].ghostsheep = 0;
				}
			}
		}
	}
}


void HandleDemon() {
	int idx;
	
	// Demon's is an 8-way explosion that passes through walls, destroying  everything, knocking back
	// herders, paralyzing sheep. Since we need all 8 special objects for this one, we will include
	// a delay animation state for the countdown of the voice effect

	// Start the sound effect for demon blast
	sound_effect_player(playeridx, SND_DEMONPOWER+(ch_rand()%MAXSND_SPECIAL), SPECIALVOL);
	
	// set up all 8 objects
	for (idx=0; idx<NUM_SPECIALS; idx++) {
		specialDat[playeridx][idx].type = SP_DEMONBLAST;
		specialDat[playeridx][idx].animframe = 17;	// this will be the countdown timer
		memcpy(&specialDat[playeridx][idx].spr, &herder[playeridx].spr, sizeof(SPRITE));	// start point same as player, will offset
		specialDat[playeridx][idx].spr.tilenumber = DemonBlastFrames[idx];
		specialDat[playeridx][idx].spr.y-=12;
		specialDat[playeridx][idx].spr.xd=(GRIDSIZE/8)*DemonBlastXDir[idx];
		specialDat[playeridx][idx].spr.yd=(GRIDSIZE/8)*DemonBlastYDir[idx];
		specialDat[playeridx][idx].spr.z+=4;		// below the demon
		specialDat[playeridx][idx].misccnt = 19;	// number of frames to live for
	}
	
	// set up animation frames (demon only has one)
	herder[playeridx].spr.tilenumber = 16;
	herder[playeridx].specialFreeze = 10;		// we won't let this count down normally
}

// update one demon fireball - do not draw or animate it until it's animframe counts down to 0
// then count down its life with misccnt
// if nCountdown is not -1, don't capture or steal sheep
void ProcessDemon(int who, int idx, int nCountdown) {
	int x1,y1,x2,y2;
	int idx3;
	int x,y;
	
	// check countdown first
	if (specialDat[who][idx].animframe > 0) {
		specialDat[who][idx].animframe--;
		herder[playeridx].specialFreeze = 10;		// we won't let this count down normally
		return;
	}
	
	// now check life
	if (specialDat[who][idx].misccnt > 0) {
		specialDat[who][idx].misccnt--;
	}
	if (specialDat[who][idx].misccnt <= 0) {
		// we're finished!
		specialDat[who][idx].type = SP_NONE;
		// player freeze can countdown now
		return;
	}
	 
	// otherwise, don't count it down
	herder[who].specialFreeze = 5;		// we won't let this count down normally
	
	// move the fireball and then draw it
	specialDat[who][idx].spr.x += specialDat[who][idx].spr.xd;
	specialDat[who][idx].spr.y += specialDat[who][idx].spr.yd;
	
	// if it moves off screen, discard it
	if ((specialDat[who][idx].spr.x < 1) || (specialDat[who][idx].spr.x > 639) ||
		(specialDat[who][idx].spr.y < 1) || (specialDat[who][idx].spr.y > 479)) {
		// we're finished!
		specialDat[who][idx].type = SP_NONE;
		// player freeze can countdown now
		return;
	}
	// draw it	
	SortSprite(&specialDat[who][idx].spr, herder[who].map, POLY_SPECIAL);

	// set up the special locations for testing	
	x1=specialDat[who][idx].spr.x;
	y1=specialDat[who][idx].spr.y;
	// only need this to check destructibles
	x2=((x1-PLAYFIELDXOFF)+GRIDSIZE/2)/GRIDSIZE;
	y2=((y1-PLAYFIELDYOFF)+GRIDSIZE/2)/GRIDSIZE;

	// destroy any destructible objects we hit
	if (LevelData[y2][x2].nDestructible == 1) {
		LevelData[y2][x2].nDestructible=2;
		herder[who].score+=10;
	}
		
	// check for sheep
	// Check for stunned sheep
	for (idx3=0; idx3<MAX_SHEEP; idx3++) {
		if ((sheep[idx3].stuntime > 0) || (sheep[idx3].type < 1)) {
			// Can't re-zap a stunned sheep
			continue;
		}
		if (!sheep[idx3].invincible) {
			x=(int)sheep[idx3].spr.x;
			x-=x1;
			if (x<0) x=x*(-1);
			y=(int)sheep[idx3].spr.y;
			y-=y1;
			if (y<0) y=y*(-1);
			if ((x<28)&&(y<28)) {
				// got this sheep :)
				herder[who].score+=10;
				sheep[idx3].stuntime=90;	// 3 second stun time
			}
		}
	}

	// How about other herders?
	for (idx3=0; idx3<4; idx3++) {
		if ((idx3!=who)&&((herder[idx3].type&PLAY_MASK)!=PLAY_NONE)) {	// don't check ourselves or non-players
			// check for collision
			x=herder[idx3].spr.x;
			x-=x1;
			y=herder[idx3].spr.y;
			y-=y1;
			if ((abs(x)<28)&&(abs(y)<28)) {
				// if a first time hit, add some special
				if (herder[idx3].stuntime == 0) {
					herder[idx3].special+=5;
					if (herder[idx3].special > MAXSPECIAL) {
						herder[idx3].special = MAXSPECIAL;
					}
				}

				// got this guy :) (even if stunned!)
				herder[idx3].stuntime = 25;		// 5/6s second stun time
				herder[idx3].stunframe = 0;		// no special frame
				herder[idx3].destx += specialDat[who][idx].spr.xd * (GRIDSIZE*5);
				herder[idx3].desty += specialDat[who][idx].spr.yd * (GRIDSIZE*5);	// hard shove
				herder[who].score+=10;			// will probably score multiple hits
				
				// play the other players hit sound (throttled)
				sound_effect_player_throttled(idx3, SND_ZEUSDAMAGE1+(ch_rand()%MAXSND_DAMAGE), PLAYERVOL);
				
				herder[idx3].charge = 0;	// zero the victim's lightning gauge
				
				if (nCountdown == -1) {
					// lose all sheep, if he has any
					while (herder[idx3].sheep>0) {
						herder[idx3].sheep--;
						// find a free sheep to start
						// Sheep start on grid offsets
						x=x2*GRIDSIZE+PLAYFIELDXOFF;
						y=y2*GRIDSIZE+PLAYFIELDYOFF;
						RestartSheep(x,y,xstep,ystep);
					}
					// lose all ghost sheep too
					herder[idx3].ghostsheep = 0;
				}
			}
		}
	}
}

void HandleHerder() {
	int idx,idx2,x2,y2;
	
	// Classic Herder's special is four kiwi birds who drop down, then
	// run through the maze until they hit a player, or time out.
	// unlike the gumball, we do NOT use the player position to determine direction,
	// we just run them randomly, turning when a junction offers itself.
	// When they hit a player, or turn 'x' times, they leave.

	// Start the sound effect for kiwi
	sound_effect_player(playeridx, SND_CLASSICHERDERPOWER+(ch_rand()%MAXSND_SPECIAL), SPECIALVOL);
	
	// zero unused part of the struct
	for (idx=4; idx<NUM_SPECIALS; idx++) {
		specialDat[playeridx][idx].type = SP_NONE;
	}
	// set the kiwis up
	for (idx=0; idx<4; idx++) {
		specialDat[playeridx][idx].type = SP_KIWI;
		specialDat[playeridx][idx].misccnt = 10;	// number of turns (not frames!) they are good for
		memcpy(&specialDat[playeridx][idx].spr, &herder[playeridx].spr, sizeof(SPRITE));
		specialDat[playeridx][idx].spr.xd = 0;
		specialDat[playeridx][idx].spr.yd = 0;
	
		// pick a valid starting position - needed in case the player is facing a wall
		// we really should only do this once and save the result, but it's fine.
		for (idx2=0; idx2<5; idx2++) {
			switch (idx2) {
				case 0:		// player's choice
					break;
					
				case 1:		// up
					xstep = 0;
					ystep = -GRIDSIZE;
					break; 
					
				case 2:		// down
					xstep = 0;
					ystep = GRIDSIZE;
					break; 

				case 3:		// left
					xstep = -GRIDSIZE;
					ystep = 0;
					break; 

				case 4:		// right
					xstep = GRIDSIZE;
					ystep = 0;
					break; 
			}

			// get a start point
			specialDat[playeridx][idx].spr.x += xstep;
			specialDat[playeridx][idx].spr.y += ystep;
			// align position to the grid so that it's going to be able to choose a direction
			// Note if this first point isn't valid, just pick another, I guess.
			x2=((specialDat[playeridx][idx].spr.x-PLAYFIELDXOFF)+GRIDSIZE/2)/GRIDSIZE;
			y2=((specialDat[playeridx][idx].spr.y-PLAYFIELDYOFF)+GRIDSIZE/2)/GRIDSIZE;
			if (!isFlat(x2,y2)) {
				specialDat[playeridx][idx].spr.x = herder[playeridx].spr.x;
				specialDat[playeridx][idx].spr.y = herder[playeridx].spr.y;
				continue;
			}
			specialDat[playeridx][idx].spr.x = (x2*GRIDSIZE)+PLAYFIELDXOFF;
			specialDat[playeridx][idx].spr.y = (y2*GRIDSIZE)+PLAYFIELDYOFF;

			// now make that a target to drop to, and place them off the top of the grid
			specialDat[playeridx][idx].destx=specialDat[playeridx][idx].spr.x;
			specialDat[playeridx][idx].desty=specialDat[playeridx][idx].spr.y;
			specialDat[playeridx][idx].spr.y = -24*idx-16;
			break;
		}
		
		// set up animation frames
		if (xstep > 0) {
			herder[playeridx].spr.tilenumber = 26;
			specialDat[playeridx][idx].animframe = 2;	// direction
			specialDat[playeridx][idx].spr.tilenumber = KiwiFrames[2][0];
			specialDat[playeridx][idx].spr.xd = 4;
			specialDat[playeridx][idx].spr.yd = 0;
		} else if (xstep < 0) {
			herder[playeridx].spr.tilenumber = 6;
			specialDat[playeridx][idx].animframe = 0;	// direction
			specialDat[playeridx][idx].spr.tilenumber = KiwiFrames[0][0];
			specialDat[playeridx][idx].spr.xd = -4;
			specialDat[playeridx][idx].spr.yd = 0;
		} else if (ystep > 0) {
			herder[playeridx].spr.tilenumber = 16;
			specialDat[playeridx][idx].animframe = 1;	// direction
			specialDat[playeridx][idx].spr.tilenumber = KiwiFrames[1][0];
			specialDat[playeridx][idx].spr.xd = 0;
			specialDat[playeridx][idx].spr.yd = 4;
		} else {
			herder[playeridx].spr.tilenumber = 36;
			specialDat[playeridx][idx].animframe = 3;	// direction
			specialDat[playeridx][idx].spr.tilenumber = KiwiFrames[3][0];
			specialDat[playeridx][idx].spr.xd = 0;
			specialDat[playeridx][idx].spr.yd = -4;
		}
		specialDat[playeridx][idx].spr.z-=60;	// overtop of everything while dropping
	}
	
	herder[playeridx].specialFreeze = 10;	// temporary value
}

// update a Kiwi!
// if nCountdown is not -1, don't capture or steal sheep
void ProcessHerder(int who, int idx, int nCountdown) {
	int x1,y1,x2,y2;
	int idx3;
	int x,y;

	// are we animating it, first? (dropping in or flying out)
	if (specialDat[who][idx].destx != 0) {
		if (specialDat[who][idx].desty > 0) {
			herder[playeridx].specialFreeze = 10;	// keep the herder in the deploy animation
			// just drop into place, then. x movement should actually be 0
			specialDat[who][idx].spr.y += specialDat[who][idx].desty/15+1;	// get there in about 1/2 second no matter the height
			if (specialDat[who][idx].spr.y >= specialDat[who][idx].desty) {
				specialDat[who][idx].spr.y = specialDat[who][idx].desty;
				specialDat[who][idx].destx = 0;	// turn it off, normal movement next turn
			} else if (specialDat[who][idx].spr.y >= specialDat[who][idx].desty-48) {
				// fix the z order so we drop behind the scenery
				specialDat[who][idx].spr.z=herder[who].spr.z;
			}
		} else {
			// fly out, then!
			specialDat[who][idx].spr.y -= 30;		// fast exit
			specialDat[playeridx][idx].spr.z=herder[who].spr.z-60;	// overtop of everything while rising (will be away quick enough)
			if (specialDat[who][idx].spr.y <= 0) {
				specialDat[who][idx].type = SP_NONE;
			}
		}
		return;
	}
		
	// otherwise, we're on the move. When we run out of turns (misccnt), or hit a herder,
	// set destx to current x and desty to 0 for the hop out animation

	// set up the special locations for testing	
	x1=specialDat[who][idx].spr.x;
	y1=specialDat[who][idx].spr.y;
	x2=((x1-PLAYFIELDXOFF)+GRIDSIZE/2)/GRIDSIZE;
	y2=((y1-PLAYFIELDYOFF)+GRIDSIZE/2)/GRIDSIZE;

	// first, determine if we are at an absolute offset which means we need to choose a direction
	if (((x2*GRIDSIZE)+PLAYFIELDXOFF == x1) && ((y2*GRIDSIZE)+PLAYFIELDYOFF == y1)) {
		// fixed choices must use ch_rand() - always generate this value
		idx3=(int)ch_rand();
		
		// if the current path is invalid, though, we overwrite the random value so that all paths are legal
		if (!isFlat(x2+sgn(specialDat[who][idx].spr.xd), y2+sgn(specialDat[who][idx].spr.yd))) {
			idx3 = 0xffffffff;
			specialDat[who][idx].spr.yd = 0;	// zero these to check for errors
			specialDat[who][idx].spr.xd = 0;	
		}
		
		// Choose a legal direction. We only want to turn perpendicular to the current path,
		// so ignore paths we are already following. Turns are random unless the current path was blocked
		// if we are moving left or right
		if ((specialDat[playeridx][idx].animframe==0) || (specialDat[playeridx][idx].animframe==2)) {
			if (idx3&0x04) {
				// check for up
				if (isFlat(x2,y2-1)) {
					specialDat[who][idx].spr.yd=-4;
					specialDat[who][idx].spr.xd=0;
					specialDat[who][idx].animframe=3;
					if (--specialDat[who][idx].misccnt <= 0) {
						specialDat[who][idx].destx=1;
						specialDat[who][idx].desty=0;
						return;
					}
				}
			}
			if (idx3&0x08) {
				// check for down
				if (isFlat(x2,y2+1)) {
					specialDat[who][idx].spr.yd=4;
					specialDat[who][idx].spr.xd=0;
					specialDat[who][idx].animframe=1;
					if (--specialDat[who][idx].misccnt <= 0) {
						specialDat[who][idx].destx=1;
						specialDat[who][idx].desty=0;
						return;
					}
				}
			}
		} else {
			if (idx3 & 0x10) {
				// check for left
				if (isFlat(x2-1,y2)) {
					specialDat[who][idx].spr.yd=0;
					specialDat[who][idx].spr.xd=-4;
					specialDat[who][idx].animframe=0;
					if (--specialDat[who][idx].misccnt <= 0) {
						specialDat[who][idx].destx=1;
						specialDat[who][idx].desty=0;
						return;
					}
				}
			}
			if (idx3 & 0x20) {
				// check for right
				if (isFlat(x2+1,y2)) {
					specialDat[who][idx].spr.yd=0;
					specialDat[who][idx].spr.xd=4;
					specialDat[who][idx].animframe=2;
					if (--specialDat[who][idx].misccnt <= 0) {
						specialDat[who][idx].destx=1;
						specialDat[who][idx].desty=0;
						return;
					}
				}
			}
		}
		
		if ((specialDat[who][idx].spr.yd == 0) && (specialDat[who][idx].spr.xd == 0)) {
			// something went wrong in the pathfinding, so just lose the kiwi
			debug("Early kiwi abort - got stuck");
			specialDat[who][idx].destx=1;
			specialDat[who][idx].desty=0;
			return;
		}
	}
	
	// move the kiwi
	specialDat[who][idx].spr.x += specialDat[who][idx].spr.xd;
	specialDat[who][idx].spr.y += specialDat[who][idx].spr.yd;
	// animate it
	specialDat[who][idx].spr.tilenumber = KiwiFrames[specialDat[who][idx].animframe][(myJiffies>>5)&0x01];
	
	// now check collisions
	// destroy any destructible objects we hit
	if (LevelData[y2][x2].nDestructible == 1) {
		LevelData[y2][x2].nDestructible=2;
		herder[who].score+=10;
	}
		
	// check for sheep
	// Check for stunned sheep
	for (idx3=0; idx3<MAX_SHEEP; idx3++) {
		if ((sheep[idx3].stuntime > 0) || (sheep[idx3].type < 1)) {
			// Can't re-zap a stunned sheep
			continue;
		}
		if (!sheep[idx3].invincible) {
			x=(int)sheep[idx3].spr.x;
			x-=x1;
			if (x<0) x=x*(-1);
			y=(int)sheep[idx3].spr.y;
			y-=y1;
			if (y<0) y=y*(-1);
			if ((x<28)&&(y<28)) {
				// got this sheep :)
				herder[who].score+=10;
				sheep[idx3].stuntime=30;	// 1 second stun time
			}
		}
	}

	// How about other herders?
	for (idx3=0; idx3<4; idx3++) {
		if ((idx3!=who)&&((herder[idx3].type&PLAY_MASK)!=PLAY_NONE)) {	// don't check ourselves or non-players
			// check for collision
			x=herder[idx3].spr.x;
			x-=x1;
			y=herder[idx3].spr.y;
			y-=y1;
			if ((abs(x)<28)&&(abs(y)<28)) {
				// decrement this kiwi only if the herder was not already stunned
				if (herder[idx3].stuntime == 0) {
					if (--specialDat[who][idx].misccnt <= 0) {
						specialDat[who][idx].misccnt = 0;	// is out
						specialDat[who][idx].destx=1;
						specialDat[who][idx].desty=0;
						return;
					}
					// if a first time hit, add some special
					herder[idx3].special+=5;
					if (herder[idx3].special > MAXSPECIAL) {
						herder[idx3].special = MAXSPECIAL;
					}
				}

				// got this guy :) (even if stunned!)
				herder[idx3].stuntime = 30;		// 1 second stun time
				herder[idx3].stunframe = 0;		// no animation frame
				herder[idx3].destx = herder[idx3].spr.x + specialDat[who][idx].spr.xd;
				herder[idx3].desty = herder[idx3].spr.y + specialDat[who][idx].spr.yd;	// push him along!
				herder[who].score+=10;			// will probably score multiple hits
				
				// play the other players hit sound (throttled)
				sound_effect_player_throttled(idx3, SND_ZEUSDAMAGE1+(ch_rand()%MAXSND_DAMAGE), PLAYERVOL);

				herder[idx3].charge = 0;	// zero the victim's lightning gauge
								
				if (nCountdown == -1) {
					// lose all sheep, if he has any
					while (herder[idx3].sheep>0) {
						herder[idx3].sheep--;
						// find a free sheep to start
						// Sheep start on grid offsets
						x=x2*GRIDSIZE+PLAYFIELDXOFF;
						y=y2*GRIDSIZE+PLAYFIELDYOFF;
						RestartSheep(x,y,xstep,ystep);
					}
					// lose all ghost sheep too
					herder[idx3].ghostsheep = 0;
				}
			}
		}
	}
}

void HandleIskur() {
	int idx;
	
	// Iskur's special is a flood wave that flows out both in front and behind him
	// It jumps walls, and carries on to the end of the stage (will that work??)
	// So here, we need to put down the first two elements
	// On each turn, each element animates. On frame 3 it attempts to spawn a new
	// wave ahead of itself. Since that one may already have been processed, it
	// will have to draw the new one manually if it has a lower index.

	// Start the sound effect for flash flood
	sound_effect_player(playeridx, SND_ISKURPOWER+(ch_rand()%MAXSND_SPECIAL), SPECIALVOL);
	
	// zero most of the struct
	for (idx=2; idx<NUM_SPECIALS; idx++) {
		specialDat[playeridx][idx].type = SP_NONE;
	}
	// used as a flag to time when Iskur is released - once it is set the first time, he counts down
	specialDat[playeridx][7].spr.tilenumber = 0;
	// set the first two objects into motion
	specialDat[playeridx][0].type = SP_FLOOD;
	specialDat[playeridx][0].animframe = 0;
	specialDat[playeridx][0].misccnt = -20;	// animation timing count (negative being a delay for timing the first one)
	memcpy(&specialDat[playeridx][0].spr, &herder[playeridx].spr, sizeof(SPRITE));
	specialDat[playeridx][0].spr.x += xstep;
	specialDat[playeridx][0].spr.y += ystep;
	specialDat[playeridx][0].spr.tilenumber = FloodFrames[0];
	
	memcpy(&specialDat[playeridx][1], &specialDat[playeridx][0], sizeof(SpecialStruct));
	specialDat[playeridx][1].spr.x -= xstep+xstep;
	specialDat[playeridx][1].spr.y -= ystep+ystep;
	
	specialDat[playeridx][0].spr.xd = sgn(xstep);
	specialDat[playeridx][0].spr.yd = sgn(ystep);
	specialDat[playeridx][1].spr.xd = -sgn(xstep);
	specialDat[playeridx][1].spr.yd = -sgn(ystep);
	
	// set up animation frames
	if (xstep > 0) {
		herder[playeridx].spr.tilenumber = 26;
	} else if (xstep < 0) {
		herder[playeridx].spr.tilenumber = 6;
	} else if (ystep > 0) {
		herder[playeridx].spr.tilenumber = 16;
	} else {
		herder[playeridx].spr.tilenumber = 36;
	}
	
	herder[playeridx].specialFreeze = 50;		// we DO let this count down normally
}

// update one Iskur wave, they count down their own life and
// try to spawn a new one (looping as needed)
// if nCountdown is not -1, don't capture or steal sheep
void ProcessIskur(int who, int idx, int nCountdown) {
	int x1,y1,x2,y2;
	int free,idx3;
	int x,y;

	// check delay first
	if (specialDat[who][idx].misccnt < 0) {
		// don't frame animate, but bob it a little
		// by offsetting by 1, when it should be even, we can fix it by masking at the end
		specialDat[who][idx].misccnt++;
		if (specialDat[who][idx].misccnt >= 0) {
			specialDat[who][idx].misccnt = 0;
		}
		if (specialDat[who][idx].misccnt&1) {
			specialDat[who][idx].spr.y|=1;
		} else {
			specialDat[who][idx].spr.y&=~1;
		}
		return;
	}
	
	// set up the special locations for testing	
	x1=specialDat[who][idx].spr.x;
	y1=specialDat[who][idx].spr.y;
	// so now is the correct time, for each valid direction 
	// we spawn only if there is a valid structure to load into
	x2=((x1-PLAYFIELDXOFF)+GRIDSIZE/2)/GRIDSIZE;
	y2=((y1-PLAYFIELDYOFF)+GRIDSIZE/2)/GRIDSIZE;
	
	// check animation, death, spawning
	specialDat[who][idx].misccnt++;
	if (specialDat[who][idx].misccnt >= 3) {
		specialDat[who][idx].misccnt=0;
	
		// check for rapid wall skimming
		if (specialDat[who][idx].animframe == 0) {
			if ((LevelData[y2][x2].isPassable==false) && (LevelData[y2][x2].is3D) && (LevelData[y2][x2].nDestructible==0)) {
				// don't animate, just move
				specialDat[who][idx].spr.x+=specialDat[who][idx].spr.xd*GRIDSIZE;
				specialDat[who][idx].spr.y+=specialDat[who][idx].spr.yd*GRIDSIZE;
				if ((specialDat[who][idx].spr.x < PLAYFIELDXOFF+GRIDSIZE*2) || (specialDat[who][idx].spr.x > PLAYFIELDXOFF+GRIDSIZE*19) ||
					(specialDat[who][idx].spr.y < PLAYFIELDYOFF+GRIDSIZE*3) || (specialDat[who][idx].spr.y > PLAYFIELDYOFF+GRIDSIZE*14)) {
						// off the edge, kill it
						specialDat[who][idx].type = SP_NONE;
				}
				return;
			}
		}
		
		// next animation frame
		specialDat[who][idx].animframe++;
		// check for end of life
		if (specialDat[who][idx].animframe >= 8) {
			specialDat[who][idx].type = SP_NONE;
			return;
		}
		if (specialDat[who][idx].animframe == 2) {
			// try to spawn a new one
			free = -1;
			for (idx3=0; idx3<NUM_SPECIALS; idx3++) {
				if (specialDat[who][idx3].type == SP_NONE) {
					free=idx3;
					break;
				}
				if ((specialDat[who][idx3].type == SP_FLOOD) && (specialDat[who][idx3].animframe == 7)) {
					free=idx3;
					break;
				}
			}
			// if we can't find one for some reason, we'll just die out
			if (free != -1) {
				// spawn a new one - if the index is lower it will be delayed being drawn by one frame
				memcpy(&specialDat[who][free], &specialDat[who][idx], sizeof(SpecialStruct));
				specialDat[who][free].animframe=0;
				specialDat[who][free].spr.tilenumber = FloodFrames[0];
				specialDat[who][free].spr.x += specialDat[who][free].spr.xd*GRIDSIZE;
				specialDat[who][free].spr.y += specialDat[who][free].spr.yd*GRIDSIZE;
				if ((specialDat[who][free].spr.x < PLAYFIELDXOFF+GRIDSIZE*2) || (specialDat[who][free].spr.x > PLAYFIELDXOFF+GRIDSIZE*19) ||
					(specialDat[who][free].spr.y < PLAYFIELDYOFF+GRIDSIZE*3) || (specialDat[who][free].spr.y > PLAYFIELDYOFF+GRIDSIZE*14)) {
						// off the edge, kill it
						specialDat[who][free].type = SP_NONE;
				}
			}
		}
		specialDat[who][idx].spr.tilenumber = FloodFrames[specialDat[who][idx].animframe];
	}
					
	// destroy any destructible objects we hit
	if (LevelData[y2][x2].nDestructible == 1) {
		LevelData[y2][x2].nDestructible=2;
		herder[who].score+=10;
	}
		
	// check for sheep
	// Check for stunned sheep
	for (idx3=0; idx3<MAX_SHEEP; idx3++) {
		if ((sheep[idx3].stuntime > 0) || (sheep[idx3].type < 1)) {
			// Can't re-zap a stunned sheep
			continue;
		}
		if (!sheep[idx3].invincible) {
			x=(int)sheep[idx3].spr.x;
			x-=x1;
			if (x<0) x=x*(-1);
			y=(int)sheep[idx3].spr.y;
			y-=y1;
			if (y<0) y=y*(-1);
			if ((x<28)&&(y<28)) {
				// got this sheep :)
				herder[who].score+=10;
				sheep[idx3].stuntime=60;	// 2 second stun time
			}
		}
	}

	// How about other herders?
	for (idx3=0; idx3<4; idx3++) {
		if ((idx3!=who)&&((herder[idx3].type&PLAY_MASK)!=PLAY_NONE)) {	// don't check ourselves or non-players
			// check for collision
			x=herder[idx3].spr.x;
			x-=x1;
			y=herder[idx3].spr.y;
			y-=y1;
			if ((abs(x)<28)&&(abs(y)<28)) {
				// if a first time hit, add some special
				if (herder[idx3].stuntime == 0) {
					herder[idx3].special+=5;
					if (herder[idx3].special > MAXSPECIAL) {
						herder[idx3].special = MAXSPECIAL;
					}
				}

				// got this guy :) (even if stunned!)
				herder[idx3].stuntime = 15;		// 1/2 second stun time
				herder[idx3].stunframe = 0;		// no special frame
				herder[idx3].destx = specialDat[who][idx].spr.x + specialDat[who][idx].spr.xd*GRIDSIZE;
				herder[idx3].desty = specialDat[who][idx].spr.y + specialDat[who][idx].spr.yd*GRIDSIZE;	// carry him along!
				herder[who].score+=10;			// will probably score multiple hits
				
				// play the other players hit sound (throttled)
				sound_effect_player_throttled(idx3, SND_ZEUSDAMAGE1+(ch_rand()%MAXSND_DAMAGE), PLAYERVOL);
				
				herder[idx3].charge = 0;	// zero the victim's lightning gauge
				
				if (nCountdown == -1) {
					// lose all sheep, if he has any
					while (herder[idx3].sheep>0) {
						herder[idx3].sheep--;
						// find a free sheep to start
						// Sheep start on grid offsets
						x=x2*GRIDSIZE+PLAYFIELDXOFF;
						y=y2*GRIDSIZE+PLAYFIELDYOFF;
						RestartSheep(x,y,xstep,ystep);
					}
					// lose all ghost sheep too
					herder[idx3].ghostsheep = 0;
				}
			}
		}
	}
}

void HandleNH5() {
	int idx;
	
	// NH-5's special is ribbon that shoots out in front of him, and gift-wraps
	// any herder it hits (with the usual sheep stunning effect). Since it's
	// directional, it results in a longer stun than others

	// On each turn, each element counts down. When it goes from 3, it spawns
	// the next element, if there is a next element to spawn. It will also
	// stop on walls, so must be fired in a valid direction. Otherwise, it is
	// wasted, we will not auto-aim this one.

	// Start the sound effect for ribbon wrap
	sound_effect_player(playeridx, SND_NH5POWER+(ch_rand()%MAXSND_SPECIAL), SPECIALVOL);
	
	// zero most of the struct
	for (idx=1; idx<NUM_SPECIALS; idx++) {
		specialDat[playeridx][idx].type = SP_NONE;
	}
	// set the first object into motion
	specialDat[playeridx][0].type = SP_RIBBON;
	specialDat[playeridx][0].animframe = 2;
	specialDat[playeridx][0].misccnt = NH5SPEED;	// animation timing count 
	memcpy(&specialDat[playeridx][0].spr, &herder[playeridx].spr, sizeof(SPRITE));
	specialDat[playeridx][0].spr.x += xstep;
	specialDat[playeridx][0].spr.y += ystep;
	specialDat[playeridx][0].spr.xd = sgn(xstep);
	specialDat[playeridx][0].spr.yd = sgn(ystep);
	
	// set up animation frames
	if (xstep > 0) {
		specialDat[playeridx][0].spr.tilenumber = BeamFrames[SHEEPRIGHT][2][0];
		specialDat[playeridx][0].destx = SHEEPRIGHT;	// direction variable
		herder[playeridx].spr.tilenumber = 26;
	} else if (xstep < 0) {
		specialDat[playeridx][0].spr.tilenumber = BeamFrames[SHEEPLEFT][2][0];
		specialDat[playeridx][0].destx = SHEEPLEFT;	// direction variable
		herder[playeridx].spr.tilenumber = 6;
	} else if (ystep > 0) {
		specialDat[playeridx][0].spr.tilenumber = BeamFrames[SHEEPDOWN][2][0];
		specialDat[playeridx][0].destx = SHEEPDOWN;	// direction variable
		herder[playeridx].spr.tilenumber = 16;
	} else {
		specialDat[playeridx][0].spr.tilenumber = BeamFrames[SHEEPUP][2][0];
		specialDat[playeridx][0].destx = SHEEPUP;	// direction variable
		herder[playeridx].spr.tilenumber = 36;
	}
	
	herder[playeridx].specialFreeze = 10;		// we won't let this count down normally
}

// update one NH5 ribbon segment, they count down their own life and
// try to spawn a new one when needed (higher only)
// if nCountdown is not -1, don't capture or steal sheep
void ProcessNH5(int who, int idx, int nCountdown) {
	int x1,y1,x2,y2;
	int idx3;
	int x,y;
	
	// allow the countdown after the first one is freed
	if (specialDat[who][0].type != SP_NONE) {
		herder[who].specialFreeze = 5;
	}
	
	// set up the special locations for testing	
	x1=specialDat[who][idx].spr.x;
	y1=specialDat[who][idx].spr.y;
	// so now is the correct time, for each valid direction 
	// we spawn only if there is a valid structure to load into
	x2=((x1-PLAYFIELDXOFF)+GRIDSIZE/2)/GRIDSIZE;
	y2=((y1-PLAYFIELDYOFF)+GRIDSIZE/2)/GRIDSIZE;

	// we are also dead if we are on a non-flat cell (note: isFlat checks /passable/, but there is also flat IMPASSIBLE
	if ((LevelData[y2][x2].isPassable==false) && (LevelData[y2][x2].is3D) && (LevelData[y2][x2].nDestructible==0)) {
		specialDat[who][idx].type = SP_NONE;
		return;
	}

	// check delay first
	if (specialDat[who][idx].misccnt-- <= 0) {
		// check if this segment is dead
		if (specialDat[who][idx].animframe == 0) {
			// yes it is, end it
			specialDat[who][idx].type = SP_NONE;
			return;
		}

		// check if we need to spawn a new piece
		if ((specialDat[who][idx].animframe == 2) && (idx < 7)) {
			if (specialDat[who][idx+1].type == SP_NONE) {
				memcpy(&specialDat[who][idx+1], &specialDat[who][idx], sizeof(SpecialStruct));
				specialDat[who][idx+1].animframe=2;
				if (idx==6) {
					// piece 7 gets a longer period
					specialDat[who][idx+1].misccnt=NH5SPEED*7;
				} else {
					specialDat[who][idx+1].misccnt=NH5SPEED;
				}
				specialDat[who][idx+1].spr.x += specialDat[who][idx+1].spr.xd*GRIDSIZE;
				specialDat[who][idx+1].spr.y += specialDat[who][idx+1].spr.yd*GRIDSIZE;
			}
		}

		// next frame - special case for first and last segment
		if ((idx == 0) && (specialDat[who][idx].animframe==2))  {
			specialDat[who][idx].animframe = 0;	// skip the middle piece
			specialDat[who][idx].misccnt = NH5SPEED*7;
		} else if ((idx == 7) && (specialDat[who][idx].animframe==2)) {
			specialDat[who][idx].animframe = 0;	// skip the middle piece
			specialDat[who][idx].misccnt = NH5SPEED;
		} else {
			// everyone else works normally
			specialDat[who][idx].animframe--;
			if (specialDat[who][idx].animframe == 1) {
				// lives for 6 frames
				specialDat[who][idx].misccnt = NH5SPEED*6;
			} else {
				specialDat[who][idx].misccnt = NH5SPEED;
			}
		}
	}

	// update the animation frame
	specialDat[who][idx].spr.tilenumber = BeamFrames[specialDat[who][idx].destx][specialDat[who][idx].animframe][(myJiffies>>2)&1];
					
	// destroy any destructible objects we hit
	if (LevelData[y2][x2].nDestructible == 1) {
		LevelData[y2][x2].nDestructible=2;
		herder[who].score+=10;
	}
		
	// check for sheep
	// Check for stunned sheep
	for (idx3=0; idx3<MAX_SHEEP; idx3++) {
		if ((sheep[idx3].stuntime > 0) || (sheep[idx3].type < 1)) {
			// Can't re-zap a stunned sheep
			continue;
		}
		if (!sheep[idx3].invincible) {
			x=(int)sheep[idx3].spr.x;
			x-=x1;
			if (x<0) x=x*(-1);
			y=(int)sheep[idx3].spr.y;
			y-=y1;
			if (y<0) y=y*(-1);
			if ((x<28)&&(y<28)) {
				// got this sheep :)
				herder[who].score+=10;
				sheep[idx3].stuntime=90;	// 3 second stun time
			}
		}
	}

	// How about other herders?
	for (idx3=0; idx3<4; idx3++) {
		if ((idx3!=who)&&((herder[idx3].type&PLAY_MASK)!=PLAY_NONE)) {	// don't check ourselves or non-players
			// check for collision
			x=herder[idx3].spr.x;
			x-=x1;
			y=herder[idx3].spr.y;
			y-=y1;
			if ((abs(x)<28)&&(abs(y)<28)) {
				// if a first time hit, add some special
				if (herder[idx3].stuntime == 0) {
					herder[idx3].special+=5;
					if (herder[idx3].special > MAXSPECIAL) {
						herder[idx3].special = MAXSPECIAL;
					}
				}

				// got this guy :) (even if stunned!)
				herder[idx3].stuntime = 75;		// 2.5 second stun time!
				herder[idx3].stunframe = 4;		// wrapped frame
				herder[idx3].destx = herder[idx3].spr.x;
				herder[idx3].desty = herder[idx3].spr.y;	// no movement
				herder[who].score+=10;			// will probably score multiple hits
				
				// play the other players hit sound (throttled)
				sound_effect_player_throttled(idx3, SND_ZEUSDAMAGE1+(ch_rand()%MAXSND_DAMAGE), PLAYERVOL);
				
				herder[idx3].charge = 0;	// zero the victim's lightning gauge
				
				if (nCountdown == -1) {
					// lose all sheep, if he has any
					while (herder[idx3].sheep>0) {
						herder[idx3].sheep--;
						// find a free sheep to start
						// Sheep start on grid offsets
						x=x2*GRIDSIZE+PLAYFIELDXOFF;
						y=y2*GRIDSIZE+PLAYFIELDYOFF;
						RestartSheep(x,y,xstep,ystep);
					}
					// lose all ghost sheep too
					herder[idx3].ghostsheep = 0;
				}
			}
		}
	}
} 

// Warning: Thalia's map is hand editted to fix an error in the horizontal beam start frames (17 and 19)
void HandleThalia() {
	int idx;
	
	// Thalia's special is a wide-bean cannon in front of her, electrocuting
	// any herder it hits (with the usual sheep stunning effect). Since it's
	// directional, it results in a longer stun than others. It is functionally
	// similar to NH-5's ribbon, but faster (but with a longer launch time)

	// On each turn, each element counts down. When it goes from 3, it spawns
	// the next element, if there is a next element to spawn. It will also
	// stop on walls, so must be fired in a valid direction. Otherwise, it is
	// wasted, we will not auto-aim this one.

	// Start the sound effect for Thalia Beam
	sound_effect_player(playeridx, SND_THALIAPOWER1+(ch_rand()%MAXSND_SPECIAL), SPECIALVOL);
	
	// zero most of the struct
	for (idx=1; idx<NUM_SPECIALS; idx++) {
		specialDat[playeridx][idx].type = SP_NONE;
	}
	// set the first object into motion
	specialDat[playeridx][0].type = SP_BEAM;
	specialDat[playeridx][0].animframe = -20;	// will be 2 after a delay countdown
	specialDat[playeridx][0].misccnt = THALIASPEED;	// animation timing count
	memcpy(&specialDat[playeridx][0].spr, &herder[playeridx].spr, sizeof(SPRITE));
	specialDat[playeridx][0].spr.x += xstep;
	specialDat[playeridx][0].spr.y += ystep;
	specialDat[playeridx][0].spr.xd = sgn(xstep);
	specialDat[playeridx][0].spr.yd = sgn(ystep);
	
	// set up animation frames
	if (xstep > 0) {
		specialDat[playeridx][0].spr.tilenumber = BeamFrames[SHEEPRIGHT][0][0];
		specialDat[playeridx][0].destx = SHEEPRIGHT;	// direction variable
		herder[playeridx].spr.tilenumber = 26;
	} else if (xstep < 0) {
		specialDat[playeridx][0].spr.tilenumber = BeamFrames[SHEEPLEFT][0][0];
		specialDat[playeridx][0].destx = SHEEPLEFT;	// direction variable
		herder[playeridx].spr.tilenumber = 6;
	} else if (ystep > 0) {
		specialDat[playeridx][0].spr.tilenumber = BeamFrames[SHEEPDOWN][0][0];
		specialDat[playeridx][0].destx = SHEEPDOWN;	// direction variable
		herder[playeridx].spr.tilenumber = 16;
	} else {
		specialDat[playeridx][0].spr.tilenumber = BeamFrames[SHEEPUP][0][0];
		specialDat[playeridx][0].destx = SHEEPUP;	// direction variable
		herder[playeridx].spr.tilenumber = 36;
	}
	
	herder[playeridx].specialFreeze = 10;		// we won't let this count down normally
}

// update one Thalia laser segment, they count down their own life and
// try to spawn a new one when needed (higher only)
// if nCountdown is not -1, don't capture or steal sheep
void ProcessThalia(int who, int idx, int nCountdown) {
	int x1,y1,x2,y2;
	int idx3;
	int x,y;

	// just hold off if we are counting up the audio delay
	if (specialDat[who][idx].animframe < 0) {
		specialDat[who][idx].animframe++;
		if (specialDat[who][idx].animframe >= 0) {
			specialDat[who][idx].animframe=2;	// true first frame
		}
		herder[who].specialFreeze = 10;
		return;
	}
	
	// allow the countdown after the first one is freed
	if (specialDat[who][0].type != SP_NONE) {
		herder[who].specialFreeze = 5;
	}
	
	// set up the special locations for testing	
	x1=specialDat[who][idx].spr.x;
	y1=specialDat[who][idx].spr.y;
	// so now is the correct time, for each valid direction 
	// we spawn only if there is a valid structure to load into
	x2=((x1-PLAYFIELDXOFF)+GRIDSIZE/2)/GRIDSIZE;
	y2=((y1-PLAYFIELDYOFF)+GRIDSIZE/2)/GRIDSIZE;

	// we are also dead if we are on a non-flat cell
	if ((LevelData[y2][x2].isPassable==false) && (LevelData[y2][x2].is3D) && (LevelData[y2][x2].nDestructible==0)) {
		// we wait till now so there is some brief visual indication why the special failed
		specialDat[who][idx].type = SP_NONE;
		return;
	}

	// check delay first
	if (specialDat[who][idx].misccnt-- <= 0) {
		// check if this segment is dead
		if (specialDat[who][idx].animframe == 0) {
			// yes it is, end it
			specialDat[who][idx].type = SP_NONE;
			return;
		} 

		// check if we need to spawn a new piece
		if ((specialDat[who][idx].animframe == 2) && (idx < 7)) {
			if (specialDat[who][idx+1].type == SP_NONE) {
				memcpy(&specialDat[who][idx+1], &specialDat[who][idx], sizeof(SpecialStruct));
				specialDat[who][idx+1].animframe=2;
				// they all get the same lifetime, unlike NH-5
				specialDat[who][idx+1].misccnt=THALIASPEED;
				specialDat[who][idx+1].spr.x += specialDat[who][idx+1].spr.xd*GRIDSIZE;
				specialDat[who][idx+1].spr.y += specialDat[who][idx+1].spr.yd*GRIDSIZE;
			}
		}

		// next frame - special case for first segment
		if ((idx == 0) && (specialDat[who][idx].animframe==2))  {
			specialDat[who][idx].animframe = 0;	// skip the middle piece
			specialDat[who][idx].misccnt = THALIASPEED*7;
		} else {
			// everyone else works normally
			specialDat[who][idx].animframe--;
			if (specialDat[who][idx].animframe == 0) {
				// lives for 7 frames
				specialDat[who][idx].misccnt = THALIASPEED*7;
			} else {
				// this one shouldn't happen anymore
				specialDat[who][idx].misccnt = THALIASPEED;
			}
		}
	} 
 
	// update the animation frame
	if (idx == 0) {
		specialDat[who][idx].spr.tilenumber = BeamFrames[specialDat[who][idx].destx][0][(myJiffies>>1)&1];
	} else {
		specialDat[who][idx].spr.tilenumber = BeamFrames[specialDat[who][idx].destx][1][(myJiffies>>1)&1];
	}

	// destroy any destructible objects we hit
	if (LevelData[y2][x2].nDestructible == 1) {
		LevelData[y2][x2].nDestructible=2;
		herder[who].score+=10;
	}
		
	// check for sheep
	// Check for stunned sheep
	for (idx3=0; idx3<MAX_SHEEP; idx3++) {
		if ((sheep[idx3].stuntime > 0) || (sheep[idx3].type < 1)) {
			// Can't re-zap a stunned sheep
			continue;
		}
		if (!sheep[idx3].invincible) {
			x=(int)sheep[idx3].spr.x;
			x-=x1;
			if (x<0) x=x*(-1);
			y=(int)sheep[idx3].spr.y;
			y-=y1;
			if (y<0) y=y*(-1);
			if ((x<28)&&(y<28)) {
				// got this sheep :)
				herder[who].score+=10;
				sheep[idx3].stuntime=90;	// 3 second stun time
			}
		}
	}

	// How about other herders?
	for (idx3=0; idx3<4; idx3++) {
		if ((idx3!=who)&&((herder[idx3].type&PLAY_MASK)!=PLAY_NONE)) {	// don't check ourselves or non-players
			// check for collision
			x=herder[idx3].spr.x;
			x-=x1;
			y=herder[idx3].spr.y;
			y-=y1;
			if ((abs(x)<28)&&(abs(y)<28)) {
				// if a first time hit, add some special
				if (herder[idx3].stuntime == 0) {
					herder[idx3].special+=5;
					if (herder[idx3].special > MAXSPECIAL) {
						herder[idx3].special = MAXSPECIAL;
					}
				}

				// got this guy :) (even if stunned!)
				if (herder[idx3].stuntime > 0) {
					// to help the animation along, we cheat a bit here
					// we don't touch the frame, and we try to keep the last
					// few bits of the time untouched
					herder[idx3].stuntime |= 0x3C;
				} else {
					// first hit
					herder[idx3].stunframe = 9;		// first electrified frame
					herder[idx3].stuntime = 63;		// 2 second stun time
				}
				herder[idx3].destx = herder[idx3].spr.x;
				herder[idx3].desty = herder[idx3].spr.y;	// no movement
				herder[who].score+=10;			// will probably score multiple hits
				
				// play the other players hit sound (throttled)
				sound_effect_player_throttled(idx3, SND_ZEUSDAMAGE1+(ch_rand()%MAXSND_DAMAGE), PLAYERVOL);
				
				herder[idx3].charge = 0;	// zero the victim's lightning gauge
				
				if (nCountdown == -1) {
					// lose all sheep, if he has any
					while (herder[idx3].sheep>0) {
						herder[idx3].sheep--;
						// find a free sheep to start
						// Sheep start on grid offsets
						x=x2*GRIDSIZE+PLAYFIELDXOFF;
						y=y2*GRIDSIZE+PLAYFIELDYOFF;
						RestartSheep(x,y,xstep,ystep);
					}
					// lose all ghost sheep too
					herder[idx3].ghostsheep = 0;
				}
			}
		}
	}
}

void HandleTrey() {
	int idx;
	
	// Trey Volta's special is a large disco ball that rezzes above him.
	// For a larger range, it causes all sheep to dance towards him, and all herders to lose
	// their sheep and dance along.
	// So here, we need to create the first and only element.
	// On each turn, we update the sheep animation and time the effect.
	// It warps in and out like the gumball but is overtop of the background and players

	// Start the sound effect for discoball
	sound_effect_player(playeridx, SND_TREYVOLTAPOWER+(ch_rand()%MAXSND_SPECIAL), SPECIALVOL);
	
	// zero unused part of the struct
	for (idx=1; idx<NUM_SPECIALS; idx++) {
		specialDat[playeridx][idx].type = SP_NONE;
	}
	// set up the disco ball
	specialDat[playeridx][0].type = SP_GIANTDISCO;
	specialDat[playeridx][0].animframe = -1;	// dropping
	specialDat[playeridx][0].misccnt = 1;		// disco ball range
	memcpy(&specialDat[playeridx][0].spr, &herder[playeridx].spr, sizeof(SPRITE));
	specialDat[playeridx][0].spr.xd = 0;
	specialDat[playeridx][0].spr.yd = 0;
	specialDat[playeridx][0].spr.x-=16;		// adjust for first sprite of 4
	specialDat[playeridx][0].spr.y-=16;
	specialDat[playeridx][0].desty = specialDat[playeridx][0].spr.y - GRIDSIZE*2 - 10;	// adjust for above Trey
	specialDat[playeridx][0].spr.y = -48;				// start off screen
	specialDat[playeridx][0].spr.z -= (fx16)0x60;		// overtop of all
	specialDat[playeridx][0].oldx = 260;				// duration of disco ball
	specialDat[playeridx][0].oldy = 0;					// animation frame counter

	// we cheat and use some of the others as flags just to see if we caught the other herders (just for audio today)
	specialDat[playeridx][1].misccnt=0;				// we don't bother here to check which one is us ;)
	specialDat[playeridx][2].misccnt=0;
	specialDat[playeridx][3].misccnt=0;
	specialDat[playeridx][4].misccnt=0;

	// set up animation frames
	herder[playeridx].spr.tilenumber = 6;
	herder[playeridx].specialFreeze = 10;	// temporary value
}

// update Trey's disco ball.
// if nCountdown is not -1, don't capture or steal sheep
void ProcessTrey(int who, int idx, int nCountdown) {
	int x1,y1;
	int idx3;
	int nRange;
	int x,y;
	int nDistance;

	// are we animating it, first?
	if (specialDat[who][idx].animframe == -1) {
		// yes, work the direction
		if (specialDat[who][idx].desty > -100) {
			// dropping in

			// no freeze countdown during rez frames
			herder[who].specialFreeze = 5;		// we won't let this count down normally

			specialDat[who][idx].spr.y += specialDat[who][idx].desty/15+1;
			if (specialDat[who][idx].spr.y >= specialDat[who][idx].desty) {
				specialDat[who][idx].spr.y = specialDat[who][idx].desty;
				specialDat[who][idx].misccnt = 0;
				specialDat[who][idx].animframe = 0;		// dance next frame
				specialDat[who][idx].desty = -200;		// so leave when animframe becomes -1 again
			}
			// render it here so we can exit
			goto drawball;
		}
		// else, flying up
		specialDat[who][idx].spr.y -= 32;
		if (specialDat[who][idx].spr.y <= 0) {
			// then, it's over
			specialDat[who][idx].type = SP_NONE;
			return;
		}
		goto drawball;
	}

	// animation speed counter
	specialDat[playeridx][0].oldy++;
	if (specialDat[playeridx][0].oldy >= 3) {
		specialDat[playeridx][0].oldy = 0;
	}

	// update the range, if appropriate
	if (specialDat[playeridx][idx].misccnt < 120) {
		specialDat[playeridx][idx].misccnt += 12;
	} else {
		// count down the disco ball
		specialDat[who][idx].oldx--;
		if (specialDat[who][idx].oldx < 1) {
			// send the ball away
			specialDat[who][idx].animframe = -1;
		}
	}
			
	// count up the global dance frame
	if (specialDat[playeridx][0].oldy == 0) {
		specialDat[who][idx].animframe++;
		if (specialDat[who][idx].animframe >= 44) specialDat[who][idx].animframe=0;
	}
	
	// Trey dances for the duration
	herder[who].specialFreeze = 5;
	herder[who].spr.tilenumber = GetDanceFrame();

	// destructibles are handled in the level draw code
	
	x1=herder[who].spr.x;
	y1=herder[who].spr.y;
	
	// calculate range
	nRange = specialDat[who][idx].misccnt * specialDat[who][idx].misccnt;
	
	// check for sheep
	// Check for stunned sheep
	for (idx3=0; idx3<MAX_SHEEP; idx3++) {
		if ((sheep[idx3].stuntime > 0) || (sheep[idx3].type < 1)) {
			// Can't re-zap a stunned sheep
			continue;
		}
		if (!sheep[idx3].invincible) {
			x=(int)sheep[idx3].spr.x;
			x-=x1;
			y=(int)sheep[idx3].spr.y;
			y-=y1;
			nDistance = (x*x)+(y*y);
			if (nDistance < nRange) {
				// got this sheep :)
				herder[who].score+=10;
				sheep[idx3].stuntime=60;	// 2 second stun time (this is fine, all stunned sheep dance during dance specials)
			}
		}
	}

	// How about other herders?
	for (idx3=0; idx3<4; idx3++) {
		if ((idx3!=who)&&((herder[idx3].type&PLAY_MASK)!=PLAY_NONE)) {	// don't check ourselves or non-players
			// check for collision
			x=herder[idx3].spr.x;		// x is centered
			x-=x1;	
			y=herder[idx3].spr.y-16;	// y is BOTTOM based
			y-=y1;
			nDistance = (x*x) + (y*y);
			if (nDistance < nRange) {
				// if a first time hit, add some special
				if (herder[idx3].stuntime == 0) {
					herder[idx3].special+=5;
					if (herder[idx3].special > MAXSPECIAL) {
						herder[idx3].special = MAXSPECIAL;
					}
				}

				// got this guy :) (even if stunned!)
				herder[idx3].stuntime = 15;		// 1/2 second stun time
				herder[idx3].stunframe = -1;	// dancing animation frame
				herder[who].score+=10;			// will probably score multiple hits (up to 8700 points?)
				
				// play the other players hit sound (throttled)
				// hearing it over and over again is annoying, so we set a flag to only play it once
				if (specialDat[who][idx3+1].misccnt == 0) {
					specialDat[who][idx3+1].misccnt = 1;
					sound_effect_player_throttled(idx3, SND_ZEUSDAMAGE1+(ch_rand()%MAXSND_DAMAGE), PLAYERVOL);
				}

				herder[idx3].charge = 0;	// zero the victim's lightning gauge
								
				if (nCountdown == -1) {
					// lose all sheep, if he has any
					while (herder[idx3].sheep>0) {
						herder[idx3].sheep--;
						// find a free sheep to start
						// Sheep start on grid offsets
						x=((herder[idx3].spr.x-PLAYFIELDXOFF)/GRIDSIZE);
						x=x*GRIDSIZE+PLAYFIELDXOFF;
						y=((herder[idx3].spr.y-PLAYFIELDYOFF)/GRIDSIZE);
						y=y*GRIDSIZE+PLAYFIELDYOFF;
						RestartSheep(x,y,xstep,ystep);
					}
					// lose all ghost sheep too
					herder[idx3].ghostsheep = 0;
				}
			}
		}
	}
	
drawball:
	// draw the giant disco ball (4 sprites) - note each sprite is independently animated
	// to give the illusion of more animation frames
	int nFrame = (myJiffies>>2)%7;
	specialDat[who][idx].spr.tilenumber = GiantDiscoFrames[nFrame][0];
	SortSprite(&specialDat[who][idx].spr, herder[who].map, POLY_SPECIAL);

	specialDat[who][idx].spr.y+=GRIDSIZE;
	specialDat[who][idx].spr.tilenumber = GiantDiscoFrames[nFrame][1];
	SortSprite(&specialDat[who][idx].spr, herder[who].map, POLY_SPECIAL);

	specialDat[who][idx].spr.x+=GRIDSIZE;
	specialDat[who][idx].spr.tilenumber = GiantDiscoFrames[nFrame][3];
	SortSprite(&specialDat[who][idx].spr, herder[who].map, POLY_SPECIAL);

	specialDat[who][idx].spr.y-=GRIDSIZE;
	specialDat[who][idx].spr.tilenumber = GiantDiscoFrames[nFrame][2];
	SortSprite(&specialDat[who][idx].spr, herder[who].map, POLY_SPECIAL);

	specialDat[who][idx].spr.x-=GRIDSIZE;
}

void HandleHades() {
	int idx;
	
	// Hades' special grants him the power of high speed, the ability to capture sheep as ghosts.
	// During his power, he can not use lightning, but can destroy crates on touch as well as
	// knock opponents sheep free.
	// He has no actual special objects to manage. (We still use the struct to manage it).
	// He has to stay in specialFreeze count 1 so he can run around during it.

	// Start the sound effect for power of the moon
	sound_effect_player(playeridx, SND_HADESPOWER+(ch_rand()%MAXSND_SPECIAL), SPECIALVOL);
	
	// zero unused part of the struct
	for (idx=1; idx<NUM_SPECIALS; idx++) {
		specialDat[playeridx][idx].type = SP_NONE;
	}
	// set up the wolf - not too much data here since there is no sprite object to draw
	specialDat[playeridx][0].type = SP_WOLF;
	specialDat[playeridx][0].misccnt = 210;		// roughly 7 seconds worth of power
	specialDat[playeridx][0].destx = herder[playeridx].speed;	// save the original speed
	herder[playeridx].speed = HERDERMAXSPEED+2;
	herder[playeridx].charge = 0;			// no lightning!

	// set up animation frames
	if (xstep < 0) {
		herder[playeridx].spr.tilenumber = 6;
	} else {
		herder[playeridx].spr.tilenumber = 26;
	}
	herder[playeridx].specialFreeze = 55;	// 2 seconds of animation delay
}

// Countdown Hades' rampage - no object to draw
// if nCountdown is not -1, don't capture or steal sheep
void ProcessHades(int who, int idx, int nCountdown) {
	int x1,y1,x2,y2,x3,y3;
	int idx3;
	int x,y;
		
	// all we are really checking is whether the rampage is still on
	if (herder[who].specialFreeze <= 1) {
		// if the rampage is still on, update specialFreeze so it stays locked
		if (specialDat[who][0].misccnt > 0) {
			specialDat[who][0].misccnt--;
			herder[who].specialFreeze = 2;	// so that it counts down to 1
		} else {
			// we're done
			specialDat[who][idx].type = SP_NONE;
			herder[playeridx].speed = specialDat[playeridx][0].destx;	// restore the original speed (note any speed powerups found will be lost!)
			return;
		}
	}
	
	// update special modifiers
	herder[who].charge = 0;			// no lightning!

	// set up the special locations for testing	
	x1=herder[who].spr.x;
	y1=herder[who].spr.y;
	x2=((x1-PLAYFIELDXOFF)+GRIDSIZE/2)/GRIDSIZE;
	y2=((y1-PLAYFIELDYOFF)+GRIDSIZE/2)/GRIDSIZE;
	x3=x2*GRIDSIZE+PLAYFIELDXOFF;
	y3=y2*GRIDSIZE+PLAYFIELDYOFF;

	// destroy any destructible objects we're on (needs to be legal to run over them)
	// we have to be super loose with this, to avoid Hades getting stuck at the end of
	// his turn (due to the sloppy collision detection system)
	if (LevelData[y2][x2].nDestructible == 1) {
		LevelData[y2][x2].nDestructible=2;
		herder[who].score+=10;
	}
	if (x3>x1) {
		if (LevelData[y2][x2+1].nDestructible == 1) {
			LevelData[y2][x2+1].nDestructible=2;
			herder[who].score+=10;
		}
	}
	if (x3<x1) {
		if (LevelData[y2][x2-1].nDestructible == 1) {
			LevelData[y2][x2-1].nDestructible=2;
			herder[who].score+=10;
		}
	}
	if (y3>y1) {
		if (LevelData[y2+1][x2].nDestructible == 1) {
			LevelData[y2+1][x2].nDestructible=2;
			herder[who].score+=10;
		}
	}
	if (y3<y1) {
		if (LevelData[y2-1][x2].nDestructible == 1) {
			LevelData[y2-1][x2].nDestructible=2;
			herder[who].score+=10;
		}
	}
	
	// sheep are handled in the normal capture routine
	
	// How about other herders?
	for (idx3=0; idx3<4; idx3++) {
		if ((idx3!=who)&&((herder[idx3].type&PLAY_MASK)!=PLAY_NONE)) {	// don't check ourselves or non-players
			// check for collision
			x=herder[idx3].spr.x;
			x-=x1;
			y=herder[idx3].spr.y;
			y-=y1;
			if ((abs(x)<28)&&(abs(y)<28)) {
				// if a first time hit, add some special
				if (herder[idx3].stuntime == 0) {
					herder[idx3].special+=5;
					if (herder[idx3].special > MAXSPECIAL) {
						herder[idx3].special = MAXSPECIAL;
					}
				}

				// got this guy :) (even if stunned!)
				herder[idx3].stuntime = 15;		// 1/2 second stun time
				herder[idx3].stunframe = -1;	// dancing animation frame
				herder[idx3].destx = herder[idx3].spr.x - herder[who].spr.xd;
				herder[idx3].desty = herder[idx3].spr.y - herder[who].spr.yd;	// knock back the opposite direction we are going
				herder[who].score+=10;			// will probably score multiple hits
				
				// play the other players hit sound (throttled)
				sound_effect_player_throttled(idx3, SND_ZEUSDAMAGE1+(ch_rand()%MAXSND_DAMAGE), PLAYERVOL);

				herder[idx3].charge = 0;	// zero the victim's lightning gauge
								
				if (nCountdown == -1) {
					// lose all sheep, if he has any
					while (herder[idx3].sheep>0) {
						herder[idx3].sheep--;
						// find a free sheep to start
						{
							// Sheep start on grid offsets
							RestartSheep(x3,y3,xstep,ystep);
						}
					}
					// lose all ghost sheep too
					herder[idx3].ghostsheep = 0;
				}
			}
		}
	}
}

void HandleZombie() {
	int idx;
	
	// Zombie's special grants him dark lightning. He himself gets full lightning, and the
	// lightning lets him steal speed from others (and can free ghost sheep 2 at a time)
	// He has no actual special objects to manage. (We still use the struct to manage it).
	// He has to stay in specialFreeze count 1 so he can run around during it.

	// Start the sound effect for black lightning
	sound_effect_player(playeridx, SND_ZOMBIEPOWER+(ch_rand()%MAXSND_SPECIAL), SPECIALVOL);
	
	// zero unused part of the struct
	for (idx=1; idx<NUM_SPECIALS; idx++) {
		specialDat[playeridx][idx].type = SP_NONE;
	}
	// set up the zombie - not too much data here since there is no sprite object to draw
	specialDat[playeridx][0].type = SP_ZOMBIE;
	specialDat[playeridx][0].misccnt = 150;		// roughly 5 seconds worth of power
	specialDat[playeridx][0].destx = herder[playeridx].range;	// save the original lightning
	herder[playeridx].range = MAXLIGHTNING;

	// set up animation frames
	if (xstep > 0) {
		herder[playeridx].spr.tilenumber = 26;
	} else if (xstep < 0) {
		herder[playeridx].spr.tilenumber = 6;
	} else if (ystep > 0) {
		herder[playeridx].spr.tilenumber = 16;
	} else {
		herder[playeridx].spr.tilenumber = 36;
	}
	herder[playeridx].specialFreeze = 55;	// 2 seconds of animation delay
}

// Countdown Zombie's attack - no object to draw
// if nCountdown is not -1, don't capture or steal sheep
void ProcessZombie(int who, int idx, int /*nCountdown*/) {
	// all we are really checking is whether the rampage is still on
	if (herder[who].specialFreeze <= 1) {
		// if the rampage is still on, update specialFreeze so it stays locked
		if (specialDat[who][0].misccnt > 0) {
			specialDat[who][0].misccnt--;
			herder[who].specialFreeze = 2;	// so that it counts down to 1
		} else {
			// we're done
			specialDat[who][idx].type = SP_NONE;
			herder[playeridx].range = specialDat[playeridx][0].destx;	// restore the original range (note any lightning powerups found will be lost!)
			return;
		}
	}
}

// modify a sprite object from normal to zombie lightning and sort it
void DrawZombieLightning(SPRITE spr, int who) {
	if (spr.tilenumber > 32) return;
	
	spr.txr_addr=herder[who].spr.txr_addr;
	spr.pal_addr=herder[who].spr.pal_addr;
	spr.tilenumber = DarkLightningMap[spr.tilenumber];
	
	SortSprite(&spr, herder[who].map, POLY_MISC);
}

// parse out a special to the correct handler
// return 1 to just do lightning (Chrys) :)
int HandleSpecial(int i_idx, int i_xstep, int i_ystep, int i_currentx, int i_currenty, int i_oldtile) {
	playeridx=i_idx;
	xstep=i_xstep;
	ystep=i_ystep;
	currentx=i_currentx;
	currenty=i_currenty;
	oldtile=i_oldtile;

	switch (herder[i_idx].type & PLAY_CHAR_MASK) {
		// value is 0-12
		case HERD_AFROZEUS: // fall through
		case HERD_ZEUS:	 	HandleZeus(); 	break;
		case HERD_HERD:	 	HandleHerder(); break;
		case HERD_CANDY:	HandleCandy(); 	break;
		case HERD_NH5:	 	HandleNH5();  	break;
		case HERD_DANCER:  	HandleDancer(); break;
		case HERD_ZOMBIE:  	HandleZombie(); break;
		case HERD_THALIA:  	HandleThalia(); break;
		case HERD_ISKUR:  	HandleIskur();  break;
		case HERD_ANGEL:  	HandleAngel();  break;
		case HERD_WOLF:  	HandleHades();  break;
		case HERD_GODDANCE: HandleTrey();   break;
		case HERD_DEVIL: 	HandleDemon();  break;

		default:
			// Chrys is a special case, normal lightning is her special
			return 1;
	}
	
	return 0;
}

// zero out the specials structure
void InitSpecials() {
	memset(specialDat, 0, sizeof(specialDat));
}

// this function handles any active special commands for the current
// player. It returns a player animation frame if the player should not
// move, otherwise it returns -1 and the player may move normally.
// if nCountdown is not -1, don't steal or capture sheep
int ProcessSpecials(int who, int nCountdown) {
	int nReturn = -1;
	int idx;
	
	// first check for freeze countdown and, if set, return an
	// appropriate animation frame
	if (herder[who].specialFreeze > 0) {
		herder[who].specialFreeze--;
		if (herder[who].specialFreeze > 1) {
			// Candy Striper locks at 1 in order to keep the special active while
			// she moves, everyone else frees up one frame early
			nReturn = herder[who].spr.tilenumber;
		}
	}
	
	// check if we are being stunned by a special
	if ((herder[who].stuntime > 0) && (herder[who].stunframe != 0)) {
		// we also have to count down the stuntime in this case
		if ((--herder[who].stuntime) == 0) {
			herder[who].stunframe=0;
		} else {
			// Disco is not handled here (stunframe==-1)
			if (herder[who].stunframe != -1) {
				// there are only two other possibilities - a gift box (4), or shocked animation (9/14)
				if (herder[who].stunframe == 4) {
					nReturn = herder[who].stunframe;	// stay a gift box
				} else if ((herder[who].stuntime&0x03)==0) {
					// otherwise, animate the shocked effect
					if (herder[who].stunframe == 9) {
						herder[who].stunframe=14;
					} else {
						herder[who].stunframe=9;
					}
				}
			}
			nReturn=herder[who].stunframe;
		}
	}

	// now check if they have any objects to be moved
	for (idx=0; idx<NUM_SPECIALS; idx++) {
		if (specialDat[who][idx].type == SP_NONE) continue;
		switch (specialDat[who][idx].type) {
			case SP_GIANTDISCO:		// Trey's giant disco ball
				ProcessTrey(who, idx, nCountdown);
				continue;			// sorts the sprite itself as it needs to sort 4 of them
				
			case SP_BEAM:			// Thalia's power beam
				ProcessThalia(who, idx, nCountdown);
				break;

			case SP_RIBBON:			// NH-5's ribbon wrap
				ProcessNH5(who, idx, nCountdown);
				break;

			case SP_FLOOD:			// Iskur's flash flood
				ProcessIskur(who, idx, nCountdown);
				break;
				
			case SP_KIWI:			// classic herder's Kiwi bird
				ProcessHerder(who, idx, nCountdown);
				break;

			case SP_DEMONBLAST:		// demon blast wave
				ProcessDemon(who, idx, nCountdown);
				continue;			// sorts the sprite itself since it's not always visible
				
			case SP_DISCOBALL:		// backup dancer's small disco ball
				ProcessDancer(who, idx, nCountdown);
				continue;			// sorts the sprite itself due to special effects

			case SP_GUMBALL:		// candy striper's giant gumball
				ProcessCandy(who, idx, nCountdown);
				continue;			// sorts the sprite itself due to special effects

			case SP_ANGELLIGHT:		// Angel's light
				ProcessAngel(who, idx, nCountdown);
				continue;			// do not sort any sprite
				
			case SP_LIGHTNING:		// Zeus's lightning blast
				ProcessZeus(who, idx, nCountdown);
				break;
				
			case SP_WOLF:			// Hades rampage
				ProcessHades(who, idx, nCountdown);
				continue;			// no object to draw
				
			case SP_ZOMBIE:			// zombie mambo
				ProcessZombie(who, idx, nCountdown);
				continue;			// no object to draw

		};
		// sort the sprite if it's still valid (Note some don't come down to here!)
		if (specialDat[who][idx].type != SP_NONE) {
			SortSprite(&specialDat[who][idx].spr, herder[who].map, POLY_SPECIAL);
		}
	}
	
	return nReturn;
}

/*
HERD_AFROZEUS	- same as Zeus
HERD_ZEUS		- fires a large lightning bolt that spreads through the maze in front of him. Stuns sheep, destroys crates, frees all captured sheep and electrocutes herders
HERD_HERD		- calls four giant kiwis who run through the maze at random. Stuns sheep, destroys crates, frees all captured sheep
HERD_CANDY		- calls a giant gumball which can be steered through the maze. Stuns sheep, destroys crates, frees all captured sheep
HERD_NH5		- deploys a giant ribbon in front of him. Stuns sheep, destroys crates, frees all captured sheep and giftwraps herders.
HERD_DANCER		- deploys a small disco ball. Sheep and herders in the light are forced to dance. Destroys crates, frees all captured sheep. She can move.	
HERD_ZOMBIE		- gives temporary black lightning. Black lightning captures sheep as ghosts, frees 2 sheep at a time from herders and steals speed.
HERD_THALIA		- fires a plasma beam in front of her. Stuns sheep, destroys crates, frees all captured sheep and electrocutes herders
HERD_ISKUR		- deploys a flood in front and behind himself. Stuns sheep, destroys crates, passes under walls, frees all captured sheep and pushes herders
HERD_ANGEL		- deploys a circle of light around itself. Captures sheep, destroys crates, frees all captured sheep
HERD_WOLF		- activates power of the moon. Wolf can not fire lightning but moves fast, can destroy crates, capture sheep as ghosts and free all captured sheep from herders
HERD_GODDANCE	- deploys large disco ball with large radius of effect. Destroys crates, frees all captured sheep and forces herders to dance. Sheep dance towards him, but he can not move.
HERD_DEVIL		- fires 8 way explosion that passes through walls. Destroys crates, stuns sheep, frees all captured sheep
HERD_AFROCHRYS	- same as Chrys
HERD_CHRYS		- fires a brief, continuous lightning blast with normal effect (secret character)
*/
