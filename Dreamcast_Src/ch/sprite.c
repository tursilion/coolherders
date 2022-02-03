/****************************************/
/* Cool Herders                         */
/* A demented muse by Brendan Wiese     */
/* http://starbow.org                   */
/* Programmed by Mike Brent             */
/* http://harmlesslion.com              */
/* Copyright 2000-2005 Brendan Wiese &  */
/* Mike Brent.                          */
/* sprite.c                             */
/****************************************/

/* Ported to KOS 1.1.x by Dan Potter */

/* these functions all assume 256x256 textures, except where noted */
/* Sprites are tiles fixed at 48x48 pixels */

#include <stdio.h>
#include <malloc.h>

#ifdef WIN32
#include "kosemulation.h"
#else
#include <kos.h>
#endif

#include "sprite.h"
#include "cool.h"
 
/* from KOS itself... so we can tell what the current list is! */
/* All functions now sort into whatever the currently open list is */
#ifndef WIN32
#include "../kernel/arch/dreamcast/hardware/pvr/pvr_internal.h"
#endif

// This variable is set by pvr_list_begin(), so it always contains the current
// type of list being registered. That lets these functions be more generic
extern volatile pvr_state_t pvr_state;
#define CURRENT_LIST (pvr_state.list_reg_open)

// We use the cpu version of sprites so we can do some manual blits!
extern char *cpu_sprites;
extern int nFrames;
extern int gReturnToMenu;

static int LastListType=-1;
static int LastTxrFormat=-1;
pvr_ptr_t LastTexturePage=NULL;

// This is honored by all methods (for flashlight herding). Set to 0-255 for amount to darken by.
int gGfxDarken=0;

inline void BeginScene() {
	LastTexturePage=NULL; 
	pvr_wait_ready(); 
	pvr_scene_begin();
}

// Sorts a context header, checking whether it's unneeded
void SortHeader(int NewList, int NewTxrFormat, int nWidth, int nHeight, pvr_ptr_t NewTexturePage) {
	pvr_poly_cxt_t cxt;
	pvr_poly_hdr_t hdr;

	if ((LastListType != NewList) || (NewTxrFormat != LastTxrFormat) || (NewTexturePage != LastTexturePage)) {
		LastListType=NewList;
		LastTxrFormat=NewTxrFormat;
		LastTexturePage=NewTexturePage;

		pvr_poly_cxt_txr(&cxt, NewList, NewTxrFormat, nWidth, nHeight, NewTexturePage, PVR_FILTER_NONE);
		pvr_poly_compile(&hdr, &cxt);
		pvr_prim(&hdr, sizeof(hdr));
	}
}
// Sorts a context header with bilinear filtering
void SortHeaderStretch(int NewList, int NewTxrFormat, int nWidth, int nHeight, pvr_ptr_t NewTexturePage) {
	pvr_poly_cxt_t cxt;
	pvr_poly_hdr_t hdr;

	// clear out the flags - we rarely do more than one scaled object at a time so it doesn't save much
	LastListType=-1;
	LastTxrFormat=-1;
	LastTexturePage=NULL;

	pvr_poly_cxt_txr(&cxt, NewList, NewTxrFormat, nWidth, nHeight, NewTexturePage, PVR_FILTER_BILINEAR);
	pvr_poly_compile(&hdr, &cxt);
	pvr_prim(&hdr, sizeof(hdr));
}

// I tested a version of SortSprite that did not test the header or the effect flags,
// but it did not end up any faster. The polygon render method in use is limiting us
// to about 66,000 polys per second (2 poly triangle strips), which is about 550 sprites per frame
// In other words, we aren't CPU bound here.
void SortSprite(SPRITE *spr) {
	pvr_vertex_t vert;
	float u1, u2, v1, v2, off;
	int tmpX, tmpY, c, tile;
	int hflip=0, vflip=0;

	// Check for sprite flips
	if (spr->tilenumber & SPR_VFLIP) {
		tile=spr->tilenumber&(~SPR_VFLIP);
		vflip=1;
	} else if (spr->tilenumber & SPR_HFLIP) {
		tile=spr->tilenumber&(~SPR_HFLIP);
		hflip=1;
	} else {
		tile=spr->tilenumber;
	}

	// Calculate shading
	c=min(max(0xff-gGfxDarken, 0), 255);

	// calculate texture offsets
	// texture addresses are 8x8
	off=48.0/CH_PAGESIZE;
	u1=((tile%5)*TILESIZE)/CH_PAGESIZE;
	u2=u1+off;
	v1=((tile/5)*TILESIZE)/CH_PAGESIZE;
	v2=v1+off;

	SortHeader(CURRENT_LIST, PVR_TXRFMT_ARGB1555, CH_PAGESIZE, CH_PAGESIZE, spr->txr_addr);

	tmpX=spr->x-(TILESIZE-GRIDSIZE)/2;		// X is centered
	tmpY=spr->y-(TILESIZE-GRIDSIZE);		// Y is drawn tall

	vert.flags = PVR_CMD_VERTEX;
	vert.x = tmpX;
	vert.y = tmpY+TILESIZE;
	vert.z = spr->z;
	vert.u = hflip?u2:u1;
	vert.v = vflip?v1:v2;
	vert.argb = INT_PACK_COLOR(spr->alpha, c, c, c);
	vert.oargb = 0;
	pvr_prim(&vert, sizeof(vert));
	
	vert.x = tmpX;
	vert.y = tmpY;
	vert.z = spr->is3D ? spr->z+10.0 : spr->z;
	vert.u = hflip?u2:u1;
	vert.v = vflip?v2:v1;
	pvr_prim(&vert, sizeof(vert));
	
	vert.x = tmpX+TILESIZE;
	vert.y = tmpY+TILESIZE;
	vert.z = spr->z;
	vert.u = hflip?u1:u2;
	vert.v = vflip?v1:v2;
	pvr_prim(&vert, sizeof(vert));

	vert.flags = PVR_CMD_VERTEX_EOL;
	vert.x = tmpX+TILESIZE;
	vert.y = tmpY;
	vert.z = spr->is3D ? spr->z+10.0 : spr->z;
	vert.u = hflip?u1:u2;
	vert.v = vflip?v2:v1;
	pvr_prim(&vert, sizeof(vert));
}

