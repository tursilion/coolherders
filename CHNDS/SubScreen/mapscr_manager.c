/****************************************/
/* Cool Herders                         */
/* http://harmlesslion.com              */
/* Copyright 2009 HarmlessLion LLC      */
/* mapscr_manager.c                     */
/****************************************/

// NOTE!  This entire file ASSUMES that the CHARBASE for BG0, BG1, BG2, and BG3 is and will always be...
// THE SAME!!!!!  If they are different, the VRAM allocator is going to seriously fscking break, and
// I am NOT PICKING UP THE PIECES!  No, this is not a gigantic limitation.  It does prevent them from
// using more than 1 block of VRAM (32 or 16k), but CH is not intended to exceed that.

#include <types.h>
#include <string.h>
#include <stdlib.h>

#include "..\ch\kosemulation.h"
#include "..\ch\sprite.h"
#include "..\ch\cool.h"
#include "..\ch\levels.h"
#include "..\ch\sheep.h"

#include "mapscr_manager.h"

#include "Tileset_A.h"
#include "Tileset_B.h"
#include "TilesetC.h"

#define MAPXSIZE 19
#define MAPYSIZE 13
#define MAPXOFF  1
#define MAPYOFF  3

#define MAZESPROFFX 57
#define MAZESPROFFY 34
#define MAZESPRSKIPX (MAPXOFF * 32)
#define MAZESPRSKIPY (MAPYOFF * 32)

const u32 c_IllegalTile = 0xFFFFFFFF;
int bMapActive = 0;

u32 nNextVramTileUsed;
u8 nNextPaletteOffset;

MapScr_TileSet TilesetA;
MapScr_TileSet TilesetBa;
MapScr_TileSet TilesetBb;
MapScr_TileSet TilesetBc;

static unsigned short *pWorkingMapA;

void* pSpriteSrc;
void* pSpritePalette;

u16 BkgTiles [32][32];
u16 TextTiles [32][32];
u16 MapTiles [32][32];
u16 ThermoTiles [32][32];

GXOamAttr Sprites[128];

// text for indication of this stage's special, if any
char szSpecial[17]="                ";

MapScr_HerderSidebar herderSidebarDestinations[4] = {
	{216, 0},
	{216, 48},
	{216, 96},
	{216, 144},
};

MapScr_HerderSidebar herderCurrentLocations[4];

// Draws 8x16 text at the given location
const MapScr_FontInfo sConvertArray[] = { 
    { ' ', 0xff, 0xff},
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
	{ '@', 0xfb, 0xff},	// st
	{ '#', 0xfc, 0xff},	// no
	{ '$', 0xfd, 0xff},	// ro
	{ '%', 0xfe, 0xff}, // tm
	{ '^', 0xf0, 0xf1}, // little x?
	{ '*', 0x5e, 0x5f},	// heart
};

#if 0
const u8 DiseaseInfo[][9] = 
{
	{0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff}, // 0 = no disease
	{0x00,0x01,0x02,0x03,0x04,0x05,0xff,0x06,0xff}, // 1 = rabbit
	{0xff,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e}, // 2 = turtle
	{0x0f,0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17}, // 3 = wiggy-foot man
	{0x18,0x19,0x1a,0x1b,0x1c,0x1d,0xff,0x1e,0xff}, // 4 = 4-way lightning
	{0x1f,0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27}, // 5 = omega magnet and sheep
	{0x28,0x29,0x2a,0x2b,0x2c,0x2d,0x2e,0x2f,0x30}, // 6 = u-magnet and sheep
	{0xff,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38}, // 7 = dotted line guy   
	{0x39,0x3a,0x3b,0x3c,0x3d,0x3e,0xff,0x3f,0xff}, // 8 = question mark     
	{0x40,0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48}, // 9 = crossed hand      
	{0x40,0x49,0x4a,0x4b,0x4c,0x4d,0x46,0x4e,0x4f}, // 10= crossed sheep    
	{0x50,0x51,0x52,0x53,0x54,0x55,0x56,0x57,0x58}, // 11= swirl            
	{0x59,0x5a,0x5b,0x5c,0x5d,0x5e,0xff,0x5f,0xff}, // 12= shrunken lightning
	{0x60,0x61,0x62,0x63,0x64,0x65,0x66,0x67,0x68}, // 13= swollen lightning
};
#endif


u8 uCurrentThermo1;
u8 uCurrentThermo2;

static int bAllSettingsMade = 0;
static int bDoNotShow = 1; // Do not show subscreen if this is set

// MapScr_InitTileset: Takes a tileset, and a pointer to the memory and palette blocks
// Prepares a tileset for loading from main memory to VRAM, but does not allocate or copy
// any VRAM.  Do NOT call this on a tileset if that tileset has VRAM allocated to it.
void MapScr_InitTileset(MapScr_TileSet* i_pTileSet, void* i_pbMemory, void* i_pbPalette) {
	i_pTileSet->nVramTile = c_IllegalTile;
	i_pTileSet->nPalette = 0;
	i_pTileSet->pbMemoryAddress = i_pbMemory;
	i_pTileSet->pbPaletteAddress = i_pbPalette;	
}

