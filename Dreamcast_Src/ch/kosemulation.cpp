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

// NOTE: These functions do not necessarily duplicate the functions of the
// original KOS functions. In most cases, they only reproduce the behaviour
// that I need for Cool Herders to run! (And not necessarily 100% correctly)

// The PNG loading seems broken in ImageSource... we should import libPNG,
// perhaps, just to have the same loader as the DC uses.

#include <stdio.h>
#include <process.h>
#include <stdlib.h>
#include <math.h>
#include <io.h>
#include <fcntl.h>

#include "kosemulation.h"
#include "c:\work\imgsource\islibs\isource.h"
//#include "\Work\ogg\include\vorbis\codec.h"
//#include "\Work\ogg\include\vorbis\vorbisfile.h"

#ifdef _DEBUG
	extern "C" {
		#include "sprite.h"
		#include "cool.h"
		extern int colr[4];
		extern int colg[4];
		extern int colb[4];
	}
#else
	extern "C" {
		// cool.h
		void debug(char *str, ...);;
	}
#endif

// Needed vars
HANDLE endEvent=NULL;
void (*BtnCallback)(uint8, uint32)=NULL;
DWORD dwBtnMask=0;
HANDLE hMainThread;
HWND myWnd;
HDC hDC;
HBITMAP hBmp;
HANDLE hOldBmp;
pvr_ptr_t pCurrentTexture=NULL;
int nCurrentWidth=0, nCurrentHeight=0;
int nCurrentTrans=0;
int nCurVertex=0;			// We draw 4 vertices at a time and need to sort one rectangle from it
BITMAPINFO myInfo;
// Ogg playback stuff
//char pcmout[4096];
//OggVorbis_File vf;
// Sound fx
int nLastSound=0;
char szSounds[16][128];

maple_device_t AllDevs[MAPLE_PORT_COUNT][MAPLE_UNIT_COUNT];

// Used externally
volatile pvr_state_t pvr_state;

char padding[128];

struct DrawListItem {
	int x, y, w, h;
	int destX, destY, destW, destH;
	float z;			// used for sorting
	int  alpha;			// will try to use this ;)
	void *pData;		// Address of buffer
	int nWidth, nHeight;// Size of buffer
	int fTrans;			// Transparent?
	struct DrawListItem *pPrev;
	struct DrawListItem *pNext;
};

// Static head and tail greatly simplifies the list
// Note there's no locking! Not thread safe!
struct DrawListItem theHead;
struct DrawListItem theTail;

//
// Video
//
// We emulate the PVR system by creating a list of tiles to draw. Noting that
// Cool Herders only uses triangles in pairs to draw rectangles, we assume that.

// Prepares the video system
void pvr_init_defaults() {
	HDC hTmp;

	// Since this is called early, we'll use it to init the system
	memset(AllDevs, 0, sizeof(AllDevs));
	// The first two joysticks are available (on my PC, anyway)
	AllDevs[0][0].valid=1;
	AllDevs[0][0].port=0;
	AllDevs[0][0].unit=0;
	AllDevs[0][0].info.functions=MAPLE_FUNC_CONTROLLER;

	AllDevs[1][0].valid=1;
	AllDevs[1][0].port=1;
	AllDevs[1][0].unit=0;
	AllDevs[1][0].info.functions=MAPLE_FUNC_CONTROLLER;

	// Init the list
	theHead.pPrev=NULL;
	theHead.pNext=&theTail;
	theTail.pPrev=&theHead;
	theTail.pNext=NULL;

	// Setup the vertex counter
	nCurVertex=0;

	// Create the GDI objects (Direct what? ;) )
	hDC=CreateCompatibleDC(NULL);
	hTmp=GetDC(myWnd);
	hBmp=CreateCompatibleBitmap(hTmp, 640, 480);
	ReleaseDC(myWnd, hTmp);
	hOldBmp=SelectObject(hDC, hBmp);

	// Initialize the imglib (for PNG loading)
	ISInitialize("{50EB2018-DCE6-4773-B3C2-3C58192F64DA}");
}

void pvr_init(pvr_init_params_t *params) {
	// We don't care about the params in this version
	pvr_init_defaults();
}

// Wait for the video chip to be ready
void pvr_wait_ready() {
	// always ready (could wait for VSync in here, but that's silly in a Window)
}

