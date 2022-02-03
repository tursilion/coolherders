/****************************************/
/* Cool Herders                         */
/* A demented muse by Brendan Wiese     */
/* http://starbow.org                   */
/* Programmed by Mike Brent             */
/* http://harmlesslion.com              */
/* Copyright 2000-2005 Brendan Wiese &  */
/* Mike Brent.                          */
/* collide.c                            */
/****************************************/
// Collision buffer routines

/* Ported to KOS 1.1.x by Dan Potter */

#include <stdio.h>
#include <malloc.h>

#ifdef WIN32
#include "kosemulation.h"
#include <string.h>
#else
#include <kos.h>
#endif

#include "sprite.h"
#include "cool.h"
#include "collide.h"

#ifdef WIN32
unsigned char CollisionBuffer[640*480];
#else
unsigned char CollisionBuffer[640*480] __attribute__ ((aligned(4)));
#endif

// This works by assuming everything is a GRIDSIZExGRIDSIZE pixel block
// The buffer contains numbers indicating what is at each
// position. 0 is clear.

// Clear the collision buffer
void clearbuffer() {
	memset(CollisionBuffer, 0, sizeof(CollisionBuffer));
}

// Draw a block into the buffer (loses what's underneath)
// Must be at a 4 pixel offset
void drawbuffer(int x, int y, uint32 type) {
	int i1, i2;
	int offset;
	int xdest, ydest;

	xdest=x+GRIDSIZE;
	if (x<0) x=0;
	if (xdest>640) xdest=640;
	x=x&0xfffc;
	ydest=y+GRIDSIZE;
	if (y<0) y=0;
	if (ydest>480) ydest=480;
	offset=y*640;
	type=type|(type<<24)|(type<<16)|(type<<8);

	for (i1=y; i1<ydest; i1++) {
		for (i2=x; i2<xdest; i2+=4) {
			*((uint32*)(CollisionBuffer+offset+i2))=type;
		}
		offset+=640;
	}
}

// Merge a block into the buffer (used for ghost walls)
// Must be at a 4 pixel offset
void mergebuffer(int x, int y, uint32 type) {
	int i1, i2;
	int offset;
	int xdest, ydest;

	xdest=x+GRIDSIZE;
	if (x<0) x=0;
	if (xdest>640) xdest=640;
	x=x&0xfffc;
	ydest=y+GRIDSIZE;
	if (y<0) y=0;
	if (ydest>480) ydest=480;
	offset=y*640;
	type=type|(type<<24)|(type<<16)|(type<<8);

	for (i1=y; i1<ydest; i1++) {
		for (i2=x; i2<xdest; i2+=4) {
			*((uint32*)(CollisionBuffer+offset+i2))|=type;
		}
		offset+=640;
	}
}

// Check a block in the buffer - returns the first non-zero found, or 0 if none
// pass '0' for type to check anything, or pass a specific type
uint32 checkblock(int x, int y, uint32 type) {
	int i1, i2;
	int offset;
	int xdest, ydest;

	if (type==0) {
		type=0xffffffff;
	} else {
		type=type|(type<<24)|(type<<16)|(type<<8);
	}

	xdest=x+GRIDSIZE;
	if (x<0) x=0;
	if (xdest>640) xdest=640;
	ydest=y+GRIDSIZE;
	if (y<0) y=0;
	if (ydest>480) ydest=480;
	x=x&0xfffc;
	offset=y*640;

	for (i1=y; i1<ydest; i1++) {
		for (i2=x; i2<xdest; i2+=4) {	// Check 4 bytes at a time :)
			if ((*((uint32*)(CollisionBuffer+offset+i2))&type)) {
				return *((uint32*)(CollisionBuffer+offset+i2));
			}
		}
		offset+=640;
	}
	return TYPE_NONE;
}

#if 0

uint32 TxrBuf=0xffffffff;

// Add the collision buffer as a translucent sprite
void showcollidebuf(int x, int y) {
	vertex_ot_t vert;
	poly_hdr_t poly;

	if (TxrBuf==0xffffffff) {
		TxrBuf=ta->txr_allocate(1024*512*2);
	}

	ta->txr_load(TxrBuf, CollisionBuffer, sizeof(CollisionBuffer));

	ta->poly_hdr_txr(&poly, TA_TRANSLUCENT, TA_RGB565, 1024, 512, TxrBuf, TA_NO_FILTER);
	ta->commit_poly_hdr(&poly);

	vert.flags = TA_VERTEX_NORMAL;
	vert.x = x;
	vert.y = y+120;
	vert.z = 600;
	vert.u = 0;
	vert.v = 0.93f;
	vert.a = 0.5f;
	vert.r = 1.0f;
	vert.g = 1.0f;
	vert.b = 1.0f;
	vert.oa = vert.or = vert.og = vert.ob = 0.0f;
	ta->commit_vertex(&vert, sizeof(vert));
	
	vert.x = x;
	vert.y = y;
	vert.u = 0;
	vert.v = 0;
	ta->commit_vertex(&vert, sizeof(vert));
	
	vert.x = x+160;
	vert.y = y+120;
	vert.u = 0.625f;
	vert.v = 0.93f;
	ta->commit_vertex(&vert, sizeof(vert));

	vert.flags = TA_VERTEX_EOL;
	vert.x = x+160;
	vert.y = y;
	vert.u = 0.625f;
	vert.v = 0;
	ta->commit_vertex(&vert, sizeof(vert));
}
#endif
