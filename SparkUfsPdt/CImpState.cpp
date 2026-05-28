#include "pch.h"
#include "CImpState.h"
#include "SparkUfsPdtDlg.h"
#include "resource.h"
#include "libsparkusb.h"
#include "PubFunc.h"
#include "DataFormatter.h"
#include "IspMarkCache.h"
#include <cctype>
#include "DialogAdapter.h"
#include <vector>

using namespace spark::sm3350;
using TaskProgressMsg = CSparkUfsPdtDlg::TaskProgressMsg;

namespace
{
    bool EncodeIspMark(const char* ispMark, BYTE* outBuf, size_t outBufLen)
    {
        if (ispMark == nullptr || outBuf == nullptr || outBufLen < 12)
        {
            return false;
        }

        char part2Str[5] = { 0 };
        char part3Str[9] = { 0 };
        memcpy(part2Str, ispMark + 4, 4);
        memcpy(part3Str, ispMark + 8, 8);

        for (int i = 0; i < 4; ++i)
        {
            if (!std::isdigit(static_cast<unsigned char>(part2Str[i])))
            {
                return false;
            }
        }
        for (int i = 0; i < 8; ++i)
        {
            if (!std::isdigit(static_cast<unsigned char>(part3Str[i])))
            {
                return false;
            }
        }

        unsigned long part2Value = strtoul(part2Str, nullptr, 10);
        unsigned long part3Value = strtoul(part3Str, nullptr, 10);
        if (part2Value > 0xFFFFUL || part3Value > 0xFFFFFFFFUL)
        {
            return false;
        }

        outBuf[0] = static_cast<BYTE>(part2Value & 0xFF);          // 2964 -> 94 0B
        outBuf[1] = static_cast<BYTE>((part2Value >> 8) & 0xFF);
        outBuf[2] = 0x00;
        outBuf[3] = 0x00;
        outBuf[4] = static_cast<BYTE>(part3Value & 0xFF);          // 04270945 -> 61 2B 41 00
        outBuf[5] = static_cast<BYTE>((part3Value >> 8) & 0xFF);
        outBuf[6] = static_cast<BYTE>((part3Value >> 16) & 0xFF);
        outBuf[7] = static_cast<BYTE>((part3Value >> 24) & 0xFF);
        memcpy(outBuf + 8, ispMark, 4);                             // M53B -> 4D 35 33 42
        return true;
    }
}

// IspMark storage moved to IspMarkCache singleton

CImpState::CImpState(ISettingsProvider* settings, ILogger* logger, IUiNotifier* notifier)
    : settings_(settings), logger_(logger), notifier_(notifier)
{
}

bool CImpState::ConvertWCharDataToCharData(const WCHAR* wSrc, size_t wSrcLen,
    char* cDest, size_t cDestLen,
    UINT codePage)
{
    if (wSrc == nullptr || cDest == nullptr || wSrcLen == 0 || cDestLen == 0)
    {
        return false;
    }
    size_t tempBufSize = wSrcLen * 3;
    std::vector<char> tempBuf(tempBufSize);
    memset(tempBuf.data(), 0, tempBufSize);

    int convertLen = WideCharToMultiByte(
        codePage,
        0,
        wSrc,
        static_cast<int>(wSrcLen),
        tempBuf.data(),
        static_cast<int>(tempBufSize),
        nullptr,
        nullptr);

    if (convertLen == 0)
    {
        memset(cDest, 0, cDestLen);
        return false;
    }

    if (static_cast<size_t>(convertLen) >= cDestLen)
    {
        memcpy(cDest, tempBuf.data(), cDestLen);
    }
    else
    {
        memcpy(cDest, tempBuf.data(), convertLen);
        memset(cDest + convertLen, 0, cDestLen - convertLen);
    }
    return true;
}

int CImpState::PowerOffStage(int portIndex, pdt_log_config_t& lg)
{
    int ret = ERROR_SUCCESS;

    UCHAR u08PhyIdx = CSparkSm3350Util::GetPhysicalIndex((UCHAR)portIndex);
    CSparkSm3350Util& sm3350 = CSparkSm3350Util::getInstance(u08PhyIdx);
    do
    {
        if ((ret = sm3350.UfsPowerOff()) != ERROR_SUCCESS) break;
        Sleep(100);
    } while (0);

    if (ret != ERROR_SUCCESS)
    {
        if (notifier_) notifier_->PostTaskProgress(portIndex, 0, ret, _T("PowerOff Failed"));
        lg.error_code = ret;
        ZeroMemory(lg.state, sizeof(lg.state));
        strncpy_s(lg.state, _countof(lg.state), "PowerOff Failed", _TRUNCATE);
    }
    return ret;
}

