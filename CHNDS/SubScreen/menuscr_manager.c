// NOTE!  This entire file ASSUMES that the CHARBASE for BG0, BG1, BG2, and BG3 is and will always be...
// THE SAME!!!!!  If they are different, the VRAM allocator is going to seriously fscking break, and
// I am NOT PICKING UP THE PIECES!  No, this is not a gigantic limitation.  It does prevent them from
// using more than 1 block of VRAM (32 or 16k), but CH is not intended to exceed that.

#include <types.h>
#include <ctype.h>
#include <string.h>
#include "menuscr_manager.h"
#include "mapscr_manager.h"

#include "Tileset_A.h"
#include "Tileset_B.h"
#include "menufont.h"
#include "Menu_BG.h"
#include "kosemulation.h"
#include "sprite.h"
#include "sound.h"
#include "menu.h"
#include "vmu_logo.h"
#include "cool.h"
#include "control.h"

#ifndef min
	#define min(a,b) ((a)<(b)?(a):(b))
#endif

extern int nFrames;
extern int isMultiplayer;
extern volatile unsigned int myJiffies;

int nLastInitMenu = -1;

char *szLoadText="";

// Draws 8x16 text at the given location
const MenuScr_FontInfo sConvertArray8[] = {
	{ 'A', 0x00, 0x10},
	{ 'B', 0x01, 0x11},
	{ 'C', 0x02, 0x12},
	{ 'D', 0x03, 0x11},
	{ 'E', 0x04, 0x13},
	{ 'F', 0x04, 0x14},
	{ 'G', 0x05, 0x15},
	{ 'H', 0x06, 0x16},
	{ 'I', 0x07, 0x17},
	{ 'J', 0x08, 0x18},
	{ 'K', 0x09, 0x10},
	{ 'L', 0x0A, 0x19},
	{ 'M', 0x0B, 0x1A},
	{ 'N', 0x0C, 0x10},
	{ 'O', 0x0D, 0x1B},
	{ 'P', 0x0E, 0x14},
	{ 'Q', 0x0D, 0x1C},
	{ 'R', 0x0E, 0x10},
	{ 'S', 0x0F, 0x1D},
	{ 'T', 0x20, 0x1E},
	{ 'U', 0x21, 0x1B},
	{ 'V', 0x22, 0x1F},
	{ 'W', 0x23, 0x30},
	{ 'X', 0x24, 0x31},
	{ 'Y', 0x25, 0x18},
	{ 'Z', 0x26, 0x32},
	{ 'a', 0x27, 0x33},
	{ 'b', 0x28, 0x11},
	{ 'c', 0x29, 0x12},
	{ 'd', 0x2a, 0x33},
	{ 'e', 0x2b, 0x12},
	{ 'f', 0x2c, 0x1e},
	{ 'g', 0x2d, 0x34},
	{ 'h', 0x2e, 0x10},
	{ 'i', 0x2f, 0x17},
	{ 'j', 0x2f, 0x35},
	{ 'k', 0x40, 0x36},
	{ 'l', 0x41, 0x17},
	{ 'm', 0x42, 0x37},
	{ 'n', 0x43, 0x10},
	{ 'o', 0x44, 0x1b},
	{ 'p', 0x43, 0x38},
	{ 'q', 0x45, 0x39},
	{ 'r', 0x46, 0x14},
	{ 's', 0x47, 0x1d},
	{ 't', 0x48, 0x3a},
	{ 'u', 0x49, 0x33},
	{ 'v', 0x4a, 0x3b},
	{ 'w', 0x4b, 0x30},
	{ 'x', 0x4c, 0x31},
	{ 'y', 0x49, 0x34},
	{ 'z', 0x4d, 0x32},
	{ '1', 0x4e, 0x17},
	{ '2', 0x4f, 0x19},
	{ '3', 0x50, 0x18},
	{ '4', 0x51, 0x3c},
	{ '5', 0x52, 0x18},
	{ '6', 0x53, 0x1b},
	{ '7', 0x54, 0x1e},
	{ '8', 0x55, 0x1b},
	{ '9', 0x56, 0x18},
	{ '0', 0x0d, 0x1b},
	{ '!', 0x57, 0x3d},
	{ '?', 0x58, 0x3d},
	{ '.', 0xff, 0x3d},
	{ ',', 0xff, 0x3e},
	{ ':', 0x3f, 0x3d},
	{ ';', 0x3f, 0x3e},
	{ '\"', 0x59, 0xff},		//" Do not remove this line. (fix syntax highlighting)
	{ '\'', 0x5a, 0xff},
	{ '-', 0x5b, 0xff},
	{ '@', 0xfb, 0xff}, // st
	{ '#', 0xfc, 0xff},	// no
	{ '$', 0xfd, 0xff},	// ro
	{ '%', 0xfe, 0xff},	// th
	{ '^', 0xf0, 0xf1},	// 'x' (times)
	{ '*', 0x5e, 0x5f},	// heart
};

// Draws 16x16 text at the given location
const MenuScr_FontInfo16 sConvertArray16[] = {
	{ 'A', 0x00, 0x01, 0x10, 0x11},
	{ 'B', 0x02, 0x03, 0x12, 0x13},
	{ 'C', 0x04, 0x05, 0x14, 0x15},
	{ 'D', 0x06, 0x07, 0x16, 0x17},
	{ 'E', 0x08, 0x09, 0x18, 0x19},
	{ 'F', 0x0A, 0x0B, 0x1A, 0x1B},
	{ 'G', 0x0C, 0x0D, 0x1C, 0x1D},
	{ 'H', 0x0E, 0x0F, 0x1E, 0x1F},
	{ 'I', 0x20, 0x21, 0x30, 0x31},
	{ 'J', 0x22, 0x23, 0x32, 0x33},
	{ 'K', 0x24, 0x25, 0x34, 0x35},
	{ 'L', 0x26, 0x27, 0x36, 0x37},
	{ 'M', 0x28, 0x29, 0x38, 0x39},
	{ 'N', 0x2A, 0x2B, 0x3A, 0x3B},
	{ 'O', 0x2C, 0x2D, 0x3C, 0x3D},
	{ 'P', 0x2E, 0x2F, 0x3E, 0x3F},
	{ 'Q', 0x40, 0x41, 0x50, 0x51},
	{ 'R', 0x42, 0x43, 0x52, 0x53},
	{ 'S', 0x44, 0x45, 0x54, 0x55},
	{ 'T', 0x46, 0x47, 0x56, 0x57},
	{ 'U', 0x48, 0x49, 0x58, 0x59},
	{ 'V', 0x4A, 0x4B, 0x5A, 0x5B},
	{ 'W', 0x4C, 0x4D, 0x5C, 0x5D},
	{ 'X', 0x4E, 0x4F, 0x5E, 0x5F},
	{ 'Y', 0x60, 0x61, 0x70, 0x71},
	{ 'Z', 0x62, 0x63, 0x72, 0x73},
	{ 'a', 0x80, 0x81, 0x90, 0x91},
	{ 'b', 0x82, 0x83, 0x92, 0x93},
	{ 'c', 0x84, 0x85, 0x94, 0x95},
	{ 'd', 0x86, 0x87, 0x96, 0x97},
	{ 'e', 0x88, 0x89, 0x98, 0x99},
	{ 'f', 0x8A, 0x8B, 0x9A, 0x9B},
	{ 'g', 0x8C, 0x8D, 0x9C, 0x9D},
	{ 'h', 0x8E, 0x8F, 0x9E, 0x9F},
	{ 'i', 0xA0, 0xA1, 0xB0, 0xB1},
	{ 'j', 0xA2, 0xA3, 0xB2, 0xB3},
	{ 'k', 0xA4, 0xA5, 0xB4, 0xB5},
	{ 'l', 0xA6, 0xA7, 0xB6, 0xB7},
	{ 'm', 0xA8, 0xA9, 0xB8, 0xB9},
	{ 'n', 0xAA, 0xAB, 0xBA, 0xBB},
	{ 'o', 0xAC, 0xAD, 0xBC, 0xBD},
	{ 'p', 0xAE, 0xAF, 0xBE, 0xBF},
	{ 'q', 0xC0, 0xC1, 0xD0, 0xD1},
	{ 'r', 0xC2, 0xC3, 0xD2, 0xD3},
	{ 's', 0xC4, 0xC5, 0xD4, 0xD5},
	{ 't', 0xC6, 0xC7, 0xD6, 0xD7},
	{ 'u', 0xC8, 0xC9, 0xD8, 0xD9},
	{ 'v', 0xCA, 0xCB, 0xDA, 0xDB},
	{ 'w', 0xCC, 0xCD, 0xDC, 0xDD},
	{ 'x', 0xCE, 0xCF, 0xDE, 0xDF},
	{ 'y', 0xE0, 0xE1, 0xF0, 0xF1},
	{ 'z', 0xE2, 0xE3, 0xF2, 0xF3},
};


extern int nFrames;
extern GXOamAttr Sprites[128];		// from mapscr_manager, so we can clear the sprites

const static u32 c_IllegalTile = 0xFFFFFFFF;

static u32 nNextVramTileUsed;
static u32 nNextPaletteOffset;

static MenuScr_TileSet TilesetA;
static MenuScr_TileSet TilesetBa;
static MenuScr_TileSet TilesetBb;
static MenuScr_TileSet TilesetBc;
static MenuScr_TileSet TilesetD;

static unsigned short *pWorkingMapA;

static u16 BkgTiles [32][32];
static u16 TextTiles [32][32];
static u16 BubbleTiles [32][32];
static u16 SelectedBubbleTiles [32][32];

static int bMenuIsRendering;
static MenuScr_MenuItem MenuItems[8];
static int nSelectedButton;
static int nPreferredDefault=-1;

static int nFramesSinceKeypress;
static int nWaitingTouchOff;

static int nInMenu = 0;

enum {	// possible options
	NO_OPTIONS,
	OPTIONS,
	SYSINFO,
	WORLD_SELECT,
	PLAYER_SELECT,
	BATTLE_OPTIONS,
	MUSIC_OPTIONS
};

static int nInOptions = NO_OPTIONS;
static int nSelOption = 0;
static int nMaxSelOption = 0;
static int nBorderX;
static int nBorderY;

MenuScr_Options MenuScr_CurrentOptions;

// MenuScr_InitTileset: Takes a tileset, and a pointer to the memory and palette blocks
// Prepares a tileset for loading from main memory to VRAM, but does not allocate or copy
// any VRAM.  Do NOT call this on a tileset if that tileset has VRAM allocated to it.
void MenuScr_InitTileset(MenuScr_TileSet* i_pTileSet, void* i_pbMemory, void* i_pbPalette) {
	i_pTileSet->nVramTile = c_IllegalTile;
	i_pTileSet->nPalette = 0;
	i_pTileSet->pbMemoryAddress = i_pbMemory;
	i_pTileSet->pbPaletteAddress = i_pbPalette;	
}