// MapScr_LoadTileSet: Takes a tileset, and the size of the tiles (in tiles) and palette (in
// blocks of 16 colors).
// Actually loads a tileset to VRAM and notes down where the information was placed
// These numbers (nVramTile, nPalette) can be used directly to NitroSDK calls
// NOTE!  256 color tiles count as two tiles!  If you want to use these, divide the tile
// number by 2 before passing to a 256 color BG.  256 color tiles must come first or be aligned
// by 2.
void MapScr_LoadTileset(MapScr_TileSet* i_pTileSet, u32 i_nTileSize, u32 i_nPaletteSize) {
	DC_FlushRange(i_pTileSet->pbMemoryAddress, i_nTileSize*32);
	GXS_LoadBG0Char(i_pTileSet->pbMemoryAddress, nNextVramTileUsed*32, i_nTileSize*32);
	i_pTileSet->nVramTile = nNextVramTileUsed;
	nNextVramTileUsed += i_nTileSize;
	
	GXS_LoadBGPltt(i_pTileSet->pbPaletteAddress, (unsigned long)(nNextPaletteOffset*16)*2, (i_nPaletteSize*16)*2);
	i_pTileSet->nPalette = nNextPaletteOffset;
	nNextPaletteOffset += i_nPaletteSize;
}

// MapScr_ResetSubScr(void)
// Resets all the I/O registers needed to cause the subscreen to display
// the map screen.  If VRAM has not been manipulated, this is all that
// needs to be called to restore graphics after other uses of the map screen.
// Inputs: takes an int that is 1 if it is ok to make the screen visible
//         set to 0 if the other pointers should be set but it's not time to show the screen
void MapScr_ResetSubScr(int bOkToMakeVisible) {
	// This initializes the hardware for the needs of the information screen
	// It does not need to be called again unless the 2D B engine has been altered
	GX_SetBankForSubBG(GX_VRAM_SUB_BG_32_H);
	GX_SetBankForSubOBJ(GX_VRAM_SUB_OBJ_16_I);

	GXS_SetGraphicsMode(GX_BGMODE_0);

	G2S_SetBG0Priority(3); // Tiled background on bottom
	G2S_SetBG1Priority(0); // Then text on top
	G2S_SetBG2Priority(2); // Then the maze layer
	G2S_SetBG3Priority(1); // Then thermometers
	
	G2S_SetBG1Offset(0, -4);
	G2S_SetBG2Offset(-45, -5);
	
	G2S_SetWnd0InsidePlane(GX_WND_PLANEMASK_BG0 | GX_WND_PLANEMASK_BG1 |
		GX_WND_PLANEMASK_BG2 | GX_WND_PLANEMASK_BG3 | GX_WND_PLANEMASK_OBJ, TRUE);
	G2S_SetWndOutsidePlane(GX_WND_PLANEMASK_BG0 | GX_WND_PLANEMASK_BG1 |
		GX_WND_PLANEMASK_BG2 | GX_WND_PLANEMASK_BG3 | GX_WND_PLANEMASK_OBJ, FALSE);
	
	G2S_SetWnd0Position(43, 0, 214, 192);
	GXS_SetVisibleWnd(GX_WNDMASK_W0);
	G2S_SetBlendBrightness(GX_BLEND_PLANEMASK_BG0, -8);	
	
	GXS_SetOBJVRamModeChar(GX_OBJVRAMMODE_CHAR_1D_64K);

	if (bOkToMakeVisible) 
	{
		GXS_SetVisiblePlane(GX_PLANEMASK_BG0 | GX_PLANEMASK_BG1 | GX_PLANEMASK_BG2 | GX_PLANEMASK_BG3 | GX_PLANEMASK_OBJ);
	}
	
}

#define PLOTTILEBA(array, xpos, ypos, tile) \
	(array[ypos][xpos] = (u16)((pWorkingMapA[tile] + TilesetBa.nVramTile)))
#define PLOTTILEBC(array, xpos, ypos, tile) \
	(array[ypos][xpos] = (u16)((Tileset_B_V6c_Map[tile] + TilesetBc.nVramTile)))
	
// MapScr_DrawTextXY816: Takes an x and y location where the string should start, and a string
// Converts ASCII to font, and renders an 8x16 font (stored in a 16x16 tile array).
void MapScr_DrawTextXY816(u8 x, u8 y, char* pszText)
{
	u16 nCharCode;
	char* pszCurChar = pszText;
	
	while (NULL != *pszCurChar) 
	{
		for (nCharCode = 0; nCharCode < (sizeof(sConvertArray)/sizeof(sConvertArray[0])); nCharCode++) 
		{
			if (*pszCurChar == sConvertArray[nCharCode].cCharName) 
			{
				PLOTTILEBA(TextTiles, x, y, sConvertArray[nCharCode].uUpper);
				PLOTTILEBA(TextTiles, x, y+1, sConvertArray[nCharCode].uLower);
				break;
			}
		}
		pszCurChar++;
		x++;
	}
}

