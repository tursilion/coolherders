// CoolHerdersAnimTestDlg.cpp : implementation file
//

// Write a new 256x256x32 PNG file containing the sprites
// packed as best we can

#include "stdafx.h"
#include "CoolHerdersAnimTest.h"
#include "CoolHerdersAnimTestDlg.h"
#include "c:\work\imgsource\_isource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

HGLOBAL hImg=NULL;
UINT32 width, height;
RGBQUAD pal[256];
UINT_PTR hTimer=0;

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
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CCoolHerdersAnimTestDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CCoolHerdersAnimTestDlg)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CCoolHerdersAnimTestDlg, CDialog)
	//{{AFX_MSG_MAP(CCoolHerdersAnimTestDlg)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON1, OnButton1)
	ON_WM_TIMER()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCoolHerdersAnimTestDlg message handlers

BOOL CCoolHerdersAnimTestDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	_ISInitialize(IS KEY HERE);

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
	
	// TODO: Add extra initialization here
	
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
	{
		CDialog::OnPaint();
	}
}

// The system calls this to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CCoolHerdersAnimTestDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

void CCoolHerdersAnimTestDlg::OnButton1() 
{
	OPENFILENAME fn;
	char buf[1024];
	
	ZeroMemory(&fn, sizeof(fn));
	fn.lStructSize=sizeof(fn);
	fn.hwndOwner=GetSafeHwnd();
	fn.lpstrFilter="PNG Files\0*.png\0";
	fn.nMaxFile=1024;
	fn.lpstrFile=buf;
	fn.nFilterIndex=1;
	buf[0]='\0';
	fn.Flags=OFN_FILEMUSTEXIST;
	
	if (NULL != hImg) {
		GlobalFree(hImg);
		hImg=NULL;
	}

	if (GetOpenFileName(&fn)) {
		HISSRC src;

		src=_ISOpenFileSource(buf);
		hImg=_ISReadPNG(src, &width, &height, 8, pal);
		_ISCloseSource(src);

		if (NULL == hImg) {
			AfxMessageBox("Failed to read PNG as 8-bit - make sure it has a palette - only the first 16 colors count!");
			return;
		}
	} else {
		return;
	}

	// At this point it was loaded - do our work
	// The current image map, of 48x48 tiles, is:
	//  0  5 10 15 20 25 30 
	//  1  6 11 16 21 26 31 
	//  2  7 12 17 22 27 32 
	//  3  8 13 18 23 28 33 
	//  4  9 14 19 24 29 34 
	// Therefore it must be at least 336x240 (larger is ok)
	if ((width<336)||(height<240)) {
		AfxMessageBox("Image must be at least 336x240");
		return;
	}

	// We write out a 256x256 image file, and a text file containing the data for each sprite:
	// X, Y, Width, Height, X Offset, Y Offset
	// X and Y are the coordinates in the new image of the character
	// Width and Height are the actual size of the new image
	// X Offset and Y Offset are how far left and up the image shifted from it's original position in the 48x48 cell
	unsigned char *pNewImg=(unsigned char *)malloc(256*256);	// image data
	unsigned char *pNewMap=(unsigned char *)malloc(256*256);	// map of used space
	struct {
		int x, y, width, height, xoff, yoff;
	} NewData[35];

	memset(pNewImg, 0, 256*256);
	memset(pNewMap, 0, 256*256);
	
	int nCurrentCellIdx=0;
	for (int cellx=0; cellx<7; cellx++) {
		for (int celly=0; celly<5; celly++) {
			// For each cell, we need to size the data, move it into the new map, and
			// record the location. Color 0 is always transparent
			int tx, ty;
			unsigned char *pBase=((unsigned char *)hImg)+(celly*48*width)+(cellx*48);
			// map y values
			NewData[nCurrentCellIdx].yoff=-1;	// just in case
			for (ty=0; ty<48; ty++) {
				for (tx=0; tx<48; tx++) {
					if (*(pBase+(ty*width)+(tx))) {
						// top pixel
						NewData[nCurrentCellIdx].yoff=ty;
						break;
					}
				}
				if (-1 != NewData[nCurrentCellIdx].yoff) {
					break;
				}
			}
			NewData[nCurrentCellIdx].height=48-ty;	// just in case
			for (; ty<48; ty++) {
				for (tx=0; tx<48; tx++) {
					if (*(pBase+(ty*width)+(tx))) {
						// need a blank row
						break;
					}
				}
				if (tx >= 48) {
					// row is blank
					NewData[nCurrentCellIdx].height=ty-NewData[nCurrentCellIdx].yoff;
					break;
				}
			}
			// map x values
			NewData[nCurrentCellIdx].xoff=-1;	// just in case
			for (tx=0; tx<48; tx++) {
				for (ty=0; ty<48; ty++) {
					if (*(pBase+(ty*width)+(tx))) {
						// left pixel
						NewData[nCurrentCellIdx].xoff=tx;
						break;
					}
				}
				if (-1 != NewData[nCurrentCellIdx].xoff) {
					break;
				}
			}
			NewData[nCurrentCellIdx].width=48-tx;	// just in case
			for (; tx<48; tx++) {
				for (ty=0; ty<48; ty++) {
					if (*(pBase+(ty*width)+(tx))) {
						// need a blank col
						break;
					}
				}
				if (ty >= 48) {
					// col is blank
					NewData[nCurrentCellIdx].width=tx-NewData[nCurrentCellIdx].xoff;
					break;
				}
			}
			// for debug, print out what we found
			char buf[256];
			sprintf(buf, "%d - %d, %d, %d, %d\n", nCurrentCellIdx, NewData[nCurrentCellIdx].xoff, NewData[nCurrentCellIdx].yoff, NewData[nCurrentCellIdx].width, NewData[nCurrentCellIdx].height);
			OutputDebugString(buf);
			// copy into the new image - find a place for it

			nCurrentCellIdx++;
		}
	}

	// Save new image




	// clean up
	free(pNewImg);
	free(pNewMap);
}

void CCoolHerdersAnimTestDlg::OnTimer(UINT nIDEvent) 
{
	CDialog::OnTimer(nIDEvent);
}
