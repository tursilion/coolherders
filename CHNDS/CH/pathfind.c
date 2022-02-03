/****************************************/
/* Cool Herders                         */
/* http://harmlesslion.com              */
/* Copyright 2009 HarmlessLion LLC      */
/* pathfind.c                           */
/****************************************/

// Known problems, if I have time:
// - Why does the destination sometimes end up as 1,1? (Because we didn't choose a target?)
// - Player sometimes falls off his path
// - Player sometimes doesn't have a path but runs anyway

#include <stdio.h>
#include <string.h>
#include "kosemulation.h"
#include "sprite.h"
#include "cool.h"
#include "levels.h"
#include "pathfind.h"

// from cool.h
void debug(char *str, ...);

// 64 paths, 50 steps each (6.4k)
static char PathFound[64][50][2];
static char pathwalk[16][21];		// y,x
int nPaths, nRange;
int startx,starty,endx,endy;

int isFlat(int x, int y);
void DoPaths();

void FindPath(int debugidx, char *PathList, int sx, int sy, int dx, int dy) {
	int idx, idx2, n;
	int x,y;

	x=dx;
	y=dy;
    sx=((sx-PLAYFIELDXOFF)+GRIDSIZE/2)/GRIDSIZE;
	sy=((sy-PLAYFIELDYOFF)+GRIDSIZE/2)/GRIDSIZE;
	dx=((dx-PLAYFIELDXOFF)+GRIDSIZE/2)/GRIDSIZE;
	dy=((dy-PLAYFIELDYOFF)+GRIDSIZE/2)/GRIDSIZE;
	
	if ((dx>18)||(dx<2)||(dy>13)||(dy<3)) {
#ifdef PATHFIND_DEBUG
		// this happens sometimes - the random algorithm doesn't take all the offsets
		// into account for speed's sake. That's okay, occasionally stopping helps an
		// AI look more human. We'll pick a new target next frame.
		debug("* Bad parameters to FindPath: %d,%d - %d,%d (%d,%d), herder %d\n", sx, sy, dx, dy, x, y, debugidx);
#endif
		PathList[0]=-1;
		PathList[1]=-1;
		PathList[2]=-1;
		PathList[3]=-1;
		return;
	}

//	debug("Pathfind from %d,%d to %d,%d\n", sx, sy, dx, dy);

	if (!isFlat(dx,dy)) {
		// This is especially possible on the haunted level! Find a nearby flat spot to move to
		if (isFlat(dx+1,dy)) {
			dx+=1;
		} else if (isFlat(dx-1,dy)) {
			dx-=1;
		} else if (isFlat(dx,dy+1)) {
			dy+=1;
		} else if (isFlat(dx,dy-1)) {
			dy-=1;
		} else {
			debug("* Can't find flat region near %d,%d, herder %d\n", dx, dy, debugidx);
			PathList[0]=-1;
			PathList[1]=-1;
			PathList[2]=-1;
			PathList[3]=-1;
			return;
		}
	}

	if ((sx==dx)&&(sy==dy)) {
		// No comment - there's just no move required
		PathList[0]=-1;
		PathList[1]=-1;
		PathList[2]=-1;
		PathList[3]=-1;
		return;
	}

	startx=sx;
	starty=sy;
	endx=dx;
	endy=dy;

	nPaths=1;
	nRange=0;
	memset(PathFound, 0, sizeof(PathFound));
	PathFound[0][0][0]=(char)startx;
	PathFound[0][0][1]=(char)starty;
	memset(pathwalk, 0, sizeof(pathwalk));
	pathwalk[starty][startx]=1;

	while (-1 != PathFound[0][0][0]) {
		DoPaths();
	}

	// got it
	n=PathFound[0][0][1];
	if (-1 == n) {
		debug("* Path not found: %d,%d - %d,%d, herder %d\n", sx, sy, dx, dy, debugidx);
		PathList[0]=-1;
		PathList[1]=-1;
		PathList[2]=-1;
		PathList[3]=-1;
		return;
	}

	idx2=0;
	x=-1;
	y=-1;
	for (idx=1; idx<=nRange; idx++) {
		if ((x != PathFound[n][idx][0]) || (y != PathFound[n][idx][1])) {
			x=PathFound[n][idx][0];
			y=PathFound[n][idx][1];
#ifdef PATHFIND_DEBUG
			debug("Herder %d PATH: %2d - (%4d,%4d)\n", debugidx, idx2/2, x, y);
#endif
			PathList[idx2]=(char)x;
			PathList[idx2+1]=(char)y;
			idx2+=2;
		}
	}
	PathList[idx2]=-1;
	PathList[idx2+1]=-1;
}

// checks for passable
int isFlat(int x, int y)
{
	if ((LevelData[y][x].nPage == 3) || (LevelData[y][x].isPassable)) {
		return 1;
	}
	return 0;
}

