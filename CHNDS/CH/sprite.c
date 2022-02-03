/****************************************/
/* Cool Herders                         */
/* http://harmlesslion.com              */
/* Copyright 2009 HarmlessLion LLC      */
/* sprite.c                             */
/****************************************/

/* Ported to KOS 1.1.x by Dan Potter */
/* ported to DS by Mike Brent again */
/* Sprites are tiles fixed at 48x48 pixels */

#include <stdio.h>
#include "kosemulation.h"

#include "sprite.h"
#include "cool.h"

// CARD library
#include <nitro/fs.h>

extern pvr_ptr_t txr_level;
 
extern int nFrames;
extern int nLast3DCount;

static char mapbuf[11000];		// big enough to load the whole file

// table of rotation values for this sprite system with 128 entries,
// each containing 4 offset coordinates for the sprite position
// generated with this BASIC code in Blassic (and tested on an Apple2, eheh):
#if 0
     10 REM points to rotate around center
     20 DIM px(4),py(4)
     30 px(1)=-23:py(1)=-31
     40 px(2)=-23:py(2)=16
     50 px(3)=24:py(3)=16
     60 px(4)=24:py(4)=-31
    100 FOR a=0 TO 6.23 STEP (6.23/100)
    105 PRINT "  ";
    110 REM calculate the new rotation for this step for each point
    120 FOR b=1 TO 4
    130 tx=px(b)*COS(a)-py(b)*SIN(a)
    140 ty=px(b)*SIN(a)+py(b)*COS(a)
    145 REM convert to integer offsets
    146 tx=INT(tx-px(b)+.5):ty=INT(ty-py(b)+.5)
    150 PRINT tx;",";ty;", ";
    160 NEXT b
    170 PRINT
    180 NEXT a
#endif
const char RotationTable[128*8] = {
  0,0, 0,0, 0,0, 0,0,
  2,-1, -1,-1, -1,1, 1,1,
  3,-2, -1,-2, -2,2, 3,2,
  5,-3, -2,-4, -3,3, 4,4,
  6,-4, -3,-5, -4,4, 6,5,
  8,-5, -3,-6, -5,5, 7,7,
  10,-5, -4,-7, -6,6, 8,8,
  12,-6, -4,-9, -7,7, 9,10,
  13,-6, -4,-10, -8,8, 10,11,
  15,-7, -5,-11, -9,9, 11,13,
  17,-7, -5,-13, -10,9, 12,15,
  19,-7, -5,-14, -12,10, 12,17,
  21,-8, -5,-15, -13,11, 13,18,
  23,-8, -5,-17, -14,11, 14,20,
  25,-8, -5,-18, -15,12, 14,22,
  27,-7, -5,-19, -17,12, 15,24,
  28,-7, -5,-21, -18,12, 15,26,
  30,-7, -4,-22, -20,12, 15,28,
  32,-7, -4,-23, -21,13, 15,30,
  34,-6, -4,-25, -22,13, 15,32,
  36,-5, -3,-26, -24,13, 15,33,
  37,-5, -3,-27, -25,13, 15,35,
  39,-4, -2,-29, -27,13, 15,37,
  41,-3, -1,-30, -28,13, 14,39,
  43,-2, -1,-31, -29,12, 14,41,
  44,-1, 0,-32, -31,12, 13,43,
  46,0, 1,-33, -32,12, 13,45,
  47,1, 2,-34, -33,11, 12,46,
  49,2, 3,-35, -35,11, 11,48,
  50,3, 4,-36, -36,10, 10,50,
  51,5, 5,-37, -37,10, 9,51,
  53,6, 6,-38, -38,9, 8,53,
  54,8, 7,-39, -40,8, 7,55,
  55,9, 8,-40, -41,7, 6,56,
  56,11, 9,-40, -42,7, 5,58,
  57,12, 10,-41, -43,6, 4,59,
  58,14, 11,-42, -44,5, 2,60,
  58,16, 13,-42, -45,4, 1,61,
  59,17, 14,-43, -46,3, -1,63,
  60,19, 15,-43, -47,2, -2,64,
  60,21, 17,-43, -48,0, -4,65,
  61,23, 18,-44, -48,-1, -6,66,
  61,25, 19,-44, -49,-2, -7,66,
  61,27, 21,-44, -50,-3, -9,67,
  62,28, 22,-44, -50,-4, -11,68,
  62,30, 23,-44, -51,-6, -13,69,
  62,32, 25,-44, -51,-7, -15,69,
  61,34, 26,-44, -52,-8, -16,69,
  61,36, 27,-44, -52,-10, -18,70,
  61,38, 29,-43, -52,-11, -20,70,
  61,40, 30,-43, -53,-13, -22,70,
  60,41, 31,-43, -53,-14, -24,70,
  60,43, 33,-42, -53,-15, -26,70,
  59,45, 34,-42, -53,-17, -28,70,
  58,47, 35,-41, -53,-18, -30,70,
  57,48, 36,-41, -53,-20, -32,69,
  57,50, 38,-40, -52,-21, -33,69,
  56,52, 39,-39, -52,-22, -35,69,
  55,53, 40,-38, -52,-24, -37,68,
  53,55, 41,-38, -51,-25, -39,67,
  52,56, 42,-37, -51,-26, -41,67,
  51,58, 43,-36, -50,-28, -42,66,
  50,59, 44,-35, -50,-29, -44,65,
  48,60, 45,-34, -49,-30, -46,64,
  47,61, 46,-33, -48,-31, -47,63,
  45,63, 46,-31, -48,-33, -49,61,
  44,64, 47,-30, -47,-34, -50,60,
  42,65, 48,-29, -46,-35, -52,59,
  40,65, 48,-28, -45,-36, -53,58,
  39,66, 49,-27, -44,-37, -54,56,
  37,67, 49,-25, -43,-38, -55,55,
  35,68, 50,-24, -42,-39, -56,53,
  34,68, 50,-23, -41,-39, -57,51,
  32,69, 50,-21, -40,-40, -58,50,
  30,69, 51,-20, -38,-41, -59,48,
  28,69, 51,-19, -37,-42, -60,46,
  26,69, 51,-17, -36,-42, -61,45,
  24,70, 51,-16, -35,-43, -61,43,
  22,70, 51,-15, -33,-43, -62,41,
  20,70, 51,-13, -32,-44, -62,39,
  19,69, 51,-12, -31,-44, -63,37,
  17,69, 50,-11, -29,-44, -63,35,
  15,69, 50,-9, -28,-45, -63,33,
  13,68, 50,-8, -26,-45, -63,32,
  11,68, 49,-7, -25,-45, -63,30,
  10,67, 49,-5, -24,-45, -63,28,
  8,66, 48,-4, -22,-45, -63,26,
  6,66, 48,-3, -21,-45, -63,24,
  4,65, 47,-2, -19,-44, -62,22,
  3,64, 46,-1, -18,-44, -62,20,
  1,63, 46,1, -17,-44, -61,18,
  0,62, 45,2, -15,-44, -60,17,
  -2,61, 44,3, -14,-43, -60,15,
  -3,59, 43,4, -13,-43, -59,13,
  -5,58, 42,5, -11,-42, -58,11,
  -6,57, 41,5, -10,-41, -57,10,
  -7,55, 40,6, -9,-41, -56,8,
  -8,54, 39,7, -8,-40, -55,7,
  -9,52, 38,8, -7,-39, -54,5,
  -10,51, 36,9, -6,-38, -52,4,
  -11,49, 35,9, -4,-37, -51,3,
  -12,47, 34,10, -3,-36, -50,1,
  -13,46, 33,10, -3,-35, -48,0,
  -13,44, 31,11, -2,-34, -46,-1,
  -14,42, 30,11, -1,-33, -45,-2,
  -14,40, 29,11, 0,-32, -43,-3,
  -15,38, 27,12, 1,-31, -42,-4,
  -15,36, 26,12, 1,-30, -40,-5,
  -15,35, 25,12, 2,-28, -38,-6,
  -16,33, 23,12, 3,-27, -36,-6,
  -16,31, 22,12, 3,-26, -34,-7,
  -16,29, 21,12, 4,-24, -33,-7,
  -15,27, 19,12, 4,-23, -31,-8,
  -15,25, 18,12, 4,-22, -29,-8,
  -15,23, 17,11, 5,-20, -27,-8,
  -14,22, 15,11, 5,-19, -25,-8,
  -14,20, 14,11, 5,-17, -23,-8,
  -13,18, 13,10, 5,-16, -21,-8,
  -13,16, 11,10, 5,-15, -19,-8,
  -12,15, 10,9, 5,-13, -17,-8,
  -11,13, 9,8, 5,-12, -16,-7,
  -10,11, 8,8, 4,-10, -14,-7,
  -9,10, 7,7, 4,-9, -12,-6,
  -8,8, 6,6, 4,-8, -10,-6,
  -7,7, 5,5, 3,-6, -8,-5,
  -6,5, 4,4, 3,-5, -7,-4,
  -4,4, 3,3, 2,-4, -5,-3,
  -3,2, 2,2, 2,-3, -3,-2
};
	

