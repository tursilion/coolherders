// CoolHerdersAnimTestDlg.cpp : implementation file
//
// Screens are 21x16 
// 3 chars per tile.
// First is the page, 1-3. 3 is always destructible impassible, 1-2 are freeform
// Second is the type: .=flat, passable; -=flat, unpassable; /=3D, unpassable
// Third is the tile number, A-Y (goes across)

#include "stdafx.h"
#include "CoolHerdersAnimTest.h"
#include "CoolHerdersAnimTestDlg.h"
#include "c:\work\imgsource\_isource.h"
#include "Framesel.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern CCoolHerdersAnimTestApp theApp;

char fnbuf[1024];
unsigned char *hImg[4][4];		// [0-2 are tiles, 3 is workspace][frmae 0-3]
HBITMAP hBmp[4][4];				// cached converted bitmaps for display
char TileData[3][25];			// Terrain style for each tile (not stored in CH)
char MapData[21][16][3];		// x, y, data
char UndoMapData[21][16][3];
unsigned int width, height;
RGBQUAD pal[256];
UINT_PTR hTimer=0;
int nLevel=-1;
int nFrame=0;

// Sync a texture with it's hBmp
void SyncTexture(int page, int frame) {
	unsigned char rgb[256*256*3];

	// alpha processing - ensure all parts with transparent alpha are magenta
	for (int x=0; x<256*256*4; x+=4) {
		if (0 == hImg[page][frame][x+3]) {
			hImg[page][frame][x]=0xff;	// red
			hImg[page][frame][x+1]=0;	// green
			hImg[page][frame][x+2]=0xff;// blue
		}
	}

	// now get a new hBmp for this guy
	_ISSplitRGBAToRGBPlusA(hImg[page][frame], 256, 256, rgb, NULL);
	if (NULL != hBmp[page][frame]) {
		DeleteObject(hBmp[page][frame]);
	}

	CDC *myDC=AfxGetMainWnd()->GetDC();
	if (NULL == myDC) {
		AfxMessageBox("Resources are extremely low! Save your work, quit, and restart this program");
		return;
	}
	hBmp[page][frame]=_ISRGBToHBITMAP(rgb, 256, 256, NULL, *myDC);
	AfxGetMainWnd()->ReleaseDC(myDC);
}

void SyncMainPage(int frame) {
	if (NULL != hBmp[3][frame]) {
		DeleteObject(hBmp[3][frame]);
	}
	CDC *myDC=AfxGetMainWnd()->GetDC();
	if (NULL == myDC) {
		AfxMessageBox("Resources are extremely low! Save your work, quit, and restart this program");
		return;
	}
	hBmp[3][frame]=_ISRGBToHBITMAP(hImg[3][frame], 640, 480, NULL, *myDC);
	AfxGetMainWnd()->ReleaseDC(myDC);
}

