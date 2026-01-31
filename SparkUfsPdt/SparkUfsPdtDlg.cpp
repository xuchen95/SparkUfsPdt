// SparkUfsPdtDlg.cpp: 实现文件
//

#include "pch.h"
#include "framework.h"
#include "SparkUfsPdt.h"
#include "SparkUfsPdtDlg.h"
#include "afxdialogex.h"
#include "libsparkusb.h"
//#include "../CommonLibs/ThreadPool/ThreadPool.h"
#include "ThreadPool.h"
#include <chrono>
#include <vector>
#include <fstream>
#include <sstream>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

using namespace spark::sm3350;

// define static pool pointer
std::unique_ptr<ThreadPool> CSparkUfsPdtDlg::s_pool = nullptr;

// simple critical section to protect log file writes
static CRITICAL_SECTION g_logLock;
static bool g_logLockInited = false;

// CSparkUfsPdtDlg 对话框



CSparkUfsPdtDlg::CSparkUfsPdtDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_SPARKUFSPDT_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CSparkUfsPdtDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CSparkUfsPdtDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_BTN_SCAN_DEVICE, &CSparkUfsPdtDlg::OnBnClickedBtnScanDevice)
	ON_NOTIFY(NM_CUSTOMDRAW, IDC_LIST_DEVICE, &CSparkUfsPdtDlg::OnNMCustomdrawListDevice)
    ON_BN_CLICKED(IDC_BTN_START_PDT, &CSparkUfsPdtDlg::OnBnClickedBtnStartPdt)
    ON_BN_CLICKED(IDC_BTN_PDT_INI, &CSparkUfsPdtDlg::OnBnClickedBtnPdtIni)
    ON_MESSAGE(WM_USER+0x65, &CSparkUfsPdtDlg::OnTaskProgress)
END_MESSAGE_MAP()


// CSparkUfsPdtDlg 消息处理程序

BOOL CSparkUfsPdtDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

    // TODO: 在此添加额外的初始化代码
    // 初始化列表视图列并添加 16 个端口行，同时在 "Progress" 列上覆盖进度条控件
    CListCtrl* pList = static_cast<CListCtrl*>(GetDlgItem(IDC_LIST_DEVICE));
    if (pList)
    {
        // 设置样式为报表视图
        pList->ModifyStyle(0, LVS_REPORT | LVS_SHOWSELALWAYS);
        pList->SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

        // 插入列：Port, Progress, Status, Drive, Start Time, 3350Version, SerialNo, MID, OID, FW Version
        pList->InsertColumn(0, _T("Port"), LVCFMT_LEFT, 60);
        pList->InsertColumn(1, _T("Progress"), LVCFMT_LEFT, 150);
        pList->InsertColumn(2, _T("Status"), LVCFMT_LEFT, 80);
        pList->InsertColumn(3, _T("Drive"), LVCFMT_LEFT, 60);
        pList->InsertColumn(4, _T("Start Time"), LVCFMT_LEFT, 120);
        pList->InsertColumn(5, _T("3350Version"), LVCFMT_LEFT, 100);
        pList->InsertColumn(6, _T("SerialNo"), LVCFMT_LEFT, 120);
        pList->InsertColumn(7, _T("MID"), LVCFMT_LEFT, 80);
        pList->InsertColumn(8, _T("OID"), LVCFMT_LEFT, 80);
        pList->InsertColumn(9, _T("FW Version"), LVCFMT_LEFT, 100);

        // 插入 16 行默认数据
        for (int i = 0; i < CSparkUfsPdtDlg::UI_THREAD_COUNT; ++i)
        {
            CString port;
            port.Format(_T("Port %d"), i + 1);
            int idx = pList->InsertItem(i, port);
            pList->SetItemText(idx, 1, _T("")); // Progress 列由进度条覆盖
            pList->SetItemText(idx, 2, _T("Idle"));
            pList->SetItemText(idx, 3, _T("")); // Drive
            // 设置开始时间为当前时间的字符串（可在实际开始时更新）'
            CTime now = CTime::GetCurrentTime();
            pList->SetItemText(idx, 4, now.Format(_T("%Y-%m-%d %H:%M:%S")));
            pList->SetItemText(idx, 5, _T("")); // 3350Version
            pList->SetItemText(idx, 6, _T("")); // SerialNo
            pList->SetItemText(idx, 7, _T("")); // MID
            pList->SetItemText(idx, 8, _T("")); // OID
            pList->SetItemText(idx, 9, _T("")); // FW Version
        }

        // 为每一行在 Progress 列上创建进度控件并放置到正确位置
        const int progressCol = 1;
        for (int i = 0; i < CSparkUfsPdtDlg::UI_THREAD_COUNT; ++i)
        {
            // 确保项可见以便获取子项矩形
            pList->EnsureVisible(i, FALSE);
            CRect rcSubItem;
            pList->GetSubItemRect(i, progressCol, LVIR_BOUNDS, rcSubItem);
            // 将子项矩形从 list 客户区坐标转换为对话框客户区坐标
            pList->ClientToScreen(&rcSubItem);
            ScreenToClient(&rcSubItem);

            int nProgressId = CSparkUfsPdtDlg::IDC_S_UI_THREAD_BASE + CSparkUfsPdtDlg::UI_THREAD_COUNT + i;
            // 创建进度条覆盖在 Progress 列上
            if (!m_progress[i].Create(WS_CHILD | WS_VISIBLE | PBS_SMOOTH, rcSubItem, this, nProgressId))
            {
                // failed to create, continue
            }
            m_progress[i].SetRange(0, 100);
            m_progress[i].SetPos(0);
        }
    }

    return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CSparkUfsPdtDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CSparkUfsPdtDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


