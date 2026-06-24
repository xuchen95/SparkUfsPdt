#include "pch.h"
#include <cstring>
#include <cstdio>
#include <functional>
#include <string>
#include <vector>
#include <memory>

#include "SparkUfsPdtDlg.h"
#include "resource.h"
#include "libsparkusb.h"
#include "../SparkLog/SparkLog.h"
#include "CImpState.h"
#include "DialogAdapter.h"
// Use the shared StagePipeline header from the repository include directory
#include "../include/StagePipeline.h"

using namespace spark::sm3350;
using TaskProgressMsg = CSparkUfsPdtDlg::TaskProgressMsg;

CRITICAL_SECTION CSparkUfsPdtDlg::g_logLock;
bool CSparkUfsPdtDlg::g_logLockInited = false;

//struct SparkLogAutoInit {
//    SparkLogAutoInit() { SparkLog_Init(); }
//};
//static SparkLogAutoInit g_sparkLogAutoInit;

// Execute ordered stages using PrefStartContext + GenericState helper.
// Returns error code and sets outLastStageName on failure.
static int ExecuteStagesWithPipeline(int portIndex, CImpState* state, IUiNotifier* notifier, pdt_log_config_t* lg,
    const std::vector<CString>& stageNames,
    const std::vector<std::function<int(TaskContext&)>>& executors,
    CString& outLastStageName)
{
    const size_t total = stageNames.size();
    PrefStartContext pipeline;
    for (size_t i = 0; i < total; ++i)
    {
        CString name = stageNames[i];
        auto exec = executors[i];
        // Capture outLastStageName by reference so we can record the actual stage that failed
        pipeline.AddState(new GenericState(std::string(CT2A(name, CP_UTF8)), [i, total, name, exec, &outLastStageName](TaskContext& ctx)->int {
            int port = ctx.portIndex;
            IUiNotifier* notifierLocal = ctx.notifier;
            CString cName = name;
            int progressLocal = static_cast<int>((i * 100) / total);
            if (notifierLocal) notifierLocal->PostTaskProgress(port, progressLocal, 0, cName);
            int r = exec(ctx);
            if (r == ERROR_SUCCESS)
            {
                int nextProgress = static_cast<int>(((i + 1) * 100) / total);
                if (notifierLocal) notifierLocal->PostTaskProgress(port, nextProgress, 0, cName);
            }
            else
            {
                // record the failing stage name (use friendly StageNameFromFunction if needed by caller)
                outLastStageName = name;
            }
            return r;
        }));
    }

    TaskContext tctx;
    tctx.portIndex = portIndex;
    tctx.state = state;
    tctx.notifier = notifier;
    tctx.lg = lg;
    int ret = pipeline.Exec(tctx);
    return ret;
}

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
    PST_UFS_BASE_SETTING pBase = nullptr;
    PUFS_OPTION pOpt = nullptr;
    if (state && state->GetSettings())
    {
        pBase = state->GetSettings()->GetBaseSetting();
        pOpt = state->GetSettings()->GetUfsOption();
    }
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
    CString strFunName;

    if (bBurnInTest)
    {
		strFunName = _T("FT1");

	}
    else
    {
        strFunName = _T("FT3");
    }
    strncpy_s(lg.func_name, _countof(lg.func_name), CT2A(strFunName, CP_UTF8), _TRUNCATE);

    CTime now = CTime::GetCurrentTime();
    CStringA dateA(now.Format(_T("%Y-%m-%d")));
    CStringA timeA(now.Format(_T("%H:%M:%S")));
    strncpy_s(lg.start_date, _countof(lg.start_date), dateA.GetString(), _TRUNCATE);
    strncpy_s(lg.start_time, _countof(lg.start_time), timeA.GetString(), _TRUNCATE);
    CString lastStageName = _T("");
    int selectRet = sm3350.DeviceSelect(u08PhyIdx);
    if (selectRet == ERROR_SUCCESS)
    {
        // Build ordered stage list (name + executor) and run via shared StagePipeline
        std::vector<CString> stageNames;
        std::vector<std::function<int(TaskContext&)>> executors;

        auto pushStage = [&](const CString& name, std::function<int(TaskContext&)> fn){ stageNames.push_back(name); executors.push_back(fn); };

        // helper to call CImpState methods by binding
        pushStage(_T("Rebooting"), [&](TaskContext& ctx)->int { return ctx.state->RebootStage(ctx.portIndex, *ctx.lg); });
        if (pBase->ForceRomMode == UPIU_FORCE_ROM_MODE)
        {
            pushStage(_T("UpiuForceRom"), [&](TaskContext& ctx)->int { return ctx.state->UpiuForceRomStage(ctx.portIndex, *ctx.lg); });

        }
        else if (pBase->ForceRomMode == VCC_FORCE_ROM_MODE)
        {
            pushStage(_T("VccOffForceRom"), [&](TaskContext& ctx)->int { return ctx.state->VccOffForceRomStage(ctx.portIndex, *ctx.lg); });
        }
        //pushStage(_T("ForceRom"), [&](TaskContext& ctx)->int { return ctx.state->ForceRomStage(ctx.portIndex, *ctx.lg); });
        pushStage(_T("VerifyUID"), [&](TaskContext& ctx)->int { return ctx.state->VerifyUIDStage(ctx.portIndex, *ctx.lg); });
        pushStage(_T("MpStart"), [&](TaskContext& ctx)->int { return ctx.state->MpStartStage(ctx.portIndex, *ctx.lg); });
        pushStage(_T("Write1024KIspMp"), [&](TaskContext& ctx)->int { return ctx.state->Write1024KIspMpStage(ctx.portIndex, *ctx.lg); });
        pushStage(_T("MpExit"), [&](TaskContext& ctx)->int { return ctx.state->MpExitStage(ctx.portIndex, *ctx.lg); });
        if (!bBurnInTest)
        {
            pushStage(_T("CardInit"), [&](TaskContext& ctx)->int { return ctx.state->CardInitStage(ctx.portIndex, *ctx.lg); });
            if (pOpt->mainPrm.bDLCID)
            {
                pushStage(_T("SetMdt"), [&](TaskContext& ctx)->int { return ctx.state->SetMdtStage(ctx.portIndex, *ctx.lg); });
                pushStage(_T("SetSn"), [&](TaskContext& ctx)->int { return ctx.state->SetSnStage(ctx.portIndex, *ctx.lg); });
                pushStage(_T("CardInit"), [&](TaskContext& ctx)->int { return ctx.state->CardInitStage(ctx.portIndex, *ctx.lg); });
                pushStage(_T("VerifySn"), [&](TaskContext& ctx)->int { return ctx.state->VerifySnStage(ctx.portIndex, *ctx.lg); });
            }
            
            pushStage(_T("VerifyIsp"), [&](TaskContext& ctx)->int { return ctx.state->VerifyIspStage(ctx.portIndex, *ctx.lg); });
        }
        pushStage(_T("PowerOff"), [&](TaskContext& ctx)->int { return ctx.state->PowerOffStage(ctx.portIndex, *ctx.lg); });

        ret = ExecuteStagesWithPipeline(portIndex, state, notifier, &lg, stageNames, executors, lastStageName);
    }
    else
    {
        ret = selectRet;
        lg.error_code = (UINT32)ret;
        strncpy_s(lg.stage, _countof(lg.stage), "DeviceSelect Failed", _TRUNCATE);
    }

    DWORD tEnd = GetTickCount();
    DWORD durMs = tEnd - tStart;

    lg.build_time = (int)(durMs / 1000);
    lg.error_code = (UINT32)ret;
    if (lg.stage[0] == '\0')
    {
        strncpy_s(lg.stage, _countof(lg.stage), (ret == ERROR_SUCCESS) ? "Success" : "Failed", _TRUNCATE);
    }

    SparkLog_EnqueuePdtLog(lg);

    // Notify UI via adapter. On failure send only the last stage name and let
    // DialogAdapter format the failure string including the error code.
    if (notifier)
    {
        CString finalStatus;
        if (ret == ERROR_SUCCESS) finalStatus = _T("Success");
        else finalStatus = lastStageName; // may be empty
        // success -> numeric final progress; failure -> final status post
        if (ret == ERROR_SUCCESS)
        {
            notifier->PostTaskProgress(portIndex, 100, ret, finalStatus);
        }
        else
        {
            notifier->PostTaskStatus(portIndex, ret, finalStatus);
        }
    }

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
    PUFS_OPTION pOpt=nullptr;
    if (state && state->GetSettings())
    {
        pBase = state->GetSettings()->GetBaseSetting();
        pOpt = state->GetSettings()->GetUfsOption();
    }
    BOOL bForceRomMode = pBase ? pBase->ForceRomMode : FALSE;
    IUiNotifier* notifierQc = state->GetNotifier();
    lg.ufs_port = (uint8_t)portIndex;
    CString strFunName = _T("QC");
    strncpy_s(lg.func_name, _countof(lg.func_name), CT2A(strFunName, CP_UTF8), _TRUNCATE);

    CTime now = CTime::GetCurrentTime();
    CStringA dateA(now.Format(_T("%Y-%m-%d")));
    CStringA timeA(now.Format(_T("%H:%M:%S")));
    strncpy_s(lg.start_date, _countof(lg.start_date), dateA.GetString(), _TRUNCATE);
    strncpy_s(lg.start_time, _countof(lg.start_time), timeA.GetString(), _TRUNCATE);
    CString lastStageName = _T("");
    int selectRet = sm3350.DeviceSelect(u08PhyIdx);
    if (selectRet == ERROR_SUCCESS)
    {
        // Build QC stage list and run via PrefStartContext
        std::vector<CString> stageNames;
        std::vector<std::function<int(TaskContext&)>> executors;

        auto pushStage = [&](const CString& name, std::function<int(TaskContext&)> fn){ stageNames.push_back(name); executors.push_back(fn); };

        pushStage(_T("Rebooting"), [&](TaskContext& ctx)->int { return ctx.state->RebootStage(ctx.portIndex, *ctx.lg); });
        if (pBase->ForceRomMode == UPIU_FORCE_ROM_MODE)
        {
			pushStage(_T("UpiuForceRom"), [&](TaskContext& ctx)->int { return ctx.state->UpiuForceRomStage(ctx.portIndex, *ctx.lg); });

		}
        else if (pBase->ForceRomMode == VCC_FORCE_ROM_MODE)
        {
            pushStage(_T("VccOffForceRom"), [&](TaskContext& ctx)->int { return ctx.state->VccOffForceRomStage(ctx.portIndex, *ctx.lg); });
        }
        //pushStage(_T("ForceRom"), [&](TaskContext& ctx)->int { return ctx.state->ForceRomStage(ctx.portIndex, *ctx.lg); });
        pushStage(_T("VerifyUID"), [&](TaskContext& ctx)->int { return ctx.state->VerifyUIDStage(ctx.portIndex, *ctx.lg); });
        pushStage(_T("CardInit"), [&](TaskContext& ctx)->int { return ctx.state->CardInitStage(ctx.portIndex, *ctx.lg); });
        pushStage(_T("VerifyCid"), [&](TaskContext& ctx)->int { return ctx.state->VerifyCidStage(ctx.portIndex, *ctx.lg); });
        if (pOpt->qcPrm.bCheckPrv)
        {
            pushStage(_T("VerifyPrv"), [&](TaskContext& ctx)->int { return ctx.state->VerifyPrvStage(ctx.portIndex, *ctx.lg); });
        }
        
        if (pOpt->qcPrm.bCheckIsp)
        {
            pushStage(_T("VerifyIsp"), [&](TaskContext& ctx)->int { return ctx.state->VerifyQcIspStage(ctx.portIndex, *ctx.lg); });
        }
        
        if (pBase->ForceRomMode == UPIU_FORCE_ROM_MODE)
        {
            pushStage(_T("UpiuForceRom"), [&](TaskContext& ctx)->int { return ctx.state->UpiuForceRomStage(ctx.portIndex, *ctx.lg); });

        }
        else if (pBase->ForceRomMode == VCC_FORCE_ROM_MODE)
        {
            pushStage(_T("VccOffForceRom"), [&](TaskContext& ctx)->int { return ctx.state->VccOffForceRomStage(ctx.portIndex, *ctx.lg); });
        }
        //pushStage(_T("ForceRom"), [&](TaskContext& ctx)->int { return ctx.state->ForceRomStage(ctx.portIndex, *ctx.lg); });
        pushStage(_T("MpStart"), [&](TaskContext& ctx)->int { return ctx.state->MpStartStage(ctx.portIndex, *ctx.lg); });
        pushStage(_T("WriteSram"), [&](TaskContext& ctx)->int { return ctx.state->WriteSramStage(ctx.portIndex, *ctx.lg); });
        pushStage(_T("MpExit"), [&](TaskContext& ctx)->int { return ctx.state->MpExitStage(ctx.portIndex, *ctx.lg); });
        pushStage(_T("CardInit"), [&](TaskContext& ctx)->int { return ctx.state->CardInitStage(ctx.portIndex, *ctx.lg); });
        pushStage(_T("VerifySram1"), [&](TaskContext& ctx)->int { return ctx.state->VerifySram1Stage(ctx.portIndex, *ctx.lg); });
        pushStage(_T("CardInit"), [&](TaskContext& ctx)->int { return ctx.state->CardInitStage(ctx.portIndex, *ctx.lg); });
        pushStage(_T("VerifySram2"), [&](TaskContext& ctx)->int { return ctx.state->VerifySram2Stage(ctx.portIndex, *ctx.lg); });
        pushStage(_T("CardInit"), [&](TaskContext& ctx)->int { return ctx.state->CardInitStage(ctx.portIndex, *ctx.lg); });
        pushStage(_T("VerifyGeometry"), [&](TaskContext& ctx)->int { return ctx.state->VerifyGeometryStage(ctx.portIndex, *ctx.lg); });
        pushStage(_T("PowerOff"), [&](TaskContext& ctx)->int { return ctx.state->PowerOffStage(ctx.portIndex, *ctx.lg); });

        ret = ExecuteStagesWithPipeline(portIndex, state, notifierQc, &lg, stageNames, executors, lastStageName);
    }
    else
    {
        ret = selectRet;
        lg.error_code = (UINT32)ret;
        strncpy_s(lg.stage, _countof(lg.stage), "DeviceSelect Failed", _TRUNCATE);
    }

    DWORD tEnd = GetTickCount();
    DWORD durMs = tEnd - tStart;

    lg.build_time = (int)(durMs / 1000);
    lg.error_code = (UINT32)ret;
    if (lg.stage[0] == '\0')
    {
        strncpy_s(lg.stage, _countof(lg.stage), (ret == ERROR_SUCCESS) ? "Success" : "Failed", _TRUNCATE);
    }

    SparkLog_EnqueuePdtLog(lg);

    // Notify UI via adapter. Do not force progress to 100% on failure ¡ª
    // only report 100 when the overall result is success. On failure keep
    // the last reported progress so UI reflects where it failed.
    if (notifierQc)
    {
        CString finalStatus;
        if (ret == ERROR_SUCCESS) finalStatus = _T("Success");
        else finalStatus = (!lastStageName.IsEmpty()) ? (lastStageName + _T(" Failed")) : _T("Failed");
        if (ret == ERROR_SUCCESS)
        {
            notifierQc->PostTaskProgress(portIndex, 100, ret, finalStatus);
        }
        else
        {
            // keep last numeric progress in slot but report final failure status
            notifierQc->PostTaskStatus(portIndex, ret, finalStatus);
        }
    }

    return ret;
}