// MapScr_ResetVRAM(void)
// Resets all allocations within VRAM, and then reloads it.  If main memory has not been touched
// since the map screen was initialized, it is safe to call this if you manipulated VRAM.  This
// will rewrite everything.  However, it has maintained direct pointers within any structures
// in the main program.  If those have moved....zzhhhaakkkkkkk!
void MapScr_ResetVRAM(void) {
	u32 nScreenTables;
	u16 nXBkg, nYBkg;
	int nTempHerder, nTempSheep;
	
	MapScr_ResetSubScr(0);

	nNextVramTileUsed = 0;
	nNextPaletteOffset = 0;
	
	MapScr_LoadTileset(&TilesetA, 32, 4); // 16 tiles, and these are 256 color ones, so doubled. 
											// Plus, 4 16 color palettes to give 64 colors
	TilesetA.nVramTile /= 2; // And that divide by 2 fixup I talked about.
	
	MapScr_LoadTileset(&TilesetBa, 214, 1); // 214 tiles for the main tiles, all palettes loaded for now
	MapScr_LoadTileset(&TilesetBb, 100, 1); // 100 tiles for the main tiles, all palettes loaded for now
	MapScr_LoadTileset(&TilesetBc, 10, 1); // 10 tiles for the main tiles, all palettes loaded for now

	if (NULL != pSpriteSrc) 
	{
		GXS_LoadOBJ(pSpriteSrc, 0, 16384);
		GXS_LoadOBJPltt(pSpritePalette, 0, 512);
	}
	
	nScreenTables=((nNextVramTileUsed * 32) / 2048) + 1;
	
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
			PLOTTILEBA(MapTiles, nXBkg, nYBkg, 0xff);
			PLOTTILEBA(ThermoTiles, nXBkg, nYBkg, 0xff);
		}
	}
	
	for (nXBkg = 1; nXBkg <= MAPXSIZE; nXBkg++) {
		PLOTTILEBA(MapTiles, nXBkg, 0, 0x76);
		PLOTTILEBA(MapTiles, nXBkg, 1, 0x6f);
		PLOTTILEBA(MapTiles, nXBkg, 2, 0x6f);
		for (int nTempShit = 3; nTempShit <= MAPYSIZE + 2; nTempShit++) 
		{
			PLOTTILEBA(MapTiles, nXBkg, nTempShit, 0x6e);
		}
		PLOTTILEBA(MapTiles, nXBkg, MAPYSIZE + 2, 0x6f);
		PLOTTILEBA(MapTiles, nXBkg, MAPYSIZE + 3, 0x6f);
		PLOTTILEBA(MapTiles, nXBkg, MAPYSIZE + 4, 0x6f);
		PLOTTILEBA(MapTiles, nXBkg, MAPYSIZE + 5, 0x77);
	}
	for (nYBkg = 1; nYBkg <= MAPYSIZE+5; nYBkg++) {
		PLOTTILEBA(MapTiles, 0, nYBkg, 0x74);
		PLOTTILEBA(MapTiles, MAPXSIZE+1, nYBkg, 0x75);
	}
	PLOTTILEBA(MapTiles, 0, 0, 0x70);
	PLOTTILEBA(MapTiles, MAPXSIZE+1, 0, 0x71);
	PLOTTILEBA(MapTiles, 0, MAPYSIZE+5, 0x72);
	PLOTTILEBA(MapTiles, MAPXSIZE+1, MAPYSIZE+5, 0x73);
	
	// Draw the sheepie for the counter
	PLOTTILEBC(ThermoTiles, 7, MAPYSIZE + 2, 0x00);
	PLOTTILEBC(ThermoTiles, 8, MAPYSIZE + 2, 0x01);
	PLOTTILEBC(ThermoTiles, 9, MAPYSIZE + 2, 0x02);
	PLOTTILEBC(ThermoTiles, 7, MAPYSIZE + 3, 0x03);
	PLOTTILEBC(ThermoTiles, 8, MAPYSIZE + 3, 0x04);
	PLOTTILEBC(ThermoTiles, 9, MAPYSIZE + 3, 0x05);
	PLOTTILEBC(ThermoTiles, 7, MAPYSIZE + 4, 0x06);
	PLOTTILEBC(ThermoTiles, 8, MAPYSIZE + 4, 0x07);
	PLOTTILEBC(ThermoTiles, 9, MAPYSIZE + 4, 0x08);
	
	// Draw the clock for the Timer
	PLOTTILEBA(ThermoTiles, 19, MAPYSIZE + 2, 0xB8);
	PLOTTILEBA(ThermoTiles, 20, MAPYSIZE + 2, 0xB9);
	PLOTTILEBA(ThermoTiles, 21, MAPYSIZE + 2, 0xBA);
	PLOTTILEBA(ThermoTiles, 19, MAPYSIZE + 3, 0xC8);
	PLOTTILEBA(ThermoTiles, 20, MAPYSIZE + 3, 0xC9);
	PLOTTILEBA(ThermoTiles, 21, MAPYSIZE + 3, 0xCA);
	PLOTTILEBA(ThermoTiles, 19, MAPYSIZE + 4, 0xD8);
	PLOTTILEBA(ThermoTiles, 20, MAPYSIZE + 4, 0xD9);
	PLOTTILEBA(ThermoTiles, 21, MAPYSIZE + 4, 0xDA);
	
	// Draw the main bulk of the sidebar text
	for (nYBkg = 0; nYBkg < 12; nYBkg++) {
		PLOTTILEBA(ThermoTiles, 1, nYBkg + 1, 0xA0 + nYBkg);
	} 
	for (nYBkg = 12; nYBkg < 20; nYBkg++) {
		PLOTTILEBA(ThermoTiles, 1, nYBkg + 1, 0xB0 + nYBkg - 12);
	} 

	// Draw the right hand side of the thermometer
	for (nYBkg = 1; nYBkg < 20; nYBkg++) {
		PLOTTILEBA(ThermoTiles, 4, nYBkg, 0x9A);
	}
	PLOTTILEBA(ThermoTiles, 4, 0, 0x7A);
	PLOTTILEBA(ThermoTiles, 4, 20, 0x7B); 
	
	// Draw top and bottom halves of the thermometer bulbs
	for (nXBkg = 0; nXBkg < 4; nXBkg++){
		PLOTTILEBA(ThermoTiles, 1 + nXBkg, 21, 0x9C + nXBkg);
	}
	for (nXBkg = 0; nXBkg < 4; nXBkg++){
		PLOTTILEBA(ThermoTiles, 1 + nXBkg, 22, 0xAC + nXBkg);
	}
	
	// Draw an empty thermo stem
	for (nXBkg = 0; nXBkg < 20; nXBkg++) {
		PLOTTILEBA(ThermoTiles, 2, 20-nXBkg, 0x99);
		PLOTTILEBA(ThermoTiles, 3, 20-nXBkg, 0x99);
	}

	// Draw first bit of sidebar text
	PLOTTILEBA(ThermoTiles, 1, 0, 0x9B);
	
	// Draw thermometer tips
	PLOTTILEBA(ThermoTiles, 2, 0, 0x78);
	PLOTTILEBA(ThermoTiles, 3, 0, 0x78);
	
	for (nXBkg = 0; nXBkg < 16; nXBkg++) {
		PLOTTILEBA(ThermoTiles, 7 + nXBkg, 2, 0x80 + nXBkg);
	}
	
	// draw the current map
	MapScr_DrawMazeMap(LevelData);
	
	DC_FlushRange(Sprites, sizeof(Sprites));
	MI_DmaFill32(MAPSCR_DMA_CHANNEL, Sprites, 192, sizeof(Sprites)); // Clear to an 'idle pattern'.  I think.
	DC_FlushRange(Sprites, sizeof(Sprites));
	GXS_LoadOAM(Sprites, 0, sizeof(Sprites));
	       
    for (nTempHerder = 0; nTempHerder < MAX_HERDER; nTempHerder++) {
    	// Configure the map point of light for this herder
	    G2_SetOBJAttr(&Sprites[nTempHerder * 2],     // a pointer to the attributes
    	          0,               // x
                  0,               // y
                  0,               // priority
                  GX_OAM_MODE_NORMAL,       // OBJ mode
                  FALSE,           // mosaic
                  GX_OAM_EFFECT_NONE,       // flip/affine/no display/affine(double)
                  GX_OAM_SHAPE_8x8,       // size and shape
                  GX_OAM_COLORMODE_256,     // OBJ character data are in 256-color format
                  0,               // character name
                  0,               // color param
                  0                // affine param
	        );
        // Configure the sidebar for this herder
	    G2_SetOBJAttr(&Sprites[(nTempHerder * 2) + 1],     // a pointer to the attributes
    	          0,               // x
                  0,               // y
                  1,               // priority
                  GX_OAM_MODE_NORMAL,       // OBJ mode
                  FALSE,           // mosaic
                  GX_OAM_EFFECT_NONE,       // flip/affine/no display/affine(double)
                  GX_OAM_SHAPE_64x64,       // size and shape
                  GX_OAM_COLORMODE_16,     // OBJ character data are in 16-color format
                  0,               // character name
                  nTempHerder+2,   // color param
                  0                // affine param
        );
    }
    
    for (nTempSheep = 0; nTempSheep < MAX_SHEEP; nTempSheep++) {
	    G2_SetOBJAttr(&Sprites[(MAX_HERDER*2) + nTempSheep],     // a pointer to the attributes
    	          0,               // x
                  0,               // y
                  0,               // priority
                  GX_OAM_MODE_NORMAL,       // OBJ mode
                  FALSE,           // mosaic
                  GX_OAM_EFFECT_NONE,       // flip/affine/no display/affine(double)
                  GX_OAM_SHAPE_8x8,       // size and shape
                  GX_OAM_COLORMODE_256,     // OBJ character data are in 256-color format
                  0,               // character name
                  0,               // color param
                  0                // affine param
        );
    }
    
	DC_FlushRange(BkgTiles, 1024*2);
	GXS_LoadBG0Scr(BkgTiles, 0, 1024*2);
	DC_FlushRange(TextTiles, 1024*2);
	GXS_LoadBG1Scr(TextTiles, 0, 1024*2);
	DC_FlushRange(MapTiles, 1024*2);
	GXS_LoadBG2Scr(MapTiles, 0, 1024*2);
	DC_FlushRange(ThermoTiles, 1024*2);
	GXS_LoadBG3Scr(ThermoTiles, 0, 1024*2);
}


