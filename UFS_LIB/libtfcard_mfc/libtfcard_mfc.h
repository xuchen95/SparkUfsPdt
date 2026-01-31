// libtfcard_mfc.h : main header file for the libtfcard_mfc DLL
//

#pragma once

#ifndef __AFXWIN_H__
	#error "include 'pch.h' before including this file for PCH"
#endif

#include "resource.h"		// main symbols


// ClibtfcardmfcApp
// See libtfcard_mfc.cpp for the implementation of this class
//

class ClibtfcardmfcApp : public CWinApp
{
public:
	ClibtfcardmfcApp();

// Overrides
public:
	virtual BOOL InitInstance();

	DECLARE_MESSAGE_MAP()
};