// Start to describe a scene
void pvr_scene_begin() {
	// Delete the old list, if present
	struct DrawListItem *pItem, *pNext;

	pItem=theHead.pNext;
	while (&theTail != pItem) {
		pNext=pItem->pNext;
		free(pItem);
		pItem=pNext;
	}

	theHead.pPrev=NULL;
	theHead.pNext=&theTail;
	theTail.pPrev=&theHead;
	theTail.pNext=NULL;
}

// Start a polygon list of the specified type
void pvr_list_begin(DWORD dwType) {
	pvr_state.list_reg_open = dwType;
	pvr_state.render_busy=0;
}

// Finish a polygon list
void pvr_list_finish() {
	// We don't care about types
}

// Finish and draw the scene
void pvr_scene_finish() {
	// Process the list, render it into the tmp buffer
	// The list is already in the exact order that we want to draw it
	HANDLE hTmpBmp=NULL;
	HBITMAP hNewBmp;
	HDC hTmpDC;
	void *pOldTexture=NULL;
	struct DrawListItem *pItem=theHead.pNext;
	int nCount=0, ret;

	hTmpDC=CreateCompatibleDC(hDC);
	BitBlt(hDC, 0, 0, 640, 480, NULL, 0, 0, BLACKNESS);
	while (&theTail != pItem) {
		BLENDFUNCTION myBlend;

		if (pOldTexture != pItem->pData) {
			if (hTmpBmp) {
				SelectObject(hTmpDC, hTmpBmp);
			}
			hTmpBmp=SelectObject(hTmpDC, pItem->pData);
			pOldTexture=pItem->pData;
			myBlend.BlendOp=AC_SRC_OVER;
			myBlend.BlendFlags=0;
			myBlend.AlphaFormat=AC_SRC_ALPHA;
		}
		
		myBlend.SourceConstantAlpha=(BYTE)(pItem->alpha);
		ret=AlphaBlend(hDC, pItem->destX, pItem->destY, pItem->destW, pItem->destH, hTmpDC, pItem->x, pItem->y, pItem->w, pItem->h, myBlend);

		nCount++;
		pItem=pItem->pNext;
	}

#ifdef _DEBUG2
	if (nCount > 100) {
		// probably ingame - draw the pathfinding information out
		for (int idx=0; idx<4; idx++) {
			HPEN hPen=CreatePen(PS_SOLID, 3, RGB(colr[idx],colg[idx],colb[idx]));
			HPEN hOld=(HPEN)SelectObject(hDC, hPen);
			int idx2=2;
			int lastx=herder[idx].path[0];
			int lasty=herder[idx].path[1];

			lastx=lastx*GRIDSIZE+PLAYFIELDXOFF+16;
			lasty=lasty*GRIDSIZE+PLAYFIELDYOFF+16;

			MoveToEx(hDC, lastx, lasty, NULL);
			while (herder[idx].path[idx2]>-1) {
				int lastx=herder[idx].path[idx2];
				int lasty=herder[idx].path[idx2+1];
				lastx=lastx*GRIDSIZE+PLAYFIELDXOFF+16;
				lasty=lasty*GRIDSIZE+PLAYFIELDYOFF+16;
				LineTo(hDC, lastx, lasty);
				idx2+=2;
			}
			SelectObject(hDC, hOld);
			DeleteObject(hPen);
		}
	}
#endif

//	debug("Drew %d objects.\n", nCount);
	if (NULL != hTmpBmp) {
		SelectObject(hTmpDC, hTmpBmp);
	}
	DeleteDC(hTmpDC);

	// Refresh the window
	InvalidateRect(myWnd, NULL, false);

	// After drawing, check exit event
	if (WaitForSingleObject(endEvent, 0) == WAIT_OBJECT_0) {
		if (NULL != BtnCallback) {
			BtnCallback(0, 0);
		} else {
			_endthreadex(0);
		}
	}

	// And if we're still running, this is where we need to delay for timing
	// TODO
	Sleep(0);
}

// Used to load information about the following textures
void pvr_poly_cxt_txr(pvr_poly_cxt_t *pContext, int pvrListType, int pvrTextureFormat, int nWidth, int nHeight, pvr_ptr_t pAddr, DWORD dwFilter) {
	if (pCurrentTexture == pAddr) {
		return;		// we already know this one
	}

	pCurrentTexture=pAddr;
	nCurrentWidth=nWidth;
	nCurrentHeight=nHeight;
	nCurrentTrans=(pvrListType == PVR_LIST_TR_POLY);
}

