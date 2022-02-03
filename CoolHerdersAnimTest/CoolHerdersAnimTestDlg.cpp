// CoolHerdersAnimTestDlg.cpp : implementation file
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

HGLOBAL hImg=NULL;
UINT32 width, height;
RGBQUAD pal[256];
UINT_PTR hTimer=0;
int Size=1;
int QuitMode=0;	// 0 means it's quit, 1 means it's reanimate

// Offsets: 2 chars, xy
// Array: 0=left, 1=down, 2=right, 3=up
// run repeats three times, then character stops
char *runanim[4]= {
	"000001020203000001020203000001020203040404040404",
	"101011121213101011121213101011121213141414141414",
	"202021222223202021222223202021222223242424242424",
	"303031323233303031323233303031323233343434343434"
};

// char runs twice, charges, fires
char *chargeanim[4] = {
	"000001020203000001020203444444446060606060606060",
	"101011121213101011121213535353536161616161616161",
	"202021222223202021222223646464646262626262626262",
	"303031323233303031323233545454546363636363636363"
};

// only one dance animation
char *danceanim = "4040505040405050414151514141515123234242343452523434424234345252343403132333434343434343";

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
	DDX_Control(pDX, IDC_QUIT, m_ctlQuit);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CCoolHerdersAnimTestDlg, CDialog)
	//{{AFX_MSG_MAP(CCoolHerdersAnimTestDlg)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON1, OnButton1)
	ON_WM_TIMER()
	ON_BN_CLICKED(IDC_BIGGER, OnBigger)
	ON_BN_CLICKED(IDC_QUIT, OnQuit)
	ON_BN_CLICKED(IDC_SMALLER, OnSmaller)
	ON_BN_CLICKED(IDC_STEPFORWARD, OnStepforward)
	ON_BN_CLICKED(IDC_STEPBACK, OnStepback)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCoolHerdersAnimTestDlg message handlers

BOOL CCoolHerdersAnimTestDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	_ISInitialize("{50EB2018-DCE6-4773-B3C2-3C58192F64DA}");

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
		// Force a redraw if the timer isn't running
		if (0 == hTimer) {
			OnTimer(0);
		}
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
		hImg=_ISReadPNG(src, &width, &height, 24, pal);
		_ISCloseSource(src);

		if (NULL != hImg) {
			if (0 == hTimer) {
				// approx anim speed on DC - 60fps, change every 4 frames
				hTimer=SetTimer(1, 16*4, NULL);
			}
		}
	}
}

// assumes a 48x48 source fragment
void myDrawRGBCrop(HDC hDC, BYTE *pRGB, UINT32 uWidth, UINT32 uHeight, __int32 iXPos, __int32 iYPos, UINT32 uSrcXStart, UINT32 uSrcYStart, UINT32 uOutWidth, UINT32 uOutHeight, HPALETTE hPal) {
	char *pMem=(char*)malloc(48*48*4);
	_ISCropImage(pRGB, uWidth, uHeight, (BYTE*)pMem, uSrcXStart, uSrcYStart, 48, 48, 3);
	_ISStretchDrawRGB(hDC, (BYTE*)pMem, 48, 48, iXPos, iYPos, uOutWidth, uOutHeight);
	free(pMem);
}

void CCoolHerdersAnimTestDlg::OnTimer(UINT nIDEvent) 
{
	static int nFrame=0;
	int x,y, w;

	if (NULL == hImg) return;

	switch (nIDEvent) {
	case 1:	nFrame++; break;
	case 2: nFrame--; break;
	}

	CDC *myDC=GetDC();
	int i,sx,sy,p;

	// running frames
	y=80;
	x=20;
	w=64*Size;
	for (i=0; i<4; i++) {
		p=nFrame%(strlen(runanim[0])/2);
		sx=(runanim[i][p*2]-'0')*48;
		sy=(runanim[i][p*2+1]-'0')*48;
        myDrawRGBCrop(myDC->GetSafeHdc(), (unsigned char*)hImg, width, height, x, y, sx, sy, 48*Size, 48*Size, NULL);
		x+=w;
	}

	// charge frames
	y+=64*Size;
	x=20;
	w=64*Size;
	for (i=0; i<4; i++) {
		p=nFrame%(strlen(chargeanim[0])/2);
		sx=(chargeanim[i][p*2]-'0')*48;
		sy=(chargeanim[i][p*2+1]-'0')*48;
		myDrawRGBCrop(myDC->GetSafeHdc(), (unsigned char*)hImg, width, height, x, y, sx, sy, 48*Size, 48*Size, NULL);
		x+=w;
	}

	// dancing
	y+=64*Size;
	x=20;
	p=nFrame%(strlen(danceanim)/2);
	sx=(danceanim[p*2]-'0')*48;
	sy=(danceanim[p*2+1]-'0')*48;
	myDrawRGBCrop(myDC->GetSafeHdc(), (unsigned char*)hImg, width, height, x, y, sx, sy, 48*Size, 48*Size, NULL);

	myDC->DeleteDC();
	CDialog::OnTimer(nIDEvent);
}

void CCoolHerdersAnimTestDlg::OnBigger() 
{
	RECT myrect;
	int x;

	Size*=2;

	if (Size == 8) {
		AfxMessageBox("Yes, I *will* just keep growing.");
	} else if (Size == 32) {
		AfxMessageBox("This is getting silly...");
	} else if (Size == 128) {
		AfxMessageBox("I'll crash eventually...");
	} else if (Size == 512) {
		AfxMessageBox("Besides my disbelief that this is even working....\nYou sir, are a nut. ;)");
	}

	GetWindowRect(&myrect);
	x=myrect.bottom-myrect.top;
	myrect.top-=x/2;
	myrect.bottom+=x/2;
	if (myrect.top < 0) {
		myrect.bottom-=myrect.top;
		myrect.top=0;
	}
	
	x=myrect.right-myrect.left;
	myrect.right+=x/2;
	myrect.left-=x/2;
	if (myrect.left < 0) {
		myrect.right-=myrect.left;
		myrect.left=0;
	}
	
	MoveWindow(&myrect);
	Invalidate();
	
}

void CCoolHerdersAnimTestDlg::OnQuit() 
{
	if (!QuitMode) {
		EndDialog(IDOK);	
	} else {
		if (0 == hTimer) {
			// approx anim speed on DC - 60fps, change every 4 frames
			hTimer=SetTimer(1, 16*4, NULL);
		}
		QuitMode=0;
		m_ctlQuit.SetWindowText("&Quit");
	}
}

void CCoolHerdersAnimTestDlg::OnSmaller() 
{
	RECT myrect;
	int x;

	if (Size > 1) {
		Size/=2;

		GetWindowRect(&myrect);
		x=myrect.bottom-myrect.top;
		myrect.bottom-=x/2;
		
		x=myrect.right-myrect.left;
		myrect.right-=x/2;
		
		MoveWindow(&myrect);
		Invalidate();
	} else {
		AfxMessageBox("No. You're being silly.");
	}
	
}

void CCoolHerdersAnimTestDlg::OnStepforward() 
{
	KillTimer(hTimer);
	hTimer=0;

	QuitMode=1;
	m_ctlQuit.SetWindowText("&Reanimate");

	OnTimer(1);
}

void CCoolHerdersAnimTestDlg::OnStepback() 
{
	KillTimer(hTimer);
	hTimer=0;
	
	QuitMode=1;
	m_ctlQuit.SetWindowText("&Reanimate");

	OnTimer(2);
}