int nOffsetX, nOffsetY;
int nGlobalDarken = 0;			// darken value to subtract from colors (used anywhere the color is set!)

// sets the current color mode, but takes the global darken into account
// please use this instead of G3_Color unless you mean it
void myG3_Color(int r, int g, int b) {
	if (nGlobalDarken) {
		r-=nGlobalDarken; if (r<0) r=0;
		g-=nGlobalDarken; if (g<0) g=0;
		b-=nGlobalDarken; if (b<0) b=0;
	}
	G3_Color(GX_RGB(r,g,b));
}
void Set3DDarken(int val) {
	nGlobalDarken = val;
}

// Sets the screen offset used by drawsprite (and maybe others)
// Blocks at the edges of the screen
void SetOffset(int x, int y) 
{
	if (x>0) 
	{
		if (x>383) 
		{
			nOffsetX=383;
		}
		else 
		{
			nOffsetX=x;
		}
	}
	else 
	{
		nOffsetX=0;
	}
	if (y>0)
	{
		if (y > 287) 
		{
			nOffsetY=287;
		}
		else 
		{
			nOffsetY=y;
		}
	}
	else
	{
		nOffsetY=0;
	}
	
}

// NOTE: Both our textures (BMP) and the screen are mapped with Y=0 at the BOTTOM, not the top!
// The Map's Y is already fixed up appropriately.
// Polygon number is taken because polys with the same ID will not alpha blend together
// No explicit shading happens in this function
void SortSprite(SPRITE *spr, TXRMAP *map, int PolyNo) {
	int u1, u2, v1, v2;
	int tmpX1, tmpY1, tmpX2, tmpY2;
	fx16 sz1,sz2;
	fx32 sx1, sx2, sy1, sy2;
	fx32 tx1,ty1,tx2,ty2;
	int tile;
	int hflip=0, vflip=0;
	int w,h;

	nLast3DCount++;

	tile=spr->tilenumber;

	// texture addresses are 8x8
	tmpX1=(spr->x-((TILESIZE-GRIDSIZE)>>1));// X is centered
	tmpY1=(spr->y-(TILESIZE-GRIDSIZE));		// Y is drawn from the bottom

	w=map[tile].w;
	h=map[tile].h;	
	
	// Check for sprite flips
	if (w < 0) {
		w=-w;
		hflip=1;
	}
	if (h < 0) {
		h=-h;
		vflip=1;
	}

	// The image map contains the texture offsets, real size, and pixel offset
	// texture coordinates
	u1=map[tile].x;
	v1=map[tile].y;
	u2=u1+w;
	v2=v1-h;
	// screen coordinates 
	tmpX1+=map[tile].offx;
	tmpY1+=map[tile].offy;
	tmpX2=tmpX1+w;
	tmpY2=tmpY1+h;
	 
	// TODO: REMOVE ME - HACK FOR THE EMULATOR?
	// I submitted a fix for the identity projection and was told
	// that the emulator's math was 100% correct. I guess it's
	// hardware that is wrong... well, that's okay, this code
	// is written for hardware. But that's why most emulators
	// have black lines on the background, they are all using the
	// off-by-one math. DraStic is the only one I tried that works
	// like hardware. Otherwise, this fix makes the emulator render
	// correctly (but makes hardware render improperly.)
#if 0
	v1+=1; 
	v2+=1;
#endif
	
    // these are still 640x480 coordinates - translate to something useful
	#define FX32_ONEPOINTFRAC (FX32_ONE+FX32_HALF)
	sx1=ConvX(tmpX1);
    sx2=ConvX(tmpX2);
    // if well offscreen, just drop out
	if ((sx1>FX32_ONEPOINTFRAC)||(sx2>FX32_ONEPOINTFRAC)||(sx1<-FX32_ONEPOINTFRAC)||(sx2<-FX32_ONEPOINTFRAC)) 
	{
		return;
	}

    sy1=ConvY(tmpY1);
    sy2=ConvY(tmpY2);
    // if well offscreen, just drop out
	if ((sy1>FX32_ONEPOINTFRAC)||(sy2>FX32_ONEPOINTFRAC)||(sy1<-FX32_ONEPOINTFRAC)||(sy2<-FX32_ONEPOINTFRAC)) 
	{
		return;
	}
    
    sz2=spr->z;
    if (spr->is3D) {
    	sz1=sz2-(fx16)0x30;
    } else {
    	sz1=sz2;
    }

	// we support various bit depth and texture sizes
	switch (spr->nDepth) 
	{
		case DEPTH_256x256x8:
			G3_TexImageParam(GX_TEXFMT_PLTT256, GX_TEXGEN_TEXCOORD, GX_TEXSIZE_S256, GX_TEXSIZE_T256, 
					GX_TEXREPEAT_NONE, GX_TEXFLIP_NONE, GX_TEXPLTTCOLOR0_TRNS, spr->txr_addr);
			G3_TexPlttBase(spr->pal_addr, GX_TEXFMT_PLTT256);
			break;
		
		case DEPTH_256x256x4:
			G3_TexImageParam(GX_TEXFMT_PLTT16, GX_TEXGEN_TEXCOORD, GX_TEXSIZE_S256, GX_TEXSIZE_T256, 
				GX_TEXREPEAT_NONE, GX_TEXFLIP_NONE, GX_TEXPLTTCOLOR0_TRNS, spr->txr_addr);
			G3_TexPlttBase(spr->pal_addr, GX_TEXFMT_PLTT16);
			break;
			
		case DEPTH_512x512x8:
			G3_TexImageParam(GX_TEXFMT_PLTT256, GX_TEXGEN_TEXCOORD, GX_TEXSIZE_S512, GX_TEXSIZE_T512, 
					GX_TEXREPEAT_NONE, GX_TEXFLIP_NONE, GX_TEXPLTTCOLOR0_TRNS, spr->txr_addr);
			G3_TexPlttBase(spr->pal_addr, GX_TEXFMT_PLTT256);
			break;
	}

    G3_PolygonAttr(GX_LIGHTMASK_NONE,  		// no lights
                   GX_POLYGONMODE_MODULATE,	// graphic mode
                   GX_CULL_NONE,       		// cull none
                   PolyNo,             		// polygon ID(0 - 63)
                   spr->alpha,				// alpha(0 - 31 0-wireframe,31=opaque)
                   GX_POLYGON_ATTR_MISC_XLU_DEPTH_UPDATE	// OR of GXPolygonAttrMisc's value
        );	
        
	// textures
	if (hflip)
	{
	    tx1=(u2-1)*FX32_ONE;
	    tx2=(u1-1)*FX32_ONE;
	}
	else 
	{
	    tx1=u1*FX32_ONE;
	    tx2=u2*FX32_ONE;
	}
	
	if (vflip) 
	{
	    ty1=(v2-1)*FX32_ONE;
	    ty2=(v1-1)*FX32_ONE;
	}
	else 
	{
	    ty1=v1*FX32_ONE;
	    ty2=v2*FX32_ONE;		
	}

	G3_Begin(GX_BEGIN_QUADS);
		// counter-clockwise for quads (versus the old triangle list order)
		// offsetting z with y here doesn't seem to help the priority when
		// players bump each other vertically
		G3_TexCoord(tx1,ty1);
		G3_Vtx(sx1, sy1, sz1);

		G3_TexCoord(tx1,ty2);
		G3_Vtx(sx1, sy2, sz2);

		G3_TexCoord(tx2, ty2);
		G3_Vtx(sx2, sy2, sz2);

		G3_TexCoord(tx2, ty1);
		G3_Vtx(sx2, sy1, sz1);
	G3_End();
}

