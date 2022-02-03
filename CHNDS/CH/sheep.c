/****************************************/
/* Cool Herders                         */
/* http://harmlesslion.com              */
/* Copyright 2009 HarmlessLion LLC      */
/* sheep.c                              */
/****************************************/

/* Ported to KOS 1.1.x by Dan Potter */

#include <stdio.h>
#include <string.h>

#include "kosemulation.h"

#include "sprite.h"
#include "cool.h"
#include "rand.h"
#include "collide.h"
#include "levels.h"
#include "sheep.h"
#include "menu.h"	// for globals
#include "sound.h"
#include "storymode.h"
#include "special.h"
#include "chwireless.h"

// We read level 0 (NZ Island) for the ghost sheep
extern char Level[NUMLEVELS][LEVELBYTES];
extern int nFrames;
extern pvr_ptr_t pal_sprites, pal_sheep, pal_level, pal_herder[6];
extern char bSpecialDim;
extern SpecialStruct specialDat[4][8];
extern volatile unsigned int myJiffies;
extern int nTotalSheep;
extern int gStageSpecialEffect;
extern int g_DestructiblesLeft;

// Sheep animation frames
// [Direction 0-3][Frame 0-3]
int SheepAnimationFrame[4][4] = {
	{	0, 10, 20, 30,	},
	{	1, 11, 21, 31,	},
	{	2, 12, 22, 32,	},
	{	3, 13, 23, 33	}
};

int WallDirTabX[4] = {
	-GRIDSIZE, -GRIDSIZE, GRIDSIZE, GRIDSIZE
};
int WallDirTabY[4] = {
	GRIDSIZE, -GRIDSIZE, GRIDSIZE, -GRIDSIZE
};

int SheepSpeed=NORMALSHEEPSPEED;
SheepStruct sheep[MAX_SHEEP];

// forward declaration
void updatesheeppos(SheepStruct *pHerd);
void FindSheepPosition(int idx);

// Find a legal location for this sheep - used by initSheep and when
// relocating a sheep out of bounds. I don't think this really matters
// anymore, so to reduce rand hits, just sets them in the center. RestartSheep()
// does the real work now.
void FindSheepPosition(int idx) {
	int x,y;

	x=10;
	y=8;	
	do {
//		x=(int)((ch_rand()%7-3)+(LEVELXSIZE/2-1));
//		y=(int)(ch_rand()%LEVELYSIZE);
		if (gStageSpecialEffect&STAGE_EFFECT_GHOST_SHEEP) {
			// A little more complex cause we didn't load the NZ level for ghost sheep
			// So we have to do a little parsing here.
			int pos;
			pos=y*LEVELXSIZE*3+x*3;
			if (Level[0][pos]=='3') break;
			if (Level[0][pos+1]=='.') break;
		} else {
			if (LevelData[y][x].isPassable) break;
		}
		++x;
		++y;
	} while (1);

	sheep[idx].spr.x=x*GRIDSIZE+PLAYFIELDXOFF;
	sheep[idx].spr.y=y*GRIDSIZE+PLAYFIELDYOFF;
}