int CImpState::RebootStage(int portIndex, pdt_log_config_t& lg)
{
    int ret = ERROR_SUCCESS;


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
        if (notifier_) notifier_->PostTaskProgress(portIndex, 0, ret, _T("Reboot Failed"));
        lg.error_code = ret;
        ZeroMemory(lg.state, sizeof(lg.state));
        strncpy_s(lg.state, _countof(lg.state), "Reboot Failed", _TRUNCATE);
    }
    return ret;
}

int CImpState::CardInitStage(int portIndex, pdt_log_config_t& lg)
{
    int ret = ERROR_SUCCESS;


    UCHAR u08PhyIdx = CSparkSm3350Util::GetPhysicalIndex((UCHAR)portIndex);
    CSparkSm3350Util& sm3350 = CSparkSm3350Util::getInstance(u08PhyIdx);

    ret = sm3350.UfsCardInit();
    if (ret != ERROR_SUCCESS)
    {
        if (notifier_) notifier_->PostTaskProgress(portIndex, 0, ret, _T("CardInit Failed"));
        lg.error_code = ret;
        ZeroMemory(lg.state, sizeof(lg.state));
        strncpy_s(lg.state, _countof(lg.state), "CardInit Failed", _TRUNCATE);
    }
    Sleep(300);
    return ret;
}

int CImpState::ForceRomStage(int portIndex, pdt_log_config_t& lg)
{
    int ret = ERROR_SUCCESS;
    BOOL bForceRomMode = FALSE;
    if (settings_ && settings_->GetBaseSetting())
    {
        bForceRomMode = settings_->GetBaseSetting()->ForceRomMode ? FALSE : FALSE; // preserve behavior for now
    }
    do
    {
        if (UPIU_FORCE_ROM_MODE == bForceRomMode)
        {
            if ((ret = UpiuForceRomStage(portIndex, lg)) != ERROR_SUCCESS) break;
        }
        else if (VCC_FORCE_ROM_MODE == bForceRomMode)
        {
            if ((ret = VccOffForceRomStage(portIndex, lg)) != ERROR_SUCCESS) break;
        }
    } while (0);
    Sleep(300);
    return ret;
}

int CImpState::UpiuForceRomStage(int portIndex, pdt_log_config_t& lg)
{
    int ret = ERROR_SUCCESS;


    UCHAR u08PhyIdx = CSparkSm3350Util::GetPhysicalIndex((UCHAR)portIndex);
    CSparkSm3350Util& sm3350 = CSparkSm3350Util::getInstance(u08PhyIdx);

    ret = sm3350.UpiuForceRom();
    if (ret != ERROR_SUCCESS)
    {
        if (notifier_) notifier_->PostTaskProgress(portIndex, 0, ret, _T("UpiuForceRom Failed"));
        lg.error_code = ret;
        ZeroMemory(lg.state, sizeof(lg.state));
        strncpy_s(lg.state, _countof(lg.state), "UpiuForceRom Failed", _TRUNCATE);
    }
    return ret;
}

int CImpState::VccOffForceRomStage(int portIndex, pdt_log_config_t& lg)
{
    int ret = ERROR_SUCCESS;


    UCHAR u08PhyIdx = CSparkSm3350Util::GetPhysicalIndex((UCHAR)portIndex);
    CSparkSm3350Util& sm3350 = CSparkSm3350Util::getInstance(u08PhyIdx);

    ret = sm3350.VccOffForceRom();
    if (ret != ERROR_SUCCESS)
    {
        if (notifier_) notifier_->PostTaskProgress(portIndex, 0, ret, _T("VccOffForceRom Failed"));
        lg.error_code = ret;
        ZeroMemory(lg.state, sizeof(lg.state));
        strncpy_s(lg.state, _countof(lg.state), "VccOffForceRom Failed", _TRUNCATE);
    }
    return ret;
}

