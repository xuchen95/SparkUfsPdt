// SparkUfsPdt.cpp: defines application behavior.
//

#include "pch.h"
#include "framework.h"
#include "SparkUfsPdt.h"
#include "SparkUfsPdtDlg.h"
#include "../SparkLog/SparkLog.h"
#include <memory>
#ifdef _DEBUG
#include <crtdbg.h>
#endif

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

// Debug memory leak detection removed

CSparkUfsPdtApp theApp;


// CSparkUfsPdtApp initialization

BOOL CSparkUfsPdtApp::InitInstance()
{
#ifdef _DEBUG
	// 临时在第 6658 次分配处触发断点以便调试分配来源
	static long s_breakAllocBlock = -1; // -1 = disabled
	if (s_breakAllocBlock != -1)
		_CrtSetBreakAlloc(s_breakAllocBlock);
#endif

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
	auto pShellManager = std::make_unique<CShellManager>();

    // Activate "Windows Native" visual manager to enable themes in MFC controls
	CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerWindows));

    // Standard initialization
    // If these features are not used and you wish to reduce the size
    // of the final executable, remove the unnecessary initialization
    // routines below. Change the registry key used to store settings
    // TODO: modify the string appropriately (company/organization name)
    SetRegistryKey(_T("Local AppWizard-Generated Applications"));

#ifdef _DEBUG
    // Debug memory leak detection disabled by request
#endif

#ifdef _DEBUG
#ifdef _DEBUG
    // Keep CRT reports directed to debugger output (default) so leaks appear in Visual Studio Output window
#endif
#endif

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
	// Smart pointer will automatically release pShellManager when it goes out of scope
	pShellManager.reset();
    // no-op: keep patch context anchored here

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

    // 先调用 MFC 基类清理，让 MFC 内部资源释放完毕
    int result = CWinApp::ExitInstance();

    // Debug memory leak detection disabled by request

    return result;
}

