#if !defined(AFX_FRAMESEL_H__4F1694F5_B964_4332_94E5_AD879B7ECC9C__INCLUDED_)
#define AFX_FRAMESEL_H__4F1694F5_B964_4332_94E5_AD879B7ECC9C__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Framesel.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// Framesel dialog

class Framesel : public CDialog
{
// Construction
public:
	int nReturn;
	CString csFileName;
	Framesel(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(Framesel)
	enum { IDD = IDD_FRAMESEL };
	CStatic	m_FileText;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(Framesel)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(Framesel)
	afx_msg void OnBtn1();
	afx_msg void OnBtn2();
	afx_msg void OnBtn3();
	afx_msg void OnBtn4();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_FRAMESEL_H__4F1694F5_B964_4332_94E5_AD879B7ECC9C__INCLUDED_)
