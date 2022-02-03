/****************************************/
/* Cool Herders                         */
/* http://harmlesslion.com              */
/* Copyright 2011 HarmlessLion LLC      */
/* font.c                               */
/****************************************/

#include <string.h>
void debug(char *str, ...);

extern OSOwnerInfo OwnerInformation;

// handlers for the top screen font

// list of palettes for each character - Nitro Character sets this
// up. To use, shift this into the top nibble of the 16-bit value
// and OR with the character index itself
char PalTable[1024];
int nNextY=0;		// tracks the Y coordinate for easier writing

// This is a list of UTF codes > xxx that we support, and what to map them to
// UTF,'ASCII', repeat
static unsigned short myMap[] = {
//	128,144,160,176,192,208,224,240

	0x007e,152,

	0x00a1,161,
	0x00a2,162,
	0x00a3,163,
	
	0x00a9,169,
	
	0x00ae,174,
	
	0x00b0,176,
	0x00b1,177,
	
	0x00b7,183,
	
	0x00bf,191,
	0x00c0,192,
	0x00c1,193,
	0x00c2,194,
	0x00c3,195,
	0x00c4,196,
	0x00c5,197,
	0x00c6,198,
	0x00c7,199,
	0x00c8,200,
	0x00c9,201,
	0x00ca,202,
	0x00cb,203,
	0x00cc,204,
	0x00cd,205,
	0x00ce,206,
	0x00cf,207,
	0x00d0,208,
	0x00d1,209,
	0x00d2,210,
	0x00d3,211,
	0x00d4,212,
	0x00d5,213,
	0x00d6,214,
	0x00d7,215,
	0x00d8,216,
	0x00d9,217,
	0x00da,218,
	0x00db,219,
	0x00dc,220,
	0x00dd,221,
//  0x00de,not supported
	0x00df,223,
	0x00e0,224,
	0x00e1,225,
	0x00e2,226,
	0x00e3,227,	// not supported by NDS?
	0x00e4,228,
	0x00e5,229,
	0x00e6,230,
	0x00e7,231,
	0x00e8,232,
	0x00e9,233,
	0x00ea,234,
	0x00eb,235,
	0x00ec,236,
	0x00ed,237,
	0x00ee,238,
	0x00ef,239,
	0x00f1,241,
	0x00f2,242,
	0x00f3,243,
	0x00f4,244,
	0x00f5,245,
	0x00f6,246,
	0x00f7,247,
	0x00f8,248,
	0x00f9,249,
	0x00fa,250,
	0x00fb,251,
	0x00fc,252,
	0x00fd,253,
//	0x00fe,not supported
// 	0x00ff,not supported	

	0x0152,140,
	0x0153,156,
	
	0x20ac,128,
	0x201c,34,
	0x201d,34,
	0x2122,153,
	0x266d,'b',
	0x266f,'#',

// DS has the following
// musical note (0x1d160?), no replacement
// infinity, no replacement
// Three horizontal dots ... in the middle of the cell (not elipsis?), no replacement
// several lines, no replacement
// dot product (2e2b) no replacement
// arrows (two sets), no replacement
// graphics chars, no replacement
// japanese characters, no replacement

	0,0
};

// reparses the UTF-16 username into our 8-bit font. If we can't
// do it, then it creates a name based on the old name
// Returns number of ASCII-ish characters in new name.
int ReparseName(unsigned char *p, int nLen) {
	// we support characters ASCII 32-125
	// The range 0xD800-0xDFFF is used for supplemental planes and so we don't support them
	// The UTF seems to be in little-endian format
	char buf[4];

	unsigned char *pOut = p;
	unsigned char *pIn = p;
	
	if (nLen > 10) nLen = 10;
	
	// save off the first 4 bytes in case we need them later
	memcpy(buf, p, 4);
	
	// we're going to change *p in place, since it should be smaller
	// we also nul-terminate
	while (nLen) {
		unsigned short nVal = (*pIn) + ((*(pIn+1))<<8);
		unsigned char nNew = 0;
		
		pIn+=2;
		
		if (nVal == 0) {
			break;
		}
		
		if ((nVal >= 32) && (nVal <= 125)) {
			// already ASCII compatible
			nNew = (unsigned char)(nVal&0xff);
		} else {
			int idx=0;
			while (myMap[idx] != 0) {
				if (myMap[idx] == nVal) {
					nNew = (unsigned char)(myMap[idx+1]&0xff);
					break;
				}
				idx+=2;
			}
		}
		if (nNew == 0) {
			// create a generic name from the username, as hex (10 characters max, plus NUL)
			// technically since I an going from UTF16 to ASCII-ish, I have room for 21,
			// but that won't fit the usual plan!
			sprintf((char*)p, "Herd%02X%02X%02X", (buf[0]*OwnerInformation.favoriteColor)&0xff, (buf[1]*OwnerInformation.favoriteColor)&0xff, (buf[2]*OwnerInformation.favoriteColor)&0xff);
			debug("Couldn't find match for unicode 0x%04X, generating name.", nVal);
			return 10;		// 10 chars exactly
		}
		*(pOut++) = nNew;
		nLen--;
	}
	*(pOut++) = '\0';		// NUL terminate
	// the name MUST be an even number of characters to transfer properly, so add an extra NUL if needed
	if (strlen((char*)p)&1) {
		*(pOut++) = '\0';
	}
	debug("Name conversion succeeded.");
	return pOut-p;
}