// Used to compile a context set up with pvr_poly_cxt_txr into a primitive for the list
void pvr_poly_compile(pvr_poly_hdr_t *pHdr, pvr_poly_cxt_t *pContext) {
	// We don't need to use it here
}

// Submit a structure of the *current* list type to the hardware
// CH sorts rectangles like this:
//    2 4
//    1 3
// We ignore the color adjustment component of the RGB value, which
// is currently used for fading in and out. So we may need it someday.
void pvr_prim(void *pHdr, DWORD dwHdrSize) {
	pvr_vertex_t *pV;
	struct DrawListItem *pNew, *pItem;
	static int destx[2], desty[2], x[2], y[2];
	static void *pData;
	static float z;
	static int alpha;

	// We are only interested in vertices, we can tell what it is by the structure size
	switch (dwHdrSize) {
	case (sizeof(pvr_poly_hdr_t)):		// don't care about context headers
		break;
	
	case (sizeof(pvr_vertex_t)):		// we DO care about vertices
		pV=(pvr_vertex_t*)pHdr;

		nCurVertex++;
		switch (nCurVertex) {
		case 2:
			alpha=nCurrentTrans ? (pV->argb>>24) : 255;
			pData=pCurrentTexture;
			z=pV->z;
			destx[0]=pV->x;
			desty[0]=pV->y;
			x[0]=(int)(pV->u * nCurrentWidth);
			y[0]=(int)(pV->v * nCurrentHeight);
			break;

		case 3:
			destx[1]=pV->x;
			desty[1]=pV->y;
			x[1]=(int)(pV->u * nCurrentWidth);
			y[1]=(int)(pV->v * nCurrentHeight);

			// check for different order than we expected
			if (destx[1] < destx[0]) {
				// sending as 1 2
				//            3 4
				// So we need to shuffle the X data  (Y is okay)
				// (Note we still assume we never get hidden polys)
				int t1;
				t1=destx[0];
				destx[0]=destx[1];
				destx[1]=t1;
				t1=x[0];
				x[0]=x[1];
				x[1]=t1;
			}

			// Create the new structure
			if (pV->z == z) {
				// Flat poly
				pNew=(struct DrawListItem*)malloc(sizeof(struct DrawListItem));
				pNew->alpha=alpha;
				pNew->fTrans=nCurrentTrans;
				pNew->pData=pData;
				pNew->nWidth=nCurrentWidth;
				pNew->nHeight=nCurrentHeight;
				pNew->destX=destx[0];
				pNew->destY=desty[0];
				pNew->destW=destx[1]-destx[0];
				pNew->destH=desty[1]-desty[0];
				pNew->x=x[0];
				pNew->y=y[0];
				pNew->w=x[1]-x[0];
				pNew->h=y[1]-y[0];
				pNew->z=z;
			} else {
				// 3D poly: Split into two items for 3D tiles
				pNew=(struct DrawListItem*)malloc(sizeof(struct DrawListItem));
				pNew->alpha=alpha;
				pNew->fTrans=nCurrentTrans;
				pNew->pData=pData;
				pNew->nWidth=nCurrentWidth;
				pNew->nHeight=nCurrentHeight;
				pNew->destX=destx[0];
				pNew->destY=desty[0];
				pNew->destW=destx[1]-destx[0];
				pNew->destH=(desty[1]-desty[0])/2;
				pNew->x=x[0];
				pNew->y=y[0];
				pNew->w=x[1]-x[0];
				pNew->h=(y[1]-y[0])/2;
				pNew->z=z;

				// Sort this one
				// Now find it's place in the list
				// We sort first by Z order (ascending), then by Y order (ascending), and last by X order (ascending)
				pItem=theHead.pNext;
				while (&theTail != pItem) {
					if (pNew->z < pItem->z) break;
					pItem=pItem->pNext;
				}
				while (&theTail != pItem) {
					if (pNew->y < pItem->y) break;
					pItem=pItem->pNext;
				}
				while (&theTail != pItem) {
					if (pNew->x < pItem->x) break;
					pItem=pItem->pNext;
				}
				// This is it - insert before pItem
				pNew->pNext=pItem;
				pNew->pPrev=pItem->pPrev;
				pNew->pPrev->pNext=pNew;
				pNew->pNext->pPrev=pNew;

				// This one will be sorted below
				pNew=(struct DrawListItem*)malloc(sizeof(struct DrawListItem));
				pNew->alpha=alpha;
				pNew->fTrans=nCurrentTrans;
				pNew->pData=pData;
				pNew->nWidth=nCurrentWidth;
				pNew->nHeight=nCurrentHeight;
				pNew->destX=destx[0];
				pNew->destY=desty[0]+((desty[1]-desty[0])/2);
				pNew->destW=destx[1]-destx[0];
				pNew->destH=(desty[1]-desty[0])/2;
				pNew->x=x[0];
				pNew->y=y[0]+((y[1]-y[0])/2);
				pNew->w=x[1]-x[0];
				pNew->h=(y[1]-y[0])/2;
				pNew->z=pV->z;
			}

			// Now find it's place in the list
			// We sort first by Z order (ascending), then by Y order (ascending), and last by X order (ascending)
			pItem=theHead.pNext;
			while (&theTail != pItem) {
				if (pNew->z <= pItem->z) break;
				pItem=pItem->pNext;
			}
			if (pNew->z == pItem->z) {
				while (&theTail != pItem) {
					if (pNew->y <= pItem->y) break;
					pItem=pItem->pNext;
				}
				if (pNew->y == pItem->y) {
					while (&theTail != pItem) {
						if (pNew->x <= pItem->x) break;
						pItem=pItem->pNext;
					}
				}
			}
			// This is it - insert before pItem
			pNew->pNext=pItem;
			pNew->pPrev=pItem->pPrev;
			pNew->pPrev->pNext=pNew;
			pNew->pNext->pPrev=pNew;
			break;
		}

		if (pV->flags == PVR_CMD_VERTEX_EOL) nCurVertex=0;
		break;
	}
}

