/****************************************/
/* Cool Herders                         */
/* A demented muse by Brendan Wiese     */
/* http://starbow.org                   */
/* Programmed by Mike Brent             */
/* http://harmlesslion.com              */
/* Copyright 2000-2005 Brendan Wiese &  */
/* Mike Brent.                          */
/* sheep.c                              */
/****************************************/

/* Ported to KOS 1.1.x by Dan Potter */

#include <stdio.h>
#include <malloc.h>

#ifdef WIN32
#include "kosemulation.h"
#else
#include <kos.h>
#endif

#include "sprite.h"
#include "cool.h"
#include "rand.h"
#include "collide.h"
#include "levels.h"
#include "sheep.h"
#include "menu.h"	// for globals
#include "sound.h"
#include "storymode.h"
#include "score.h"

// We read level 0 (NZ Island) for the ghost sheep
extern char Level[NUMLEVELS][LEVELBYTES];
extern int sndCountdown;
extern int nFrames;
extern int gStoryModeSpecialFlags;

// Sheep animation is a bit messed up thanks to the giant sheep
// [Direction 0-3][Frame 0-3]
int SheepAnimationFrame[4][4] = {
	{	0, 5, 10, 15,	},
	{	1, 6, 11, 16,	},
	{	2, 7, 20, 21,	},
	{	3, 8, 4, 9		}
};

int SheepSpeed=NORMALSHEEPSPEED;
SheepStruct sheep[MAX_SHEEP];
int fGhostSheep=0;

// forward declaration
void updatesheeppos(SheepStruct *pHerd);
void FindSheepPosition(int idx);

// Find a legal location for this sheep - used by initSheep and when
// relocating a sheep out of bounds
void FindSheepPosition(int idx) {
	int x,y;

	do {
		x=(ch_rand()%7-3)+(LEVELXSIZE/2-1);
		y=ch_rand()%LEVELYSIZE;
		if (fGhostSheep) {
			// A little more complex cause we didn't load the NZ level for ghost sheep
			// So we have to do a little parsing here.
			int pos;
			pos=y*LEVELXSIZE*3+x*3;
			if (Level[0][pos]=='3') break;
			if (Level[0][pos+1]=='.') break;
		} else {
			if (LevelData[y][x].isPassable) break;
		}
	} while (1);

	sheep[idx].spr.x=x*GRIDSIZE+PLAYFIELDXOFF;
	sheep[idx].spr.y=y*GRIDSIZE+PLAYFIELDYOFF;
}

int RestartSheep(int x, int y, int xstep, int ystep) {
	int idx4, idx5;
	int ret=-1;

	for (idx4=0; idx4<MAX_SHEEP; idx4++) {
		if (sheep[idx4].type==0) {
			ret=idx4;
			memset(&sheep[idx4], 0, sizeof(SheepStruct));
			sheep[idx4].type=3;		// 3x sheep
			sheep[idx4].invincible=1;
			sheep[idx4].range=90;	// 1+1/2 seconds of freedom!
			sheep[idx4].spr.txr_addr=txr_sheep;
			sheep[idx4].spr.tilenumber=16;
			sheep[idx4].spr.z=516.0;
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

			if (fGhostSheep) {
				// Ghost sheep are transparent and '3d' to reduce glitches
				sheep[idx4].spr.alpha=128;
				sheep[idx4].spr.is3D=true;
				// Furthermore, they don't follow the same walls! So we need
				// to make sure this sheep starts in a good place.
				// We'll very quickly check eight directions around us. If
				// we can't find a good starting point, then we'll throw the
				// sheep back into the middle. It's a ghost anyway. ;)
				if (checkblock(x, y, TYPE_GHOSTWALL)) {
					x=sheep[idx4].spr.x;
					y=sheep[idx4].spr.y;

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
						// We give up - nowhere to put the sheep! So center it :)
						sheep[idx4].spr.x=10*GRIDSIZE+PLAYFIELDXOFF;
						sheep[idx4].spr.y=8*GRIDSIZE+PLAYFIELDYOFF;
					}
				}
			} else {
				sheep[idx4].spr.alpha=255;
				sheep[idx4].spr.is3D=false;
			}
			for (idx5=0; idx5<SHEEPHISTORY; idx5++) {
				sheep[idx4].oldx[idx5]=sheep[idx4].spr.x;
				sheep[idx4].oldy[idx5]=sheep[idx4].spr.y;
			}
			sheep[idx4].oldidx=0;
			if (!sndCountdown) {
				sound_effect(snd_sheep[FREED], SHEEPVOL);
				sndCountdown=SNDDELAY;
			}
			break;
		}
	}	// if we couldn't find one (somehow!), no worries

	return ret;
}