void DoPaths()
{
	int nTmpPaths=nPaths;
	int idx, idx2;

	// First check the existing path
	for (idx=0; idx<nTmpPaths; idx++) {
		int x,y,z;

		if (0 == PathFound[idx][0][0]) {
			// dead path
			continue;
		}

		x=PathFound[idx][nRange][0];
		y=PathFound[idx][nRange][1];
		z=0;

		// next path up
		if ((nRange==0)||((nRange>0)&&(y-1!=PathFound[idx][nRange-1][1]))) {
			// only 1 var can change per step
			if (isFlat(x,y-1)) {
				// check for negative push tile
				if ((LevelData[y][x].nPush!=3)||(nRange&1)) {
					if (0==pathwalk[y-1][x]) {
						// we can do this
						pathwalk[y-1][x]=1;
						PathFound[idx][nRange+1][0]=(char)x;
						PathFound[idx][nRange+1][1]=(char)(y-1);
						if ((y-1==endy)&&(x==endx)) {
							PathFound[0][0][0]=-1;
							PathFound[0][0][1]=(char)idx;
							break;
						}
						z=1;
					}
				}
			}
		}

		// and right
		if ((nRange==0)||((nRange>0)&&(x+1!=PathFound[idx][nRange-1][0]))) {
			// only 1 var can change per step
			if (isFlat(x+1,y)) {
				// check for negative push tile
				if ((LevelData[y][x].nPush!=4)||(nRange&1)) {
					if (0==pathwalk[y][x+1]) {
						// we can do this
						if (!z) {
							pathwalk[y][x+1]=1;
							PathFound[idx][nRange+1][0]=(char)(x+1);
							PathFound[idx][nRange+1][1]=(char)y;
							if ((y==endy)&&(x+1==endx)) {
								PathFound[0][0][0]=-1;
								PathFound[0][0][1]=(char)idx;
								break;
							}
							z=1;
						} else {
							// creating a new path
							if (nPaths < 63) {
								for (idx2=0; idx2<=nRange; idx2++) {
									PathFound[nPaths][idx2][0]=PathFound[idx][idx2][0];
									PathFound[nPaths][idx2][1]=PathFound[idx][idx2][1];
								}
								pathwalk[y][x+1]=1;
								PathFound[nPaths][nRange+1][0]=(char)(x+1);
								PathFound[nPaths][nRange+1][1]=(char)y;
								if ((y==endy)&&(x+1==endx)) {
									PathFound[0][0][0]=-1;
									PathFound[0][0][1]=(char)nPaths;
								}
								nPaths++;
							}
							z=1;
						}
					} 
				}
			}
		}

		// and left
		if ((nRange==0)||((nRange>0)&&(x-1!=PathFound[idx][nRange-1][0]))) {
			// only 1 var can change per step
			if (isFlat(x-1,y)) {
				// check for negative push tile
				if ((LevelData[y][x].nPush!=2)||(nRange&1)) {
					if (0==pathwalk[y][x-1]) {
						// we can do this
						if (!z) {
							pathwalk[y][x-1]=1;
							PathFound[idx][nRange+1][0]=(char)(x-1);
							PathFound[idx][nRange+1][1]=(char)y;
							if ((y==endy)&&(x-1==endx)) {
								PathFound[0][0][0]=-1;
								PathFound[0][0][1]=(char)idx;
								break;
							}
							z=1;
						} else {
							// new path
							if (nPaths < 63) {
								for (idx2=0; idx2<=nRange; idx2++) {
									PathFound[nPaths][idx2][0]=PathFound[idx][idx2][0];
									PathFound[nPaths][idx2][1]=PathFound[idx][idx2][1];
								}
								pathwalk[y][x-1]=1;
								PathFound[nPaths][nRange+1][0]=(char)(x-1);
								PathFound[nPaths][nRange+1][1]=(char)y;
								if ((y==endy)&&(x-1==endx)) {
									PathFound[0][0][0]=-1;
									PathFound[0][0][1]=(char)nPaths;
								}
								nPaths++;
							}
							z=1;
						}
					}
				}
			}
		}

		// and down
		if ((nRange==0)||((nRange>0)&&(y+1!=PathFound[idx][nRange-1][1]))) {
			// only 1 var can change per step
			if (isFlat(x,y+1)) {
				// check for negative push tile
				if ((LevelData[y][x].nPush!=1)||(nRange&1)) {
					if (0==pathwalk[y+1][x]) {
						// we can do this
						if (!z) {
							pathwalk[y+1][x]=1;
							PathFound[idx][nRange+1][0]=(char)x;
							PathFound[idx][nRange+1][1]=(char)(y+1);
							if ((y+1==endy)&&(x==endx)) {
								PathFound[0][0][0]=-1;
								PathFound[0][0][1]=(char)idx;
								break;
							}
							z=1;
						} else {
							// new path
							if (nPaths < 63) {
								for (idx2=0; idx2<=nRange; idx2++) {
									PathFound[nPaths][idx2][0]=PathFound[idx][idx2][0];
									PathFound[nPaths][idx2][1]=PathFound[idx][idx2][1];
								}
								pathwalk[y+1][x]=1;
								PathFound[nPaths][nRange+1][0]=(char)x;
								PathFound[nPaths][nRange+1][1]=(char)(y+1);
								if ((y+1==endy)&&(x==endx)) {
									PathFound[0][0][0]=-1;
									PathFound[0][0][1]=(char)nPaths;
								}
								nPaths++;
							}
							z=1;
						}
					}
				}
			}
		}

		if (0 == PathFound[idx][nRange+1][0]) {
			// dead end - this stream is dead
			PathFound[idx][0][0]=0;
		}
	}


	nRange++;
	if (nRange>49) {
		debug("*** PathFind OVERFLOW ***\n");
		PathFound[0][0][0]=-1;
		PathFound[0][0][1]=-1;
	}
}

