/****************************************/
/* Cool Herders                         */
/* A demented muse by Brendan Wiese     */
/* http://starbow.org                   */
/* Programmed by Mike Brent             */
/* http://harmlesslion.com              */
/* Copyright 2000-2005 Brendan Wiese &  */
/* Mike Brent.                          */
/* font.h                               */
/****************************************/

extern char fontstr[];
extern const int LGX;
extern const int LGY;
extern const int SMX;
extern const int SMY;

// used for all the old code
#define DEFAULT_FONT_Z 1024.0f

// New code all uses Z
void DrawFontZ(float z, int large, int x, int y, uint32 color, char *str);
void DrawFontBackgroundZ(float z, int large, int x, int y, uint32 color, uint32 bgcolor, char *str);
void DrawFontMenuZ(float z, int large, int x, int y, uint32 color, char *str);
void VDrawFontZ(float z, int large, int x, int y, uint32 color, char *str);
void CenterDrawFontZ(float z, int large, int y, uint32 color, char *str);
void CenterDrawFontBackgroundZ(float z, int large, int y, uint32 color, uint32 bgcolor, char *str);
void CenterDrawFontBreaksZ(float z, int large, int y, uint32 color, char *str);
void CenterDrawFontBackgroundBreaksZ(float z, int minwidth, int y, uint32 color, uint32 bgcolor, char *str);

// Wrappers for the old pre-Z versions
#define DrawFont(large,x,y,color,str) DrawFontZ(DEFAULT_FONT_Z, large, x, y, color, str)
#define DrawFontMenu(large,x,y,color,str) DrawFontMenuZ(DEFAULT_FONT_Z, large, x, y, color, str)
#define VDrawFont(large,x,y,color,str) VDrawFontZ(DEFAULT_FONT_Z, large, x, y, color, str)
#define CenterDrawFont(large,y,color,str) CenterDrawFontZ(DEFAULT_FONT_Z, large, y, color, str)
#define DrawFontZ(z, large, x, y, color, str) DrawFontBackgroundZ(z, large, x, y, color, 0, str)

// Doesn't make sense to have z versions of these
void DrawFontTxr(pvr_ptr_t txr, int large, int x, int y, char *str);
void EraseFontTxr(pvr_ptr_t txr, int large, int x, int y, int cnt);

