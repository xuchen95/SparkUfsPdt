// SparkUfsPdt.cpp: defines application behavior.
//

#include "pch.h"
#include "framework.h"
#include "SparkUfsPdt.h"
#include "SparkUfsPdtDlg.h"
#include "../SparkLog/SparkLog.h"
#include <memory>

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

#ifdef _DEBUG
static _CrtMemState s_initMemState;
#endif

CSparkUfsPdtApp theApp;


// CSparkUfsPdtApp initialization

BOOL CSparkUfsPdtApp::InitInstance()
{
#ifdef _DEBUG
    // ① BreakAlloc 必须在所有分配之前设置，否则目标块已分配，断点永远不触发。
    //    若块号在 InitInstance 之前（全局构造阶段），
    //    则需在文件顶部 theApp 之前通过全局 helper 对象设置。
    static long s_breakAllocBlock =-1;   // ← 改为泄漏块号，-1 = 禁用
    if (s_breakAllocBlock != -1)
        _CrtSetBreakAlloc(s_breakAllocBlock);

    // 只启用分配跟踪；不设置 LEAK_CHECK_DF / DELAY_FREE_MEM_DF
    int flags = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
    flags |= _CRTDBG_ALLOC_MEM_DF;
    _CrtSetDbgFlag(flags);

    _CrtSetReportMode(_CRT_WARN,  _CRTDBG_MODE_DEBUG);
    _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_DEBUG);
    // 注意：基准在下方，紧靠 DoModal，排除 MFC 初始化噪音
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
    // 基准点：MFC 框架初始化完毕后，对话框运行前。
    // 排除 CMFCVisualManager 单例、m_pszRegistryKey 等由 ~CWinApp() 释放的
    // 框架内部分配块（437、443 等），只报告对话框执行期间的真实泄漏。
    _CrtMemCheckpoint(&s_initMemState);
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

#ifdef _DEBUG
    // 只 dump InitInstance 基准之后的分配，排除全局构造阶段的早期块。
    // 若输出中有带文件/行号的块，则为真实的用户代码泄漏。
    // 若只看到 "Dumping objects -> Object dump complete."，则无泄漏。
    _RPT0(_CRT_WARN, "=== Leak check (allocations since InitInstance) ===\n");
    _CrtMemDumpAllObjectsSince(&s_initMemState);
#endif

    return result;
}

