#include <cstring>
#include <cstdio>
#include <functional>

#include "pch.h"
#include "SparkUfsPdtDlg.h"
#include "resource.h"
#include "libsparkusb.h"
#include "../SparkLog/SparkLog.h"
#include "CImpState.h"
#include "DialogAdapter.h"

using namespace spark::sm3350;
using TaskProgressMsg = CSparkUfsPdtDlg::TaskProgressMsg;

CRITICAL_SECTION CSparkUfsPdtDlg::g_logLock;
bool CSparkUfsPdtDlg::g_logLockInited = false;

struct SparkLogAutoInit {
    SparkLogAutoInit() { SparkLog_Init(); }
};
static SparkLogAutoInit g_sparkLogAutoInit;

void CSparkUfsPdtDlg::AppendLogLine(const CString& line)
{
    if (!g_logLockInited)
    {
        InitializeCriticalSection(&g_logLock);
        g_logLockInited = true;
    }
    EnterCriticalSection(&g_logLock);
    FILE* fp = NULL;
    errno_t e = fopen_s(&fp, "pdt_run_log.txt", "ab");
    if (e == 0 && fp)
    {
        CT2A lineA(line);
        fwrite(lineA.m_psz, 1, strlen(lineA.m_psz), fp);
        fputc('\n', fp);
        fclose(fp);
    }
    LeaveCriticalSection(&g_logLock);
}

int RunFtTaskImpl(int portIndex, CImpState* state)
{
    DWORD tStart = GetTickCount();
    int ret = ERROR_SUCCESS;

    UCHAR u08PhyIdx = CSparkSm3350Util::GetPhysicalIndex((UCHAR)portIndex);
    CSparkSm3350Util& sm3350 = CSparkSm3350Util::getInstance(u08PhyIdx);

    pdt_log_config_t lg;
    ZeroMemory(&lg, sizeof(lg));
    // Use DialogAdapter to access settings/logger/ui and create a CImpState instance
    if (!state) return ERROR_INVALID_PARAMETER;
    ISettingsProvider* settings = state->GetSettings();
    IUiNotifier* notifier = state->GetNotifier();
    ILogger* logger = state->GetLogger();

    BOOL bBurnInTest = FALSE;
    if (settings && settings->GetUfsOption()) bBurnInTest = settings->GetUfsOption()->mainPrm.bBurnInTest;

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
            if ((ret = state->RebootStage(portIndex, lg)) != ERROR_SUCCESS) break;
            if ((ret = state->ForceRomStage(portIndex, lg)) != ERROR_SUCCESS) break;
            if ((ret = state->MpStartStage(portIndex, lg)) != ERROR_SUCCESS) break;
            if ((ret = state->Write1024KIspMpStage(portIndex, lg)) != ERROR_SUCCESS) break;
            if ((ret = state->MpExitStage(portIndex, lg)) != ERROR_SUCCESS) break;
            if (!bBurnInTest)
            {
                if ((ret = state->CardInitStage(portIndex, lg)) != ERROR_SUCCESS) break;
                if ((ret = state->SetMdtStage(portIndex, lg)) != ERROR_SUCCESS) break;
                if ((ret = state->SetSnStage(portIndex, lg)) != ERROR_SUCCESS) break;
                if ((ret = state->CardInitStage(portIndex, lg)) != ERROR_SUCCESS) break;
                if ((ret = state->VerifySnStage(portIndex, lg)) != ERROR_SUCCESS) break;
                if ((ret = state->VerifyIspStage(portIndex, lg)) != ERROR_SUCCESS) break;
            }
            if ((ret = state->PowerOffStage(portIndex, lg)) != ERROR_SUCCESS) break;
        } while (0);
    }
    else
    {
        ret = selectRet;
        lg.error_code = (UINT32)ret;
        strncpy_s(lg.state, _countof(lg.state), "DeviceSelect Failed", _TRUNCATE);
    }

    DWORD tEnd = GetTickCount();
    DWORD durMs = tEnd - tStart;

    lg.build_time = (int)(durMs / 1000);
    lg.error_code = (UINT32)ret;
    if (lg.state[0] == '\0')
    {
        strncpy_s(lg.state, _countof(lg.state), (ret == ERROR_SUCCESS) ? "Success" : "Failed", _TRUNCATE);
    }

    SparkLog_EnqueuePdtLog(lg);

    // Notify UI via adapter
    if (notifier) notifier->PostTaskProgress(portIndex, 100, ret, (ret == ERROR_SUCCESS) ? _T("Success") : _T("Failed"));

    return ret;
}

