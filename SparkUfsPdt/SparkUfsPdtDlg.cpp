// SparkUfsPdtDlg.cpp: implementation file
//

#include "pch.h"
#include "framework.h"
#include "SparkUfsPdt.h"
#include "SparkUfsPdtDlg.h"
#include "afxdialogex.h"
#include "libsparkusb.h"
//#include "../CommonLibs/ThreadPool/ThreadPool.h"
#include "ThreadPool.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

using namespace spark::sm3350;

// define static pool pointer
std::unique_ptr<ThreadPool> CSparkUfsPdtDlg::s_pool = nullptr;


// CSparkUfsPdtDlg dialog



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


// CSparkUfsPdtDlg message handlers

BOOL CSparkUfsPdtDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

    // Set the dialog icon. When the application's main window is not a
    // dialog, the framework performs this automatically.
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

    // TODO: add extra initialization here if needed
    // Initialize the list view and add rows for each port. A progress
    // control will be overlaid on the "Progress" column.
    CreateListViewColumns();
    return TRUE;  // return TRUE unless you set the focus to a control
}

// The following code draws the icon when the dialog is minimized.
// For MFC applications using the document/view model, the framework
// performs this automatically.

void CSparkUfsPdtDlg::OnPaint()
{
	if (IsIconic())
	{
        CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

        // center the icon in the client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

        // draw the icon
        dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// Called when the user drags the minimized window: return the cursor to display.
HCURSOR CSparkUfsPdtDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


void CSparkUfsPdtDlg::OnBnClickedBtnScanDevice()
{
    // Scan connected SM3350 devices and update the UI list for Ready ports.
    // This function enumerates devices via the CSparkSm3350Util helper and
    // sets the appropriate list row to "Ready" with drive and start time.
    InitListViewItems();
	UCHAR i;
	int nTesterCnt = 0;
    CString strDrive;
    // TODO: add control notification handler code here
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
                    // Mark the list row as Ready and update its start time
                    CListCtrl* pList = static_cast<CListCtrl*>(GetDlgItem(IDC_LIST_DEVICE));
                    if (pList)
                    {
                        // 查找对应的 Port 行（通过 Port 名称匹配）'
                        // Find corresponding Port row by port name
                        CHAR u08Id = CSparkSm3350Util::GetTesterIndex(i);
                        CString portName;
                        portName.Format(_T("Port %d"), u08Id + 1);
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
                                // Update start time to current time (column index 4)
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
    // Handle custom draw notifications for the device list control.
    // We only change the text color of the Status subitem when it
    // contains "Ready" to make it visually distinct.

    LPNMLVCUSTOMDRAW pLVCD = reinterpret_cast<LPNMLVCUSTOMDRAW>(pNMHDR);

    // Default processing
    *pResult = CDRF_DODEFAULT;

    if (pLVCD->nmcd.dwDrawStage == CDDS_ITEMPREPAINT)
    {
        // request subitem color notifications
        *pResult = CDRF_NOTIFYSUBITEMDRAW;
    }
    else if (pLVCD->nmcd.dwDrawStage == (CDDS_SUBITEM | CDDS_ITEMPREPAINT))
    {
        int nItem = static_cast<int>(pLVCD->nmcd.dwItemSpec);
        int nSubItem = pLVCD->iSubItem;
        // Status column index is 2
        if (nSubItem == 2)
        {
            CListCtrl* pList = static_cast<CListCtrl*>(GetDlgItem(IDC_LIST_DEVICE));
            if (pList)
            {
                CString txt = pList->GetItemText(nItem, nSubItem);
                if (txt.CompareNoCase(_T("Ready")) == 0)
                {
                    // set blue text color
                    pLVCD->clrText = RGB(0, 0, 255);
                }
            }
        }

        *pResult = CDRF_DODEFAULT;
    }
}

// Wrapper methods that forward to the implementation in RunPdtTaskImpl.cpp

int CSparkUfsPdtDlg::RunPdtTask(int portIndex)
{
    return RunFT3TaskImpl(portIndex, this);
}

// UI thread message handler for progress updates
LRESULT CSparkUfsPdtDlg::OnTaskProgress(WPARAM wParam, LPARAM lParam)
{
    // UI handler for progress updates coming from worker threads.
    // The function expects a pointer to TaskProgressMsg allocated by
    // the worker; after processing the pointer is deleted here.

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
    // Start PDT tasks for all Ready ports. Create the thread pool if
    // necessary and enqueue a task per Ready row. Errors during enqueue
    // are logged but do not abort other tasks.

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
                    CString err; err.Format(_T("Failed to start task for port %d"), i + 1);
                    AppendLogLine(err);
                }
            }
        }
    }
}

