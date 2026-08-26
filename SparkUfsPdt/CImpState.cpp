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

    // Return a friendly stage name from a compiler function name like
    // "CImpState::PowerOffStage" -> "PowerOff" (remove class prefix and trailing "Stage").
    static CString StageNameFromFunction(const TCHAR* func)
    {
    CString s(func);
    // strip class qualifier if present (find last "::")
    int idx = s.ReverseFind(':');
    if (idx > 0 && idx - 1 >= 0 && s[idx - 1] == ':')
    {
        s = s.Mid(idx + 1);
    }
    // remove trailing "Stage" if present
    if (s.GetLength() > 5 && s.Right(5).CompareNoCase(_T("Stage")) == 0)
    {
        s = s.Left(s.GetLength() - 5);
    }

    // Split CamelCase / PascalCase into words: e.g., PowerOff -> Power Off
    CString out;
    bool newWord = true;
    for (int i = 0; i < s.GetLength(); ++i)
    {
        TCHAR ch = s[i];
        if (i > 0 && _istupper(ch) && !_istspace(s[i-1]) && !_istupper(s[i-1]))
        {
            out.AppendChar(' ');
            newWord = true;
        }
        if (newWord)
        {
            out.AppendChar(_totupper(ch));
            newWord = false;
        }
        else
        {
            out.AppendChar(_totlower(ch));
        }
    }
    out.Trim();
    return out.IsEmpty() ? s : out;
    }
}

// IspMark storage moved to IspMarkCache singleton

CImpState::CImpState(ISettingsProvider* settings, ILogger* logger, IUiNotifier* notifier)
    : settings_(settings), logger_(logger), notifier_(notifier)
{
}

