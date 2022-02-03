/****************************************/
/* Cool Herders                         */
/* A demented muse by Brendan Wiese     */
/* http://starbow.org                   */
/* Programmed by Mike Brent             */
/* http://harmlesslion.com              */
/* Copyright 2000-2005 Brendan Wiese &  */
/* Mike Brent.                          */
/* kosemulation.c                       */
/****************************************/
/* This is a simple wrapper for Windows */
/* around some of the KOS functions I   */
/* use, for easier testing/dev          */
/****************************************/

#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

#pragma warning (disable: 4244)		// data type conversion, possible loss of data
#pragma warning (disable: 4018)		// signed/unsigned compare
#pragma warning (disable: 4101)		// unreferenced local variable

/* types */
typedef __int32				int32;
typedef unsigned __int32	uint32;
typedef unsigned short		uint16;
typedef unsigned char		uint8;

typedef HANDLE				semaphore_t;
typedef HANDLE				kthread_t;

typedef struct maple_devinfo {
	uint32		functions;
} maple_devinfo_t;

typedef struct maple_device {
	int					valid;				/* Is this a valid device? */
	int					port, unit;			/* Maple address */
	maple_devinfo_t		info;				/* Device info struct */
} maple_device_t;

typedef struct pvr_state {
	DWORD list_reg_open;
	int   render_busy;
} pvr_state_t;

typedef struct {
	/* Bin sizes: opaque polygons, opaque modifiers, translucent
	   polygons, translucent modifiers, punch-thrus */
	int		opb_sizes[5];

	/* Vertex buffer size (should be a nice round number) */
	int		vertex_buf_size;

	/* Non-zero if we want to enable vertex DMA mode. Note that
	   if this is set, then _all_ enabled lists need to have a
	   vertex buffer assigned. */
	int		dma_enabled;
} pvr_init_params_t;

#define PVR_BINSIZE_0			0
#define PVR_BINSIZE_8			8
#define PVR_BINSIZE_16			16
#define PVR_BINSIZE_32			32

#define MAPLE_FUNC_CONTROLLER	0x01000000
#define MAPLE_FUNC_MEMCARD		0x02000000
#define MAPLE_FUNC_LCD			0x04000000

typedef char*				sfxhnd_t;	
typedef HANDLE				pvr_ptr_t;	

#ifndef NULL
#define NULL ((void*)0)
#endif

typedef struct {
	uint32		cmd;			/* TA command */
	uint32		mode1, mode2, mode3;	/* mode parameters */
} pvr_poly_hdr_t;

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
		int		filter;		/* none, bi-linear, tri-linear, etc */
		int		mipmap;
		int		mipmap_bias;
		int		uv_flip;
		int		uv_clamp;
		int		alpha;
		int		env;
		int		width;
		int		height;
		int		format;		/* bit format, vq, twiddle, stride */
		pvr_ptr_t	base;		/* texture location */
	} txr;
} pvr_poly_cxt_t;

typedef struct kos_img {
	void	* data;
	uint32	w, h;
	uint32	fmt;
	uint32	byte_count;
} kos_img_t;

// takes a char * to initialize
#define KOS_INIT_ROMDISK(romdisk)
// Takes a set of flags
#define KOS_INIT_FLAGS(flags)

#define MAPLE_PORT_COUNT	4
#define MAPLE_UNIT_COUNT	6

// Helper - top of an emuneration loop of all controllers
#define MAPLE_FOREACH_BEGIN(TYPE, VARTYPE, VAR) \
do { \
	maple_device_t	* __dev; \
	VARTYPE * VAR; \
	int	__i; \
\
	__i = 0; \
	while ( __i < MAPLE_PORT_COUNT ) { \
		__dev = maple_enum_dev(__i, 0); \
		if ((__dev->valid) && (__dev->info.functions & TYPE)) {	\
			VAR = maple_dev_status(__dev);	\
			do {
		
#define MAPLE_FOREACH_END() \
			} while(0); \
		}	\
		__i++; \
	} \
} while(0);

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

// Controller bitflags used
#define CONT_C				(1<<0)
#define CONT_B				(1<<1)
#define CONT_A				(1<<2)
#define CONT_START			(1<<3)
#define CONT_DPAD_UP		(1<<4)
#define CONT_DPAD_DOWN		(1<<5)
#define CONT_DPAD_LEFT		(1<<6)
#define CONT_DPAD_RIGHT		(1<<7)
#define CONT_Z				(1<<8)
#define CONT_Y				(1<<9)
#define CONT_X				(1<<10)
#define CONT_D				(1<<11)
#define CONT_DPAD2_UP		(1<<12)
#define CONT_DPAD2_DOWN		(1<<13)
#define CONT_DPAD2_LEFT		(1<<14)
#define CONT_DPAD2_RIGHT	(1<<15)

typedef struct {
	uint32	buttons;		/* Buttons bitfield */
	int	ltrig, rtrig;		/* Triggers */
	int	joyx, joyy;			/* Joystick X/Y */
	int	joy2x, joy2y;		/* Second Joystick X/Y (if applicable) */
} cont_state_t;