// this function works with only the characters on the left side
// of the image, ASCII 32-255
// Produces a shadowed text with a second text screen, slightly offset
void WriteFont2D(int r, int c, char *psz) {
	u16* pOut;
	
	if (r==-1) r=nNextY;
	pOut=(u16*)((u32)G2_GetBG1ScrPtr() + (((r<<5)+c)<<1));
	
	while (*psz) {
		u16 val = (*(psz++))-' ';
		if (val > 127) val+=32;		// there are two rows of gap to ignore

		// fixup VAL - chars are 2 rows tall plus there
		// are only 16 in a row, the other half is graphics
		// So:
		val=((val>>4)<<6) + (val&0xf);		// ((val/16)*64)+(val%16)

		*(pOut++) = val | (PalTable[val]<<12);
		
		val+=32;
		*(pOut+31) = val | (PalTable[val]<<12);
	}
	
	nNextY=r+2;
}

// center around r,c
void CenterWriteFont2D(int r, int c, char *psz) {
	WriteFont2D(r, (int)(c-(strlen(psz)>>1)), psz);
}

// center around r,c, support line breaks
void CenterWriteFontBreak2D(int r, int c, char *psz) {
	char szBuf[32];
	int i;
	
	i=0;
	while (*psz) {
		if (*psz == '\n') {
			szBuf[i]='\0';
			WriteFont2D(r, c-(i>>1), szBuf);
			r+=2;
			i=0;
			psz++;
		} else {
			szBuf[i++]=*(psz++);
		}
	}
	if (i>0) {
		szBuf[i]='\0';
		WriteFont2D(r, c-(i>>1), szBuf);
	}
}

// This clears the 2d text screens - note it clears the WHOLE
// screen a little slowly, so it would be faster to just
// erase text if it's a single line. Also resets the offset.
void Clear2D() {
   	// clear 2D memory so there's no garbage onscreen
   	MI_CpuClear8(G2_GetBG1ScrPtr(), 2048);
   	G2_SetBG1Offset(0,0);
   	G2_SetBG2Offset(-1,-1);
}

// sets the 2d screen offset to x,y
void SetOffset2D(int x, int y) {
	G2_SetBG1Offset(x,y);
	G2_SetBG2Offset(x-1,y-1);
}

// this function works with only the characters on the right side
// of the image, ASCII 0-255 (note: no offset for space!)
// Produces a shadowed text with a second text screen, slightly offset
void WriteGfx2D(int r, int c, char *psz) {
	u16* pOut;
	
	if (r==-1) r=nNextY;
	pOut=(u16*)((u32)G2_GetBG1ScrPtr() + (((r<<5)+c)<<1));
	
	while (*psz) {
		u16 val = (*(psz++));

		// fixup VAL - chars are 2 rows tall plus there
		// are only 16 in a row, the other half is graphics
		// So:
		val=((val>>4)<<6) + (val&0xf) + 16;		// ((val/16)*64)+(val%16)+16

		*(pOut++) = val | (PalTable[val]<<12);
		
		val+=32;
		*(pOut+31) = val | (PalTable[val]<<12);
	}
	
	nNextY=r+2;
}
 
// draws "ready..." in the current charset - 14x4 characters starting at 0
// in the gfx block
void DrawReady() {
	u16 *pOut;
	int r,c;
	u16 val;
	
	pOut=(u16*)((u32)G2_GetBG1ScrPtr() + (((10<<5)+9)<<1));
	val=16;
	
	for (r=0; r<4; r++) {
		for (c=0; c<14; c++) {
			*(pOut++)=val|(PalTable[val]<<12);
			val++;
		}
		pOut+=32-c;
		val+=32-c;
	}
}

// draws GO! in the current charset - 16x8 characters starting at 0
// in the gfx block
void DrawGo() {
	u16 *pOut;
	int r,c;
	u16 val;
	
	pOut=(u16*)((u32)G2_GetBG1ScrPtr() + (((8<<5)+8)<<1));
	val=144;
	
	for (r=0; r<8; r++) {
		for (c=0; c<16; c++) {
			*(pOut++)=val|(PalTable[val]<<12);
			val++;
		}
		pOut+=32-c;
		val+=32-c;
	}
}