// =============================================================================
// P1-2 root-cure centralized port-acquisition helper (moved here from header).
// See CImpState.h for contract / caller pattern.  Moved out-of-line because
// the in-class version dereferenced notifier_->PostTaskStatus() before
// IUiNotifier's full definition was visible → C2027 "use of undefined type
// IUiNotifier" on MBCS builds.  Keeping the out-of-line definition lets the
// header rely on a plain forward declaration of IUiNotifier (smaller header
// graph, no risk of circular include with IUiNotifier.h → CString / MFC).
// =============================================================================
spark::sm3350::CSparkSm3350Util* CImpState::GetSm3350OrInvalid(int portIndex, pdt_log_config_t& lg)
{
    using namespace spark::sm3350;
    PhyIndex phy = CSparkSm3350Util::ResolvePhyIndex(static_cast<UCHAR>(portIndex));
    if (!phy.IsValid()) {
        lg.error_code = ERROR_NO_SUCH_DEVICE;
        strncpy_s(lg.stage, _countof(lg.stage), "NoSuchPort", _TRUNCATE);
        if (notifier_) {
            CString msg;
            msg.Format(_T("No physical device for port %d"), portIndex + 1);
            notifier_->PostTaskStatus(portIndex, ERROR_NO_SUCH_DEVICE, msg);
        }
        return nullptr;
    }
    return &CSparkSm3350Util::getInstance(phy);
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

    // P2-10 fix (Plan A: strict failure, identical policy to convertLen==0 above):
    //
    // OLD buggy code was:
    //   if ((size_t)convertLen >= cDestLen) { memcpy(cDest, tempBuf.data(), cDestLen); /* return true! */ }
    //
    // TWO bugs in the old branch:
    //  (1) The function returned TRUE (claimed success), so the caller had no idea
    //      the output got truncated (SN / model strings / CID fields got mangled
    //      silently → production traceability lost).
    //  (2) memcpy() filled the ENTIRE cDestLen bytes with payload, leaving ZERO
    //      bytes for a trailing NUL and writing no terminator.  Any caller that
    //      treated cDest as a C string (strlen / strcmp / strcpy_s without
    //      explicit length, printf("%s"), etc.) read PAST cDest into uninitialised
    //      stack memory → OOB read + garbage in downstream logs & reports.
    //
    // NEW Plan A: treat "too small" exactly the same as "conversion failed (0 bytes):
    //  memset the output buffer to all zeros (safe empty C-string, guaranteed NULs
    //  even for callers that skip the bool return check) and return false so
    //  attentive callers get a clear signal.
    if (static_cast<size_t>(convertLen) >= cDestLen)
    {
        memset(cDest, 0, cDestLen);
        return false;
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

    spark::sm3350::CSparkSm3350Util* sm3350 = GetSm3350OrInvalid(portIndex, lg);
    if (!sm3350) return lg.error_code;
    do
    {
        if ((ret = sm3350->UfsPowerOff()) != ERROR_SUCCESS) break;
        Sleep(100);
    } while (0);

    if (ret != ERROR_SUCCESS)
    {
        if (notifier_)
        {
            CString stage = StageNameFromFunction(_T(__FUNCTION__));
            // Use centralized formatter for failure messages
            CString fmt = ::DialogAdapter::FormatFailureStatus(stage, ret);
            notifier_->PostTaskStatus(portIndex, ret, fmt);
        }
        lg.error_code = ret;
        ZeroMemory(lg.stage, sizeof(lg.stage));
        strncpy_s(lg.stage, _countof(lg.stage), "PowerOff Failed", _TRUNCATE);
    }
    return ret;
}

int CImpState::RebootStage(int portIndex, pdt_log_config_t& lg)
{
    int ret = ERROR_SUCCESS;


    spark::sm3350::CSparkSm3350Util* sm3350 = GetSm3350OrInvalid(portIndex, lg);
    if (!sm3350) return lg.error_code;
    //char pData[512 * 8] = { 0 };
    //DEBUG data
    //SetMdtData(pDlg, pData);
    //SetSnData(pDlg, portIndex, pData);
    do
    {
        if ((ret = sm3350->UfsPowerOff()) != ERROR_SUCCESS) break;
        Sleep(1000);
        if ((ret = sm3350->UfsPowerOn()) != ERROR_SUCCESS) break;
    } while (0);

    if (ret != ERROR_SUCCESS)
    {
        if (notifier_)
        {
            CString stage = StageNameFromFunction(_T(__FUNCTION__));
            CString fmt = ::DialogAdapter::FormatFailureStatus(stage, ret);
            notifier_->PostTaskStatus(portIndex, ret, fmt);
        }
        lg.error_code = ret;
        ZeroMemory(lg.stage, sizeof(lg.stage));
        strncpy_s(lg.stage, _countof(lg.stage), "Reboot Failed", _TRUNCATE);
    }
    return ret;
}

int CImpState::CardInitStage(int portIndex, pdt_log_config_t& lg)
{
    int ret = ERROR_SUCCESS;


    spark::sm3350::CSparkSm3350Util* sm3350 = GetSm3350OrInvalid(portIndex, lg);
    if (!sm3350) return lg.error_code;

    ret = sm3350->UfsCardInit();
    if (ret != ERROR_SUCCESS)
    {
        if (notifier_)
        {
            CString stage = StageNameFromFunction(_T(__FUNCTION__));
            CString fmt = ::DialogAdapter::FormatFailureStatus(stage, ret);
            notifier_->PostTaskStatus(portIndex, ret, fmt);
        }
        lg.error_code = ret;
        ZeroMemory(lg.stage, sizeof(lg.stage));
        strncpy_s(lg.stage, _countof(lg.stage), "CardInit Failed", _TRUNCATE);
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
        bForceRomMode = settings_->GetBaseSetting()->ForceRomMode;
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
    if (ret != ERROR_SUCCESS)
    {
        if (notifier_)
        {
            CString stage = StageNameFromFunction(_T(__FUNCTION__));
            CString fmt = ::DialogAdapter::FormatFailureStatus(stage, ret);
            notifier_->PostTaskStatus(portIndex, ret, fmt);
        }
    }
    Sleep(300);
    return ret;
}

int CImpState::UpiuForceRomStage(int portIndex, pdt_log_config_t& lg)
{
    int ret = ERROR_SUCCESS;

    spark::sm3350::CSparkSm3350Util* sm3350 = GetSm3350OrInvalid(portIndex, lg);
    if (!sm3350) return lg.error_code;
    if (notifier_)
    {
        notifier_->PostTaskStatus(portIndex, 0, StageNameFromFunction(_T(__FUNCTION__)));
    }

    ret = sm3350->UpiuForceRom();
    if (ret != ERROR_SUCCESS)
    {
        if (notifier_)
        {
            CString stage = StageNameFromFunction(_T(__FUNCTION__));
            CString fmt = ::DialogAdapter::FormatFailureStatus(stage, ret);
            notifier_->PostTaskStatus(portIndex, ret, fmt);
        }
        lg.error_code = ret;
        ZeroMemory(lg.stage, sizeof(lg.stage));
        strncpy_s(lg.stage, _countof(lg.stage), "UpiuForceRom Failed", _TRUNCATE);
    }
    return ret;
}

int CImpState::VccOffForceRomStage(int portIndex, pdt_log_config_t& lg)
{
    int ret = ERROR_SUCCESS;


    spark::sm3350::CSparkSm3350Util* sm3350 = GetSm3350OrInvalid(portIndex, lg);
    if (!sm3350) return lg.error_code;
    if (notifier_)
    {
        notifier_->PostTaskStatus(portIndex, 0, StageNameFromFunction(_T(__FUNCTION__)));
    }

    ret = sm3350->VccOffForceRom();
    if (ret != ERROR_SUCCESS)
    {
        if (notifier_)
        {
            CString status;
            status.Format(_T("%s"), StageNameFromFunction(_T(__FUNCTION__)));
            CString fmt = ::DialogAdapter::FormatFailureStatus(status, ret);
            notifier_->PostTaskStatus(portIndex, ret, fmt);
        }
        lg.error_code = ret;
        ZeroMemory(lg.stage, sizeof(lg.stage));
        strncpy_s(lg.stage, _countof(lg.stage), "VccOffForceRom Failed", _TRUNCATE);
    }
    return ret;
}

int CImpState::MpStartStage(int portIndex, pdt_log_config_t& lg)
{
    int ret = ERROR_SUCCESS;


    spark::sm3350::CSparkSm3350Util* sm3350 = GetSm3350OrInvalid(portIndex, lg);
    if (!sm3350) return lg.error_code;
    if (notifier_)
    {
        notifier_->PostTaskStatus(portIndex, 0, StageNameFromFunction(_T(__FUNCTION__)));
    }

    ret = sm3350->UfsMpStartMode();
    if (ret != ERROR_SUCCESS)
    {
        if (notifier_)
        {
            CString stage = StageNameFromFunction(_T(__FUNCTION__));
            CString fmt = ::DialogAdapter::FormatFailureStatus(stage, ret);
            notifier_->PostTaskStatus(portIndex, ret, fmt);
        }
        lg.error_code = ret;
        ZeroMemory(lg.stage, sizeof(lg.stage));
        strncpy_s(lg.stage, _countof(lg.stage), "MpStart Failed", _TRUNCATE);
    }
    return ret;
}

int CImpState::Write1024KIspMpStage(int portIndex, pdt_log_config_t& lg)
{
    int ret = ERROR_SUCCESS;
    PUFS_OPTION pOpt = settings_ ? settings_->GetUfsOption() : nullptr;
    BOOL bFuncOption = pOpt ? pOpt->mainPrm.funcSel : FALSE;

    spark::sm3350::CSparkSm3350Util* sm3350 = GetSm3350OrInvalid(portIndex, lg);
    if (!sm3350) return lg.error_code;

    ret = sm3350->UfsWrite1024KIspMp(g_UfsIsp, BYTE2SECTOR(sizeof(g_UfsIsp)), bFuncOption);
    if (ret != ERROR_SUCCESS)
    {
        if (notifier_)
        {
            CString stage = StageNameFromFunction(_T(__FUNCTION__));
            CString fmt = ::DialogAdapter::FormatFailureStatus(stage, ret);
            notifier_->PostTaskStatus(portIndex, ret, fmt);
        }
        lg.error_code = ret;
        ZeroMemory(lg.stage, sizeof(lg.stage));
        strncpy_s(lg.stage, _countof(lg.stage), "Write1024KIspMp Failed", _TRUNCATE);
    }
    return ret;
}

int CImpState::MpExitStage(int portIndex, pdt_log_config_t& lg)
{
    int ret = ERROR_SUCCESS;


    spark::sm3350::CSparkSm3350Util* sm3350 = GetSm3350OrInvalid(portIndex, lg);
    if (!sm3350) return lg.error_code;
    // Stage entry: progress/status will be reported by caller pipeline

    ret = sm3350->UfsMpExit();
    if (ret != ERROR_SUCCESS)
    {
        if (notifier_)
        {
            CString stage = StageNameFromFunction(_T(__FUNCTION__));
            CString fmt = ::DialogAdapter::FormatFailureStatus(stage, ret);
            notifier_->PostTaskStatus(portIndex, ret, fmt);
        }
        lg.error_code = ret;
        ZeroMemory(lg.stage, sizeof(lg.stage));
        strncpy_s(lg.stage, _countof(lg.stage), "MpExit Failed", _TRUNCATE);
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
    char meto1[4] = { 0 };
	char meto2[4] = { 0 };
    if (!settings_)
    {
        // settings provider missing, cannot format SN reliably
        return;
    }
    PUFS_OPTION pOpt = settings_->GetUfsOption();
    if (pOpt)
    {
        memcpy(meto1, pOpt->mainPrm.meto1, 4);
        memcpy(meto2, pOpt->mainPrm.meto2, 4);
    }


    SYSTEMTIME st;
    GetLocalTime(&st);
    wchar_t timeStr[9];
    swprintf_s(timeStr, L"%04d%02d%02d", st.wYear, st.wMonth, st.wDay);

    wchar_t psn[9] = {0};
    // CImpState::m_strwSn is CStringW[] (always UTF-16), so use CStringW here too.
    // On MBCS builds plain `CString' resolves to CStringA, and CStringA::GetString()
    // returns const char* → C2664 when passed to wcscpy_s(). Declaring CStringW
    // makes GetString() return PCWSTR unconditionally and matches the source type
    // exactly (no implicit MBCS conversion round-trip that could lose data).
    CStringW psnText;
    if (portIndex >= 0 && portIndex < _countof(m_strwSn))
    {
        psnText = m_strwSn[portIndex];
        if (8 == psnText.GetLength())
        {
            // P2-7 fix: direct wcscpy_s (known length = 8 + null, fits psn[9] exactly,
            //      and uses the native wchar_t width).
            wcscpy_s(psn, _countof(psn), psnText.GetString());
        }
    }

    auto formatted = spark::ufspdt::DataFormatter::FormatSnData(meto1, meto2, timeStr, psn);

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
        cachedSn = cachedSn.Mid(1, 18);
        SetCachedSnForPort(portIndex, cachedSn);
    }

    // UFS SN payload WCHAR fields must be written as big-endian byte order,
    // e.g. "20" -> 00 32 00 30.
    std::array<BYTE, 64> payloadToWrite = formatted;
    auto swapUtf16FieldToBe = [&payloadToWrite](size_t offset, size_t byteCount)
    {
        if (offset + byteCount > payloadToWrite.size()) return;
        for (size_t i = offset; i + 1 < offset + byteCount; i += 2)
        {
            std::swap(payloadToWrite[i], payloadToWrite[i + 1]);
        }
    };
    swapUtf16FieldToBe(2, 4);
    swapUtf16FieldToBe(6, 16);   // date field: WCHAR[8]
    swapUtf16FieldToBe(22, 16);  // psn field: WCHAR[8]

    memcpy(pData, payloadToWrite.data(), payloadToWrite.size());
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

int CImpState::WCharFieldCompare(const char* pField, const char* pSrcField, const int nSize)
{
    // ===== P0-1 最小改动 1/4: 前置参数校验 (nullptr / 零长) =====
    if (pField == nullptr || pSrcField == nullptr || nSize <= 0) {
        return -1;
    }
    int bRet = ERROR_SUCCESS;
    const SIZE_T mnmFieldChars = nSize;
    // P2-9 fix: replace raw new[]/delete[] + ZeroMemory with std::vector<WCHAR>.
    // vector(size_t) value-initialises each WCHAR to 0 — equivalent to the
    // removed ZeroMemory.  Any C++ exception thrown by CPubFunc::CharToWChar
    // (or any future helper added between here and the return) will run the
    // vector destructor during unwinding and free the buffer → no leak, ever.
    std::vector<WCHAR> Expected(nSize);
    const size_t DestLen = strnlen_s(pField, nSize);
    if (DestLen > 0)
    {
        if (!CPubFunc::CharToWChar(pField, (int)DestLen, Expected.data(), (int)mnmFieldChars))
        {
            bRet = -1;
            // ===== P0-1 最小改动 2/4: 编码失败立即 return, 避免 bRet=-1 被循环覆盖 =====
            // (delete[] Expected removed — vector destructor runs automatically on return)
            return bRet;
        }
    }
    //const WCHAR* pSrcData = (const WCHAR*)(pSrcField);
    // ===== P0-1 最小改动 3/4: 去掉 *2, i 是"第几个字符"不是字节数（避免 Expected[] 越界读） =====
    for (size_t i = 0; i < DestLen; i++)
    {
        WCHAR beChar = *(const WCHAR*)(pSrcField+i*2);
        WCHAR leChar = _byteswap_ushort((USHORT)beChar); // 大端 → 小端
        if (leChar != Expected[i])
        {
            // ===== P0-1 最小改动 4/4: 显式强转 int 避免 unsigned short wraparound =====
			bRet = (int)leChar - (int)Expected[i];
            break;
        }
    }

    return bRet;
}

BOOL CImpState::IsValidUid(char* pUID, int nUidSize, char* pValidUidBuff)
{
    // 输入基础校验：pUID不能为空、长度必须512；存结果的缓冲区非空才拷贝
    if (pUID == nullptr || nUidSize != 512)
    {
        return FALSE;
    }

    const int GROUP_COUNT = 2;
    const int GROUP_SIZE = 32;
    const int UID_PART = 16; // 有效UID占16字节

    for (int group = 0; group < GROUP_COUNT; group++)
    {
        char* pGroup = pUID + group * GROUP_SIZE;
        char* pRaw = pGroup;
        char* pCheck = pGroup + UID_PART;

        bool bCurGroupOk = true;
        for (int i = 0; i < UID_PART; i++)
        {
            BYTE rawByte = (BYTE)pRaw[i];
            BYTE checkByte = (BYTE)pCheck[i];
            if ((rawByte ^ checkByte) != 0xFF)
            {
                bCurGroupOk = false;
                break;
            }
        }

        // 当前分组校验成功
        if (bCurGroupOk)
        {
            // 外部传入缓冲区有效，则把匹配成功的16字节UID复制出去
            if (pValidUidBuff != nullptr)
            {
                memcpy(pValidUidBuff, pRaw, UID_PART);
            }
            return TRUE;
        }
    }

    // 所有分组全部无效
    return FALSE;
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


    spark::sm3350::CSparkSm3350Util* sm3350 = GetSm3350OrInvalid(portIndex, lg);
    if (!sm3350) return lg.error_code;
    do
    {
        SetSnData(portIndex, pData);
        if ((ret = sm3350->UfsSetSrialNumberString(pData)) != ERROR_SUCCESS) break;
    } while (0);
    if (ret != ERROR_SUCCESS)
    {
        if (notifier_)
        {
            CString stage = StageNameFromFunction(_T(__FUNCTION__));
            CString fmt = ::DialogAdapter::FormatFailureStatus(stage, ret);
            notifier_->PostTaskStatus(portIndex, ret, fmt);
        }
        lg.error_code = ret;
        ZeroMemory(lg.stage, sizeof(lg.stage));
        strncpy_s(lg.stage, _countof(lg.stage), "SetSn Failed", _TRUNCATE);
    }
    return ret;
}

int CImpState::SetMdtStage(int portIndex, pdt_log_config_t& lg)
{
    int ret = ERROR_SUCCESS;
    char pData[512 * 8] = { 0 };


    spark::sm3350::CSparkSm3350Util* sm3350 = GetSm3350OrInvalid(portIndex, lg);
    if (!sm3350) return lg.error_code;

    do
    {
        SetMdtData(pData);
        if ((ret = sm3350->UfsSetManuDate(pData)) != ERROR_SUCCESS) break;
    } while (0);
    if (ret != ERROR_SUCCESS)
    {
        if (notifier_)
        {
            CString stage = StageNameFromFunction(_T(__FUNCTION__));
            CString fmt = ::DialogAdapter::FormatFailureStatus(stage, ret);
            notifier_->PostTaskStatus(portIndex, ret, fmt);
        }
        lg.error_code = ret;
        ZeroMemory(lg.stage, sizeof(lg.stage));
        strncpy_s(lg.stage, _countof(lg.stage), "SetMdt Failed", _TRUNCATE);
    }
    return ret;
}

int CImpState::VerifyIspStage(int portIndex, pdt_log_config_t& lg)
{
    int ret = ERROR_SUCCESS;
    char ispString[16] = { 0 };
    char pData[512 * 8] = { 0 };

    GetIspMark(ispString);

    spark::sm3350::CSparkSm3350Util* sm3350 = GetSm3350OrInvalid(portIndex, lg);
    if (!sm3350) return lg.error_code;
    do
    {
        if ((ret = sm3350->UfsCheckIsp(pData)) != ERROR_SUCCESS) break;

        if (memcmp(ispString, pData, 12))
        {
            ret = ERR_ISP_VER_MISMATCH;
        }
    } while (0);
    if (ret != ERROR_SUCCESS)
    {
        if (notifier_)
        {
            CString stage = StageNameFromFunction(_T(__FUNCTION__));
            CString fmt = ::DialogAdapter::FormatFailureStatus(stage, ret);
            notifier_->PostTaskStatus(portIndex, ret, fmt);
        }
        lg.error_code = ret;
        ZeroMemory(lg.stage, sizeof(lg.stage));
        strncpy_s(lg.stage, _countof(lg.stage), "VerifyISP Failed", _TRUNCATE);
    }
    return ret;
}

int CImpState::VerifyQcIspStage(int portIndex, pdt_log_config_t& lg)
{
    int ret = ERROR_SUCCESS;
    char ispString[16] = { 0 };
    char pData[512 * 8] = { 0 };
    spark::sm3350::CSparkSm3350Util* sm3350 = GetSm3350OrInvalid(portIndex, lg);
    if (!sm3350) return lg.error_code;
    do
    {
        GetQCIspString(ispString);
        if ((ret = sm3350->UfsCheckIsp(pData)) != ERROR_SUCCESS) break;
        if (notifier_)
        {
            INT32 temps[4] = {};
            memcpy(temps, pData + 12, sizeof(temps));

            CString tempText;
            tempText.Format(_T("%d/%d/%d/%d"),
                temps[0],
                temps[1],
                temps[2],
                temps[3]);
            notifier_->PostPortTemp(portIndex, tempText);
        }
        if (memcmp(ispString, pData, 8))
        {
            ret = ERR_ISP_VER_MISMATCH;
        }
        
    } while (0);
    if (ret != ERROR_SUCCESS)
    {
        if (notifier_)
        {
            CString stage = StageNameFromFunction(_T(__FUNCTION__));
            CString fmt = ::DialogAdapter::FormatFailureStatus(stage, ret);
            notifier_->PostTaskStatus(portIndex, ret, fmt);
        }
        lg.error_code = ret;
        ZeroMemory(lg.stage, sizeof(lg.stage));
        strncpy_s(lg.stage, _countof(lg.stage), "VerifyQcISP Failed", _TRUNCATE);
    }
    return ret;
}

int CImpState::WriteSramStage(int portIndex, pdt_log_config_t& lg)
{
    int ret = ERROR_SUCCESS;


    spark::sm3350::CSparkSm3350Util* sm3350 = GetSm3350OrInvalid(portIndex, lg);
    if (!sm3350) return lg.error_code;
    do
    {
        if ((ret = sm3350->UfsWriteSramMp(g_UfsIsp, BYTE2SECTOR(sizeof(g_UfsIsp)))) != ERROR_SUCCESS) break;
    } while (0);
    if (ret != ERROR_SUCCESS)
    {
        if (notifier_)
        {
            CString stage = StageNameFromFunction(_T(__FUNCTION__));
            CString fmt = ::DialogAdapter::FormatFailureStatus(stage, ret);
            notifier_->PostTaskStatus(portIndex, ret, fmt);
        }
        lg.error_code = ret;
        ZeroMemory(lg.stage, sizeof(lg.stage));
        strncpy_s(lg.stage, _countof(lg.stage), "WriteSram Failed", _TRUNCATE);
    }
    return ret;
}

int CImpState::VerifySram1Stage(int portIndex, pdt_log_config_t& lg)
{
    int ret = ERROR_SUCCESS;
    char pData[512 * 8] = { 0 };


    spark::sm3350::CSparkSm3350Util* sm3350 = GetSm3350OrInvalid(portIndex, lg);
    if (!sm3350) return lg.error_code;
    do
    {
        if ((ret = sm3350->UfsReadSramResult(pData, 8)) != ERROR_SUCCESS) break;

        if (!(pData[0] == 0x00 && pData[1] == 0x00 && pData[2] == 0x00 && pData[3] == 0x00))
        {
            ret = ERR_SRAM1_TEST_FAILED;
        }
    } while (0);
    if (ret != ERROR_SUCCESS)
    {
        if (notifier_)
        {
            CString stage = StageNameFromFunction(_T(__FUNCTION__));
            CString fmt = ::DialogAdapter::FormatFailureStatus(stage, ret);
            notifier_->PostTaskStatus(portIndex, ret, fmt);
        }
        lg.error_code = ret;
        ZeroMemory(lg.stage, sizeof(lg.stage));
        strncpy_s(lg.stage, _countof(lg.stage), "VerifySram Failed", _TRUNCATE);
    }
    return ret;
}

int CImpState::VerifySram2Stage(int portIndex, pdt_log_config_t& lg)
{
    int ret = ERROR_SUCCESS;
    char pData1[512 * 8] = { 0 };
    char pData2[512 * 8] = { 0 };


    spark::sm3350::CSparkSm3350Util* sm3350 = GetSm3350OrInvalid(portIndex, lg);
    if (!sm3350) return lg.error_code;
    do
    {
        if ((ret = sm3350->UfsCheckSram2(pData1, pData2)) != ERROR_SUCCESS) break;

        if (!(pData2[0] == 0x00 && pData2[1] == 0x00 && pData2[2] == 0x00 && pData2[3] == 0x00))
        {
            ret = ERR_SRAM2_TEST_FAILED;
        }
    } while (0);
    if (ret != ERROR_SUCCESS)
    {
        if (notifier_)
        {
            CString stage = StageNameFromFunction(_T(__FUNCTION__));
            CString fmt = ::DialogAdapter::FormatFailureStatus(stage, ret);
            notifier_->PostTaskStatus(portIndex, ret, fmt);
        }
        lg.error_code = ret;
        ZeroMemory(lg.stage, sizeof(lg.stage));
        strncpy_s(lg.stage, _countof(lg.stage), "VerifySram Failed", _TRUNCATE);
    }
    return ret;
}

int CImpState::ReadCidStage(int portIndex, pdt_log_config_t& lg)
{
    int ret = ERROR_SUCCESS;
    char pData[512 * 0x03] = { 0 };
    CStringW strSn;
    // Offsets within CID data block
    constexpr size_t MNM_DATA_OFFSET = 0x02;
    constexpr size_t CAP_DATA_OFFSET = (16 * 16 + 13);
    constexpr size_t MID_DATA_OFFSET = (16 * 20 + 4);
    constexpr size_t PNM_DATA_OFFSET = (16 * 44 + 6);
    constexpr size_t PSN_DATA_OFFSET = (16 * 76 + 2);
    constexpr size_t MDT_DATA_OFFSET = (16 * 76 + 9);
    constexpr size_t PRV_DATA_OFFSET = (16 * 92);

    PUFS_OPTION pOpt = settings_ ? settings_->GetUfsOption() : nullptr;
    // P2-2 fix: use centralized PhyIndex resolution (same as the other 24 stages).
    // Previously this stage called GetPhysicalIndex(UCHAR) + getInstance(UCHAR)
    // directly, bypassing the PhyIndex strong type and range checks, so a
    // mapping miss (return 0xFF) caused an OOB reference bind and member crash.
    spark::sm3350::CSparkSm3350Util* sm3350 = GetSm3350OrInvalid(portIndex, lg);
    if (pOpt == nullptr || !sm3350)
    {
        ret = pOpt ? ERROR_NO_SUCH_DEVICE : ERROR_INVALID_PARAMETER;
        // NOTE: if !sm3350 then GetSm3350OrInvalid has already set lg.error_code,
        // lg.stage="NoSuchPort" and posted the UI failure notification.
    }
    else
    {
        do
        {
			if ((ret = sm3350->UfsReadCidInfo(pData, BYTE2SECTOR(sizeof(pData)))) != ERROR_SUCCESS) break;

			auto WCharToCharFromBeField = [&](size_t offset, size_t wcharCount, char* dest, size_t destLen)
			{
				if (dest == nullptr || destLen == 0 || offset + wcharCount * sizeof(WCHAR) > sizeof(pData))
				{
					if (dest != nullptr && destLen > 0)
					{
						ZeroMemory(dest, destLen);
					}
					return;
				}

				std::vector<WCHAR> leChars(wcharCount, 0);
				for (size_t i = 0; i < wcharCount; ++i)
				{
					USHORT beValue = 0;
					memcpy(&beValue, pData + offset + i * sizeof(WCHAR), sizeof(beValue));
					leChars[i] = static_cast<WCHAR>(_byteswap_ushort(beValue));
				}

				CPubFunc::WCharToChar(leChars.data(), wcharCount, dest, destLen, CP_UTF8);
			};

			WCharToCharFromBeField(MNM_DATA_OFFSET, sizeof(pOpt->mainPrm.mnm), lg.manufacturer, sizeof(lg.manufacturer));

			char midBuffer[sizeof(pOpt->mainPrm.mid)] = { 0 };
			CString strMid = CPubFunc::BytesToHex((const BYTE*)pData + MID_DATA_OFFSET, sizeof(midBuffer));
			memcpy(lg.mid, strMid.GetString(), strMid.GetLength());

			WCharToCharFromBeField(PNM_DATA_OFFSET, sizeof(pOpt->mainPrm.pnm), lg.product_name, sizeof(lg.product_name));
			ZeroMemory(lg.serial_number, sizeof(lg.serial_number));
			WCharToCharFromBeField(PSN_DATA_OFFSET, 18, lg.serial_number, sizeof(lg.serial_number));
            if (notifier_)
            {
                CString snText(lg.serial_number);
                notifier_->PostPortSerial(portIndex, snText);
            }
			WCharToCharFromBeField(PRV_DATA_OFFSET, 4, lg.prv, sizeof(lg.prv));
        } while (0);
    }
    if (ret != ERROR_SUCCESS)
    {
        if (notifier_)
        {
            CString stage = StageNameFromFunction(_T(__FUNCTION__));
            CString fmt = ::DialogAdapter::FormatFailureStatus(stage, ret);
            notifier_->PostTaskStatus(portIndex, ret, fmt);
        }
        lg.error_code = ret;
        ZeroMemory(lg.stage, sizeof(lg.stage));
        strncpy_s(lg.stage, _countof(lg.stage), "VerifyCid Failed", _TRUNCATE);
    }
    return ret;
}

int CImpState::VerifyCidStage(int portIndex, pdt_log_config_t& lg)
{
    int ret = ERROR_SUCCESS;
    char pData[512 * 0x03] = { 0 };
    CStringW strSn;
    // Offsets within CID data block
    constexpr size_t MNM_DATA_OFFSET = 0x02;
    constexpr size_t CAP_DATA_OFFSET = (16 * 16 + 13);
    constexpr size_t MID_DATA_OFFSET = (16 * 20 + 4);
    constexpr size_t PNM_DATA_OFFSET = (16 * 44 + 6);
    constexpr size_t PSN_DATA_OFFSET = (16 * 76 + 2);
    constexpr size_t MDT_DATA_OFFSET = (16 * 76 + 10);
    constexpr size_t PRV_DATA_OFFSET = (16 * 92);

    PUFS_OPTION pOpt = settings_ ? settings_->GetUfsOption() : nullptr;
    // P2-2 fix (VerifyCidStage): same centralized resolver as the other stages.
    spark::sm3350::CSparkSm3350Util* sm3350 = GetSm3350OrInvalid(portIndex, lg);
    if (pOpt == nullptr || !sm3350)
    {
        ret = pOpt ? ERROR_NO_SUCH_DEVICE : ERROR_INVALID_PARAMETER;
    }
    else
    {
        do
        {
            if ((ret = sm3350->UfsReadCidInfo(pData, BYTE2SECTOR(sizeof(pData)))) != ERROR_SUCCESS) break;

            //---------------------------------------------------------------------
            // MNM: 修复大端 WCHAR 比较
            //---------------------------------------------------------------------
            if (pOpt->qcPrm.bCheckMnm)
            {
                if (WCharFieldCompare(pOpt->qcPrm.mnm, pData + MNM_DATA_OFFSET, sizeof(pOpt->qcPrm.mnm)) != ERROR_SUCCESS)
                {
                    ret = ERR_MNM_MISMATCH;
                    break;
                }
            }
            

            //---------------------------------------------------------------------
            // 4KB Count 
            //---------------------------------------------------------------------
            if (pOpt->qcPrm.bCheckDiskInfo)
            {
                ULONG capRaw = 0;
                memcpy(&capRaw, pData + CAP_DATA_OFFSET, sizeof(capRaw));
                const ULONG n4KBCntD = pOpt->qcPrm.n4KBCnt;
                const ULONG n4KBCntS = _byteswap_ulong(capRaw);
                if (n4KBCntD != n4KBCntS)
                {
                    ret = ERR_4KBCNT_MISMATCH;
                    break;
                }
            }
            //---------------------------------------------------------------------
            // MID
            //---------------------------------------------------------------------
            if (pOpt->qcPrm.bCheckMidOid)
            {
                const SIZE_T midLen = sizeof(pOpt->qcPrm.mid);
                if (memcmp(pOpt->qcPrm.mid, pData + MID_DATA_OFFSET, midLen) != 0)
                {
                    ret = ERR_MID_MISMATCH;
                    break;
                }
            }
            //---------------------------------------------------------------------
            // PNM: 
            //---------------------------------------------------------------------
            if (pOpt->qcPrm.bCheckPnm)
            {
                if (WCharFieldCompare(pOpt->qcPrm.pnm, pData + PNM_DATA_OFFSET, sizeof(pOpt->qcPrm.pnm)) != ERROR_SUCCESS)
                {
                    ret = ERR_PNM_MISMATCH;
                    break;
                }
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

            //---------------------------------------------------------------------
            // MDT: 
            //---------------------------------------------------------------------
            if (pOpt->qcPrm.bCheckMdt)
            {
                if (WCharFieldCompare(pOpt->qcPrm.mdt, pData + MDT_DATA_OFFSET, sizeof(pOpt->qcPrm.mdt)) != ERROR_SUCCESS)
                {
                    ret = ERR_MDT_MISMATCH;
                    break;
                }
            }
            //---------------------------------------------------------------------
            // PRV: 
            //---------------------------------------------------------------------
            if (pOpt->qcPrm.bCheckPrv)
            {
                if (WCharFieldCompare(pOpt->qcPrm.prv, pData + PRV_DATA_OFFSET, sizeof(pOpt->qcPrm.prv)) != ERROR_SUCCESS)
                {
                    ret = ERR_PRV_MISMATCH;
                    break;
                }
            }
        } while (0);
    }
    if (ret != ERROR_SUCCESS)
    {
        if (notifier_)
        {
            CString stage = StageNameFromFunction(_T(__FUNCTION__));
            CString fmt = ::DialogAdapter::FormatFailureStatus(stage, ret);
            notifier_->PostTaskStatus(portIndex, ret, fmt);
        }
        lg.error_code = ret;
        ZeroMemory(lg.stage, sizeof(lg.stage));
        strncpy_s(lg.stage, _countof(lg.stage), "VerifyCid Failed", _TRUNCATE);
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

    spark::sm3350::CSparkSm3350Util* sm3350 = GetSm3350OrInvalid(portIndex, lg);
    if (!sm3350) return lg.error_code;

    do
    {
        if ((ret = sm3350->UfsGetGeometry(pData)) != ERROR_SUCCESS) break;
        //Geometry Identifier
        if(0x57 != pData[0] || 0x07 != pData[1])
        {
            ret = ERR_GEOMETRY_MISMATCH;
            break;
		}
        //capacity
        if (pOpt->qcPrm.bCheckDiskInfo)
        {
            DWORD capLaw = _byteswap_ulong(*(DWORD*)(pData + 0x04));
            DWORD capHigh = _byteswap_ulong(*(DWORD*)(pData + 0x08));
            if (SectorCntStd != capHigh)
            {
                ret = ERR_CAPACITY_MISMATCH;
                break;
            }
        }
        
    } while (0);
    if (ret != ERROR_SUCCESS)
    {
        if (notifier_)
        {
            CString stage = StageNameFromFunction(_T(__FUNCTION__));
            CString fmt = ::DialogAdapter::FormatFailureStatus(stage, ret);
            notifier_->PostTaskStatus(portIndex, ret, fmt);
        }
        lg.error_code = ret;
        ZeroMemory(lg.stage, sizeof(lg.stage));
        strncpy_s(lg.stage, _countof(lg.stage), "VerifyGeometry Failed", _TRUNCATE);
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
    // P2-2 fix (VerifySnStage): centralized PhyIndex resolver + null check.
    spark::sm3350::CSparkSm3350Util* sm3350 = GetSm3350OrInvalid(portIndex, lg);
    if (pOpt == nullptr || !sm3350)
    {
        ret = pOpt ? ERROR_NO_SUCH_DEVICE : ERROR_INVALID_PARAMETER;
    }
    else
    {
        do
        {
            if ((ret = sm3350->UfsReadCidInfo(pData, BYTE2SECTOR(sizeof(pData)))) != ERROR_SUCCESS) break;
            //---------------------------------------------------------------------
            // SN 校验
            //---------------------------------------------------------------------
            char* pSn = pData + PSN_DATA_OFFSET+2;
            for (int i = 0; i < 36; i += 2)
            {
                USHORT beValue = (pSn[i] << 8) | pSn[i + 1];
                WCHAR wch = beValue;
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
        if (notifier_)
        {
            CString stage = StageNameFromFunction(_T(__FUNCTION__));
            CString fmt = ::DialogAdapter::FormatFailureStatus(stage, ret);
            notifier_->PostTaskStatus(portIndex, ret, fmt);
        }
        lg.error_code = ret;
        ZeroMemory(lg.stage, sizeof(lg.stage));
        strncpy_s(lg.stage, _countof(lg.stage), "VerifySn Failed", _TRUNCATE);
    }
    return ret;
}

int CImpState::VerifyPrvStage(int portIndex, pdt_log_config_t& lg)
{
    int ret = ERROR_SUCCESS;

    PUFS_OPTION pOpt = settings_ ? settings_->GetUfsOption() : nullptr;
    spark::sm3350::CSparkSm3350Util* sm3350 = GetSm3350OrInvalid(portIndex, lg);
    if (!sm3350) return lg.error_code;
    char pData[512] = { 0 };

    do
    {
        if ((ret = sm3350->UFSReadPRV(pData)) != ERROR_SUCCESS) break;
        if (pOpt->qcPrm.bCheckPrv)
        {
            const SIZE_T prvFieldChars = sizeof(pOpt->qcPrm.prv);
            WCHAR prvExpected[sizeof(pOpt->qcPrm.prv)] = { 0 };
            const size_t prvSrcLen = strnlen_s(pOpt->qcPrm.prv, sizeof(pOpt->qcPrm.prv));
            if (prvSrcLen == 0)
            {
                ret = ERR_PRV_MISMATCH;
                break;
            }
            if (!CPubFunc::CharToWChar(pOpt->qcPrm.prv, (int)prvSrcLen, prvExpected, (int)prvFieldChars))
            {
                ret = ERR_PRV_MISMATCH;
                break;
            }

            // pData 是大端 WCHAR，逐个字节反转再比较
            bool prvMatch = true;
            const WCHAR* pPrvData = (const WCHAR*)(pData+2);
            for (size_t i = 0; i < prvSrcLen; i++)
            {
                WCHAR beChar = pPrvData[i];
                WCHAR leChar = _byteswap_ushort((USHORT)beChar);
                if (leChar != prvExpected[i])
                {
                    prvMatch = false;
                    break;
                }
            }
            if (!prvMatch)
            {
                ret = ERR_PRV_MISMATCH;
                break;
            }
        }

    } while (0);

    if (ret != ERROR_SUCCESS)
    {
        if (notifier_)
        {
            CString stage = StageNameFromFunction(_T(__FUNCTION__));
            CString fmt = ::DialogAdapter::FormatFailureStatus(stage, ret);
            notifier_->PostTaskStatus(portIndex, ret, fmt);
        }
        lg.error_code = ret;
        ZeroMemory(lg.stage, sizeof(lg.stage));
        strncpy_s(lg.stage, _countof(lg.stage), "VerifyPrv Failed", _TRUNCATE);
    }
    return ret;
}

int CImpState::VerifyUIDStage(int portIndex, pdt_log_config_t& lg)
{
    int ret = ERROR_SUCCESS;
    char pData[512 * 4] = { 0 };
    const size_t ASICIDOffset = 0;
    const size_t FlashIDOffset = 512;
    const size_t UniqueIDOffset = 1024;
    spark::sm3350::CSparkSm3350Util* sm3350 = GetSm3350OrInvalid(portIndex, lg);
    if (!sm3350) return lg.error_code;
    do
    {
        if ((ret = sm3350->UFSReadID(pData)) != ERROR_SUCCESS) break;
        if (!IsValidUid(pData + UniqueIDOffset, 512, lg.UID))
        {
            //show invalid UID
            memcpy(lg.UID, pData + UniqueIDOffset, 16);
            ret = ERR_INVALID_UID;
        }
    } while (0);
    if (ret != ERROR_SUCCESS)
    {
        if (notifier_)
        {
            CString stage = StageNameFromFunction(_T(__FUNCTION__));
            CString fmt = ::DialogAdapter::FormatFailureStatus(stage, ret);
            notifier_->PostTaskStatus(portIndex, ret, fmt);
        }
        lg.error_code = ret;
        ZeroMemory(lg.stage, sizeof(lg.stage));
        strncpy_s(lg.stage, _countof(lg.stage), "VerifyUID Failed", _TRUNCATE);
    }
    return ret;
}

int CImpState::ReadAgingStage(int portIndex, pdt_log_config_t& lg)
{
    int ret = ERROR_SUCCESS;
    char ispString[16] = { 0 };
    char* pData = new char[512 * 48 * 2];
	ZeroMemory(pData, 512 * 48 * 2);

    spark::sm3350::CSparkSm3350Util* sm3350 = GetSm3350OrInvalid(portIndex, lg);
    if (!sm3350) return lg.error_code;
    do
    {
        if ((ret = sm3350->UfsReadAging(pData, 48 * 2)) != ERROR_SUCCESS) break;

        char modulePath[MAX_PATH] = { 0 };
        if (GetModuleFileNameA(nullptr, modulePath, _countof(modulePath)) > 0)
        {
            char* slash = strrchr(modulePath, '\\');
            if (slash)
            {
                *(slash + 1) = '\0';
            }

            char outPath[MAX_PATH] = { 0 };
            _snprintf_s(outPath, _countof(outPath), _TRUNCATE, "%sPort%d_aging.bin", modulePath, portIndex+1);
            spark::file::fnWriteFile(outPath, pData, 512 * 48 * 2);
        }

    } while (0);
    if (ret != ERROR_SUCCESS)
    {
        if (notifier_)
        {
            CString stage = StageNameFromFunction(_T(__FUNCTION__));
            CString fmt = ::DialogAdapter::FormatFailureStatus(stage, ret);
            notifier_->PostTaskStatus(portIndex, ret, fmt);
        }
        lg.error_code = ret;
        ZeroMemory(lg.stage, sizeof(lg.stage));
        strncpy_s(lg.stage, _countof(lg.stage), "VerifyISP Failed", _TRUNCATE);
    }
	delete[] pData;
    return ret;
}

int CImpState::Q100ReadStage(int portIndex, pdt_log_config_t& lg)
{
    int ret = ERROR_SUCCESS;
    char* pData = new char[512 * 0x14];
    spark::sm3350::CSparkSm3350Util* sm3350 = GetSm3350OrInvalid(portIndex, lg);
    if (!sm3350) return lg.error_code;
    do
    {
        if ((ret = sm3350->UfsReadQ100(pData)) != ERROR_SUCCESS) break;


    } while (0);
    if (ret != ERROR_SUCCESS)
    {
        if (notifier_)
        {
            CString stage = StageNameFromFunction(_T(__FUNCTION__));
            CString fmt = ::DialogAdapter::FormatFailureStatus(stage, ret);
            notifier_->PostTaskStatus(portIndex, ret, fmt);
        }
        lg.error_code = ret;
        ZeroMemory(lg.stage, sizeof(lg.stage));
        strncpy_s(lg.stage, _countof(lg.stage), "Q100Read Failed", _TRUNCATE);
    }
    delete[] pData;
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