int RestartSheep(int x, int y, int xstep, int ystep) {
	int idx4;
	int ret=-1;
	int nBackup = -1;	// to handle animated but invalid sheep, in case we need one

rescan:
	for (idx4=0; idx4<MAX_SHEEP; idx4++) {
		if (sheep[idx4].type==0) {
			ret=idx4;
			memset(&sheep[idx4], 0, sizeof(SheepStruct));
			sheep[idx4].type=3;		// 3x sheep
			sheep[idx4].invincible=1;
			sheep[idx4].range=90;	// 1+1/2 seconds of freedom!
			sheep[idx4].spr.nDepth=DEPTH_256x256x4;
			sheep[idx4].spr.txr_addr=txr_sheep;
			sheep[idx4].spr.pal_addr=pal_sheep;
			sheep[idx4].spr.tilenumber=DOWN_STAND_SPRITE;
			sheep[idx4].spr.z=(fx16)2032;
			sheep[idx4].spr.xd=1;
			sheep[idx4].spr.yd=0;
			sheep[idx4].maxframes=SHEEPFRAMES;
			sheep[idx4].recaptured=120;
			sheep[idx4].spr.x=x;
			sheep[idx4].spr.y=y;
			if ((xstep==0)&&(ystep==0)) {
				sheep[idx4].destx=0;
				sheep[idx4].desty=0;
			} else {
				sheep[idx4].destx=x+(xstep*5);
				sheep[idx4].desty=y+(ystep*5);
			}

			if (gStageSpecialEffect&STAGE_EFFECT_GHOST_SHEEP) {
				// Ghost sheep are transparent and '3d' to reduce glitches
				sheep[idx4].spr.alpha=SHEEP_ALPHA_GHOST;
				sheep[idx4].spr.is3D=true;
				// Furthermore, they don't follow the same walls! So we need
				// to make sure this sheep starts in a good place.
				// We'll very quickly check eight directions around us. If
				// we can't find a good starting point, then we'll throw the
				// sheep back into the middle. It's a ghost anyway. ;)
				if (checkblock(x, y, TYPE_GHOSTWALL)) {
					if (0==checkblock(x,y-GRIDSIZE,TYPE_GHOSTWALL)) {
						sheep[idx4].spr.y-=GRIDSIZE;
					} else if (0==checkblock(x-GRIDSIZE,y,TYPE_GHOSTWALL)) {
						sheep[idx4].spr.x-=GRIDSIZE;
					} else if (0==checkblock(x+GRIDSIZE,y,TYPE_GHOSTWALL)) {
						sheep[idx4].spr.x+=GRIDSIZE;
					} else if (0==checkblock(x,y+GRIDSIZE,TYPE_GHOSTWALL)) {
						sheep[idx4].spr.y+=GRIDSIZE;
					} else if (0==checkblock(x-GRIDSIZE,y-GRIDSIZE,TYPE_GHOSTWALL)) {
						sheep[idx4].spr.x-=GRIDSIZE;
						sheep[idx4].spr.y-=GRIDSIZE;
					} else if (0==checkblock(x+GRIDSIZE,y-GRIDSIZE,TYPE_GHOSTWALL)) {
						sheep[idx4].spr.x+=GRIDSIZE;
						sheep[idx4].spr.y-=GRIDSIZE;
					} else if (0==checkblock(x-GRIDSIZE,y+GRIDSIZE,TYPE_GHOSTWALL)) {
						sheep[idx4].spr.x-=GRIDSIZE;
						sheep[idx4].spr.y+=GRIDSIZE;
					} else if (0==checkblock(x+GRIDSIZE,y+GRIDSIZE,TYPE_GHOSTWALL)) {
						sheep[idx4].spr.x+=GRIDSIZE;
						sheep[idx4].spr.y+=GRIDSIZE;
					} else {
						// We give up - nowhere to put the sheep! So center it :) (Center must always be clear for ghosts!)
						debug("Can't find spot for ghost sheep, placing in center.");
						sheep[idx4].spr.x=10*GRIDSIZE+PLAYFIELDXOFF;
						sheep[idx4].spr.y=8*GRIDSIZE+PLAYFIELDYOFF;
					}
				}
			} else {
				// non-ghost sheep need legal start points too
				// rather than a random 8 direction search, we maintain a cyclical
				// search in 4 diagonal directions. This function gets hit fairly often
				// now depending on the stage, so we want even distribution.
				static int nDir=-1;
				if (checkblock(x, y, TYPE_WALL)) {
					// position is no good (probably center). Check next one diagonally, if no good, take the second one out (should be good!)
					int nDirX, nDirY;
					if (++nDir > 3) nDir = 0;
					nDirX = WallDirTabX[nDir];
					nDirY = WallDirTabY[nDir];
					x+=nDirX;
					y+=nDirY;
					if (!checkblock(x, y, TYPE_WALL)) {
						sheep[idx4].spr.x = x;
						sheep[idx4].spr.y = y;
					} else {
						x+=nDirX;
						y+=nDirY;
						if ((x>0) && (x<639) && (y>0) && (y<479) && (!checkblock(x, y, TYPE_WALL))) {
							sheep[idx4].spr.x = x;
							sheep[idx4].spr.y = y;
						} else {
							// We give up - nowhere to put the sheep! We know center is wrong, so place
							// at a herder start point based in nDir
							debug("Can't find spot for sheep, placing in corner %d.", nDir);
							switch (nDir) {
								default:	// should never hit
								case 0:
									sheep[idx4].spr.x=GRIDSIZE+GRIDSIZE+PLAYFIELDXOFF;
									sheep[idx4].spr.y=(3*GRIDSIZE)+PLAYFIELDYOFF;
									break;
								case 1:
									sheep[idx4].spr.x=(GRIDSIZE*(LEVELXSIZE-3))+PLAYFIELDXOFF;
									sheep[idx4].spr.y=(3*GRIDSIZE)+PLAYFIELDYOFF;
									break;
								case 2:
									sheep[idx4].spr.x=GRIDSIZE+GRIDSIZE+PLAYFIELDXOFF;
									sheep[idx4].spr.y=(GRIDSIZE*(LEVELYSIZE-3))+PLAYFIELDYOFF;
									break;
								case 3:
									sheep[idx4].spr.x=(GRIDSIZE*(LEVELXSIZE-3))+PLAYFIELDXOFF;
									sheep[idx4].spr.y=(GRIDSIZE*(LEVELYSIZE-3))+PLAYFIELDYOFF;
									break;
							}
						}
					}
				}
				sheep[idx4].spr.alpha=SHEEP_ALPHA_NORMAL;
				sheep[idx4].spr.is3D=false;
			}
			sheep[idx4].oldx=sheep[idx4].spr.x;
			sheep[idx4].oldy=sheep[idx4].spr.y;
			break;
		} else if (sheep[idx4].type < 0) {
			nBackup = idx4;
		}
	}	// if we couldn't find one (somehow!), no worries (we can try once more)

	if ((ret == -1) && (nBackup != -1)) {
		sheep[nBackup].type = 0;
		nBackup = -1;
		goto rescan;
	}

	return ret;
}

