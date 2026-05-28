// SparkUfsPdtDlg.h: header file
//

#pragma once
#include <afxcmn.h> // include this header to use `CProgressCtrl`
#include <memory>
#include "libsparkusb.h"
#include "CDialogBase.h"
#include "CSerialPort.h"
#include "SerialDef.h"
extern char g_UfsIsp[UFS_ISP_SIZE];
#include "CImpState.h"
#include "EventMessages.h"
#include <array>

// CSparkUfsPdtDlg dialog
class CSparkUfsPdtDlg : public CDialogBase
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
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedBtnScanDevice();
    afx_msg void OnNMCustomdrawListDevice(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnBnClickedBtnPdtSetting();

protected:
    // Progress control ID base (choose an ID range not used by other controls)
    static constexpr int IDC_S_UI_THREAD_BASE = 2000;
    static constexpr int IDC_STATUS_BAR = 3000;

    // Progress values for owner-draw rendering
    // (overlay controls removed)
public:
    afx_msg void OnBnClickedBtnStartPdt();
    afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
    afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnTimer(UINT_PTR nIDEvent);
    afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
    afx_msg void OnSettingConfig();
    afx_msg void OnCbnSelchangeCbComSel();

    // Run PDT for a specific port index (0-based) and report progress to UI
    int RunPdtTask(int portIndex, const CString& allocatedSn = CString());

	// Static thread pool shared by dialog -- destroyed on dialog close to exit threads safely
	static std::unique_ptr<class ThreadPool> s_pool;

    // Custom message for worker threads to report progress to the UI thread.
    static const UINT WM_TASK_PROGRESS = MSG_WM_TASK_PROGRESS;
    // Synchronous request from worker thread to retrieve SerialNo text for a port.
    static const UINT WM_GET_PORT_SN = MSG_WM_GET_PORT_SN;
    afx_msg LRESULT OnTaskProgress(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnFactoryCmdDownload(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnFactoryCmdStartTest(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnGetPortSn(WPARAM wParam, LPARAM lParam);

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
    friend int RunFtTaskImpl(int portIndex, CImpState* state);
    friend int RunQcTaskImpl(int portIndex, CImpState* state);

    void CreateListViewColumns();
    void InitListViewItems();
    void RepositionProgressControls();
    void UpdatePdtNameText();
    void InitStatusBar();
    void UpdateStatusBarLayout();
    void UpdateStatusBarText();
    void ResetTaskCounts(int totalCount);

    // Timer id used to coalesce frequent scroll events and refresh overlays
    static constexpr UINT_PTR SCROLL_REFRESH_TIMER_ID = 0x1001;
    UINT_PTR m_scrollRefreshTimer = 0;

private:
    bool LoadSettingFromPath(const CString& path, bool showError);
    void SaveLastSettingPath(const CString& path);
    CString GetBaseSettingIniPath() const;
    void RefreshFactoryComList();
    bool ConnectFactorySerial(const CString& comName);
    void UpdateFactorySerialLinkIndicator();
    void DiagnosticSerialPortStatus();  // New diagnostic function
    CString GetDefaultFactoryCom() const;
    void SaveDefaultFactoryCom(const CString& comName);
    UINT GetFactoryComBaudRate() const;
    UINT GetFactoryComByteSize() const;
    UINT GetFactoryComParity() const;
    UINT GetFactoryComStopBits() const;
    bool IsPortRunnableStatus(const CString& status) const;
    void SetScanButtonEnabled(bool enabled);

    CBrush m_pdtNameBrush;
    CBrush m_factoryComLinkedBrushGreen;
    CBrush m_factoryComLinkedBrushRed;
    bool m_factoryComConnected = false;
    bool m_scanButtonDisabledByRun = false; // now private; use IsScanButtonDisabled() to read
    std::atomic<int> m_activeTaskCount{0};
    void IncrementActiveTasks(int n = 1);
    void DecrementActiveTasks(int n = 1);
    CString m_settingPath;
    CFont m_pdtNameFont;
    CStatusBarCtrl m_statusBar;
    CMenu m_mainMenu;
    int m_totalCount = 0;
    int m_passCount = 0;
    int m_failCount = 0;

    // Per-port state consolidated into a structure for clarity and easier maintenance
    struct PortState {
        bool completed = false;
        bool failed = false;
        int progress = 0;
        CStringW serial = CStringW(L"", 128);
        int groupIdx = -1;
        int groupPos = -1;
    };

    std::array<PortState, UI_THREAD_COUNT> m_ports;

    CSerialPort m_factorySerial;
    int m_groupPending[2] = { 0, 0 };
    WORD m_groupResult[2][MACHINE_DEVICE_CNT] = {};
public:
    // Per-port serials are stored in m_ports[i].serial
    // Subclass handling removed; rely on NM_CUSTOMDRAW for custom painting
    // Accessors used by list subclass drawing
    int GetPortProgress(int idx) const { return (idx >= 0 && idx < UI_THREAD_COUNT) ? m_ports[idx].progress : 0; }
    bool IsPortFailed(int idx) const { return (idx >= 0 && idx < UI_THREAD_COUNT) ? m_ports[idx].failed : false; }
    // Read-only accessor for scan button disabled state. Use Increment/DecrementActiveTasks to change.
    bool IsScanButtonDisabled() const { return m_scanButtonDisabledByRun; }
};