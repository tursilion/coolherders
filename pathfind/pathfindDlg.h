// pathfindDlg.h : header file
//

#if !defined(AFX_PATHFINDDLG_H__95BDC0E3_6E90_403D_8AC1_012FF834D2DD__INCLUDED_)
#define AFX_PATHFINDDLG_H__95BDC0E3_6E90_403D_8AC1_012FF834D2DD__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/////////////////////////////////////////////////////////////////////////////
// CPathfindDlg dialog

class CPathfindDlg : public CDialog
{
// Construction
public:
	void DoPaths();
	CString m_Buffer;
	bool isFlat(int x, int y);
	CPathfindDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CPathfindDlg)
	enum { IDD = IDD_PATHFIND_DIALOG };
	CString	m_Edit;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPathfindDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	//{{AFX_MSG(CPathfindDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnButton1();
	afx_msg void OnButton6();
	afx_msg void OnButton5();
	afx_msg void OnButton4();
	afx_msg void OnButton3();
	afx_msg void OnButton2();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PATHFINDDLG_H__95BDC0E3_6E90_403D_8AC1_012FF834D2DD__INCLUDED_)