// Redraw the map hImg[3] for animation index idx
void CCoolHerdersAnimTestDlg::RedrawMap(int idx) {
	int x, y;
	int tx, ty, sx, sy;
	unsigned char hTmp[48*48*4];
	unsigned char hTmp2[48*48*4];
	bool fUseTmp2=false;
	bool fShowSafe=false;

	CButton *pWnd=(CButton*)GetDlgItem(IDC_SAFE);
	if (NULL != pWnd) {
		if (pWnd->GetCheck()==BST_CHECKED) {
			fShowSafe=true;
		}
	}

	if (NULL == hImg[3][idx]) {
		return;
	}
	
	memset(hImg[3][idx], 0, 640*480*3);
	
	for (y=0; y<16; y++) {
		for (x=0; x<21; x++) {
			bool fFlag=false;
			fUseTmp2=false;

			tx=x*32-24;
			ty=y*32-24;

			if (NULL == hImg[MapData[x][y][0]-'1'][idx]) {
				continue;
			}

			if (0 != MapData[x][y][0]) {
				if (MapData[x][y][0] != '3') {
					// Tile page 1 or 2 is normal
					sx=((MapData[x][y][2]-'A')%5)*48;
					sy=((MapData[x][y][2]-'A')/5)*48;
					_ISCropImage(hImg[MapData[x][y][0]-'1'][idx], 256, 256, hTmp, sx, sy, 48, 48, 4);
					switch (m_Terrain) {
					case 0: fFlag=MapData[x][y][1]=='.'; break;
					case 1: fFlag=MapData[x][y][1]=='-'; break;
					case 2: fFlag=MapData[x][y][1]=='/'; break;
					default: fFlag=false;
					}
				} else {
					// Tile page 3 requires a tile from page 1 or 2, AND the tile from 3 on top of it
					// The 'flag' byte indicates for us which tile, A-Z. Uppercase is page 1, lowercase
					// is page 2.
					if (isalpha(MapData[x][y][1])) {
						if (isupper(MapData[x][y][1])) {
							sx=((MapData[x][y][1]-'A')%5)*48;
							sy=((MapData[x][y][1]-'A')/5)*48;
							_ISCropImage(hImg[0][idx], 256, 256, hTmp, sx, sy, 48, 48, 4);
						} else {
							sx=((MapData[x][y][1]-'a')%5)*48;
							sy=((MapData[x][y][1]-'a')/5)*48;
							_ISCropImage(hImg[1][idx], 256, 256, hTmp, sx, sy, 48, 48, 4);
						}
						fUseTmp2=true;
					}

					sx=((MapData[x][y][2]-'A')%5)*48;
					sy=((MapData[x][y][2]-'A')/5)*48;
					_ISCropImage(hImg[2][idx], 256, 256, hTmp2, sx, sy, 48, 48, 4);

					// check the color flag - always flat
					if (m_Terrain == 0) {
						fFlag=true;
					}
				}

				if (fFlag) {
					// Tint the whole image green if highlighted
					for (int i=1; i<48*48*4; i+=4) {
						hTmp[i]=min(hTmp[i]+128, 255);
					}
				}
				if (fShowSafe) {
					// Tint it red if outside the safe area
					if ((y<3)||(x>18)||(x<2)||(y>13)) {
						for (int i=0; i<48*48*4; i+=4) {
							hTmp[i]=min(hTmp[i]+128, 255);
						}
					}
					// Tint the sheep and player starts blue
					if ((y==3)&&(x==2)) {
						for (int i=2; i<48*48*4; i+=4) {
							hTmp[i]=min(hTmp[i]+128, 255);
						}
					}
					if ((y==3)&&(x==18)) {
						for (int i=2; i<48*48*4; i+=4) {
							hTmp[i]=min(hTmp[i]+128, 255);
						}
					}
					if ((y==13)&&(x==18)) {
						for (int i=2; i<48*48*4; i+=4) {
							hTmp[i]=min(hTmp[i]+128, 255);
						}
					}
					if ((y==13)&&(x==2)) {
						for (int i=2; i<48*48*4; i+=4) {
							hTmp[i]=min(hTmp[i]+128, 255);
						}
					}
					if ((y==3)&&(x==18)) {
						for (int i=2; i<48*48*4; i+=4) {
							hTmp[i]=min(hTmp[i]+128, 255);
						}
					}
					if ((y==8)&&(x==10)) {
						for (int i=2; i<48*48*4; i+=4) {
							hTmp[i]=min(hTmp[i]+128, 255);
						}
					}
				}

				_ISAlphaBlendRGBA(hImg[3][idx], 640, 480, hTmp, 48, 48, tx, ty);
				if ((BST_UNCHECKED ==ctlHideBreak.GetCheck()) && (fUseTmp2)) {
					_ISAlphaBlendRGBA(hImg[3][idx], 640, 480, hTmp2, 48, 48, tx, ty);
				}
			}
		}
	}

	SyncMainPage(idx);
}

/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	//{{AFX_DATA(CAboutDlg)
	enum { IDD = IDD_ABOUTBOX };
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAboutDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	//{{AFX_MSG(CAboutDlg)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
	//{{AFX_DATA_INIT(CAboutDlg)
	//}}AFX_DATA_INIT
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAboutDlg)
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	//{{AFX_MSG_MAP(CAboutDlg)
		// No message handlers
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCoolHerdersAnimTestDlg dialog

CCoolHerdersAnimTestDlg::CCoolHerdersAnimTestDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CCoolHerdersAnimTestDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CCoolHerdersAnimTestDlg)
	m_Terrain = 3;	// default to 'none'
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

	fButtonDown=false;
	fMapDraw=false;
	nCurrentTile=-1;
	nCurrentTilePage=-1;
	nCurrentTileMode=-1;

	for (int i=0; i<3; i++) {
		for (int j=0; j<4; j++) {
			hImg[i][j]=(unsigned char*)calloc(1, 256*256*4);
			for (int k=3; k<256*256*4; k+=4) {
				hImg[i][j][k]=255;
			}
			hBmp[i][j]=NULL;
		}
	}
	// Main backdrop
	for (i=0; i<4; i++) {
		hImg[3][i]=(unsigned char*)calloc(1, 640*480*3);
		hBmp[3][i]=NULL;
	}

	for (i=0; i<21; i++) {
		for (int j=0; j<16; j++) {
			MapData[i][j][0]='1';
			MapData[i][j][1]='.';
			MapData[i][j][2]='A';
		}
	}
}

void CCoolHerdersAnimTestDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CCoolHerdersAnimTestDlg)
	DDX_Control(pDX, IDC_HIDEBREAK, ctlHideBreak);
	DDX_Control(pDX, IDC_COORD, m_Coord);
	DDX_Control(pDX, IDC_ANIM, m_ctlAnim);
	DDX_Radio(pDX, IDC_FLAT, m_Terrain);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CCoolHerdersAnimTestDlg, CDialog)
	//{{AFX_MSG_MAP(CCoolHerdersAnimTestDlg)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_TIMER()
	ON_WM_DROPFILES()
	ON_WM_CLOSE()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_BN_CLICKED(IDC_FRAME1, OnFrame1)
	ON_BN_CLICKED(IDC_FRAME2, OnFrame2)
	ON_BN_CLICKED(IDC_FRAME3, OnFrame3)
	ON_BN_CLICKED(IDC_FRAME4, OnFrame4)
	ON_WM_MOUSEMOVE()
	ON_BN_CLICKED(IDC_FLAT, OnFlat)
	ON_BN_CLICKED(IDC_3DIMPASS, On3dimpass)
	ON_BN_CLICKED(IDC_IMPASS, OnImpass)
	ON_BN_CLICKED(IDC_NOTERRAIN, OnNoterrain)
	ON_BN_CLICKED(IDC_UNDO, OnUndo)
	ON_BN_CLICKED(IDC_SAFE, OnSafe)
	ON_BN_CLICKED(IDC_ABORT, OnAbort)
	ON_BN_CLICKED(IDC_SAVE, OnSave)
	ON_BN_CLICKED(IDC_HIDEBREAK, OnHidebreak)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCoolHerdersAnimTestDlg message handlers

BOOL CCoolHerdersAnimTestDlg::OnInitDialog()
{
	bool fFlagBreakUpgrade=false;

	CDialog::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		CString strAboutMenu;
		strAboutMenu.LoadString(IDS_ABOUTBOX);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon
	
	RECT myRect;
	int extraW=0, extraH=0;

	GetWindowRect(&myRect);

	extraW+=GetSystemMetrics(SM_CXBORDER)*2;
	extraH+=GetSystemMetrics(SM_CYBORDER)*2;
	extraH+=GetSystemMetrics(SM_CYCAPTION);

	SetWindowPos(NULL, 0, 0, 900+extraW, 770+extraH, SWP_NOMOVE|SWP_NOZORDER);

	DragAcceptFiles();

	OPENFILENAME fn;
	
	ZeroMemory(&fn, sizeof(fn));
	fn.lStructSize=sizeof(fn);
	fn.hwndOwner=GetSafeHwnd();
	fn.lpstrFilter="(Any File)\0*.*\0";
	fn.nMaxFile=1024;
	fn.lpstrFile=fnbuf;
	fn.nFilterIndex=1;
	fn.lpstrTitle="Select working folder...";
	fnbuf[0]='\0';
	fn.Flags=OFN_FILEMUSTEXIST;
	
	if (!GetOpenFileName(&fn)) {
		EndDialog(IDCANCEL);
	}

	fnbuf[fn.nFileOffset]='\0';

	// Try to load any data that might already be here. Files of interest:
	// -- level[0-9][a-c][1-4].png
	// First digit is the level number
	// Second is the tile page
	// Third is the animation frame (and on load, is optional, meaning same one in all four)
	// -- level[0-9].txt
	// Text file containing the layout data in ASCII mode

	// look for any of the files
	for (int x=0; x<10 && nLevel==-1; x++) {
		char szbuf[1024];
		FILE *fp;

		for (int c='a'; c<'d'; c++) {
			sprintf(szbuf, "%slevel%d%c.png", fnbuf, x, c);
			fp=fopen(szbuf, "rb");
			if (NULL != fp) {
				HISSRC src;
				HGLOBAL htmpImg;

				fclose(fp);
				src=_ISOpenFileSource(szbuf);
				htmpImg=_ISReadPNG(src, &width, &height, 32, pal);
				_ISCloseSource(src);

				if (NULL != htmpImg) {
					if ((width!=256)||(height!=256)) {
						char mBuf[128];
						sprintf(mBuf, "Can't load %s because it is not 256x256", szbuf);
						AfxMessageBox(mBuf);
					} else {
						for (int i=0; i<4; i++) {
							memcpy(hImg[c-'a'][i], htmpImg, 256*256*4);
							//SyncTexture(c-'a',i);
						}
						nLevel=x;
					}

					GlobalFree(htmpImg);
				}
			} else {
				for (int i=0; i<4; i++) {
					sprintf(szbuf, "%slevel%d%c%d.png", fnbuf, x, c, i+1);
					fp=fopen(szbuf, "rb");
					if (NULL != fp) {
						HISSRC src;
						HGLOBAL htmpImg;

						fclose(fp);
						src=_ISOpenFileSource(szbuf);
						htmpImg=_ISReadPNG(src, &width, &height, 32, pal);
						_ISCloseSource(src);

						if (NULL != htmpImg) {
							if ((width!=256)||(height!=256)) {
								char mBuf[128];
								sprintf(mBuf, "Can't load %s because it is not 256x256", szbuf);
								AfxMessageBox(mBuf);
							} else {
								memcpy(hImg[c-'a'][i], htmpImg, 256*256*4);
								//SyncTexture(c-'a',i);
								nLevel=x;
							}

							GlobalFree(htmpImg);
						}
					}
				}
			}
		}

		sprintf(szbuf, "%slevel%d.txt", fnbuf, x);
		memset(TileData, '.', sizeof(TileData));	// set all terrain to flat. We'll guess during the load, then look for a line of absolute data
		fp=fopen(szbuf, "r");
		if (NULL != fp) {
			int ix=0, y=0, d=0;
			while (!feof(fp)) {
				int ch=fgetc(fp);
				if ((ch<=' ')||(ch=='\"')) continue;

				switch (d) {
				case 0:	// tile page 1-3
					if ((ch < '1') || (ch > '3')) {
						char buf[128];
						sprintf(buf, "Bad character '%c' at row %d, col %d in %s", ch, y, ix, fnbuf);
						AfxMessageBox(buf);
						fseek(fp, 0, SEEK_END);
					}
					break;
				case 1:	// permissions (.-/)
					if (MapData[ix][y][0]=='3') {
						if ((toupper(ch) < 'A') || (toupper(ch) > 'Y')) {
							if (NULL == strchr(".-/", ch)) {
								char buf[128];
								sprintf(buf, "Bad character '%c' at row %d, col %d in %s", ch, y, ix, fnbuf);
								AfxMessageBox(buf);
								fseek(fp, 0, SEEK_END);
							} else {
								// wipe out the badly formatted breakable - set it to tile 1-Y
								MapData[ix][y][0]='1';
								MapData[ix][y][1]='.';
								// Skip the next char
								fgetc(fp);
								d++;
								ch='Y';

								fFlagBreakUpgrade=true;
							}
						}
					} else {
						if (NULL == strchr(".-/", ch)) {
							char buf[128];
							sprintf(buf, "Bad character '%c' at row %d, col %d in %s", ch, y, ix, fnbuf);
							AfxMessageBox(buf);
							fseek(fp, 0, SEEK_END);
						}
					}
					break;
				case 2:	// tile index (A-Y)
					if ((ch < 'A') || (ch > 'Y')) {
						char buf[128];
						sprintf(buf, "Bad character '%c' at row %d, col %d in %s", ch, y, ix, fnbuf);
						AfxMessageBox(buf);
						fseek(fp, 0, SEEK_END);
					} else {
						// Set guessed tiledata based on how it's used in the map
						// If there is a line of absolute data at the end, that will override this
						TileData[MapData[ix][y][0]-'1'][ch-'A']=MapData[ix][y][1];
					}
					break;
				}
				MapData[ix][y][d]=ch;
				d++;
				if (d>2) {
					d=0;
					ix++;
					if (ix>20) {
						ix=0;
						y++;
						if (y>15) {
							break;
						}
					}
				}
			}
			if (!feof(fp)) {
				// if that's not the end, read up to two lines looking for tile terrain data
				// used by this map editor ;)
				char buf[128];

				memset(buf, 0, 128);
				fgets(buf, 128, fp);	// this is expected to finish the previous line
				if (buf[0] != '>') {
					fgets(buf, 128, fp);
				}
				if (buf[0] == '>') {
					int idx=1;	// start after the '>'
					// we found our line - read it in
					for (int a=0; a<2; a++) {
						for (int b=0; b<25; b++) {
							TileData[a][b]=buf[idx++];
						}
					}
				}
			}

			fclose(fp);
			nLevel=x;
		}
	}

	for (int i=0; i<3; i++) {
		for (int j=0; j<4; j++) {
			SyncTexture(i,j);
		}
	}
	for (i=0; i<4; i++) {
		RedrawMap(i);
	}
	// Update title bar
	{
		char buf[128];
		sprintf(buf, "Cool Herders Map Editor - %s", fnbuf);
		SetWindowText(buf);
	}

	if (fFlagBreakUpgrade) {
		AfxMessageBox("Note: the breakable format changed, so all your breakables are, well, broken. Sorry!");
	}

	memcpy(UndoMapData, MapData, sizeof(UndoMapData));

	OnFrame1();
	m_ctlAnim.SetCheck(BST_CHECKED);

	// 4 fps
	hTimer=SetTimer(1, 250, NULL);
	
	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CCoolHerdersAnimTestDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CCoolHerdersAnimTestDlg::OnPaint() 
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, (WPARAM) dc.GetSafeHdc(), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{	CPaintDC myDC(this); // device context for painting

		if (NULL != hImg[0][nFrame]) {
			_ISDrawHBITMAP(myDC.GetSafeHdc(), hBmp[0][nFrame], 0, 0, 256, 256, NULL);
			//_ISDrawRGBA(myDC.GetSafeHdc(), hImg[0][nFrame], 256, 256, 0, 0, 256, 256, NULL);
		}
		if (NULL != hImg[1][nFrame]) {
			_ISDrawHBITMAP(myDC.GetSafeHdc(), hBmp[1][nFrame], 0, 257, 256, 256, NULL);
			//_ISDrawRGBA(myDC.GetSafeHdc(), hImg[1][nFrame], 256, 256, 0, 257, 256, 256, NULL);
		}
		if (NULL != hImg[2][nFrame]) {
			_ISDrawHBITMAP(myDC.GetSafeHdc(), hBmp[2][nFrame], 0, 514, 256, 256, NULL);
			//_ISDrawRGBA(myDC.GetSafeHdc(), hImg[2][nFrame], 256, 256, 0, 514, 256, 256, NULL);
		}
		if (NULL != hImg[3][nFrame]) {
			_ISDrawHBITMAP(myDC.GetSafeHdc(), hBmp[3][nFrame], 260, 0, 640, 480, NULL);
			//_ISDrawRGB(myDC.GetSafeHdc(), hImg[3][nFrame], 640, 480, 260, 0, 640, 480, NULL);
		} 

		CPoint pt;
		GetCursorPos(&pt);
		ScreenToClient(&pt);
		DrawBox(pt);
	}
}