int CImpState::MpStartStage(int portIndex, pdt_log_config_t& lg)
{
    int ret = ERROR_SUCCESS;


    UCHAR u08PhyIdx = CSparkSm3350Util::GetPhysicalIndex((UCHAR)portIndex);
    CSparkSm3350Util& sm3350 = CSparkSm3350Util::getInstance(u08PhyIdx);

    ret = sm3350.UfsMpStartMode();
    if (ret != ERROR_SUCCESS)
    {
        if (notifier_) notifier_->PostTaskProgress(portIndex, 0, ret, _T("MpStart Failed"));
        lg.error_code = ret;
        ZeroMemory(lg.state, sizeof(lg.state));
        strncpy_s(lg.state, _countof(lg.state), "MpStart Failed", _TRUNCATE);
    }
    return ret;
}

int CImpState::Write1024KIspMpStage(int portIndex, pdt_log_config_t& lg)
{
    int ret = ERROR_SUCCESS;
    PUFS_OPTION pOpt = settings_ ? settings_->GetUfsOption() : nullptr;
    BOOL bFuncOption = pOpt ? pOpt->mainPrm.funcSel : FALSE;

    UCHAR u08PhyIdx = CSparkSm3350Util::GetPhysicalIndex((UCHAR)portIndex);
    CSparkSm3350Util& sm3350 = CSparkSm3350Util::getInstance(u08PhyIdx);

    ret = sm3350.UfsWrite1024KIspMp(g_UfsIsp, BYTE2SECTOR(sizeof(g_UfsIsp)), bFuncOption);
    if (ret != ERROR_SUCCESS)
    {
        if (notifier_) notifier_->PostTaskProgress(portIndex, 0, ret, _T("Write1024KIspMp Failed"));
        lg.error_code = ret;
        ZeroMemory(lg.state, sizeof(lg.state));
        strncpy_s(lg.state, _countof(lg.state), "Write1024KIspMp Failed", _TRUNCATE);
    }
    return ret;
}

int CImpState::MpExitStage(int portIndex, pdt_log_config_t& lg)
{
    int ret = ERROR_SUCCESS;


    UCHAR u08PhyIdx = CSparkSm3350Util::GetPhysicalIndex((UCHAR)portIndex);
    CSparkSm3350Util& sm3350 = CSparkSm3350Util::getInstance(u08PhyIdx);

    ret = sm3350.UfsMpExit();
    if (ret != ERROR_SUCCESS)
    {
        if (notifier_) notifier_->PostTaskProgress(portIndex, 0, ret, _T("MpExit Failed"));
        lg.error_code = ret;
        ZeroMemory(lg.state, sizeof(lg.state));
        strncpy_s(lg.state, _countof(lg.state), "MpExit Failed", _TRUNCATE);
    }
    return ret;
}

void CImpState::SetSnData(int portIndex, char* pData)
{
    if (pData == nullptr)
    {
        return;
    }
    // Use DataFormatter to build SN payload, keep original behavior
    char meto[4] = {0};
    if (!settings_)
    {
        // settings provider missing, cannot format SN reliably
        return;
    }
    PUFS_OPTION pOpt = settings_->GetUfsOption();
    if (pOpt) memcpy(meto, pOpt->mainPrm.meto, 4);

    SYSTEMTIME st;
    GetLocalTime(&st);
    wchar_t timeStr[9];
    swprintf_s(timeStr, L"%04d%02d%02d", st.wYear, st.wMonth, st.wDay);

    wchar_t psn[9] = {0};
    CString psnText;
    // Attempt to use cached wide SN if present
    if (portIndex >= 0 && portIndex < _countof(m_strwSn))
    {
        psnText = m_strwSn[portIndex];
        if (8 == psnText.GetLength())
        {
            swprintf_s(psn, L"%S", psnText);
        }
    }

    auto formatted = spark::ufspdt::DataFormatter::FormatSnData(meto, timeStr, psn);

    

    if (portIndex >= 0 && portIndex < _countof(m_strwSn))
    {
        // Interpret the entire 64-byte formatted payload as WCHAR array and store
        const size_t wcharCount = formatted.size() / sizeof(WCHAR);
        const WCHAR* pWide = reinterpret_cast<const WCHAR*>(formatted.data());
        CStringW cachedSn;
        if (pWide && wcharCount > 0)
        {
            cachedSn = CStringW(pWide, static_cast<int>(wcharCount));
        }
        // Store full converted content into cache
        SetCachedSnForPort(portIndex, cachedSn);
    }

    memcpy(pData, formatted.data(), formatted.size());
    // Do not query UI for SerialNo during worker execution.
    // The allocated SN (if any) should be provided by the caller and cached
    // via SetCachedSnForPort before the task runs. Preserve any existing
    // cached value in m_strwSn.
}