// Set up the sheep on this level
// LevelData must already be set!
void initSheep() {
	int idx, idx2;

	debug("Initializing Sheep\n");

	for (idx=0; idx<MAX_SHEEP; idx++) {
		memset(&sheep[idx], 0, sizeof(SheepStruct));
		sheep[idx].spr.txr_addr=txr_sheep;
		sheep[idx].spr.tilenumber=16;
		FindSheepPosition(idx);
		sheep[idx].spr.z=516.0;
		sheep[idx].spr.xd=1;
		sheep[idx].spr.yd=0;
		if (gStoryModeSpecialFlags&EFFECT_NO_SHEEP) {
			// no sheep at first in conveyor mode (still need to init the arrays though, to be safe)
			sheep[idx].type=0;	// none
		} else {
			sheep[idx].type=2;	// active
		}
		sheep[idx].invincible=1;
		sheep[idx].maxframes=SHEEPFRAMES;
		sheep[idx].range=180;	// start with 3 seconds of invincibility
		sheep[idx].recaptured=0;
		sheep[idx].stuntime=0;

		if (fGhostSheep) {
			// Ghost sheep are transparent and '3d' to reduce glitches
			sheep[idx].spr.alpha=128;
			sheep[idx].spr.is3D=true;
			SheepSpeed=GHOSTSHEEPSPEED;
		} else {
			sheep[idx].spr.alpha=255;
			sheep[idx].spr.is3D=false;
			SheepSpeed=NORMALSHEEPSPEED;
		}
		SheepSpeed+=gOptions.SheepSpeed;
		
		// override speed for challenge
		if (gStoryModeSpecialFlags&EFFECT_SHEEP_RIDE_CONVEYORS) {
			SheepSpeed=1;	// starts easy :)
		}

		for (idx2=0; idx2<SHEEPHISTORY; idx2++) {
			sheep[idx].oldx[idx2]=sheep[idx].spr.x;
			sheep[idx].oldy[idx2]=sheep[idx].spr.y;
		}
		sheep[idx].oldidx=0;
	}
}

