#include <chrono>
#include <cstring>
#include <cstdio>
#include <functional>

// RunPdtTaskImpl.cpp - moved PDT task implementation
// This file contains the main PDT (Production Device Test) worker logic.
// The heavy lifting was moved out of the UI dialog to keep UI code small
// and to allow worker threads to execute the PDT procedures by calling
// `RunPdtTaskImpl(portIndex, pDlg)`.
#include "pch.h"
#include "SparkUfsPdtDlg.h"
#include "libsparkusb.h"
#include <chrono>
#include <cstring>
#include <cstdio>
#include <functional>
#include "../SparkLog/SparkLog.h"

using namespace spark::sm3350;

// define log lock globals (declared in header as static members)
CRITICAL_SECTION CSparkUfsPdtDlg::g_logLock;
bool CSparkUfsPdtDlg::g_logLockInited = false;

// 日志功能在模块加载时初始化
struct SparkLogAutoInit {
    SparkLogAutoInit() { SparkLog_Init(); }
};
static SparkLogAutoInit g_sparkLogAutoInit;

// Use the TaskProgressMsg defined in the dialog header for UI communication.
// Worker threads allocate and post pointers to this structure to the UI
// thread (which will delete the pointer after processing).
using TaskProgressMsg = CSparkUfsPdtDlg::TaskProgressMsg;

// Simple record used to store per-stage timing and result data.  Records
// are collected and written to the run log once the whole PDT task ends.
struct StageRecord { CString name; int code; double ms; CString timeStr; };

// Helper: append a line to the log file protected by a critical section.
// This function is thread-safe and can be called from worker threads.


CHAR g_szPrmData[512*0x08];

static int RebootStage(CSparkUfsPdtDlg* pDlg, int portIndex, pdt_log_config_t& lg)
{
    int ret = ERROR_SUCCESS;
    TaskProgressMsg* pmsg = new TaskProgressMsg{ portIndex, 0, 0, _T("Rebooting") };
    if (pDlg) pDlg->PostMessage(CSparkUfsPdtDlg::WM_TASK_PROGRESS, (WPARAM)pmsg, 0);

    UCHAR u08PhyIdx = CSparkSm3350Util::GetPhysicalIndex((UCHAR)portIndex);
    CSparkSm3350Util& sm3350 = CSparkSm3350Util::getInstance(u08PhyIdx);
    do
    {
        if ((ret = sm3350.UfsPowerOff()) != ERROR_SUCCESS) break;
        Sleep(1000);
        if ((ret = sm3350.UfsPowerOn()) != ERROR_SUCCESS) break;
    } while (0);

    if (ret != ERROR_SUCCESS)
    {
        TaskProgressMsg* pErr = new TaskProgressMsg{ portIndex, 0, ret, _T("Reboot Failed") };
        if (pDlg) pDlg->PostMessage(CSparkUfsPdtDlg::WM_TASK_PROGRESS, (WPARAM)pErr, 0);
        lg.error_code = ret;
        ZeroMemory(lg.state, sizeof(lg.state));
        strncpy_s(lg.state, _countof(lg.state), "Reboot Failed", _TRUNCATE);
    }
    return ret;
}

static int CardInitStage(CSparkUfsPdtDlg* pDlg, int portIndex, pdt_log_config_t& lg)
{
    int ret = ERROR_SUCCESS;
    TaskProgressMsg* pmsg = new TaskProgressMsg{ portIndex, 0, 0, _T("CardInit") };
    if (pDlg) pDlg->PostMessage(CSparkUfsPdtDlg::WM_TASK_PROGRESS, (WPARAM)pmsg, 0);

    UCHAR u08PhyIdx = CSparkSm3350Util::GetPhysicalIndex((UCHAR)portIndex);
    CSparkSm3350Util& sm3350 = CSparkSm3350Util::getInstance(u08PhyIdx);

    ret = sm3350.UfsCardInit();
    if (ret != ERROR_SUCCESS)
    {
        TaskProgressMsg* pErr = new TaskProgressMsg{ portIndex, 0, ret, _T("CardInit Failed") };
        if (pDlg) pDlg->PostMessage(CSparkUfsPdtDlg::WM_TASK_PROGRESS, (WPARAM)pErr, 0);
        lg.error_code = ret;
        ZeroMemory(lg.state, sizeof(lg.state));
        strncpy_s(lg.state, _countof(lg.state), "CardInit Failed", _TRUNCATE);
    }
    return ret;
}

