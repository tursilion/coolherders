// Framesel.cpp : implementation file
//

#include "stdafx.h"
#include "CoolHerdersAnimTest.h"
#include "Framesel.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Framesel dialog


Framesel::Framesel(CWnd* pParent /*=NULL*/)
	: CDialog(Framesel::IDD, pParent)
{
	//{{AFX_DATA_INIT(Framesel)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void Framesel::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(Framesel)
	DDX_Control(pDX, IDC_FILETEXT, m_FileText);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(Framesel, CDialog)
	//{{AFX_MSG_MAP(Framesel)
	ON_BN_CLICKED(IDC_BTN1, OnBtn1)
	ON_BN_CLICKED(IDC_BTN2, OnBtn2)
	ON_BN_CLICKED(IDC_BTN3, OnBtn3)
	ON_BN_CLICKED(IDC_BTN4, OnBtn4)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Framesel message handlers

void Framesel::OnBtn1() 
{
	nReturn=0;
	EndDialog(IDOK);
}

void Framesel::OnBtn2() 
{
	nReturn=1;
	EndDialog(IDOK);
}

void Framesel::OnBtn3() 
{
	nReturn=2;
	EndDialog(IDOK);
}

void Framesel::OnBtn4() 
{
	nReturn=3;
	EndDialog(IDOK);
}

BOOL Framesel::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	m_FileText.SetWindowText(csFileName);
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
