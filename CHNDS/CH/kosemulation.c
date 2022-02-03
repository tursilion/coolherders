/****************************************/
/* Cool Herders                         */
/* http://harmlesslion.com              */
/* Copyright 2009 HarmlessLion LLC      */
/* kosemulation.c                       */
/****************************************/
/* This is a simple wrapper for Windows */
/* around some of the KOS functions I   */
/* use, for easier testing/dev          */
/****************************************/

// TODO: Optimization - go through once we're working and convert all the floats to
// a proper fixed-point system - helps memory and performance.

// BIG NOTE: Most of the comments in this file refer to the Windows version,
// not the DS version! Keep in mind that comments may be wrong!

// NOTE: These functions do not necessarily duplicate the functions of the
// original KOS functions. In most cases, they only reproduce the behaviour
// that I need for Cool Herders to run! (And not necessarily 100% correctly)

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <fcntl.h>
#include <string.h>

#include "kosemulation.h"
#include "levels.h"		// necessary for mapscr_manager.h
#include "mapscr_manager.h"
#include "menuscr_manager.h"
#include "sound.h"
#include "sprite.h"
#include "font.h"
#include "control.h"
#include "chwireless.h"

// CARD library
#include <nitro/fs.h>

// cool.h
extern void debug(char *str, ...);
extern void ShowBlack();
extern int  PreAllocPVR();
extern int nFrames;
extern pvr_ptr_t txr_herder[];
extern pvr_ptr_t pal_herder[];
extern int gDontCheckLid;
extern int  gHumanPlayer;
extern int isMultiplayer;		// 0 = no, story mode. 1 = yes, solo. 2 = yes, wireless

#ifndef min
	#define min(a,b) ((a)<(b)?(a):(b))
#endif

// sound.h
extern void sound_init();

// disclaimer.c
extern void LoadFixedGfx2DA();

static volatile int myOldJiffies=0;

#define   MAIN_HEAP_SIZE    (384*1024)
OSHeapHandle handle;

// theses variables are used for the subscreen display
u32 uPlayerScore=0, 				// score for our player
	uNumSheep=0,					// number of sheep for our player
	uTimer=0;						// time left
u8	uThermo1,						// in pixels, lightning (0-160)
	uThermo2;						// in pixels, special (0-160)
u8  nSnapFlag=0;					// Should we snap the next frame

int nFrameSpeed=2;					// for 30fps, want to fix this rate now

int nLast3DCount = 0;				// number of 3D objects sorted in the last begin..end sequence

// some mutators for those variables
void SetScore(u32 x) {
	uPlayerScore=x;
}
void SetNumSheep(u32 x){
	uNumSheep=x;
}
void SetTimer(u32 x){
	uTimer=x;
}
void SetLightning(u8 x){
	uThermo1=x;
}
void SetSpecial(u8 x){
	uThermo2=x;
}
// Sets a flag to indicate that the next drawn frame should be snapped (initial position setting)
void SetSnapFlag(void) 
{
	nSnapFlag = 1;
}

// VRAM allocation (KOS style simple)
#define MAX_VRAM_SIZE 512*1024
u32 nCurrentVRAMPtr = 0;
#define MAX_PAL_SIZE 16*1024
u32 nCurrentPALPtr = 0;
int nVRAMAllocFailures = 0;

extern int myJiffies;

unsigned int WrapThreadStart(void *p);
void VBlankIntr(void);

//
// Video
//

//---------------------------------------------------------------------------
// V-Blank interrupt function:
//
// Interrupt handlers are registered on the interrupt table by OS_SetIRQFunction.
// OS_EnableIrqMask selects IRQ interrupts to enable, and
// OS_EnableIrq enables IRQ interrupts.
// Notice that you have to call 'OS_SetIrqCheckFlag' to check a V-Blank interrupt.
//---------------------------------------------------------------------------
void VBlankIntr(void)
{
    OS_SetIrqCheckFlag(OS_IE_V_BLANK); 		// checking V-Blank interrupt
    myJiffies++;
}