void MapScr_SetSpecialText(char *p) {
	strncpy(szSpecial, p, 16);
	szSpecial[16]='\0';		// 17 char buffer
}

// MapScr_RedrawTextLayer(void)
// Draws the needed text on the text screen.
void MapScr_RedrawTextLayer(u32 i_nScore, u32 i_nNumSheep, u32 i_nTimer)
{
	char szScore[8];
	char szSheep[4];
	char szTimer[4];
	int nNumPlayers = 0;
	int nHerder;
	
	MI_DmaFill32(MAPSCR_DMA_CHANNEL, TextTiles, 0xFFFFFFFF, sizeof(TextTiles)); // Clear the tile array

	sprintf(szScore, "%07d", i_nScore);
	sprintf(szSheep, "%02d", i_nNumSheep);
	sprintf(szTimer, ":%02d", i_nTimer);

	MapScr_DrawTextXY816(9, 19, "SCORE: ");
	MapScr_DrawTextXY816(8, 21, szSpecial);

	for (nHerder = 0; nHerder<MAX_HERDER; nHerder++){
		if ((PLAY_HUMAN == (herder[nHerder].type & PLAY_MASK)) ||
			(PLAY_COMPUTER == (herder[nHerder].type & PLAY_MASK))) {
			nNumPlayers++;
		}
	}
	if (bAllSettingsMade) 
	{
		MapScr_DrawTextXY816(16, 19, szScore);
		MapScr_DrawTextXY816(11, 15, szSheep);
		MapScr_DrawTextXY816(22, 15, szTimer);
		switch (nNumPlayers) 
		{
			case 4:
				MapScr_DrawTextXY816(27, 18, "4%");
			case 3:
				MapScr_DrawTextXY816(27, 12, "3$");
			case 2:
				MapScr_DrawTextXY816(27, 6, "2#");
			case 1:
				MapScr_DrawTextXY816(27, 0, "1@");
		}
	}
	MapScr_DrawTextXY816(10, 15, "^");
	
	DC_FlushRange(TextTiles, 1024*2);
	GXS_LoadBG1Scr(TextTiles, 0, 1024*2);
}