void CImpState::GetQCIspString(char* isp)
{
    if (isp == nullptr)
    {
        return;
    }
    if (!settings_)
    {
        ZeroMemory(isp, 16);
        return;
    }
    PUFS_OPTION pOpt = settings_->GetUfsOption();
    if (pOpt)
    {
        CPubFunc::HexToBytes(pOpt->qcPrm.isp, (BYTE*)isp, sizeof(pOpt->qcPrm.isp)/2);
    }
}

void CImpState::GetIspMark(char* isp)
{
    if (isp == nullptr) return;
    ZeroMemory(isp, 12);
    unsigned char encoded[12] = {0};
    if (!spark::ufspdt::IspMarkCache::Instance().GetEncodedMark(encoded, sizeof(encoded)))
    {
        return;
    }
    memcpy(isp, encoded, sizeof(encoded));
}

void CImpState::SetMdtData(char* pData)
{
    if (pData == nullptr) return;
    PUFS_OPTION pOpt = settings_ ? settings_->GetUfsOption() : nullptr;
    if (!pOpt) return;
    auto mdt = spark::ufspdt::DataFormatter::FormatMdtFromHex(pOpt->mainPrm.mdt);
    memcpy(pData, mdt.data(), mdt.size());
}

int CImpState::SetSnStage(int portIndex, pdt_log_config_t& lg)
{
    int ret = ERROR_SUCCESS;
    char pData[512 * 8] = { 0 };


    UCHAR u08PhyIdx = CSparkSm3350Util::GetPhysicalIndex((UCHAR)portIndex);
    CSparkSm3350Util& sm3350 = CSparkSm3350Util::getInstance(u08PhyIdx);
    do
    {
        SetSnData(portIndex, pData);
        if ((ret = sm3350.UfsSetSrialNumberString(pData)) != ERROR_SUCCESS) break;
    } while (0);
    if (ret != ERROR_SUCCESS)
    {
        if (notifier_) notifier_->PostTaskProgress(portIndex, 0, ret, _T("SetSn Failed"));
        lg.error_code = ret;
        ZeroMemory(lg.state, sizeof(lg.state));
        strncpy_s(lg.state, _countof(lg.state), "SetSn Failed", _TRUNCATE);
    }
    return ret;
}

int CImpState::SetMdtStage(int portIndex, pdt_log_config_t& lg)
{
    int ret = ERROR_SUCCESS;
    char pData[512 * 8] = { 0 };


    UCHAR u08PhyIdx = CSparkSm3350Util::GetPhysicalIndex((UCHAR)portIndex);
    CSparkSm3350Util& sm3350 = CSparkSm3350Util::getInstance(u08PhyIdx);

    do
    {
        SetMdtData(pData);
        if ((ret = sm3350.UfsSetManuDate(pData)) != ERROR_SUCCESS) break;
    } while (0);
    if (ret != ERROR_SUCCESS)
    {
        if (notifier_) notifier_->PostTaskProgress(portIndex, 0, ret, _T("SetMdt Failed"));
        lg.error_code = ret;
        ZeroMemory(lg.state, sizeof(lg.state));
        strncpy_s(lg.state, _countof(lg.state), "SetMdt Failed", _TRUNCATE);
    }
    return ret;
}

int CImpState::VerifyIspStage(int portIndex, pdt_log_config_t& lg)
{
    int ret = ERROR_SUCCESS;
    char ispString[16] = { 0 };
    char pData[512 * 8] = { 0 };

    GetIspMark(ispString);

    UCHAR u08PhyIdx = CSparkSm3350Util::GetPhysicalIndex((UCHAR)portIndex);
    CSparkSm3350Util& sm3350 = CSparkSm3350Util::getInstance(u08PhyIdx);
    do
    {
        if ((ret = sm3350.UfsCheckIsp(pData)) != ERROR_SUCCESS) break;

        if (memcmp(ispString, pData, 12))
        {
            ret = ERR_ISP_VER_MISMATCH;
        }
    } while (0);
    if (ret != ERROR_SUCCESS)
    {
        if (notifier_) notifier_->PostTaskProgress(portIndex, 0, ret, _T("VerifyISP Failed"));
        lg.error_code = ret;
        ZeroMemory(lg.state, sizeof(lg.state));
        strncpy_s(lg.state, _countof(lg.state), "VerifyISP Failed", _TRUNCATE);
    }
    return ret;
}