// Prepares the system 
void pvr_init_defaults() {
    void   *heapStart;
    void   *nstart;
    int idx;
    
    OS_Init();		// prepare the DS system
    FX_Init();		// prepare the fixed point APIs
    OS_InitTick();		// give the system a tick timer
    OS_InitAlarm();		// needed for OS_Sleep()
    SND_Init();			// prepare the sound engine

	// prepare the memory heap, since we do alloc a bit (from NITRO sample)
    //---- Initialize memory allocation system for MainRAM arena
    nstart = OS_InitAlloc(OS_ARENA_MAIN, OS_GetMainArenaLo(), OS_GetMainArenaHi(), 2);
    OS_SetMainArenaLo(nstart);

    //---- Allocate region for heap from arena
    heapStart = OS_AllocFromMainArenaLo(MAIN_HEAP_SIZE, 32);
    debug("heapStart %x\n", heapStart);

    //---- Create heap
    handle = OS_CreateHeap(OS_ARENA_MAIN, heapStart, (void *)((u32)heapStart + MAIN_HEAP_SIZE));
    debug("heap handle %d\n", handle);

    //---- Set current heap
    (void)OS_SetCurrentHeap(OS_ARENA_MAIN, handle);

	// prepare the FS library
	FS_Init(FS_DMA_CHANNEL);	// we'll see if we need DMA later
	debug("ROM Table size (for improving access speed) is %d bytes\n", FS_GetTableSize());
	// Basically, we can get a buffer that big, and call FS_LoadTable() to precache the FAT tables

    // All VRAM banks to LCDC
    GX_SetBankForLCDC(GX_VRAM_LCDC_ALL);
    // Clear all LCDC space
    MI_CpuClearFast((void *)HW_LCDC_VRAM, HW_LCDC_VRAM_SIZE);
    // Disable the banks on LCDC
    (void)GX_DisableBankForLCDC();

    MI_CpuFillFast((void *)HW_OAM, 192, HW_OAM_SIZE);   // clear OAM
    MI_CpuClearFast((void *)HW_PLTT, HW_PLTT_SIZE);     // clear the standard palette

    MI_CpuFillFast((void *)HW_DB_OAM, 192, HW_DB_OAM_SIZE);     // clear OAM
    MI_CpuClearFast((void *)HW_DB_PLTT, HW_DB_PLTT_SIZE);       // clear the standard palette

	GX_Init();		// initializes the 2D system and turns everything on
	G3X_Init();		// initializes the 3D system
    G3X_InitMtxStack();	// Initialize the matrix stacks and the matrix stack pointers
	G3_SwapBuffers(GX_SORTMODE_AUTO, GX_BUFFERMODE_Z);	// Swap once to kickstart something ;)
	// Seems we need all the graphics on...nothing to power off!

	// Set up graphics modes - 0-3d, 1-text, 2-text, 3-text
	GX_SetGraphicsMode(GX_DISPMODE_GRAPHICS, GX_BGMODE_0, GX_BG0_AS_3D);
	// hide the setup
    GX_DispOff();
    
    // Allocate my 2d memory
    GX_SetBankForBG(GX_VRAM_BG_80_EF);			// 80k for tiles

   	// setup the 2d screen
   	// 16 color mode, screen at 0x0000 (32x32x2bytes each = 2k)
   	// Patterns at 0x4000 (64k for pats, 64 bytes each, 1024 chars, but only 512
   	// are possible to reference, as each takes two indexes)
   	G2_SetBG1Control(GX_BG_SCRSIZE_TEXT_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE_0x0000, GX_BG_CHARBASE_0x04000, GX_BG_EXTPLTT_01);
   	G2_SetBG1Priority(0);	// 0 is highest, 3 is lowest
   	G2_BG1Mosaic(false);
	// BG2 gives shadowed text (same screen location, same font!)
   	G2_SetBG2ControlText(GX_BG_SCRSIZE_TEXT_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE_0x0000, GX_BG_CHARBASE_0x04000);
   	G2_SetBG2Priority(1);	// 0 is highest, 3 is lowest
   	G2_BG2Mosaic(false);

   	// set up blending window, used for a dark background behind text
	G2_SetWnd0InsidePlane(GX_WND_PLANEMASK_BG0 | GX_WND_PLANEMASK_BG1 |
		GX_WND_PLANEMASK_BG2 | GX_WND_PLANEMASK_BG3 | GX_WND_PLANEMASK_OBJ, TRUE);
	G2_SetWndOutsidePlane(GX_WND_PLANEMASK_BG0 | GX_WND_PLANEMASK_BG1 |
		GX_WND_PLANEMASK_BG2 | GX_WND_PLANEMASK_BG3 | GX_WND_PLANEMASK_OBJ, FALSE);
	
	G2_SetWnd0Position(1, 1, 256, 16);		// is 0 offscreen? 255 is not the right edge, 256 is, but the docs says it's an 8-bit register
	GX_SetVisibleWnd(GX_WNDMASK_NONE);		// no visible window makes the whole screen "inside" the window
  	
   	// need to dim the font slightly so it shows on the white of story mode
   	G2_SetBlendBrightness(GX_BLEND_PLANEMASK_BG2, -9);		// can only have one alpha/brightness effect at a time

	// now load the static 2D graphics
	LoadFixedGfx2DA();
   	// clear 2D memory so there's no garbage onscreen
   	Clear2D();

    // Allocate my 3d memory (note we assume this in a few places like clearimage and story mode)
    GX_SetBankForTex(GX_VRAM_TEX_0123_ABCD);	// banks A,B,C,D (512k) - Subscreen uses H and I
    GX_SetBankForTexPltt(GX_VRAM_TEXPLTT_0_G);	// 16k for texture palettes

    // Set priority of BG0 plane (bottom)
    G2_SetBG0Priority(3);
	// shading is not used, though
    G3X_SetShading(GX_SHADING_TOON);
    // No alpha tests
    G3X_AlphaTest(FALSE, 0);
    // Allow alpha blending (no speed hit)
    G3X_AlphaBlend(TRUE);
	// enable anti-aliasing (no speed hit)
	G3X_AntiAlias(TRUE);
	// disable edge marking
	G3X_EdgeMarking(FALSE);
    // No fog
	G3X_SetFog(FALSE, (GXFogBlend)0, (GXFogSlope)0, 0);
    // Set up clear color, depth, and polygon ID.
    G3X_SetClearColor(0, 0, 0x7fff, 63, FALSE);
    // Viewport (full screen)
    G3_ViewPort(0, 0, 255, 191);

    // Set BG0 (3d) and BG1/BG2 (text) visible
    GX_SetVisiblePlane(GX_PLANEMASK_BG0 | GX_PLANEMASK_BG1 | GX_PLANEMASK_BG2);

    // Prepare Keith's Subscreen code (leaves subscreen off for the moment)
    MapScr_Init(0, 0);
	MapScr_ShowBlack();
	nSnapFlag = 0;
	PreAllocPVR();
	
	// prepare controller code
	ControllerState[0] = eContLocal;
	for (idx=1; idx<4; idx++) {
		ControllerState[idx] = eContNone;
	}
	gHumanPlayer = 0;	// always update this if ControllerState[] changes
	
	// setup vblank
	OS_InitIrqTable();
    OS_SetIrqFunction(OS_IE_V_BLANK, VBlankIntr);
    (void)OS_EnableIrqMask(OS_IE_V_BLANK);
    (void)OS_EnableIrq();
    (void)GX_VBlankIntr(TRUE);         // to generate V-Blank interrupt request

    OS_WaitVBlankIntr();               // Waiting the end of V-Blank interrupt

	// Init sound system - do not call OS_SetIrqMask after this call
	sound_init();

    // Start display
    GX_DispOn();
	GXS_DispOn();										// now turn it on
}

