// CoolHerdersAnimTest.h : main header file for the COOLHERDERSANIMTEST application
//

#if !defined(AFX_COOLHERDERSANIMTEST_H__D04F6683_F0F8_479E_BD42_1BBF38338E9D__INCLUDED_)
#define AFX_COOLHERDERSANIMTEST_H__D04F6683_F0F8_479E_BD42_1BBF38338E9D__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CCoolHerdersAnimTestApp:
// See CoolHerdersAnimTest.cpp for the implementation of this class
//

class CCoolHerdersAnimTestApp : public CWinApp
{
public:
	CCoolHerdersAnimTestApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CCoolHerdersAnimTestApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CCoolHerdersAnimTestApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_COOLHERDERSANIMTEST_H__D04F6683_F0F8_479E_BD42_1BBF38338E9D__INCLUDED_)