static int VccOffForceRomStage(CSparkUfsPdtDlg* pDlg, int portIndex, pdt_log_config_t& lg)
{
    int ret = ERROR_SUCCESS;
    TaskProgressMsg* pmsg = new TaskProgressMsg{ portIndex, 0, 0, _T("VccOffForceRom") };
    if (pDlg) pDlg->PostMessage(CSparkUfsPdtDlg::WM_TASK_PROGRESS, (WPARAM)pmsg, 0);

    UCHAR u08PhyIdx = CSparkSm3350Util::GetPhysicalIndex((UCHAR)portIndex);
    CSparkSm3350Util& sm3350 = CSparkSm3350Util::getInstance(u08PhyIdx);

    ret = sm3350.VccOffForceRom();
    if (ret != ERROR_SUCCESS)
    {
        TaskProgressMsg* pErr = new TaskProgressMsg{ portIndex, 0, ret, _T("VccOffForceRom Failed") };
        if (pDlg) pDlg->PostMessage(CSparkUfsPdtDlg::WM_TASK_PROGRESS, (WPARAM)pErr, 0);
        lg.error_code = ret;
        ZeroMemory(lg.state, sizeof(lg.state));
        strncpy_s(lg.state, _countof(lg.state), "VccOffForceRom Failed", _TRUNCATE);
    }
    return ret;
}

static int MpStartStage(CSparkUfsPdtDlg* pDlg, int portIndex, pdt_log_config_t& lg)
{
    int ret = ERROR_SUCCESS;
    TaskProgressMsg* pmsg = new TaskProgressMsg{ portIndex, 0, 0, _T("MpStart") };
    if (pDlg) pDlg->PostMessage(CSparkUfsPdtDlg::WM_TASK_PROGRESS, (WPARAM)pmsg, 0);

    UCHAR u08PhyIdx = CSparkSm3350Util::GetPhysicalIndex((UCHAR)portIndex);
    CSparkSm3350Util& sm3350 = CSparkSm3350Util::getInstance(u08PhyIdx);

    ret = sm3350.UfsMpStartMode();
    if (ret != ERROR_SUCCESS)
    {
        TaskProgressMsg* pErr = new TaskProgressMsg{ portIndex, 0, ret, _T("MpStart Failed") };
        if (pDlg) pDlg->PostMessage(CSparkUfsPdtDlg::WM_TASK_PROGRESS, (WPARAM)pErr, 0);
        lg.error_code = ret;
        ZeroMemory(lg.state, sizeof(lg.state));
        strncpy_s(lg.state, _countof(lg.state), "MpStart Failed", _TRUNCATE);
    }
    return ret;
}

static int Write1024KIspMpStage(CSparkUfsPdtDlg* pDlg, int portIndex, pdt_log_config_t& lg)
{
    int ret = ERROR_SUCCESS;
    PUFS_OPTION pOpt = CDialogBase::GetSharedUfsOption();
    BOOL bFuncOption = pOpt->mainPrm.funcSel;

    TaskProgressMsg* pmsg = new TaskProgressMsg{ portIndex, 0, 0, _T("Write1024KIspMp") };
    if (pDlg) pDlg->PostMessage(CSparkUfsPdtDlg::WM_TASK_PROGRESS, (WPARAM)pmsg, 0);

    UCHAR u08PhyIdx = CSparkSm3350Util::GetPhysicalIndex((UCHAR)portIndex);
    CSparkSm3350Util& sm3350 = CSparkSm3350Util::getInstance(u08PhyIdx);

    ret = sm3350.UfsWrite1024KIspMp(g_UfsIsp, BYTE2SECTOR(sizeof(g_UfsIsp)), bFuncOption);
    if (ret != ERROR_SUCCESS)
    {
        TaskProgressMsg* pErr = new TaskProgressMsg{ portIndex, 0, ret, _T("Write1024KIspMp Failed") };
        if (pDlg) pDlg->PostMessage(CSparkUfsPdtDlg::WM_TASK_PROGRESS, (WPARAM)pErr, 0);
        lg.error_code = ret;
        ZeroMemory(lg.state, sizeof(lg.state));
        strncpy_s(lg.state, _countof(lg.state), "Write1024KIspMp Failed", _TRUNCATE);
    }
    return ret;
}