// Start to describe a scene
void pvr_scene_begin() {
	// Reset the states of 3D graphics
	G3X_Reset();
    
    // In this sample, the camera matrix and the projection matrix are set as
	// an identity matrices for simplicity.
	G3_MtxMode(GX_MTXMODE_TEXTURE);
	G3_Identity();
	G3_MtxMode(GX_MTXMODE_POSITION_VECTOR);
	G3_Identity();
	
	nLast3DCount = 0;
}

// Finish and draw the scene
void pvr_scene_finish() {
	int nSync = 0;
	nFrames++;

	// swapping the polygon list RAM, the vertex RAM, etc.
	G3_SwapBuffers(GX_SORTMODE_AUTO, GX_BUFFERMODE_Z);
	
	// draw the map subscreen if active
	if (MapScr_isMapActive()) {
		// draw the subscreen with our current values if we're not in the menu
		MapScr_DrawFrame(uPlayerScore, uNumSheep, uTimer, uThermo1, uThermo2, nSnapFlag);	
		nSnapFlag = 0;
	}
	 
	// frame throttle
	{
		
		if ((myOldJiffies != 0) && (myOldJiffies <= myJiffies)) {
			// network game, so don't add more latency in a network game, we're covered by
			// the network sync handler
			if (isMultiplayer!=2) {
				while (myOldJiffies+nFrameSpeed > myJiffies) {
					// wait for vblank
					OS_WaitVBlankIntr();
					SyncNetwork();
					nSync = 1;
				}
			}
		}
		// I still need this, though it worries me a bit
		if (!nSync) {
			if (!SyncNetwork()) {
				OS_WaitVBlankIntr();
			}
		}
				
		myOldJiffies=myJiffies;
	}

	// do audio processing
	sound_frame_update();

	// This is a nice central place that is always being called.  You have to poll for cover close.
	// If you want to do things before/after sleep, rather than putting them here, put them in the
	// AppendPostSleepCallback and its related PM functions.
	// We make sure not to do this if the eeprom is saving
    if ((PAD_DetectFold() == TRUE) && (!gDontCheckLid))
    {
    	debug("Going to sleep....");
    	EndWireless();
        PM_GoSleepMode(PM_TRIGGER_COVER_OPEN | PM_TRIGGER_CARD, 0, 0);
    }
}

