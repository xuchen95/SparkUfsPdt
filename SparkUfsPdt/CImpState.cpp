#include "pch.h"
#include "CImpState.h"
#include "SparkUfsPdtDlg.h"
#include "resource.h"
#include "libsparkusb.h"
#include "PubFunc.h"

using namespace spark::sm3350;
using TaskProgressMsg = CSparkUfsPdtDlg::TaskProgressMsg;

bool CImpState::ConvertWCharDataToCharData(const WCHAR* wSrc, size_t wSrcLen,
    char* cDest, size_t cDestLen,
    UINT codePage)
{
    if (wSrc == nullptr || cDest == nullptr || wSrcLen == 0 || cDestLen == 0)
    {
        return false;
    }

    size_t tempBufSize = wSrcLen * 3;
    char* tempBuf = new char[tempBufSize];
    memset(tempBuf, 0, tempBufSize);

    int convertLen = WideCharToMultiByte(
        codePage,
        0,
        wSrc,
        static_cast<int>(wSrcLen),
        tempBuf,
        static_cast<int>(tempBufSize),
        nullptr,
        nullptr);

    if (convertLen == 0)
    {
        delete[] tempBuf;
        memset(cDest, 0, cDestLen);
        return false;
    }

    if (static_cast<size_t>(convertLen) >= cDestLen)
    {
        memcpy(cDest, tempBuf, cDestLen);
    }
    else
    {
        memcpy(cDest, tempBuf, convertLen);
        memset(cDest + convertLen, 0, cDestLen - convertLen);
    }

    delete[] tempBuf;
    return true;
}

