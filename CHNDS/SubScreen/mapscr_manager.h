/****************************************/
/* Cool Herders                         */
/* http://harmlesslion.com              */
/* Copyright 2009 HarmlessLion LLC      */
/* mapscr_manager.h                     */
/****************************************/

#define LEVELXSIZE 21
#define LEVELYSIZE 16

#define MAX_HERDER 4

typedef struct MapScr_TileSet {
	u32		nVramTile; 			// The VRAM tile where this tileset starts
	void*	pbMemoryAddress;	// The Main Memory address where the tile data can be found
	u8		nPalette;			// The palette number used if this is a 16-color tileset
	void*	pbPaletteAddress;	// The address of the palette in main memory
} MapScr_TileSet;

typedef struct MapScr_FontInfo {
	char cCharName;
	u16 uUpper;
	u16 uLower;
} MapScr_FontInfo;

typedef struct MapScr_HerderSidebar {
	int x;
	int y;
} MapScr_HerderSidebar;

void MapScr_Init(int nBackgroundNum, int nAssumeSpriteVRAM);
void MapScr_InitTileset(MapScr_TileSet* i_pTileSet, void* i_pbMemory, void* i_pbPalette);
void MapScr_LoadTileset(MapScr_TileSet* i_pTileSet, u32 i_nTileSize, u32 i_nPaletteSize);
void MapScr_DrawTextXY816(u8 x, u8 y, char* pszText);
void MapScr_RedrawTextLayer(u32 i_nScore, u32 i_nNumSheep, u32 i_nTimer);
void MapScr_DrawMazeMap(struct _leveldata pInLevelData[LEVELYSIZE][LEVELXSIZE]);
void MapScr_DrawFrame(u32 uScore, u32 uNumSheep, u32 uTimer, u8 uThermo1, u8 uThermo2, int bSnap);
void MapScr_ResetSubScr(int bOkToMakeVisible);
void MapScr_DrawOneThermo(u8 i_nXpos, u8 i_nHeight);
void MapScr_DrawThermo(u8 uThermo1, u8 uThermo2, int bSnap);
void MapScr_ResetVRAM(void);
void MapScr_DrawAllSheep(void);
void MapScr_DrawAllHerders(void);
void MapScr_PositionSprite(int i_nSprite, int i_nX, int i_nY);
void MapScr_DrawMic(u32 i_nMicLevel);
void MapScr_MoveHerders(int bSnap);
void* MapScr_GetHerderAddress(u8 uHerder);
void MapScr_FlushTextOnly();
void MapScr_EraseTextOnly();
void MapScr_EndMap();
int MapScr_isMapActive();
void MapScr_SetSpecialText(char *p);

void MapScr_SetHerderPalette(u32 uHerder, void* pHerderData);
void MapScr_ShowBlack(void);