// MenuScr_LoadTileSet: Takes a tileset, and the size of the tiles (in tiles) and palette (in
// blocks of 16 colors).
// Actually loads a tileset to VRAM and notes down where the information was placed
// These numbers (nVramTile, nPalette) can be used directly to NitroSDK calls
// NOTE!  256 color tiles count as two tiles!  If you want to use these, divide the tile
// number by 2 before passing to a 256 color BG.  256 color tiles must come first or be aligned
// by 2.
void MenuScr_LoadTileset(MenuScr_TileSet* i_pTileSet, u32 i_nTileSize, u32 i_nPaletteSize) {
	GXS_LoadBG0Char(i_pTileSet->pbMemoryAddress, nNextVramTileUsed*32, i_nTileSize*32);
	i_pTileSet->nVramTile = nNextVramTileUsed;
	nNextVramTileUsed += i_nTileSize;
	
	GXS_LoadBGPltt(i_pTileSet->pbPaletteAddress, (unsigned long)(nNextPaletteOffset*16)*2, (i_nPaletteSize*16)*2);
	i_pTileSet->nPalette = nNextPaletteOffset;
	nNextPaletteOffset += i_nPaletteSize;
}

// MenuScr_ResetSubScr(void)
// Resets all the I/O registers needed to cause the subscreen to display
// the map screen.  If VRAM has not been manipulated, this is all that
// needs to be called to restore graphics after other uses of the map screen.
void MenuScr_ResetSubScr(void) {
	// This initializes the hardware for the needs of the information screen
	// It does not need to be called again unless the 2D B engine has been altered
	GX_SetBankForSubBG(GX_VRAM_SUB_BG_32_H);
	GX_SetBankForSubOBJ(GX_VRAM_SUB_OBJ_16_I);

	GXS_SetGraphicsMode(GX_BGMODE_0);

	GXS_SetVisiblePlane(GX_PLANEMASK_BG0 | GX_PLANEMASK_BG1 | GX_PLANEMASK_BG2 | GX_PLANEMASK_BG3);
	GXS_SetOBJVRamModeChar(GX_OBJVRAMMODE_CHAR_1D_64K);
	G2S_SetBlendBrightness(GX_BLEND_PLANEMASK_BG0, 0);	
	GXS_SetVisibleWnd(GX_WNDMASK_NONE);

	G2S_SetBG0Offset(0, 0);
	G2S_SetBG1Offset(0, 0);
	G2S_SetBG2Offset(0, 0);
	G2S_SetBG3Offset(-4, -4);

	G2S_SetBG1Priority(0); // Text on top
	G2S_SetBG3Priority(1); // Then the selected button
	G2S_SetBG2Priority(2); // Then any unselected buttons
	G2S_SetBG0Priority(3); // Tiled background on bottom
}

#define PLOTTILEBA(array, xpos, ypos, tile) \
	(array[ypos][xpos] = (u16)((pWorkingMapA[tile] + TilesetBa.nVramTile)))
#define PLOTTILEBAOPT(array, xpos, ypos, tile) \
	(array[ypos][xpos] = (u16)(((pWorkingMapA[tile] + TilesetBa.nVramTile) & 0x0FFF) | 0x8000))
#define PLOTTILEBC(array, xpos, ypos, tile) \
	(array[ypos][xpos] = (u16)((Tileset_B_V6c_Map[tile] + TilesetBc.nVramTile)))
#define PLOTTILED(array, xpos, ypos, tile) \
	(array[ypos][xpos] = (u16)((menufont_Map[tile] + TilesetD.nVramTile)))


// Takes a string to draw, and centers it on the subscreen (doesn't pay attention
// to clouds, so make sure that they are correct). Calls into MenuScr_DrawTextXY816()
void MenuScr_DrawCenteredTextXY816(char *pszText) {
	char *p1, *p2;
	char buf[40];
	int nLines=0;
	int y,x;
	
	p1=pszText;
	do {
		nLines++;
		p2=strchr(p1, '\n');
		p1=p2+1;
	} while (NULL != p2);
	y=12-(nLines);	// not nLines/2 because characters are 2 rows tall, this gives us half-offsets

	p1=pszText;
	do {
		p2=strchr(p1, '\n');
		if (NULL == p2) {
			p2=strchr(p1, '\0');
		}
		if (NULL == p2) return;

		memset(buf, '\0', 40);
		memcpy(buf, p1, (unsigned)min(40,p2-p1));
		buf[39]='\0';
		
		x=(32-strlen(buf))/2;
		MenuScr_DrawTextXY816(x, y, buf, 1);

		p1=p2;
		if (*p1) p1++;
		y+=2;
	} while (*p1);
}

// MapScr_DrawTextXY816: Takes an x and y location where the string should start, and a string
// Converts ASCII to font, and renders an 8x16 font (stored in a 16x16 tile array).
void MenuScr_DrawTextXY816(u8 x, u8 y, char* pszText, int bUnselOption)
{
	u16 nCharCode;
	char* pszCurChar = pszText;
	
	while (NULL != *pszCurChar) 
	{
		for (nCharCode = 0; nCharCode < sizeof(sConvertArray8)/sizeof(sConvertArray8[1]); nCharCode++) 
		{
			if (*pszCurChar == sConvertArray8[nCharCode].cCharName) 
			{
				if (bUnselOption) 
				{
					PLOTTILEBAOPT(TextTiles, x, y, sConvertArray8[nCharCode].uUpper);
					PLOTTILEBAOPT(TextTiles, x, y+1, sConvertArray8[nCharCode].uLower);
				}
				else 
				{
					PLOTTILEBA(TextTiles, x, y, sConvertArray8[nCharCode].uUpper);
					PLOTTILEBA(TextTiles, x, y+1, sConvertArray8[nCharCode].uLower);
				}
				break;
			}
		}
		pszCurChar++;
		x++;
	}
}

// hacky little function to get the text buffer onto the screen
// without any other functionality (only called externally). it also
// copies to BG2 for shadow (used by the high score code)
void MenuScr_FlushTextOnly() {
	DC_FlushRange(TextTiles, 1024*2);
	GXS_LoadBG1Scr(TextTiles, 0, 1024*2);
}

// draws a single line of text, centered
void MenuScr_DrawCenteredTextXY1616(int x, int y, char *pszText, int bForceUpper) {
	MenuScr_DrawTextXY1616(x - (strlen(pszText)) + 2, y + 1, pszText, bForceUpper);
}

// MenuScr_DrawTextXY1616: Takes an x and y location where the string should start, and a string
// Converts ASCII to font, and renders a 16x16 font (stored in a 16x16 tile array).
void MenuScr_DrawTextXY1616(int x, int y, char* pszText, int bForceUpper)
{
	u16 nCharCode;
	char* pszCurChar = pszText;
	char cTempChar;
	
	while (NULL != *pszCurChar) 
	{
		for (nCharCode = 0; nCharCode < sizeof(sConvertArray16)/sizeof(sConvertArray16[1]); nCharCode++) 
		{
			if (bForceUpper) 
			{
				cTempChar = toupper(*pszCurChar);
			}
			else 
			{
				cTempChar = *pszCurChar;
			}
			if (cTempChar == sConvertArray16[nCharCode].cCharName) 
			{
				PLOTTILED(TextTiles, x, y, sConvertArray16[nCharCode].uUpperLeft);
				PLOTTILED(TextTiles, x, y+1, sConvertArray16[nCharCode].uLowerLeft);
				PLOTTILED(TextTiles, x+1, y, sConvertArray16[nCharCode].uUpperRight);
				PLOTTILED(TextTiles, x+1, y+1, sConvertArray16[nCharCode].uLowerRight);
				break;
			}
		}
		pszCurChar++;
		x += 2;
	}
}

// MenuScr_ResetVRAM(void)
// Resets all allocations within VRAM, and then reloads it.  If main memory has not been touched
// since the map screen was initialized, it is safe to call this if you manipulated VRAM.  This
// will rewrite everything.  However, it has maintained direct pointers within any structures
// in the main program.  If those have moved....zzhhhaakkkkkkk!
void MenuScr_ResetVRAM(void) {
	u32 nScreenTables;
	u16 nXBkg, nYBkg;
	
	MenuScr_ResetSubScr();

	nNextVramTileUsed = 0;
	nNextPaletteOffset = 0;
	
	MenuScr_LoadTileset(&TilesetA, 32, 4); // 16 tiles, and these are 256 color ones, so doubled. 
											// Plus, 4 16 color palettes to give 64 colors
	TilesetA.nVramTile /= 2; // And that divide by 2 fixup I talked about.
	
	MenuScr_LoadTileset(&TilesetBa, 214, 1); // 214 tiles for the main tiles, all palettes loaded for now
	MenuScr_LoadTileset(&TilesetBb, 100, 1); // 100 tiles for the main tiles, all palettes loaded for now
	MenuScr_LoadTileset(&TilesetBc, 10, 1); // 10 tiles for the main tiles, all palettes loaded for now
	MenuScr_LoadTileset(&TilesetD, 197, 1); // 197 tiles for cool font, all palettes loaded
	debug("Tileset D loaded with palette %d\n", TilesetD.nPalette);
	
	nScreenTables=((nNextVramTileUsed * 32) / 2048) + 1;
	
	debug("First available screen table is at: %d\n", nScreenTables);
	
	G2S_SetBG0Control(GX_BG_SCRSIZE_TEXT_256x256, GX_BG_COLORMODE_256, (GXBGScrBase)nScreenTables,
		GX_BG_CHARBASE_0x00000, GX_BG_EXTPLTT_01);
	G2S_SetBG1Control(GX_BG_SCRSIZE_TEXT_256x256, GX_BG_COLORMODE_16, (GXBGScrBase)(nScreenTables+1),
		GX_BG_CHARBASE_0x00000, GX_BG_EXTPLTT_01);
	G2S_SetBG2ControlText(GX_BG_SCRSIZE_TEXT_256x256, GX_BG_COLORMODE_16, (GXBGScrBase)(nScreenTables + 2),
		GX_BG_CHARBASE_0x00000);
	G2S_SetBG3ControlText(GX_BG_SCRSIZE_TEXT_256x256, GX_BG_COLORMODE_16, (GXBGScrBase)(nScreenTables + 3),
		GX_BG_CHARBASE_0x00000);
	
	for (nYBkg = 0; nYBkg < 32; nYBkg++) {
		for (nXBkg = 0; nXBkg < 32; nXBkg++){
			BkgTiles[nYBkg][nXBkg] = (u16)((nXBkg % 4) + (4 * (nYBkg % 4)));
			PLOTTILEBA(TextTiles, nXBkg, nYBkg, 0xff);
			PLOTTILEBA(BubbleTiles, nXBkg, nYBkg, 0xff);
			PLOTTILEBA(SelectedBubbleTiles, nXBkg, nYBkg, 0xff);
		}
	}

	// reset sprites
	DC_FlushRange(Sprites, sizeof(Sprites));
	MI_DmaFill32(MAPSCR_DMA_CHANNEL, Sprites, 192, sizeof(Sprites)); // Clear to an 'idle pattern'.  I think.
	DC_FlushRange(Sprites, sizeof(Sprites));
	GXS_LoadOAM(Sprites, 0, sizeof(Sprites));
	
	GXS_LoadBG0Scr(BkgTiles, 0, 1024*2);
	GXS_LoadBG1Scr(TextTiles, 0, 1024*2);
	GXS_LoadBG2Scr(BubbleTiles, 0, 1024*2);
	GXS_LoadBG3Scr(SelectedBubbleTiles, 0, 1024*2);
}