// this is a 3d-safe version of the above code, so it leaves the poly engine alone
// it also preserves the subscreen (used in story mode so assumes menu is up). Also
// increments nFrames while it's at it.
void pvr_scene_finish_2d() {
	int nSync = 0;
	
	nFrames++;
	// frame throttle
	{
		
		if ((myOldJiffies != 0) && (myOldJiffies <= myJiffies)) {
			// network game, so don't add more latency in a network game, we're covered by
			// the network sync handler
			if (isMultiplayer!=2) {
				while (myOldJiffies+nFrameSpeed > myJiffies) {
					// wait for vblank
					OS_WaitVBlankIntr();
					SyncNetwork();
					nSync=1;
				}
			}
		}
		if (!nSync) {
			if (!SyncNetwork()) {
				OS_WaitVBlankIntr();
			}
		}
		myOldJiffies=myJiffies;
	}

	// do audio processing
	sound_frame_update();

	// This is a nice central place that is always being called.  You have to poll for cover close.
	// If you want to do things before/after sleep, rather than putting them here, put them in the
	// AppendPostSleepCallback and it's related PM functions.
	// We make sure not to do this if the eeprom is saving
    if ((PAD_DetectFold() == TRUE) && (!gDontCheckLid))
    {
    	debug("Going to sleep (2d)....");
    	EndWireless();
        PM_GoSleepMode(PM_TRIGGER_COVER_OPEN | PM_TRIGGER_CARD, 0, 0);
    }

}

// Submit a structure of the *current* list type to the hardware
// CH sorts rectangles like this:
//    2 4
//    1 3
void pvr_prim(void *pHdr, DWORD dwHdrSize) {
	static BOOL bFirstVertex=TRUE;
	static DWORD lastargb=0xffffffff;
	fx16 x,y;
	float z;
	pvr_vertex_t *pV;
	
	nLast3DCount++;
	
	// We are currently only interested in vertices, we can tell what it is by the structure size
	switch (dwHdrSize) {
	case (sizeof(pvr_vertex_t)):		// we DO care about vertices
		pV=(pvr_vertex_t*)pHdr;

		if (bFirstVertex) 
		{
			// for transparent polys, we have to register one of these for each
			// We never actually want to pass 0 here - if we see any wireframes, fix
			// the code to crop alpha at 1, not 0 (actually 8 since we shift extra)
		    G3_PolygonAttr(GX_LIGHTMASK_NONE,  // no lights
		                   GX_POLYGONMODE_MODULATE,     // modulation mode
		                   GX_CULL_NONE,       // cull none
		                   POLY_MISC,          // polygon ID(0 - 63)
//			                   (int)((pV->argb&0xff000000)>>27),                 // alpha(0 - 31 0-wireframe,31=opaque)
						   1,	// still using zero anyway, since this code is being made obsolete
		                   0                   // OR of GXPolygonAttrMisc's value
		        );	

			G3_Begin(GX_BEGIN_TRIANGLE_STRIP);
		//	G3_Begin(GX_BEGIN_QUADS);
			bFirstVertex=FALSE;
			lastargb=~(pV->argb);
		}
		
		if (pV->argb!=lastargb) {
			myG3_Color((pV->argb&0x00ff0000)>>19, (pV->argb&0x0000ff00)>>11, (pV->argb&0x000000ff)>>3);
			lastargb=pV->argb;
		}
		
		// Translate coordinates to a range -FX16_ONE to +FX16_ONE
		x=ConvX(pV->x);
		y=ConvY(pV->y);
		// assume a maximum z of 1024.0
		z=(1024.0f-pV->z);
		G3_Vtx(x, y, (fx16)(z*4));
		
		if (pV->flags == PVR_CMD_VERTEX_EOL) 
		{
			bFirstVertex=TRUE;
			G3_End();
		}
		break;
	}
}

// Allocate a memory buffer (replacement for pvr_mem_malloc)
pvr_ptr_t ALLOC_IMAGE(int xsize, int ysize, int shift4bit) {
	u32 nTmpAddr = 0;
	u32 nNewSize=((u32)(xsize*ysize)>>1)<<shift4bit;
	
	// round up to next 32-bit size
	nNewSize=(nNewSize+3)&0xfffffffc;
	
	if (nCurrentVRAMPtr+(nNewSize) <= MAX_VRAM_SIZE) {
		nTmpAddr=nCurrentVRAMPtr;
		nCurrentVRAMPtr+=nNewSize;
//		debug("Okayed to alloc %dx%d\n", xsize, ysize);
	} else {
		debug("************* Failed to alloc %dx%d\n", xsize, ysize);
		nVRAMAllocFailures++;
	}
	
	return nTmpAddr;
}