// NOTE: Both our textures (BMP) and the screen are mapped with Y=0 at the BOTTOM, not the top!
// The Map's Y is already fixed up appropriately.
// Polygon number is taken because polys with the same ID will not alpha blend together
// No explicit shading happens in this function
// Same as SortSprite, but the is3D variable is instead a 2D rotation value around the center of the sprite
// This will only work, though, if the sprite is actually 48x48 pixels - smaller 'compressed' sprites
// will distort as they rotate.
void SortSpriteRotated(SPRITE *spr, TXRMAP *map, int PolyNo) {
	int u1, u2, v1, v2;
	int tmpX1, tmpY1, tmpX2, tmpY2;
	int outX1,outY1, outX2,outY2, outX3,outY3, outX4,outY4;
	fx16 sz1;
	fx32 sx1, sx2, sx3, sx4, sy1, sy2, sy3, sy4;
	fx32 tx1,ty1,tx2,ty2;
	int tile;
	int hflip=0, vflip=0;
	int w,h;

	nLast3DCount++;
	
	tile=spr->tilenumber;

	// texture addresses are 8x8
	tmpX1=(spr->x-((TILESIZE-GRIDSIZE)>>1));// X is centered
	tmpY1=(spr->y-(TILESIZE-GRIDSIZE));		// Y is drawn from the bottom

	w=map[tile].w;
	h=map[tile].h;	
	
	// Check for sprite flips
	if (w < 0) {
		w=-w;
		hflip=1;
	}
	if (h < 0) {
		h=-h;
		vflip=1;
	}

	// The image map contains the texture offsets, real size, and pixel offset
	// texture coordinates
	u1=map[tile].x;
	v1=map[tile].y;
	u2=u1+w;
	v2=v1-h;
	// screen coordinates
	tmpX1+=map[tile].offx;
	tmpY1+=map[tile].offy;
	tmpX2=tmpX1+w;
	tmpY2=tmpY1+h;
	// get individual coordinates so we can rotate them if needed
	outX1=tmpX1; outY1=tmpY1;
	outX2=tmpX1; outY2=tmpY2;
	outX3=tmpX2; outY3=tmpY2;
	outX4=tmpX2; outY4=tmpY1;
	
	// rotate if needed
	if (spr->is3D) {
		// x of the player is already centered
		// y needs to have 16 added to it to reach the center
		outX1+=RotationTable[(spr->is3D&0x7f)<<3];
		outY1+=RotationTable[((spr->is3D&0x7f)<<3)+1];
		outX2+=RotationTable[((spr->is3D&0x7f)<<3)+2];
		outY2+=RotationTable[((spr->is3D&0x7f)<<3)+3];
		outX3+=RotationTable[((spr->is3D&0x7f)<<3)+4];
		outY3+=RotationTable[((spr->is3D&0x7f)<<3)+5];
		outX4+=RotationTable[((spr->is3D&0x7f)<<3)+6];
		outY4+=RotationTable[((spr->is3D&0x7f)<<3)+7];
	}
	
    // these are still 640x480 coordinates - translate to something useful
	#define FX32_ONEPOINTFRAC (FX32_ONE+FX32_HALF)
	sx1=ConvX(outX1);
    sx2=ConvX(outX2);
    sx3=ConvX(outX3);
    sx4=ConvX(outX4);
    // if well offscreen, just drop out
	if ((sx1>FX32_ONEPOINTFRAC)||(sx2>FX32_ONEPOINTFRAC)||(sx3>FX32_ONEPOINTFRAC)||(sx4>FX32_ONEPOINTFRAC)||(sx1<-FX32_ONEPOINTFRAC)||(sx2<-FX32_ONEPOINTFRAC)||(sx3<-FX32_ONEPOINTFRAC)||(sx4<-FX32_ONEPOINTFRAC)) 
	{
		return;
	}

    sy1=ConvY(outY1);
    sy2=ConvY(outY2);
    sy3=ConvY(outY3);
    sy4=ConvY(outY4);
    // if well offscreen, just drop out
	if ((sy1>FX32_ONEPOINTFRAC)||(sy2>FX32_ONEPOINTFRAC)||(sy3>FX32_ONEPOINTFRAC)||(sy4>FX32_ONEPOINTFRAC)||(sy1<-FX32_ONEPOINTFRAC)||(sy2<-FX32_ONEPOINTFRAC)||(sy3<-FX32_ONEPOINTFRAC)||(sy4<-FX32_ONEPOINTFRAC)) 
	{
		return;
	}
    
    sz1=spr->z;

	// we support various bit depth and texture sizes
	switch (spr->nDepth) 
	{
		case DEPTH_256x256x8:
			G3_TexImageParam(GX_TEXFMT_PLTT256, GX_TEXGEN_TEXCOORD, GX_TEXSIZE_S256, GX_TEXSIZE_T256, 
					GX_TEXREPEAT_NONE, GX_TEXFLIP_NONE, GX_TEXPLTTCOLOR0_TRNS, spr->txr_addr);
			G3_TexPlttBase(spr->pal_addr, GX_TEXFMT_PLTT256);
			break;
		
		case DEPTH_256x256x4:
			G3_TexImageParam(GX_TEXFMT_PLTT16, GX_TEXGEN_TEXCOORD, GX_TEXSIZE_S256, GX_TEXSIZE_T256, 
				GX_TEXREPEAT_NONE, GX_TEXFLIP_NONE, GX_TEXPLTTCOLOR0_TRNS, spr->txr_addr);
			G3_TexPlttBase(spr->pal_addr, GX_TEXFMT_PLTT16);
			break;
			
		case DEPTH_512x512x8:
			G3_TexImageParam(GX_TEXFMT_PLTT256, GX_TEXGEN_TEXCOORD, GX_TEXSIZE_S512, GX_TEXSIZE_T512, 
					GX_TEXREPEAT_NONE, GX_TEXFLIP_NONE, GX_TEXPLTTCOLOR0_TRNS, spr->txr_addr);
			G3_TexPlttBase(spr->pal_addr, GX_TEXFMT_PLTT256);
			break;
	}

    G3_PolygonAttr(GX_LIGHTMASK_NONE,  		// no lights
                   GX_POLYGONMODE_MODULATE,	// graphic mode
                   GX_CULL_NONE,       		// cull none
                   PolyNo,             		// polygon ID(0 - 63)
                   spr->alpha,				// alpha(0 - 31 0-wireframe,31=opaque)
                   GX_POLYGON_ATTR_MISC_XLU_DEPTH_UPDATE	// OR of GXPolygonAttrMisc's value
        );	
        
	// textures
	if (hflip)
	{
	    tx1=(u2-1)*FX32_ONE;
	    tx2=(u1-1)*FX32_ONE;
	}
	else 
	{
	    tx1=u1*FX32_ONE;
	    tx2=u2*FX32_ONE;
	}
	
	if (vflip) 
	{
	    ty1=(v2-1)*FX32_ONE;
	    ty2=(v1-1)*FX32_ONE;
	}
	else 
	{
	    ty1=v1*FX32_ONE;
	    ty2=v2*FX32_ONE;		
	}

	G3_Begin(GX_BEGIN_QUADS);
		// counter-clockwise for quads (versus the old triangle list order)
		// offsetting z with y here doesn't seem to help the priority when
		// players bump each other vertically
		G3_TexCoord(tx1,ty1);
		G3_Vtx(sx1, sy1, sz1);

		G3_TexCoord(tx1,ty2);
		G3_Vtx(sx2, sy2, sz1);

		G3_TexCoord(tx2, ty2);
		G3_Vtx(sx3, sy3, sz1);

		G3_TexCoord(tx2, ty1);
		G3_Vtx(sx4, sy4, sz1);
	G3_End();
}

