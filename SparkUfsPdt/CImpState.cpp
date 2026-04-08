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
            ret = ERR_ISP_VER_MISMATCH;
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
            ret = ERR_SRAM1_TEST_FAILED;
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
            ret = ERR_SRAM2_TEST_FAILED;
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
//
//int CImpState::VerifyCidStage(CSparkUfsPdtDlg* pDlg, int portIndex, pdt_log_config_t& lg)
//{
//    int ret = ERROR_SUCCESS;
//    char pData[512 * 0x03] = { 0 };
//
//#define MNM_DATA_OFFSET 0x02
//#define CAP_DATA_OFFSET (16 * 16 + 13)
//#define MID_DATA_OFFSET (16 * 20 + 4)
//#define PNM_DATA_OFFSET (16 * 44 + 6)
//#define PSN_DATA_OFFSET (16 * 76 + 2)
//
//    TaskProgressMsg* pmsg = new TaskProgressMsg{ portIndex, 0, 0, _T("VerifyCid") };
//    if (pDlg) pDlg->PostMessage(CSparkUfsPdtDlg::WM_TASK_PROGRESS, (WPARAM)pmsg, 0);
//
//    if (pDlg == nullptr || pDlg->GetUfsOption() == nullptr)
//    {
//        ret = ERROR_INVALID_PARAMETER;
//    }
//    else
//    {
//        UCHAR u08PhyIdx = CSparkSm3350Util::GetPhysicalIndex((UCHAR)portIndex);
//        CSparkSm3350Util& sm3350 = CSparkSm3350Util::getInstance(u08PhyIdx);
//
//        do
//        {
//            if ((ret = sm3350.UfsReadCidInfo(pData, BYTE2SECTOR(sizeof(pData)))) != ERROR_SUCCESS) break;
//
//            // MNM: 固定字段长度比较（WCHAR）
//            const SIZE_T mnmFieldChars = sizeof(pDlg->GetUfsOption()->qcPrm.mnm);
//            WCHAR mnmExpected[sizeof(pDlg->GetUfsOption()->qcPrm.mnm)] = { 0 };
//            const size_t mnmSrcLen = strnlen_s(pDlg->GetUfsOption()->qcPrm.mnm, sizeof(pDlg->GetUfsOption()->qcPrm.mnm));
//            if (mnmSrcLen > 0)
//            {
//                if (!CPubFunc::CharToWChar(pDlg->GetUfsOption()->qcPrm.mnm, mnmSrcLen, mnmExpected, mnmFieldChars))
//                {
//                    ret = ERR_MNM_MISMATCH;
//                    break;
//                }
//            }
//            if (0 != memcmp(mnmExpected, pData + MNM_DATA_OFFSET, mnmFieldChars * sizeof(WCHAR)))
//            {
//                ret = ERR_MNM_MISMATCH;
//                break;
//            }
//
//            // 4KB Count: 避免未对齐访问
//            ULONG capRaw = 0;
//            memcpy(&capRaw, pData + CAP_DATA_OFFSET, sizeof(capRaw));
//            const ULONG n4KBCntD = pDlg->GetUfsOption()->qcPrm.n4KBCnt;
//            const ULONG n4KBCntS = _byteswap_ulong(capRaw);
//            if (n4KBCntD != n4KBCntS)
//            {
//                ret = ERR_4KBCNT_MISMATCH;
//                break;
//            }
//
//            // MID: 固定长度字节比较
//            const SIZE_T midLen = sizeof(pDlg->GetUfsOption()->qcPrm.mid);
//            if (0 != memcmp(pDlg->GetUfsOption()->qcPrm.mid, pData + MID_DATA_OFFSET, midLen))
//            {
//                ret = ERR_MID_MISMATCH;
//                break;
//            }
//
//            // PNM: 按字符串实际长度比较（WCHAR）
//            const SIZE_T pnmFieldChars = sizeof(pDlg->GetUfsOption()->qcPrm.pnm);
//            WCHAR pnmExpected[sizeof(pDlg->GetUfsOption()->qcPrm.pnm)] = { 0 };
//            const size_t pnmSrcLen = strnlen_s(pDlg->GetUfsOption()->qcPrm.pnm, sizeof(pDlg->GetUfsOption()->qcPrm.pnm));
//            if (pnmSrcLen == 0)
//            {
//                ret = ERR_PNM_MISMATCH;
//                break;
//            }
//            if (!CPubFunc::CharToWChar(pDlg->GetUfsOption()->qcPrm.pnm, pnmSrcLen, pnmExpected, pnmFieldChars))
//            {
//                ret = ERR_PNM_MISMATCH;
//                break;
//            }
//            if (0 != memcmp(pnmExpected, pData + PNM_DATA_OFFSET, pnmSrcLen * sizeof(WCHAR)))
//            {
//                ret = ERR_PNM_MISMATCH;
//                break;
//            }
//
//            CString strsn;
//            for (int i = PSN_DATA_OFFSET; i < PSN_DATA_OFFSET + 24; ++i)
//            {
//                strsn.AppendChar(pData[i]);
//            }
//            strsn.AppendChar('\0');
//
//        } while (0);
//    }
//
//    if (ret != ERROR_SUCCESS)
//    {
//        TaskProgressMsg* pErr = new TaskProgressMsg{ portIndex, 0, ret, _T("VerifyCid Failed") };
//        if (pDlg) pDlg->PostMessage(CSparkUfsPdtDlg::WM_TASK_PROGRESS, (WPARAM)pErr, 0);
//        lg.error_code = ret;
//        ZeroMemory(lg.state, sizeof(lg.state));
//        strncpy_s(lg.state, _countof(lg.state), "VerifyCid Failed", _TRUNCATE);
//    }
//    return ret;
//}
//

