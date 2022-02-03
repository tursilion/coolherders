typedef struct MenuScr_TileSet {
	u32		nVramTile; 			// The VRAM tile where this tileset starts
	void*	pbMemoryAddress;	// The Main Memory address where the tile data can be found
	u32		nPalette;			// The palette number used if this is a 16-color tileset
	void*	pbPaletteAddress;	// The address of the palette in main memory
} MenuScr_TileSet;

typedef struct MenuScr_FontInfo {
	char cCharName;
	u16 uUpper;
	u16 uLower;
} MenuScr_FontInfo;

typedef struct MenuScr_FontInfo16 {
	char cCharName;
	u16 uUpperLeft;
	u16 uUpperRight;
	u16 uLowerLeft;
	u16 uLowerRight;
} MenuScr_FontInfo16;

typedef struct MenuScr_Options 
{
	int bAutosave;
	int nSoundVol;
	int nMusicVol;
} MenuScr_Options;

enum MenuScr_RenderDir
{
	rdNone,
	rdAppearing,
	rdDisappearing,
	rdWaitAppear,
	rdWaitDisappear,
	rdShown,
};

typedef struct Menuscr_MenuItem 
{
	int uInUse;
	enum MenuScr_RenderDir renderDir;
	int x, y, w, h;
	int curw, curh;
	int keyup, keydn, keylt, keyrt; // Button to select on D-pad action
	int menuval;
	int menualtval; // This value is returned if the the L+R buttons are held down
	int renderwait;
	int renderdelay;
	int renderdelaywork;
	char szMenuString[256];
} MenuScr_MenuItem;

#define MENU_X1CONVERT 4
#define MENU_Y1CONVERT 4
#define MENU_X2CONVERT -4
#define MENU_Y2CONVERT -4

void MenuScr_Init(void);
void MenuScr_InitMenu(int i_nMenu);
void MenuScr_InitTileset(MenuScr_TileSet* i_pTileSet, void* i_pbMemory, void* i_pbPalette);
void MenuScr_LoadTileset(MenuScr_TileSet* i_pTileSet, u32 i_nTileSize, u32 i_nPaletteSize);
void MenuScr_DrawCenteredTextXY1616(int x, int y, char* pszText, int bForceUpper);
void MenuScr_DrawTextXY1616(int x, int y, char* pszText, int bForceUpper);
void MenuScr_DrawCenteredTextXY816(char *pszText);
void MenuScr_DrawTextXY816(u8 x, u8 y, char* pszText, int bUnselOption);
void MenuScr_SnapMenuItems();
int MenuScr_DrawFrame(int *pSelButton);
void MenuScr_ResetMenu(void);
void MenuScr_FlushTextOnly();

void MenuScr_ResetSubScr(void);
int MenuScr_IsInMenu(void);
void MenuScr_SetupOptions(int i_bStartup);
void MenuScr_DoOptions(int i_bDoOptions);
void MenuScr_DoBattleOptions(int i_bDoOptions);
void MenuScr_DoSystemInfo(int i_bSysInfo);
void MenuScr_DoWorldSelect(int i_bWorld, int i_nWorld, int i_nMax);
void MenuScr_DoPlayerSelect(int i_bPlayer, int i_nPlayer);
void MenuScr_DoMusicSelect(int i_bEnter);
void MenuScr_UpdateBubbleSize(int i_nNumber, int i_w, int i_h);
void MenuScr_UpdateMenuString(int i_nNumber, char * i_MenuString);
void MenuScr_SetSelOption(int i_nOption);
int MenuScr_GetSelOption();
void MenuScr_CloseMenu(int i_bImmediate);
int MenuScr_DrawMenuItems(void);

extern MenuScr_Options MenuScr_CurrentOptions;