int CImpState::WriteSramStage(int portIndex, pdt_log_config_t& lg)
{
    int ret = ERROR_SUCCESS;


    UCHAR u08PhyIdx = CSparkSm3350Util::GetPhysicalIndex((UCHAR)portIndex);
    CSparkSm3350Util& sm3350 = CSparkSm3350Util::getInstance(u08PhyIdx);
    do
    {
        if ((ret = sm3350.UfsWriteSramMp(g_UfsIsp, BYTE2SECTOR(sizeof(g_UfsIsp)))) != ERROR_SUCCESS) break;
    } while (0);
    if (ret != ERROR_SUCCESS)
    {
        if (notifier_) notifier_->PostTaskProgress(portIndex, 0, ret, _T("WriteSram Failed"));
        lg.error_code = ret;
        ZeroMemory(lg.state, sizeof(lg.state));
        strncpy_s(lg.state, _countof(lg.state), "WriteSram Failed", _TRUNCATE);
    }
    return ret;
}

int CImpState::VerifySram1Stage(int portIndex, pdt_log_config_t& lg)
{
    int ret = ERROR_SUCCESS;
    char pData[512 * 8] = { 0 };


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
        if (notifier_) notifier_->PostTaskProgress(portIndex, 0, ret, _T("VerifySram Failed"));
        lg.error_code = ret;
        ZeroMemory(lg.state, sizeof(lg.state));
        strncpy_s(lg.state, _countof(lg.state), "VerifySram Failed", _TRUNCATE);
    }
    return ret;
}

