/****************************************/
/* Cool Herders                         */
/* A demented muse by Brendan Wiese     */
/* http://starbow.org                   */
/* Programmed by Mike Brent             */
/* http://harmlesslion.com              */
/* Copyright 2000-2005 Brendan Wiese &  */
/* Mike Brent.                          */
/* sprite.h                             */
/****************************************/

#ifdef WIN32
#ifndef inline
#define inline 
#endif
#endif

#ifndef WIN32
#define DRAW_TEXT(txrtmp, x, y, pstr) bfont_draw_str(((uint16*)(txrtmp)) + (y)*512+(x), 512, 0, (pstr))
#define ALLOC_IMAGE(x, y) pvr_mem_malloc((x)*(y)*2)
#define CLEAR_IMAGE(p, x, y) memset((p), 0, (x)*(y)*2)
#endif

typedef struct SPRITE {
	pvr_ptr_t txr_addr;
	int		tilenumber;
	int		x, y;
	int		xd, yd;
	int		alpha;
	int		is3D;
	float	z;
} SPRITE;

typedef unsigned int UINT32;
typedef unsigned short UINT16;

#define TILESIZE 48
#define GRIDSIZE 32
#define CH_PAGESIZE 256.0f
#define SPR_HFLIP 0x80
#define SPR_VFLIP 0x40
#define PIC_HFLIP 0x8000		// Not used everywhere.
#define PAGE512 (512*512*2)
#define PAGE256 (256*256*2)

#define DEFAULT_COLOR 0xffffffff

extern int gGfxDarken;

/* rather than converting floats on the fly everywhere */
#define INT_PACK_COLOR(a,r,g,b) ( \
	((a) << 24 ) | \
	((r) << 16 ) | \
	((g) << 8 ) | \
	((b) << 0 ) )

#define INT_PACK_ALPHA(a) ( ((a) << 24) | 0x00ffffff )

void SortHeader(int NewList, int NewTxrFormat, int nWidth, int nHeight, pvr_ptr_t NewTexurePage);
void SortHeaderStretch(int NewList, int NewTxrFormat, int nWidth, int nHeight, pvr_ptr_t NewTexurePage);

void SortSprite(SPRITE *spr);
void SortSpriteTinted(SPRITE *spr, int r, int g, int b);
void SortScaledSprite(SPRITE *spr, float xScale, float yScale);
void addPage(pvr_ptr_t txr, int x1, int y1, int x2, int y2);
void addPage2(pvr_ptr_t txr, int x, int y, int x1, int y1, int x2, int y2, uint32 color, float z);
void addPageAlpha(pvr_ptr_t txr, int x, int y, int x1, int y1, int x2, int y2, uint32 color, float z);
void addPageAlphaStretch(pvr_ptr_t txr, int x1, int y1, int x2, int y2, int tx1, int ty1, int tx2, int ty2, uint32 color, float z);
void addPageLarge(pvr_ptr_t txr, int x, int y, int tx1, int ty1, int tx2, int ty2, uint32 color, float z);
void addPageStory(pvr_ptr_t txr, float z);
void stretchPage2(pvr_ptr_t txr, int x1, int y1, int x2, int y2, int tx1, int ty1, int tx2, int ty2, float z, uint32 color);
void stretchLarge2(pvr_ptr_t txr, int x1, int y1, int x2, int y2, int tx1, int ty1, int tx2, int ty2, float z, uint32 color);
void stretchLarge3(pvr_ptr_t txr, int x1, int y1, int x2, int y2, int tx1, int ty1, int tx2, int ty2, uint32 color);
void addNonTwiddledPage(pvr_ptr_t txr, int x, int y, int tx1, int ty1, int tx2, int ty2, uint32 color);
void SortRect(float z, int x1, int y1, int x2, int y2, uint32 color1, uint32 color2);
void SortRectHoriz(float z, int x1, int y1, int x2, int y2, uint32 color1, uint32 color2);
void SortRollingRect(float z, uint32 color1, uint32 color2);
void SortLight(float z, int x1, int y1, int x2, int y2, uint32 color);
void DrawSpriteTxr(pvr_ptr_t txr_out, SPRITE *spr);
void SortFullPictureX(pvr_ptr_t t512, pvr_ptr_t t256, float z, int x);
void BeginScene(); 
void checkSnapshot();

#define SortFullPicture(t512, t256, z) SortFullPictureX(t512, t256, z, 0)
#define pvr_scene_end pvr_scene_finish(); nFrames++; checkSnapshot();

