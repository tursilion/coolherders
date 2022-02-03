/****************************************/
/* Cool Herders                         */
/* http://harmlesslion.com              */
/* Copyright 2009 HarmlessLion LLC      */
/* sprite.h                             */
/****************************************/

// texture sizes, don't use these directly
#define SIZE_256x256	0x00
#define SIZE_512x512	0x80

// texture size and depth settings, use these for the SPRITE structure
#define	DEPTH_512x512x8	(SIZE_512x512 | GX_TEXFMT_PLTT256)
#define	DEPTH_256x256x4	(SIZE_256x256 | GX_TEXFMT_PLTT16)
#define	DEPTH_256x256x8	(SIZE_256x256 | GX_TEXFMT_PLTT256)

typedef struct SPRITE {
	pvr_ptr_t txr_addr;		// address of the texture information
	pvr_ptr_t pal_addr;		// address of the texture palette
	int		tilenumber;		// tile index in the texture page - numbered horizontally (10 per row)
	int		x, y;			// x and y position on the 640x480 grid
	int		xd, yd;			// user-defined, direction indicators
	int		alpha;			// alpha value from 0-31 (NDS values)
	int		is3D;			// true or false for the 3d trick, rotation value for the rotateSprite function
	fx16	z;				// z depth of the sprite (fx16 values)
	int 	nDepth; 		// bitdepth - use DEPTH_000x000x0 defines above
} SPRITE;

typedef struct TXRMAP {		// just track a pointer to this for each texture page
	int x, y, w, h;			// use load_map() to fill it in
	int offx, offy;
} TXRMAP;

typedef unsigned int UINT32;
typedef unsigned short UINT16;

extern int nOffsetX, nOffsetY;	// these are the screen values

#define TILESIZE 48
#define GRIDSIZE 32
#define SPR_HFLIP 0x8000		// OR into the tilenumber for horiztonal flip
#define SPR_VFLIP 0x4000		// OR into the tilenumber for vertical flip
#define DOWN_STAND_SPRITE 41

// More than sheep now - tile numbers for first of each
#define SHEEPUP 3
#define SHEEPDOWN 1
#define SHEEPLEFT 0
#define SHEEPRIGHT 2

#define DEFAULT_COLOR 0xffffffff

extern pvr_ptr_t txr512_misc; 

// Polygon IDs for blending (only different numbers alpha blend)
#define POLY_BG		0
#define POLY_HERD	1
#define POLY_SHEEP	2
#define POLY_MISC	3
#define POLY_SPECIAL 4

// Some coordinate conversion code #defined so it's inline
// alternate math - each X pixel is 12.8 units, each Y pixel is 17.066 pixels
// 640x480 to 2FX_ONEx2FX_ONE (center at 0,0), and each Z unit is 4 units
// Floating point is software, so eliminate it where we can
#if 0
	// fullscreen version
	#define ConvX(x) ((fx16)(x*12.8f)-FX16_ONE)
	#define ConvY(y) ((fx16)((479-y)*17.066f)-FX16_ONE)
#else
	// windowed version - pixels are sized differently to get 1:1, offscreen returns quicker
	// ranges are based on the 48x48 tiles, so there is some overlap
	inline fx32 ConvX(int x)
	{
		// multiply by 32 to give us a useful FX32 value
		return (fx32)((x-nOffsetX)<<5)-FX32_ONE;
	}

	inline fx32 ConvY(int y) 
	{
	//	return (fx32)((191-(y-off))*42.6666f)-FX32_ONE;
		// the real ratio is 42.66666... - but 42 seems to work?
	//	return (fx32)((191-(y-off))*42)-FX32_ONE;
		int t=(191-(y-nOffsetY));
		// 42.5? - need to check whether 4 shifts are slower than 1 multiply?
		return (fx32)(t<<5)+(fx32)(t<<3)+(fx32)(t<<1)+(fx32)(t>>1)-FX32_ONE;
	}

#endif

// these convert 256x192 pixels to -FX16_ONE - +FX16_ONE
#define PixXToScrn(x) ((((float)(x)/128.0)-1.0) * FX16_ONE)
#define PixYToScrn(y) ((((float)(191-(y))/96.0)-1.0) * FX16_ONE)

void myG3_Color(int r, int g, int b);
void Set3DDarken(int val);

void pvr_scene_begin(); 
void load_map(char *szName, TXRMAP *pTxrMap);
void load_level_map(char *szName, TXRMAP *pTxrMap);
void SetOffset(int x, int y);
void SortSprite(SPRITE *spr, TXRMAP *pMap, int PolyNo);
void SortSpriteRotated(SPRITE *spr, TXRMAP *map, int PolyNo);
void SortSpriteSquashed(SPRITE *spr, TXRMAP *map, int PolyNo, int xPercent, int yPercent);
void scaleimage(pvr_ptr_t txr, pvr_ptr_t pal, int ix1, int iy1, int ix2, int iy2, int alpha);
void SortMenuPictureX(pvr_ptr_t pImg, pvr_ptr_t pPal, int nDarken, int xoff, int yoff);
void SortFullPictureX(GXTexFmt fmt, pvr_ptr_t pImg, pvr_ptr_t pPal, int nDarken);
void addPageStory(pvr_ptr_t txr, fx16 z, int nAlpha);
void DrawTexture128(pvr_ptr_t pImg, pvr_ptr_t pPal, int x, int y, int nDarken);