// NOTE: Both our textures (BMP) and the screen are mapped with Y=0 at the BOTTOM, not the top!
// The Map's Y is already fixed up appropriately.
// Polygon number is taken because polys with the same ID will not alpha blend together
// No explicit shading happens in this function
// This function squashes a sprite on the X and Y axes, taking percentages
void SortSpriteSquashed(SPRITE *spr, TXRMAP *map, int PolyNo, int xPercent, int yPercent) {
	int u1, u2, v1, v2;
	int tmpX1, tmpY1, tmpX2, tmpY2;
	fx16 sz1,sz2;
	fx32 sx1, sx2, sy1, sy2;
	fx32 tx1,ty1,tx2,ty2;
	int tile;
	int hflip=0, vflip=0;
	int w,h;
	int wOut, hOut;

	nLast3DCount++;
	
	tile=spr->tilenumber;

	// texture addresses are 8x8
	tmpX1=(spr->x-((TILESIZE-GRIDSIZE)>>1));// X is centered
	tmpY1=(spr->y-(TILESIZE-GRIDSIZE));		// Y is drawn from the bottom

	w=map[tile].w;
	h=map[tile].h;	
	
	// Check for sprite flips
	if (w < 0) {
		w=-w;
		hflip=1;
	}
	if (h < 0) {
		h=-h;
		vflip=1;
	}
	wOut = w*xPercent/100;
	hOut = h*yPercent/100;

	// The image map contains the texture offsets, real size, and pixel offset
	// texture coordinates
	u1=map[tile].x;
	v1=map[tile].y;
	u2=u1+w;
	v2=v1-h;
	// screen coordinates
	tmpX1+=map[tile].offx+((w-wOut)/2);	// stretch out from middle
	tmpY1+=map[tile].offy+(h-hOut);		// rise up from bottom
	tmpX2=tmpX1+wOut;
	tmpY2=tmpY1+hOut;
	
    // these are still 640x480 coordinates - translate to something useful
	#define FX32_ONEPOINTFRAC (FX32_ONE+FX32_HALF)
	sx1=ConvX(tmpX1);
    sx2=ConvX(tmpX2);
    // if well offscreen, just drop out
	if ((sx1>FX32_ONEPOINTFRAC)||(sx2>FX32_ONEPOINTFRAC)||(sx1<-FX32_ONEPOINTFRAC)||(sx2<-FX32_ONEPOINTFRAC)) 
	{
		return;
	}

    sy1=ConvY(tmpY1);
    sy2=ConvY(tmpY2);
    // if well offscreen, just drop out
	if ((sy1>FX32_ONEPOINTFRAC)||(sy2>FX32_ONEPOINTFRAC)||(sy1<-FX32_ONEPOINTFRAC)||(sy2<-FX32_ONEPOINTFRAC)) 
	{
		return;
	}
    
    sz2=spr->z;
    if (spr->is3D) {
    	sz1=sz2-(fx16)0x30;
    } else {
    	sz1=sz2;
    }

	// we support various bit depth and texture sizes
	switch (spr->nDepth) 
	{
		case DEPTH_256x256x8:
			G3_TexImageParam(GX_TEXFMT_PLTT256, GX_TEXGEN_TEXCOORD, GX_TEXSIZE_S256, GX_TEXSIZE_T256, 
					GX_TEXREPEAT_NONE, GX_TEXFLIP_NONE, GX_TEXPLTTCOLOR0_TRNS, spr->txr_addr);
			G3_TexPlttBase(spr->pal_addr, GX_TEXFMT_PLTT256);
			break;
		
		case DEPTH_256x256x4:
			G3_TexImageParam(GX_TEXFMT_PLTT16, GX_TEXGEN_TEXCOORD, GX_TEXSIZE_S256, GX_TEXSIZE_T256, 
				GX_TEXREPEAT_NONE, GX_TEXFLIP_NONE, GX_TEXPLTTCOLOR0_TRNS, spr->txr_addr);
			G3_TexPlttBase(spr->pal_addr, GX_TEXFMT_PLTT16);
			break;
			
		case DEPTH_512x512x8:
			G3_TexImageParam(GX_TEXFMT_PLTT256, GX_TEXGEN_TEXCOORD, GX_TEXSIZE_S512, GX_TEXSIZE_T512, 
					GX_TEXREPEAT_NONE, GX_TEXFLIP_NONE, GX_TEXPLTTCOLOR0_TRNS, spr->txr_addr);
			G3_TexPlttBase(spr->pal_addr, GX_TEXFMT_PLTT256);
			break;
	}

    G3_PolygonAttr(GX_LIGHTMASK_NONE,  		// no lights
                   GX_POLYGONMODE_MODULATE,	// graphic mode
                   GX_CULL_NONE,       		// cull none
                   PolyNo,             		// polygon ID(0 - 63)
                   spr->alpha,				// alpha(0 - 31 0-wireframe,31=opaque)
                   GX_POLYGON_ATTR_MISC_XLU_DEPTH_UPDATE	// OR of GXPolygonAttrMisc's value
        );	
        
	// textures
	if (hflip)
	{
	    tx1=(u2-1)*FX32_ONE;
	    tx2=(u1-1)*FX32_ONE;
	}
	else 
	{
	    tx1=u1*FX32_ONE;
	    tx2=u2*FX32_ONE;
	}
	
	if (vflip) 
	{
	    ty1=(v2-1)*FX32_ONE;
	    ty2=(v1-1)*FX32_ONE;
	}
	else 
	{
	    ty1=v1*FX32_ONE;
	    ty2=v2*FX32_ONE;		
	}

	G3_Begin(GX_BEGIN_QUADS);
		// counter-clockwise for quads (versus the old triangle list order)
		// offsetting z with y here doesn't seem to help the priority when
		// players bump each other vertically
		G3_TexCoord(tx1,ty1);
		G3_Vtx(sx1, sy1, sz1);

		G3_TexCoord(tx1,ty2);
		G3_Vtx(sx1, sy2, sz2);

		G3_TexCoord(tx2, ty2);
		G3_Vtx(sx2, sy2, sz2);

		G3_TexCoord(tx2, ty1);
		G3_Vtx(sx2, sy1, sz1);
	G3_End();
}