int CImpState::VerifyCidStage(CSparkUfsPdtDlg* pDlg, int portIndex, pdt_log_config_t& lg)
{
    int ret = ERROR_SUCCESS;
    char pData[512 * 0x03] = { 0 };
    CStringW strSn;
#define MNM_DATA_OFFSET 0x02
#define CAP_DATA_OFFSET (16 * 16 + 13)
#define MID_DATA_OFFSET (16 * 20 + 4)
#define PNM_DATA_OFFSET (16 * 44 + 6)
#define PSN_DATA_OFFSET (16 * 76 + 2)

    TaskProgressMsg* pmsg = new TaskProgressMsg{ portIndex, 0, 0, _T("VerifyCid") };
    if (pDlg) pDlg->PostMessage(CSparkUfsPdtDlg::WM_TASK_PROGRESS, (WPARAM)pmsg, 0);

    if (pDlg == nullptr || pDlg->GetUfsOption() == nullptr)
    {
        ret = ERROR_INVALID_PARAMETER;
    }
    else
    {
        UCHAR u08PhyIdx = CSparkSm3350Util::GetPhysicalIndex((UCHAR)portIndex);
        CSparkSm3350Util& sm3350 = CSparkSm3350Util::getInstance(u08PhyIdx);

        do
        {
            if ((ret = sm3350.UfsReadCidInfo(pData, BYTE2SECTOR(sizeof(pData)))) != ERROR_SUCCESS) break;

            //---------------------------------------------------------------------
            // MNM: 修复大端 WCHAR 比较
            //---------------------------------------------------------------------
            const SIZE_T mnmFieldChars = sizeof(pDlg->GetUfsOption()->qcPrm.mnm);
            WCHAR mnmExpected[sizeof(pDlg->GetUfsOption()->qcPrm.mnm)] = { 0 };
            const size_t mnmSrcLen = strnlen_s(pDlg->GetUfsOption()->qcPrm.mnm, sizeof(pDlg->GetUfsOption()->qcPrm.mnm));
            if (mnmSrcLen > 0)
            {
                if (!CPubFunc::CharToWChar(pDlg->GetUfsOption()->qcPrm.mnm, (int)mnmSrcLen, mnmExpected, (int)mnmFieldChars))
                {
                    ret = ERR_MNM_MISMATCH;
                    break;
                }
            }

            // pData 是大端 WCHAR，逐个字节反转再比较
            bool mnmMatch = true;
            const WCHAR* pMnmData = (const WCHAR*)(pData + MNM_DATA_OFFSET);
            for (size_t i = 0; i < mnmSrcLen; i++)
            {
                WCHAR beChar = pMnmData[i];
                WCHAR leChar = _byteswap_ushort((USHORT)beChar); // 大端 → 小端
                if (leChar != mnmExpected[i])
                {
                    mnmMatch = false;
                    break;
                }
            }
            if (!mnmMatch)
            {
                ret = ERR_MNM_MISMATCH;
                break;
            }

            //---------------------------------------------------------------------
            // 4KB Count 
            //---------------------------------------------------------------------
            ULONG capRaw = 0;
            memcpy(&capRaw, pData + CAP_DATA_OFFSET, sizeof(capRaw));
            const ULONG n4KBCntD = pDlg->GetUfsOption()->qcPrm.n4KBCnt;
            const ULONG n4KBCntS = _byteswap_ulong(capRaw);
            if (n4KBCntD != n4KBCntS)
            {
                ret = ERR_4KBCNT_MISMATCH;
                break;
            }

            //---------------------------------------------------------------------
            // MID
            //---------------------------------------------------------------------
            const SIZE_T midLen = sizeof(pDlg->GetUfsOption()->qcPrm.mid);
            if (memcmp(pDlg->GetUfsOption()->qcPrm.mid, pData + MID_DATA_OFFSET, midLen) != 0)
            {
                ret = ERR_MID_MISMATCH;
                break;
            }

            //---------------------------------------------------------------------
            // PNM: 
            //---------------------------------------------------------------------
            const SIZE_T pnmFieldChars = sizeof(pDlg->GetUfsOption()->qcPrm.pnm);
            WCHAR pnmExpected[sizeof(pDlg->GetUfsOption()->qcPrm.pnm)] = { 0 };
            const size_t pnmSrcLen = strnlen_s(pDlg->GetUfsOption()->qcPrm.pnm, sizeof(pDlg->GetUfsOption()->qcPrm.pnm));
            if (pnmSrcLen == 0)
            {
                ret = ERR_PNM_MISMATCH;
                break;
            }
            if (!CPubFunc::CharToWChar(pDlg->GetUfsOption()->qcPrm.pnm, (int)pnmSrcLen, pnmExpected, (int)pnmFieldChars))
            {
                ret = ERR_PNM_MISMATCH;
                break;
            }

            // pData 是大端 WCHAR，逐个字节反转再比较
            bool pnmMatch = true;
            const WCHAR* pPnmData = (const WCHAR*)(pData + PNM_DATA_OFFSET);
            for (size_t i = 0; i < pnmSrcLen; i++)
            {
                WCHAR beChar = pPnmData[i];
                WCHAR leChar = _byteswap_ushort((USHORT)beChar);
                if (leChar != pnmExpected[i])
                {
                    pnmMatch = false;
                    break;
                }
            }
            if (!pnmMatch)
            {
                ret = ERR_PNM_MISMATCH;
                break;
            }

            //---------------------------------------------------------------------
            // SN 读取
            //---------------------------------------------------------------------
            
            for (int i = 0; i < 36; i += 2)
            {
                USHORT beValue = (pData[PSN_DATA_OFFSET + i] << 8) | pData[PSN_DATA_OFFSET + i + 1];
                WCHAR wch = _byteswap_ushort(beValue);

                if (wch == L'\0') break;
                strSn.AppendChar(wch);
            }
        } while (0);
    }

    if (ret != ERROR_SUCCESS)
    {
        TaskProgressMsg* pErr = new TaskProgressMsg{ portIndex, 0, ret, _T("VerifyCid Failed") };
        if (pDlg) pDlg->PostMessage(CSparkUfsPdtDlg::WM_TASK_PROGRESS, (WPARAM)pErr, 0);
        lg.error_code = ret;
        ZeroMemory(lg.state, sizeof(lg.state));
        strncpy_s(lg.state, _countof(lg.state), "VerifyCid Failed", _TRUNCATE);
    }
    return ret;
}