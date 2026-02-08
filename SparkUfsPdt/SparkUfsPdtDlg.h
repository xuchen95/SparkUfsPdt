// SparkUfsPdtDlg.h: header file
//

#pragma once
#include <afxcmn.h> // include this header to use `CProgressCtrl`
#include <memory>
#include "libsparkusb.h"

static char g_UfsIsp[UFS_ISP_SIZE];

// CSparkUfsPdtDlg dialog
class CSparkUfsPdtDlg : public CDialogEx
{
// Construction
public:
    // 线程/端口数量常量定义
    static constexpr int UI_THREAD_COUNT = 16;

	CSparkUfsPdtDlg(CWnd* pParent = nullptr);	// standard constructor
	

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_SPARKUFSPDT_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持


// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnDestroy();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedBtnScanDevice();
    afx_msg void OnNMCustomdrawListDevice(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnBnClickedBtnPdtSetting();

protected:
    // Progress control ID base (choose an ID range not used by other controls)
    static constexpr int IDC_S_UI_THREAD_BASE = 2000;

    // Fix: declare progress control array
    CProgressCtrl m_progress[UI_THREAD_COUNT];
public:
    afx_msg void OnBnClickedBtnStartPdt();

    // Run PDT for a specific port index (0-based) and report progress to UI
    int RunPdtTask(int portIndex);

	// Static thread pool shared by dialog -- destroyed on dialog close to exit threads safely
	static std::unique_ptr<class ThreadPool> s_pool;

    // Custom message for worker threads to report progress to the UI thread.
    static const UINT WM_TASK_PROGRESS = (WM_USER + 0x65);
    afx_msg LRESULT OnTaskProgress(WPARAM wParam, LPARAM lParam);

    // Shared progress message structure used between worker code and UI handler
    struct TaskProgressMsg {
        int portIndex; // 0-based
        int progress; // 0-100
        int result; // final result code or 0 for ongoing
        CString statusText;
    };

    // Append a line to the PDT run log (implemented in Run file)
    static void AppendLogLine(const CString& line);

    // Globals for log locking (defined in Run file)
    static CRITICAL_SECTION g_logLock;
    static bool g_logLockInited;

    // Implementation entry point moved to a separate compilation unit. The
    // wrapper methods in the dialog call this function which receives the
    // port index and a pointer to the dialog instance for UI notifications.
    friend int RunFT1TaskImpl(int portIndex, CSparkUfsPdtDlg* pDlg);
    friend int RunFT3TaskImpl(int portIndex, CSparkUfsPdtDlg* pDlg);
	//CHAR m_szIspBuff[1024*512*2];
	afx_msg void OnBnClickedBtnPdtIni();
    void CreateListViewColumns();
    void InitListViewItems();
};