// Add a full 256x192 page as a positioned sprite with alpha, stretching onscreen
void scaleimage(pvr_ptr_t txr, pvr_ptr_t pal, int ix1, int iy1, int ix2, int iy2, int alpha) {
	fx32 sx1, sx2, sy1, sy2;
	fx32 tx1,ty1,tx2,ty2;
	
	nLast3DCount++;
	
	G3_TexImageParam(GX_TEXFMT_PLTT256, GX_TEXGEN_TEXCOORD, GX_TEXSIZE_S256, GX_TEXSIZE_T256, 
			GX_TEXREPEAT_NONE, GX_TEXFLIP_NONE, GX_TEXPLTTCOLOR0_USE, txr);
	G3_TexPlttBase(pal, GX_TEXFMT_PLTT256);
	
	// clamp alpha
	if (alpha < 1) alpha=1;
	if (alpha > 31) alpha=31;
	
    G3_PolygonAttr(GX_LIGHTMASK_NONE,  	// no lights
               GX_POLYGONMODE_MODULATE,	// graphic mode
               GX_CULL_NONE,       		// cull none
               POLY_MISC,          		// polygon ID(0 - 63)
               alpha,					// alpha(0 - 31 0-wireframe,31=opaque)
               GX_POLYGON_ATTR_MISC_XLU_DEPTH_UPDATE	// OR of GXPolygonAttrMisc's value
    );

	// convert screen coordinates to fixed point coordinates
	sx1=FX32_ONE*((ix1-128)/128.0f);
	sx2=FX32_ONE*((ix2-128)/128.0f);
	sy1=FX32_ONE*((96-iy1)/96.0f);
	sy2=FX32_ONE*((96-iy2)/96.0f);
	tx1=0*FX32_ONE;
	tx2=255*FX32_ONE;
	ty1=191*FX32_ONE;	// invert the Y axis
	ty2=0*FX32_ONE;
	
	G3_Begin(GX_BEGIN_QUADS);
		// counter-clockwise for quads (versus the old triangle list order)
		G3_TexCoord(tx1,ty1);
		G3_Vtx(sx1, sy1, 0);

		G3_TexCoord(tx1,ty2);
		G3_Vtx(sx1, sy2, 0);

		G3_TexCoord(tx2, ty2);
		G3_Vtx(sx2, sy2, 0);

		G3_TexCoord(tx2, ty1);
		G3_Vtx(sx2, sy1, 0);
	G3_End();
}

