// Handlers for the top screen font
extern char PalTable[];

// functions
void WriteFont2D(int r, int c, char *psz);
void CenterWriteFont2D(int r, int c, char *psz);
void Clear2D();
void SetOffset2D(int x, int y);
void CenterWriteFontBreak2D(int r, int c, char *psz);
int  ReparseName(unsigned char *p, int nLen);

// character functions
void DrawReady();
void DrawGo();