// MenuScr_DrawBubble
// Draws a bubble on the main bubble layer, with the given x, y, w, h
// i_x : input for X
// i_y : input for Y
// i_w : input for width
// i_h : input for height
void MenuScr_DrawBubble(int i_x, int i_y, int i_w, int i_h) 
{
	int xline, yline;
	
	// Draw the first line
	PLOTTILEBA(BubbleTiles, i_x, i_y, 0xBC);
	for (xline = 1; xline < i_w; xline++) 
	{
		// If we're drawing an odd column number, use the left hand middle piece
		// If we're drawing an evin column number, use the right hand middle piece
		PLOTTILEBA(BubbleTiles, i_x + xline, i_y, (xline & 0x01) ? 0xBD : 0xBE);
	}
	PLOTTILEBA(BubbleTiles, i_x + i_w, i_y, 0xBF);
	
	for (yline = 1; yline < i_h; yline++) 
	{
		PLOTTILEBA(BubbleTiles, i_x, i_y + yline, (yline & 0x01) ? 0xCC : 0xDC);
		for (xline = 1; xline < i_w; xline++) 
		{
			PLOTTILEBA(BubbleTiles, i_x + xline, i_y + yline, 0xBB);
		}
		PLOTTILEBA(BubbleTiles, i_x + i_w, i_y + yline, ((yline & 0x01) ? 0xCF : 0xDF));
	}
	
	// Draw the last line
	PLOTTILEBA(BubbleTiles, i_x, i_y + i_h, 0xEC);
	for (xline = 1; xline < i_w; xline++) 
	{
		// If we're drawing an odd column number, use the left hand middle piece
		// If we're drawing an evin column number, use the right hand middle piece
		PLOTTILEBA(BubbleTiles, i_x + xline, i_y + i_h, (xline & 0x01) ? 0xED : 0xEE);
	}
	PLOTTILEBA(BubbleTiles, i_x + i_w, i_y + i_h, 0xEF);
	
}

// MenuScr_DrawSelectedBubble
// Draws a bubble on the main bubble layer, with the given x, y, w, h
// i_x : input for X
// i_y : input for Y
// i_w : input for width
// i_h : input for height
void MenuScr_DrawSelectedBubble(int i_x, int i_y, int i_w, int i_h) 
{
	int xline, yline;
	
	// Draw the first line
	PLOTTILEBA(SelectedBubbleTiles, i_x, i_y, 0xBC);
	for (xline = 1; xline < i_w; xline++) 
	{
		// If we're drawing an odd column number, use the left hand middle piece
		// If we're drawing an evin column number, use the right hand middle piece
		PLOTTILEBA(SelectedBubbleTiles, i_x + xline, i_y, (xline & 0x01) ? 0xBD : 0xBE);
	}
	PLOTTILEBA(SelectedBubbleTiles, i_x + i_w, i_y, 0xBF);
	
	for (yline = 1; yline < i_h; yline++) 
	{
		PLOTTILEBA(SelectedBubbleTiles, i_x, i_y + yline, (yline & 0x01) ? 0xCC : 0xDC);
		for (xline = 1; xline < i_w; xline++) 
		{
			PLOTTILEBA(SelectedBubbleTiles, i_x + xline, i_y + yline, 0xBB);
		}
		PLOTTILEBA(SelectedBubbleTiles, i_x + i_w, i_y + yline, ((yline & 0x01) ? 0xCF : 0xDF));
	}
	
	// Draw the last line
	PLOTTILEBA(SelectedBubbleTiles, i_x, i_y + i_h, 0xEC);
	for (xline = 1; xline < i_w; xline++) 
	{
		// If we're drawing an odd column number, use the left hand middle piece
		// If we're drawing an evin column number, use the right hand middle piece
		PLOTTILEBA(SelectedBubbleTiles, i_x + xline, i_y + i_h, (xline & 0x01) ? 0xED : 0xEE);
	}
	PLOTTILEBA(SelectedBubbleTiles, i_x + i_w, i_y + i_h, 0xEF);
	
}

// MenuScr_DrawBubbleBorder
// Draws a bubble on the border of the screen, with a given width and height of the visible portion
// i_w : input for width
// i_h : input for height
void MenuScr_DrawBubbleBorder(int i_w, int i_h) 
{
	int xline, yline1, yline2;
	
	// Start by drawing the white fields at the top and bottom of the screen
	for (yline1=0,yline2=23;yline1 < 12 - i_h;yline1++,yline2--) 
	{
		for (xline = 0; xline < 32; xline++) 
		{
			PLOTTILEBA(SelectedBubbleTiles, xline, yline1, 0xBB);
			PLOTTILEBA(SelectedBubbleTiles, xline, yline2, 0xBB);
		}
	}
	
	// Now draw the white fields to either side of the top and bottom borders
	for (xline = 0; xline < 16 - i_w; xline++) 
	{
		PLOTTILEBA(SelectedBubbleTiles, xline, yline1, 0xBB);
		PLOTTILEBA(SelectedBubbleTiles, xline, yline2, 0xBB);
		PLOTTILEBA(SelectedBubbleTiles, 31 - xline, yline1, 0xBB);
		PLOTTILEBA(SelectedBubbleTiles, 31 - xline, yline2, 0xBB);
	}
	PLOTTILEBA(SelectedBubbleTiles, xline-1, yline1, 0xCD);
	PLOTTILEBA(SelectedBubbleTiles, 31 - xline + 1 , yline1, 0xCE);
	PLOTTILEBA(SelectedBubbleTiles, xline-1, yline2, 0xDD);
	PLOTTILEBA(SelectedBubbleTiles, 31 - xline + 1, yline2, 0xDE);
	
	// Now draw the top and bottom borders of the visible area
	for (xline = 16 - i_w; xline < 16 + i_w; xline++) 
	{
		PLOTTILEBA(SelectedBubbleTiles, xline, yline1, (xline & 0x01) ? 0xED : 0xEE);
		PLOTTILEBA(SelectedBubbleTiles, xline, yline2, (xline & 0x01) ? 0xBD : 0xBE);
	}
	
	// And finally, draw the sides of the box
	for (yline1 = 12 - i_h + 1; yline1 < 12 + i_h - 1; yline1++) 
	{
		for (xline = 0; xline < 16 - i_w - 1; xline++) 
		{
			PLOTTILEBA(SelectedBubbleTiles, xline, yline1, 0xBB);
			PLOTTILEBA(SelectedBubbleTiles, 31 - xline, yline1, 0xBB);
		}
		PLOTTILEBA(SelectedBubbleTiles, xline, yline1, (yline1 & 0x01) ? 0xCF : 0xDF);
		PLOTTILEBA(SelectedBubbleTiles, 31 - xline, yline1, (yline1 & 0x01) ? 0xCC : 0xDC);
	}
}

// MenuScr_DrawSheepLine
// Draws a line of sheepie icons, given a number and a number of selected
void MenuScr_DrawSheepLine(int i_x, int i_y, int i_nNumber, int i_nNumSel) 
{
	int xline;
	
	for (xline = i_x; xline < i_x + i_nNumSel; xline++) 
	{
		PLOTTILEBA(TextTiles, xline, i_y, 0x5C);
		PLOTTILEBA(TextTiles, xline, i_y+1, 0x5D);
	}

	for (xline = i_x + i_nNumSel; xline < i_x + i_nNumber; xline++) 
	{
		PLOTTILEBAOPT(TextTiles, xline, i_y, 0x5C);
		PLOTTILEBAOPT(TextTiles, xline, i_y+1, 0x5D);
	}
}

void DrawWorldSelect(int xxRow, char *xxName, int xxIndex) {
	if (nMaxSelOption >= xxIndex) {
		MenuScr_DrawCenteredTextXY1616(14, xxRow, xxName, (xxIndex == nSelOption));
	} else {
		MenuScr_DrawCenteredTextXY1616(14, xxRow, "locked", 0);
	}
}

// snaps all menu items to their fully realized sizes
void MenuScr_SnapMenuItems() {
	int i;
	for (i=0; i<8; i++) {
		MenuItems[i].renderDir = rdShown;
		MenuItems[i].curw = MenuItems[i].w;
		MenuItems[i].curh = MenuItems[i].h;
	}
}