// MapScr_DrawMazeMap: Takes a pointer to a _leveldata structure to render
// Draws the maze to the screen.. NOTE: Assumes data is valid!
void MapScr_DrawMazeMap(struct _leveldata pInLevelData[LEVELYSIZE][LEVELXSIZE]) 
{
	int nXTile, nYTile;
	
	for (nYTile = 0; nYTile < MAPYSIZE; nYTile++) 
	{
		for (nXTile = 0; nXTile < MAPXSIZE; nXTile++) 
		{
			if ((!(pInLevelData[nYTile + MAPYOFF][nXTile + MAPXOFF].isPassable))
				&& (0 == pInLevelData[nYTile + MAPYOFF][nXTile + MAPXOFF].nDestructible))
			{
				PLOTTILEBA(MapTiles, nXTile+1, nYTile+3, 0x6f);
			}
			else {
				PLOTTILEBA(MapTiles, nXTile+1, nYTile+3, 0x6e);
			}
		}
	}
	DC_FlushRange(MapTiles, 1024*2);
	GXS_LoadBG2Scr(MapTiles, 0, 1024*2);
	
}

// MapScr_DrawOneThermo: Takes x position and height of the thermometer in pixels
void MapScr_DrawOneThermo(u8 i_nXpos, u8 i_nHeight) {
	// maximum size
	if (i_nHeight > 160) 
	{
		i_nHeight = 160;
	}
	
	int nSolidBlox = i_nHeight >> 3;	// div8
	int nCounter;
	
	// solid part
	for (nCounter = 0; nCounter < nSolidBlox; nCounter++) {
		PLOTTILEBA(ThermoTiles, i_nXpos, 20-nCounter, 0x90);
	}

	// partial character
	PLOTTILEBA(ThermoTiles, i_nXpos, 20-nCounter, 0x99 - (i_nHeight % 8));
	nCounter++;

	// empty part
	for (;nCounter < 20; nCounter++) {
		PLOTTILEBA(ThermoTiles, i_nXpos, 20-nCounter, 0x99);
	}

	// check for solid tip
	if (20 == nSolidBlox) 
	{
		PLOTTILEBA(ThermoTiles, i_nXpos, 0, 0x79);	
	}
	else 
	{
		PLOTTILEBA(ThermoTiles, i_nXpos, 0, 0x78);	
	}
}


