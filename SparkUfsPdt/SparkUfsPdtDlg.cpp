// SparkUfsPdtDlg.cpp: implementation file
//

#include "pch.h"
#include "framework.h"
#include "SparkUfsPdt.h"
#include "SparkUfsPdtDlg.h"
#include "afxdialogex.h"
#include "libsparkusb.h"
#include "CDialogSetting.h"
#include "CDialogBaseSet.h"
#include "ThreadPool.h"
#include "../SparkLog/SparkLog.h"
#include <cerrno>


#ifdef _DEBUG
#define new DEBUG_NEW
#endif
//maybe have to change include path
using namespace spark::sm3350;

char g_UfsIsp[UFS_ISP_SIZE] = {};

static bool AcquireAndAdvanceSerialNumber(CString& allocatedSn)
{
    allocatedSn.Empty();

    PST_UFS_BASE_SETTING pBase = CDialogBase::GetSharedBaseSetting();
    if (pBase == nullptr || !pBase->bSnSeparateIni || pBase->szRemoteSnPath[0] == '\0')
    {
        return false;
    }

    CString iniPath = CA2T(pBase->szRemoteSnPath);
    if (iniPath.IsEmpty() || GetFileAttributes(iniPath) == INVALID_FILE_ATTRIBUTES)
    {
        return false;
    }

    WCHAR snBuf[128] = {};
    DWORD len = GetPrivateProfileStringW(L"TEST", L"SerialNumber", L"", snBuf, _countof(snBuf), CT2W(iniPath));
    if (len == 0)
    {
        return false;
    }

    CStringW currentSn(snBuf);
    currentSn.Trim();
    if (currentSn.IsEmpty())
    {
        return false;
    }

    errno = 0;
    wchar_t* endPtr = nullptr;
    unsigned long long currentValue = wcstoull(currentSn, &endPtr, 10);
    if (errno != 0 || endPtr == currentSn.GetString() || *endPtr != L'\0')
    {
        return false;
    }

    allocatedSn = CString(currentSn);

    unsigned long long nextValue = currentValue + 1;
    int width = currentSn.GetLength();
    CStringW nextSn;
    nextSn.Format(L"%0*llu", width > 0 ? width : 1, nextValue);

    if (!WritePrivateProfileStringW(L"TEST", L"SerialNumber", nextSn, CT2W(iniPath)))
    {
        return false;
    }

    return true;
}

static bool ReadTextFileA(const CString& path, CStringA& content)
{
    CFile file;
    if (!file.Open(path, CFile::modeRead | CFile::typeBinary))
    {
        return false;
    }
    ULONGLONG length = file.GetLength();
    CStringA data;
    UINT toRead = static_cast<UINT>(length);
    char* buffer = data.GetBuffer(toRead);
    UINT read = file.Read(buffer, toRead);
    data.ReleaseBuffer(read);
    content = data;
    return true;
}

static CString GetGitVersionString()
{
    TCHAR modulePath[MAX_PATH] = {};
    GetModuleFileName(nullptr, modulePath, MAX_PATH);
    CString dir = modulePath;
    int pos = dir.ReverseFind(_T('\\'));
    if (pos >= 0)
    {
        dir = dir.Left(pos);
    }

    CStringA headContent;
    CString gitHash;
    CString searchDir = dir;
    for (int i = 0; i < 6 && !searchDir.IsEmpty(); ++i)
    {
        CString headPath = searchDir + _T("\\.git\\HEAD");
        if (GetFileAttributes(headPath) != INVALID_FILE_ATTRIBUTES && ReadTextFileA(headPath, headContent))
        {
            headContent.Trim();
            if (headContent.Left(4) == "ref:")
            {
                CStringA refPathA = headContent.Mid(4);
                refPathA.Trim();
                CString refPath = searchDir + _T("\\.git\\") + CString(refPathA);
                CStringA refContent;
                if (ReadTextFileA(refPath, refContent))
                {
                    refContent.Trim();
                    gitHash = CString(refContent);
                }
            }
            else
            {
                gitHash = CString(headContent);
            }
            break;
        }

        int lastSlash = searchDir.ReverseFind(_T('\\'));
        if (lastSlash < 0)
        {
            break;
        }
        searchDir = searchDir.Left(lastSlash);
    }

    if (gitHash.IsEmpty())
    {
        return _T("unknown");
    }

    if (gitHash.GetLength() > 8)
    {
        gitHash = gitHash.Left(8);
    }
    return gitHash;
}