// Add a full screen 256x192x16bit texture for story mode
// NOTE: Both our textures (BMP) and the screen are mapped with Y=0 at the BOTTOM, not the top!
// Polygon number is taken because polys with the same ID will not alpha blend together
// No explicit shading happens in this function
void addPageStory(pvr_ptr_t txr, fx16 z, int nAlpha) {
	// these are true color images	
	G3_TexImageParam(GX_TEXFMT_DIRECT, GX_TEXGEN_TEXCOORD, GX_TEXSIZE_S256, GX_TEXSIZE_T256, 
			GX_TEXREPEAT_NONE, GX_TEXFLIP_NONE, GX_TEXPLTTCOLOR0_USE, txr);

    G3_PolygonAttr(GX_LIGHTMASK_NONE,  		// no lights
                   GX_POLYGONMODE_DECAL,	// graphic mode
                   GX_CULL_NONE,       		// cull none
                   z&63,           			// polygon ID(0 - 63)
                   nAlpha,					// alpha(0 - 31 0-wireframe,31=opaque)
                   0						// OR of GXPolygonAttrMisc's value
        );	
        
	G3_Begin(GX_BEGIN_QUADS);
		// counter-clockwise for quads (versus the old triangle list order)
		G3_TexCoord(0,0);
		G3_Vtx(-FX16_ONE, -FX16_ONE, z);

		G3_TexCoord(0,191*FX32_ONE);
		G3_Vtx(-FX16_ONE, FX16_ONE, z);

		G3_TexCoord(255*FX32_ONE, 191*FX32_ONE);
		G3_Vtx(FX16_ONE, FX16_ONE, z);

		G3_TexCoord(255*FX32_ONE, 0);
		G3_Vtx(FX16_ONE, -FX16_ONE, z);
	G3_End();
}

