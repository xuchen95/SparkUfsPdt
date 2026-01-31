// SparkUfsPdtDlg.h: 头文件
//

#pragma once
#include <afxcmn.h> // 添加此头文件以使用CProgressCtrl
#include <memory>


static char g_UfsIsp[1024 * 512 * 2];

// CSparkUfsPdtDlg 对话框
class CSparkUfsPdtDlg : public CDialogEx
{
// 构造
public:
    // 线程/端口数量常量定义
    static constexpr int UI_THREAD_COUNT = 16;

	CSparkUfsPdtDlg(CWnd* pParent = nullptr);	// 标准构造函数
	

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_SPARKUFSPDT_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持


// 实现
protected:
	HICON m_hIcon;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnDestroy();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedBtnScanDevice();
    afx_msg void OnNMCustomdrawListDevice(NMHDR* pNMHDR, LRESULT* pResult);

protected:
    // 进度条控件ID起始值（建议使用未被其他控件占用的ID范围）
    static constexpr int IDC_S_UI_THREAD_BASE = 2000;

    // 修复：添加进度条控件数组声明
    CProgressCtrl m_progress[UI_THREAD_COUNT];
public:
	static UINT DoThreadClickedButtonStartPdt(LPVOID pParam);
	afx_msg void OnBnClickedBtnStartPdt();

    // New: move PDT work into an instance method so it can be invoked by a thread pool task
    int RunPdtTask();
    // Run PDT for a specific port index (0-based) and report progress to UI
    int RunPdtTask(int portIndex);

	// Static thread pool shared by dialog -- destroyed on dialog close to exit threads safely
	static std::unique_ptr<class ThreadPool> s_pool;

    // Custom message for worker threads to report progress to the UI thread.
    static const UINT WM_TASK_PROGRESS = (WM_USER + 0x65);
    afx_msg LRESULT OnTaskProgress(WPARAM wParam, LPARAM lParam);

	//CHAR m_szIspBuff[1024*512*2];
	afx_msg void OnBnClickedBtnPdtIni();
};