static int MpExitStage(CSparkUfsPdtDlg* pDlg, int portIndex, pdt_log_config_t& lg)
{
    int ret = ERROR_SUCCESS;
    TaskProgressMsg* pmsg = new TaskProgressMsg{ portIndex, 0, 0, _T("MpExit") };
    if (pDlg) pDlg->PostMessage(CSparkUfsPdtDlg::WM_TASK_PROGRESS, (WPARAM)pmsg, 0);

    UCHAR u08PhyIdx = CSparkSm3350Util::GetPhysicalIndex((UCHAR)portIndex);
    CSparkSm3350Util& sm3350 = CSparkSm3350Util::getInstance(u08PhyIdx);

    ret = sm3350.UfsMpExit();
    if (ret != ERROR_SUCCESS)
    {
        TaskProgressMsg* pErr = new TaskProgressMsg{ portIndex, 0, ret, _T("MpExit Failed") };
        if (pDlg) pDlg->PostMessage(CSparkUfsPdtDlg::WM_TASK_PROGRESS, (WPARAM)pErr, 0);
        lg.error_code = ret;
        ZeroMemory(lg.state, sizeof(lg.state));
        strncpy_s(lg.state, _countof(lg.state), "MpExit Failed", _TRUNCATE);
    }
    return ret;
}

static int SetSnStage(CSparkUfsPdtDlg* pDlg, int portIndex, pdt_log_config_t& lg)
{
    int ret = ERROR_SUCCESS;
    TaskProgressMsg* pmsg = new TaskProgressMsg{ portIndex, 0, 0, _T("SetSn") };
    if (pDlg) pDlg->PostMessage(CSparkUfsPdtDlg::WM_TASK_PROGRESS, (WPARAM)pmsg, 0);

    UCHAR u08PhyIdx = CSparkSm3350Util::GetPhysicalIndex((UCHAR)portIndex);
    CSparkSm3350Util& sm3350 = CSparkSm3350Util::getInstance(u08PhyIdx);

    do
    {
        if ((ret = sm3350.UfsSetSrialNumberString(g_szPrmData)) != ERROR_SUCCESS) break;
    } while (0);
    if (ret != ERROR_SUCCESS)
    {
        TaskProgressMsg* pErr = new TaskProgressMsg{ portIndex, 0, ret, _T("SetSn Failed") };
        if (pDlg) pDlg->PostMessage(CSparkUfsPdtDlg::WM_TASK_PROGRESS, (WPARAM)pErr, 0);
        lg.error_code = ret;
        ZeroMemory(lg.state, sizeof(lg.state));
        strncpy_s(lg.state, _countof(lg.state), "SetSn Failed", _TRUNCATE);
    }
    return ret;
}