int CImpState::PowerOffStage(CSparkUfsPdtDlg* pDlg, int portIndex, pdt_log_config_t& lg)
{
    int ret = ERROR_SUCCESS;
    TaskProgressMsg* pmsg = new TaskProgressMsg{ portIndex, 0, 0, _T("PowerOff") };
    if (pDlg) pDlg->PostMessage(CSparkUfsPdtDlg::WM_TASK_PROGRESS, (WPARAM)pmsg, 0);

    UCHAR u08PhyIdx = CSparkSm3350Util::GetPhysicalIndex((UCHAR)portIndex);
    CSparkSm3350Util& sm3350 = CSparkSm3350Util::getInstance(u08PhyIdx);
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

int CImpState::RebootStage(CSparkUfsPdtDlg* pDlg, int portIndex, pdt_log_config_t& lg)
{
    int ret = ERROR_SUCCESS;
    TaskProgressMsg* pmsg = new TaskProgressMsg{ portIndex, 0, 0, _T("Rebooting") };
    if (pDlg) pDlg->PostMessage(CSparkUfsPdtDlg::WM_TASK_PROGRESS, (WPARAM)pmsg, 0);

    UCHAR u08PhyIdx = CSparkSm3350Util::GetPhysicalIndex((UCHAR)portIndex);
    CSparkSm3350Util& sm3350 = CSparkSm3350Util::getInstance(u08PhyIdx);
    char pData[512 * 8] = { 0 };
    //DEBUG data
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

int CImpState::CardInitStage(CSparkUfsPdtDlg* pDlg, int portIndex, pdt_log_config_t& lg)
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

int CImpState::UpiuForceRomStage(CSparkUfsPdtDlg* pDlg, int portIndex, pdt_log_config_t& lg)
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

int CImpState::VccOffForceRomStage(CSparkUfsPdtDlg* pDlg, int portIndex, pdt_log_config_t& lg)
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

int CImpState::MpStartStage(CSparkUfsPdtDlg* pDlg, int portIndex, pdt_log_config_t& lg)
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

int CImpState::Write1024KIspMpStage(CSparkUfsPdtDlg* pDlg, int portIndex, pdt_log_config_t& lg)
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

int CImpState::MpExitStage(CSparkUfsPdtDlg* pDlg, int portIndex, pdt_log_config_t& lg)
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

void CImpState::SetSnData(CSparkUfsPdtDlg* pDlg, int portIndex, char* pData)
{
    if (pDlg == nullptr || pData == nullptr)
    {
        return;
    }

    constexpr size_t kSnPayloadBytes = 0x40;
    ZeroMemory(pData, kSnPayloadBytes);
    int offset = 0;
    pData[0] = 0x40;
    pData[1] = 0x05;
    offset += 2;

    memcpy(pData + offset, pDlg->GetUfsOption()->mainPrm.meto, 4);
    offset += 4;

    SYSTEMTIME st;
    GetLocalTime(&st);

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
    for (int i = offset; i < offset + (13 * 2); i += 2)
    {
        *(pData + i) = 0x00;
        *(pData + i + 1) = 0x20;
    }
}

void CImpState::GetIspString(CSparkUfsPdtDlg* pDlg, char* isp)
{
    if (pDlg == nullptr || isp == nullptr)
    {
        return;
    }
	CPubFunc::HexToBytes(pDlg->GetUfsOption()->qcPrm.isp, (BYTE*)isp, sizeof(pDlg->GetUfsOption()->qcPrm.isp)/2);
}

void CImpState::SetMdtData(CSparkUfsPdtDlg* pDlg, char* pData)
{
    if (pDlg == nullptr || pData == nullptr)
    {
        return;
    }
    BYTE vMdt[2];
	CPubFunc::HexToBytes(pDlg->GetUfsOption()->mainPrm.mdt, vMdt,sizeof(vMdt));
    memcpy(pData, vMdt, sizeof(vMdt));
}

int CImpState::SetSnStage(CSparkUfsPdtDlg* pDlg, int portIndex, pdt_log_config_t& lg)
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

int CImpState::SetMdtStage(CSparkUfsPdtDlg* pDlg, int portIndex, pdt_log_config_t& lg)
{
    int ret = ERROR_SUCCESS;
    char pData[512 * 8] = { 0 };
    TaskProgressMsg* pmsg = new TaskProgressMsg{ portIndex, 0, 0, _T("SetMdt") };
    if (pDlg) pDlg->PostMessage(CSparkUfsPdtDlg::WM_TASK_PROGRESS, (WPARAM)pmsg, 0);

    UCHAR u08PhyIdx = CSparkSm3350Util::GetPhysicalIndex((UCHAR)portIndex);
    CSparkSm3350Util& sm3350 = CSparkSm3350Util::getInstance(u08PhyIdx);

    do
    {
        SetMdtData(pDlg, pData);
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

int CImpState::VerifyIspStage(CSparkUfsPdtDlg* pDlg, int portIndex, pdt_log_config_t& lg)
{
    int ret = ERROR_SUCCESS;
    char ispString[32] = { 0 };
    char pData[512 * 8] = { 0 };
    TaskProgressMsg* pmsg = new TaskProgressMsg{ portIndex, 0, 0, _T("VerifyISP") };
    if (pDlg) pDlg->PostMessage(CSparkUfsPdtDlg::WM_TASK_PROGRESS, (WPARAM)pmsg, 0);
    GetIspString(pDlg, ispString);

    UCHAR u08PhyIdx = CSparkSm3350Util::GetPhysicalIndex((UCHAR)portIndex);
    CSparkSm3350Util& sm3350 = CSparkSm3350Util::getInstance(u08PhyIdx);
    do
    {
        if ((ret = sm3350.UfsCheckIsp(pData)) != ERROR_SUCCESS) break;

        if (memcmp(ispString, pData, 8))
        {
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

int CImpState::WriteSramStage(CSparkUfsPdtDlg* pDlg, int portIndex, pdt_log_config_t& lg)
{
    int ret = ERROR_SUCCESS;
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

int CImpState::VerifySram1Stage(CSparkUfsPdtDlg* pDlg, int portIndex, pdt_log_config_t& lg)
{
    int ret = ERROR_SUCCESS;
    char pData[512 * 8] = { 0 };
    TaskProgressMsg* pmsg = new TaskProgressMsg{ portIndex, 0, 0, _T("VerifySram") };
    if (pDlg) pDlg->PostMessage(CSparkUfsPdtDlg::WM_TASK_PROGRESS, (WPARAM)pmsg, 0);

    UCHAR u08PhyIdx = CSparkSm3350Util::GetPhysicalIndex((UCHAR)portIndex);
    CSparkSm3350Util& sm3350 = CSparkSm3350Util::getInstance(u08PhyIdx);
    do
    {
        if ((ret = sm3350.UfsReadSramResult(pData, 8)) != ERROR_SUCCESS) break;

        if (!(pData[0] == 0x00 && pData[1] == 0x00 && pData[2] == 0x00 && pData[3] == 0x00))
        {
            ret = 0xF21;
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

int CImpState::VerifySram2Stage(CSparkUfsPdtDlg* pDlg, int portIndex, pdt_log_config_t& lg)
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

        if (!(pData2[0] == 0x00 && pData2[1] == 0x00 && pData2[2] == 0x00 && pData2[3] == 0x00))
        {
            ret = 0xF22;
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