// MenuScr_DrawMenuItems
// Draws all menu items currently registered, resizing them as need be
// Returns: 1 if some items are still rendering, 0 if menu is fully drawn
int MenuScr_DrawMenuItems(void) 
{
	int i;
	int bOneIsRendering = FALSE;

	for (int nYBkg = 0; nYBkg < 32; nYBkg++) {
		for (int nXBkg = 0; nXBkg < 32; nXBkg++){
			PLOTTILEBA(TextTiles, nXBkg, nYBkg, 0xff);
			PLOTTILEBA(BubbleTiles, nXBkg, nYBkg, 0xff);
			PLOTTILEBA(SelectedBubbleTiles, nXBkg, nYBkg, 0xff);
		}
	}
	
	for (i = 0; i < 8; i++) 
	{
		if (MenuItems[i].uInUse) 
		{
			if (nSelectedButton == i) 
			{
				MenuScr_DrawSelectedBubble(MenuItems[i].x - MenuItems[i].curw, 
									MenuItems[i].y - MenuItems[i].curh,
									(MenuItems[i].curw * 2) + 2,
									(MenuItems[i].curh * 2) + 2);
									
				if (rdShown == MenuItems[i].renderDir) 
				{
					if (MenuItems[i].szMenuString[0] >= ' ') 
					{
						MenuScr_DrawCenteredTextXY1616(MenuItems[i].x, MenuItems[i].y, MenuItems[i].szMenuString, -3 != MenuItems[i].menuval);
					}
				}
			}
			else 
			{
				MenuScr_DrawBubble(MenuItems[i].x - MenuItems[i].curw + 1, 
									MenuItems[i].y - MenuItems[i].curh + 1,
									(MenuItems[i].curw * 2) + 1,
									(MenuItems[i].curh * 2) + 1);
				if (rdShown == MenuItems[i].renderDir) 
				{
					if (MenuItems[i].szMenuString[0] >= ' ') 
					{
						MenuScr_DrawCenteredTextXY1616(MenuItems[i].x, MenuItems[i].y, MenuItems[i].szMenuString, 0);
					}
				}
			}
			
			// handles the extra text whether selected or not
			if ((rdShown == MenuItems[i].renderDir) && (MenuItems[i].szMenuString[0] < ' ')) {
				switch (MenuItems[i].szMenuString[0]) {
					case '\001':
						MenuScr_DrawTextXY1616(11,5, "PRESS", 0);
						MenuScr_DrawTextXY1616(11,7, "START", 0);
						MenuScr_DrawTextXY1616(14,10, "or", 0);
						MenuScr_DrawTextXY1616(9,13, "TAP THE", 0);
						MenuScr_DrawTextXY1616(10,15, "SCREEN", 0);
						MenuScr_DrawTextXY1616(8,18, "to begin", 0);
						break;

					case '\002':
						// \002 is used for the story mode stuff, so it's formatted
						MenuScr_DrawCenteredTextXY816(&MenuItems[i].szMenuString[1]);
						break;
						
					case '\003':
						// \003 is used for the stage intros, wide font
						MenuScr_DrawCenteredTextXY1616(14, 10, &MenuItems[i].szMenuString[1], 0);
						break;
				}
			}
		}
		
		switch (MenuItems[i].renderDir) 
		{
			case rdWaitAppear:
				if (0 == (MenuItems[i].renderwait--)) 
				{
					MenuItems[i].renderDir = rdAppearing;
				}
				bOneIsRendering = TRUE;
			break;
			
			case rdWaitDisappear:
				if (0 == (MenuItems[i].renderwait--)) 
				{
					MenuItems[i].renderDir = rdDisappearing;
				}
				bOneIsRendering = TRUE;
			break;
			
			case rdAppearing:
				if (0 == MenuItems[i].renderdelaywork) 
				{
					MenuItems[i].renderdelaywork = MenuItems[i].renderdelay;
				}
				else 
				{
					bOneIsRendering = TRUE;
					MenuItems[i].renderdelaywork--;
					break;
				}
				MenuItems[i].renderDir = rdShown;
				if (MenuItems[i].curw != MenuItems[i].w) 
				{
					MenuItems[i].curw++;
					MenuItems[i].renderDir = rdAppearing;
				}
				if (MenuItems[i].curh != MenuItems[i].h) 
				{
					MenuItems[i].curh++;
					MenuItems[i].renderDir = rdAppearing;
				}
				bOneIsRendering = TRUE;
			break;
			
			case rdDisappearing:
				if (0 == MenuItems[i].renderdelaywork) 
				{
					MenuItems[i].renderdelaywork = MenuItems[i].renderdelay;
				}
				else 
				{
					bOneIsRendering = TRUE;
					MenuItems[i].renderdelaywork--;
					break;
				}
				MenuItems[i].renderDir = rdNone;
				if (MenuItems[i].curw != 1) 
				{
					MenuItems[i].curw--;
					MenuItems[i].renderDir = rdDisappearing;
				}
				if (MenuItems[i].curh != 1) 
				{
					MenuItems[i].curh--;
					MenuItems[i].renderDir = rdDisappearing;
				}
				if ((1 == MenuItems[i].curw) && (1 == MenuItems[i].curh)) 
				{
					// Once it's as small as it can go, pop it
					MenuItems[i].renderDir = rdNone;
					MenuItems[i].uInUse = 0;
				}
				bOneIsRendering = TRUE;
			break;
		}
	}

	if (nInOptions == NO_OPTIONS) {
		// TODO: this is pretty hacky - the text color varies for one!
		if ((nLastInitMenu == MENU_LOAD) || (nLastInitMenu == MENU_SAVE)) {
			// make sure the font color is useful
			*(u16*)(HW_DB_BG_PLTT+258) = 0x7fff;	// set color
			*(u16*)(HW_DB_BG_PLTT+264) = 0x7fff;	// set color
			// print a little extra text for the load/save menu
			MenuScr_DrawTextXY816((32-strlen(szLoadText))/2, 0, szLoadText, 1);
		}
	} else {
		char szTempStr[13];
		u8 uMacAddr[6];

		MenuScr_DrawBubbleBorder(nBorderX, nBorderY);
		if ((nBorderX > 12) && (nFrames&1)) 
		{
			nBorderX--;	
		}
		if ((nBorderY > 9)  && (nFrames&1))
		{
			nBorderY--;	
		}
		
		switch (nInOptions) {
			case BATTLE_OPTIONS:
				switch (myJiffies&0x100) {
					default:
						MenuScr_DrawCenteredTextXY1616(14,0, "battle options", 0); 
						break;
					case 0x100:
						MenuScr_DrawCenteredTextXY1616(14,0, "l or r to change", 0); 
						break;
				}
				
				if (gOptions.Powers&POW_POWERUPS) {
					MenuScr_DrawCenteredTextXY1616(14,3, "powerups", (0==nSelOption));
				} else {
					MenuScr_DrawCenteredTextXY1616(14,3, "no powerups", (0==nSelOption));
				}

				if (gOptions.Powers&POW_SPECIALS) {
					MenuScr_DrawCenteredTextXY1616(14,5, "specials", (1==nSelOption));
				} else {
					MenuScr_DrawCenteredTextXY1616(14,5, "no specials", (1==nSelOption));
				}

				MenuScr_DrawCenteredTextXY1616(14,7, "cpu skill", (2==nSelOption));
				if (gOptions.CPU) {
					switch (gOptions.Skill) {
						case 0:
							MenuScr_DrawCenteredTextXY1616(14,9, "lost", (2==nSelOption));
							break;
						case 1:
							MenuScr_DrawCenteredTextXY1616(14,9, "normal", (2==nSelOption));
							break;
						case 2:
							MenuScr_DrawCenteredTextXY1616(14,9, "determined", (2==nSelOption));
							break;
					}
				} else {
					MenuScr_DrawCenteredTextXY1616(14,9, "off", (2==nSelOption));
				}
				
				
				MenuScr_DrawCenteredTextXY1616(14,11, "sheep speed", (3==nSelOption));
				switch (gOptions.SheepSpeed) {
					case -1:
						MenuScr_DrawCenteredTextXY1616(14,13, "slow", (3==nSelOption));
						break;
					case 0:
						MenuScr_DrawCenteredTextXY1616(14,13, "normal", (3==nSelOption));
						break;
					case 1:
						MenuScr_DrawCenteredTextXY1616(14,13, "fast", (3==nSelOption));
						break;
				}
				
				MenuScr_DrawCenteredTextXY1616(14,15, "timer", (4==nSelOption));
				switch (gOptions.Timer) {
					case 0:
						MenuScr_DrawCenteredTextXY1616(14,17, "off", (4==nSelOption));
						break;
					case 30:
						MenuScr_DrawCenteredTextXY1616(14,17, "thirty", (4==nSelOption));
						break;
					case 60:
						MenuScr_DrawCenteredTextXY1616(14,17, "sixty", (4==nSelOption));
						break;
					case 90:
						MenuScr_DrawCenteredTextXY1616(14,17, "nintey", (4==nSelOption));
						break;
				}
				break;
		
			case OPTIONS:
				MenuScr_DrawTextXY816(5, 5, "Autosave", (0 == nSelOption));
				MenuScr_DrawTextXY816(18, 5, "ON", MenuScr_CurrentOptions.bAutosave);
				MenuScr_DrawTextXY816(23, 5, "OFF", !MenuScr_CurrentOptions.bAutosave);
				MenuScr_DrawTextXY816(5, 8, "Sound", (1 == nSelOption));
				MenuScr_DrawSheepLine(17,8, 8, MenuScr_CurrentOptions.nSoundVol);
				MenuScr_DrawTextXY816(5, 11, "Music", (2 == nSelOption));
				MenuScr_DrawSheepLine(17,11, 8, MenuScr_CurrentOptions.nMusicVol);
				break;
				
			case SYSINFO: 
				{
					OS_GetMacAddress((u8*)&uMacAddr);
					sprintf(szTempStr, "%012X", uMacAddr);
					MenuScr_DrawTextXY816(5, 5, "MAC Addr", 0);
					MenuScr_DrawTextXY816(15, 5, szTempStr, 0);
				}
				break;
				
			case WORLD_SELECT:
				// quick fixup if the option goes out of legal range
				// probably the wrong place to do this, but eh.
				while ((nSelOption > 0) && (nSelOption != 7) && (nSelOption > nMaxSelOption)) {
					nSelOption--;
				}	

				MenuScr_DrawCenteredTextXY1616(14,0, "select world", 0);
				MenuScr_DrawCenteredTextXY1616(14, 3, "zealande", (0 == nSelOption));	// never locked, so not drawn with DrawWorldSelect
				DrawWorldSelect(5, "candy shoppe", 1);
				DrawWorldSelect(7, "hades place", 2);
				DrawWorldSelect(9, "toy factory", 3);
				DrawWorldSelect(11, "disco", 4);
				DrawWorldSelect(13, "waterworks", 5);
				DrawWorldSelect(15, "underworld", 6);
				break;			
				
			case PLAYER_SELECT:
				MenuScr_DrawCenteredTextXY1616(14,0, "select player", 0);
				MenuScr_DrawCenteredTextXY1616(8, 4, "zeus", (0 == nSelOption));
				MenuScr_DrawCenteredTextXY1616(8, 6, "herder", (1 == nSelOption));
				MenuScr_DrawCenteredTextXY1616(8, 8, "candy", (2 == nSelOption));
				MenuScr_DrawCenteredTextXY1616(8, 10, "nhfive", (3 == nSelOption));
				MenuScr_DrawCenteredTextXY1616(8, 12, "dancer", (4 == nSelOption));
				MenuScr_DrawCenteredTextXY1616(8, 14, "zombie", (5 == nSelOption));
				MenuScr_DrawCenteredTextXY1616(20, 4, "thalia", (6 == nSelOption));
				MenuScr_DrawCenteredTextXY1616(20, 6, "iskur", (7 == nSelOption));
				MenuScr_DrawCenteredTextXY1616(20, 8, "angel", (8 == nSelOption));
				MenuScr_DrawCenteredTextXY1616(20, 10, "hades", (9 == nSelOption));
				MenuScr_DrawCenteredTextXY1616(20, 12, "trey", (10 == nSelOption));
				MenuScr_DrawCenteredTextXY1616(20, 14, "demon", (11 == nSelOption));
				MenuScr_DrawCenteredTextXY1616(14, 16, "random", (12 == nSelOption));
				break;

			case MUSIC_OPTIONS:
				MenuScr_DrawCenteredTextXY1616(14,0, "select music", 0);
				MenuScr_DrawCenteredTextXY1616(8, 4, "title", (0 == nSelOption));
				MenuScr_DrawCenteredTextXY1616(8, 6, "select", (1 == nSelOption));
				MenuScr_DrawCenteredTextXY1616(8, 8, "scores", (2 == nSelOption));
				MenuScr_DrawCenteredTextXY1616(8, 10, "disco", (3 == nSelOption));
				MenuScr_DrawCenteredTextXY1616(8, 12, "haunt", (4 == nSelOption));
				MenuScr_DrawCenteredTextXY1616(8, 14, "toys", (5 == nSelOption));
				MenuScr_DrawCenteredTextXY1616(20, 4, "candy", (6 == nSelOption));
				MenuScr_DrawCenteredTextXY1616(20, 6, "water", (7 == nSelOption));
				MenuScr_DrawCenteredTextXY1616(20, 8, "under", (8 == nSelOption));
				MenuScr_DrawCenteredTextXY1616(20, 10, "n z", (9 == nSelOption));
				MenuScr_DrawCenteredTextXY1616(20, 12, "win", (10 == nSelOption));
				MenuScr_DrawCenteredTextXY1616(20, 14, "over", (11 == nSelOption));
				MenuScr_DrawCenteredTextXY1616(14, 16, "creds", (12 == nSelOption));
				break;
		}

	}
	
	DC_FlushRange(TextTiles, 1024*2);
	DC_FlushRange(BubbleTiles, 1024*2);
	DC_FlushRange(SelectedBubbleTiles, 1024*2);
	GXS_LoadBG1Scr(TextTiles, 0, 1024*2);
	GXS_LoadBG2Scr(BubbleTiles, 0, 1024*2);
	GXS_LoadBG3Scr(SelectedBubbleTiles, 0, 1024*2);
	
	return bOneIsRendering;
}