static int SetMdtStage(CSparkUfsPdtDlg* pDlg, int portIndex, pdt_log_config_t& lg)
{
    int ret = ERROR_SUCCESS;
    TaskProgressMsg* pmsg = new TaskProgressMsg{ portIndex, 0, 0, _T("SetMdt") };
    if (pDlg) pDlg->PostMessage(CSparkUfsPdtDlg::WM_TASK_PROGRESS, (WPARAM)pmsg, 0);

    UCHAR u08PhyIdx = CSparkSm3350Util::GetPhysicalIndex((UCHAR)portIndex);
    CSparkSm3350Util& sm3350 = CSparkSm3350Util::getInstance(u08PhyIdx);

    do
    {
        if ((ret = sm3350.UfsSetManuDate(g_szPrmData)) != ERROR_SUCCESS) break;
    } while (0);
    if (ret != ERROR_SUCCESS)
    {
        TaskProgressMsg* pErr = new TaskProgressMsg{ portIndex, 0, ret, _T("SetMdt Failed") };
        if (pDlg) pDlg->PostMessage(CSparkUfsPdtDlg::WM_TASK_PROGRESS, (WPARAM)pErr, 0);
        lg.error_code = ret;
        ZeroMemory(lg.state, sizeof(lg.state));
        strncpy_s(lg.state, _countof(lg.state), "SetMdt Failed", _TRUNCATE);
    }
    return ret;
}

int RunFtTaskImpl(int portIndex, CSparkUfsPdtDlg* pDlg)
{
    auto tStart = std::chrono::steady_clock::now();
    int ret = ERROR_SUCCESS;

    UCHAR u08PhyIdx = CSparkSm3350Util::GetPhysicalIndex((UCHAR)portIndex);
    CSparkSm3350Util& sm3350 = CSparkSm3350Util::getInstance(u08PhyIdx);

    pdt_log_config_t lg;
    ZeroMemory(&lg, sizeof(lg));

    lg.ufs_port = (uint8_t)portIndex;
    strncpy_s(lg.func_name, _countof(lg.func_name), "RunFtTaskImpl", _TRUNCATE);

    CTime now = CTime::GetCurrentTime();
    CStringA dateA(now.Format(_T("%Y-%m-%d")));
    CStringA timeA(now.Format(_T("%H:%M:%S")));
    strncpy_s(lg.start_date, _countof(lg.start_date), dateA.GetString(), _TRUNCATE);
    strncpy_s(lg.start_time, _countof(lg.start_time), timeA.GetString(), _TRUNCATE);
    int selectRet = sm3350.DeviceSelect(u08PhyIdx);
    if (selectRet == ERROR_SUCCESS)
    {
        do
        {
            if ((ret = RebootStage(pDlg, portIndex, lg)) != ERROR_SUCCESS) break;
            if ((ret = CardInitStage(pDlg, portIndex, lg)) != ERROR_SUCCESS) break;
            if ((ret = VccOffForceRomStage(pDlg, portIndex, lg)) != ERROR_SUCCESS) break;
            if ((ret = MpStartStage(pDlg, portIndex, lg)) != ERROR_SUCCESS) break;
            if ((ret = Write1024KIspMpStage(pDlg, portIndex, lg)) != ERROR_SUCCESS) break;
            if ((ret = MpExitStage(pDlg, portIndex, lg)) != ERROR_SUCCESS) break;
        } while (0);
    }
    else
    {
        ret = selectRet;
        lg.error_code = (UINT32)ret;
        strncpy_s(lg.state, _countof(lg.state), "DeviceSelect Failed", _TRUNCATE);
    }
    
    auto tEnd = std::chrono::steady_clock::now();
    auto durMs = std::chrono::duration_cast<std::chrono::milliseconds>(tEnd - tStart).count();

    lg.build_time = (int)(durMs / 1000);
    lg.error_code = (UINT32)ret;
    if (lg.state[0] == '\0')
    {
        strncpy_s(lg.state, _countof(lg.state), (ret == ERROR_SUCCESS) ? "Success" : "Failed", _TRUNCATE);
    }

    SparkLog_EnqueuePdtLog(lg);

    TaskProgressMsg* finalMsg = new TaskProgressMsg{ portIndex, 100, ret, (ret == ERROR_SUCCESS) ? _T("Success") : _T("Failed") };
    if (pDlg) pDlg->PostMessage(CSparkUfsPdtDlg::WM_TASK_PROGRESS, (WPARAM)finalMsg, 0);

    return ret;
}

int RunQcTaskImpl(int portIndex, CSparkUfsPdtDlg* pDlg)
{
    

    return 0;
}