// Allocate a memory buffer (replacement for pvr_mem_malloc)
HANDLE ALLOC_IMAGE(int xsize, int ysize) {
	return CreateCompatibleBitmap(hDC, xsize, ysize);
}

// Free PVR memory
void pvr_mem_free(pvr_ptr_t chunk) {
	DeleteObject(chunk);
}

// Free ALL PVR memory (no equivalent here)
void pvr_mem_reset() {
}

// Print PVR memory in use (no equivalent)
void pvr_mem_stats() {
}

// Draw a centered text string directly into a buffer, fixed at 512x512 (ignores 'x')
void DRAW_TEXT(pvr_ptr_t pBuffer, int x, int y, char *pStr) {		// replacement for bfont_draw_str
	HDC hTmpDC;
	HANDLE hOldBmp;
	RECT myRect;

	hTmpDC=CreateCompatibleDC(hDC);
	hOldBmp=SelectObject(hTmpDC, pBuffer);

	myRect.left=0;
	myRect.right=512;
	myRect.top=y;
	myRect.bottom=9999;

	SetMapMode(hTmpDC, MM_TEXT);
	SetTextColor(hTmpDC, RGB(0xff, 0xff, 0xff));
	SetBkColor(hTmpDC, RGB(0,0,0));
	DrawText(hTmpDC, pStr, -1, &myRect, DT_CENTER|DT_SINGLELINE|DT_TOP|DT_NOPREFIX);
	
	SelectObject(hTmpDC, hOldBmp);
	DeleteDC(hTmpDC);
}

// Set an image to all black
void CLEAR_IMAGE(pvr_ptr_t p, int x, int y) {					// replacement for memset ;)
	HDC hTmpDC;
	HANDLE hOldBmp;

	hTmpDC=CreateCompatibleDC(hDC);
	hOldBmp=SelectObject(hTmpDC, p);
	BitBlt(hTmpDC, 0, 0, x, y, NULL, 0, 0, BLACKNESS);
	SelectObject(hTmpDC, hOldBmp);
	DeleteDC(hTmpDC);
}

// Load a PNG into a KOS img buffer, return negative value on error
int  png_to_img(char *szFn, DWORD dwFlags, kos_img_t *pImg) {
	HISSRC hSource;
	UINT32 uW, uH;
	HGLOBAL pDat;
	RGBQUAD Pal[256];

	hSource=ISOpenFileSource(szFn);

	if (NULL == hSource) return -1;
	
	// Can read the Alpha channel with 32bpp, so the mask isn't important under windows
	pDat=ISReadPNG(hSource, &uW, &uH, 32, Pal);

	ISCloseSource(hSource);

	if (NULL == pDat) return -1;

	pImg->data=malloc(uW*uH*4);
	memcpy(pImg->data, pDat, uW*uH*4);
	GlobalFree(pDat);

	pImg->byte_count=uW*uH*4;
	pImg->fmt=32;		// bits per pixel count (here, not in KOS)
	pImg->h=uH;
	pImg->w=uW;

	return 0;
}