// MenuScr_TestButtonsHit
// Tests all buttons to see if they have been poked with the stylus
// Returns: integer representing hit button, or -1 if none.
// Inputs: coords that were poked with the stylus
int MenuScr_TestButtonsHit(int i_nXHit, int i_nYHit) {
	int nButton;
	int nMenuX1, nMenuY1, nMenuX2, nMenuY2;
	
	for (nButton = 0; nButton < 8; nButton++) 
	{
		if (MenuItems[nButton].uInUse) 
		{
			nMenuX1 = ((MenuItems[nButton].x - MenuItems[nButton].curw + 1) * 8) + MENU_X1CONVERT;
			nMenuY1 = ((MenuItems[nButton].y - MenuItems[nButton].curh + 1) * 8) + MENU_Y1CONVERT;
			nMenuX2 = ((MenuItems[nButton].x + MenuItems[nButton].curw + 3) * 8) + MENU_X2CONVERT;
			nMenuY2 = ((MenuItems[nButton].y + MenuItems[nButton].curh + 3) * 8) + MENU_Y2CONVERT;
			debug("Button coords: %d x1=%d y1=%d x2=%d y2=%d\n", nButton, nMenuX1, nMenuY1, nMenuX2, nMenuY2);
			if ((i_nXHit > nMenuX1) &&
			    (i_nXHit < nMenuX2) &&
			    (i_nYHit > nMenuY1) &&
			    (i_nYHit < nMenuY2)) 
			{
				return nButton;
			}
		}
	}
	return -1;
}

// MenuScr_IsInMenu
// Returns 1 if a menu is active, 0 if no
int MenuScr_IsInMenu(void) 
{
	return nInMenu;
}

// MenuScr_CloseMenu
// Closes all bubbles and then returns control
void MenuScr_CloseMenu(int i_bImmediate)
{
	int i;
	
	nSelectedButton = -1;
	
	for (i = 0; i < 8; i++) 
	{
		if (MenuItems[i].uInUse) 
		{
			if (i_bImmediate) {
				MenuItems[i].renderDir = rdNone;
				MenuItems[i].uInUse = 0;
			} else {
				MenuItems[i].renderDir = rdDisappearing;
				MenuItems[i].renderdelaywork = MenuItems[i].renderdelay;
			}
		}
	}
	if (!i_bImmediate) {
		while(MenuScr_DrawMenuItems()) 
		{
			// handles 3d and end of frame
			if (!HandleTopMenuView(0)) {
				// otherwise, use the safe 2d end of frame
				pvr_scene_finish_2d();
			}
		}
	}
	
	nInMenu = 0;
}

// MenuScr_ResetMenu
// Resets all menu items to unused so that they are not drawn
void MenuScr_ResetMenu(void) 
{
	int i;
	
	for (i= 0; i < 8; i++) 
	{
		MenuItems[i].uInUse = 0;
	}
	bMenuIsRendering = TRUE;
	nSelectedButton = -1;
	nFramesSinceKeypress = 1;	// don't allow immediate input
	nWaitingTouchOff = 1;
	nPreferredDefault = 0;
	
	// make sure controls are released
	DisableControlsTillReleased();
	
	nInMenu = 0;
	
}

// MenuScr_SetMenuItem
// Sets a menu item for eventual use
// x, y, w, h = x, y of box center, width, height in character units
// width and height will be doubled by 2 when rendered.
// keyup, keydn, keylt, keyrt = menu item to select on relevant d-pad press
// menuval = code returned when button pressed, menualtval = code returned when L+R+A pressed with button selected
// renderwait = time before starting to render the button
// renderdelay = time between size changes (based on 60fps)
// MenuString = string to print.  Please avoid custom-drawn buttons.  One for the first menu is bad enough.
void MenuScr_SetMenuItem(int i_nNumber, int i_x, int i_y, int i_w, int i_h,
						int i_keyup, int i_keydn, int i_keylt, int i_keyrt,
						int i_menuval, int i_menualtval,
						int i_renderwait, int i_renderdelay,
						char* i_MenuString) 
{
#ifdef DEBUG
	if (i_nNumber > 7) {
		debug("Illegal Menu Item Number!\n");
	}
	if (strlen(i_MenuString > 127)) {
		debug("Illegal Menu String Length!\n");
	}
#endif
	// tweak renderdelay - was built for 60fps, we are now running 30
	i_renderdelay=(i_renderdelay/2)-1;
	if (i_renderdelay < 0) i_renderdelay=0;

		MenuItems[i_nNumber].uInUse = 1;
		MenuItems[i_nNumber].renderDir = rdWaitAppear;
		MenuItems[i_nNumber].x = i_x;
		MenuItems[i_nNumber].y = i_y;
		MenuItems[i_nNumber].w = i_w;
		MenuItems[i_nNumber].h = i_h;
		MenuItems[i_nNumber].keyup = i_keyup;
		MenuItems[i_nNumber].keydn = i_keydn;
		MenuItems[i_nNumber].keylt = i_keylt;
		MenuItems[i_nNumber].keyrt = i_keyrt;
		MenuItems[i_nNumber].curw = 1;
		MenuItems[i_nNumber].curh = 1;
		MenuItems[i_nNumber].menuval = i_menuval;
		MenuItems[i_nNumber].menualtval = i_menualtval;
		MenuItems[i_nNumber].renderwait = i_renderwait;
		MenuItems[i_nNumber].renderdelay = i_renderdelay;
		MenuItems[i_nNumber].renderdelaywork = i_renderdelay;
		strcpy(MenuItems[i_nNumber].szMenuString, i_MenuString);
}

// only works to go bigger!! Assumes that it was already grown up once.
void MenuScr_UpdateBubbleSize(int i_nNumber, int i_w, int i_h) {
	MenuItems[i_nNumber].renderDir = rdAppearing;
	MenuItems[i_nNumber].w = i_w;
	MenuItems[i_nNumber].h = i_h;
}

// This function changes just the string - used by story mode
// It always skips the first character for that reason!
void MenuScr_UpdateMenuString(int i_nNumber, char * i_MenuString) {
	strncpy(&MenuItems[i_nNumber].szMenuString[1], i_MenuString, 254);
	MenuItems[i_nNumber].szMenuString[255] = '\0';
}