int CImpState::VerifySram2Stage(int portIndex, pdt_log_config_t& lg)
{
    int ret = ERROR_SUCCESS;
    char pData1[512 * 8] = { 0 };
    char pData2[512 * 8] = { 0 };


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
        if (notifier_) notifier_->PostTaskProgress(portIndex, 0, ret, _T("VerifySram Failed"));
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

int CImpState::VerifyCidStage(int portIndex, pdt_log_config_t& lg)
{
    int ret = ERROR_SUCCESS;
    char pData[512 * 0x03] = { 0 };
    CStringW strSn;
#define MNM_DATA_OFFSET 0x02
#define CAP_DATA_OFFSET (16 * 16 + 13)
#define MID_DATA_OFFSET (16 * 20 + 4)
#define PNM_DATA_OFFSET (16 * 44 + 6)
#define PSN_DATA_OFFSET (16 * 76 + 2)


    PUFS_OPTION pOpt = settings_ ? settings_->GetUfsOption() : nullptr;
    if (pOpt == nullptr)
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
            const SIZE_T mnmFieldChars = sizeof(pOpt->qcPrm.mnm);
            WCHAR mnmExpected[sizeof(pOpt->qcPrm.mnm)] = { 0 };
            const size_t mnmSrcLen = strnlen_s(pOpt->qcPrm.mnm, sizeof(pOpt->qcPrm.mnm));
            if (mnmSrcLen > 0)
            {
                if (!CPubFunc::CharToWChar(pOpt->qcPrm.mnm, (int)mnmSrcLen, mnmExpected, (int)mnmFieldChars))
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
            const ULONG n4KBCntD = pOpt->qcPrm.n4KBCnt;
            const ULONG n4KBCntS = _byteswap_ulong(capRaw);
            if (n4KBCntD != n4KBCntS)
            {
                ret = ERR_4KBCNT_MISMATCH;
                break;
            }

            //---------------------------------------------------------------------
            // MID
            //---------------------------------------------------------------------
            const SIZE_T midLen = sizeof(pOpt->qcPrm.mid);
            if (memcmp(pOpt->qcPrm.mid, pData + MID_DATA_OFFSET, midLen) != 0)
            {
                ret = ERR_MID_MISMATCH;
                break;
            }

            //---------------------------------------------------------------------
            // PNM: 
            //---------------------------------------------------------------------
            const SIZE_T pnmFieldChars = sizeof(pOpt->qcPrm.pnm);
            WCHAR pnmExpected[sizeof(pOpt->qcPrm.pnm)] = { 0 };
            const size_t pnmSrcLen = strnlen_s(pOpt->qcPrm.pnm, sizeof(pOpt->qcPrm.pnm));
            if (pnmSrcLen == 0)
            {
                ret = ERR_PNM_MISMATCH;
                break;
            }
            if (!CPubFunc::CharToWChar(pOpt->qcPrm.pnm, (int)pnmSrcLen, pnmExpected, (int)pnmFieldChars))
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
        if (notifier_) notifier_->PostTaskProgress(portIndex, 0, ret, _T("VerifyCid Failed"));
        lg.error_code = ret;
        ZeroMemory(lg.state, sizeof(lg.state));
        strncpy_s(lg.state, _countof(lg.state), "VerifyCid Failed", _TRUNCATE);
    }
    return ret;
}

int CImpState::VerifyGeometryStage(int portIndex, pdt_log_config_t& lg)
{
    int ret = ERROR_SUCCESS;
    char pData[512] = { 0 };



    if (!settings_) return ERR_INVALID_DATA;
    PUFS_OPTION pOpt = settings_->GetUfsOption();
    ULONG SectorCntStd = pOpt ? (pOpt->qcPrm.n4KBCnt * 4 * 2) : 0;

    UCHAR u08PhyIdx = CSparkSm3350Util::GetPhysicalIndex((UCHAR)portIndex);
    CSparkSm3350Util& sm3350 = CSparkSm3350Util::getInstance(u08PhyIdx);

    do
    {
        if ((ret = sm3350.UfsGetGeometry(pData)) != ERROR_SUCCESS) break;
        //Geometry Identifier
        if(0x57 != pData[0] || 0x07 != pData[1])
        {
            ret = ERR_GEOMETRY_MISMATCH;
            break;
		}
        //capacity
        DWORD capLaw = _byteswap_ulong(*(DWORD*)(pData + 0x04));
        DWORD capHigh = _byteswap_ulong(*(DWORD*)(pData + 0x08));
		if (SectorCntStd != capHigh)
        {
            ret = ERR_CAPACITY_MISMATCH;
            break;
        }
    } while (0);
    if (ret != ERROR_SUCCESS)
    {
        if (notifier_) notifier_->PostTaskProgress(portIndex, 0, ret, _T("VerifyGeometry Failed"));
        lg.error_code = ret;
        ZeroMemory(lg.state, sizeof(lg.state));
        strncpy_s(lg.state, _countof(lg.state), "VerifyGeometry Failed", _TRUNCATE);
    }
    return ret;
}

int CImpState::VerifySnStage(int portIndex, pdt_log_config_t& lg)
{
    int ret = ERROR_SUCCESS;
    char pData[512 * 0x03] = { 0 };
    CStringW strSn;
#define PSN_DATA_OFFSET (16 * 76)


    PUFS_OPTION pOpt = settings_ ? settings_->GetUfsOption() : nullptr;
    if (pOpt == nullptr)
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
            // SN 校验
            //---------------------------------------------------------------------

            for (int i = 0; i < 64; i += 2)
            {
                USHORT beValue = (pData[PSN_DATA_OFFSET + i] << 8) | pData[PSN_DATA_OFFSET + i + 1];
                WCHAR wch = _byteswap_ushort(beValue);

                if (wch == L'\0') break;
                strSn.AppendChar(wch);
            }
            if (portIndex < 0 || portIndex >= _countof(m_strwSn) || 0 != m_strwSn[portIndex].Compare(strSn))
            {
                ret = ERR_SN_MISMATCH;
                break;
            }
        } while (0);
    }

    if (ret != ERROR_SUCCESS)
    {
        if (notifier_) notifier_->PostTaskProgress(portIndex, 0, ret, _T("VerifySn Failed"));
        lg.error_code = ret;
        ZeroMemory(lg.state, sizeof(lg.state));
        strncpy_s(lg.state, _countof(lg.state), "VerifySn Failed", _TRUNCATE);
    }
    return ret;
}

void CImpState::UpdateIspMark(const char* ispBuf, int ispFileSize)
{
    spark::ufspdt::IspMarkCache::Instance().Update(ispBuf, ispFileSize);
}

void CImpState::SetCachedSnForPort(int portIndex, const CStringW& sn)
{
    if (portIndex < 0 || portIndex >= _countof(m_strwSn)) return;
    m_strwSn[portIndex] = sn;
}