// The system calls this to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CCoolHerdersAnimTestDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

void CCoolHerdersAnimTestDlg::OnTimer(UINT nIDEvent) 
{
	if (fButtonDown) return;

	if (m_ctlAnim.GetCheck()==BST_CHECKED) {
		nFrame++;
		if (nFrame > 3) nFrame=0;
	}

	switch (nFrame) {
	case 0:	OnFrame1(); break;
	case 1: OnFrame2(); break;
	case 2:	OnFrame3(); break;
	case 3:	OnFrame4(); break;
	}

	CDialog::OnTimer(nIDEvent);
}

void CCoolHerdersAnimTestDlg::OnDropFiles(HDROP hDropInfo) 
{
	int nTilePage=-1;
	POINT pt;

	if (!DragQueryPoint(hDropInfo, &pt)) {
		DragFinish(hDropInfo);
		return;
	}

	// Check which tile page we dropped onto. If none, just discard the drop
	if (pt.x < 240) {
		if (pt.y < 240) {
			nTilePage=0;
		} else if ((pt.y > 256) && (pt.y < 496)) {
			nTilePage=1;
		} else if ((pt.y > 513) && (pt.y < 753)) {
			nTilePage=2;
		}
	}

	if (nTilePage != -1) {
		int nCount=DragQueryFile(hDropInfo, 0xffffffff, NULL, 0);

		for (int idx=0; idx<nCount; idx++) {
			char buf[1024];

			DragQueryFile(hDropInfo, idx, buf, 1024);

			// We only care about PNG files
			strlwr(buf);	// note! No unix filesystems!
			if (strstr(buf, ".png")) {
				// okay, load it up and see what we have. We know what to do with 32x32, 48x48, and 256x256 tiles)
				HISSRC src;
				HGLOBAL htmpImg;

				src=_ISOpenFileSource(buf);
				htmpImg=_ISReadPNG(src, &width, &height, 32, pal);
				_ISCloseSource(src);

				if (NULL != htmpImg) {
					if ((width==256)&&(height==256)) {
						// do full page mapping
						Framesel dlg;
						dlg.csFileName=buf;
						dlg.nReturn=nFrame;
						if ((m_ctlAnim.GetCheck()==BST_UNCHECKED) || (IDOK == dlg.DoModal())) {
							// Copy the picture over the selected value
							memcpy(hImg[nTilePage][dlg.nReturn], htmpImg, 256*256*4);
							SyncTexture(nTilePage,dlg.nReturn);
							RedrawMap(dlg.nReturn);
							if (m_ctlAnim.GetCheck()==BST_UNCHECKED) Invalidate(FALSE);
						}
					} else if ((width==48)&&(height==48)) {
						// do full tile mapping
						Framesel dlg;
						dlg.csFileName=buf;
						dlg.nReturn=nFrame;
						if ((m_ctlAnim.GetCheck()==BST_UNCHECKED) || (IDOK == dlg.DoModal())) {
							int tx,ty;
							tx=(pt.x/48)*48;
							ty=((pt.y-(257*nTilePage))/48)*48;
							_ISOverlayImage(hImg[nTilePage][dlg.nReturn], 256, 256, (unsigned char*)htmpImg, width, height, 4, tx, ty, 1.0, 0);
							SyncTexture(nTilePage,dlg.nReturn);
							RedrawMap(dlg.nReturn);
							if (m_ctlAnim.GetCheck()==BST_UNCHECKED) Invalidate(FALSE);
						}
					} else if ((width==32)&&(height==32)) {
						// do partial tile mapping
						Framesel dlg;
						dlg.csFileName=buf;
						dlg.nReturn=nFrame;
						if ((m_ctlAnim.GetCheck()==BST_UNCHECKED) || (IDOK == dlg.DoModal())) {
							int tx,ty;
							tx=(pt.x/48)*48+8;
							ty=((pt.y-(257*nTilePage))/48)*48+16;
							_ISOverlayImage(hImg[nTilePage][dlg.nReturn], 256, 256, (unsigned char*)htmpImg, width, height, 4, tx, ty, 1.0, 0);
							SyncTexture(nTilePage,dlg.nReturn);
							RedrawMap(dlg.nReturn);
							if (m_ctlAnim.GetCheck()==BST_UNCHECKED) Invalidate(FALSE);
						}
					} else {
						AfxMessageBox("You may only import tiles of 32x32 or 48x48, or full 256x256 pages");
					}

					GlobalFree(htmpImg);
				}
			}
		}
	}

	DragFinish(hDropInfo);
}