// MenuScr_InitMenu
// Sets things up to start drawing a menu
// To add your own menu, add a new enum, add a case block, and put your code in there.
// Don't forget to set nInMenu so that the rest of the system knows that the menu system is now
// controlling the subscreen.
// i_nMenu: Menu number to render
void MenuScr_InitMenu(int i_nMenu) 
{
	MapScr_EndMap();
	MenuScr_ResetMenu();

	// set the high word to the preferred menu option if you want that
	// must be after InitMenu!
	if (0 != (i_nMenu>>16)) {
		nPreferredDefault = i_nMenu>>16;
		i_nMenu&=0xffff;
	}
	
	nLastInitMenu = i_nMenu;
	// reset font color to black
	*(u16*)(HW_DB_BG_PLTT+258) = 0;	// set color
	*(u16*)(HW_DB_BG_PLTT+264) = 0;	// set color

	switch (i_nMenu)
	{
		case MENU_PRESSSTART: // 'Press Start' menu
			MenuScr_SetMenuItem(0, 14, 10, 8, 8, -1, -1, -1, -1, 0, 10, 0, 2, "\001");	
			nInMenu = 1;
			break;
	
		case MENU_MAIN: // Main menu
			MenuScr_SetMenuItem(0, 14, 2, 11, 1, -1, 1, -1, -1, MENU_STORY, MENU_STORY, 0, 2, "story mode");
			MenuScr_SetMenuItem(1, 14, 6, 11, 1, 0, 2, -1, -1, MENU_MULTIPLAYER, MENU_MULTIPLAYER, 1, 2, "battle mode");
			MenuScr_SetMenuItem(2, 14, 10, 11, 1, 1, 3, -1, -1, MENU_OPTIONS, MENU_SYS_INFO, 2, 2, "options");
			MenuScr_SetMenuItem(3, 14, 14, 11, 1, 2, -1, -1, -1, MENU_EXTRAS, MENU_EXTRAS, 3, 2, "extras");
			MenuScr_SetMenuItem(6, 3, 19, 4, 2, -1, -1, -1, -1, -1, -1, 4, 2, "back");	
			MenuScr_SetMenuItem(7, 25, 19, 4, 2, -1, -1, -1, -1, -2, -1, 4, 2, "ok");	
			nInMenu = 1;
			break;
			
		case MENU_STORY: // Story mode menu
			MenuScr_SetMenuItem(0, 14, 2, 9, 1, -1, 1, -1, -1, MENU_STORY_NEW, MENU_STORY_NEW, 0, 2, "new game");	
			MenuScr_SetMenuItem(1, 14, 7, 9, 1, 0, 2, -1, -1, MENU_CONTINUE, MENU_CONTINUE, 1, 2, "continue");	
			MenuScr_SetMenuItem(2, 14, 12, 9, 1, 1, 3, -1, -1, MENU_LOAD, MENU_LOAD, 2, 2, "load game");
			MenuScr_SetMenuItem(6, 3, 19, 4, 2, -1, -1, -1, -1, -1, -1, 4, 2, "back");	
			MenuScr_SetMenuItem(7, 25, 19, 4, 2, -1, -1, -1, -1, -2, -1, 4, 2, "ok");	
			nInMenu = 1;
			break;

		case MENU_MULTIPLAYER: // Multiplayer menu
			MenuScr_SetMenuItem(0, 14, 2, 13, 1, -1, 1, -1, -1, MENU_QUICKPLAY, MENU_QUICKPLAY, 0, 2, "solo play");
			MenuScr_SetMenuItem(1, 14, 6, 13, 1, 0, 2, -1, -1, MENU_BATTLE_OPTIONS, MENU_BATTLE_OPTIONS, 1, 2, "solo options");
			MenuScr_SetMenuItem(2, 14, 10, 13, 1, 1, 3, -1, -1, MENU_HOST_GAME, MENU_HOST_GAME, 2, 2, "host game");
			MenuScr_SetMenuItem(3, 14, 14, 13, 1, 2, -1, -1, -1, MENU_JOIN_GAME, MENU_JOIN_GAME, 3, 2, "join game");
			MenuScr_SetMenuItem(6, 3, 19, 4, 2, -1, -1, -1, -1, -1, -1, 4, 2, "back");	
			MenuScr_SetMenuItem(7, 25, 19, 4, 2, -1, -1, -1, -1, -2, -1, 4, 2, "ok");	
			nInMenu = 1;
			break;

		case MENU_EXTRAS: // Extras menu
			MenuScr_SetMenuItem(0, 14, 2, 7, 1, -1, 1, -1, -1, MENU_FILE, MENU_FILE, 0, 2, "file");	
			MenuScr_SetMenuItem(1, 14, 7, 7, 1, 0, 2, -1, -1, MENU_BONUS, MENU_BONUS, 1, 2, "bonus");	
			MenuScr_SetMenuItem(2, 14, 12, 7, 1, 1, -1, -1, -1, MENU_DEMO, MENU_DEMO, 2, 2, "demo");	
			MenuScr_SetMenuItem(6, 3, 19, 4, 2, -1, -1, -1, -1, -1, -1, 4, 2, "back");	
			MenuScr_SetMenuItem(7, 25, 19, 4, 2, -1, -1, -1, -1, -2, -2, 4, 2, "ok");	
			nInMenu = 1;
			break;
			
		case MENU_OPTIONS: // Options menu
		case MENU_MUSICPLAYER:
			MenuScr_SetMenuItem(6, 3, 19, 4, 2, -1, -1, -1, -1, -1, -1, 4, 2, "back");	
			MenuScr_SetMenuItem(7, 25, 19, 4, 2, -1, -1, -1, -1, -4, -4, 4, 2, "ok");
			nInMenu = 1;
			break;
			
		case MENU_FILE:		// file menu
			MenuScr_SetMenuItem(0, 14, 2, 12, 1, -1, 1, -1, -1, MENU_LOAD, MENU_LOAD, 0, 2, "load game");	
			MenuScr_SetMenuItem(1, 14, 7, 12, 1, 0, 2, -1, -1, MENU_SAVE, MENU_SAVE, 1, 2, "save game");
			
			if (gGame.AutoSave) {
				MenuScr_SetMenuItem(2, 14, 12, 12, 1, 1, -1, -1, -1, MENU_AUTOSAVE, MENU_AUTOSAVE, 1, 2, "autosave:on");
			} else {
				MenuScr_SetMenuItem(2, 14, 12, 12, 1, 1, -1, -1, -1, MENU_AUTOSAVE, MENU_AUTOSAVE, 1, 2, "autosave:off");
			}			
			
			MenuScr_SetMenuItem(6, 3, 19, 4, 2, -1, -1, -1, -1, -1, -1, 4, 2, "back");	
			MenuScr_SetMenuItem(7, 25, 19, 4, 2, -1, -1, -1, -1, -2, -1, 4, 2, "ok");
			nInMenu = 1;
			break;
			
		case MENU_LOAD: // Load game menu
			if (nBlockStatus[1] > 0) 
			{
				MenuScr_SetMenuItem(0, 14, 2, 8, 1, -1, 1, -1, -1, MENU_LOAD_A, MENU_LOAD_A, 0, 2, "game a");	
			}
			else 
			{
				MenuScr_SetMenuItem(0, 14, 2, 8, 1, -1, 1, -1, -1, -3, -3, 0, 2, "empty");	
			}
			if (nBlockStatus[2] > 0) 
			{
				MenuScr_SetMenuItem(1, 14, 7, 8, 1, 0, 2, -1, -1, MENU_LOAD_B, MENU_LOAD_B, 1, 2, "game b");	
			}
			else 
			{
				MenuScr_SetMenuItem(1, 14, 7, 8, 1, 0, 2, -1, -1, -3, -3, 1, 2, "empty");	
			}
			if (nBlockStatus[3] > 0) 
			{
				MenuScr_SetMenuItem(2, 14, 12, 8, 1, 1, -1, -1, -1, MENU_LOAD_C, MENU_LOAD_C, 2, 2, "game c");	
			}
			else 
			{
				MenuScr_SetMenuItem(2, 14, 12, 8, 1, 1, -1, -1, -1, -3, -3, 2, 2, "empty");	
			}
			MenuScr_SetMenuItem(6, 3, 19, 4, 2, -1, -1, -1, -1, -1, -1, 4, 2, "back");	
			MenuScr_SetMenuItem(7, 25, 19, 4, 2, -1, -1, -1, -1, -2, -1, 4, 2, "ok");	
			nInMenu = 1;
			break;
			
		case MENU_SAVE: // Save game menu
			if (nBlockStatus[1] > 0) 
			{
				MenuScr_SetMenuItem(0, 14, 2, 8, 1, -1, 1, -1, -1, MENU_SAVE_A, MENU_SAVE_A, 0, 2, "game a");	
			}
			else 
			{
				MenuScr_SetMenuItem(0, 14, 2, 8, 1, -1, 1, -1, -1, MENU_SAVE_A, MENU_SAVE_A, 0, 2, "empty");	
			}
			if (nBlockStatus[2] > 0) 
			{
				MenuScr_SetMenuItem(1, 14, 7, 8, 1, 0, 2, -1, -1, MENU_SAVE_B, MENU_SAVE_B, 1, 2, "game b");	
			}
			else 
			{
				MenuScr_SetMenuItem(1, 14, 7, 8, 1, 0, 2, -1, -1, MENU_SAVE_B, MENU_SAVE_B, 1, 2, "empty");	
			}
			if (nBlockStatus[3] > 0) 
			{
				MenuScr_SetMenuItem(2, 14, 12, 8, 1, 1, -1, -1, -1, MENU_SAVE_C, MENU_SAVE_C, 2, 2, "game c");	
			}
			else 
			{
				MenuScr_SetMenuItem(2, 14, 12, 8, 1, 1, -1, -1, -1, MENU_SAVE_C, MENU_SAVE_C, 2, 2, "empty");	
			}
			MenuScr_SetMenuItem(6, 3, 19, 4, 2, -1, -1, -1, -1, -1, -1, 4, 2, "back");	
			MenuScr_SetMenuItem(7, 25, 19, 4, 2, -1, -1, -1, -1, -2, -1, 4, 2, "ok");	
			nInMenu = 1;
			break;

		case MENU_BONUS: // Bonus items menu
			MenuScr_SetMenuItem(0, 14, 2, 12, 1, -1, 1, -1, -1, MENU_MINIGAMES, MENU_MINIGAMES, 0, 2, "minigames");	
			MenuScr_SetMenuItem(1, 14, 6, 12, 1, 0, 2, -1, -1, MENU_HIGHSCORES, MENU_HIGHSCORES, 1, 2, "high scores");	
			MenuScr_SetMenuItem(2, 14, 10, 12, 1, 1, 3, -1, -1, MENU_MUSICPLAYER, MENU_MUSICPLAYER, 2, 2, "music player");	
			MenuScr_SetMenuItem(3, 14, 14, 12, 1, 2, -1, -1, -1, MENU_CREDITS, MENU_CREDITS, 3, 2, "credits");	
			MenuScr_SetMenuItem(6, 3, 19, 4, 2, -1, -1, -1, -1, -1, -1, 4, 2, "back");
			MenuScr_SetMenuItem(7, 25, 19, 4, 2, -1, -1, -1, -1, -2, -2, 4, 2, "ok");	
			nInMenu = 1;
			break;

		case MENU_MINIGAMES: // Minigames 1
			MenuScr_SetMenuItem(0, 14, 2, 14, 1, -1, 1, -1, -1, MENU_MINI_CANDY, MENU_MINI_CANDY, 0, 2, "candyland");	
			MenuScr_SetMenuItem(1, 14, 6, 14, 1, 0, -1, -1, -1, MENU_MINI_WATER, MENU_MINI_WATER, 1, 2, "iskurs water");	
			MenuScr_SetMenuItem(6, 3, 19, 4, 2, -1, -1, -1, -1, -1, -1, 4, 2, "back");	
			MenuScr_SetMenuItem(7, 25, 19, 4, 2, -1, -1, -1, -1, -2, -2, 4, 2, "ok");	
			nInMenu = 1;
			break;
			
		case MENU_PAUSE: 		// Paws menu
			MenuScr_SetMenuItem(0, 14, 2, 13, 1, -1, 1, -1, -1, MENU_CONTINUE, MENU_CONTINUE, 0, 2, "continue");	
			MenuScr_SetMenuItem(1, 14, 7, 13, 1, 0, 2, -1, -1, MENU_SAVE, MENU_SAVE, 1, 2, "save");	
			MenuScr_SetMenuItem(2, 14, 12, 13, 1, 1, -1, -1, -1, MENU_QUIT, MENU_QUIT, 2, 2, "quit");	
			MenuScr_SetMenuItem(6, 3, 19, 4, 2, -1, -1, -1, -1, -1, -1, 4, 2, "back");	
			MenuScr_SetMenuItem(7, 25, 19, 4, 2, -1, -1, -1, -1, -2, -1, 4, 2, "ok");	
			nInMenu = 1;
			break;

		case MENU_QUIT:			// quit confirmation
			MenuScr_SetMenuItem(0, 14, 2, 14, 1, -1, 1, -1, -1, MENU_QUIT_YES, MENU_QUIT_YES, 0, 2, "quit game");	
			MenuScr_SetMenuItem(1, 14, 7, 14, 1, 0, -1, -1, -1, MENU_QUIT_NO, MENU_QUIT_NO, 1, 2, "return to game");
			MenuScr_SetMenuItem(6, 3, 19, 4, 2, -1, -1, -1, -1, -1, -1, 4, 2, "back");	
			MenuScr_SetMenuItem(7, 25, 19, 4, 2, -1, -1, -1, -1, -2, -1, 4, 2, "ok");
			nInMenu = 1;
			break;


		case MENU_STORY_TEXT: // Story mode screen
			MenuScr_SetMenuItem(0, 14, 10, 14, 6, -1, -1, -1, -1, 0, 10, 0, 0, "\002");		
			nInMenu = 1;
			break;

		case MENU_STAGEINTRO_TEXT: // stage intro - zooming picture
			MenuScr_SetMenuItem(0, 14, 10, 12, 6, -1, -1, -1, -1, 0, 10, 0, 0, "\003");
			nInMenu = 1;
			break;
			
		case MENU_JOIN_MULTIPLAYER: // Multiplayer game selection
			{
				int nEntry = 0;
				int nNext,nPrev;
				for (int idx = 0 ; idx < MAX_WIRELESS_GAMES ; idx++) {
					if ('\0' != gMPStuff.MPGames[idx].GameName[0]) {
						char buf[32], *p;
						strcpy(buf, gMPStuff.MPGames[idx].GameName);
						p=buf;
						while (*p) {
							*p=tolower(*p);
							p++;
						}
						if (nEntry == 0) {
							nPrev = -1;
						} else {
							// I know this amounts to the same thing, but it's a flag when it's -1, being clear.
							nPrev = nEntry-1;
						}
						// peek ahead at the next one (assumes games are a packed array with no gaps)
						if ((idx < MAX_WIRELESS_GAMES-1) && ('\0' != gMPStuff.MPGames[idx+1].GameName[0])) {
							nNext = nEntry+1;
						} else {
							nNext = -1;
						}
						MenuScr_SetMenuItem(nEntry, 14, 1 + (nEntry*4), 14, 1, nPrev, nNext, -1, -1, idx, idx, 0, 2, buf);
						nEntry++;
					}
				}
				if (nEntry == 0) {
					// returns 'back' if selected
					MenuScr_SetMenuItem(nEntry++, 14, 1 + (nEntry*4), 14, 1, -1, -1, -1, -1, -1, -1, 0, 2, "no games found");
				}

				MenuScr_SetMenuItem(nEntry++, 3, 20, 4, 2, -1, -1, -1, -1, -1, -1, 4, 2, "back");	
				MenuScr_SetMenuItem(nEntry++, 25, 20, 4, 2, -1, -1, -1, -1, MENU_JOIN_SCAN, MENU_JOIN_SCAN, 4, 2, "scan");
			}
			nInMenu = 1;
			break;
			
		case MENU_WIRELESS_GAME:
			MenuScr_SetMenuItem(0, 14, 2, 11, 1, -1, 1, -1, -1, MENU_LEVEL_SELECT, MENU_LEVEL_SELECT, 0, 2, "world");
			MenuScr_SetMenuItem(1, 14, 6, 11, 1, 0, 2, -1, -1, MENU_BATTLE_OPTIONS, MENU_BATTLE_OPTIONS, 1, 2, "options");
			MenuScr_SetMenuItem(2, 14, 10, 11, 1, 1, 3, -1, -1, MENU_CHARACTER_SELECT, MENU_CHARACTER_SELECT, 2, 2, "character");
			MenuScr_SetMenuItem(3, 14, 14, 11, 1, 2, -1, -1, -1, MENU_START, MENU_START, 3, 2, "start");
			MenuScr_SetMenuItem(6, 3, 19, 4, 2, -1, -1, -1, -1, -1, -1, 4, 2, "back");	
			MenuScr_SetMenuItem(7, 25, 19, 4, 2, -1, -1, -1, -1, -2, -1, 4, 2, "ok");
			nInMenu = 1;
			break;
			
		default:	// should never happen?
			MenuScr_SetMenuItem(6, 3, 19, 4, 2, -1, -1, -1, -1, -1, -1, 4, 2, "back");	
			nInMenu = 1;
	}
}