// Allocate some palette memory
pvr_ptr_t ALLOC_PAL(int nsize) {
	u32 nTmpAddr = 0;
	u32 nNewSize;
	
	if (nsize == SIZE_4BITx3) {
		nNewSize = 32*3;
	} else if (nsize == SIZE_4BIT) {
		nNewSize=32;
	} else {
		nNewSize=512;
	}
	
	if (nCurrentPALPtr+(nNewSize) <= MAX_PAL_SIZE) {
		nTmpAddr=nCurrentPALPtr;
		nCurrentPALPtr+=nNewSize;
	} else {
		nVRAMAllocFailures++;
	}
	
	return nTmpAddr;
}

// Free ALL PVR memory (all memory cleared)
void pvr_mem_reset() {
	nCurrentVRAMPtr = 0;
	nCurrentPALPtr = 0;
	nVRAMAllocFailures = 0;
}

// Print PVR memory in use 
void pvr_mem_stats() {
	debug("VRAM Stats:\n");
	debug(" Used:  0x%08x bytes\n", nCurrentVRAMPtr);
	debug(" Total: 0x%08x bytes\n", MAX_VRAM_SIZE);
	// The +1 works around a bug which was storing the result in the high 16 bits!?
	debug(" Free:  %d %%\n", ((MAX_VRAM_SIZE-nCurrentVRAMPtr)*100)/(MAX_VRAM_SIZE+1));
	debug("PALETTE:\n");
	debug(" Used:  0x%08x bytes\n", nCurrentPALPtr);
	debug(" Total: 0x%08x bytes\n", MAX_PAL_SIZE);
	// The +1 works around a bug which was storing the result in the high 16 bits!?
	debug(" Free:  %d %%\n", ((MAX_PAL_SIZE-nCurrentPALPtr)*100)/(MAX_PAL_SIZE+1));
	debug("Fails: %d times\n", nVRAMAllocFailures);
	
    //---- Display heap information
	debug("-- Heap information:\n");
    OS_DumpHeap(OS_ARENA_MAIN, handle);
}

// Set an image to all black (or color 0)
// you must set the bank to LCDC first
// replacement for memset ;)
void CLEAR_IMAGE(pvr_ptr_t p, int x, int y, int depth) {
	if (INVALID_PTR == p) return;
	int nSize=x*y;
	switch (depth) {
		case 4: nSize=nSize/2; break;
		case 8: break;
		
		case 16: 
			nSize=nSize*2; 
			// special case 16, since we need to set the alpha bit to avoid pure white
			GX_BeginLoadTex();		
				MI_CpuFillFast((void*)(HW_LCDC_VRAM_A+p), 0x80008000, nSize);
			GX_EndLoadTex();
			return;			
				
		default: debug("What size is %d? Not clearing image.\n", depth); return;
	}
	GX_BeginLoadTex();		
		MI_CpuClearFast((void*)(HW_LCDC_VRAM_A+p), nSize);
	GX_EndLoadTex();
}

// Load a BMP into a KOS img buffer, return negative value on error
// We use BMP just for simplicity 
// Note this only works with crappy simple v1 BMPs

// this function handles the palette only
// file pointer must be at the beginning of the file
int bmp_to_pal(FSFile *fp, kos_img_t *pImg) {
	unsigned char buf[40]={
		0
	};	// enough space for the header
	u32 idx;
	u16 *pCol;

	// read header
	// TODO: do we need to report errors to pass lotcheck? Probably!
	FS_ReadFile(fp, buf, 14);

	if ((buf[0] != 'B') || (buf[1] != 'M')) {
		debug("Image does not look like a BMP file.\n");
		FS_CloseFile(fp);
		return -1;
	}
	
	// read infoheader
	FS_ReadFile(fp, buf, 40);
	pImg->fmt = (u32)((buf[35]<<24)|(buf[34]<<16)|(buf[33]<<8)|buf[32]);	// color count
	
	if (pImg->fmt != 0) {
		// read color table (this is expected to be here)
		pImg->pal=OS_Alloc(pImg->fmt<<1);		// *2 - 16 bits per color
		if (NULL == pImg->pal) {
			debug("Failed to allocate palette memory - %d bytes\n", pImg->fmt<<1);
		    //---- Display heap information
		    OS_DumpHeap(OS_ARENA_MAIN, handle);
			FS_CloseFile(fp);
			return -1;
		}
		pCol=(u16*)pImg->pal;
		for (idx=0; idx<pImg->fmt; idx++) {
			// read one color and convert it
			FS_ReadFile(fp, buf, 4);
			// bgr0 8880 format palette entry, we need bgr 555
			pCol[idx]=(u16)(((buf[0]>>3)<<10)|((buf[1]>>3)<<5)|(buf[2]>>3));
		}
	} else {
		pImg->pal = NULL;
	}
	
	return 0;
}

