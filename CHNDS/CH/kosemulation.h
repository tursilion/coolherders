/****************************************/
/* Cool Herders                         */
/* http://harmlesslion.com              */
/* Copyright 2009 HarmlessLion LLC      */
/* kosemulation.h                       */
/****************************************/
/* This is a simple wrapper for Windows */
/* around some of the KOS functions I   */
/* use, for easier testing/dev          */
/****************************************/

#include <nitro.h>		// NITRO

// TODO: what's a safe channel for generic DMA?
// GX uses 3, apparently...
#define GENERIC_DMA_CHANNEL 1
#define MAPSCR_DMA_CHANNEL 2
//#define GX_DMA_CHANNEL 3	// default in NITRO
// Filesystem DMA does not appear to speed things up
// So only useful if you need asynchronous streaming
#define FS_DMA_CHANNEL FS_DMA_NOT_USE
  
/* types */
typedef void* HANDLE;
typedef unsigned int DWORD;

typedef int					int32;
typedef unsigned int		uint32;
typedef unsigned short		uint16;
typedef unsigned char		uint8;

typedef HANDLE				semaphore_t;
typedef OSThread			kthread_t;

// Some stuff for  bitmap allocation
#define SIZE_4BIT				0
#define SIZE_8BIT				1
#define SIZE_4BITx3				2

typedef u32		pvr_ptr_t;
#define INVALID_PTR 0xFFFFFFFF

#ifndef NULL
#define NULL ((void*)0)
#endif

typedef struct {
	uint32		flags;			/* vertex flags */
	float		x, y, z;		/* the coodinates */
	float		u, v;			/* texture coords */
	uint32		argb;			/* vertex color */
	uint32		oargb;			/* offset color */
} pvr_vertex_t;

typedef struct {
	int		list_type;
	struct {
		int		alpha;
		int		shading;
		int		fog_type;
		int		culling;
		int		color_clamp;
		int		clip_mode;
		int		modifier_mode;
	} gen;
	struct {
		int		src, dst;
		int		src_enable, dst_enable;
	} blend;
	struct {
		int		color;
		int		uv;
		int		modifier;
	} fmt;
	struct {
		int		comparison;
		int		write;
	} depth;
	struct {
		int		enable;
		int		filter;			/* none, bi-linear, tri-linear, etc */
		int		mipmap;
		int		mipmap_bias;
		int		uv_flip;
		int		uv_clamp;
		int		alpha;
		int		env;
		int		width;
		int		height;
		int		format;			/* bit format, vq, twiddle, stride */
		pvr_ptr_t	base;		/* texture location */
	} txr;
} pvr_poly_cxt_t;

typedef struct kos_img {
	void	*data;
	void    *pal;
	uint32	w, h;
	uint32	fmt;			// number of colors in palette
	uint32	byte_count;
} kos_img_t;

/* defines */
#ifndef true
	#define true TRUE
#endif
#ifndef false
	#define false FALSE
#endif

// INIT_xxxx bitflags used
#define INIT_DEFAULT		1
#define INIT_MALLOCSTATS	2

// Controller bitflags used (these now match the NDS settings to improve performance)
#define CONT_B				PAD_BUTTON_B
#define CONT_A				PAD_BUTTON_A
#define CONT_START			PAD_BUTTON_START
#define CONT_DPAD_UP		PAD_KEY_UP
#define CONT_DPAD_DOWN		PAD_KEY_DOWN
#define CONT_DPAD_LEFT		PAD_KEY_LEFT
#define CONT_DPAD_RIGHT		PAD_KEY_RIGHT
#define CONT_Y				PAD_BUTTON_Y
#define CONT_X				PAD_BUTTON_X
// these two don't exist on Dreamcast, but the code used them
// Specifically the code mapped D and Z to left and right
#define CONT_L				PAD_BUTTON_L
#define CONT_R				PAD_BUTTON_R
#define CONT_C				PAD_BUTTON_SELECT

// PowerVR polygon list types
#define PVR_LIST_OP_POLY		0	/* opaque poly */
#define PVR_LIST_TR_POLY		2	/* translucent poly */
#define PVR_LIST_PT_POLY		4	/* punch-through poly */

// PowerVR texture formats
#define PVR_TXRFMT_ARGB1555		GX_TEXFMT_NONE
#define PVR_TXRFMT_RGB565		GX_TEXFMT_NONE
#define PVR_TXRFMT_ARGB4444		GX_TEXFMT_NONE
// now some formats for Cool Herders ;)
#define PVR_TXRFMT_PAL256		GX_TEXFMT_PLTT256
#define PVR_TXRFMT_PAL16		GX_TEXFMT_PLTT16

// PowerVR filters
#define PVR_FILTER_NONE			0	/* txr_filter */
#define PVR_FILTER_BILINEAR		2

// PowerVR Flags
#define PVR_CMD_VERTEX		0xe0000000
#define PVR_CMD_VERTEX_EOL	0xf0000000

#ifndef PVR_PACK_COLOR
/* Small macro for packing float color values */
#define PVR_PACK_COLOR(a, r, g, b) ( \
	( ((uint8)( a * 255 ) ) << 24 ) | \
	( ((uint8)( r * 255 ) ) << 16 ) | \
	( ((uint8)( g * 255 ) ) << 8 ) | \
	( ((uint8)( b * 255 ) ) << 0 ) )
#endif

// for the safeish functions, higher number is slower but less data per blank
// Must be a divisor of 49152 (number of bytes being copied)
// Flickers slightly in debug build, but must not flicker in ROM build. If
// you notice it, let me know! :) It looks like Capture mode will take too much VRAM
#define SAFE_COPY_BYTES_PER_VBLANK	12288

/* Custom */
void SetScore(u32 x);
void SetNumSheep(u32 x);
void SetTimer(u32 x);
void SetLightning(u8 x);
void SetSpecial(u8 x);
void SetSnapFlag(void);

/* Function prototypes */
// Video
void pvr_init_defaults();
void pvr_scene_begin();
void pvr_scene_finish();
void pvr_scene_finish_2d();
void pvr_poly_cxt_txr(pvr_poly_cxt_t *pContext, DWORD pvrListType, int pvrTextureFormat, int nWidth, int nHeight, pvr_ptr_t pAddr, DWORD dwFilter);
void pvr_prim(void *pHdr, DWORD dwHdrSize);
int  bmp_to_img(char *szFn, char *szPf, kos_img_t *pImg);
int bmp_to_pixels(FSFile *fp, kos_img_t *pImg);
int bmp_to_pal(FSFile *fp, kos_img_t *pImg);
void pvr_txr_load_kimg(kos_img_t *pImg, pvr_ptr_t pWhere, pvr_ptr_t pPalette, int herd);
void kos_img_free(kos_img_t *pImg);
void pvr_mem_reset();
void pvr_mem_stats();

pvr_ptr_t ALLOC_IMAGE(int x, int y, int shift4bit);				// replacement for pvr_mem_malloc
pvr_ptr_t ALLOC_PAL(int nsize);
void CLEAR_IMAGE(pvr_ptr_t p, int x, int y, int depth);			// replacement for memset ;)

int ReadNitroPalette(char *pszFile, void *pWhere);
int ReadNitroChar(char *pszFile, void *pWhere, char *pTable);

// System
void arch_exit();
void thd_sleep(int nDelay);

