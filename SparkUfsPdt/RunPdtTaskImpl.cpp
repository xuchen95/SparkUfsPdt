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
#include "resource.h"
#include "libsparkusb.h"
#include <chrono>
#include <cstring>
#include <cstdio>
#include <functional>
#include "../SparkLog/SparkLog.h"

using namespace spark::sm3350;

static void SetMdtData(CSparkUfsPdtDlg* pDlg, char* pData);
static void SetSnData(CSparkUfsPdtDlg* pDlg, int portIndex, char* pData);
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

bool ConvertWCharDataToCharData(const WCHAR* wSrc, size_t wSrcLen,
    char* cDest, size_t cDestLen,
    UINT codePage = CP_ACP)
{
    // 1. 安全校验：指针非空 + 数组长度有效
    if (wSrc == nullptr || cDest == nullptr || wSrcLen == 0 || cDestLen == 0)
    {
        return false;
    }

    // 2. 计算临时缓冲区大小：1个WCHAR最多转3个char（UTF-8），预留足够空间
    size_t tempBufSize = wSrcLen * 3;
    char* tempBuf = new char[tempBufSize];
    memset(tempBuf, 0, tempBufSize); // 初始化临时缓冲区

    // 3. 执行宽字符转多字节（按元素个数转换，非字符串）
    int convertLen = WideCharToMultiByte(
        codePage,        // 目标编码页
        0,               // 转换标志：0=默认（不添加\0）
        wSrc,            // 输入WCHAR数组
        static_cast<int>(wSrcLen), // 要转换的WCHAR元素个数（关键：不用-1）
        tempBuf,         // 临时存储转换结果
        static_cast<int>(tempBufSize), // 临时缓冲区大小
        nullptr,         // 无效字符替换符（NULL=系统默认）
        nullptr          // 是否使用了替换符
    );

    // 转换失败处理
    if (convertLen == 0)
    {
        delete[] tempBuf;
        memset(cDest, 0, cDestLen); // 失败时清空目标数组
        return false;
    }

    // 4. 填充目标char数组（按长度规则处理）
    if (static_cast<size_t>(convertLen) >= cDestLen)
    {
        // 转换结果≥目标长度：取前cDestLen个元素（截断）
        memcpy(cDest, tempBuf, cDestLen);
    }
    else
    {
        // 转换结果<目标长度：复制有效数据，剩余位置补0
        memcpy(cDest, tempBuf, convertLen);
        memset(cDest + convertLen, 0, cDestLen - convertLen);
    }

    // 释放临时缓冲区
    delete[] tempBuf;
    return true;
}

static int PowerOffStage(CSparkUfsPdtDlg* pDlg, int portIndex, pdt_log_config_t& lg)
{
    int ret = ERROR_SUCCESS;
    TaskProgressMsg* pmsg = new TaskProgressMsg{ portIndex, 0, 0, _T("PowerOff") };
    if (pDlg) pDlg->PostMessage(CSparkUfsPdtDlg::WM_TASK_PROGRESS, (WPARAM)pmsg, 0);

    UCHAR u08PhyIdx = CSparkSm3350Util::GetPhysicalIndex((UCHAR)portIndex);
    CSparkSm3350Util& sm3350 = CSparkSm3350Util::getInstance(u08PhyIdx);
    //char pData[512 * 8] = { 0 };
    //SetMdtData(pDlg, pData);
    do
    {
        if ((ret = sm3350.UfsPowerOff()) != ERROR_SUCCESS) break;
        Sleep(100);
    } while (0);

    if (ret != ERROR_SUCCESS)
    {
        TaskProgressMsg* pErr = new TaskProgressMsg{ portIndex, 0, ret, _T("PowerOff Failed") };
        if (pDlg) pDlg->PostMessage(CSparkUfsPdtDlg::WM_TASK_PROGRESS, (WPARAM)pErr, 0);
        lg.error_code = ret;
        ZeroMemory(lg.state, sizeof(lg.state));
        strncpy_s(lg.state, _countof(lg.state), "PowerOff Failed", _TRUNCATE);
    }
    return ret;
}