// Set up the sheep on this level
// LevelData must already be set!
void initSheep() {
	int idx;

	debug("Initializing Sheep\n");

	for (idx=0; idx<MAX_SHEEP; idx++) {
		memset(&sheep[idx], 0, sizeof(SheepStruct));
		sheep[idx].spr.nDepth=DEPTH_256x256x4;
		sheep[idx].spr.txr_addr=txr_sheep;
		sheep[idx].spr.pal_addr=pal_sheep;
		sheep[idx].spr.tilenumber=DOWN_STAND_SPRITE;
		FindSheepPosition(idx);
		sheep[idx].spr.z=(fx16)2032;
		sheep[idx].spr.xd=1;
		sheep[idx].spr.yd=0;
		if ((idx >= FIRST_SHEEP) || (gStageSpecialEffect & STAGE_EFFECT_NO_SHEEP)) {
			sheep[idx].type = 0;	// not active yet
		} else {
			sheep[idx].type = 2;	// active
		}
		sheep[idx].invincible=1;
		sheep[idx].maxframes=SHEEPFRAMES;
		sheep[idx].range=180;	// start with 3 seconds of invincibility
		sheep[idx].recaptured=0;
		sheep[idx].stuntime=0;

		if (gStageSpecialEffect&STAGE_EFFECT_GHOST_SHEEP) {
			// Ghost sheep are transparent and '3d' to reduce glitches
			sheep[idx].spr.alpha=SHEEP_ALPHA_GHOST;
			sheep[idx].spr.is3D=true;
			SheepSpeed=GHOSTSHEEPSPEED;
		} else {
			sheep[idx].spr.alpha=SHEEP_ALPHA_NORMAL;
			sheep[idx].spr.is3D=false;
			SheepSpeed=NORMALSHEEPSPEED;
		}
		SheepSpeed+=gOptions.SheepSpeed;
		
		sheep[idx].oldx=sheep[idx].spr.x;
		sheep[idx].oldy=sheep[idx].spr.y;
	}
}