void CSparkUfsPdtDlg::OnBnClickedBtnScanDevice()
{
	UCHAR i;
	int nTesterCnt = 0;
    CString strDrive;
	// TODO: 在此添加控件通知处理程序代码
	if (CSparkSm3350Util::EnumSm3350())
	{
		{
			for (i = 0; i < MAX_DEVICE_CNT; i++)
			{
				PST_DEVICE_INFO pInfo = CSparkSm3350Util::GetDeviceInfo(i);

				if (pInfo != nullptr)
				{
					nTesterCnt++;
                    strDrive = CString(pInfo->szDriveName);
                    // 在列表中显示 Ready 状态，并更新相应行的起始时间
                    CListCtrl* pList = static_cast<CListCtrl*>(GetDlgItem(IDC_LIST_DEVICE));
                    if (pList)
                    {
                        // 查找对应的 Port 行（通过 Port 名称匹配）'
                        CString portName;
                        portName.Format(_T("Port %d"), i + 1);
                        LVFINDINFO fi = { 0 };
                        fi.flags = LVFI_STRING;
                        fi.psz = portName;
                        int found = pList->FindItem(&fi);
                        if (found >= 0)
                        {
                            if (pInfo != nullptr)
                            {
                                CString drv;
                                drv.Format(_T("%hs"), strDrive);
                                pList->SetItemText(found, 3, drv);
                                pList->SetItemText(found, 2, _T("Ready"));
                                // 更新开始时间为当前时间 (列索引为 4)
                                CTime now = CTime::GetCurrentTime();
                                pList->SetItemText(found, 4, now.Format(_T("%Y-%m-%d %H:%M:%S")));
                            }
                        }
                    }
				}
			}
		}
	}
}