// Copy a KOS img type into video memory
void pvr_txr_load_kimg(kos_img_t *pImg, pvr_ptr_t pWhere, int nUnknown) {
	int x,y;
	unsigned char *pSrc=(unsigned char*)pImg->data;
	DWORD ret;

	// color conversion from RGBA to BGRA
	// premultiply the Alpha channel for AlphaBlend
	for (y=0; y<pImg->h; y++) {
		for (x=0; x<pImg->w; x++) {
			char x;
			int alpha;

			// Swap R and B
			x=*pSrc;
			*pSrc=*(pSrc+2);
			*(pSrc+2)=x;
			
			// Premultiply Alpha
			alpha=*(pSrc+3);
			*(pSrc)=(*(pSrc)*alpha)/0xff;
			*(pSrc+1)=(*(pSrc+1)*alpha)/0xff;
			*(pSrc+2)=(*(pSrc+2)*alpha)/0xff;
			
			pSrc+=4;
		}
	}

	myInfo.bmiHeader.biSize=sizeof(myInfo.bmiHeader);
	myInfo.bmiHeader.biPlanes=1;
	myInfo.bmiHeader.biSizeImage=0;
	myInfo.bmiHeader.biXPelsPerMeter=1;
	myInfo.bmiHeader.biYPelsPerMeter=1;
	myInfo.bmiHeader.biClrUsed=0;
	myInfo.bmiHeader.biClrImportant=0;
	myInfo.bmiHeader.biWidth=pImg->w;
	myInfo.bmiHeader.biHeight=-(signed)(pImg->h);
	myInfo.bmiHeader.biBitCount=pImg->fmt;
	myInfo.bmiHeader.biCompression=BI_RGB;

	ret=SetDIBits(NULL, (HBITMAP)pWhere, 0, pImg->h, pImg->data, &myInfo, DIB_RGB_COLORS);
	if (ret == 0) {
		debug("SetDIBits failed, code %d\n", GetLastError());
	}
}

// Free a KOS image buffer
void kos_img_free(kos_img_t *pImg, int nUnknown) {
	free(pImg->data);
	pImg->data=NULL;
}


// 
// Controllers
//

// Registers a function to call when all requested buttons are pressed
// Num is the controller number (or 0 for any - our usage)
void cont_btn_callback(int num, DWORD dwBtns, void (*fctn)(uint8, uint32)) {
	// The uint8 is the controller address, the uint32 are a copy of the flags. But we don't use them
	// We also don't use the controller number - we expect it to be 0
	BtnCallback=fctn;
	dwBtnMask=dwBtns;
}

// Return the first connected controller index (only used to test for any)
// 0 means none
int maple_first_controller() {
	return 1;
}