// Sort a 256x192x8bit image - darken is 0 for black and 31 for full bright
void SortFullPictureX(GXTexFmt fmt, pvr_ptr_t pLevel, pvr_ptr_t pPal, int nDarken) {
	nLast3DCount++;
	
    myG3_Color(nDarken,nDarken,nDarken);

	G3_TexImageParam(fmt, GX_TEXGEN_TEXCOORD, GX_TEXSIZE_S256, GX_TEXSIZE_T256, 
		GX_TEXREPEAT_NONE, GX_TEXFLIP_NONE, GX_TEXPLTTCOLOR0_USE, pLevel);

	G3_TexPlttBase(pPal, fmt);
	
    G3_PolygonAttr(GX_LIGHTMASK_NONE,  		// no lights
                   GX_POLYGONMODE_MODULATE, // modulate for the fade
                   GX_CULL_NONE,       		// cull none
                   POLY_BG,            		// polygon ID(0 - 63)
                   31,                 		// alpha(0 - 31 0-wireframe,31=opaque)
                   0                   		// OR of GXPolygonAttrMisc's value
        );
        
	// Position 0
	G3_Begin(GX_BEGIN_QUADS);
		// counter-clockwise for quads (versus the old triangle list order)
		G3_TexCoord(0,0);
		G3_Vtx(-FX16_ONE, -FX16_ONE, FX16_ONE);

		G3_TexCoord(0,191*FX32_ONE);
		G3_Vtx(-FX16_ONE, FX16_ONE, FX16_ONE);

		G3_TexCoord(255*FX32_ONE, 191*FX32_ONE);
		G3_Vtx(FX16_ONE, FX16_ONE, FX16_ONE);

		G3_TexCoord(255*FX32_ONE, 0);
		G3_Vtx(FX16_ONE, -FX16_ONE, FX16_ONE);
	G3_End();

	// don't let anything else carry our changed shade	
    myG3_Color(31,31,31);
}

// Sort a 256x192x8bit image from a larger texture for the menu
// - darken is 0 for black and 31 for full bright
// Thus, xoff and yoff are relative to the title screen, which
// is actually in the middle. Any positive yoff means the
// bottom image, which is actually stored top right in the texture
#define SORTMENUQUAD(tx,ty,xo,yo)											\
		G3_TexCoord(tx*FX32_ONE,ty*FX32_ONE);					\
		G3_Vtx(-FX16_ONE+(xo), -FX16_ONE+(yo), FX16_ONE);					\
																			\
		G3_TexCoord(tx*FX32_ONE,(ty+191)*FX32_ONE);			\
		G3_Vtx(-FX16_ONE+(xo), FX16_ONE+(yo), FX16_ONE);					\
																			\
		G3_TexCoord((tx+255)*FX32_ONE, (ty+191)*FX32_ONE);		\
		G3_Vtx(FX16_ONE+(xo), FX16_ONE+(yo), FX16_ONE);						\
																			\
		G3_TexCoord((tx+255)*FX32_ONE, ty*FX32_ONE);			\
		G3_Vtx(FX16_ONE+(xo), -FX16_ONE+(yo), FX16_ONE);			


void SortMenuPictureX(pvr_ptr_t pLevel, pvr_ptr_t pPal, int nDarken, int xoff, int yoff) {

	fx16 xdiff=(xoff*(FX16_ONE/128));	// works evenly
	fx16 ydiff=(yoff*(FX16_ONE/96));	// subject to rounding errors

	nLast3DCount++;

    myG3_Color(nDarken,nDarken,nDarken);

	G3_TexImageParam(GX_TEXFMT_PLTT256, GX_TEXGEN_TEXCOORD, GX_TEXSIZE_S1024, GX_TEXSIZE_T512,
		GX_TEXREPEAT_NONE, GX_TEXFLIP_NONE, GX_TEXPLTTCOLOR0_USE, pLevel);

	G3_TexPlttBase(pPal, GX_TEXFMT_PLTT256);
	
    G3_PolygonAttr(GX_LIGHTMASK_NONE,  		// no lights
                   GX_POLYGONMODE_MODULATE, // modulate for the fade
                   GX_CULL_NONE,       		// cull none
                   POLY_BG,            		// polygon ID(0 - 63)
                   31,                 		// alpha(0 - 31 0-wireframe,31=opaque)
                   0                   		// OR of GXPolygonAttrMisc's value
        );
        
	// First quad - main display
	// we just draw 5 quads all the time, for simplicity. It's not like the
	// hardware can't deal with it. :)
	G3_Begin(GX_BEGIN_QUADS);
		// counter-clockwise for quads (versus the old triangle list order)
		SORTMENUQUAD(256,0,xdiff,ydiff);					// title page
		SORTMENUQUAD(256,192,xdiff,ydiff+(FX16_ONE*2));		// north
		SORTMENUQUAD(512,192,xdiff,ydiff-(FX16_ONE*2));		// south
		SORTMENUQUAD(0,0,xdiff-(FX16_ONE*2),ydiff);			// east
		SORTMENUQUAD(512,0,xdiff+(FX16_ONE*2),ydiff);		// west
	G3_End();

	// don't let anything else carry our changed shade	
    myG3_Color(31,31,31);
}