// define static pool pointer
std::unique_ptr<ThreadPool> CSparkUfsPdtDlg::s_pool = nullptr;


// CSparkUfsPdtDlg dialog



CSparkUfsPdtDlg::CSparkUfsPdtDlg(CWnd* pParent /*=nullptr*/)
	: CDialogBase(IDD_SPARKUFSPDT_DIALOG, pParent)
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
	ON_WM_CTLCOLOR()
	ON_WM_SIZE()
	ON_BN_CLICKED(IDC_BTN_SCAN_DEVICE, &CSparkUfsPdtDlg::OnBnClickedBtnScanDevice)
	ON_NOTIFY(NM_CUSTOMDRAW, IDC_LIST_DEVICE, &CSparkUfsPdtDlg::OnNMCustomdrawListDevice)
	ON_BN_CLICKED(IDC_BTN_START_PDT, &CSparkUfsPdtDlg::OnBnClickedBtnStartPdt)
	ON_BN_CLICKED(IDC_BTN_PDT_SETTING, &CSparkUfsPdtDlg::OnBnClickedBtnPdtSetting)
	ON_COMMAND(ID_SETTING_CONFIG, &CSparkUfsPdtDlg::OnSettingConfig)
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

    CString title;
    title.Format(_T("Metorage UFS TOOL VER %s"), GetGitVersionString().GetString());
    SetWindowText(title);

    CString baseIniPath = GetBaseSettingIniPath();
    CDialogBase::LoadBaseSettingFromIni(baseIniPath);
    if (PST_UFS_BASE_SETTING pBase = CDialogBase::GetSharedBaseSetting())
    {
        if (pBase->szReportPath[0] != '\0')
        {
            SparkLog_SetReportPath(pBase->szReportPath);
        }
    }

    if (m_mainMenu.LoadMenu(IDR_MAINMENU))
    {
        SetMenu(&m_mainMenu);
    }

    CreateListViewColumns();

	if (CWnd* pWnd = GetDlgItem(IDC_S_PDT_NAME))
	{
		pWnd->ModifyStyle(SS_LEFT | SS_RIGHT, SS_CENTER | SS_CENTERIMAGE);
	}

	m_pdtNameBrush.DeleteObject();
	m_pdtNameBrush.CreateSolidBrush(RGB(255, 255, 255));
	m_settingPath.Empty();
	UpdatePdtNameText();
	InitStatusBar();
	ResetTaskCounts(0);

    if (CWnd* pStart = GetDlgItem(IDC_BTN_START_PDT))
    {
        pStart->EnableWindow(FALSE);
    }

    TCHAR lastPath[MAX_PATH * 4] = {};
    if (GetPrivateProfileString(_T("Base"), _T("LastSettingPath"), _T(""), lastPath, _countof(lastPath), baseIniPath) > 0)
    {
        LoadSettingFromPath(lastPath, false);
    }

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
    bool isQcConfig = false;
    if (!m_settingPath.IsEmpty())
    {
        CString upperPath = m_settingPath;
        upperPath.MakeUpper();
        isQcConfig = (upperPath.Find(_T("QC")) >= 0);
    }

    if (isQcConfig)
    {
        return RunQcTaskImpl(portIndex, this);
    }

    return RunFtTaskImpl(portIndex, this);
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

    if (progress >= 100 && port >= 0 && port < UI_THREAD_COUNT)
    {
        if (!m_portCompleted[port])
        {
            m_portCompleted[port] = true;
            if (result == 0 || result == ERROR_SUCCESS)
            {
                m_passCount++;
            }
            else
            {
                m_failCount++;
            }
            UpdateStatusBarText();
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

    int totalReady = 0;
    CListCtrl* pList = static_cast<CListCtrl*>(GetDlgItem(IDC_LIST_DEVICE));
    if (pList)
    {
        for (int i = 0; i < UI_THREAD_COUNT; ++i)
        {
            CString status = pList->GetItemText(i, 2);
            if (status.CompareNoCase(_T("Ready")) == 0)
            {
                totalReady++;
            }
        }
    }

    // 保留累计计数，只更新本轮计划测试数量并重置端口完成标记
    m_totalCount += totalReady;
    for (int i = 0; i < UI_THREAD_COUNT; ++i)
    {
        m_portCompleted[i] = false;
    }
    UpdateStatusBarText();

    bool isFt3Config = false;
    if (!m_settingPath.IsEmpty())
    {
        CString upperPath = m_settingPath;
        upperPath.MakeUpper();
        isFt3Config = (upperPath.Find(_T("FT3")) >= 0);
    }

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
    if (pList)
    {
        for (int i = 0; i < UI_THREAD_COUNT; ++i)
        {
            CString status = pList->GetItemText(i, 2);
            if (status.CompareNoCase(_T("Ready")) == 0)
            {
                if (isFt3Config)
                {
                    CString snAllocated;
                    if (AcquireAndAdvanceSerialNumber(snAllocated))
                    {
                        pList->SetItemText(i, 6, snAllocated);
                    }
                    else
                    {
                        CString err;
                        err.Format(_T("Read/advance SerialNumber failed for port %d"), i + 1);
                        AppendLogLine(err);
                    }
                }

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

    SparkLog_Close();

    if (g_logLockInited)
    {
        DeleteCriticalSection(&g_logLock);
        g_logLockInited = false;
    }

    m_pdtNameBrush.DeleteObject();
    m_pdtNameFont.DeleteObject();
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

HBRUSH CSparkUfsPdtDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	HBRUSH hbr = CDialogEx::OnCtlColor(pDC, pWnd, nCtlColor);
	if (pWnd && pWnd->GetDlgCtrlID() == IDC_S_PDT_NAME)
	{
		pDC->SetTextColor(RGB(0, 0, 255));
		pDC->SetBkColor(RGB(255, 255, 255));
		return (HBRUSH)m_pdtNameBrush.GetSafeHandle();
	}
	return hbr;
}

void CSparkUfsPdtDlg::UpdatePdtNameText()
{
	CString label = _T("NONE");
	if (!m_settingPath.IsEmpty())
	{
		CString upperPath = m_settingPath;
		upperPath.MakeUpper();
		if (upperPath.Find(_T("FT1")) >= 0)
			label = _T("FT1");
		else if (upperPath.Find(_T("FT2")) >= 0)
			label = _T("FT2");      
		else if (upperPath.Find(_T("FT3")) >= 0)
			label = _T("FT3");
		else if (upperPath.Find(_T("QC")) >= 0)
			label = _T("QC");
	}
	SetDlgItemText(IDC_S_PDT_NAME, label);

	if (CWnd* pWnd = GetDlgItem(IDC_S_PDT_NAME))
	{
		CRect rc;
		pWnd->GetClientRect(&rc);
		int height = rc.Height();
		if (height > 0)
		{
			LOGFONT lf = {};
			lf.lfHeight = -((height * 3) / 5);
			lf.lfWeight = FW_BOLD;
			_tcscpy_s(lf.lfFaceName, LF_FACESIZE, _T("Segoe UI"));
			m_pdtNameFont.DeleteObject();
			m_pdtNameFont.CreateFontIndirect(&lf);
			pWnd->SetFont(&m_pdtNameFont, TRUE);
		}
		pWnd->Invalidate();
	}
}

void CSparkUfsPdtDlg::OnBnClickedBtnPdtSetting()
{
    char currentDirectory[MAX_PATH] = {};
    GetCurrentDirectory(MAX_PATH, currentDirectory);
    CString initialDir;
    initialDir.Format(_T("%hs"), currentDirectory);

    CFileDialog dlg(TRUE, _T("ini"), _T("setting.ini"), OFN_FILEMUSTEXIST | OFN_HIDEREADONLY,
        _T("INI Files (*.ini)|*.ini|All Files (*.*)|*.*||"));
    dlg.m_ofn.lpstrInitialDir = initialDir;

    if (dlg.DoModal() != IDOK)
    {
        return;
    }

    CString path = dlg.GetPathName();
    PUFS_OPTION pOption = CDialogBase::GetSharedUfsOption();
    if (!CDialogSetting::LoadFromIni(path, pOption))
    {
        MessageBox(_T("Load failed."), _T("Setting"), MB_ICONERROR);
        return;
    }
    CString upperPath = path;
    upperPath.MakeUpper();
    bool isQc = (upperPath.Find(_T("QC")) >= 0);

    CDialogSetting settingDlg(this);
    settingDlg.SetUfsOption(pOption);
    settingDlg.SetLastSavePath(path);
    settingDlg.SetVisiblePages(!isQc, isQc);
    settingDlg.DoModal();

    m_settingPath = path;
    UpdatePdtNameText();

    if (CWnd* pStart = GetDlgItem(IDC_BTN_START_PDT))
    {
        pStart->EnableWindow(TRUE);
    }
    if (isQc)
    {
        if (pOption && pOption->qcPrm.szSramTestPath[0])
        {
            CString configuredPath = CString(pOption->qcPrm.szSramTestPath);
            if (!configuredPath.IsEmpty() && GetFileAttributes((LPCSTR)configuredPath.GetString()) != INVALID_FILE_ATTRIBUTES)
            {
                if (spark::file::fnReadFile(configuredPath, (PCHAR)g_UfsIsp) == 0)
                {
                }
                else
                {
                    MessageBox(_T("SRAM Info Read error (from configured path)."), _T("Spark UFS Card PDT"), MB_ICONERROR);
                }
            }
        }
    }
    else
    {
        if (pOption && pOption->mainPrm.strIspPath[0])
        {
            CString configuredPath = CString(pOption->mainPrm.strIspPath);
            if (!configuredPath.IsEmpty() && GetFileAttributes((LPCSTR)configuredPath.GetString()) != INVALID_FILE_ATTRIBUTES)
            {
                if (spark::file::fnReadFile(configuredPath, (PCHAR)g_UfsIsp) == 0)
                {
                }
                else
                {
                    MessageBox(_T("ISP Info Read error (from configured path)."), _T("Spark UFS Card PDT"), MB_ICONERROR);
                }
            }
        }
    }

    SaveLastSettingPath(path);
}

CString CSparkUfsPdtDlg::GetBaseSettingIniPath() const
{
    TCHAR currentDirectory[MAX_PATH] = {};
    GetCurrentDirectory(MAX_PATH, currentDirectory);
    CString baseIniPath;
    baseIniPath.Format(_T("%s\\BoostSetting.ini"), currentDirectory);
    return baseIniPath;
}

void CSparkUfsPdtDlg::SaveLastSettingPath(const CString& path)
{
    if (path.IsEmpty())
    {
        return;
    }

    WritePrivateProfileString(_T("Base"), _T("LastSettingPath"), path, GetBaseSettingIniPath());
}

bool CSparkUfsPdtDlg::LoadSettingFromPath(const CString& path, bool showError)
{
    if (path.IsEmpty() || GetFileAttributes(path) == INVALID_FILE_ATTRIBUTES)
    {
        return false;
    }

    PUFS_OPTION pOption = CDialogBase::GetSharedUfsOption();
    if (!CDialogSetting::LoadFromIni(path, pOption))
    {
        if (showError)
        {
            MessageBox(_T("Load failed."), _T("Setting"), MB_ICONERROR);
        }
        return false;
    }

    CString upperPath = path;
    upperPath.MakeUpper();
    bool isQc = (upperPath.Find(_T("QC")) >= 0);

    m_settingPath = path;
    UpdatePdtNameText();

    if (CWnd* pStart = GetDlgItem(IDC_BTN_START_PDT))
    {
        pStart->EnableWindow(TRUE);
    }

    if (isQc)
    {
        if (pOption && pOption->qcPrm.szSramTestPath[0])
        {
            CString configuredPath = CString(pOption->qcPrm.szSramTestPath);
            if (!configuredPath.IsEmpty() && GetFileAttributes((LPCSTR)configuredPath.GetString()) != INVALID_FILE_ATTRIBUTES)
            {
                if (spark::file::fnReadFile(configuredPath, (PCHAR)g_UfsIsp) != 0 && showError)
                {
                    MessageBox(_T("SRAM Info Read error (from configured path)."), _T("Spark UFS Card PDT"), MB_ICONERROR);
                }
            }
        }
    }
    else
    {
        if (pOption && pOption->mainPrm.strIspPath[0])
        {
            CString configuredPath = CString(pOption->mainPrm.strIspPath);
            if (!configuredPath.IsEmpty() && GetFileAttributes((LPCSTR)configuredPath.GetString()) != INVALID_FILE_ATTRIBUTES)
            {
                if (spark::file::fnReadFile(configuredPath, (PCHAR)g_UfsIsp) != 0 && showError)
                {
                    MessageBox(_T("ISP Info Read error (from configured path)."), _T("Spark UFS Card PDT"), MB_ICONERROR);
                }
            }
        }
    }

    return true;
}

void CSparkUfsPdtDlg::InitStatusBar()
{
    if (!m_statusBar.GetSafeHwnd())
    {
        CRect rc(0, 0, 0, 0);
        m_statusBar.Create(WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP, rc, this, IDC_STATUS_BAR);
    }
    UpdateStatusBarLayout();
    UpdateStatusBarText();
}

void CSparkUfsPdtDlg::UpdateStatusBarLayout()
{
    if (!m_statusBar.GetSafeHwnd())
    {
        return;
    }

    CRect rcClient;
    GetClientRect(&rcClient);
    int height = GetSystemMetrics(SM_CYHSCROLL) + 4;
    m_statusBar.SetWindowPos(nullptr, rcClient.left, rcClient.bottom - height, rcClient.Width(), height, SWP_NOZORDER);

    int part1 = rcClient.Width() / 3;
    int part2 = (rcClient.Width() * 2) / 3;
    int parts[3] = { part1, part2, -1 };
    m_statusBar.SetParts(3, parts);
}

void CSparkUfsPdtDlg::UpdateStatusBarText()
{
    if (!m_statusBar.GetSafeHwnd())
    {
        return;
    }

    CString text;
    text.Format(_T("Test Count: %d"), m_totalCount);
    m_statusBar.SetText(text, 0, 0);
    text.Format(_T("Pass: %d"), m_passCount);
    m_statusBar.SetText(text, 1, 0);
    text.Format(_T("Fail: %d"), m_failCount);
    m_statusBar.SetText(text, 2, 0);
}

void CSparkUfsPdtDlg::ResetTaskCounts(int totalCount)
{
    m_totalCount = totalCount;
    m_passCount = 0;
    m_failCount = 0;
    for (int i = 0; i < UI_THREAD_COUNT; ++i)
    {
        m_portCompleted[i] = false;
    }
    UpdateStatusBarText();
}

void CSparkUfsPdtDlg::OnSize(UINT nType, int cx, int cy)
{
    CDialogEx::OnSize(nType, cx, cy);
    UpdateStatusBarLayout();
}

void CSparkUfsPdtDlg::OnSettingConfig()
{
    CDialogBaseSet dlg(this);
    dlg.DoModal();
}