// Move the sheep. This is a lot like the computer Herder movement
// This function is not called on the conveyor belt challenge
void moveSheep() {
	int idx, miscflag, tmp, mask;
	int currentx, currenty, gridx, gridy;
	int xstep, ystep, desiredx, desiredy;
	int origx, origy, loop;
	int nSheepLeft=0;
	int nOldGfxDarken=gGfxDarken;

	if (fGhostSheep) {
		// ghost sheep stay bright!
		gGfxDarken=0;
	}
	
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
					int fNoCheck=0;

					// if we are currently in an illegal position, then allow any movement just to get back on track
					if (fGhostSheep) {
						if (checkblock(sheep[idx].spr.x, sheep[idx].spr.y, TYPE_GHOSTWALL)) {
							fNoCheck=1;
						}
					} else {
						if (checkblock(sheep[idx].spr.x, sheep[idx].spr.y, TYPE_WALL|TYPE_BOX)) {
							fNoCheck=1;
						}
					}						

					// Check - if we didn't move, choose a new destination
					if ((sheep[idx].oldx[sheep[idx].oldidx>0?sheep[idx].oldidx-1:SHEEPHISTORY-1]==sheep[idx].spr.x)&&(sheep[idx].oldy[sheep[idx].oldidx>0?sheep[idx].oldidx-1:SHEEPHISTORY-1]==sheep[idx].spr.y)) {
						sheep[idx].destx=0;
					}

					updatesheeppos(&sheep[idx]);

					if ((sheep[idx].destx==0)||(sheep[idx].desty==0)) {
						// new target - it doesn't have to be a legal destination,
						// as we'll pick a new one as soon as we get stuck :)
						sheep[idx].destx=(ch_rand()%20) * GRIDSIZE;
						sheep[idx].desty=(ch_rand()%15) * GRIDSIZE;
					}

					sheep[idx].spr.xd=sheep[idx].destx-sheep[idx].spr.x;
					sheep[idx].spr.yd=sheep[idx].desty-sheep[idx].spr.y;
					if (fGhostSheep) {
						// Ghost sheep animate much more slowly
						if (nGameFrames%12 == 0) {
							sheep[idx].animframe++;
						}
					} else {
						if (nGameFrames%4 == 0) {
							sheep[idx].animframe++;
						}
					}
					miscflag=1;

					if (sheep[idx].invincible) {
						sheep[idx].spr.alpha=64;
						if (sheep[idx].range>0) {
							sheep[idx].range--;
						}
						if (sheep[idx].range==0) {
							sheep[idx].type=1;		// back to normal next frame
							sheep[idx].invincible=0;
						} else {
							if (sheep[idx].range<60) {
								sheep[idx].type=2;	// slow down in preparation...
							}
						}
					} else {
						if (fGhostSheep) {
							sheep[idx].spr.alpha=128;
						} else {
							sheep[idx].spr.alpha=255;
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
					// No action if no movement
					if (miscflag) {
						currentx=sheep[idx].spr.x;
						currenty=sheep[idx].spr.y;
						gridx=((currentx-PLAYFIELDXOFF)+GRIDSIZE/2)/GRIDSIZE;
						gridy=((currenty-PLAYFIELDYOFF)+GRIDSIZE/2)/GRIDSIZE;
						xstep=(int)sheep[idx].spr.xd;
						ystep=(int)sheep[idx].spr.yd;
						desiredx=currentx;
						desiredy=currenty;

						if (fGhostSheep) {
							if ((!sheep[idx].invincible)&&(!(gStoryModeSpecialFlags&EFFECT_LOOKING_FOR_HADES))) {
								mask=TYPE_GHOSTWALL|TYPE_PLAY;
							} else {
								mask=TYPE_GHOSTWALL;		// only care about walls
							}
						} else {
							if (!sheep[idx].invincible) {
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
							if (sheep[idx].oldx[sheep[idx].oldidx>0?sheep[idx].oldidx-1:SHEEPHISTORY-1]!=origx) {
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
						sheep[idx].spr.tilenumber=SHEEPDOWN;
						if (origy>sheep[idx].spr.y) sheep[idx].spr.tilenumber=SHEEPUP;
						if (origx>sheep[idx].spr.x) sheep[idx].spr.tilenumber=SHEEPLEFT;
						if (origx<sheep[idx].spr.x) sheep[idx].spr.tilenumber=SHEEPRIGHT;
		
						sheep[idx].spr.tilenumber=SheepAnimationFrame[sheep[idx].spr.tilenumber][sheep[idx].animframe];
					}
				} else {	// stunned
					sheep[idx].stuntime--;
					tmp=(nGameFrames/10)%4;
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

				SortSprite(&sheep[idx].spr);
			}
		}
	}

	if (fGhostSheep) {
		gGfxDarken=nOldGfxDarken;
	}

	if (gStoryModeSpecialFlags&EFFECT_CREATE_NEW_SHEEP) {
		if (nSheepLeft < MAX_SHEEP-5) {
			// Start a new sheep in each of the four corner
			RestartSheep(GRIDSIZE+GRIDSIZE+PLAYFIELDXOFF, (3*GRIDSIZE)+PLAYFIELDYOFF, 0, 0);
			RestartSheep((GRIDSIZE*(LEVELXSIZE-3))+PLAYFIELDXOFF, (3*GRIDSIZE)+PLAYFIELDYOFF, 0, 0);
			RestartSheep(GRIDSIZE+GRIDSIZE+PLAYFIELDXOFF, (GRIDSIZE*(LEVELYSIZE-3))+PLAYFIELDYOFF, 0, 0);
			RestartSheep((GRIDSIZE*(LEVELXSIZE-3))+PLAYFIELDXOFF, (GRIDSIZE*(LEVELYSIZE-3))+PLAYFIELDYOFF, 0, 0);
		}
	}
}

void moveSheepConveyor() {
	int idx;
	int nSheepLeft=0;
	int nOldGfxDarken=gGfxDarken;
	int nSpeed;

	if (fGhostSheep) {
		// ghost sheep stay bright!
		gGfxDarken=0;
	}

	nSpeed=SheepSpeed/8;
	if ((nFrames&7) < (SheepSpeed%8)) nSpeed++;
	
	for (idx=0; idx<MAX_SHEEP; idx++) {
		// the type indicates which conveyor the sheep is on!
		if (sheep[idx].type>0) {
			nSheepLeft++;

			if (sheep[idx].invincible) {
				sheep[idx].invincible=0;
			}

			if ((sheep[idx].recaptured)&&(!sheep[idx].range)) {
				sheep[idx].recaptured--;
			}

			updatesheeppos(&sheep[idx]);

			if (fGhostSheep) {
				sheep[idx].spr.alpha=128;
			} else {
				sheep[idx].spr.alpha=255;
			}
			sheep[idx].spr.tilenumber=16;

			// In this function, we just move the sheep. The main function will check them.
			if (sheep[idx].type & 1) {
				// odd ones move right
				sheep[idx].spr.x+=nSpeed;
				if (sheep[idx].spr.x > GRIDSIZE*16+PLAYFIELDXOFF) {
					herder[gHumanPlayer].flags|=FLAG_CHALLENGE_COMPLETE;
				}
			} else {
				// even ones move left
				sheep[idx].spr.x-=nSpeed;
				if (sheep[idx].spr.x < GRIDSIZE*4+PLAYFIELDXOFF) {
					herder[gHumanPlayer].flags|=FLAG_CHALLENGE_COMPLETE;
				}
			}

			SortSprite(&sheep[idx].spr);
		}
	}

	if (fGhostSheep) {
		gGfxDarken=nOldGfxDarken;
	}

	if (gStoryModeSpecialFlags&EFFECT_CREATE_NEW_SHEEP) {
		idx=-1;
		if (nSheepLeft <= SheepSpeed) {
			// Start a new sheep randomly on one of the conveyor belts
			// The locations are fixed. This is coded quickly. not elegantly
			int pos=ch_rand()&0xf;
			switch (pos) {
			case 1:	idx=RestartSheep((GRIDSIZE*(4))+PLAYFIELDXOFF, (GRIDSIZE*(5))+PLAYFIELDYOFF, 0, 0); break;
			case 2: idx=RestartSheep((GRIDSIZE*(16))+PLAYFIELDXOFF, (GRIDSIZE*(7))+PLAYFIELDYOFF, 0, 0); break;
			case 3: idx=RestartSheep((GRIDSIZE*(4))+PLAYFIELDXOFF, (GRIDSIZE*(9))+PLAYFIELDYOFF, 0, 0); break;
			case 4: idx=RestartSheep((GRIDSIZE*(16))+PLAYFIELDXOFF, (GRIDSIZE*(11))+PLAYFIELDYOFF, 0, 0); break;
			}
			if (idx>-1) {
				// save which belt the sheep is on
				sheep[idx].type=pos;
			}
		}
	}
}

// Add the current position to the list of old positions
// And adjust the pointer. The list is a circular buffer
// SHEEPHISTORY entries long. pHerd->oldidx is the current
// position, and pHerd->oldidx-1 is the previous position
void updatesheeppos(SheepStruct *pHerd) {
	pHerd->oldidx++;
	if (pHerd->oldidx>=SHEEPHISTORY) pHerd->oldidx=0;

	pHerd->oldx[pHerd->oldidx]=pHerd->spr.x;
	pHerd->oldy[pHerd->oldidx]=pHerd->spr.y;
}