// Same as sort sprite, but tints it by r,g,b
void SortSpriteTinted(SPRITE *spr, int r, int g, int b) {
	pvr_vertex_t vert;
	float u1, u2, v1, v2, off;
	int tmpX, tmpY, tile;
	int hflip=0, vflip=0;

	// Check for sprite flips
	if (spr->tilenumber & SPR_VFLIP) {
		tile=spr->tilenumber&(~SPR_VFLIP);
		vflip=1;
	} else {
		tile=spr->tilenumber;
	}
	if (spr->tilenumber & SPR_HFLIP) {
		tile=spr->tilenumber&(~SPR_HFLIP);
		hflip=1;
	} else {
		tile=spr->tilenumber;
	}

	// Calculate shading
	r=min(max(r-gGfxDarken, 0), 255);
	g=min(max(g-gGfxDarken, 0), 255);
	b=min(max(b-gGfxDarken, 0), 255);

	// calculate texture offsets
	// texture addresses are 8x8
	off=48.0/CH_PAGESIZE;
	u1=((tile%5)*TILESIZE)/CH_PAGESIZE;
	u2=u1+off;
	v1=((tile/5)*TILESIZE)/CH_PAGESIZE;
	v2=v1+off;

	SortHeader(CURRENT_LIST, PVR_TXRFMT_ARGB1555, CH_PAGESIZE, CH_PAGESIZE, spr->txr_addr);

	tmpX=spr->x-(TILESIZE-GRIDSIZE)/2;		// X is centered
	tmpY=spr->y-(TILESIZE-GRIDSIZE);		// Y is drawn tall

	vert.flags = PVR_CMD_VERTEX;
	vert.x = tmpX;
	vert.y = tmpY+TILESIZE;
	vert.z = spr->z;
	vert.u = hflip?u2:u1;
	vert.v = vflip?v1:v2;
	vert.argb = INT_PACK_COLOR(spr->alpha, r, g, b);
	vert.oargb = 0;
	pvr_prim(&vert, sizeof(vert));
	
	vert.x = tmpX;
	vert.y = tmpY;
	vert.z = spr->is3D ? spr->z+10.0 : spr->z;
	vert.u = hflip?u2:u1;
	vert.v = vflip?v2:v1;
	pvr_prim(&vert, sizeof(vert));
	
	vert.x = tmpX+TILESIZE;
	vert.y = tmpY+TILESIZE;
	vert.z = spr->z;
	vert.u = hflip?u1:u2;
	vert.v = vflip?v1:v2;
	pvr_prim(&vert, sizeof(vert));

	vert.flags = PVR_CMD_VERTEX_EOL;
	vert.x = tmpX+TILESIZE;
	vert.y = tmpY;
	vert.z = spr->is3D ? spr->z+10.0 : spr->z;
	vert.u = hflip?u1:u2;
	vert.v = vflip?v2:v1;
	pvr_prim(&vert, sizeof(vert));
}

void SortScaledSprite(SPRITE *spr, float xScale, float yScale) {
	pvr_vertex_t vert;
	float u1, u2, v1, v2, off;
	int tmpX, tmpY, c, tile;
	int hflip=0, vflip=0;

	// Check for sprite flips
	if (spr->tilenumber & SPR_VFLIP) {
		tile=spr->tilenumber&(~SPR_VFLIP);
		vflip=1;
	} else {
		tile=spr->tilenumber;
	}

	if (spr->tilenumber & SPR_HFLIP) {
		tile=spr->tilenumber&(~SPR_HFLIP);
		hflip=1;
	} else {
		tile=spr->tilenumber;
	}

	// Calculate shading
	c=min(max(0xff-gGfxDarken, 0), 255);

	// calculate texture offsets
	// texture addresses are 8x8
	off=48.0/CH_PAGESIZE;
	u1=((tile%5)*TILESIZE)/CH_PAGESIZE;
	u2=u1+off;
	v1=((tile/5)*TILESIZE)/CH_PAGESIZE;
	v2=v1+off;

	SortHeaderStretch(CURRENT_LIST, PVR_TXRFMT_ARGB1555, CH_PAGESIZE, CH_PAGESIZE, spr->txr_addr);

	tmpX=spr->x + (TILESIZE * xScale);
	tmpY=spr->y + (TILESIZE * yScale);

	vert.flags = PVR_CMD_VERTEX;
	vert.x = spr->x;
	vert.y = tmpY;
	vert.z = spr->z;
	vert.u = hflip?u2:u1;
	vert.v = vflip?v1:v2;
	vert.argb = INT_PACK_COLOR(spr->alpha, c, c, c);
	vert.oargb = 0;
	pvr_prim(&vert, sizeof(vert));
	
	vert.x = spr->x;
	vert.y = spr->y;
	vert.z = spr->is3D ? spr->z+10.0 : spr->z;
	vert.u = hflip?u2:u1;
	vert.v = vflip?v2:v1;
	pvr_prim(&vert, sizeof(vert));
	
	vert.x = tmpX;
	vert.y = tmpY;
	vert.z = spr->z;
	vert.u = hflip?u1:u2;
	vert.v = vflip?v1:v2;
	pvr_prim(&vert, sizeof(vert));

	vert.flags = PVR_CMD_VERTEX_EOL;
	vert.x = tmpX;
	vert.y = spr->y;
	vert.z = spr->is3D ? spr->z+10.0 : spr->z;
	vert.u = hflip?u1:u2;
	vert.v = vflip?v2:v1;
	pvr_prim(&vert, sizeof(vert));
}

// Add an abitrary portion of a texture page as a centered sprite
void addPage(pvr_ptr_t txr, int tx1, int ty1, int tx2, int ty2) {
	float x1, y1;

	x1=(640.0-(tx2-tx1))/2.0;
	y1=(480.0-(ty2-ty1))/2.0;

	addPage2(txr, x1, y1, tx1, ty1, tx2, ty2, INT_PACK_ALPHA(204), 1024.0f);
}

