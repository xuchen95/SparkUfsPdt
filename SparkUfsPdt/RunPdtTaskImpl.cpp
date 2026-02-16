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

using namespace spark::sm3350;

// define log lock globals (declared in header as static members)
CRITICAL_SECTION CSparkUfsPdtDlg::g_logLock;
bool CSparkUfsPdtDlg::g_logLockInited = false;

// Use the TaskProgressMsg defined in the dialog header for UI communication.
// Worker threads allocate and post pointers to this structure to the UI
// thread (which will delete the pointer after processing).
using TaskProgressMsg = CSparkUfsPdtDlg::TaskProgressMsg;

// Simple record used to store per-stage timing and result data.  Records
// are collected and written to the run log once the whole PDT task ends.
struct StageRecord { CString name; int code; double ms; CString timeStr; };

// Helper: append a line to the log file protected by a critical section.
// This function is thread-safe and can be called from worker threads.
void CSparkUfsPdtDlg::AppendLogLine(const CString& line)
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

// RunStage: execute a single PDT stage (callable), measure duration and
// record the result. The function performs three responsibilities:
//  1) invoke the provided stage callable and time it
//  2) update the in-memory records array for later detailed logging
//  3) post a progress message to the UI thread and append a text log line
// It returns the stage return code (ERROR_SUCCESS on success).
static int RunStage(CSparkUfsPdtDlg* pDlg, int portIndex, LPCTSTR stageName, const std::function<int()>& callExpr,
    StageRecord records[], int maxStages, int &recCount, LARGE_INTEGER freq)
{
    // Post a "stage started" UI update so the list control status column
    // shows the currently executing stage while callExpr runs. Use the
    // current recCount to compute a pre-stage progress value.
    if (pDlg)
    {
        int preProgress = (int)(((recCount) * 100) / maxStages);
        TaskProgressMsg* pstart = new TaskProgressMsg{ portIndex, preProgress, 0, CString(stageName) };
        pDlg->PostMessage(CSparkUfsPdtDlg::WM_TASK_PROGRESS, (WPARAM)pstart, 0);
    }

    LARGE_INTEGER t0, t1; QueryPerformanceCounter(&t0);
    int rc = callExpr();
    QueryPerformanceCounter(&t1);
    double ms = ((double)(t1.QuadPart - t0.QuadPart) * 1000.0) / (double)freq.QuadPart;
    CTime t = CTime::GetCurrentTime();
    CString timeStr = t.Format(_T("%Y-%m-%d %H:%M:%S"));
    if (recCount < maxStages) { records[recCount].name = stageName; records[recCount].code = rc; records[recCount].ms = ms; records[recCount].timeStr = timeStr; recCount++; }

    // Post an intermediate progress message to the UI thread so the
    // dialog can update the progress bar and status cell for this port.
    TaskProgressMsg* pmsg = new TaskProgressMsg{ portIndex, (int)((recCount * 100) / maxStages), rc, CString(stageName) };
    if (pDlg) pDlg->PostMessage(CSparkUfsPdtDlg::WM_TASK_PROGRESS, (WPARAM)pmsg, 0);

    // enqueue an async text line into SparkLog queue (avoid ABI/runtime concerns)
    try {
        CString cs; cs.Format(_T("PDT_LOG | func=%s | port=%d | time=%s | state=%s | err=0x%X"), stageName, portIndex, timeStr, stageName, rc);
        CSparkUfsPdtDlg::AppendLogLine(cs);
    } catch (...) { }

    return rc;
}