// Load a layout texture map so we can map the gfx transparently back to the original 48x48 size
// NOTE: BMPs load upside down, and our SCREEN maps upside down, too. So we just remap
// the Y coordinates to deal with that oddity ;)
// The map structure passed must be large enough for the map!
// The extra at the right side (partial 11th column) is ignored
// Indexes count across, then down. Note that width and height can be negative, this means
// that the sprite is flipped on that axis!
void load_map(char *szName, TXRMAP *pTxrMap) {
	FSFile fp;
	int nIdx, nBufPos, nBufNow, nBufMax;
	int x,y,w,h,offx,offy;
	
	debug("Loading map %s\n", szName);
		
	FS_InitFile(&fp);

	if (FALSE == FS_OpenFile(&fp, szName)) {
		debug("%s not found\n", szName);
		return;
	}
	
	// read header
	nBufMax=FS_ReadFile(&fp, mapbuf, sizeof(mapbuf));
	FS_CloseFile(&fp);
	
	nBufPos=0;
	while (nBufPos < nBufMax) {
		sscanf(&mapbuf[nBufPos], "./tmp\\out-%d.png|%d|%d|%d|%d|%*d|%*d|%d|%d\n%n",
			&nIdx, &x, &y, &w, &h, &offx, &offy, &nBufNow	);
//		debug("Read %2d:%d,%d|%d,%d|%d,%d\n", nIdx, x, y, w, h, offx, offy);
		pTxrMap[nIdx].x=x;
		pTxrMap[nIdx].y=255-y;
		pTxrMap[nIdx].w=w;
		pTxrMap[nIdx].h=h;
		pTxrMap[nIdx].offx=offx;
		pTxrMap[nIdx].offy=offy;
		nBufPos+=nBufNow;
	}
}

// almost the same function, but for the level maps which are larger
// because we're condensing a 3D level map into a 1d value, we need to
// read it a little different. Each page represented 25 images, 
// and each stage had 12 pages (a-c, 1-4). So the breakdown is:
// class  (0-2)*100
// page   (0-3)*25
// row    (0-4)
// column (0-4)*5
void load_level_map(char *szName, TXRMAP *pTxrMap) {
	FSFile fp;
	int nIdx, nBufPos, nBufNow, nBufMax;
	int npage;
	int x,y,w,h,offx,offy;
	char nclass;
	
	debug("Loading map %s\n", szName);
		
	FS_InitFile(&fp);

	if (FALSE == FS_OpenFile(&fp, szName)) {
		debug("%s not found\n", szName);
		return;
	}
 	
	// read header
	nBufMax=FS_ReadFile(&fp, mapbuf, sizeof(mapbuf));
	FS_CloseFile(&fp);
	
	nBufPos=0;
	while (nBufPos < nBufMax) {
		sscanf(&mapbuf[nBufPos], "./tmp\\%c%d-%d.png|%d|%d|%d|%d|%*d|%*d|%d|%d\n%n",
			&nclass, &npage, &nIdx, &x, &y, &w, &h, &offx, &offy, &nBufNow	);
//		debug("Read %c%d-%d:%d,%d|%d,%d|%d,%d\n", nclass, npage, nIdx, x, y, w, h, offx, offy);
 
		// Now add the page offsets
		nIdx+=(nclass-'a')*100+(npage-1)*25;
//		debug("Filling in %d\n", nIdx);
		pTxrMap[nIdx].x=x;
		pTxrMap[nIdx].y=511-y;
		pTxrMap[nIdx].w=w;
		pTxrMap[nIdx].h=h;
		pTxrMap[nIdx].offx=offx;
		pTxrMap[nIdx].offy=offy;
		nBufPos+=nBufNow;
	}
}

// draws a 128x128x4 texture at x,y
// - darken is 0 for black and 31 for full bright
// color 0 is transparent
void DrawTexture128(pvr_ptr_t pImg, pvr_ptr_t pPal, int x, int y, int nDarken) {
	fx16 x1,y1,x2,y2;

    myG3_Color(nDarken,nDarken,nDarken);

	G3_TexImageParam(GX_TEXFMT_PLTT16, GX_TEXGEN_TEXCOORD, GX_TEXSIZE_S128, GX_TEXSIZE_T128,
		GX_TEXREPEAT_NONE, GX_TEXFLIP_NONE, GX_TEXPLTTCOLOR0_TRNS, pImg);

	G3_TexPlttBase(pPal, GX_TEXFMT_PLTT16);

    G3_PolygonAttr(GX_LIGHTMASK_NONE,  		// no lights
                   GX_POLYGONMODE_MODULATE, // modulate for the fade
                   GX_CULL_NONE,       		// cull none
                   POLY_HERD,          		// polygon ID(0 - 63)
                   31,                 		// alpha(0 - 31 0-wireframe,31=opaque)
                   0                   		// OR of GXPolygonAttrMisc's value
        );

    // calculate and render the poly onscreen
    x1=PixXToScrn(x);
    y1=PixYToScrn(y);
    x2=PixXToScrn(x+127);
    y2=PixYToScrn(y+127);

	G3_Begin(GX_BEGIN_QUADS);
		// counter-clockwise for quads (versus the old triangle list order)
		G3_TexCoord(0,0);
		G3_Vtx(x1, y2, FX16_ONE-1);

		G3_TexCoord(0,127*FX32_ONE);
		G3_Vtx(x1, y1, FX16_ONE-1);

		G3_TexCoord(127*FX32_ONE, 127*FX32_ONE);
		G3_Vtx(x2, y1, FX16_ONE-1);

		G3_TexCoord(127*FX32_ONE, 0);
		G3_Vtx(x2, y2, FX16_ONE-1);
	G3_End();

	// don't let anything else carry our changed shade	
    myG3_Color(31,31,31);
}