// Add a specific portion of a texture page as a positioned sprite with alpha
// supports PIC_HFLIP on tx1
void addPage2(pvr_ptr_t txr, int x, int y, int tx1, int ty1, int tx2, int ty2, uint32 color, float z) {
	pvr_vertex_t vert;
	int r, g, b;
	float u1, u2, v1, v2;
	float x1, x2, y1, y2;
	int hflip;

	// Calculate shading
	r=min(max(((color&0x00ff0000)>>16)-gGfxDarken, 0), 255);
	g=min(max(((color&0x0000ff00)>>8)-gGfxDarken, 0), 255);
	b=min(max(((color&0x000000ff))-gGfxDarken, 0), 255);
	color=(color&0xff000000)|(r<<16)|(g<<8)|(b);

	hflip=0;
	if (tx1&PIC_HFLIP) {
		tx1&=0x7fff;
		hflip=1;
	}

	x1=x;
	x2=x1+(tx2-tx1)+1.0;
	y1=y;
	y2=y1+(ty2-ty1)+1.0;

	u1=tx1/256.0;
	u2=(tx2+1.0)/256.0;
	v1=ty1/256.0;
	v2=(ty2+1.0)/256.0;

	SortHeader(CURRENT_LIST, PVR_TXRFMT_ARGB1555, 256, 256, txr);

	vert.flags = PVR_CMD_VERTEX;
	vert.x = x1;
	vert.y = y2;
	vert.z = z;
	vert.u = hflip?u2:u1;
	vert.v = v2;
	vert.argb = color;
	vert.oargb = 0;
	pvr_prim(&vert, sizeof(vert));
	
	vert.x = x1;
	vert.y = y1;
	vert.u = hflip?u2:u1;
	vert.v = v1;
	pvr_prim(&vert, sizeof(vert));
	
	vert.x = x2;
	vert.y = y2;
	vert.u = hflip?u1:u2;
	vert.v = v2;
	pvr_prim(&vert, sizeof(vert));

	vert.flags = PVR_CMD_VERTEX_EOL;
	vert.x = x2;
	vert.y = y1;
	vert.u = hflip?u1:u2;
	vert.v = v1;
	pvr_prim(&vert, sizeof(vert));
}

// Add a specific portion of a texture page as a positioned sprite with alpha in 4444 mode
// supports PIC_HFLIP on tx1
void addPageAlpha(pvr_ptr_t txr, int x, int y, int tx1, int ty1, int tx2, int ty2, uint32 color, float z) {
	pvr_vertex_t vert;
	int r, g, b;
	float u1, u2, v1, v2;
	float x1, x2, y1, y2;
	int hflip;

	// Calculate shading
	r=min(max(((color&0x00ff0000)>>16)-gGfxDarken, 0), 255);
	g=min(max(((color&0x0000ff00)>>8)-gGfxDarken, 0), 255);
	b=min(max(((color&0x000000ff))-gGfxDarken, 0), 255);
	color=(color&0xff000000)|(r<<16)|(g<<8)|(b);

	hflip=0;
	if (tx1&PIC_HFLIP) {
		tx1&=0x7fff;
		hflip=1;
	}

	x1=x;
	x2=x1+(tx2-tx1)+1.0;
	y1=y;
	y2=y1+(ty2-ty1)+1.0;

	u1=tx1/256.0;
	u2=(tx2+1.0)/256.0;
	v1=ty1/256.0;
	v2=(ty2+1.0)/256.0;

	SortHeader(CURRENT_LIST, PVR_TXRFMT_ARGB4444, 256, 256, txr);

	vert.flags = PVR_CMD_VERTEX;
	vert.x = x1;
	vert.y = y2;
	vert.z = z;
	vert.u = hflip?u2:u1;
	vert.v = v2;
	vert.argb = color;
	vert.oargb = 0;
	pvr_prim(&vert, sizeof(vert));
	
	vert.x = x1;
	vert.y = y1;
	vert.u = hflip?u2:u1;
	vert.v = v1;
	pvr_prim(&vert, sizeof(vert));
	
	vert.x = x2;
	vert.y = y2;
	vert.u = hflip?u1:u2;
	vert.v = v2;
	pvr_prim(&vert, sizeof(vert));

	vert.flags = PVR_CMD_VERTEX_EOL;
	vert.x = x2;
	vert.y = y1;
	vert.u = hflip?u1:u2;
	vert.v = v1;
	pvr_prim(&vert, sizeof(vert));
}

// Add a specific portion of a texture page as a positioned sprite with alpha in 4444 mode with stretching
// does not support flipping
void addPageAlphaStretch(pvr_ptr_t txr, int x1, int y1, int x2, int y2, int tx1, int ty1, int tx2, int ty2, uint32 color, float z) {
	pvr_vertex_t vert;
	int r, g, b;
	float u1, u2, v1, v2;
	
	// Calculate shading
	r=min(max(((color&0x00ff0000)>>16)-gGfxDarken, 0), 255);
	g=min(max(((color&0x0000ff00)>>8)-gGfxDarken, 0), 255);
	b=min(max(((color&0x000000ff))-gGfxDarken, 0), 255);
	color=(color&0xff000000)|(r<<16)|(g<<8)|(b);

	u1=tx1/256.0;
	u2=(tx2+1.0)/256.0;
	v1=ty1/256.0;
	v2=(ty2+1.0)/256.0;

	SortHeaderStretch(CURRENT_LIST, PVR_TXRFMT_ARGB4444, 256, 256, txr);

	vert.flags = PVR_CMD_VERTEX;
	vert.x = x1;
	vert.y = y2;
	vert.z = z;
	vert.u = u1;
	vert.v = v2;
	vert.argb = color;
	vert.oargb = 0;
	pvr_prim(&vert, sizeof(vert));
	
	vert.x = x1;
	vert.y = y1;
	vert.u = u1;
	vert.v = v1;
	pvr_prim(&vert, sizeof(vert));
	
	vert.x = x2;
	vert.y = y2;
	vert.u = u2;
	vert.v = v2;
	pvr_prim(&vert, sizeof(vert));

	vert.flags = PVR_CMD_VERTEX_EOL;
	vert.x = x2;
	vert.y = y1;
	vert.u = u2;
	vert.v = v1;
	pvr_prim(&vert, sizeof(vert));
}