// MapScr_DrawThermo: Draws the thermometers
// Input: Thermometer 1 value, as a u8.  Thermometer 2 value, as a u8.
void MapScr_DrawThermo(u8 uThermo1, u8 uThermo2, int bSnap){
	int diff1 = (uThermo1 - uCurrentThermo1);
	int diff2 = (uThermo2 - uCurrentThermo2);

	if (!bAllSettingsMade) 
	{
		uThermo1 = 0;
		uThermo2 = 0;
		bSnap = 1;
	}
	if (bSnap) 
	{
		uCurrentThermo1 = uThermo1;
		uCurrentThermo2 = uThermo2;
	}
	else 
	{
		if (0 != diff1) {
			if (diff1 > 0) 
			{
				uCurrentThermo1 += (diff1/20) + 1; 
			}
			else 
			{
				uCurrentThermo1 += (diff1/20) - 1;
			} 
		}
		if (0 != diff2) {
			if (diff2 > 0) 
			{
				uCurrentThermo2 += (diff2/20) + 1; 
			}
			else 
			{
				uCurrentThermo2 += (diff2/20) - 1;
			} 
		}
	}
	MapScr_DrawOneThermo(2, uCurrentThermo1);
	MapScr_DrawOneThermo(3, uCurrentThermo2);

	DC_FlushRange(ThermoTiles, 1024*2);
	GXS_LoadBG3Scr(ThermoTiles, 0, 1024*2);
}

// MapScr_PositionSprite: Takes a sprite number, and game x and y co-ords
// Converts game coordinates to map coordinates and positions the sprite on the screen.
void MapScr_PositionSprite(int i_nSprite, int i_nX, int i_nY){
	G2_SetOBJPosition(&Sprites[i_nSprite],
		((i_nX - MAZESPRSKIPX) / 4) + MAZESPROFFX,
		((i_nY - MAZESPRSKIPY) / 4) + MAZESPROFFY);
	
}

// MapScr_DrawAllSheep(void)
// Draws all sheep that it can find
void MapScr_DrawAllSheep(void) {
	int nSheep;
	
	if (bAllSettingsMade) 
	{
		for (nSheep = 0; nSheep<MAX_SHEEP; nSheep++){
			if (sheep[nSheep].type > 0) {
				G2_SetOBJCharName(&Sprites[nSheep+(MAX_HERDER*2)],MAX_HERDER);
				MapScr_PositionSprite(nSheep + (MAX_HERDER*2), sheep[nSheep].spr.x, sheep[nSheep].spr.y);
			}
			else {
				G2_SetOBJCharName(&Sprites[nSheep+(MAX_HERDER*2)],0xFF);
			}
		}
	}
}

// MapScr_DrawAllHerders(void)
// Draws all herders that it can find
void MapScr_DrawAllHerders(void) {
	int nHerder;
	
	if (bAllSettingsMade) 
	{
		for (nHerder = 0; nHerder<MAX_HERDER; nHerder++){
			if ((PLAY_HUMAN == (herder[nHerder].type & PLAY_MASK)) ||
				(PLAY_COMPUTER == (herder[nHerder].type & PLAY_MASK))) {
				G2_SetOBJCharName(&Sprites[nHerder*2],nHerder);
				MapScr_PositionSprite(nHerder*2, herder[nHerder].spr.x, herder[nHerder].spr.y);
				G2_SetOBJCharName(&Sprites[(nHerder*2)+1],MAX_HERDER+1 + (nHerder * 32));
				G2_SetOBJPosition(&Sprites[(nHerder*2)+1], herderCurrentLocations[nHerder].x, herderCurrentLocations[nHerder].y);
			}
			else {
				G2_SetOBJCharName(&Sprites[nHerder*2],0xff);
				G2_SetOBJCharName(&Sprites[(nHerder*2)+1],0xA0);
			}
		}

		DC_FlushRange(Sprites, sizeof(Sprites));
		GXS_LoadOAM(Sprites, 0, sizeof(Sprites));
	}
}