// Query the requested controller and return a pointer to the cont_state_t struct
// Also check the buttons and call the callback if they match
cont_state_t *maple_dev_status(maple_device_t *pDev) {
	// trigger on end event
	static cont_state_t cState;
	JOYINFOEX myJoy;
	SHORT nKeyDownBit=0;

	if (NULL == pDev) {
		return NULL;
	}

	if (WaitForSingleObject(endEvent, 0) == WAIT_OBJECT_0) {
		if (NULL != BtnCallback) {
			BtnCallback(0, 0);
		} else {
			_endthreadex(0);
		}
	}

	// Read controller into struct. First buttons
	cState.buttons=0;
	cState.ltrig=0;
	cState.rtrig=0;
	cState.joyx=0;
	cState.joyy=0;

	if (pDev->port == 1) {
		nKeyDownBit=(SHORT)0x8000;

		if (GetAsyncKeyState(VK_SPACE)&nKeyDownBit) {
			cState.buttons|=CONT_A;
		}
		if (GetAsyncKeyState(VK_ESCAPE)&nKeyDownBit) {
			cState.buttons|=CONT_B;
		}
		if (GetAsyncKeyState(VK_RETURN)&nKeyDownBit) {
			cState.buttons|=CONT_START;
		}
		if (GetAsyncKeyState(VK_UP)&nKeyDownBit) {
			cState.buttons|=CONT_DPAD_UP;
		}
		if (GetAsyncKeyState(VK_RIGHT)&nKeyDownBit) {
			cState.buttons|=CONT_DPAD_RIGHT;
		}
		if (GetAsyncKeyState(VK_DOWN)&nKeyDownBit) {
			cState.buttons|=CONT_DPAD_DOWN;
		}
		if (GetAsyncKeyState(VK_LEFT)&nKeyDownBit) {
			cState.buttons|=CONT_DPAD_LEFT;
		}
		if (GetAsyncKeyState(VK_SHIFT)&nKeyDownBit) {
			cState.ltrig=255;
		}
		return &cState;
	}
	
	memset(&myJoy, 0, sizeof(myJoy));
	myJoy.dwSize=sizeof(myJoy);
	myJoy.dwFlags=JOY_RETURNBUTTONS | JOY_RETURNX | JOY_RETURNY | JOY_USEDEADZONE;
	if (JOYERR_NOERROR == joyGetPosEx(pDev->port, &myJoy)) {
		// Buttons configured for my PSX pad (in Analog mode) to match the DC stick
		if (myJoy.dwButtons & 0x01) {
			cState.buttons|=CONT_Y;
		}
		if (myJoy.dwButtons & 0x02) {
			cState.buttons|=CONT_B;
		}
		if (myJoy.dwButtons & 0x04) {
			cState.buttons|=CONT_A;
		}
		if (myJoy.dwButtons & 0x08) {
			cState.buttons|=CONT_X;
		}
		// 0x100 is SELECT, not used on DC
		if (myJoy.dwButtons & 0x200) {
			cState.buttons|=CONT_START;
		}
		// 0x400 and 0x800 are the analog joystick buttons, not used on DC
		if (myJoy.dwButtons & 0x1000) {
			cState.buttons|=CONT_DPAD_UP;
		}
		if (myJoy.dwButtons & 0x2000) {
			cState.buttons|=CONT_DPAD_RIGHT;
		}
		if (myJoy.dwButtons & 0x4000) {
			cState.buttons|=CONT_DPAD_DOWN;
		}
		if (myJoy.dwButtons & 0x8000) {
			cState.buttons|=CONT_DPAD_LEFT;
		}
		// What's the range in KOS of the analog stuff? (triggers 0-255, sticks -128-+127? )
		// Hmm. My PSX pads needed a 16 bit shift. The Gravis needs 8.
		cState.joyx=myJoy.dwXpos>>8;
		cState.joyy=myJoy.dwYpos>>8;
		// Dreamcast has 2 analog triggers, but the PSX has 4 digital, so I'll merge them ;)
		if (myJoy.dwButtons & 0x10) {
			cState.ltrig+=127;		// 1/2 applied
		}
		if (myJoy.dwButtons & 0x20) {
			cState.rtrig+=127;		// 1/2 applied
		}
		if (myJoy.dwButtons & 0x40) {
			cState.ltrig+=127;		// 1/2 applied
		}
		if (myJoy.dwButtons & 0x80) {
			cState.rtrig+=127;		// 1/2 applied
		}
	}

	// We check against dwBtnMask, then call BtnCallback if matched here
	if ((cState.buttons & dwBtnMask) == dwBtnMask) {
		BtnCallback(0, cState.buttons);
	}

	return &cState;
}

// Return a pointer to the device at Port nPort, unit nUnit (controller, memory, LCD, etc)
maple_device_t *maple_enum_dev(int nPort, int nUnit) {
	if ((nPort>=MAPLE_PORT_COUNT)||(nUnit>=MAPLE_UNIT_COUNT)) return NULL;
	return &AllDevs[nPort][nUnit];
}

// Return a pointer to the index 'nIndex' device of type dwType
// Only used for LCD, we don't need to implement it here
maple_device_t *maple_enum_type(int nIndex, DWORD dwType) {
	return NULL;
}

// Draw an icon on the LCD (don't need to implement)
void vmu_draw_lcd(maple_device_t *pDev, unsigned char *pDat) {
}

//
// System
//

// Terminate the program - do not return
void arch_exit() {
	SendMessage(myWnd, WM_CLOSE, 0, 0);
	_endthreadex(0);
}

// Sleep for nDelay ms
void thd_sleep(int nDelay) {
	Sleep(nDelay);
}