// PowerVR polygon list types
#define PVR_LIST_OP_POLY		0	/* opaque poly */
#define PVR_LIST_TR_POLY		2	/* translucent poly */
#define PVR_LIST_PT_POLY		4	/* punch-through poly */

// PowerVR texture formats
#define PVR_TXRFMT_ARGB1555		(0 << 27)
#define PVR_TXRFMT_RGB565		(1 << 27)
#define PVR_TXRFMT_ARGB4444		(2 << 27)
#define PVR_TXRFMT_NONTWIDDLED	(1 << 26)

// PowerVR filters
#define PVR_FILTER_NONE			0	/* txr_filter */
#define PVR_FILTER_BILINEAR		2

// PowerVR Flags
#define PVR_CMD_VERTEX		0xe0000000
#define PVR_CMD_VERTEX_EOL	0xf0000000

// Image loader flags
#define PNG_MASK_ALPHA		1
// these values probably aren't right, didn't look it up
#define PNG_FULL_ALPHA		2
#define PNG_NO_ALPHA		4
#define PNG_MAGIC_FONT      254
#define PNG_MAGIC_SHEEP		255		// my own value! loads most in 1555, and the bottom right (>112) as 4444!

#ifndef PVR_PACK_COLOR
/* Small macro for packing float color values */
#define PVR_PACK_COLOR(a, r, g, b) ( \
	( ((uint8)( a * 255 ) ) << 24 ) | \
	( ((uint8)( r * 255 ) ) << 16 ) | \
	( ((uint8)( g * 255 ) ) << 8 ) | \
	( ((uint8)( b * 255 ) ) << 0 ) )
#endif

/* externs */
extern CRITICAL_SECTION csEnd;
extern volatile pvr_state_t pvr_state;


/* Function prototypes */
// Video
void pvr_init_defaults();
void pvr_init(pvr_init_params_t *params);
void pvr_wait_ready();
void pvr_scene_begin();
void pvr_list_begin(DWORD dwType);
void pvr_list_finish();
void pvr_scene_finish();
void pvr_poly_cxt_txr(pvr_poly_cxt_t *pContext, int pvrListType, int pvrTextureFormat, int nWidth, int nHeight, pvr_ptr_t pAddr, DWORD dwFilter);
void pvr_poly_compile(pvr_poly_hdr_t *pHdr, pvr_poly_cxt_t *pContext);
void pvr_prim(void *pHdr, DWORD dwHdrSize);
//void *pvr_mem_malloc(DWORD nSize);
void pvr_mem_free(pvr_ptr_t chunk);
int  png_to_img(char *szFn, DWORD dwFlags, kos_img_t *pImg);
void pvr_txr_load_kimg(kos_img_t *pImg, pvr_ptr_t pWhere, int nUnknown);
void kos_img_free(kos_img_t *pImg, int nUnknown);
void pvr_mem_reset();
void pvr_mem_stats();

void DRAW_TEXT(pvr_ptr_t txrtmp, int x, int y, char *pStr);		// replacement for bfont_draw_str
HANDLE ALLOC_IMAGE(int x, int y);								// replacement for pvr_mem_malloc
void CLEAR_IMAGE(pvr_ptr_t p, int x, int y);					// replacement for memset ;)

// Controllers
void cont_btn_callback(int num, DWORD dwBtns, void (*fctn)(uint8, uint32));
int maple_first_controller();
cont_state_t *maple_dev_status(maple_device_t *pDev);
maple_device_t *maple_enum_dev(int nPort, int nUnit);
maple_device_t *maple_enum_type(int nIndex, DWORD dwType);
void vmu_draw_lcd(maple_device_t *pDev, unsigned char *pDat);

// System
void arch_exit();
void thd_sleep(int nDelay);
int  sem_wait_timed(semaphore_t *pSem, DWORD dwTime);
semaphore_t *sem_create(int nUnknown);
kthread_t *thd_create(void (*vmuThreadproc)(void*), void *pUnk);
void sem_signal(semaphore_t *pSem);
void thd_wait(kthread_t *pThread);
void sem_destroy(semaphore_t *pSem);

// Sound
void sndoggvorbis_init();
void sndoggvorbis_stop();
void sndoggvorbis_start(char *pData, int nRepeat);
void snd_stream_init(void *pNull);
sfxhnd_t snd_sfx_load(char *szName);
void snd_sfx_play(sfxhnd_t pSnd, int nVol, int nPan);
void snd_sfx_stop_all();
void snd_sfx_unload_all();
void sndoggvorbis_shutdown();
int  sndoggvorbis_isplaying();
void snd_stream_volume(int nVol);
#define sndoggvorbis_volume snd_stream_volume

// Filesystem
typedef int file_t;
typedef size_t ssize_t;
void fs_ramdisk_init();
void fs_ramdisk_shutdown();
uint32 fs_total(file_t f);
int fs_unlink(char *sz);
#define fs_open _open
#define fs_read _read
#define fs_write _write
#define fs_close _close
#define O_RDONLY _O_RDONLY
#define O_WRONLY _O_WRONLY
#define FILEHND_INVALID (-1)

// debug
#define printf myoutput
void myoutput(char *szFmt, ...);

#ifdef __cplusplus
}
#endif