// Add a centered 512x256 texture for story mode
void addPageStory(pvr_ptr_t txr, float z) {
	pvr_vertex_t vert;
	int r, g, b;
	float u1, u2, v1, v2;
	float x1, x2, y1, y2;

	// Calculate shading
	r=min(max(255-gGfxDarken, 0), 255);
	g=min(max(255-gGfxDarken, 0), 255);
	b=min(max(255-gGfxDarken, 0), 255);

	x1=64;
	x2=575;
	y1=64;
	y2=319;

	u1=0;
	u2=1.0;
	v1=0;
	v2=1.0;

	SortHeader(CURRENT_LIST, PVR_TXRFMT_ARGB1555, 512, 256, txr);

	vert.flags = PVR_CMD_VERTEX;
	vert.x = x1;
	vert.y = y2;
	vert.z = z;
	vert.u = u1;
	vert.v = v2;
	vert.argb = 0xff000000|(r<<16)|(g<<8)|(b);
	vert.oargb = 0;
	pvr_prim(&vert, sizeof(vert));
	
	vert.x = x1;
	vert.y = y1;
	vert.u = u1;
	vert.v = v1;
	pvr_prim(&vert, sizeof(vert));
	
	vert.x = x2;
	vert.y = y2;
	vert.u = u2;
	vert.v = v2;
	pvr_prim(&vert, sizeof(vert));

	vert.flags = PVR_CMD_VERTEX_EOL;
	vert.x = x2;
	vert.y = y1;
	vert.u = u2;
	vert.v = v1;
	pvr_prim(&vert, sizeof(vert));
}

// Similar to addPage2, but the source page is 512x512
// supports PIC_HFLIP on tx1
void addPageLarge(pvr_ptr_t txr, int x, int y, int tx1, int ty1, int tx2, int ty2, uint32 color, float z) {
	pvr_vertex_t vert;
	float u1, u2, v1, v2;
	float x1, x2, y1, y2;
	int r,g,b, hflip;

	// Calculate shading
	r=min(max(((color&0x00ff0000)>>16)-gGfxDarken, 0), 255);
	g=min(max(((color&0x0000ff00)>>8)-gGfxDarken, 0), 255);
	b=min(max(((color&0x000000ff))-gGfxDarken, 0), 255);
	color=(color&0xff000000)|(r<<16)|(g<<8)|(b);

	hflip=0;
	if (tx1&PIC_HFLIP) {
		tx1&=0x7fff;
		hflip=1;
	}

	x1=x;
	x2=x1+(tx2-tx1)+1.0;
	y1=y;
	y2=y1+(ty2-ty1)+1.0;

	u1=tx1/512.0;
	u2=(tx2+1.0)/512.0;
	v1=ty1/512.0;
	v2=(ty2+1.0)/512.0;

	SortHeader(CURRENT_LIST, PVR_TXRFMT_ARGB1555, 512, 512, txr);

	vert.flags = PVR_CMD_VERTEX;
	vert.x = x1;
	vert.y = y2;
	vert.z = z;
	vert.u = hflip?u2:u1;
	vert.v = v2;
	vert.argb = color;
	vert.oargb = 0;
	pvr_prim(&vert, sizeof(vert));
	
	vert.x = x1;
	vert.y = y1;
	vert.u = hflip?u2:u1;
	vert.v = v1;
	pvr_prim(&vert, sizeof(vert));
	
	vert.x = x2;
	vert.y = y2;
	vert.u = hflip?u1:u2;
	vert.v = v2;
	pvr_prim(&vert, sizeof(vert));

	vert.flags = PVR_CMD_VERTEX_EOL;
	vert.x = x2;
	vert.y = y1;
	vert.u = hflip?u1:u2;
	vert.v = v1;
	pvr_prim(&vert, sizeof(vert));
}

// Add a specific portion of a texture page as a positioned sprite with alpha, stretching onscreen
void stretchPage2(pvr_ptr_t txr, int x1, int y1, int x2, int y2, int tx1, int ty1, int tx2, int ty2, float z, uint32 color) {
	pvr_vertex_t vert;
	float u1, u2, v1, v2;
	int r,g,b;

	// Calculate shading
	r=min(max(((color&0x00ff0000)>>16)-gGfxDarken, 0), 255);
	g=min(max(((color&0x0000ff00)>>8)-gGfxDarken, 0), 255);
	b=min(max(((color&0x000000ff))-gGfxDarken, 0), 255);
	color=(color&0xff000000)|(r<<16)|(g<<8)|(b);

	u1=tx1/256.0;
	u2=(tx2+1.0)/256.0;
	v1=ty1/256.0;
	v2=(ty2+1.0)/256.0;

	SortHeaderStretch(CURRENT_LIST, PVR_TXRFMT_ARGB1555, 256, 256, txr);

	vert.flags = PVR_CMD_VERTEX;
	vert.x = x1;
	vert.y = y2;
	vert.z = z;
	vert.u = u1;
	vert.v = v2;
	vert.argb = color;
	vert.oargb = 0;
	pvr_prim(&vert, sizeof(vert));
	
	vert.x = x1;
	vert.y = y1;
	vert.u = u1;
	vert.v = v1;
	pvr_prim(&vert, sizeof(vert));
	
	vert.x = x2;
	vert.y = y2;
	vert.u = u2;
	vert.v = v2;
	pvr_prim(&vert, sizeof(vert));

	vert.flags = PVR_CMD_VERTEX_EOL;
	vert.x = x2;
	vert.y = y1;
	vert.u = u2;
	vert.v = v1;
	pvr_prim(&vert, sizeof(vert));
}