void CSparkUfsPdtDlg::OnNMCustomdrawListDevice(NMHDR* pNMHDR, LRESULT* pResult)
{
    LPNMLVCUSTOMDRAW pLVCD = reinterpret_cast<LPNMLVCUSTOMDRAW>(pNMHDR);

    // 默认处理
    *pResult = CDRF_DODEFAULT;

    if (pLVCD->nmcd.dwDrawStage == CDDS_ITEMPREPAINT)
    {
        // 请求子项颜色
        *pResult = CDRF_NOTIFYSUBITEMDRAW;
    }
    else if (pLVCD->nmcd.dwDrawStage == (CDDS_SUBITEM | CDDS_ITEMPREPAINT))
    {
        int nItem = static_cast<int>(pLVCD->nmcd.dwItemSpec);
        int nSubItem = pLVCD->iSubItem;

        // Status 列索引为 2
        if (nSubItem == 2)
        {
            CListCtrl* pList = static_cast<CListCtrl*>(GetDlgItem(IDC_LIST_DEVICE));
            if (pList)
            {
                CString txt = pList->GetItemText(nItem, nSubItem);
                if (txt.CompareNoCase(_T("Ready")) == 0)
                {
                    // 蓝色文字
                    pLVCD->clrText = RGB(0, 0, 255);
                }
            }
        }

        *pResult = CDRF_DODEFAULT;
    }
}

UINT CSparkUfsPdtDlg::DoThreadClickedButtonStartPdt(LPVOID pParam)
{
    CSparkUfsPdtDlg* dlg = (CSparkUfsPdtDlg*)pParam;
    if (!dlg) return 0;
    int ret = dlg->RunPdtTask();
    return (UINT)ret;
}

// Worker -> UI progress notification structure
struct TaskProgressMsg {
    int portIndex; // 0-based
    int progress; // 0-100
    int result; // final result code or 0 for ongoing
    CString statusText;
};

// Helper: append a line to the log file protected by critical section
static void AppendLogLine(const CString& line)
{
    if (!g_logLockInited)
    {
        InitializeCriticalSection(&g_logLock);
        g_logLockInited = true;
    }
    EnterCriticalSection(&g_logLock);
    FILE* fp = NULL;
    errno_t e = fopen_s(&fp, "pdt_run_log.txt", "a");
    if (e == 0 && fp)
    {
        CT2A lineA(line);
        fprintf(fp, "%s\n", lineA.m_psz);
        fclose(fp);
    }
    LeaveCriticalSection(&g_logLock);
}

// New: run the PDT logic as an instance method so it can be invoked by thread-pool tasks
int CSparkUfsPdtDlg::RunPdtTask()
{
    // legacy no-arg entry: run on port 0
    return RunPdtTask(0);
}