// this part handles just the image
// file pointer must be at the beginning of the file
int bmp_to_pixels(FSFile *fp, kos_img_t *pImg) {
	unsigned char buf[40]={
		0
	};	// enough space for the header
	u32 nOff;
	u32 idx;

	// read header
	FS_ReadFile(fp, buf, 14);

	if ((buf[0] != 'B') || (buf[1] != 'M')) {
		debug("Image does not look like a BMP file.\n");
		FS_CloseFile(fp);
		return -1;
	}
	
	nOff=(u32)((buf[13]<<24)|(buf[12]<<16)|(buf[11]<<8)|buf[10]);
	 
	// read infoheader
	FS_ReadFile(fp, buf, 40);
 
	pImg->byte_count = (u32)((buf[23]<<24)|(buf[22]<<16)|(buf[21]<<8)|buf[20]);
	// If the byte count is not a multiple of 4 (Photoshop does this), truncate
	pImg->byte_count &= 0xfffffffc;
	
	if (pImg->fmt != (u32)((buf[35]<<24)|(buf[34]<<16)|(buf[33]<<8)|buf[32])) {
		// color count should match the palette file
		debug("Warning - palette and image don't match color count!\n");
	}
	
	pImg->h = (u32)((buf[11]<<24)|(buf[10]<<16)|(buf[9]<<8)|buf[8]);
	pImg->w = (u32)((buf[7]<<24)|(buf[6]<<16)|(buf[5]<<8)|buf[4]);
	
	// Seek to the beginning of pixel data and read that
	pImg->data=OS_Alloc(pImg->byte_count);
	if (NULL == pImg->data) {
		debug("Failed to allocate image memory - %d bytes\n", pImg->byte_count);
	    //---- Display heap information
	    OS_DumpHeap(OS_ARENA_MAIN, handle);
		FS_CloseFile(fp);
		OS_Free(pImg->pal);
		return -1;
	}
  
	FS_SeekFile(fp, (s32)nOff, FS_SEEK_SET);
	FS_ReadFile(fp, pImg->data, (long)pImg->byte_count);
	// naturally, 4-bit images load in the wrong pixel order
	if (16 == pImg->fmt) {
		// process them
		for (idx=0; idx<pImg->byte_count; idx++) {
			nOff=*((char*)pImg->data+idx);
			*((unsigned char*)pImg->data+idx)=(unsigned char)((nOff&0xf)<<4)|((nOff&0xf0)>>4);
		}
	} else if (0 == pImg->fmt) {
		// convert 24 bit to 16 bit RGB
		unsigned char *pSrc=(unsigned char*)pImg->data;
		u16 *pDest=(u16*)pImg->data;
		for (idx=0; idx<pImg->byte_count; idx+=3) {
			// Input is BGR888, we want BGR555
			*pDest=(u16)(((pSrc[0]>>3)<<10)|((pSrc[1]>>3)<<5)|(pSrc[2]>>3))|0x8000;
			pSrc+=3;
			pDest++;
		}
		pImg->byte_count = pImg->w*pImg->h*2;
	}
	
	return 0;
}

// TODO: add LZ compression (see ntrcomp.exe) - gets about 20%
// on the story mode pics, should be higher on the others. Saves
// about 6-7MB on the final ROM.. maybe get it down to under
// 32MB? If we don't the compression doesn't help us much. A test
// today suggests that compressing the BMPs will save up to 15MB,
// which is enough to be worth it.
// CARD version of bmp_to_img
// szFn - filename of the BMP to load
// szPf - filename of separate palette to load (or NULL if none)
// pImg - KOS image structure which is filled in
// WARNING: allocates RAM for image data and palette, you must use kos_img_free() to release it
int  bmp_to_img(char *szFn, char *szPf, kos_img_t *pImg) {
	// This version works with the card filesystem
	FSFile fp;

	FS_InitFile(&fp); 
	 
	// handle the palette first
	if (NULL != szPf) {
		// we have a separate palette file, so process that first
		if (FALSE == FS_OpenFile(&fp, szPf)) {
			debug("Could not open palette %s\n", szPf);
			return -1;
		}
	} else {
		if (FALSE == FS_OpenFile(&fp, szFn)) {
			debug("Could not open %s\n", szFn);
			return -1;
		}
	}
	
	if (bmp_to_pal(&fp, pImg) < 0) {
		return -1;
	}
		
	// palette is done, now we want data
	if (NULL != szPf) {
		// close the palette file and open the correct one
		FS_CloseFile(&fp);

		if (FALSE == FS_OpenFile(&fp, szFn)) {
			debug("Could not open %s\n", szFn);
			return -1;
		}
	} else {
		// just seek back to the beginning
		FS_SeekFile(&fp, (s32)0, FS_SEEK_SET);
	}
	
	if (bmp_to_pixels(&fp, pImg) < 0) {
		return -1;
	}
		
	FS_CloseFile(&fp);

	return 0;
}