void CCoolHerdersAnimTestDlg::OnClose() 
{
	EndDialog(IDOK);
	CDialog::OnClose();
}

void CCoolHerdersAnimTestDlg::OnLButtonDown(UINT nFlags, CPoint pt) 
{
	int nTilePage=-1;
	char cTerrain=-1;

	switch (m_Terrain) {
	case 0: cTerrain='.'; break;
	case 1: cTerrain='-'; break;
	case 2: cTerrain='/'; break;
	}

	CDialog::OnLButtonDown(nFlags, pt);

	if (pt.x < 240) {
		if (pt.y < 240) {
			nTilePage=0;
		} else if ((pt.y > 256) && (pt.y < 496)) {
			nTilePage=1;
		} else if ((pt.y > 513) && (pt.y < 753)) {
			nTilePage=2;
		}
	} else if (pt.y < 480) {
		nTilePage=3;
	}

	switch (nTilePage) {
	case 0:		// tile pages 0-2
	case 1:
	case 2:
		{
			int tx,ty;

			// If there was an old current tile, update it's pages so the rect is cleared
			if (nCurrentTilePage != -1) {
				for (int i=0; i<4; i++) {
					SyncTexture(nCurrentTilePage,i);
				}
			}

			// If there's a tile mode set, then clear the current tile and just set the mode, else select the tile
			tx=(pt.x/48);
			ty=((pt.y-(257*nTilePage))/48);

			if (-1 == cTerrain) {
				nCurrentTile=(ty*5)+tx;
				nCurrentTilePage=nTilePage;
				nCurrentTileMode=TileData[nTilePage][nCurrentTile];
			} else {
				nCurrentTile=-1;
				nCurrentTilePage=-1;
				nCurrentTileMode=cTerrain;
				TileData[nTilePage][(ty*5)+tx]=cTerrain;
				// No box drawn
				break;
			}

			tx*=48;
			ty*=48;

			// Update the new tile with a rectangle around it, yellow and black
			// This draws only on the HBITMAP cache, not the original RGBA data!
			CDC myDC;
			myDC.CreateCompatibleDC(NULL);
			for (int i=0; i<4; i++) {
				CBitmap *pOldBmp, *myBmp;
				myBmp=CBitmap::FromHandle(hBmp[nCurrentTilePage][i]);
				pOldBmp=myDC.SelectObject(myBmp);
				myDC.Draw3dRect(tx-1, ty-1, 50, 50, RGB(255,255,0), RGB(200, 200, 0));
				myDC.Draw3dRect(tx-2, ty-2, 52, 52, RGB(50,50,50), RGB(0,0,0));
				myDC.SelectObject(pOldBmp);
				InvalidateRect(CRect(0, 0, 256, 770), FALSE);
			}
		}
		break;

	case 3:		// The big main map
		{
			int tx, ty;

			GetMainTilePos(pt.x, pt.y, &tx, &ty);

			if ((-1 != nCurrentTile)||(-1 != cTerrain)) {
				if (!fButtonDown) {
					// set undo
					memcpy(UndoMapData, MapData, sizeof(UndoMapData));
				}

				if (-1 == cTerrain) {
					if (nCurrentTilePage==2) {
						// Handle placing a breakable on a tile
						if (MapData[tx][ty][0] != '3') {
							// Changing from a normal tile to a breakable
							// Cache the letter of the tile in the flag byte
							// Set it's case by the original tile page
							if (MapData[tx][ty][0]=='1') {
								MapData[tx][ty][1]=toupper(MapData[tx][ty][2]);
							} else {
								MapData[tx][ty][1]=tolower(MapData[tx][ty][2]);
							}
							// Change the tile page
							MapData[tx][ty][0]='3';
						}
						// and always change the real tile number to current
						MapData[tx][ty][2]=nCurrentTile+'A';
					} else {
						MapData[tx][ty][0]=nCurrentTilePage+'1';
						MapData[tx][ty][1]=nCurrentTileMode;
						MapData[tx][ty][2]=nCurrentTile+'A';
					}
				} else {
					// Changing terrain flags only
					if (MapData[tx][ty][0] != '3') {
						// Don't change terrain type on breakables
						MapData[tx][ty][1]=cTerrain;	
					}
				}

				if (m_ctlAnim.GetCheck()==BST_UNCHECKED) Invalidate(FALSE);
				fMapDraw=true;

				fButtonDown=true;
				SetCapture();
			}
		}
		break;

	default:	// Not a point we care about
		return;
	}

	// Update title bar
	char buf[128];
	sprintf(buf, "* Cool Herders Map Editor - %s", fnbuf);
	SetWindowText(buf);
}

