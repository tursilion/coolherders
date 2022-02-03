/****************************************/
/* Cool Herders                         */
/* A demented muse by Brendan Wiese     */
/* http://starbow.org                   */
/* Programmed by Mike Brent             */
/* http://harmlesslion.com              */
/* Copyright 2000-2005 Brendan Wiese &  */
/* Mike Brent.                          */
/* font.c                               */
/****************************************/
 
// Font routines. Binky gave me two fonts - a
// big one and a small one

/* Ported to KOS 1.1.x by Dan Potter */

#include <stdio.h>
#include <malloc.h>

#ifdef WIN32
#include "kosemulation.h"
#else
#include <kos.h>
#endif

#include "sprite.h"
#include "cool.h"
#include "font.h"

extern pvr_ptr_t txr_smfont, txr_lgfont;
extern char *cpu_sprites, *cpu_lg_font;

char fontstr[]="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890!?&\".@-%:;',~";

const int LGX=20;
const int LGY=40;
const int SMX=16;
const int SMY=28;

int CursorX, CursorY;

/* Draw a string to the screen with font polys       */
/* Uses the current cursor position if x or y are -1 */
/* the magic character '+' will move 1/2 character over */
void DrawFontBackgroundZ(float z, int large, int x, int y, uint32 color, uint32 bgcolor, char *str) {
	pvr_ptr_t txr;
	int x1, y1;
	int top, left, bottom, right;
	int w, h = 0, n;
	int c;
	char *ptr;

	if (-1==x) x=CursorX;
	if (-1==y) y=CursorY;

	top=y;
	left=x;

	if (large) {
		txr=txr_lgfont;
		w=LGX;
		h=LGY;
		n=12;
	} else {
		txr=txr_smfont;
		w=SMX;
		h=SMY;
		n=15;
	}

	while (*str) {
		if (*str == '+') {
			// half-space!
			str++;
			x+=w/2;
			continue;
		}

		ptr=fontstr;
		c=0;
		while (*ptr) {
			if (*ptr == *str) {
				break;
			}
			ptr++;
			c++;
		}

		if (*ptr) {
			x1=(c%n)*w+1;
			y1=(c/n)*h;

			addPage2(txr, x, y, x1, y1, x1+w-2, y1+h-1, color, z);
		}

		str++;
		x+=w;
	}

	if (large) {
		bottom=y+h-10;
	} else {
		bottom=y+h-6;
	}
	right=x;
	if (bgcolor) {
		SortRect(z-0.5f, left, top, right, bottom, bgcolor, bgcolor);
	}

	CursorX=x;
	if (large) {
		CursorY=y+h-8;
	} else {
		CursorY=y+h-6;
	}
}

/* Draw a string to the screen with font polys, recognizing '*' for bold, and ~ for ignore */
/* Uses the current cursor position if x or y are -1 */
/* the magic character '+' will move 1/2 character over */
void DrawFontMenuZ(float z, int large, int x, int y, uint32 color, char *str) {
	pvr_ptr_t txr;
	int x1, y1;
	int w, h = 0, n;
	int c;
	char *ptr;
	uint32 nBaseColor=color;
	uint32 nBrightColor=color+INT_PACK_COLOR(0, 64, 64, 0);		// chosen to avoid wrapping

	if (-1==x) x=CursorX;
	if (-1==y) y=CursorY;

	if (large) {
		txr=txr_lgfont;
		w=LGX;
		h=LGY;
		n=12;
	} else {
		txr=txr_smfont;
		w=SMX;
		h=SMY;
		n=15;
	}

	color=nBaseColor;

	while (*str) {
		if (*str == '+') {
			// half-space!
			str++;
			x+=w/2;
			continue;
		}
		if (*str == '*') {
			// brightness change
			if (color==nBaseColor) {
				color=nBrightColor;
			} else {
				color=nBaseColor;
			}
			str++;
			continue;
		}
		if (*str == '~') {
			// ignore char
			str++;
			continue;
		}

		ptr=fontstr;
		c=0;
		while (*ptr) {
			if (*ptr == *str) {
				break;
			}
			ptr++;
			c++;
		}

		if (*ptr) {
			x1=(c%n)*w+1;
			y1=(c/n)*h;

			addPage2(txr, x, y, x1, y1, x1+w-2, y1+h-1, color, z);
		}

		str++;
		x+=w;
	}

	CursorX=x;
	if (large) {
		CursorY=y+h-8;
	} else {
		CursorY=y+h-6;
	}
}

/* Draw a string to the screen with font polys vertically  */
void VDrawFontZ(float z, int large, int x, int y, uint32 color, char *str) {
	pvr_ptr_t txr;
	int x1, y1;
	int w, h = 0, n;
	int c;
	char *ptr;

	if (large) {
		txr=txr_lgfont;
		w=LGX;
		h=LGY;
		n=12;
	} else {
		txr=txr_smfont;
		w=SMX;
		h=SMY;
		n=15;
	}

	while ((*str) && (y < 480)) {
		ptr=fontstr;
		c=0;
		while (*ptr) {
			if (*ptr == *str) {
				break;
			}
			ptr++;
			c++;
		}

		if (*ptr) {
			x1=(c%n)*w+1;
			y1=(c/n)*h;

			addPage2(txr, x, y, x1, y1, x1+w-1, y1+h-1, color, z);
		}

		str++;
		y+=h;
	}
}

/* Draw a centered string to the screen with font polys */
/* Uses the current cursor row if y is -1               */
void CenterDrawFontZ(float z, int large, int y, uint32 color, char *str) {
	int x;

	if (-1==y) y=CursorY;
	if (large) {
		x=strlen(str)*LGX;
	} else {
		x=strlen(str)*SMX;
	}
	x=(640-x)/2;

	DrawFontZ(z, large, x, y, color, str);
}