// Copy a KOS img type into video memory
// May optimize not to call Begin..End every single time.
// herd: 0-3, -1 for none, -2 means load tiles
// WARNING: Do not specify herd of 0-3 unless there is enough palette space for 3 palettes!
// First palette is normal. Second forces color 12 to white for Hades, third
// forces color 3 to white for sheep.
void pvr_txr_load_kimg(kos_img_t *pImg, pvr_ptr_t pWhere, pvr_ptr_t pPalette, int herd) {
	u16 nOldPal;
	
	// make sure the relevant cache lines are flushed
	DC_FlushRange(pImg->data, pImg->byte_count);
	if (INVALID_PTR != pPalette) {
		if (NULL != pImg->pal) {
			DC_FlushRange(pImg->pal, pImg->fmt<<1);
		}
	}
	
	if (pWhere != INVALID_PTR) {
		if (nLast3DCount == 0) {
			debug("Fast load available, no 3D active.");
			// the 3D engine is idle, so just load it all at once
			GX_BeginLoadTex();
				GX_LoadTex(pImg->data, pWhere, pImg->byte_count);
			GX_EndLoadTex();
		} else {
			// not idle, do a careful load
			int nDataSize = pImg->byte_count;
			int nOffset = 0;
			 
			while (nDataSize > 0) {
				// try to keep the sound running
				sound_frame_update();
				
				// to reduce flicker, wait for a vblank before loading to texture RAM
				// this way we don't have to turn off the 3D engine
				OS_WaitVBlankIntr();
				// now we have a (very brief) chance to copy data!	
				GX_BeginLoadTex();
					GX_LoadTex((char*)pImg->data+nOffset, pWhere+nOffset, min(nDataSize, SAFE_COPY_BYTES_PER_VBLANK));
				GX_EndLoadTex();
				nDataSize -= SAFE_COPY_BYTES_PER_VBLANK;
				nOffset += SAFE_COPY_BYTES_PER_VBLANK;
			}
		}
	}
	
	if (INVALID_PTR != pPalette) {
		if (NULL != pImg->pal) {
			// and we'll just hopefully have enough time for the palette ;)
			if (nLast3DCount != 0) {
				// play it safe and wait again
				sound_frame_update();
				// to reduce flicker, wait for a vblank before loading to texture RAM
				// this way we don't have to turn off the 3D engine
				OS_WaitVBlankIntr();
			}
				
			GX_BeginLoadTexPltt();
				GX_LoadTexPltt(pImg->pal, pPalette, pImg->fmt<<1);
				
				if (herd > -1) {
					// make color 2 white
					nOldPal=((u16*)pImg->pal)[2];
					((u16*)pImg->pal)[2] = 0x7fff;
					DC_FlushRange(pImg->pal, pImg->fmt<<1);
					GX_LoadTexPltt(pImg->pal, pPalette+(pImg->fmt<<1), pImg->fmt<<1);

					// make color 3 white
					((u16*)pImg->pal)[2] = nOldPal;
					nOldPal=((u16*)pImg->pal)[3];
					((u16*)pImg->pal)[3] = 0x7fff;
					DC_FlushRange(pImg->pal, pImg->fmt<<1);
					GX_LoadTexPltt(pImg->pal, pPalette+(pImg->fmt<<2), pImg->fmt<<1);
					
					// restore palette for 2D screen
					((u16*)pImg->pal)[3] = nOldPal;
					DC_FlushRange(pImg->pal, pImg->fmt<<1);
				}
			GX_EndLoadTexPltt();
			
			// after this point we are done with 3D RAM
			if (herd > -1) 
			{
				MapScr_SetHerderPalette(herd, pImg->pal);
			}
		}
	}

	// the rest are 2D operations
	if (herd > -1) 
	{
		// the herder sprite data has the top 48x48 pixels as always a
		// clear, unpacked version of the standing sprite.
		// Some centering in here may still be needed
		// Sprite is 64x64, need to fill the whole thing
		// newpic has a bug that seems to have packed it only 47 pixels tall, so
		// ignore the last row.
		u32 *pHerd=(u32*)MapScr_GetHerderAddress(herd);
		memset(pHerd, 0, 2048);	// 64x64x4bit
		int x,y,z;
		for (y=0; y<8; y++) 			// y blocks
		{
			for (x=0; x<8; x++)			// x blocks
			{
				for (z=0; z<8; z++)		// rows inside this block
				{
					if ((x>=6)||(y>=6)||((y==5)&&(z==7))) 
					{
						pHerd++;		// skip over
					}
					else 
					{
						// copies 8 4-bit pixels
						*(pHerd++)=*(u32*)(((char*)pImg->data)+(255-(y*8+z))*128+x*4);
					}
				}
			}
		}
	}

	if (herd == -2) {
		// special hack to load to the subscreen memory for the Nintendo logo
		// first reformat the video data into 8x8 tiles like for the sprite
		u32 *pHerd=(u32*)G2S_GetBG0CharPtr();
		
		// this assumes a size of 256x192 - that's the only valid size for this option
		if ((pImg->w != 256) || (pImg->h != 192) || (pImg->fmt != 16)) {
			debug("%d x %d x %d colors may not load correctly\n", pImg->w, pImg->h, pImg->fmt);
		}
		
		// make sure it's empty
		memset(pHerd, 0, 32768);	// assuming 32k bank! This limits the size of the pic
									// which is also hard coded below. Sorry ;)
		int x,y,z;
		int c=0;
		for (y=0; y<24; y++) 				// character rows
		{		
			for (x=0; x<32; x++)			// character columns
			{
				for (z=0; z<8; z++)			// scanline rows
				{
					// this line copies 8 4-bit pixels
					//                                     height             stride
					*(pHerd++)=*(u32*)(((char*)pImg->data)+(pImg->h-1-(y*8+z))*(pImg->w/2)+x*4);
				}
				c++;
			}
		}
		GXS_LoadBGPltt(pImg->pal, 0, pImg->fmt*2);
	}
}