void CSparkUfsPdtDlg::OnDestroy()
{
    // Cleanup on dialog destroy: shut down the thread pool and delete
    // the log critical section if it was initialized.

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
    // Prompt the user to select an ISP binary file and read it into the
    // global `g_UfsIsp` buffer. Show a message box indicating success or
    // failure of the read operation.

    CString strSelectedFilePath;  // stores the selected file path
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
    fileDlg.m_ofn.lpstrInitialDir = strInitPath;  // set initial directory for file dialog
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

void CSparkUfsPdtDlg::CreateListViewColumns()
{
    CListCtrl* pList = static_cast<CListCtrl*>(GetDlgItem(IDC_LIST_DEVICE));
    if (pList)
    {
        // Set list view style to report view
        pList->ModifyStyle(0, LVS_REPORT | LVS_SHOWSELALWAYS);
        pList->SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

        // Define columns used by the UI
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

        // Insert rows and create a small progress control over the Progress cell
        for (int i = 0; i < CSparkUfsPdtDlg::UI_THREAD_COUNT; ++i)
        {
            CString port;
            port.Format(_T("Port %d"), i + 1);
            int idx = pList->InsertItem(i, port);
            pList->SetItemText(idx, 1, _T("")); // Progress column will be covered by progress control
            pList->SetItemText(idx, 2, _T(""));
            pList->SetItemText(idx, 3, _T("")); // Drive
            pList->SetItemText(idx, 4, _T(""));
            pList->SetItemText(idx, 5, _T("")); // 3350Version
            pList->SetItemText(idx, 6, _T("")); // SerialNo
            pList->SetItemText(idx, 7, _T("")); // MID
            pList->SetItemText(idx, 8, _T("")); // OID
            pList->SetItemText(idx, 9, _T("")); // FW Version
        }

        // Create an overlay progress control for the Progress column on
        // each list row and position it correctly inside the dialog.
        const int progressCol = 1;
        for (int i = 0; i < CSparkUfsPdtDlg::UI_THREAD_COUNT; ++i)
        {
            // Ensure the item is visible so we can retrieve the subitem rectangle
            pList->EnsureVisible(i, FALSE);
            CRect rcSubItem;
            pList->GetSubItemRect(i, progressCol, LVIR_BOUNDS, rcSubItem);
            // Convert the subitem rectangle from list client coordinates to dialog client coordinates
            pList->ClientToScreen(&rcSubItem);
            ScreenToClient(&rcSubItem);

            int nProgressId = CSparkUfsPdtDlg::IDC_S_UI_THREAD_BASE + CSparkUfsPdtDlg::UI_THREAD_COUNT + i;
            // Create the progress control overlay in the Progress column
            if (!m_progress[i].Create(WS_CHILD | WS_VISIBLE | PBS_SMOOTH, rcSubItem, this, nProgressId))
            {
                // failed to create, continue
            }
            m_progress[i].SetRange(0, 100);
            m_progress[i].SetPos(0);
        }
    }
}

void CSparkUfsPdtDlg::InitListViewItems()
{
    CListCtrl* pList = static_cast<CListCtrl*>(GetDlgItem(IDC_LIST_DEVICE));
    {
        // Insert rows and create a small progress control over the Progress cell
        for (int i = 0; i < CSparkUfsPdtDlg::UI_THREAD_COUNT; ++i)
        {
            CString port;
            port.Format(_T("Port %d"), i + 1);
            //pList->SetItemText(i, 1, _T("")); // Progress column will be covered by progress control
            pList->SetItemText(i, 2, _T(""));
            pList->SetItemText(i, 3, _T("")); // Drive
            pList->SetItemText(i, 4, _T(""));
            pList->SetItemText(i, 5, _T("")); // 3350Version
            pList->SetItemText(i, 6, _T("")); // SerialNo
            pList->SetItemText(i, 7, _T("")); // MID
            pList->SetItemText(i, 8, _T("")); // OID
            pList->SetItemText(i, 9, _T("")); // FW Version
        }

        // Create an overlay progress control for the Progress column on
        // each list row and position it correctly inside the dialog.
        for (int i = 0; i < CSparkUfsPdtDlg::UI_THREAD_COUNT; ++i)
        {
            // Ensure the item is visible so we can retrieve the subitem rectangle
            pList->EnsureVisible(i, FALSE);
            m_progress[i].SetRange(0, 100);
            m_progress[i].SetPos(0);
        }
    }
}