// wait on a semaphore for a maximum time (ms)
// return -1 on timeout
int sem_wait_timed(semaphore_t *pSem, DWORD dwTime) {
	DWORD ret;

	ret=WaitForSingleObject(*pSem, dwTime);
	if (WAIT_TIMEOUT == ret) {
		return -1;
	} else {
		return 0;
	}
}

// Create/initialize a semaphore
semaphore_t *sem_create(int nUnknown) {
	HANDLE *pH=(HANDLE*)malloc(sizeof(HANDLE));
	*pH=CreateEvent(NULL, true, false, NULL);
	return (semaphore_t*)pH;
}

// Signal a semaphore
void sem_signal(semaphore_t *pSem) {
	SetEvent(*(HANDLE*)pSem);
}

// Destroy a semaphore
void sem_destroy(semaphore_t *pSem) {
	CloseHandle(*(HANDLE*)pSem);
	free(pSem);
}

struct sThreadWrap {
	void (*ThreadProc)(void*);
	void *pData;
};

// Start a new thread, return it's handle
unsigned int __stdcall WrapThreadStart(void *p) {
	struct sThreadWrap *pThd=(struct sThreadWrap *)p;
	pThd->ThreadProc(pThd->pData);
	delete pThd;
	return 0;
}
// Must always call thd_wait somewhere or the handle memory will leak!
kthread_t *thd_create(void (*ThreadProc)(void*), void *pUnk) {
	HANDLE *pT=(HANDLE*)malloc(sizeof(HANDLE));
	struct sThreadWrap *pThd=new struct sThreadWrap;
	pThd->ThreadProc=ThreadProc;
	pThd->pData=pUnk;
	*pT=(HANDLE)_beginthreadex(NULL, 0, WrapThreadStart, pThd, 0, NULL);
	return (kthread_t*)pT;
}

// Wait for a thread to exit
void thd_wait(kthread_t *pThread) {
	WaitForSingleObject(*(HANDLE*)pThread, INFINITE);
	CloseHandle(*(HANDLE*)pThread);
	free(pThread);
}

//
// Sound
//

// Stop playing Ogg
void sndoggvorbis_stop() {
}

// Start playing Ogg (this function should spawn a thread to do all the work)
void sndoggvorbis_start(char *pData, int nRepeat) {
	FILE *fp;
	int eof;
#if 0
	fp=fopen(pData, "rb");
	if (NULL == fp) return;

	if (ov_open(fp, &vf, NULL, 0) < 0) {
		debug("Failed to open OGG stream %s\n", pData);
		return;
	}

	{	// Dump stream info
		char **ptr=ov_comment(&vf,-1)->user_comments;
		vorbis_info *vi=ov_info(&vf,-1);
		while(*ptr){
			debug("%s\n",*ptr);
			++ptr;
		}
		debug("\nBitstream is %d channel, %ldHz\n",vi->channels,vi->rate);
		debug("\nDecoded length: %ld samples\n",
            (long)ov_pcm_total(&vf,-1));
		debug("Encoded by: %s\n\n",ov_comment(&vf,-1)->vendor);
	}

	// We need to start playing: 
#if 0
	while(!eof){
		long ret=ov_read(&vf,pcmout,sizeof(pcmout),¤t_section);
		if (ret == 0) {
			/* EOF */
			eof=1;
		} else if (ret < 0) {
		/* error in the stream.  Not a problem, just reporting it in
			case we (the app) cares.  In this case, we don't. */
		} else {
		/* we don't bother dealing with sample rate changes, etc, but
			you'll have to*/
			fwrite(pcmout,1,ret,stdout);
		}
	}
#else
	// when done
	ov_clear(&vf);
#endif
#endif
}

// Initialize the Ogg player
void sndoggvorbis_init() {
	// May not need these, but...?
	_setmode( _fileno( stdin ), _O_BINARY );
	_setmode( _fileno( stdout ), _O_BINARY );
}

// Shutdown the Ogg system
void sndoggvorbis_shutdown() {
//	ov_clear(&vf);
}

// Return non-zero if the Ogg music is playing
int  sndoggvorbis_isplaying() {
	return 0;
}

// dummy function for the stream init on DC
void snd_stream_init(void *pNull) {
}

// SFX doesn't work anyway because the format used on the DC isn't supported!
// Load a sound effect
sfxhnd_t snd_sfx_load(char *szName) {
	if (nLastSound > 15) {
		debug("Too many sound effects!\n");
		return NULL;
	}

	strcpy(&szSounds[nLastSound][0], szName);
	return &szSounds[nLastSound++][0];
}

