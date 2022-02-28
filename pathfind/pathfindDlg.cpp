// pathfindDlg.cpp : implementation file
//

#include "stdafx.h"
#include "pathfind.h"
#include "pathfindDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

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
// CPathfindDlg dialog

CPathfindDlg::CPathfindDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CPathfindDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CPathfindDlg)
	m_Edit = _T("");
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CPathfindDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPathfindDlg)
	DDX_Text(pDX, IDC_EDIT1, m_Edit);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CPathfindDlg, CDialog)
	//{{AFX_MSG_MAP(CPathfindDlg)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_TIMER()
	ON_BN_CLICKED(IDC_BUTTON1, OnButton1)
	ON_BN_CLICKED(IDC_BUTTON6, OnButton6)
	ON_BN_CLICKED(IDC_BUTTON5, OnButton5)
	ON_BN_CLICKED(IDC_BUTTON4, OnButton4)
	ON_BN_CLICKED(IDC_BUTTON3, OnButton3)
	ON_BN_CLICKED(IDC_BUTTON2, OnButton2)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPathfindDlg message handlers


// 32 paths, 50 steps each (3.2k)
char PathFound[32][50][2];
char pathwalk[16][21];		// y,x
int nPaths, nRange;
int startx,starty,endx,endy;
int nMaxPaths, nMaxRange;
int nSearches;