// RunPdtTaskImpl: main PDT workflow for a specific port index. This
// function executes a series of hardware operations (power, init, ISP
// write, etc.) and records timing / results for each stage. Progress
// and final result are reported back to the UI thread via messages.
int RunFtTaskImpl(int portIndex, CSparkUfsPdtDlg* pDlg)
{
    auto tStart = std::chrono::steady_clock::now();
    int ret = 0;
    CHAR pData[512];
    CHAR pPortInfo[1024];
    CHAR pMdtInfo[512*8];
    const int MAX_STAGES = 16;
    StageRecord records[MAX_STAGES];
    int recCount = 0;
    PUFS_OPTION pOpt = CDialogBase::GetSharedUfsOption();
    //0:ERASE_ALL_BLOCK  1:ERASE_GOOD_BLOCK
    BOOL bFuncOption = pOpt->mainPrm.funcSel;
    UCHAR u08PhyIdx = CSparkSm3350Util::GetPhysicalIndex((UCHAR)portIndex);
    PST_DEVICE_INFO pDeviceInfo = CSparkSm3350Util::GetDeviceInfo((UCHAR)u08PhyIdx);
    CSparkSm3350Util& sm3350 = CSparkSm3350Util::getInstance(u08PhyIdx);
    LARGE_INTEGER freq;
    if (!QueryPerformanceFrequency(&freq)) freq.QuadPart = 1000;

    

    if (ERROR_SUCCESS == sm3350.DeviceSelect(u08PhyIdx))
    {
        ret = RunStage(pDlg, portIndex, _T("UfsPowerOff"), [&](){ return sm3350.UfsPowerOff(pData); }, records, MAX_STAGES, recCount, freq);
        if (ret == ERROR_SUCCESS) ret = RunStage(pDlg, portIndex, _T("UfsPowerOn"), [&](){ return sm3350.UfsPowerOn(pData); }, records, MAX_STAGES, recCount, freq);
        if (ret == ERROR_SUCCESS) ret = RunStage(pDlg, portIndex, _T("UfsReadPortInfo"), [&](){ return sm3350.UfsReadPortInfo(pPortInfo); }, records, MAX_STAGES, recCount, freq);
        if (ret == ERROR_SUCCESS) ret = RunStage(pDlg, portIndex, _T("UfsCardInit"), [&](){ return sm3350.UfsCardInit(pData); }, records, MAX_STAGES, recCount, freq);
        if (ret == ERROR_SUCCESS) ret = RunStage(pDlg, portIndex, _T("VccOffForceRom"), [&](){ return sm3350.VccOffForceRom(pData); }, records, MAX_STAGES, recCount, freq);
        if (ret == ERROR_SUCCESS) ret = RunStage(pDlg, portIndex, _T("UfsMpStartMode"), [&](){ return sm3350.UfsMpStartMode(pData); }, records, MAX_STAGES, recCount, freq);
        if (ret == ERROR_SUCCESS) ret = RunStage(pDlg, portIndex, _T("UfsWrite1024KIspMp"), [&](){ return sm3350.UfsWrite1024KIspMp(g_UfsIsp, BYTE2SECTOR(sizeof(g_UfsIsp)), bFuncOption); }, records, MAX_STAGES, recCount, freq);
        if (ret == ERROR_SUCCESS) ret = RunStage(pDlg, portIndex, _T("UfsMpExit"), [&](){ return sm3350.UfsMpExit(pData); }, records, MAX_STAGES, recCount, freq);
    }

    auto tEnd = std::chrono::steady_clock::now();
    auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(tEnd - tStart).count();

    // write log
    CTime now = CTime::GetCurrentTime();
    CString header;
    header.Format(_T("%s | Port %d | TotalMs=%lld | Result=0x%X"), now.Format(_T("%Y-%m-%d %H:%M:%S")), portIndex+1, (long long)dur, ret);
    CSparkUfsPdtDlg::AppendLogLine(header);
    for (int i = 0; i < recCount; ++i)
    {
        CString line;
        line.Format(_T("%s | Port %d | Stage=%s | code=0x%X | %.3f ms"), records[i].timeStr, portIndex+1, records[i].name.GetString(), records[i].code, records[i].ms);
        CSparkUfsPdtDlg::AppendLogLine(line);
    }

    // final UI notify
    TaskProgressMsg* finalMsg = new TaskProgressMsg{portIndex, 100, ret, (ret==ERROR_SUCCESS)?_T("Success"):_T("Failed")};
    if (pDlg) pDlg->PostMessage(CSparkUfsPdtDlg::WM_TASK_PROGRESS, (WPARAM)finalMsg, 0);

    return ret;
}