// Play a sound effect (vol=0-255, pan=0-0x7f, 0x80, 0x81-0xff) 
void snd_sfx_play(sfxhnd_t pSnd, int nVol, int nPan) {
	//PlaySound(pSnd, NULL, SND_FILENAME | SND_NOWAIT);
}

// Stop all sound effects
void snd_sfx_stop_all() {
	PlaySound(NULL, NULL, SND_PURGE);
}

// Unload all sound effects
void snd_sfx_unload_all() {
	PlaySound(NULL, NULL, SND_PURGE);
	nLastSound=0;
}

// set the ogg music volume
void snd_stream_volume(int nVol) {
	// TODO someday if we need it
}

//
// Windows
//
int main(int argc, char *argv[]);
unsigned __stdcall MainWrapper(void *p) {
	main(0, NULL);
	return 0;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	PAINTSTRUCT ps;

	switch (uMsg) {
	
	case WM_CLOSE:
		PostQuitMessage(0);
		break;

	case WM_PAINT:
		BeginPaint(hWnd, &ps);
		BitBlt(ps.hdc, 0, 0, 640, 480, hDC, 0, 0, SRCCOPY);
		EndPaint(hWnd, &ps);
		return 0;
	}

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

int __stdcall WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR lpCmdLine, int nCmdShow) {
	// Ugh.. I have to implement the message loop and everything?
	int nRet=0;
	WNDCLASS cls;
	ATOM myClass;
	int nWidth, nHeight;
	BOOL bRet;
	MSG msg;

	// Initialize
	cls.style=CS_BYTEALIGNCLIENT | CS_OWNDC;
	cls.lpfnWndProc=WndProc;
	cls.cbClsExtra=0;
	cls.cbWndExtra=0;
	cls.hInstance=hInst;
	cls.hIcon=NULL;
	cls.hCursor=NULL;
	cls.hbrBackground=NULL;
	cls.lpszMenuName=NULL;
	cls.lpszClassName="FakeKOSClass";
	myClass=RegisterClass(&cls);
	if (myClass == 0) return 0;

	endEvent=CreateEvent(NULL, true, false, NULL);

	// Display Window
	nHeight=GetSystemMetrics(SM_CYFIXEDFRAME)*2 + GetSystemMetrics(SM_CYCAPTION);
	nWidth=GetSystemMetrics(SM_CXFIXEDFRAME)*2;
	myWnd=CreateWindow((LPCTSTR)myClass, (LPCTSTR)"Cool Herders Simulator for Windows", WS_POPUP | WS_BORDER | WS_CAPTION | WS_VISIBLE | WS_SYSMENU,
		0, 0, 640+nWidth, 480+nHeight, NULL, NULL, hInst, NULL);
	if (NULL == myWnd) return 0;

	// Run the app in another thread
	hMainThread=(HANDLE)_beginthreadex(NULL, 0, MainWrapper, NULL, 0, NULL);

	// Message loop
	while ((bRet=GetMessage(&msg, NULL, 0, 0)) != 0) {
		if (bRet == -1) break;

		TranslateMessage(&msg); 
        DispatchMessage(&msg); 
	}
	
	SetEvent(endEvent);
	WaitForSingleObject(hMainThread, 5000);		// give it 5 seconds to exit
	CloseHandle(hMainThread);

	// Cleanup
	return nRet;
}

// Debug redirect to Debugger
#undef debug
void myoutput(char *szFmt, ...) {
	va_list va;
	char buf[2048];

	va_start(va, szFmt);
	_vsnprintf(buf, 2048, szFmt, va);
	va_end(va);
	buf[2047]='\0';

	OutputDebugString(buf);
}

/* prepare the RAMdisk - not used here */	
void fs_ramdisk_init() {
}

/* Shutdown the RAMdisk - not used here */
void fs_ramdisk_shutdown() {
}

// return the size of a file opened with low level functions
uint32 fs_total(file_t f) {
	long now=_tell(f);
	long ret;
	_lseek(f, 0, SEEK_END);
	ret=_tell(f);
	_lseek(f, now, SEEK_SET);
	return ret;
}

// delete a file (meant for VMU, not used today except in cache)
int fs_unlink(char *sz) {
	return 0;	// always success
}