// Run PDT for specific port (0-based). Reports progress to UI and logs result.
int CSparkUfsPdtDlg::RunPdtTask(int portIndex)
{
    auto tStart = std::chrono::steady_clock::now();
    int ret = 0;
    CHAR pData[512];
    CHAR pPortInfo[1024];
    struct StageRecord { CString name; int code; double ms; CString timeStr; };
    const int MAX_STAGES = 16;
    StageRecord records[MAX_STAGES];
    int recCount = 0;

    PST_DEVICE_INFO pDeviceInfo = CSparkSm3350Util::GetDeviceInfo((UCHAR)portIndex);
    if (!pDeviceInfo)
    {
        ret = -1;
        CString line;
        CTime now = CTime::GetCurrentTime();
        line.Format(_T("%s | Port %d | ERROR: device not found"), now.Format(_T("%Y-%m-%d %H:%M:%S")), portIndex+1);
        AppendLogLine(line);
        // notify UI
        TaskProgressMsg* msg = new TaskProgressMsg{portIndex, 0, ret, _T("Device not found")};
        PostMessage(WM_USER+0x65, (WPARAM)msg, 0);
        return ret;
    }

    UCHAR u08Idx = CSparkSm3350Util::GetTesterIndex((UCHAR)portIndex);
    CSparkSm3350Util& sm3350 = CSparkSm3350Util::getInstance(u08Idx);
    LARGE_INTEGER freq;
    if (!QueryPerformanceFrequency(&freq)) freq.QuadPart = 1000;

#define RUN_STAGE_P(stageName, callExpr) \
    do { \
        LARGE_INTEGER t0,t1; QueryPerformanceCounter(&t0); \
        int rc = (callExpr); \
        QueryPerformanceCounter(&t1); \
        double ms = ((double)(t1.QuadPart - t0.QuadPart) * 1000.0)/ (double)freq.QuadPart; \
        CTime t = CTime::GetCurrentTime(); \
        CString timeStr = t.Format(_T("%Y-%m-%d %H:%M:%S")); \
        if (recCount < MAX_STAGES) { records[recCount].name = (stageName); records[recCount].code = rc; records[recCount].ms = ms; records[recCount].timeStr = timeStr; recCount++; } \
        ret = rc; \
        /* post intermediate progress message */ \
        TaskProgressMsg* pmsg = new TaskProgressMsg{portIndex, (int)((recCount*100)/MAX_STAGES), ret, CString(stageName)}; \
        PostMessage(WM_USER+0x65, (WPARAM)pmsg, 0); \
        if (ret != ERROR_SUCCESS) break; \
    } while(0)

    if (ERROR_SUCCESS == sm3350.DeviceSelect(u08Idx))
    {
        RUN_STAGE_P(_T("UfsPowerOff"), sm3350.UfsPowerOff(pData));
        if (ret == ERROR_SUCCESS) RUN_STAGE_P(_T("UfsPowerOn"), sm3350.UfsPowerOn(pData));
        if (ret == ERROR_SUCCESS) RUN_STAGE_P(_T("UfsReadPortInfo"), sm3350.UfsReadPortInfo(pPortInfo));
        if (ret == ERROR_SUCCESS) RUN_STAGE_P(_T("UfsCardInit"), sm3350.UfsCardInit(pData));
        if (ret == ERROR_SUCCESS) RUN_STAGE_P(_T("VccOffForceRom"), sm3350.VccOffForceRom(pData));
        if (ret == ERROR_SUCCESS) RUN_STAGE_P(_T("UfsMpStartMode"), sm3350.UfsMpStartMode(pData));
        if (ret == ERROR_SUCCESS) RUN_STAGE_P(_T("UfsWrite1024KIspMp"), sm3350.UfsWrite1024KIspMp(g_UfsIsp, BYTE2SECTOR(sizeof(g_UfsIsp)), FALSE));
        if (ret == ERROR_SUCCESS) RUN_STAGE_P(_T("UfsMpExit"), sm3350.UfsMpExit(pData));
    }

#undef RUN_STAGE_P

    auto tEnd = std::chrono::steady_clock::now();
    auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(tEnd - tStart).count();

    // write log
    CTime now = CTime::GetCurrentTime();
    CString header;
    header.Format(_T("%s | Port %d | TotalMs=%lld | Result=0x%X"), now.Format(_T("%Y-%m-%d %H:%M:%S")), portIndex+1, (long long)dur, ret);
    AppendLogLine(header);
    for (int i = 0; i < recCount; ++i)
    {
        CString line;
        line.Format(_T("%s | Port %d | Stage=%s | code=0x%X | %.3f ms"), records[i].timeStr, portIndex+1, records[i].name.GetString(), records[i].code, records[i].ms);
        AppendLogLine(line);
    }

    // final UI notify
    TaskProgressMsg* finalMsg = new TaskProgressMsg{portIndex, 100, ret, (ret==ERROR_SUCCESS)?_T("Success"):_T("Failed")};
    PostMessage(WM_USER+0x65, (WPARAM)finalMsg, 0);

    return ret;
}