// Free a KOS image buffer
void kos_img_free(kos_img_t *pImg) {
	if (NULL == pImg) return;
	
	OS_Free(pImg->data);
	pImg->data=NULL;
	if (NULL != pImg->pal) {
		OS_Free(pImg->pal);
		pImg->pal=NULL;
	}
}

//
// System
//

// Terminate the program - do not return
// NDS: I think we need to reboot?
void arch_exit() {
#if 0
	SendMessage(myWnd, WM_CLOSE, 0, 0);
	_endthreadex(0);
#endif
}

// Sleep for nDelay ms
void thd_sleep(int nDelay) {
	OS_Sleep((unsigned long)nDelay);
}
// To be completely honest, the DS threading system doesn't really help much

// read and load a nitro palette file into the specified hardware palette
// returns 0 on success
int ReadNitroPalette(char *pszFile, void *pWhere) {
	FSFile fp;
	char buf[16];
	int i;

	FS_InitFile(&fp);
	
	if (FALSE == FS_OpenFile(&fp, pszFile)) {
		debug("Could not open nitro palette %s\n", pszFile);
		return -1;
	}
	
	FS_SeekFile(&fp, (s32)16, FS_SEEK_SET);
	FS_ReadFile(&fp, buf, 16);
	if (memcmp(buf, "PALT", 4)) {
		debug("Does not look like a Nitro Palette file!");
		FS_CloseFile(&fp);
		return -1;
	}

	// skip header (note lack of validation!)
	// TODO: do we need to verify errors to pass lotcheck?
	FS_SeekFile(&fp, (s32)32, FS_SEEK_SET);
	
	// read palette and copy as needed
	for (i=0; i<32; i++) {
		FS_ReadFile(&fp, buf, 16);
		MI_CpuCopy16(buf, (char*)pWhere+(i<<4), 16);
	}
		
	FS_CloseFile(&fp);
	
	return 0;
}

// read and load a nitro character file into the specified VRAM address
// fills in a 1k array with the palette number for the char (shift up 
// by 4 and or with the character index to use) - need this cause
// we are using the 16 palettes of 16 colors approach in order to give
// us a full 1024 characters available.
// Note assumes 16x16 palette, 1024 4-bit characters
// returns 0 on success
int ReadNitroChar(char *pszFile, void *pWhere, char *pTable) {
	FSFile fp;
	char buf[16];
	int i;

	FS_InitFile(&fp);
	
	if (FALSE == FS_OpenFile(&fp, pszFile)) {
		debug("Could not open nitro character %s\n", pszFile);
		return -1;
	}
	
	FS_SeekFile(&fp, (s32)16, FS_SEEK_SET);
	FS_ReadFile(&fp, buf, 16);
	if (memcmp(buf, "CHAR", 4)) {
		debug("Does not look like a Nitro Character file!\n");
		FS_CloseFile(&fp);
		return -1;
	}

	// skip header (note lack of validation!)
	// TODO: do we need to verify errors to pass lotcheck?
	FS_SeekFile(&fp, (s32)36, FS_SEEK_SET);
	
	// read characters and copy as needed
	for (i=0; i<2048; i++) {
		FS_ReadFile(&fp, buf, 16);
		MI_CpuCopy16(buf, (char*)pWhere+(i<<4), 16);
	}
	
	// we should be pointing at the attribute block now
	FS_ReadFile(&fp, buf, 16);
	if (memcmp(buf, "ATTR", 4)) {
		debug("Did not find the expected ATTR block\n");
		return -1;
	}
	
	// read the attributes into the passed table
	FS_ReadFile(&fp, pTable, 1024);
	
	// done		
	FS_CloseFile(&fp);
	
	return 0;
}

	
