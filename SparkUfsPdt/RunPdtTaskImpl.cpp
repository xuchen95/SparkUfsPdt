#include <cstring>
#include <cstdio>
#include <functional>

#include "pch.h"
#include "SparkUfsPdtDlg.h"
#include "resource.h"
#include "libsparkusb.h"
#include "../SparkLog/SparkLog.h"
#include "CImpState.h"
#include "StagePipeline.h"
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
        // Build list of stage ids and associated status text. We'll switch on id to call methods.
        enum StageId {
            ST_Reboot,
            ST_ForceRom,
            ST_MpStart,
            ST_Write1024KIspMp,
            ST_MpExit,
            ST_CardInit,
            ST_SetMdt,
            ST_SetSn,
            ST_VerifySn,
            ST_VerifyIsp,
            ST_PowerOff,
            ST_Count
        };

        std::vector<std::pair<int, CString>> stages;
        stages.push_back(std::make_pair(ST_Reboot, _T("Rebooting")));
        stages.push_back(std::make_pair(ST_ForceRom, _T("ForceRom")));
        stages.push_back(std::make_pair(ST_MpStart, _T("MpStart")));
        stages.push_back(std::make_pair(ST_Write1024KIspMp, _T("Write1024KIspMp")));
        stages.push_back(std::make_pair(ST_MpExit, _T("MpExit")));
        if (!bBurnInTest)
        {
            stages.push_back(std::make_pair(ST_CardInit, _T("CardInit")));
            stages.push_back(std::make_pair(ST_SetMdt, _T("SetMdt")));
            stages.push_back(std::make_pair(ST_SetSn, _T("SetSn")));
            stages.push_back(std::make_pair(ST_CardInit, _T("CardInit")));
            stages.push_back(std::make_pair(ST_VerifySn, _T("VerifySn")));
            stages.push_back(std::make_pair(ST_VerifyIsp, _T("VerifyIsp")));
        }
        stages.push_back(std::make_pair(ST_PowerOff, _T("PowerOff")));

        size_t total = stages.size();
        for (size_t i = 0; i < total; ++i)
        {
            int progress = static_cast<int>((i * 100) / total);
            if (notifier) notifier->PostTaskProgress(portIndex, progress, 0, stages[i].second);
            int stageRet = ERROR_SUCCESS;
            switch (stages[i].first)
            {
            case ST_Reboot:
            {
                RebootStage rebootStage([](TaskContext& c) { return c.state->RebootStage(c.portIndex, *c.lg); });
                stageRet = state->RebootStage(portIndex, lg);
                break;
            }
            case ST_ForceRom: stageRet = state->ForceRomStage(portIndex, lg); break;
            case ST_MpStart: stageRet = state->MpStartStage(portIndex, lg); break;
            case ST_Write1024KIspMp: stageRet = state->Write1024KIspMpStage(portIndex, lg); break;
            case ST_MpExit: stageRet = state->MpExitStage(portIndex, lg); break;
            case ST_CardInit: stageRet = state->CardInitStage(portIndex, lg); break;
            case ST_SetMdt: stageRet = state->SetMdtStage(portIndex, lg); break;
            case ST_SetSn: stageRet = state->SetSnStage(portIndex, lg); break;
            case ST_VerifySn: stageRet = state->VerifySnStage(portIndex, lg); break;
            case ST_VerifyIsp: stageRet = state->VerifyIspStage(portIndex, lg); break;
            case ST_PowerOff:
            {
                PowerOffStage powerOffStage([](TaskContext& c) { return c.state->PowerOffStage(c.portIndex, *c.lg); });
                stageRet = state->PowerOffStage(portIndex, lg);
                break;
            }
            default: stageRet = ERROR_INVALID_PARAMETER; break;
            }
            if ((ret = stageRet) != ERROR_SUCCESS)
            {
                break;
            }
        }
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
    int progress;
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
    IUiNotifier* notifierQc = state->GetNotifier();
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
        enum StageIdQc {
            Q_Reboot,
            Q_ForceRom,
            Q_CardInit,
            Q_VerifyCid,
            Q_VerifyIsp,
            Q_MpStart,
            Q_WriteSram,
            Q_MpExit,
            Q_VerifySram1,
            Q_VerifySram2,
            Q_VerifyGeometry,
            Q_PowerOff
        };

        std::vector<std::pair<int, CString>> stages;
        stages.push_back(std::make_pair(Q_Reboot, _T("Rebooting")));
        stages.push_back(std::make_pair(Q_ForceRom, _T("ForceRom")));
        stages.push_back(std::make_pair(Q_CardInit, _T("CardInit")));
        stages.push_back(std::make_pair(Q_VerifyCid, _T("VerifyCid")));
        stages.push_back(std::make_pair(Q_VerifyIsp, _T("VerifyIsp")));
        stages.push_back(std::make_pair(Q_ForceRom, _T("ForceRom")));
        stages.push_back(std::make_pair(Q_MpStart, _T("MpStart")));
        stages.push_back(std::make_pair(Q_WriteSram, _T("WriteSram")));
        stages.push_back(std::make_pair(Q_MpExit, _T("MpExit")));
        stages.push_back(std::make_pair(Q_CardInit, _T("CardInit")));
        stages.push_back(std::make_pair(Q_VerifySram1, _T("VerifySram1")));
        stages.push_back(std::make_pair(Q_CardInit, _T("CardInit")));
        stages.push_back(std::make_pair(Q_VerifySram2, _T("VerifySram2")));
        stages.push_back(std::make_pair(Q_CardInit, _T("CardInit")));
        stages.push_back(std::make_pair(Q_VerifyGeometry, _T("VerifyGeometry")));
        stages.push_back(std::make_pair(Q_PowerOff, _T("PowerOff")));

        size_t total = stages.size();
        for (size_t i = 0; i < total; ++i)
        {
            progress = static_cast<int>((i * 100) / total);
            if (notifierQc) notifierQc->PostTaskProgress(portIndex, progress, 0, stages[i].second);
            int stageRet = ERROR_SUCCESS;
            switch (stages[i].first)
            {
            case Q_Reboot: stageRet = state->RebootStage(portIndex, lg); break;
            case Q_ForceRom: stageRet = state->ForceRomStage(portIndex, lg); break;
            case Q_CardInit: stageRet = state->CardInitStage(portIndex, lg); break;
            case Q_VerifyCid: stageRet = state->VerifyCidStage(portIndex, lg); break;
            case Q_VerifyIsp: stageRet = state->VerifyIspStage(portIndex, lg); break;
            case Q_MpStart: stageRet = state->MpStartStage(portIndex, lg); break;
            case Q_WriteSram: stageRet = state->WriteSramStage(portIndex, lg); break;
            case Q_MpExit: stageRet = state->MpExitStage(portIndex, lg); break;
            case Q_VerifySram1: stageRet = state->VerifySram1Stage(portIndex, lg); break;
            case Q_VerifySram2: stageRet = state->VerifySram2Stage(portIndex, lg); break;
            case Q_VerifyGeometry: stageRet = state->VerifyGeometryStage(portIndex, lg); break;
            case Q_PowerOff: stageRet = state->PowerOffStage(portIndex, lg); break;
            default: stageRet = ERROR_INVALID_PARAMETER; break;
            }
            if ((ret = stageRet) != ERROR_SUCCESS)
            {
                break;
            }
        }
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
    if (notifierQc) notifierQc->PostTaskProgress(portIndex, progress, ret, (ret == ERROR_SUCCESS) ? _T("Success") : _T("Failed"));

    return ret;
}