// UI thread message handler for progress updates
LRESULT CSparkUfsPdtDlg::OnTaskProgress(WPARAM wParam, LPARAM lParam)
{
    TaskProgressMsg* msg = reinterpret_cast<TaskProgressMsg*>(wParam);
    if (!msg) return 0;
    int port = msg->portIndex;
    int progress = msg->progress;
    int result = msg->result;
    CString status = msg->statusText;

    // update progress control and status column
    if (port >=0 && port < UI_THREAD_COUNT)
    {
        m_progress[port].SetPos(progress);
        CListCtrl* pList = static_cast<CListCtrl*>(GetDlgItem(IDC_LIST_DEVICE));
        if (pList)
        {
            CString portName; portName.Format(_T("Port %d"), port+1);
            LVFINDINFO fi = {0}; fi.flags = LVFI_STRING; fi.psz = portName;
            int idx = pList->FindItem(&fi);
            if (idx >= 0)
            {
                if (result == 0 || result == ERROR_SUCCESS)
                    pList->SetItemText(idx, 2, status);
                else
                {
                    CString st; st.Format(_T("%s (0x%X)"), status.GetString(), result);
                    pList->SetItemText(idx, 2, st);
                }
                if (progress >= 100)
                {
                    CTime t = CTime::GetCurrentTime();
                    pList->SetItemText(idx, 4, t.Format(_T("%Y-%m-%d %H:%M:%S")));
                }
            }
        }
    }

    delete msg;
    return 0;
}

void CSparkUfsPdtDlg::OnBnClickedBtnStartPdt()
{
    // Initialize static pool if not created. Use 4 threads as default.
    if (!s_pool)
    {
        s_pool.reset(new ThreadPool(16));
    }
    if (!g_logLockInited)
    {
        InitializeCriticalSection(&g_logLock);
        g_logLockInited = true;
    }
    // enqueue a task per detected Ready device
    CListCtrl* pList = static_cast<CListCtrl*>(GetDlgItem(IDC_LIST_DEVICE));
    if (pList)
    {
        for (int i = 0; i < UI_THREAD_COUNT; ++i)
        {
            CString status = pList->GetItemText(i, 2);
            if (status.CompareNoCase(_T("Ready")) == 0)
            {
                try {
                    s_pool->enqueue([this, i]() { return this->RunPdtTask(i); });
                }
                catch (const std::exception&)
                {
                    // enqueue failed for this port
                    CString err; err.Format(_T("Failed to start task for port %d"), i+1);
                    AppendLogLine(err);
                }
            }
        }
    }
    else
    {
        // fallback: single task
        try { s_pool->enqueue([this]() { return this->RunPdtTask(); }); }
        catch (...) { MessageBox("ThreadPool enqueue fail,please call FAE."); }
    }
}

void CSparkUfsPdtDlg::OnDestroy()
{
    CDialogEx::OnDestroy();

    // destroy the static thread pool to join worker threads gracefully
    if (s_pool)
    {
        s_pool.reset(); // destructor will join worker threads
    }

    if (g_logLockInited)
    {
        DeleteCriticalSection(&g_logLock);
        g_logLockInited = false;
    }
}



void CSparkUfsPdtDlg::OnBnClickedBtnPdtIni()
{
    CString strSelectedFilePath;  // 存储选中的文件路径
    char CurrentDirectory[MAX_PATH];
    CString strInitPath;
    int ret = GetCurrentDirectory(MAX_PATH, CurrentDirectory);
    strInitPath.Format("%s", CurrentDirectory);
    strInitPath.AppendFormat("\\System");
    CFileDialog fileDlg(TRUE,
        _T("bin"),
        _T(""),
        OFN_FILEMUSTEXIST | OFN_HIDEREADONLY,
        _T("Binary Files (*.bin)|*.bin|All Files(*.*)|*.*||"));
    fileDlg.m_ofn.lpstrInitialDir = strInitPath;  // 设置初始选择的文件夹路径
    if (IDOK == fileDlg.DoModal())
    {
        strSelectedFilePath = fileDlg.GetPathName();
    }
    if (spark::file::fnReadFile(strSelectedFilePath, (PCHAR)g_UfsIsp))
    {
        MessageBox(_T("ISP Info Read error!"), _T("Spark UFS Card PDT"), MB_ICONERROR);
    }
    else
    {
        MessageBox(_T("ISP Info Read successful!"), _T("Spark UFS Card PDT"), MB_OK);
    }
}