// Add a specific portion of a 512x512 page as a positioned sprite with alpha, stretching onscreen
void stretchLarge2(pvr_ptr_t txr, int x1, int y1, int x2, int y2, int tx1, int ty1, int tx2, int ty2, float z, uint32 color) {
	pvr_vertex_t vert;
	float u1, u2, v1, v2;
	int r,g,b;

	// Calculate shading
	r=min(max(((color&0x00ff0000)>>16)-gGfxDarken, 0), 255);
	g=min(max(((color&0x0000ff00)>>8)-gGfxDarken, 0), 255);
	b=min(max(((color&0x000000ff))-gGfxDarken, 0), 255);
	color=(color&0xff000000)|(r<<16)|(g<<8)|(b);

	u1=tx1/512.0;
	u2=(tx2+1.0)/512.0;
	v1=ty1/512.0;
	v2=(ty2+1.0)/512.0;

	SortHeaderStretch(CURRENT_LIST, PVR_TXRFMT_ARGB1555, 512, 512, txr);

	vert.flags = PVR_CMD_VERTEX;
	vert.x = x1;
	vert.y = y2;
	vert.z = z;
	vert.u = u1;
	vert.v = v2;
	vert.argb = color;
	vert.oargb = 0;
	pvr_prim(&vert, sizeof(vert));
	
	vert.x = x1;
	vert.y = y1;
	vert.u = u1;
	vert.v = v1;
	pvr_prim(&vert, sizeof(vert));
	
	vert.x = x2;
	vert.y = y2;
	vert.u = u2;
	vert.v = v2;
	pvr_prim(&vert, sizeof(vert));

	vert.flags = PVR_CMD_VERTEX_EOL;
	vert.x = x2;
	vert.y = y1;
	vert.u = u2;
	vert.v = v1;
	pvr_prim(&vert, sizeof(vert));
}

// Add a specific portion of a 512x512 page as a positioned sprite with alpha, stretching onscreen
// NOTE: takes raw nontwiddled textures!
void stretchLarge3(pvr_ptr_t txr, int x1, int y1, int x2, int y2, int tx1, int ty1, int tx2, int ty2, uint32 color) {
	pvr_vertex_t vert;
	float u1, u2, v1, v2;
	int r,g,b;

	// Calculate shading
	r=min(max(((color&0x00ff0000)>>16)-gGfxDarken, 0), 255);
	g=min(max(((color&0x0000ff00)>>8)-gGfxDarken, 0), 255);
	b=min(max(((color&0x000000ff))-gGfxDarken, 0), 255);
	color=(color&0xff000000)|(r<<16)|(g<<8)|(b);

	u1=tx1/512.0;
	u2=(tx2+1.0)/512.0;
	v1=ty1/512.0;
	v2=(ty2+1.0)/512.0;

	SortHeaderStretch(CURRENT_LIST, PVR_TXRFMT_ARGB1555|PVR_TXRFMT_NONTWIDDLED, 512, 512, txr);

	vert.flags = PVR_CMD_VERTEX;
	vert.x = x1;
	vert.y = y2;
	vert.z = 528.1f;
	vert.u = u1;
	vert.v = v2;
	vert.argb = color;
	vert.oargb = 0;
	pvr_prim(&vert, sizeof(vert));
	
	vert.x = x1;
	vert.y = y1;
	vert.u = u1;
	vert.v = v1;
	pvr_prim(&vert, sizeof(vert));
	
	vert.x = x2;
	vert.y = y2;
	vert.u = u2;
	vert.v = v2;
	pvr_prim(&vert, sizeof(vert));

	vert.flags = PVR_CMD_VERTEX_EOL;
	vert.x = x2;
	vert.y = y1;
	vert.u = u2;
	vert.v = v1;
	pvr_prim(&vert, sizeof(vert));
}

// Add a specific portion of a large texture page as a positioned sprite with alpha, non-twiddled
void addNonTwiddledPage(pvr_ptr_t txr, int x, int y, int tx1, int ty1, int tx2, int ty2, uint32 color) {
	pvr_vertex_t vert;
	float u1, u2, v1, v2;
	float x1, x2, y1, y2;
	int r,g,b;

	// Calculate shading
	r=min(max(((color&0x00ff0000)>>16)-gGfxDarken, 0), 255);
	g=min(max(((color&0x0000ff00)>>8)-gGfxDarken, 0), 255);
	b=min(max(((color&0x000000ff))-gGfxDarken, 0), 255);
	color=(color&0xff000000)|(r<<16)|(g<<8)|(b);

	x1=x;
	x2=x1+(tx2-tx1);
	y1=y;
	y2=y1+(ty2-ty1);

	u1=tx1/512.0;
	u2=tx2/512.0;
	v1=ty1/512.0;
	v2=ty2/512.0;

	SortHeader(CURRENT_LIST, PVR_TXRFMT_ARGB1555|PVR_TXRFMT_NONTWIDDLED, 512, 512, txr);

	vert.flags = PVR_CMD_VERTEX;
	vert.x = x1;
	vert.y = y2;
	vert.z = 1020.0f;
	vert.u = u1;
	vert.v = v2;
	vert.argb = color;
	vert.oargb = 0;
	pvr_prim(&vert, sizeof(vert));
	
	vert.x = x1;
	vert.y = y1;
	vert.u = u1;
	vert.v = v1;
	pvr_prim(&vert, sizeof(vert));
	
	vert.x = x2;
	vert.y = y2;
	vert.u = u2;
	vert.v = v2;
	pvr_prim(&vert, sizeof(vert));

	vert.flags = PVR_CMD_VERTEX_EOL;
	vert.x = x2;
	vert.y = y1;
	vert.u = u2;
	vert.v = v1;
	pvr_prim(&vert, sizeof(vert));
}