// MapScr_MoveHerders(void)
// Move herders to their locations
// bSnap: Set to 1 to snap players immediately
void MapScr_MoveHerders(int bSnap) {
	int nPlayerNum;
	int nPlacePlayer[4];
	int nNumPlayers = 0;

	// important to use the same sort as the end of game code!!
	GetPlayerSortedOrder(nPlacePlayer);

	// Now display the herders in place order.
	for (int nPlaceNum = 0; nPlaceNum < MAX_HERDER; nPlaceNum++) {
		nPlayerNum = nPlacePlayer[nPlaceNum];
		if (bSnap) {
			herderCurrentLocations[nPlayerNum].x = herderSidebarDestinations[nPlaceNum].x;
			herderCurrentLocations[nPlayerNum].y = herderSidebarDestinations[nPlaceNum].y;
		} else {
			int xdiff = (herderSidebarDestinations[nPlaceNum].x - herderCurrentLocations[nPlayerNum].x);
			int ydiff = (herderSidebarDestinations[nPlaceNum].y - herderCurrentLocations[nPlayerNum].y);
			if (0 != xdiff) {
				if (abs(xdiff) <= 2) {
					herderCurrentLocations[nPlayerNum].x = herderSidebarDestinations[nPlaceNum].x;
				} else {
					if (xdiff > 0) {
						herderCurrentLocations[nPlayerNum].x += (xdiff / 20) + 1;
					} else {
						herderCurrentLocations[nPlayerNum].x += (xdiff / 20) - 1;
					}
				}
			}
			if (0 != ydiff) {
				if (abs(ydiff) <= 2) {
					herderCurrentLocations[nPlayerNum].y = herderSidebarDestinations[nPlaceNum].y;
				} else {
					if (ydiff > 0) {
						herderCurrentLocations[nPlayerNum].y += (ydiff / 20) + 1;
					} else {
						herderCurrentLocations[nPlayerNum].y += (ydiff / 20) - 1;							
					}
				}
			}
		}
	}
}

// MapScr_GetHerderAddress
// Returns a pointer to the memory address where the 16-color detailed herder graphic should be stored
// If you write data to this memory, expect that it will be corrupted if your re-init the subscreen
// uHerder: Herder # for which you want the character data pointer (not a char*)
void* MapScr_GetHerderAddress(u8 uHerder) 
{
	// Gets the system defined start of OBJ character data + skips the mini-map sprites
	// plus skips 32 character names per herder, each character name referring to 64 bytes of 16-color
	// data.  You may write 2048 bytes to this pointer, and it will be drawn as an 8x8 grid of 8x8 tiles.
	// If you wish to have a 40 pixel wide sprite, you will need to use columns 0-4 and leave columns
	// 5-8 blank.  This is, apparently...your job.
	return (void*)((char*)G2S_GetOBJCharPtr() + ((MAX_HERDER + 1) + (uHerder * 32)) * 64);
}

// MapScr_SetHerderPalette
// Sets the palette for a given herder, centrallized here so changes don't need to be passed through
// the entire project.  Again, writing your own data here and then re-initializing the subscreen will
// corrupt the data
// uHerder: Herder number 0-3 to set
// pHerderData: pointer to 32 bytes of palette data
void MapScr_SetHerderPalette(u32 uHerder, void* pHerderData) 
{
	GXS_LoadOBJPltt(pHerderData, (uHerder+2) * 32, 32);
}

// MapScr_DrawFrame
// Draws one frame of the map screen
// uScore: u32 containing the local player's score
// uNumSheep: u32 containing number of sheep the player has control of
// uThermo1: u8 containing the left hand thermometer height
// uThermo2: u8 containing the right hand thermometer height
// bSnap: bool which is true if values are to be snapped
void MapScr_DrawFrame(u32 uScore, u32 uNumSheep, u32 uTimer, u8 uThermo1, u8 uThermo2, int bSnap) {
	if (bDoNotShow) 
	{
		return;	
	}
	if (bSnap) 
	{
		bAllSettingsMade = 1;
	}
	MapScr_RedrawTextLayer(uScore, uNumSheep, uTimer);
	MapScr_DrawMazeMap(LevelData);
	MapScr_DrawThermo(uThermo1, uThermo2, bSnap);	// also flushes BG3
	MapScr_DrawAllSheep();
	MapScr_MoveHerders(bSnap);
	MapScr_DrawAllHerders();
	
	GXS_SetVisiblePlane(GX_PLANEMASK_BG0 | GX_PLANEMASK_BG1 | GX_PLANEMASK_BG2 | GX_PLANEMASK_BG3 | GX_PLANEMASK_OBJ);
}

