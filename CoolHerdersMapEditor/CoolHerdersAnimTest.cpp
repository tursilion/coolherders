// CoolHerdersAnimTest.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "CoolHerdersAnimTest.h"
#include "CoolHerdersAnimTestDlg.h"
#include "c:\work\imgsource\_isource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern unsigned char *hImg[4][4];	// tile page, animation (only tp 1-3 valid in this file)
extern char MapData[21][16][3];		// x, y, data
extern char TileData[3][25];		// tile page, index
extern int nLevel;					// level to save as
extern char fnbuf[1024];

/////////////////////////////////////////////////////////////////////////////
// CCoolHerdersAnimTestApp

BEGIN_MESSAGE_MAP(CCoolHerdersAnimTestApp, CWinApp)
	//{{AFX_MSG_MAP(CCoolHerdersAnimTestApp)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG
	ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCoolHerdersAnimTestApp construction

CCoolHerdersAnimTestApp::CCoolHerdersAnimTestApp()
{
	_ISInitialize("{imgsource guid goes here}");
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CCoolHerdersAnimTestApp object

CCoolHerdersAnimTestApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CCoolHerdersAnimTestApp initialization

BOOL CCoolHerdersAnimTestApp::InitInstance()
{
	AfxEnableControlContainer();

	// Standard initialization
	// If you are not using these features and wish to reduce the size
	//  of your final executable, you should remove from the following
	//  the specific initialization routines you do not need.

#ifdef _AFXDLL
	Enable3dControls();			// Call this when using MFC in a shared DLL
#else
	Enable3dControlsStatic();	// Call this when linking to MFC statically
#endif

	CCoolHerdersAnimTestDlg dlg;
	m_pMainWnd = &dlg;
	int nResponse = dlg.DoModal();
	if (nResponse == IDOK)
	{
		SaveItAll();
	}

	for (int i=0; i<4; i++) {
		for (int j=0; j<4; j++) {
			if (NULL != hImg[i][j]) {
				free(hImg[i][j]);
				hImg[i][j]=NULL;
			}
		}
	}
	
	// Since the dialog has been closed, return FALSE so that we exit the
	//  application, rather than start the application's message pump.
	return FALSE;
}

bool CCoolHerdersAnimTestApp::SaveItAll()
{
	static bool fAlreadyBackedUp=false;

	// Save the images and map data
	if (-1 == nLevel) {
		// This means we didn't LOAD any level, so it should be save to SAVE as any
		nLevel=9;
	}

	char szbuf[1024];
	char buf[1024];		// well named, eh??
	FILE *fp;
	HISDEST hDst;
	bool fRet=true;

	// Write the PNG buffers, with a backup file for each 
	for (int t=0; t<3; t++) {
		for (int a=0; a<4; a++) {
			
			if (NULL == hImg[t][a]) {
				continue;
			}

			sprintf(szbuf, "%slevel%d%c%d.png", fnbuf, nLevel, t+'a', a+1);

			if (!fAlreadyBackedUp) {
				strcpy(buf, szbuf);
				strcat(buf, ".bak");
				MoveFileEx(szbuf, buf, MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH);
			}
			
			hDst=_ISOpenFileDest(szbuf);
			if (NULL == hDst) {
				sprintf(buf, "Failed to write %s", szbuf);
				AfxMessageBox(buf);
			} else {
				if (0 == _ISWritePNG(hDst, hImg[t][a], 256, 256, 1024, 8, 0, NULL, 6, 0, 0, NULL)) {
					sprintf(buf, "Failed to write %s", szbuf);
					AfxMessageBox(buf);
					fRet=false;
				}
				_ISCloseDest(hDst);
			}
		}
	}

	// Write out the map in our magic format
	sprintf(szbuf, "%slevel%d.txt", fnbuf, nLevel);
	if (!fAlreadyBackedUp) {
		strcpy(buf, szbuf);
		strcat(buf, ".bak");
		MoveFileEx(szbuf, buf, MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH);
	}

	fp=fopen(szbuf, "w");
	if (NULL == fp) {
		char buf[1024];
		sprintf(buf, "Failed to write layout file %s", szbuf);
		AfxMessageBox(buf);
		fRet=false;
	} else {
		for (int y=0; y<16; y++) {
			fputc('\"', fp);

			for (int x=0; x<21; x++) {
				for (int m=0; m<3; m++) {	// metadata
					fputc(MapData[x][y][m], fp);
				}
			}

			fputs("\"\n", fp);
		}

		// Now write the tilemap info that only the map editor cares about
		fputc('>', fp);
		for (int a=0; a<3; a++) {
			for (int b=0; b<25; b++) {
				fputc(TileData[a][b], fp);
			}
		}
		fputc('\n', fp);
		fclose(fp);
	}

	fAlreadyBackedUp=true;

	return fRet;
}