// Move the sheep. This is a lot like the computer Herder movement
// This function is not called on the conveyor belt challenge
void moveSheep() {
	// count down deployment, and alternate flag in LSB
	static unsigned char nLastCenter = 1;

	int idx, tmp;
	uint32 mask;
	int currentx, currenty, gridx, gridy;
	int xstep, ystep, desiredx, desiredy;
	int origx, origy, loop;
	int nSheepLeft=0;
	bool bRecaptureSheep = false;

	for (idx=0; idx<MAX_SHEEP; idx++) {
		if (sheep[idx].type>0) {
			nSheepLeft++;
			if ((sheep[idx].recaptured)&&(!sheep[idx].range)) {
				sheep[idx].recaptured--;
			}
			for (loop=0; loop<sheep[idx].type; loop++) {
				origx=sheep[idx].spr.x;
				origy=sheep[idx].spr.y;

				if (sheep[idx].stuntime==0) {
movesheepagain:					
					int fNoCheck=0;

					// if we are currently in an illegal position, then allow any movement just to get back on track
					if (gStageSpecialEffect&STAGE_EFFECT_GHOST_SHEEP) {
						if (checkblock(sheep[idx].spr.x, sheep[idx].spr.y, TYPE_GHOSTWALL)) {
							fNoCheck=1;
						}
					} else {
						if (checkblock(sheep[idx].spr.x, sheep[idx].spr.y, TYPE_WALL|TYPE_BOX)) {
							fNoCheck=1;
						}
					}						

					if (!bRecaptureSheep) {
						// Check - if we didn't move, choose a new destination
						if ((sheep[idx].oldx==sheep[idx].spr.x)&&(sheep[idx].oldy==sheep[idx].spr.y)) {
							sheep[idx].destx=0;
						}
					}

					updatesheeppos(&sheep[idx]);

					if ((sheep[idx].destx==0)||(sheep[idx].desty==0)) {
						// new target - it doesn't have to be a legal destination,
						// as we'll pick a new one as soon as we get stuck :)
						sheep[idx].destx=(int)(ch_rand()%20) * GRIDSIZE;
						sheep[idx].desty=(int)(ch_rand()%15) * GRIDSIZE;
					}

					sheep[idx].spr.xd=sheep[idx].destx-sheep[idx].spr.x;
					sheep[idx].spr.yd=sheep[idx].desty-sheep[idx].spr.y;
					if (gStageSpecialEffect&STAGE_EFFECT_GHOST_SHEEP) {
						// Ghost sheep animate much more slowly
						if ((nGameFrames&15) == 0) {
							sheep[idx].animframe++;
						}
					} else {
						if ((nGameFrames&3) == 0) {
							sheep[idx].animframe++;
						}
					}

					if (sheep[idx].invincible) {
						sheep[idx].spr.alpha=SHEEP_ALPHA_INVINC;
						if (sheep[idx].range>0) {
							sheep[idx].range--;
						}
						if (sheep[idx].range==0) {
							sheep[idx].type=1;		// back to normal next frame
							sheep[idx].invincible=0;
							sheep[idx].spr.alpha=SHEEP_ALPHA_NORMAL;
						} else {
							if (sheep[idx].range<60) {
								sheep[idx].type=2;	// slow down in preparation...
							}
						}
					} else {
						if (gStageSpecialEffect&STAGE_EFFECT_GHOST_SHEEP) {
							sheep[idx].spr.alpha=SHEEP_ALPHA_GHOST;
						} else {
							sheep[idx].spr.alpha=SHEEP_ALPHA_NORMAL;
						}
					}

					if (sheep[idx].spr.xd>0) {
						sheep[idx].spr.x+=min(sheep[idx].spr.xd, SheepSpeed);
						sheep[idx].spr.xd=1;
					} else {
						sheep[idx].spr.x-=min(-sheep[idx].spr.xd, SheepSpeed);
						sheep[idx].spr.xd=-1;
					}
					if (sheep[idx].spr.yd>0) {
						sheep[idx].spr.y+=min(sheep[idx].spr.yd, SheepSpeed);
						sheep[idx].spr.yd=1;
					} else {
						sheep[idx].spr.y-=min(-sheep[idx].spr.yd, SheepSpeed);
						sheep[idx].spr.yd=-1;
					}

					if (sheep[idx].animframe >= sheep[idx].maxframes) {
						sheep[idx].animframe=0;
					}
					
					// Check legality of new position, and shift towards legal spot
					// Save current position to reduce indirections!
					currentx=sheep[idx].spr.x;
					currenty=sheep[idx].spr.y;
					gridx=((currentx-PLAYFIELDXOFF)+GRIDSIZE/2)/GRIDSIZE;
					gridy=((currenty-PLAYFIELDYOFF)+GRIDSIZE/2)/GRIDSIZE;
					xstep=(int)sheep[idx].spr.xd;
					ystep=(int)sheep[idx].spr.yd;
					desiredx=currentx;
					desiredy=currenty;

					if (gStageSpecialEffect&STAGE_EFFECT_GHOST_SHEEP) {
						if ((!sheep[idx].invincible)&&(!bRecaptureSheep)) {
							mask=TYPE_GHOSTWALL|TYPE_PLAY;
						} else {
							mask=TYPE_GHOSTWALL;		// only care about walls
						}
					} else {
						if ((!sheep[idx].invincible)&&(!bRecaptureSheep)) {
							mask=TYPE_WALL|TYPE_BOX|TYPE_PLAY;
						} else {
							mask=TYPE_WALL|TYPE_BOX;	// only care about walls
						}
					}
					
					if (xstep!=0) {
						// We're walking left or right.
						if ((!fNoCheck)&&(checkblock(currentx, origy, mask))) {
							// we're walking into a wall
							desiredx=origx;
						}
					} else {
						desiredx=gridx*GRIDSIZE+PLAYFIELDXOFF;
					}
					if (ystep!=0) {
						// We're walking up or down
						if ((!fNoCheck)&&(checkblock(origx, currenty, mask))) {
							desiredy=origy;
						}
					} else {
						desiredy=gridy*GRIDSIZE+PLAYFIELDYOFF;
					}
					if ((desiredx!=origx)&&(desiredy!=origy)) {
						// we're still trying to go diagonally, and both dirs are legal
						// We need to choose just one. We'd prefer the direction we
						// did not move last frame
						if (sheep[idx].oldx!=origx) {
							// we WERE going horizontal, so switch to vertical
							desiredx=origx;
						} else {
							// we WERE either going nowhere or vertical, take horizontal
							desiredy=origy;
						}
					}
					
					// Now adjust towards desired. We move at SheepSpeed to override movement
					// But not more pixels than are required :)
					if (desiredx>currentx) {
						sheep[idx].spr.x+=min(desiredx-currentx, SheepSpeed+1);
					}
					if (desiredx<currentx) {
						sheep[idx].spr.x-=min(currentx-desiredx, SheepSpeed+1);
					}
					if (desiredy>currenty) {
						sheep[idx].spr.y+=min(desiredy-currenty, SheepSpeed+1);
					}
					if (desiredy<currenty) {
						sheep[idx].spr.y-=min(currenty-desiredy, SheepSpeed+1);
					}
					// Done grid adjustment - make final legality check
					if ((!fNoCheck)&&(checkblock(sheep[idx].spr.x, sheep[idx].spr.y, mask))) {
						sheep[idx].spr.x=origx;
						sheep[idx].spr.y=origy;
					}
					// check for out of bounds (should never happen, but we need to be sure)
					if ((sheep[idx].spr.x < 2*GRIDSIZE+PLAYFIELDXOFF) ||
						(sheep[idx].spr.x > 18*GRIDSIZE+PLAYFIELDXOFF) ||
						(sheep[idx].spr.y < 3*GRIDSIZE+PLAYFIELDYOFF) ||
						(sheep[idx].spr.y > 13*GRIDSIZE+PLAYFIELDYOFF)) {
						FindSheepPosition(idx);
					}
					// Finally, set the correct frame
					if (!bRecaptureSheep) {
						sheep[idx].spr.tilenumber=SHEEPDOWN;
						if (origy>sheep[idx].spr.y) sheep[idx].spr.tilenumber=SHEEPUP;
						if (origx>sheep[idx].spr.x) sheep[idx].spr.tilenumber=SHEEPLEFT;
						if (origx<sheep[idx].spr.x) sheep[idx].spr.tilenumber=SHEEPRIGHT;
		
						sheep[idx].spr.tilenumber=SheepAnimationFrame[sheep[idx].spr.tilenumber][sheep[idx].animframe];
					}
				} else {	// stunned
					sheep[idx].stuntime--;
					if ((bSpecialDim) && ( (specialDat[bSpecialDim-1][0].type == SP_DISCOBALL) || (specialDat[bSpecialDim-1][0].type == SP_GIANTDISCO) )) {
						sheep[idx].spr.tilenumber = GetDanceFrame();
						if (specialDat[bSpecialDim-1][0].type == SP_GIANTDISCO) {
							// move the sheep towards Trey, if we can
							// this is a bit hacky but it should unwind okay
							sheep[idx].destx=herder[bSpecialDim-1].spr.x;
							sheep[idx].desty=herder[bSpecialDim-1].spr.y;
							bRecaptureSheep = true;
							goto movesheepagain;
						}
					} else {
						tmp=(int)((nGameFrames/10)%4);
						switch (tmp) {
						case 0:
							sheep[idx].spr.tilenumber=SHEEPUP; break;
						case 1:
							sheep[idx].spr.tilenumber=SHEEPDOWN; break;
						case 2:
							sheep[idx].spr.tilenumber=SHEEPLEFT; break;
						case 3:
							sheep[idx].spr.tilenumber=SHEEPRIGHT; break;
						}
					}
				}

				if (0 == loop) {
					// only sort the sheep sprite once!
					SortSprite(&sheep[idx].spr, txrmap_sheep, POLY_SHEEP);
				}
			}
		} else if (sheep[idx].type<0) {
			// animate the sheep being "warped" up
			sheep[idx].type+=10;
			if (sheep[idx].type >= 0) {
				sheep[idx].type = 0;
			} else {
				SortSpriteSquashed(&sheep[idx].spr, txrmap_sheep, POLY_SHEEP, -sheep[idx].type, 100);
			}
		}
	}

	nLastCenter+=2;
	if ((nLastCenter>=240) && (0 == (gStageSpecialEffect & STAGE_EFFECT_NO_SHEEP))) {
		// If we have boxed sheep and there are no destructibles left, or there are more than 4 sheep left outstanding...
		// For a non-boxed sheep stage, any sheep left are good.
		if (	((gStageSpecialEffect&STAGE_EFFECT_BOXED_SHEEP) && ((g_DestructiblesLeft <= 0) || (nTotalSheep > 4))) ||
				(((gStageSpecialEffect&STAGE_EFFECT_BOXED_SHEEP)==0) && (nTotalSheep > 0))	   ) {
			// if there are at least 5 free sheep slots on the grid (that's okay even if we only need one
			if (nSheepLeft < MAX_SHEEP-5) {
				nHowToPlayFlags |= HOW_TO_PLAY_TIME;
				if (nLastCenter&1) {
					// Start a new sheep in each of the four corner
					RestartSheep(GRIDSIZE+GRIDSIZE+PLAYFIELDXOFF, (3*GRIDSIZE)+PLAYFIELDYOFF, 0, 0);
					if (nTotalSheep > 1) RestartSheep((GRIDSIZE*(LEVELXSIZE-3))+PLAYFIELDXOFF, (3*GRIDSIZE)+PLAYFIELDYOFF, 0, 0);
					if (nTotalSheep > 2) RestartSheep(GRIDSIZE+GRIDSIZE+PLAYFIELDXOFF, (GRIDSIZE*(LEVELYSIZE-3))+PLAYFIELDYOFF, 0, 0);
					if (nTotalSheep > 3) RestartSheep((GRIDSIZE*(LEVELXSIZE-3))+PLAYFIELDXOFF, (GRIDSIZE*(LEVELYSIZE-3))+PLAYFIELDYOFF, 0, 0);
					nLastCenter=0;
				} else {
					RestartSheep(10*GRIDSIZE+PLAYFIELDXOFF, 8*GRIDSIZE+PLAYFIELDYOFF, 0, 0);
					if (nTotalSheep > 1) RestartSheep(10*GRIDSIZE+PLAYFIELDXOFF, 8*GRIDSIZE+PLAYFIELDYOFF, 0, 0);
					if (nTotalSheep > 2) RestartSheep(10*GRIDSIZE+PLAYFIELDXOFF, 8*GRIDSIZE+PLAYFIELDYOFF, 0, 0);
					if (nTotalSheep > 3) RestartSheep(10*GRIDSIZE+PLAYFIELDXOFF, 8*GRIDSIZE+PLAYFIELDYOFF, 0, 0);
					nLastCenter=1;
				}
				nTotalSheep-=4;
				if (nTotalSheep < 0) nTotalSheep = 0;
			}
		}
	}
}

// This used to maintain a ring buffer - don't need to now
void updatesheeppos(SheepStruct *pSheep) {
	pSheep->oldx=pSheep->spr.x;
	pSheep->oldy=pSheep->spr.y;
}