// MapScr_Init: Takes a pointer to the struct holding tile and palette pointers
// This function initializes the map screen, clearing all memory and releasing it's usage
// of all VRAM resources.  If you have messed with VRAM, there are better ways than this
// to restore the resources.  It takes the number of the background (0 - 7) that
// you want to use
void MapScr_Init(int nBackgroundNum, int nAssumeSpriteVRAM) {

	void* pTileSetA;
	void* pTileSetAPalette;
	void* pTileSetBA;
	void* pTileSetBAPalette;
	
	switch (nBackgroundNum) 
	{
		case 0:
			pTileSetA = (void*)Tileset_A0_Bitmap;
			pTileSetAPalette = (void*)Tileset_A0_Palette;
			pTileSetBA = (void*)Tileset_BA0_Tiles;
			pTileSetBAPalette = (void*)Tileset_BA0_Palette;
			pWorkingMapA = (unsigned short*)Tileset_BA0_Map;
			break;
		case 1:
			pTileSetA = (void*)Tileset_A1_Bitmap;
			pTileSetAPalette = (void*)Tileset_A1_Palette;
			pTileSetBA = (void*)Tileset_BA1_Tiles;
			pTileSetBAPalette = (void*)Tileset_BA1_Palette;
			pWorkingMapA = (unsigned short*)Tileset_BA1_Map;
			break;
		case 2:
			pTileSetA = (void*)Tileset_A2_Bitmap;
			pTileSetAPalette = (void*)Tileset_A2_Palette;
			pTileSetBA = (void*)Tileset_BA2_Tiles;
			pTileSetBAPalette = (void*)Tileset_BA2_Palette;
			pWorkingMapA = (unsigned short*)Tileset_BA2_Map;
			break;
		case 3:
			pTileSetA = (void*)Tileset_A3_Bitmap;
			pTileSetAPalette = (void*)Tileset_A3_Palette;
			pTileSetBA = (void*)Tileset_BA3_Tiles;
			pTileSetBAPalette = (void*)Tileset_BA3_Palette;
			pWorkingMapA = (unsigned short*)Tileset_BA3_Map;
			break;
		case 4:
			pTileSetA = (void*)Tileset_A4_Bitmap;
			pTileSetAPalette = (void*)Tileset_A4_Palette;
			pTileSetBA = (void*)Tileset_BA4_Tiles;
			pTileSetBAPalette = (void*)Tileset_BA4_Palette;
			pWorkingMapA = (unsigned short*)Tileset_BA4_Map;
			break;
		case 5:
			pTileSetA = (void*)Tileset_A5_Bitmap;
			pTileSetAPalette = (void*)Tileset_A5_Palette;
			pTileSetBA = (void*)Tileset_BA5_Tiles;
			pTileSetBAPalette = (void*)Tileset_BA5_Palette;
			pWorkingMapA = (unsigned short*)Tileset_BA5_Map;
			break;
		case 6:
			pTileSetA = (void*)Tileset_A6_Bitmap;
			pTileSetAPalette = (void*)Tileset_A6_Palette;
			pTileSetBA = (void*)Tileset_BA6_Tiles;
			pTileSetBAPalette = (void*)Tileset_BA6_Palette;
			pWorkingMapA = (unsigned short*)Tileset_BA6_Map;
			break;
		case 7:
			pTileSetA = (void*)Tileset_A7_Bitmap;
			pTileSetAPalette = (void*)Tileset_A7_Palette;
			pTileSetBA = (void*)Tileset_BA7_Tiles;
			pTileSetBAPalette = (void*)Tileset_BA7_Palette;
			pWorkingMapA = (unsigned short*)Tileset_BA7_Map;
			break;
	}
	
	MapScr_InitTileset(&TilesetA, pTileSetA, pTileSetAPalette);	
	MapScr_InitTileset(&TilesetBa, pTileSetBA, pTileSetBAPalette);	
	MapScr_InitTileset(&TilesetBb, (void*)Tileset_B_V6b_Tiles, (void*)Tileset_B_V6b_Palette);	
	MapScr_InitTileset(&TilesetBc, (void*)Tileset_B_V6c_Tiles, (void*)Tileset_B_V6c_Palette);	
	
	if (nAssumeSpriteVRAM) 
	{
		pSpriteSrc = NULL;
		pSpritePalette = NULL;
	}
	else 
	{
		pSpriteSrc = (void*)TilesetC_gfx;
		pSpritePalette = (void**)TilesetC_pal;
	}
	
	MapScr_ResetVRAM();
	
	bDoNotShow = 0;
	bAllSettingsMade = 0;
	bMapActive = 1;
}

void MapScr_EndMap() {
	bMapActive = 0;
}

int MapScr_isMapActive() {
	return bMapActive;
}

// hacky little function to get the text buffer onto the screen
// without any other functionality (only called externally). it also
// copies to BG2 for shadow (used by the high score code)
void MapScr_FlushTextOnly() {
	DC_FlushRange(TextTiles, 1024*2);
	GXS_LoadBG1Scr(TextTiles, 0, 1024*2);
	GXS_LoadBG2Scr(TextTiles, 0, 1024*2);
}

void MapScr_EraseTextOnly() {
	MI_CpuFill32(TextTiles, 0xffffffff, 2048);
	MapScr_FlushTextOnly();
}

// A nice little utility function that turns off all output to the map screen without turning off
// the controller, which turns it to FSCKING WHITE!
void MapScr_ShowBlack(void) 
{
	GXS_SetVisiblePlane(GX_PLANEMASK_NONE);
	MI_DmaClear32(MAPSCR_DMA_CHANNEL, (void*)HW_DB_BG_PLTT, 0x200);
	bDoNotShow = 1;
}
