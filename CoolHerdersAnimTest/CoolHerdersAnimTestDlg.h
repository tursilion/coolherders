// CoolHerdersAnimTestDlg.h : header file
//

#if !defined(AFX_COOLHERDERSANIMTESTDLG_H__73625552_5D9C_4ADF_A208_92AE3AF663BF__INCLUDED_)
#define AFX_COOLHERDERSANIMTESTDLG_H__73625552_5D9C_4ADF_A208_92AE3AF663BF__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/////////////////////////////////////////////////////////////////////////////
// CCoolHerdersAnimTestDlg dialog

class CCoolHerdersAnimTestDlg : public CDialog
{
// Construction
public:
	CCoolHerdersAnimTestDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CCoolHerdersAnimTestDlg)
	enum { IDD = IDD_COOLHERDERSANIMTEST_DIALOG };
	CButton	m_ctlQuit;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CCoolHerdersAnimTestDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	//{{AFX_MSG(CCoolHerdersAnimTestDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnButton1();
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnBigger();
	afx_msg void OnQuit();
	afx_msg void OnSmaller();
	afx_msg void OnStepforward();
	afx_msg void OnStepback();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_COOLHERDERSANIMTESTDLG_H__73625552_5D9C_4ADF_A208_92AE3AF663BF__INCLUDED_)