// Register a solid color rectangle, shaded vertically
void SortRect(float z, int x1, int y1, int x2, int y2, uint32 color1, uint32 color2) {
	pvr_vertex_t vert;
	int r,g,b;

	// Calculate shading
	r=min(max(((color1&0x00ff0000)>>16)-gGfxDarken, 0), 255);
	g=min(max(((color1&0x0000ff00)>>8)-gGfxDarken, 0), 255);
	b=min(max(((color1&0x000000ff))-gGfxDarken, 0), 255);
	color1=(color1&0xff000000)|(r<<16)|(g<<8)|(b);

	// Calculate shading
	r=min(max(((color2&0x00ff0000)>>16)-gGfxDarken, 0), 255);
	g=min(max(((color2&0x0000ff00)>>8)-gGfxDarken, 0), 255);
	b=min(max(((color2&0x000000ff))-gGfxDarken, 0), 255);
	color2=(color2&0xff000000)|(r<<16)|(g<<8)|(b);

	/* white solid texture around the edge (we need the alpha to be set!) */
	SortHeader(PVR_LIST_TR_POLY, PVR_TXRFMT_ARGB1555, 256, 256, txr_sprites);

	vert.flags = PVR_CMD_VERTEX;
	vert.x = x1;
	vert.y = y2;
	vert.z = z;  /* just to give an absolute depth for the power VR to handle overlap predictably */
	vert.u = 0.99f;
	vert.v = 0.99f;
	vert.argb = color2;
	vert.oargb = 0;
	pvr_prim(&vert, sizeof(vert));
	
	vert.y = y1;
	vert.argb = color1;
	pvr_prim(&vert, sizeof(vert));

	vert.x = x2;
	vert.y = y2;
	vert.argb = color2;
	pvr_prim(&vert, sizeof(vert));

	vert.flags = PVR_CMD_VERTEX_EOL;
	vert.y = y1;
	vert.argb = color1;
	pvr_prim(&vert, sizeof(vert));
}

// Register a solid full screen color rectangle, shaded rolling by frame
void SortRollingRect(float z, uint32 color1, uint32 color2) {
	pvr_vertex_t vert;
	int r,g,b;
	int x1=0, x2=639;
	int y0,y1,y2,y3,y4;
	
	y0=(nFrames%480)-480;
	y1=y0+240;
	y2=y1+240;
	y3=y2+240;
	y4=y3+240;

	// Calculate shading
	r=min(max(((color1&0x00ff0000)>>16)-gGfxDarken, 0), 255);
	g=min(max(((color1&0x0000ff00)>>8)-gGfxDarken, 0), 255);
	b=min(max(((color1&0x000000ff))-gGfxDarken, 0), 255);
	color1=(color1&0xff000000)|(r<<16)|(g<<8)|(b);

	// Calculate shading
	r=min(max(((color2&0x00ff0000)>>16)-gGfxDarken, 0), 255);
	g=min(max(((color2&0x0000ff00)>>8)-gGfxDarken, 0), 255);
	b=min(max(((color2&0x000000ff))-gGfxDarken, 0), 255);
	color2=(color2&0xff000000)|(r<<16)|(g<<8)|(b);

	/* white solid texture around the edge (we need the alpha to be set!) */
	SortHeader(PVR_LIST_TR_POLY, PVR_TXRFMT_ARGB1555, 256, 256, txr_sprites);

	/* This one has ten points and sorts in a slightly different order */

	vert.flags = PVR_CMD_VERTEX;
	vert.x = x1;
	vert.y = y0;
	vert.z = z;  /* just to give an absolute depth for the power VR to handle overlap predictably */
	vert.u = 0.99f;
	vert.v = 0.99f;
	vert.argb = color1;
	vert.oargb = 0;
	pvr_prim(&vert, sizeof(vert));
	
	vert.x = x2;
	pvr_prim(&vert, sizeof(vert));

	vert.x = x1;
	vert.y = y1;
	vert.argb = color2;
	pvr_prim(&vert, sizeof(vert));

	vert.x=x2;
	pvr_prim(&vert, sizeof(vert));

	vert.x=x1;
	vert.y=y2;
	vert.argb=color1;
	pvr_prim(&vert, sizeof(vert));

	vert.x = x2;
	pvr_prim(&vert, sizeof(vert));

	vert.x=x1;
	vert.y=y3;
	vert.argb=color2;
	pvr_prim(&vert, sizeof(vert));

	vert.x = x2;
	pvr_prim(&vert, sizeof(vert));
	
	vert.x=x1;
	vert.y=y4;
	vert.argb=color1;
	pvr_prim(&vert, sizeof(vert));

	vert.flags = PVR_CMD_VERTEX_EOL;
	vert.x = x2;
	pvr_prim(&vert, sizeof(vert));
}

// Register a solid color rectangle, shaded horizontally
void SortRectHoriz(float z, int x1, int y1, int x2, int y2, uint32 color1, uint32 color2) {
	pvr_vertex_t vert;
	int r,g,b;

	// Calculate shading
	r=min(max(((color1&0x00ff0000)>>16)-gGfxDarken, 0), 255);
	g=min(max(((color1&0x0000ff00)>>8)-gGfxDarken, 0), 255);
	b=min(max(((color1&0x000000ff))-gGfxDarken, 0), 255);
	color1=(color1&0xff000000)|(r<<16)|(g<<8)|(b);

	// Calculate shading
	r=min(max(((color2&0x00ff0000)>>16)-gGfxDarken, 0), 255);
	g=min(max(((color2&0x0000ff00)>>8)-gGfxDarken, 0), 255);
	b=min(max(((color2&0x000000ff))-gGfxDarken, 0), 255);
	color2=(color2&0xff000000)|(r<<16)|(g<<8)|(b);

	/* white solid texture around the edge (we need the alpha to be set!) */
	SortHeader(PVR_LIST_TR_POLY, PVR_TXRFMT_ARGB1555, 256, 256, txr_sprites);

	vert.flags = PVR_CMD_VERTEX;
	vert.x = x1;
	vert.y = y2;
	vert.z = z;  /* just to give an absolute depth for the power VR to handle overlap predictably */
	vert.u = 0.99f;
	vert.v = 0.99f;
	vert.argb = color1;
	vert.oargb = 0;
	pvr_prim(&vert, sizeof(vert));
	
	vert.y = y1;
	pvr_prim(&vert, sizeof(vert));

	vert.x = x2;
	vert.y = y2;
	vert.argb = color2;
	pvr_prim(&vert, sizeof(vert));

	vert.flags = PVR_CMD_VERTEX_EOL;
	vert.y = y1;
	pvr_prim(&vert, sizeof(vert));
}