BOOL CPathfindDlg::OnInitDialog()
{
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

	OnButton6();		// Load a stage

	SetTimer(1, 100, NULL);
	
	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CPathfindDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void CPathfindDlg::OnPaint() 
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
HCURSOR CPathfindDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

void CPathfindDlg::OnTimer(UINT nIDEvent) 
{
	CString c;

	CDialog::OnTimer(nIDEvent);
	rand();

	DoPaths();

	m_Edit="";
	for (int y=0; y<16; y++) {
		for (int x=0; x<21; x++) {
			c=isFlat(x,y)?"  ":"++";
			if (pathwalk[y][x]==1) c="##";
			if ((y==starty)&&(x==startx)) c="@@";
			if ((y==endy)&&(x==endx)) c="$$";
			m_Edit+=c;
		}
		m_Edit+="\r\n";
	}
	c.Format("\r\nMax Paths: %2d  Max Range: %2d  Searches: %2d\r\n", nMaxPaths, nMaxRange, nSearches);
	m_Edit+=c;
	UpdateData(false);
}

void CPathfindDlg::OnButton1() 
{
	do {
		startx=rand()%21;
		starty=rand()%16;
		endx=rand()%21;
		endy=rand()%16;
	} while ((!isFlat(startx,starty))||(!isFlat(endx,endy))||((startx==endx)&&(starty==endy)));

	nPaths=1;
	nRange=0;
	memset(PathFound, 0, sizeof(PathFound));
	PathFound[0][0][0]=startx;
	PathFound[0][0][1]=starty;
	nMaxPaths=1;
	nMaxRange=0;
	nSearches=0;
	memset(pathwalk, 0, sizeof(pathwalk));
	pathwalk[starty][startx]=1;
}

bool CPathfindDlg::isFlat(int x, int y)
{
	if ((m_Buffer[y*21*3+x*3]=='3')||(m_Buffer[y*21*3+x*3+1]=='.')) {
		return true;
	}
	return false;
}

void CPathfindDlg::DoPaths()
{
	int nTmpPaths=nPaths;
	int idx2;

	if (-1 == PathFound[0][0][0]) {
		return;
	}

	// First check the existing path
	for (int idx=0; idx<nTmpPaths; idx++) {
		int x,y,z;

		if (0 == PathFound[idx][0][0]) {
			continue;
		}

		x=PathFound[idx][nRange][0];
		y=PathFound[idx][nRange][1];
		z=0;

		// next path up
		if ((nRange==0)||((nRange>0)&&(y-1!=PathFound[idx][nRange-1][1]))) {
			// only 1 var can change per step
			if (isFlat(x,y-1)) {
				if (0==pathwalk[y-1][x]) {
					// we can do this
					nSearches++;
					pathwalk[y-1][x]=1;
					PathFound[idx][nRange+1][0]=x;
					PathFound[idx][nRange+1][1]=y-1;
					if ((y-1==endy)&&(x==endx)) {
						PathFound[0][0][0]=-1;
						PathFound[0][0][1]=idx;
						break;
					}
					z=1;
				}
			}
		}

		// and right
		if ((nRange==0)||((nRange>0)&&(x+1!=PathFound[idx][nRange-1][0]))) {
			// only 1 var can change per step
			if (isFlat(x+1,y)) {
				if (0==pathwalk[y][x+1]) {
					// we can do this
					nSearches++;
					if (!z) {
						pathwalk[y][x+1]=1;
						PathFound[idx][nRange+1][0]=x+1;
						PathFound[idx][nRange+1][1]=y;
						if ((y==endy)&&(x+1==endx)) {
							PathFound[0][0][0]=-1;
							PathFound[0][0][1]=idx;
							break;
						}
						z=1;
					} else {
						for (idx2=0; idx2<=nRange; idx2++) {
							PathFound[nPaths][idx2][0]=PathFound[idx][idx2][0];
							PathFound[nPaths][idx2][1]=PathFound[idx][idx2][1];
						}
						pathwalk[y][x+1]=1;
						PathFound[nPaths][nRange+1][0]=x+1;
						PathFound[nPaths][nRange+1][1]=y;
						if ((y==endy)&&(x+1==endx)) {
							PathFound[0][0][0]=-1;
							PathFound[0][0][1]=nPaths;
						}
						nPaths++;
						z=1;
					}
				} 
			}
		}

		// and left
		if ((nRange==0)||((nRange>0)&&(x-1!=PathFound[idx][nRange-1][0]))) {
			// only 1 var can change per step
			if (isFlat(x-1,y)) {
				if (0==pathwalk[y][x-1]) {
					// we can do this
					nSearches++;
					if (!z) {
						pathwalk[y][x-1]=1;
						PathFound[idx][nRange+1][0]=x-1;
						PathFound[idx][nRange+1][1]=y;
						if ((y==endy)&&(x-1==endx)) {
							PathFound[0][0][0]=-1;
							PathFound[0][0][1]=idx;
							break;
						}
						z=1;
					} else {
						for (idx2=0; idx2<=nRange; idx2++) {
							PathFound[nPaths][idx2][0]=PathFound[idx][idx2][0];
							PathFound[nPaths][idx2][1]=PathFound[idx][idx2][1];
						}
						pathwalk[y][x-1]=1;
						PathFound[nPaths][nRange+1][0]=x-1;
						PathFound[nPaths][nRange+1][1]=y;
						if ((y==endy)&&(x-1==endx)) {
							PathFound[0][0][0]=-1;
							PathFound[0][0][1]=nPaths;
						}
						nPaths++;
						z=1;
					}
				}
			}
		}

		// and down
		if ((nRange==0)||((nRange>0)&&(y+1!=PathFound[idx][nRange-1][1]))) {
			// only 1 var can change per step
			if (isFlat(x,y+1)) {
				if (0==pathwalk[y+1][x]) {
					// we can do this
					nSearches++;
					if (!z) {
						pathwalk[y+1][x]=1;
						PathFound[idx][nRange+1][0]=x;
						PathFound[idx][nRange+1][1]=y+1;
						if ((y+1==endy)&&(x==endx)) {
							PathFound[0][0][0]=-1;
							PathFound[0][0][1]=idx;
							break;
						}
						z=1;
					} else {
						for (idx2=0; idx2<=nRange; idx2++) {
							PathFound[nPaths][idx2][0]=PathFound[idx][idx2][0];
							PathFound[nPaths][idx2][1]=PathFound[idx][idx2][1];
						}
						pathwalk[y+1][x]=1;
						PathFound[nPaths][nRange+1][0]=x;
						PathFound[nPaths][nRange+1][1]=y+1;
						if ((y+1==endy)&&(x==endx)) {
							PathFound[0][0][0]=-1;
							PathFound[0][0][1]=nPaths;
						}
						nPaths++;
						z=1;
					}
				}
			}
		}

		if (0 == PathFound[idx][nRange+1][0]) {
			// dead end - this stream is dead
			PathFound[idx][0][0]=0;
		}
	}


	nRange++;
	if (nRange>49) {
		OutputDebugString("\n\n*** OVERFLOW ***\n");
		PathFound[0][0][0]=-1;
		PathFound[0][0][1]=0;
	}
	nMaxPaths=nPaths;
	nMaxRange=nRange;

	if (PathFound[0][0][0]==-1) {
		// clear all paths but this one
		memset(pathwalk, 0, sizeof(pathwalk));
		for (idx=1; idx<nRange; idx++) {
			pathwalk[PathFound[PathFound[0][0][1]][idx][1]][PathFound[PathFound[0][0][1]][idx][0]]=1;
		}
	}
}

void CPathfindDlg::OnButton6() 
{
	// Each cell is 3 characters, 21x16 total size
	m_Buffer="2-K2-K2-K2-K2-K2-K2-K2-K2-K2-K2-K2-K2-K2-K2-K2-K2-K2-K2-K2-K2-K"
           "2-I2-L2-L2-U2-L2-L2-L2-U2-L2-L2-L2-L2-L2-U2-L2-L2-L2-U2-L2-L2-N"
           "2-G2-H2-K2-V2-K2-K2-K2-V2-K2-K2-K2-K2-K2-V2-K2-K2-K2-V2-K2-M2-Q"
           "2-G2-F1.H1.H3HA1/C3HA3HA3HA1/A1/F1/B3HA3HA3HA1/C3HA1.H1.H2-P2-Q"
           "2-G2-F1.H2/J3HA3HA1.H1/E1.H2/E1.H1/D1.H2/J1.H3HA3HA1/E1.H2-P2-Q"
           "2-G2-F3HA1/A1/B1.H1/A1/B1.H1.H1.H1.H1.H1/A1/B1.H1/A1/B3HA2-P2-Q"
           "2-G2-F3HA3HA1.H3HA3HA1.H1.H1/E1.H2/J1.H1.H3HA3HA1.H3HA3HA2-P2-Q"
           "2-G2-F2/O1.H1/E3HA2/J1.H1/A1/F1/F1/F1/B1.H2/J3HA1/E1.H2/T2-P2-Q"
           "2-G2-F1/C3HA1/A1/F1/B1.H1.H1.H1.H1.H1.H1.H1/A1/F1/B3HA1/C2-P2-Q"
           "2-G2-F2/O1.H2/E3HA1/D1.H1/A1/F1/F1/F1/B1.H1/D3HA2/E1.H2/T2-P2-Q"
           "2-G2-F3HA3HA1.H3HA3HA1.H1.H2/E1.H1/D1.H1.H3HA3HA1.H3HA3HA2-P2-Q"
           "2-G2-F3HA1/A1/B1.H1/A1/B1.H1.H1.H1.H1.H1/A1/B1.H1/A1/B3HA2-P2-Q"
           "2-G2-F1.H1/D3HA3HA1.H2/E1.H1/E1.H2/J1.H1/D1.H3HA3HA2/E1.H2-P2-Q"
           "2-G2-F1.H1.H3HA1/C3HA3HA3HA1/A1/F1/B3HA3HA3HA1/C3HA1.H1.H2-P2-Q"
           "2-G2-C2-A2-A2-A2-A2-A2-A2-A2-A2-A2-A2-A2-A2-A2-A2-A2-A2-A2-R2-Q"
           "2-D2-B2-B2-B2-B2-B2-B2-B2-B2-B2-B2-B2-B2-B2-B2-B2-B2-B2-B2-B2-B";

	OnButton1();
}

void CPathfindDlg::OnButton5() 
{
	// Each cell is 3 characters, 21x16 total size
	m_Buffer="2-O2-O2-O2-O2-O2-O2-O2-O2-O2-O2-O2-O2-O2-O2-O2-O2-O2-O2-O2-O2-O"
"2-O2-O2-O2-O2-O2-O2-O2-O2-O2-O2-O2-O2-O2-O2-O2-O2-O2-O2-O2-O2-O"
"2-O2-Q2-K2-K2-K2-K2-K2-K2-K2-K2-K2-K2-K2-K2-K2-K2-K2-K2-K2-P2-O"
"2-O2-M1.J1.J3JA3JA3JA3JA1.J1.J1.K1.J1.J3JA3JA3JA3JA1.J1.J2-L2-O"
"2-O2-M1.J2/G2/A2/A3AA2/A1.A2/A1.A2/A2/A2/A3AA2/A2/A2/F1.K2-L2-O"
"2-O2-M3JA2/C1.E1.E1.E2/E1.E1.E1.E1.E1.E2/E1.E1.E1.E2/B3JA2-L2-O"
"2-O2-M3JA2/C1.E2/E2/E2/E1.E2/E2/E2/E1.E2/E1.E2/E1.E2/B3JA2-L2-O"
"2-O2-M3JA3CA1.E1.E1.E1.E1.E2/E1.E1.E1.E1.E1.E2/E1.E3BA3JA2-L2-O"
"2-O2-M2/J2/C1.E2/E2/E2/E1.E2/E1.E2/E1.E2/E2/E2/E1.E2/B2/J2-L2-O"
"2-O2-M3JA3CA1.E2/E1.E1.E1.E1.E1.E2/E1.E1.E1.E1.E1.E3BA3JA2-L2-O"
"2-O2-M3JA2/C1.E2/E1.E2/E1.E2/E2/E2/E1.E2/E2/E2/E1.E2/B3JA2-L2-O"
"2-O2-M3JA2/C1.E1.E1.E2/E1.E1.E1.E1.E1.E2/E1.E1.E1.E2/B3JA2-L2-O"
"2-O2-M1.J2/I2/D2/D3DA2/D2/D2/D1.D2/D1.D2/D3DA2/D2/D2/H1.J2-L2-O"
"2-O2-M1.K1.J3JA3JA3JA3JA1.J1.J1.J1.J1.J3JA3JA3JA3JA1.J1.J2-L2-O"
"2-O2-S2-N2-N2-N2-N2-N2-N2-N2-N2-N2-N2-N2-N2-N2-N2-N2-N2-N2-R2-O"
"2-O2-O2-O2-O2-O2-O2-O2-O2-O2-O2-O2-O2-O2-O2-O2-O2-O2-O2-O2-O2-O";

	OnButton1();
}

void CPathfindDlg::OnButton4() 
{
	// Each cell is 3 characters, 21x16 total size
	m_Buffer="1/K1/K1/K1/S1/S1/K1/S1/S1/K1/S1/S1/S1/K1/S1/S1/K1/S1/S1/K1/K1/K"
"1/G1/R1/G1/W1/X1/G1/W1/X1/G1/W1/S1/X1/G1/W1/X1/G1/W1/X1/G1/R1/G"
"1/S1/V1/D1/U1/V1/A1/U1/V1/B1/U1/S1/V1/C1/U1/V1/B1/U1/V1/C1/U1/S"
"1/W1/X2.R2.N3nA3nF3nK3fA3nK2.N2.N2.N3nK3fA3nK3nF3nA2.N2.S1/W1/X"
"1/U1/V2.M1/H1/S1/I1/D3mF1/B1/H1/S1/I1/D3mF1/B1/H1/S1/I2.M1/U1/V"
"1/I1/A3mA1/B2.R2.N2.N2.L2.N2.N2.F2.N2.N2.L2.N2.N2.S1/B3mA1/B1/H"
"1/W1/X3hF3nA3iF1/W1/I2.M1/H1/X2.M1/W1/I2.M1/H1/X3hF3nA3iF1/W1/X"
"1/U1/V3mK1/P3mK1/G1/C2.M1/D1/G2.M1/G1/C2.M1/D1/G3mK1/P3mK1/U1/V"
"1/I1/B3hA3nF3lA2.N2.N2.L2.N2.N2.L2.N2.N2.L2.N2.N3lA3nF3iA1/C1/H"
"1/W1/X3mK1/P3mK1/F1/B2.M1/A1/F2.M1/F1/B2.M1/A1/F3mK1/P3mK1/W1/X"
"1/U1/V3hF3nA3iF1/U1/I2.M1/H1/V2.M1/U1/I2.M1/H1/V3hF3nA3iF1/U1/V"
"1/I1/C3mA1/B2.P2.N2.N2.L2.N2.N2.G2.N2.N2.L2.N2.N2.Q1/B3mA1/D1/H"
"1/W1/X2.M1/H1/S1/I1/D3mF1/B1/H1/S1/I1/D3mF1/B1/H1/S1/I2.M1/W1/X"
"1/U1/V2.P2.N3nA3nF3nK3gA3nK2.N2.N2.N3nK3gA3nK3nF3nA2.N2.Q1/U1/V"
"1/W1/X1/D1/W1/X1/A1/W1/X1/B1/W1/S1/X1/C1/W1/X1/D1/W1/X1/A1/W1/X"
"1/R1/R1/F1/U1/V1/F1/U1/V1/F1/U1/S1/V1/F1/U1/V1/F1/U1/V1/F1/R1/R";

	OnButton1();
}

void CPathfindDlg::OnButton3() 
{
	// Each cell is 3 characters, 21x16 total size
	m_Buffer="2-K2-K2-K2-K2-K2-K2-K2-K2-K2-K2-K2-K2-K2-K2-K2-K2-K2-K2-K2-K2-K"
"2-I2-L2-L2-U2-L2-L2-L2-U2-L2-L2-L2-L2-L2-U2-L2-L2-L2-U2-L2-L2-N"
"2-G2-H2-K2-V2-K2-K2-K2-V2-K2-K2-K2-K2-K2-V2-K2-K2-K2-V2-K2-M2-Q"
"2-G2-F1.H1.H3HA1/C3HA3HA3HA1/A1/F1/B3HA3HA3HA1/C3HA1.H1.H2-P2-Q"
"2-G2-F1.H2/J3HA3HA1.H1/E1.H2/E1.H1/D1.H2/J1.H3HA3HA1/E1.H2-P2-Q"
"2-G2-F3HA1/A1/B1.H1/A1/B1.H1.H1.H1.H1.H1/A1/B1.H1/A1/B3HA2-P2-Q"
"2-G2-F3HA3HA1.H3HA3HA1.H1.H1/E1.H2/J1.H1.H3HA3HA1.H3HA3HA2-P2-Q"
"2-G2-F2/O1.H1/E3HA2/J1.H1/A1/F1/F1/F1/B1.H2/J3HA1/E1.H2/T2-P2-Q"
"2-G2-F1/C3HA1/A1/F1/B1.H1.H1.H1.H1.H1.H1.H1/A1/F1/B3HA1/C2-P2-Q"
"2-G2-F2/O1.H2/E3HA1/D1.H1/A1/F1/F1/F1/B1.H1/D3HA2/E1.H2/T2-P2-Q"
"2-G2-F3HA3HA1.H3HA3HA1.H1.H2/E1.H1/D1.H1.H3HA3HA1.H3HA3HA2-P2-Q"
"2-G2-F3HA1/A1/B1.H1/A1/B1.H1.H1.H1.H1.H1/A1/B1.H1/A1/B3HA2-P2-Q"
"2-G2-F1.H1/D3HA3HA1.H2/E1.H1/E1.H2/J1.H1/D1.H3HA3HA2/E1.H2-P2-Q"
"2-G2-F1.H1.H3HA1/C3HA3HA3HA1/A1/F1/B3HA3HA3HA1/C3HA1.H1.H2-P2-Q"
"2-G2-C2-A2-A2-A2-A2-A2-A2-A2-A2-A2-A2-A2-A2-A2-A2-A2-A2-A2-R2-Q"
"2-D2-B2-B2-B2-B2-B2-B2-B2-B2-B2-B2-B2-B2-B2-B2-B2-B2-B2-B2-B2-B";

	OnButton1();
}

void CPathfindDlg::OnButton2() 
{
	// Each cell is 3 characters, 21x16 total size
	m_Buffer="1/A1/A1/A1/A1/A1/A1/A1/A1/A1/A1/A1/A1/A1/A1/A1/A1/A1/A1/A1/A1/A"
"1/A1/A1/A1/A1/A1/A1/A1/A1/A1/A1/A1/A1/A1/A1/A1/A1/A1/A1/A1/A1/A"
"1/A1/A1/A1/A1/A1/A1/A1/A1/A1/A1/A1/A1/A1/A1/A1/A1/A1/A1/A1/A1/A"
"1/A1/A2.R2.M3fA3mA2.S1/A3rK3mA3fA3mA3sP1/A2.R3mA3fA2.M2.S1/A1/A"
"1/A1/A2.N1/A3nF1/A3nF1/A2.N1/B2.N1/C2.N1/A3nF1/A3nF1/A2.N1/A1/A"
"1/A1/A2.H3mA3lA3mA3lA2.M2.L2.M2.G2.M2.L2.M3lA3mA3lA3mA2.I1/A1/A"
"1/A1/A3nF1/B3nF1/C2.N1/A2.N1/A1/A1/A2.N1/A2.N1/B3nF1/C3nF1/A1/A"
"1/A1/A3pK3mA3gA3mA2.I1/A2.H2.M2.F2.M2.I1/A2.H3mA3gA3mA3qP1/A1/A"
"1/A1/A1/A1/A1/A1/A2.H2.M2.I1/A2.N1/A2.H2.M2.I1/A1/A1/A1/A1/A1/A"
"1/A1/A3rK3mA3fA3mA2.I1/A2.H2.M2.G2.M2.I1/A2.H3mA3fA3mA3sP1/A1/A"
"1/A1/A3nF1/B3nF1/C2.N1/A2.N1/A1/A1/A2.N1/A2.N1/B3nF1/C3nF1/A1/A"
"1/A1/A2.H3mA3lA3mA3lA2.M2.L2.M2.F2.M2.L2.M3lA3mA3lA3mA2.I1/A1/A"
"1/A1/A2.N1/A3nF1/A3nF1/A2.N1/B2.N1/C2.N1/A3nF1/A3nF1/A2.N1/A1/A"
"1/A1/A2.P2.M3gA3mA2.Q1/A3pK3mA3gA3mA3qP1/A2.P3mA3gA2.M2.Q1/A1/A"
"1/A1/A1/A1/A1/A1/A1/A1/A1/A1/A1/A1/A1/A1/A1/A1/A1/A1/A1/A1/A1/A"
"1/A1/A1/A1/A1/A1/A1/A1/A1/A1/A1/A1/A1/A1/A1/A1/A1/A1/A1/A1/A1/A";
	
	OnButton1();
}