static int RebootStage(CSparkUfsPdtDlg* pDlg, int portIndex, pdt_log_config_t& lg)
{
    int ret = ERROR_SUCCESS;
    TaskProgressMsg* pmsg = new TaskProgressMsg{ portIndex, 0, 0, _T("Rebooting") };
    if (pDlg) pDlg->PostMessage(CSparkUfsPdtDlg::WM_TASK_PROGRESS, (WPARAM)pmsg, 0);

    UCHAR u08PhyIdx = CSparkSm3350Util::GetPhysicalIndex((UCHAR)portIndex);
    CSparkSm3350Util& sm3350 = CSparkSm3350Util::getInstance(u08PhyIdx);
    //char pData[512 * 8] = { 0 };
    //SetMdtData(pDlg, pData);
    //SetSnData(pDlg, portIndex, pData);
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

static int UpiuForceRomStage(CSparkUfsPdtDlg* pDlg, int portIndex, pdt_log_config_t& lg)
{
    int ret = ERROR_SUCCESS;
    TaskProgressMsg* pmsg = new TaskProgressMsg{ portIndex, 0, 0, _T("UpiuForceRom") };
    if (pDlg) pDlg->PostMessage(CSparkUfsPdtDlg::WM_TASK_PROGRESS, (WPARAM)pmsg, 0);

    UCHAR u08PhyIdx = CSparkSm3350Util::GetPhysicalIndex((UCHAR)portIndex);
    CSparkSm3350Util& sm3350 = CSparkSm3350Util::getInstance(u08PhyIdx);

    ret = sm3350.UpiuForceRom();
    if (ret != ERROR_SUCCESS)
    {
        TaskProgressMsg* pErr = new TaskProgressMsg{ portIndex, 0, ret, _T("UpiuForceRom Failed") };
        if (pDlg) pDlg->PostMessage(CSparkUfsPdtDlg::WM_TASK_PROGRESS, (WPARAM)pErr, 0);
        lg.error_code = ret;
        ZeroMemory(lg.state, sizeof(lg.state));
        strncpy_s(lg.state, _countof(lg.state), "UpiuForceRom Failed", _TRUNCATE);
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

static void SetSnData(CSparkUfsPdtDlg* pDlg, int portIndex, char* pData)
{
    if (pDlg == nullptr || pData == nullptr)
    {
        return;
    }

    constexpr size_t kSnPayloadBytes = 0x40;
    ZeroMemory(pData, kSnPayloadBytes);
    int offset =0;
    pData[0] = 0x40;  //length
    pData[1] = 0x05;  //IDN
    offset += 2;

    BYTE byMeto[4];
    CStringW strHex(pDlg->GetUfsOption()->mainPrm.meto);
    for (int i = 0; i < 8; i += 2)
    {
        // 截取 2 个字符
        CStringW twoChars = strHex.Mid(i, 2);

        // 关键：把这两个字符当作 16进制 转成 int
        // 这就是你要的【赋值给 int】
        BYTE hexValue = (BYTE)wcstoul(twoChars.GetBuffer(), NULL, 16);

        // 存入 int 数组
        byMeto[i / 2] = hexValue;
    }

    memcpy(pData + offset, byMeto, 4);
    offset += 4;
    //Get current time
    SYSTEMTIME st;
    GetLocalTime(&st);

    //Format time to YYYYMMDD
    wchar_t timeStr[9];
    
    swprintf_s(timeStr, L"%04d%02d%02d", st.wYear, st.wMonth, st.wDay);
    
    memcpy(pData + offset, timeStr, 16);
    offset += 16;

    
    wchar_t psn[9] = { 0 };
    CString psnText;
    CListCtrl* pList = static_cast<CListCtrl*>(pDlg->GetDlgItem(IDC_LIST_DEVICE));
    if (pList != nullptr && portIndex >= 0 && portIndex < pList->GetItemCount())
    {
        psnText = pList->GetItemText(portIndex, 6);
        if (8 == psnText.GetLength())
        {
            swprintf_s(psn, L"%S", psnText);
        }
    }
    
    memcpy(pData + offset, psn, 16);
    offset += 16;
    for (int i = offset; i < offset+(13 * 2); i+=2)
    {
        *(pData + i) = 0x00;
        *(pData + i+1) = 0x20;
    }
}

static void SetMdtData(CSparkUfsPdtDlg* pDlg,char* pData)
{
    char mdt[4] = { 0 };
    ConvertWCharDataToCharData(pDlg->GetUfsOption()->mainPrm.mdt, _countof(pDlg->GetUfsOption()->mainPrm.mdt),
        mdt, sizeof(mdt), CP_ACP);
	UINT16 mdtValue1 = wcstoul(pDlg->GetUfsOption()->mainPrm.mdt, nullptr, 16);
    mdtValue1 = _byteswap_ushort(mdtValue1);
    memcpy(pData, &mdtValue1, sizeof(mdtValue1));
    
}

static int SetSnStage(CSparkUfsPdtDlg* pDlg, int portIndex, pdt_log_config_t& lg)
{
    int ret = ERROR_SUCCESS;
    char pData[512 * 8] = { 0 };
    TaskProgressMsg* pmsg = new TaskProgressMsg{ portIndex, 0, 0, _T("SetSn") };
    if (pDlg) pDlg->PostMessage(CSparkUfsPdtDlg::WM_TASK_PROGRESS, (WPARAM)pmsg, 0);

    UCHAR u08PhyIdx = CSparkSm3350Util::GetPhysicalIndex((UCHAR)portIndex);
    CSparkSm3350Util& sm3350 = CSparkSm3350Util::getInstance(u08PhyIdx);
    do
    {
        SetSnData(pDlg, portIndex, pData);
        if ((ret = sm3350.UfsSetSrialNumberString(pData)) != ERROR_SUCCESS) break;
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
    char pData[512*8]={0};
    TaskProgressMsg* pmsg = new TaskProgressMsg{ portIndex, 0, 0, _T("SetMdt") };
    if (pDlg) pDlg->PostMessage(CSparkUfsPdtDlg::WM_TASK_PROGRESS, (WPARAM)pmsg, 0);

    UCHAR u08PhyIdx = CSparkSm3350Util::GetPhysicalIndex((UCHAR)portIndex);
    CSparkSm3350Util& sm3350 = CSparkSm3350Util::getInstance(u08PhyIdx);

    do
    {
        SetMdtData(pDlg,pData);
        if ((ret = sm3350.UfsSetManuDate(pData)) != ERROR_SUCCESS) break;
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

static int VerifyIspStage(CSparkUfsPdtDlg* pDlg, int portIndex, pdt_log_config_t& lg)
{
    int ret = ERROR_SUCCESS;
    char pData[512 * 8] = { 0 };
    TaskProgressMsg* pmsg = new TaskProgressMsg{ portIndex, 0, 0, _T("VerifyISP") };
    if (pDlg) pDlg->PostMessage(CSparkUfsPdtDlg::WM_TASK_PROGRESS, (WPARAM)pmsg, 0);

    UCHAR u08PhyIdx = CSparkSm3350Util::GetPhysicalIndex((UCHAR)portIndex);
    CSparkSm3350Util& sm3350 = CSparkSm3350Util::getInstance(u08PhyIdx);
    do
    {
        if ((ret = sm3350.UfsCheckIsp(pData)) != ERROR_SUCCESS) break;
        if (pData[0x08] == 'M' && pData[0x09] == '5' && pData[0x0A] == '3' && pData[0x0B] == 'A')
        {
            ret = ERROR_SUCCESS;
        }
        else
        {//VERIFY ISP error
            ret = 0xF18;
        }
    } while (0);
    if (ret != ERROR_SUCCESS)
    {
        TaskProgressMsg* pErr = new TaskProgressMsg{ portIndex, 0, ret, _T("VerifyISP Failed") };
        if (pDlg) pDlg->PostMessage(CSparkUfsPdtDlg::WM_TASK_PROGRESS, (WPARAM)pErr, 0);
        lg.error_code = ret;
        ZeroMemory(lg.state, sizeof(lg.state));
        strncpy_s(lg.state, _countof(lg.state), "VerifyISP Failed", _TRUNCATE);
    }
    return ret;
}

static int WriteSramStage(CSparkUfsPdtDlg* pDlg, int portIndex, pdt_log_config_t& lg)
{
    int ret = ERROR_SUCCESS;
    char pData[512 * 8] = { 0 };
    TaskProgressMsg* pmsg = new TaskProgressMsg{ portIndex, 0, 0, _T("WriteSram") };
    if (pDlg) pDlg->PostMessage(CSparkUfsPdtDlg::WM_TASK_PROGRESS, (WPARAM)pmsg, 0);

    UCHAR u08PhyIdx = CSparkSm3350Util::GetPhysicalIndex((UCHAR)portIndex);
    CSparkSm3350Util& sm3350 = CSparkSm3350Util::getInstance(u08PhyIdx);
    do
    {
        if ((ret = sm3350.UfsWriteSramMp(g_UfsIsp, BYTE2SECTOR(sizeof(g_UfsIsp)))) != ERROR_SUCCESS) break;
    } while (0);
    if (ret != ERROR_SUCCESS)
    {
        TaskProgressMsg* pErr = new TaskProgressMsg{ portIndex, 0, ret, _T("WriteSram Failed") };
        if (pDlg) pDlg->PostMessage(CSparkUfsPdtDlg::WM_TASK_PROGRESS, (WPARAM)pErr, 0);
        lg.error_code = ret;
        ZeroMemory(lg.state, sizeof(lg.state));
        strncpy_s(lg.state, _countof(lg.state), "WriteSram Failed", _TRUNCATE);
    }
    return ret;
}

static int VerifySram1Stage(CSparkUfsPdtDlg* pDlg, int portIndex, pdt_log_config_t& lg)
{
    int ret = ERROR_SUCCESS;
    char pData[512 * 8] = { 0 };
    TaskProgressMsg* pmsg = new TaskProgressMsg{ portIndex, 0, 0, _T("VerifySram") };
    if (pDlg) pDlg->PostMessage(CSparkUfsPdtDlg::WM_TASK_PROGRESS, (WPARAM)pmsg, 0);

    UCHAR u08PhyIdx = CSparkSm3350Util::GetPhysicalIndex((UCHAR)portIndex);
    CSparkSm3350Util& sm3350 = CSparkSm3350Util::getInstance(u08PhyIdx);
    do
    {
        if ((ret = sm3350.UfsReadSramResult(pData,8)) != ERROR_SUCCESS) break;

        else
        {//VERIFY SRAM error
            if (pData[0] == 0x00 && pData[1] == 0x00 && pData[2] == 0x00 && pData[3] == 0x00)
            {
                //ERROR_SUCCESS
            }
            else
            {
                ret = 0xF21;
            }

        }
    } while (0);
    if (ret != ERROR_SUCCESS)
    {
        TaskProgressMsg* pErr = new TaskProgressMsg{ portIndex, 0, ret, _T("VerifySram Failed") };
        if (pDlg) pDlg->PostMessage(CSparkUfsPdtDlg::WM_TASK_PROGRESS, (WPARAM)pErr, 0);
        lg.error_code = ret;
        ZeroMemory(lg.state, sizeof(lg.state));
        strncpy_s(lg.state, _countof(lg.state), "VerifySram Failed", _TRUNCATE);
    }
    return ret;
}

static int VerifySram2Stage(CSparkUfsPdtDlg* pDlg, int portIndex, pdt_log_config_t& lg)
{
    int ret = ERROR_SUCCESS;
    char pData1[512 * 8] = { 0 };
    char pData2[512 * 8] = { 0 };
    TaskProgressMsg* pmsg = new TaskProgressMsg{ portIndex, 0, 0, _T("VerifySram") };
    if (pDlg) pDlg->PostMessage(CSparkUfsPdtDlg::WM_TASK_PROGRESS, (WPARAM)pmsg, 0);

    UCHAR u08PhyIdx = CSparkSm3350Util::GetPhysicalIndex((UCHAR)portIndex);
    CSparkSm3350Util& sm3350 = CSparkSm3350Util::getInstance(u08PhyIdx);
    do
    {
        if ((ret = sm3350.UfsCheckSram2(pData1, pData2)) != ERROR_SUCCESS) break;

        else
        {//VERIFY SRAM error
            if (pData2[0] == 0x00 && pData2[1] == 0x00 && pData2[2] == 0x00 && pData2[3] == 0x00)
            {
                //ERROR_SUCCESS
            }
            else
            {
                ret = 0xF22;
            }
            
        }
    } while (0);
    if (ret != ERROR_SUCCESS)
    {
        TaskProgressMsg* pErr = new TaskProgressMsg{ portIndex, 0, ret, _T("VerifySram Failed") };
        if (pDlg) pDlg->PostMessage(CSparkUfsPdtDlg::WM_TASK_PROGRESS, (WPARAM)pErr, 0);
        lg.error_code = ret;
        ZeroMemory(lg.state, sizeof(lg.state));
        strncpy_s(lg.state, _countof(lg.state), "VerifySram Failed", _TRUNCATE);
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
            //if ((ret = CardInitStage(pDlg, portIndex, lg)) != ERROR_SUCCESS) break;
            if ((ret = UpiuForceRomStage(pDlg, portIndex, lg)) != ERROR_SUCCESS) break;
            Sleep(300);
            if ((ret = MpStartStage(pDlg, portIndex, lg)) != ERROR_SUCCESS) break;
            Sleep(300);
            if ((ret = Write1024KIspMpStage(pDlg, portIndex, lg)) != ERROR_SUCCESS) break;
            if ((ret = MpExitStage(pDlg, portIndex, lg)) != ERROR_SUCCESS) break;
            if ((ret = CardInitStage(pDlg, portIndex, lg)) != ERROR_SUCCESS) break;
            Sleep(300);
            if ((ret = SetMdtStage(pDlg, portIndex, lg)) != ERROR_SUCCESS) break;
            if ((ret = SetSnStage(pDlg, portIndex, lg)) != ERROR_SUCCESS) break;
            if ((ret = CardInitStage(pDlg, portIndex, lg)) != ERROR_SUCCESS) break;
            Sleep(300);
            if ((ret = VerifyIspStage(pDlg, portIndex, lg)) != ERROR_SUCCESS) break;
            if ((ret = PowerOffStage(pDlg, portIndex, lg)) != ERROR_SUCCESS) break;


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
            if ((ret = UpiuForceRomStage(pDlg, portIndex, lg)) != ERROR_SUCCESS) break;
            Sleep(300);
            if ((ret = CardInitStage(pDlg, portIndex, lg)) != ERROR_SUCCESS) break;
            if ((ret = VerifyIspStage(pDlg, portIndex, lg)) != ERROR_SUCCESS) break;
            if ((ret = UpiuForceRomStage(pDlg, portIndex, lg)) != ERROR_SUCCESS) break;
            Sleep(300);
            if ((ret = MpStartStage(pDlg, portIndex, lg)) != ERROR_SUCCESS) break;
            if ((ret = WriteSramStage(pDlg, portIndex, lg)) != ERROR_SUCCESS) break;
            if ((ret = MpExitStage(pDlg, portIndex, lg)) != ERROR_SUCCESS) break;
            if ((ret = VerifySram1Stage(pDlg, portIndex, lg)) != ERROR_SUCCESS) break;
            if ((ret = CardInitStage(pDlg, portIndex, lg)) != ERROR_SUCCESS) break;
            Sleep(300);
            if ((ret = VerifySram2Stage(pDlg, portIndex, lg)) != ERROR_SUCCESS) break;
            Sleep(300);
            if ((ret = CardInitStage(pDlg, portIndex, lg)) != ERROR_SUCCESS) break;
            if ((ret = PowerOffStage(pDlg, portIndex, lg)) != ERROR_SUCCESS) break;


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
