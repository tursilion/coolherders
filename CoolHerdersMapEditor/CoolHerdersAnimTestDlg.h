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
	bool fMapDraw;
	void FixHighlights();
	void DrawBox(CPoint pt, BOOL fErase=FALSE);
	void GetMainTilePos(int inX, int inY, int*outX, int *outY);
	int nCurrentTileMode;
	int nCurrentTilePage;
	int nCurrentTile;
	bool fButtonDown;
	CCoolHerdersAnimTestDlg(CWnd* pParent = NULL);	// standard constructor
	void RedrawMap(int idx);

// Dialog Data
	//{{AFX_DATA(CCoolHerdersAnimTestDlg)
	enum { IDD = IDD_COOLHERDERSANIMTEST_DIALOG };
	CButton	ctlHideBreak;
	CStatic	m_Coord;
	CButton	m_ctlAnim;
	int		m_Terrain;
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
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnDropFiles(HDROP hDropInfo);
	afx_msg void OnClose();
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnFrame1();
	afx_msg void OnFrame2();
	afx_msg void OnFrame3();
	afx_msg void OnFrame4();
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnFlat();
	afx_msg void On3dimpass();
	afx_msg void OnImpass();
	afx_msg void OnNoterrain();
	afx_msg void OnUndo();
	afx_msg void OnSafe();
	afx_msg void OnAbort();
	virtual void OnOK();
	virtual void OnCancel();
	afx_msg void OnSave();
	afx_msg void OnHidebreak();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_COOLHERDERSANIMTESTDLG_H__73625552_5D9C_4ADF_A208_92AE3AF663BF__INCLUDED_)