void CenterDrawFontBackgroundZ(float z, int large, int y, uint32 color, uint32 bgcolor, char *str) {
	int x;

	if (-1==y) y=CursorY;
	if (large) {
		x=strlen(str)*LGX;
	} else {
		x=strlen(str)*SMX;
	}
	x=(640-x)/2;

	DrawFontBackgroundZ(z, large, x, y, color, bgcolor, str);
}

/* break up a string at \n characters and draw it out using centerdrawfont */
void CenterDrawFontBreaksZ(float z, int large, int y, uint32 color, char *str) {
	char *p1, *p2;
	char buf[40];

	p1=str;

	do {
		p2=strchr(p1, '\n');
		if (NULL == p2) {
			p2=strchr(p1, '\0');
		}
		if (NULL == p2) return;

		memset(buf, 0, 40);
		memcpy(buf, p1, min(40,p2-p1));
		buf[39]='\0';

		CenterDrawFontZ(z, large, y, color, buf);

		p1=p2;
		if (*p1) p1++;
		y=-1;
	} while (*p1);
}

/* break up a string at \n characters and draw it out using centerdrawfontbackground */
void CenterDrawFontBackgroundBreaksZ(float z, int minwidth, int y, uint32 color, uint32 bgcolor, char *str) {
	char *p1, *p2;
	char buf[41];
	int nMax, nLen, nLines=0;

	// calculate longest string
	nMax=0;		// one space padding on each side
	p1=str;
	do {
		p2=strchr(p1, '\n');
		if (NULL == p2) {
			p2=strchr(p1, '\0');
		}
		if (NULL == p2) return;

		memset(buf, 0, 40);
		memcpy(buf, p1, min(40,p2-p1));
		buf[39]='\0';

		if (strlen(buf) > nMax) nMax=strlen(buf);

		p1=p2;
		if (*p1) p1++;
	} while (*p1);

	// Now we're ready to draw it with a full box around each line
	if (nMax < minwidth) nMax=minwidth;
	nMax+=2;
	if (nMax > 40) nMax=40;
	memset(buf, ' ', 40);
	buf[nMax]='\0';
	CenterDrawFontBackgroundZ(z, 0, y, color, bgcolor, buf);
	y=-1;

	p1=str;
	do {
		p2=strchr(p1, '\n');
		if (NULL == p2) {
			p2=strchr(p1, '\0');
		}
		if (NULL == p2) return;

		memset(buf, ' ', 40);
		nLen=min(nMax,p2-p1);
		memcpy(buf+(nMax-nLen)/2, p1, nLen);
		buf[nMax]='\0';

		CenterDrawFontBackgroundZ(z, 0, y, color, bgcolor, buf);
		nLines++;

		p1=p2;
		if (*p1) p1++;
	} while (*p1);

	memset(buf, ' ', 40);
	buf[nMax]='\0';
	CenterDrawFontBackgroundZ(z, 0, y, color, bgcolor, buf);
	if ((minwidth>0)&&(nLines<2)) {
		// always draw at least 2 lines if nMax is set
		CenterDrawFontBackgroundZ(z, 0, y, color, bgcolor, buf);
	}
}

/* Draw a string to a texture by blitting data */
/* NOTE: font textures are    256x256x2    */
/*       dest texture must be 512x512x2    */
/* The math won't work right otherwise ;)  */
/* Does not use or update cursor position  */
/* (so don't pass -1!)                     */
/* 'Large' is not used - always large font */
void DrawFontTxr(pvr_ptr_t txr_out, int large, int x, int y, char *str) {
/* Won't work in Windows port */
#ifndef WIN32
	pvr_ptr_t pSrc, pDest;
	char *ftxr;
	int idx;
	char *ptr;
	int c, x1, y1;
	int w, h, n;

	pDest=(y<<10)+(x<<1)+txr_out;

	ftxr=cpu_lg_font;
	w=LGX;
	h=LGY;
	n=12;

	while (*str) {
		ptr=fontstr;
		c=0;
		while (*ptr) {
			if (*ptr == *str) {
				break;
			}
			ptr++;
			c++;
		}

		if (*ptr) {
			x1=(c%n)*w;
			y1=(c/n)*h;

			pSrc=(y1<<9)+(x1<<1)+ftxr;
			for (idx=0; idx<h; idx++) {
				memcpy(pDest+(idx<<10), pSrc+(idx<<9), w<<1);
			}
		} else {
			for (idx=0; idx<h; idx++) {
				memset(pDest+(idx<<10), 0, w<<1);
			}
		}
		pDest+=w<<1;
		str++;
	}
#endif
}

/* Erase a string to a texture by blitting data */
/* NOTE: font textures are    256x256x2    */
/*       dest texture must be 512x512x2    */
/* The math won't work right otherwise ;)  */
/* Does not use or update cursor position  */
/* (so don't pass -1!)                     */
void EraseFontTxr(pvr_ptr_t txr_out, int large, int x, int y, int nCnt) {
/* Won't work in Windows port */
#ifndef WIN32
	pvr_ptr_t pDest;
	int idx, idx2;
	int w, h, n;

	pDest=(y<<10)+(x<<1)+txr_out;

	if (large) {
		w=LGX;
		h=LGY;
		n=12;
	} else {
		w=SMX;
		h=SMY;
		n=15;
	}

	for (idx2=0; idx2<nCnt; idx2++) {
		for (idx=0; idx<h; idx++) {
			memset(pDest+(idx<<10), 0, w<<1);
		}
		pDest+=w<<1;
	}
#endif
}