void CCoolHerdersAnimTestDlg::OnMouseMove(UINT nFlags, CPoint pt) 
{
	static CPoint oldPoint;

	DrawBox(oldPoint, TRUE);
	oldPoint=pt;
	CDialog::OnMouseMove(nFlags, pt);
	if (fButtonDown) {
		OnLButtonDown(0, pt);
	}
	DrawBox(pt);
}

void CCoolHerdersAnimTestDlg::OnLButtonUp(UINT nFlags, CPoint point) 
{
	CDialog::OnLButtonUp(nFlags, point);
	if (fButtonDown) {
		fButtonDown=false;
		ReleaseCapture();
		if (fMapDraw) {
			for (int idx=0; idx<4; idx++) {
				RedrawMap(idx);
			}
			fMapDraw=false;
			OnTimer(1);
		}
	}
}

void CCoolHerdersAnimTestDlg::OnFrame1() 
{
	CButton *pWnd;

	pWnd=(CButton*)GetDlgItem(IDC_FRAME1);
	if (pWnd) pWnd->SetCheck(BST_CHECKED);

	pWnd=(CButton*)GetDlgItem(IDC_FRAME2);
	if (pWnd) pWnd->SetCheck(BST_UNCHECKED);
	
	pWnd=(CButton*)GetDlgItem(IDC_FRAME3);
	if (pWnd) pWnd->SetCheck(BST_UNCHECKED);
	
	pWnd=(CButton*)GetDlgItem(IDC_FRAME4);
	if (pWnd) pWnd->SetCheck(BST_UNCHECKED);

	nFrame=0;
	Invalidate(FALSE);
}

void CCoolHerdersAnimTestDlg::OnFrame2() 
{
	CButton *pWnd;

	pWnd=(CButton*)GetDlgItem(IDC_FRAME1);
	if (pWnd) pWnd->SetCheck(BST_UNCHECKED);

	pWnd=(CButton*)GetDlgItem(IDC_FRAME2);
	if (pWnd) pWnd->SetCheck(BST_CHECKED);
	
	pWnd=(CButton*)GetDlgItem(IDC_FRAME3);
	if (pWnd) pWnd->SetCheck(BST_UNCHECKED);
	
	pWnd=(CButton*)GetDlgItem(IDC_FRAME4);
	if (pWnd) pWnd->SetCheck(BST_UNCHECKED);

	nFrame=1;
	Invalidate(FALSE);
}

void CCoolHerdersAnimTestDlg::OnFrame3() 
{
	CButton *pWnd;

	pWnd=(CButton*)GetDlgItem(IDC_FRAME1);
	if (pWnd) pWnd->SetCheck(BST_UNCHECKED);

	pWnd=(CButton*)GetDlgItem(IDC_FRAME2);
	if (pWnd) pWnd->SetCheck(BST_UNCHECKED);
	
	pWnd=(CButton*)GetDlgItem(IDC_FRAME3);
	if (pWnd) pWnd->SetCheck(BST_CHECKED);
	
	pWnd=(CButton*)GetDlgItem(IDC_FRAME4);
	if (pWnd) pWnd->SetCheck(BST_UNCHECKED);

	nFrame=2;
	Invalidate(FALSE);
}