int RunQcTaskImpl(int portIndex, CImpState* state)
{
    DWORD tStart = GetTickCount();
    int ret = ERROR_SUCCESS;

    UCHAR u08PhyIdx = CSparkSm3350Util::GetPhysicalIndex((UCHAR)portIndex);
    CSparkSm3350Util& sm3350 = CSparkSm3350Util::getInstance(u08PhyIdx);

    pdt_log_config_t lg;
    ZeroMemory(&lg, sizeof(lg));
    if (!state) return ERROR_INVALID_PARAMETER;
    // For QC path prefer base setting via adapter from the calling dialog state
    PST_UFS_BASE_SETTING pBase = nullptr;
    if (state && state->GetSettings())
    {
        pBase = state->GetSettings()->GetBaseSetting();
    }
    BOOL bForceRomMode = pBase ? pBase->ForceRomMode : FALSE;
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
            if ((ret = state->RebootStage(portIndex, lg)) != ERROR_SUCCESS) break;
            if ((ret = state->ForceRomStage(portIndex, lg)) != ERROR_SUCCESS) break;
            if ((ret = state->CardInitStage(portIndex, lg)) != ERROR_SUCCESS) break;
            if ((ret = state->VerifyCidStage(portIndex, lg)) != ERROR_SUCCESS) break;
            if ((ret = state->VerifyIspStage(portIndex, lg)) != ERROR_SUCCESS) break;
            if ((ret = state->ForceRomStage(portIndex, lg)) != ERROR_SUCCESS) break;
            if ((ret = state->MpStartStage(portIndex, lg)) != ERROR_SUCCESS) break;
            if ((ret = state->WriteSramStage(portIndex, lg)) != ERROR_SUCCESS) break;
            if ((ret = state->MpExitStage(portIndex, lg)) != ERROR_SUCCESS) break;
            if ((ret = state->CardInitStage(portIndex, lg)) != ERROR_SUCCESS) break;
            if ((ret = state->VerifySram1Stage(portIndex, lg)) != ERROR_SUCCESS) break;
            if ((ret = state->CardInitStage(portIndex, lg)) != ERROR_SUCCESS) break;
            if ((ret = state->VerifySram2Stage(portIndex, lg)) != ERROR_SUCCESS) break;
            if ((ret = state->CardInitStage(portIndex, lg)) != ERROR_SUCCESS) break;
            if ((ret = state->VerifyGeometryStage(portIndex, lg)) != ERROR_SUCCESS) break;
            if ((ret = state->PowerOffStage(portIndex, lg)) != ERROR_SUCCESS) break;
        } while (0);
    }
    else
    {
        ret = selectRet;
        lg.error_code = (UINT32)ret;
        strncpy_s(lg.state, _countof(lg.state), "DeviceSelect Failed", _TRUNCATE);
    }

    DWORD tEnd = GetTickCount();
    DWORD durMs = tEnd - tStart;

    lg.build_time = (int)(durMs / 1000);
    lg.error_code = (UINT32)ret;
    if (lg.state[0] == '\0')
    {
        strncpy_s(lg.state, _countof(lg.state), (ret == ERROR_SUCCESS) ? "Success" : "Failed", _TRUNCATE);
    }

    SparkLog_EnqueuePdtLog(lg);

    // Notify UI via adapter
    IUiNotifier* notifierQc = state->GetNotifier();
    if (notifierQc) notifierQc->PostTaskProgress(portIndex, 100, ret, (ret == ERROR_SUCCESS) ? _T("Success") : _T("Failed"));

    return ret;
}
