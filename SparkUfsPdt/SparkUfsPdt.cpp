// SparkUfsPdt.cpp: defines application behavior.
//

#include "pch.h"
#include "framework.h"
#include "SparkUfsPdt.h"
#include "SparkUfsPdtDlg.h"
#include "../SparkLog/SparkLog.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CSparkUfsPdtApp

BEGIN_MESSAGE_MAP(CSparkUfsPdtApp, CWinApp)
	ON_COMMAND(ID_HELP, &CWinApp::OnHelp)
END_MESSAGE_MAP()


// CSparkUfsPdtApp constructor

CSparkUfsPdtApp::CSparkUfsPdtApp()
{
	// Support Restart Manager
	m_dwRestartManagerSupportFlags = AFX_RESTART_MANAGER_SUPPORT_RESTART;

	// TODO: add construction code here,
	// put all significant initialization in InitInstance
}


// 唯一的 CSparkUfsPdtApp 对象

CSparkUfsPdtApp theApp;


// CSparkUfsPdtApp initialization

BOOL CSparkUfsPdtApp::InitInstance()
{
    // On Windows XP, InitCommonControlsEx is required when using Common
    // Controls version 6 or later to enable visual styles. Without it,
    // window creation may fail.
	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);
	// 将它设置为包括所有要在应用程序中使用的
	// 公共控件类。
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);

	CWinApp::InitInstance();


	AfxEnableControlContainer();

    // Create the shell manager in case the dialog contains any
    // shell tree view or shell list view controls.
	CShellManager *pShellManager = new CShellManager;

    // Activate "Windows Native" visual manager to enable themes in MFC controls
	CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerWindows));

    // Standard initialization
    // If these features are not used and you wish to reduce the size
    // of the final executable, remove the unnecessary initialization
    // routines below. Change the registry key used to store settings
    // TODO: modify the string appropriately (company/organization name)
    SetRegistryKey(_T("Local AppWizard-Generated Applications"));

	CSparkUfsPdtDlg dlg;
	m_pMainWnd = &dlg;
	INT_PTR nResponse = dlg.DoModal();
	if (nResponse == IDOK)
	{
        // TODO: place code here to handle when the dialog is closed with OK
	}
	else if (nResponse == IDCANCEL)
	{
        // TODO: place code here to handle when the dialog is closed with Cancel
	}
	else if (nResponse == -1)
	{
        TRACE(traceAppMsg, 0, "Warning: dialog creation failed, application will terminate unexpectedly.\n");
        TRACE(traceAppMsg, 0, "Warning: if you use MFC controls on the dialog, you cannot #define _AFX_NO_MFC_CONTROLS_IN_DIALOGS.\n");
	}

    // Delete the shell manager created above.
	if (pShellManager != nullptr)
	{
		delete pShellManager;
	}

#if !defined(_AFXDLL) && !defined(_AFX_NO_MFC_CONTROLS_IN_DIALOGS)
	ControlBarCleanUp();
#endif

    // Since the dialog has been closed, return FALSE so that we exit the
    // application rather than start the application's message pump.
    return FALSE;
}

int CSparkUfsPdtApp::ExitInstance()
{
    // 程序退出时关闭日志线程
    SparkLog_Close();
    
    return CWinApp::ExitInstance();
}