void CCoolHerdersAnimTestDlg::OnFrame4() 
{
	CButton *pWnd;

	pWnd=(CButton*)GetDlgItem(IDC_FRAME1);
	if (pWnd) pWnd->SetCheck(BST_UNCHECKED);

	pWnd=(CButton*)GetDlgItem(IDC_FRAME2);
	if (pWnd) pWnd->SetCheck(BST_UNCHECKED);
	
	pWnd=(CButton*)GetDlgItem(IDC_FRAME3);
	if (pWnd) pWnd->SetCheck(BST_UNCHECKED);
	
	pWnd=(CButton*)GetDlgItem(IDC_FRAME4);
	if (pWnd) pWnd->SetCheck(BST_CHECKED);

	nFrame=3;
	Invalidate(FALSE);
}

void CCoolHerdersAnimTestDlg::GetMainTilePos(int inX, int inY, int *outX, int *outY)
{
	*outX=((inX-260+16)/32);
	*outY=((inY+8)/32);
}

void CCoolHerdersAnimTestDlg::DrawBox(CPoint pt, BOOL fErase)
{
	CRect rect;
	int tx, ty;
	char buf[128];

	GetMainTilePos(pt.x, pt.y, &tx, &ty);

	if ((tx < 0)||(tx>20)||(ty<0)||(ty>15)) {
		m_Coord.SetWindowText("");
		return;
	}

	sprintf(buf, "(%d,%d)", tx, ty);
	m_Coord.SetWindowText(buf);

	tx*=32;
	ty*=32;

	rect.left=tx+260-16;
	rect.top=ty-8;
	rect.right=rect.left+31;
	rect.bottom=rect.top+31;

	if (fErase) {
		InvalidateRect(&rect, FALSE);
	} else {
		CRgn myRgn;
		myRgn.CreateRectRgn(260,0, 900,480);

		CDC *myDC=GetDC();
		if (NULL == myDC) {
			AfxMessageBox("Resources are extremely low! Save your work, quit, and restart this program");
			return;
		}
		myDC->SelectClipRgn(&myRgn);
		myDC->Draw3dRect(&rect, RGB(255,255,255),RGB(255,255,255));
		myDC->SelectClipRgn(NULL);
		
		ReleaseDC(myDC);
		
		myRgn.DeleteObject();
	}
}

void CCoolHerdersAnimTestDlg::OnFlat() 
{
	UpdateData();
	FixHighlights();
}

void CCoolHerdersAnimTestDlg::On3dimpass() 
{
	UpdateData();
	FixHighlights();
}

void CCoolHerdersAnimTestDlg::OnImpass() 
{
	UpdateData();
	FixHighlights();
}

void CCoolHerdersAnimTestDlg::OnNoterrain() 
{
	UpdateData();
	FixHighlights();
}

void CCoolHerdersAnimTestDlg::FixHighlights()
{
	// This function is used to overlay a highlight on certain tile
	// types. Although it's possible to have the same tile with different
	// properties, it's confusing so we don't do it here in the editor :)
	int idx;

	// First, rebuild all the textures to clear any previous highlights
	// (We don't have to redraw the map here, as we'll do that last after we're done)
	// TODO someday. Sets default values on the texture blocks themselves

	// Lastly, redraw the map
	for (idx=0; idx<4; idx++) {
		RedrawMap(idx);
	}
}

void CCoolHerdersAnimTestDlg::OnUndo() 
{
	// restore undo
	memcpy(MapData, UndoMapData, sizeof(MapData));
	for (int idx=0; idx<4; idx++) {
		RedrawMap(idx);
	}
}

void CCoolHerdersAnimTestDlg::OnSafe() 
{
	for (int idx=0; idx<4; idx++) {
		RedrawMap(idx);
	}
}

void CCoolHerdersAnimTestDlg::OnAbort() 
{
	if (IDYES == AfxMessageBox("This will ABORT the program and you will lose all your work!\nREALLY KILL IT?", MB_YESNO|MB_ICONEXCLAMATION)) {
		EndDialog(IDCANCEL);
	}
}

// Ignore the normal OK and Cancel
void CCoolHerdersAnimTestDlg::OnOK() 
{
}
void CCoolHerdersAnimTestDlg::OnCancel() 
{
}

void CCoolHerdersAnimTestDlg::OnSave() 
{
	if (theApp.SaveItAll()) {
		SetWindowText("Save Complete...");
	}
}


void CCoolHerdersAnimTestDlg::OnHidebreak() 
{
	for (int idx=0; idx<4; idx++) {
		RedrawMap(idx);
	}
	
}