// MenuScr_DrawFrame
// Draws one frame of the menu screen
// Outputs: Updates an int with the value of the selected item
// Returns: TRUE if the user has selected an option, FALSE otherwise
int MenuScr_DrawFrame(int *pSelButton) {
	TPData Data;

	if (nInOptions != NO_OPTIONS) 
	{
		u16 uPadInfo = GetController(gHumanPlayer);
		// filter L and R if not in the battle options menu
		if (nInOptions != BATTLE_OPTIONS) {
			uPadInfo &= ~(PAD_BUTTON_L|PAD_BUTTON_R);
		}
		
		if (0 == nFramesSinceKeypress) 
		{
			if (uPadInfo & PAD_KEY_UP)
			{
				sound_effect_system(SND_CLICK, SHEEPVOL);
				if (nSelOption > 0) {
					nSelOption--;
				}
			}
			if (uPadInfo & PAD_KEY_DOWN)
			{
				sound_effect_system(SND_CLICK, SHEEPVOL);
				if (nSelOption < nMaxSelOption) 
				{
					if (((nInOptions == PLAYER_SELECT)||(nInOptions == MUSIC_OPTIONS)) && (nSelOption == 5)) {
						nSelOption=12;
					} else {
						nSelOption++;	
					}
				}
			}
			// left and right only on certain screens
			if ((nInOptions == OPTIONS) || (nInOptions == PLAYER_SELECT) || (nInOptions == BATTLE_OPTIONS) || (nInOptions == MUSIC_OPTIONS)) {
				if (uPadInfo & (PAD_KEY_LEFT|PAD_BUTTON_L))
				{
					sound_effect_system(SND_CLICK, SHEEPVOL);
					if (nInOptions == BATTLE_OPTIONS) {
						switch (nSelOption) {
							case 0:
								// power ups
								gOptions.Powers|=POW_POWERUPS;
								break;
							case 1:
								// specials
								gOptions.Powers|=POW_SPECIALS;
								break;
							case 2:
								// CPU skill
								if (gOptions.CPU) {
									if (gOptions.Skill > 0) {
										gOptions.Skill--;
									} else {
										gOptions.CPU=0;
									}
								}
								break;
							case 3:
								// Sheep Speed
								if (gOptions.SheepSpeed > -1) gOptions.SheepSpeed--;
								break;
							case 4:
								// Timer
								if (gOptions.Timer > 0) gOptions.Timer-=30;
								if (gOptions.Timer < 0) gOptions.Timer=0;
								break;
						}
					} else if (nInOptions == OPTIONS) {
						switch (nSelOption) 
						{
							case 0:
								// Autosave
								MenuScr_CurrentOptions.bAutosave = 1;
							break;
							case 1:
								// Sound volume
								if (MenuScr_CurrentOptions.nSoundVol > 0) 
								{
									MenuScr_CurrentOptions.nSoundVol--;
									set_sound_volume(-1, MenuScr_CurrentOptions.nSoundVol);
								}
							break;
							case 2:
								// Music volume
								if (MenuScr_CurrentOptions.nMusicVol > 0) 
								{
									MenuScr_CurrentOptions.nMusicVol--;
									set_sound_volume(MenuScr_CurrentOptions.nMusicVol, -1);
								}
							break;
						}
					} else {
						// player select
						if (nSelOption >= 6) {
							if (nSelOption == 12) {
								nSelOption = 5;
							} else {
								nSelOption-=6;
							}
						}
					}
				}
				if (uPadInfo & (PAD_KEY_RIGHT|PAD_BUTTON_R)) 
				{
					sound_effect_system(SND_CLICK, SHEEPVOL);
					if (nInOptions == BATTLE_OPTIONS) {
						switch (nSelOption) {
							case 0:
								// power ups
								gOptions.Powers&=~POW_POWERUPS;
								break;
							case 1:
								// specials
								gOptions.Powers&=~POW_SPECIALS;
								break;
							case 2:
								// CPU skill
								if (!gOptions.CPU) {
									gOptions.CPU = 1;
								} else {
									if (gOptions.Skill < 2) {
										gOptions.Skill++;
									}
								}
								break;
							case 3:
								// Sheep Speed
								if (gOptions.SheepSpeed < 1) gOptions.SheepSpeed++;
								break;
							case 4:
								// Timer
								if (gOptions.Timer < 90) gOptions.Timer+=30;
								if (gOptions.Timer > 90) gOptions.Timer=90;
								break;
						}
					} else if (nInOptions == OPTIONS) {
						switch (nSelOption) 
						{
							case 0:
								// Autosave
								MenuScr_CurrentOptions.bAutosave = 0;
							break;
							case 1:
								// Sound volume
								if (MenuScr_CurrentOptions.nSoundVol < 8) 
								{
									MenuScr_CurrentOptions.nSoundVol++;
									set_sound_volume(-1, MenuScr_CurrentOptions.nSoundVol);
								}
							break;
							case 2:
								// Music volume
								if (MenuScr_CurrentOptions.nMusicVol < 8) 
								{
									MenuScr_CurrentOptions.nMusicVol++;
									set_sound_volume(MenuScr_CurrentOptions.nMusicVol, -1);
								}
							break;
						}
					} else {
						if (nSelOption < 6) {
							nSelOption+=6;
						} else if (nSelOption == 12) {
							nSelOption = 11;
						}
					}
				}
			}
		}
		if (0 == (uPadInfo & (PAD_KEY_UP | PAD_KEY_DOWN | PAD_KEY_LEFT | PAD_KEY_RIGHT | PAD_BUTTON_L | PAD_BUTTON_R )))
		{
			nFramesSinceKeypress = 0;
		}
		else 
		{
			nFramesSinceKeypress = ((nFramesSinceKeypress + 1) % 30);
		}
		if (uPadInfo & PAD_BUTTON_A)
		{
			*pSelButton = nSelOption;
			return TRUE;
		}
		if (uPadInfo & PAD_BUTTON_B) 
		{
			*pSelButton = -1;
			return TRUE;	
		}
	}
	else 
	{
		if (-1 != nSelectedButton) 
		{
			u16 uPadInfo=GetController(gHumanPlayer);
			
			if (0 == nFramesSinceKeypress) 
			{
				if ((uPadInfo & PAD_KEY_UP) && (-1 != MenuItems[nSelectedButton].keyup)) 
				{
					sound_effect_system(SND_CLICK, SHEEPVOL);
					nSelectedButton = MenuItems[nSelectedButton].keyup;
				}
				if ((uPadInfo & PAD_KEY_DOWN) && (-1 != MenuItems[nSelectedButton].keydn)) 
				{
					sound_effect_system(SND_CLICK, SHEEPVOL);
					nSelectedButton = MenuItems[nSelectedButton].keydn;
				}
				if ((uPadInfo & PAD_KEY_LEFT) && (-1 != MenuItems[nSelectedButton].keylt)) 
				{
					sound_effect_system(SND_CLICK, SHEEPVOL);
					nSelectedButton = MenuItems[nSelectedButton].keylt;
				}
				if ((uPadInfo & PAD_KEY_RIGHT) && (-1 != MenuItems[nSelectedButton].keyrt)) 
				{
					sound_effect_system(SND_CLICK, SHEEPVOL);
					nSelectedButton = MenuItems[nSelectedButton].keyrt;
				}
			}
			if (0 == (uPadInfo & (PAD_KEY_UP | PAD_KEY_DOWN | PAD_KEY_LEFT | PAD_KEY_RIGHT)))
			{
				nFramesSinceKeypress = 0;
			}
			else 
			{
				nFramesSinceKeypress = ((nFramesSinceKeypress + 1) % 30);
			}
			if ((uPadInfo & PAD_BUTTON_A) && (uPadInfo & PAD_BUTTON_L) && (uPadInfo & PAD_BUTTON_R) && (MenuItems[nSelectedButton].menualtval >= 0)) 
			{
				debug("Alternate option selected.\n");
				*pSelButton = MenuItems[nSelectedButton].menualtval;
				return TRUE;
			}
			if ((uPadInfo & PAD_BUTTON_A) && (MenuItems[nSelectedButton].menuval >= 0)) 
			{
				*pSelButton = MenuItems[nSelectedButton].menuval;
				return TRUE;
			}
			if (uPadInfo & PAD_BUTTON_B) 
			{
				*pSelButton = -1;
				return TRUE;	
			}
		}
	}
	
	if (0 == GetTouchData(&Data)) 
	{
		if (TP_TOUCH_ON == Data.touch)
		{
			
			if (0 == nWaitingTouchOff) 
				
			{
				int nTouchedButton;
				if (Data.validity & TP_VALIDITY_INVALID_XY)
				{
					debug("Invalid touch data encountered, ignoring as a test.\n");
				}
				else 
				{
					debug("TP: x=%d y=%d\n", Data.x, Data.y);
					nTouchedButton = MenuScr_TestButtonsHit(Data.x, Data.y);
					// -1 is 'back'
					if (-1 != nTouchedButton) 
					{
						// 'OK', so take current selection
						if (-2 == MenuItems[nTouchedButton].menuval) 
						{
							debug("Button selected is %d, with value %d\n", nSelectedButton, MenuItems[nSelectedButton].menuval);
							*pSelButton = MenuItems[nSelectedButton].menuval;
							return TRUE;
						}
						// -4 makes OK return the nSelOption value (player/world select)
						if (-4 == MenuItems[nTouchedButton].menuval) 
						{
							debug("Selected item is %d\n", nSelOption);
							*pSelButton = nSelOption;
							return TRUE;
						}
						// TODO: not really sure what -3 is for?
						// This is a NOT equal, so it must be last
						if (-3 != MenuItems[nTouchedButton].menuval) 
						{
							*pSelButton = MenuItems[nTouchedButton].menuval;
							return TRUE;
						}
					} else if (nInOptions != NO_OPTIONS) {
						// possible non-button hits - select only, does
						// not count as OK'd
						if ((Data.x > 32) && (Data.x < 224)) {
							int tmp;
							switch (nInOptions) {
								case WORLD_SELECT:
									tmp = (Data.y / 16)-2;
									if ((tmp >= 0) && (tmp < 7)) {
										if (nSelOption != tmp) {
											sound_effect_system(SND_CLICK, SHEEPVOL);
											nSelOption = tmp;
										}
									}
									break;
								
								case PLAYER_SELECT:
								case MUSIC_OPTIONS:
									tmp = ((Data.y+8) / 16) - 3;
									if ((tmp >= 0) && (tmp < 6)) {
										if (Data.x > 128) {
											tmp+=6;
										}
										if (nSelOption != tmp) {
											sound_effect_system(SND_CLICK, SHEEPVOL);
											nSelOption = tmp;
										}
									} else if (tmp == 6) {
										if (nSelOption != 12) {
											sound_effect_system(SND_CLICK, SHEEPVOL);
											nSelOption = 12;
										}
									}
									break;
							}
						}
					}
				}
			}
		}
		else 
		{
			nWaitingTouchOff = 0;
		}
	}
	
	int bMenuStillRendering = MenuScr_DrawMenuItems();
	if (bMenuIsRendering && !bMenuStillRendering) 
	{
		// We have just finished drawing the menu
		bMenuIsRendering = FALSE;
		if (MenuItems[0].uInUse) 
		{
			int i;
			
			// If menu item 0 is in use, go ahead and mark it selected
			// If no default applies, please do NOT use button 0
			nSelectedButton = 0;

			if (nPreferredDefault != 0) {
				// try to find the default button if one was requested
				for (i=0; i<8; i++) {
					if ((MenuItems[i].uInUse) && (MenuItems[i].menuval == nPreferredDefault)) {
						nSelectedButton = i;
					}
				}
			}
				
			// redraw real quick so the selection is shown (mostly helps story)
			MenuScr_DrawMenuItems();
		}
	}
	
	return FALSE;
}

