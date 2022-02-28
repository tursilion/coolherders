// pathfind.h : main header file for the PATHFIND application
//

#if !defined(AFX_PATHFIND_H__97276247_2DE0_4838_9B68_8E599954D556__INCLUDED_)
#define AFX_PATHFIND_H__97276247_2DE0_4838_9B68_8E599954D556__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CPathfindApp:
// See pathfind.cpp for the implementation of this class
//

class CPathfindApp : public CWinApp
{
public:
	CPathfindApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPathfindApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CPathfindApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PATHFIND_H__97276247_2DE0_4838_9B68_8E599954D556__INCLUDED_)