// Register a light glow effect (faked) 
// Does not honor gGfxDarken! Ignores Alpha component and makes it's own
void SortLight(float z, int x1, int y1, int x2, int y2, uint32 color) {
	pvr_vertex_t vert;
	int r,g,b;
	uint32 color1, color2;
	int mx, my, x8, y8;

	// Calculate shading
	r=(color&0x00ff0000)>>16;
	g=(color&0x0000ff00)>>8;
	b=(color&0x000000ff);

	// core
	color1=INT_PACK_COLOR(224, r, g, b);
	// edge
	color2=INT_PACK_COLOR(32, r, g, b);

	// calculate halfways
	mx=x1+(x2-x1)/2;
	my=y1+(y2-y1)/2;
	x8=(x2-x1)/8;
	y8=(y2-y1)/8;

	/* white solid texture around the edge (we need the alpha to be set!) */
	SortHeader(PVR_LIST_TR_POLY, PVR_TXRFMT_ARGB1555, 256, 256, txr_sprites);

	// We really need a triangle fan here. I can't see how to do that, though, so
	// two hacky triangle strips will do! ;)

	// 0
	vert.flags = PVR_CMD_VERTEX;
	vert.x = mx;
	vert.y = my;
	vert.z = z;  /* just to give an absolute depth for the power VR to handle overlap predictably */
	vert.u = 0.99f;
	vert.v = 0.99f;
	vert.argb = color1;
	vert.oargb = 0;
	pvr_prim(&vert, sizeof(vert));

	// 1
	vert.y=y1;
	vert.argb=color2;
	pvr_prim(&vert, sizeof(vert));

	// 2
	vert.x=x2-x8;
	vert.y=y1+y8;
//	vert.argb=color2;
	pvr_prim(&vert, sizeof(vert));

	// 3
	vert.x=mx;
	vert.y=my;
	vert.argb=color1;
	pvr_prim(&vert, sizeof(vert));

	// 4
	vert.x=x2;
//	vert.y=my;
	vert.argb=color2;
	pvr_prim(&vert, sizeof(vert));

	// 5
	vert.x=x2-x8;
	vert.y=y2-y8;
//	vert.argb=color2;
	pvr_prim(&vert, sizeof(vert));

	// 6
	vert.x=mx;
	vert.y=my;
	vert.argb=color1;
	pvr_prim(&vert, sizeof(vert));

	// 7
//	vert.x=mx;
	vert.y=y2;
	vert.argb=color2;
	pvr_prim(&vert, sizeof(vert));

	// 8
	vert.x=x1+x8;
	vert.y=y2-y8;
//	vert.argb=color2;
	pvr_prim(&vert, sizeof(vert));

	// 9
	vert.x=mx;
	vert.y=my;
	vert.argb=color1;
	pvr_prim(&vert, sizeof(vert));

	// 10
	vert.x=x1;
//	vert.y=my;
	vert.argb=color2;
	pvr_prim(&vert, sizeof(vert));
	
	// 11
	vert.x=x1+x8;
	vert.y=y1+y8;
//	vert.argb=color2;
	pvr_prim(&vert, sizeof(vert));

	// 12
	vert.x=mx;
	vert.y=my;
	vert.argb=color1;
	pvr_prim(&vert, sizeof(vert));

	// 13
	vert.flags = PVR_CMD_VERTEX_EOL;
//	vert.x=mx;
	vert.y=y1;
	vert.argb=color2;
	pvr_prim(&vert, sizeof(vert));

	// The above fails to draw two polys due to backface culling, so we'll fill them in here

	// 0
	vert.flags = PVR_CMD_VERTEX;
	vert.x = mx;
	vert.y = my;
	vert.z = z;  /* just to give an absolute depth for the power VR to handle overlap predictably */
	vert.u = 0.99f;
	vert.v = 0.99f;
	vert.argb = color1;
	vert.oargb = 0;
	pvr_prim(&vert, sizeof(vert));

	// 1
	vert.x=x2-x8;
	vert.y=y1+y8;
	vert.argb=color2;
	pvr_prim(&vert, sizeof(vert));

	// 2
	vert.x=x2;
	vert.y=my;
//	vert.argb=color2;
	pvr_prim(&vert, sizeof(vert));

	// 3
	vert.x=mx;
//	vert.y=my;
	vert.argb=color1;
	pvr_prim(&vert, sizeof(vert));

	// 4
	vert.x=x1;
//	vert.y=my;
	vert.argb=color2;
	pvr_prim(&vert, sizeof(vert));

	// 5
	vert.flags = PVR_CMD_VERTEX_EOL;
	vert.x=x1+x8;
	vert.y=y2-y8;
//	vert.argb=color2;
	pvr_prim(&vert, sizeof(vert));
}

/* Draw a sprite to a texture by blitting data */
/* NOTE: source textures are  256x256x2    */
/*       dest texture must be 512x512x2    */
/* The math won't work right otherwise ;)  */
void DrawSpriteTxr(pvr_ptr_t txr_out, SPRITE *spr) {
/* Won't work in Windows port */
#ifndef WIN32
	pvr_ptr_t pSrc, pDest;
	char *ftxr=cpu_sprites;
	int idx, idx2;
	int x, y, x1, y1;

	x=spr->x-(TILESIZE-GRIDSIZE)/2;		// X is centered
	y=spr->y-(TILESIZE-GRIDSIZE);		// Y is drawn tall
	if (y<0) return;	// don't draw if off top of screen

	pDest=(y<<10)+(x<<1)+txr_out;

	x1=((spr->tilenumber%5)*TILESIZE)+1;
	y1=((spr->tilenumber/5)*TILESIZE);
	pSrc=(y1<<9)+(x1<<1)+ftxr;
	for (idx=0; idx<48; idx++) {
		for (idx2=0; idx2<95; idx2+=2) {
			int tst=x+(idx2>>1);
			if ((tst>=0) && (tst<512)) {
				unsigned short *pS=(unsigned short*)(pSrc+(idx<<9)+idx2);
				unsigned short *pD=(unsigned short*)(pDest+(idx<<10)+idx2);

				if (*pS & 0x8000) {
					*pD=*pS;
				}
			}
		}
	}

#endif
}