int RunQcTaskImpl(int portIndex, CSparkUfsPdtDlg* pDlg)
{
    auto tStart = std::chrono::steady_clock::now();
    int ret = 0;
    CHAR pData[512];
    CHAR pPortInfo[1024];
    const int MAX_STAGES = 16;
    StageRecord records[MAX_STAGES];
    int recCount = 0;
    PUFS_OPTION pOpt = CDialogBase::GetSharedUfsOption();
    //0:ERASE_ALL_BLOCK  1:ERASE_GOOD_BLOCK
    BOOL bFuncOption = pOpt->mainPrm.funcSel;
    UCHAR u08PhyIdx = CSparkSm3350Util::GetPhysicalIndex((UCHAR)portIndex);
    PST_DEVICE_INFO pDeviceInfo = CSparkSm3350Util::GetDeviceInfo((UCHAR)u08PhyIdx);
    CSparkSm3350Util& sm3350 = CSparkSm3350Util::getInstance(u08PhyIdx);
    LARGE_INTEGER freq;
    if (!QueryPerformanceFrequency(&freq)) freq.QuadPart = 1000;



    if (ERROR_SUCCESS == sm3350.DeviceSelect(u08PhyIdx))
    {
        ret = RunStage(pDlg, portIndex, _T("UfsPowerOff"), [&]() { return sm3350.UfsPowerOff(pData); }, records, MAX_STAGES, recCount, freq);
        if (ret == ERROR_SUCCESS) ret = RunStage(pDlg, portIndex, _T("UfsPowerOn"), [&]() { return sm3350.UfsPowerOn(pData); }, records, MAX_STAGES, recCount, freq);
        if (ret == ERROR_SUCCESS) ret = RunStage(pDlg, portIndex, _T("UfsReadPortInfo"), [&]() { return sm3350.UfsReadPortInfo(pPortInfo); }, records, MAX_STAGES, recCount, freq);
        if (ret == ERROR_SUCCESS) ret = RunStage(pDlg, portIndex, _T("UfsCardInit"), [&]() { return sm3350.UfsCardInit(pData); }, records, MAX_STAGES, recCount, freq);
        if (ret == ERROR_SUCCESS) ret = RunStage(pDlg, portIndex, _T("VccOffForceRom"), [&]() { return sm3350.VccOffForceRom(pData); }, records, MAX_STAGES, recCount, freq);
        if (ret == ERROR_SUCCESS) ret = RunStage(pDlg, portIndex, _T("UfsMpStartMode"), [&]() { return sm3350.UfsMpStartMode(pData); }, records, MAX_STAGES, recCount, freq);
        if (ret == ERROR_SUCCESS) ret = RunStage(pDlg, portIndex, _T("UfsWrite1024KIspMp"), [&]() { return sm3350.UfsWrite1024KIspMp(g_UfsIsp, BYTE2SECTOR(sizeof(g_UfsIsp)), bFuncOption); }, records, MAX_STAGES, recCount, freq);
        if (ret == ERROR_SUCCESS) ret = RunStage(pDlg, portIndex, _T("UfsMpExit"), [&]() { return sm3350.UfsMpExit(pData); }, records, MAX_STAGES, recCount, freq);
    }

    auto tEnd = std::chrono::steady_clock::now();
    auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(tEnd - tStart).count();

    // write log
    CTime now = CTime::GetCurrentTime();
    CString header;
    header.Format(_T("%s | Port %d | TotalMs=%lld | Result=0x%X"), now.Format(_T("%Y-%m-%d %H:%M:%S")), portIndex + 1, (long long)dur, ret);
    CSparkUfsPdtDlg::AppendLogLine(header);
    for (int i = 0; i < recCount; ++i)
    {
        CString line;
        line.Format(_T("%s | Port %d | Stage=%s | code=0x%X | %.3f ms"), records[i].timeStr, portIndex + 1, records[i].name.GetString(), records[i].code, records[i].ms);
        CSparkUfsPdtDlg::AppendLogLine(line);
    }

    // final UI notify
    TaskProgressMsg* finalMsg = new TaskProgressMsg{ portIndex, 100, ret, (ret == ERROR_SUCCESS) ? _T("Success") : _T("Failed") };
    if (pDlg) pDlg->PostMessage(CSparkUfsPdtDlg::WM_TASK_PROGRESS, (WPARAM)finalMsg, 0);

    return ret;
}