void MenuScr_SetupOptions(int i_bStartup) {
	if (i_bStartup) 
	{
		G2S_SetBG2Priority(1); // First unselected buttons
		G2S_SetBG3Priority(2); // Then the bubble border
		G2S_SetBG3Offset(0, 0);
		GXS_LoadBGPltt(Tileset_B_V6a_OptionsPalette, (unsigned long)(nNextPaletteOffset*16)*2, (16)*2);
		debug("Options palette loaded at %d\n", nNextPaletteOffset);
		nBorderX = 15;
		nBorderY = 11;
		nSelOption = 0;
	}
	else 
	{
		G2S_SetBG3Priority(1); // Then the selected button
		G2S_SetBG2Priority(2); // Then any unselected buttons
		G2S_SetBG3Offset(-4, -4);
		nInOptions = NO_OPTIONS;
	}
}

void MenuScr_DoOptions(int i_bEnter) {
	MenuScr_SetupOptions(i_bEnter);
	if (i_bEnter) {
		nInOptions = OPTIONS;
		nMaxSelOption = 2;
		MenuScr_CurrentOptions.bAutosave = gGame.AutoSave;
		MenuScr_CurrentOptions.nSoundVol = gGame.SVol;
		MenuScr_CurrentOptions.nMusicVol = gGame.MVol;
	}
}

void MenuScr_DoBattleOptions(int i_bEnter) {
	MenuScr_SetupOptions(i_bEnter);
	if (i_bEnter) {
		nInOptions = BATTLE_OPTIONS;
		nMaxSelOption = 4;
		// make a temporary copy of the options to manipulate
		memcpy(&gOptions, &gGame.Options, sizeof(gOptions));
	}
}

void MenuScr_DoSystemInfo(int i_bEnter) {
	MenuScr_SetupOptions(i_bEnter);
	if (i_bEnter) {
		nInOptions = SYSINFO;
		nMaxSelOption = 0;
	}
}

void MenuScr_DoWorldSelect(int i_bEnter, int i_nWorld, int nMax) {
	MenuScr_SetupOptions(i_bEnter);
	if (i_bEnter) {
		nInOptions = WORLD_SELECT;
		nMaxSelOption = nMax;
		if (i_nWorld != -1) {
			nSelOption = i_nWorld;
		}
	}
}

void MenuScr_DoPlayerSelect(int i_bEnter, int i_nPlayer) {
	MenuScr_SetupOptions(i_bEnter);
	if (i_bEnter) {
		nInOptions = PLAYER_SELECT;
		nMaxSelOption = 12;
		if (i_nPlayer != -1) {
			nSelOption = i_nPlayer;
		}
	}
}

void MenuScr_DoMusicSelect(int i_bEnter) {
	MenuScr_SetupOptions(i_bEnter);
	if (i_bEnter) {
		nInOptions = MUSIC_OPTIONS;
		nMaxSelOption = 12;
		nSelOption = 0;
	}
}

// MenuScr_Init: 
// This function initializes the map screen, clearing all memory and releasing it's usage
// of all VRAM resources.  If you have messed with VRAM, there are better ways than this
// to restore the resources.
void MenuScr_Init(void) {
	nInMenu = 0;
	
	MenuScr_InitTileset(&TilesetA, (void*)Menu_BG_Bitmap, (void*)Menu_BG_Palette);	
	MenuScr_InitTileset(&TilesetBa, (void*)Tileset_BA2_Tiles, (void*)Tileset_BA2_Palette);	
	pWorkingMapA = (unsigned short*)Tileset_BA2_Map;
	MenuScr_InitTileset(&TilesetBb, (void*)Tileset_B_V6b_Tiles, (void*)Tileset_B_V6b_Palette);	
	MenuScr_InitTileset(&TilesetBc, (void*)Tileset_B_V6c_Tiles, (void*)Tileset_B_V6c_Palette);	
	MenuScr_InitTileset(&TilesetD, (void*)menufont_Tiles, (void*)menufont_Palette);	
	MenuScr_ResetVRAM();
	
	InitTouchScreen();
}

// update nSelOption externally
void MenuScr_SetSelOption(int i_nOption) {
	nSelOption = i_nOption;
}

int MenuScr_GetSelOption() {
	return nSelOption;
}