// Sort a 640x480 picture stored in a 512x512 texture for 0,0, and two pieces in
// a 256x256 texture (512,0 at 0,0 in the texture, and 512,256 at 128,0 in the texture)
// The 256x256 texture represents the last 128 pixel wide vertical strip in two pieces.
// First, a 256 pixel tall strip (0,0-127,255), and then a 224 pixel tall (128,0-128,223)
// This leaves a 128x32 pixel gap at the bottom of the tile if we need it ;)
// If x != 0 then sorts two copies of the picture, shifted over that far and mirrored ;). X should be -639-640.
void SortFullPictureX(pvr_ptr_t p512, pvr_ptr_t p256, float z, int x) {
	// %$^%^#$&! back and forth to fractions for texture addresses...
	// should have just ripped that whole bit out of KOS and used ints directly...
	addPageLarge(p512, 0+x, 0, 0, 0, 511, 480, DEFAULT_COLOR, z);
	addPage2(p256, 512+x, 0, 0, 0, 127, 255, DEFAULT_COLOR, z);
	addPage2(p256, 512+x, 256, 128, 0, 255, 224, DEFAULT_COLOR, z);
	if (x>0) {
		addPageLarge(p512, x-512, 0, PIC_HFLIP|0, 0, 511, 480, DEFAULT_COLOR, z);
		addPage2(p256, x-640, 0, PIC_HFLIP|0, 0, 128, 255, DEFAULT_COLOR, z);
		addPage2(p256, x-640, 256, PIC_HFLIP|128, 0, 256, 224, DEFAULT_COLOR, z);
	}
	if (x<0) {
		addPageLarge(p512, x+768, 0, PIC_HFLIP|0, 0, 511, 480, DEFAULT_COLOR, z);
		addPage2(p256, x+639, 0, PIC_HFLIP|0, 0, 128, 255, DEFAULT_COLOR, z);
		addPage2(p256, x+639, 256, PIC_HFLIP|128, 0, 256, 224, DEFAULT_COLOR, z);
	}
}

#ifdef ENABLE_SNAPSHOTS
#define BYTE0(x) ((x)&0xff000000)>>24
#define BYTE1(x) ((x)&0xff0000)>>16
#define BYTE2(x) ((x)&0xff00)>>8
#define BYTE3(x) ((x)&0xff)
#define LOAD4(buf, first, data) buf[first]=BYTE3(data); buf[first+1]=BYTE2(data); buf[first+2]=BYTE1(data); buf[first+3]=BYTE0(data);
#define LOAD2(buf, first, data) buf[first]=BYTE3(data); buf[first+1]=BYTE2(data);
#define do_write(file, data, count) buf[0]=data&0xff; fs_write(file, buf, 1);
extern uint16 *vram_s;
#define AUTOTIMESHOT 10800;	// every 3 minutes roughly
char buf[1920];
#endif

void checkSnapshot() {
#ifdef ENABLE_SNAPSHOTS
	int i, tmp, x,y;
	static int nCnt=0;
	static int nCountdown=AUTOTIMESHOT;
	uint16 *pData;

	i = kbd_get_key();
#ifdef ENABLE_AUTOMATIC_SNAPSHOTS
	if (nCountdown > 0) {
		nCountdown--;
	}
#endif
	if ((32 == i)||(nCountdown<1)) {
		if (nCountdown<1) {
			debug("Countdown expired - automatic capture\n");
			nCountdown=AUTOTIMESHOT;
		} else {
			debug("Got key %d\n", i);
		}

		// write a screenshot - very slow for some reason
		file_t f;

		sprintf(buf, "/pc/home/tursi/snapshot%03d.bmp", nCnt++);
		f=fs_open(buf, O_TRUNC | O_WRONLY);
		if (f) {
			// build a BMP header - 640x480x24 bit
			// Header
			buf[0]='B';					// magic tag
			buf[1]='M';
			tmp=(640*480*3)+14+40;		// file size
			LOAD4(buf, 2, tmp);
			LOAD4(buf, 6, 0);			// reserved 1,2
			LOAD4(buf, 10, 54);			// offset to data
			// information section
			LOAD4(buf, 14, 40);			// header size
			LOAD4(buf, 18, 640);		// width
			LOAD4(buf, 22, 480);		// height
			LOAD2(buf, 26, 1);			// planes
			LOAD2(buf, 28, 24);			// bits per pixel
			LOAD4(buf, 30, 0);			// compression
			tmp=(640*480*3);			// image size
			LOAD4(buf, 34, tmp);
			LOAD4(buf, 38, 72);			// x res
			LOAD4(buf, 42, 72);			// y res
			LOAD4(buf, 46, 0);			// number colors
			LOAD4(buf, 50, 0);			// number important colors

			// write it out
			fs_write(f, buf, 54);

			// now we need to read the screen buffer and dump that	
			while (pvr_state.render_busy) thd_sleep(1);

			for (y=479; y>=0; y--) {
				pData=vram_s+(y*640);
				for (x=0; x<640; x++) {
					tmp=*(pData++);
					buf[x*3]=(tmp&0x001f)<<3;
					buf[x*3+1]=(tmp&0x07e0)>>3;
					buf[x*3+2]=(tmp&0xf800)>>8;
//					do_write(f, (tmp&0x001f)<<3, 1);		// b5
//					do_write(f, (tmp&0x07e0)>>3, 1);		// g6
//					do_write(f, (tmp&0xf800)>>8, 1);		// r5
				}
				fs_write(f, buf, 640*3);
				debug("line %d saved.\n", y);
				if (gReturnToMenu) {
					break;
				}
				// 640*3 exactly divides by 4, so no line padding needed
			}

			fs_close(f);
			debug("Screenshot done.\n");
		} else {
			debug("Failed to open screenshot file!\n");
		}
	}
#endif
}
