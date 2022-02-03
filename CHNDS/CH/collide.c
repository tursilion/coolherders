/****************************************/
/* Cool Herders                         */
/* http://harmlesslion.com              */
/* Copyright 2009 HarmlessLion LLC      */
/* collide.c                            */
/****************************************/
// Collision buffer routines

#include <stdio.h>

#include "kosemulation.h"
#include <string.h>

#include "sprite.h"
#include "cool.h"
#include "collide.h"

unsigned char CollisionBuffer[640*480] ATTRIBUTE_ALIGN(4);

// This works by assuming everything is a GRIDSIZExGRIDSIZE pixel block
// The buffer contains numbers indicating what is at each
// position. 0 is clear.

// Clear the collision buffer - warning - runs ASYNC, you must be sure it's
// done before using it!!
void clearbuffer() {
	MI_DmaClear32Async(GENERIC_DMA_CHANNEL, CollisionBuffer, sizeof(CollisionBuffer), NULL, 0);
}

// Draw a block into the buffer (loses what's underneath)
// Must be at a 4 pixel offset
void drawbuffer(int x, int y, uint32 type) {
	int i1;
	unsigned char *pTarget;
	int xdest, ydest, cnt;

	xdest=x+GRIDSIZE;
	if (x<0) x=0;
	if (xdest>=640) xdest=639;
	x=x&0xfffc;
	cnt=(xdest-x)&0xfffc;
	ydest=y+GRIDSIZE;
	if (y<0) y=0;
	if (ydest>=480) ydest=479;
	y&=0xfffe;
	type=type|(type<<24)|(type<<16)|(type<<8);

	pTarget=CollisionBuffer+(y*640)+x;
	
	for (i1=y; i1<ydest; i1+=2) {
		MI_CpuFillFast(pTarget,type,cnt);
		pTarget+=1280;
	}
}

// Merge a block into the buffer (used for ghost walls)
// Must be at a 4 pixel offset
void mergebuffer(int x, int y, uint32 type) {
	int i1, i2;
	unsigned char *pTarget;
	int xdest, ydest;

	xdest=x+GRIDSIZE;
	if (x<0) x=0;
	if (xdest>=640) xdest=639;
	x=x&0xfffc;
	ydest=y+GRIDSIZE;
	if (y<0) y=0;
	if (ydest>=480) ydest=479;
	y&=0xfffe;
	type=type|(type<<24)|(type<<16)|(type<<8);

	pTarget=CollisionBuffer+(y*640);
	for (i1=y; i1<ydest; i1+=2) {
		for (i2=x; i2<xdest; i2+=4) {
			*((uint32*)(pTarget+i2))|=type;
		}
		pTarget+=1280;	// 2 lines to improve performance
	}
}
 
// Check a block in the buffer - returns the first non-zero found, or 0 if none
uint32 checkblock(int x, int y, uint32 type) {
	unsigned char *pTarget;
	int xdest, ydest;

	type=type|(type<<24)|(type<<16)|(type<<8);

	xdest=x+GRIDSIZE-1;
	if (x<0) x=0;
	if (xdest>=640) xdest=639;
	x=x&0xfffc;
	xdest=xdest&0xfffc;
	
	ydest=y+GRIDSIZE-1;
	if (y<0) y=0;
	if (ydest>=480) ydest=479;
	y=y&0xfffe;
	ydest=ydest&0xfffe;
	pTarget=CollisionBuffer+(y*640);

	// Just check the four corners of the point
	if ((*(uint32*)(pTarget+x))&type) 
	{
		return type;
	}
	if ((*(uint32*)(pTarget+xdest))&type) 
	{
		return type;
	}

	pTarget=CollisionBuffer+(ydest*640);
	if ((*(uint32*)(pTarget+x))&type) 
	{
		return type;
	}
	if ((*(uint32*)(pTarget+xdest))&type) 
	{
		return type;
	}

	return TYPE_NONE;
}

