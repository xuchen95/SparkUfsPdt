#include "pch.h"
#include "PubFunc.h"
#include "CDialogBase.h"
#include "DialogAdapter.h"
#include "SettingsService.h"
#include <cerrno>
#include <vector>   // P2-9 fix: std::vector for exception-safe temp buffers in WCharToChar / CharToWChar

CString CPubFunc::IntToHex(UINT nValue, BOOL bUppercase, int nDigits)
{
    CString strFormat;
    if (nDigits > 0)
    {
        strFormat.Format(bUppercase ? _T("%0*X") : _T("%0*x"), nDigits, nValue);
    }
    else
    {
        strFormat.Format(bUppercase ? _T("%X") : _T("%x"), nValue);
    }
    return strFormat;
}

CString CPubFunc::BytesToHex(const BYTE* pData, int nLen, BOOL bUppercase)
{
    CString strHex;
    for (int i = 0; i < nLen; i++)
    {
        strHex.AppendFormat(bUppercase ? _T("%02X") : _T("%02x"), pData[i]);
    }
    return strHex;
}

int CPubFunc::HexToBytes(LPCTSTR lpszHex, BYTE* pOut, int nOutMaxLen)
{
    if (!lpszHex || !pOut || nOutMaxLen <= 0)
        return 0;

    CString strHex = lpszHex;
    strHex.Remove(_T(' '));
    strHex.Remove(_T('-'));
    strHex.Remove(_T('_'));

    int nLen = strHex.GetLength() / 2;
    if (nLen > nOutMaxLen)
        nLen = nOutMaxLen;

    for (int i = 0; i < nLen; i++)
    {
        TCHAR chHigh = strHex[2 * i];
        TCHAR chLow = strHex[2 * i + 1];

        BYTE high = (chHigh >= _T('0') && chHigh <= _T('9')) ? (chHigh - _T('0')) :
            (chHigh >= _T('A') && chHigh <= _T('F')) ? (10 + chHigh - _T('A')) :
            (10 + chHigh - _T('a'));

        BYTE low = (chLow >= _T('0') && chLow <= _T('9')) ? (chLow - _T('0')) :
            (chLow >= _T('A') && chLow <= _T('F')) ? (10 + chLow - _T('A')) :
            (10 + chLow - _T('a'));

        pOut[i] = (high << 4) | low;
    }
    return nLen;
}

bool CPubFunc::WCharToChar(const WCHAR* wSrc, size_t wSrcLen, char* cDest, size_t cDestLen, UINT codePage)
{
    if (wSrc == nullptr || cDest == nullptr || wSrcLen == 0 || cDestLen == 0)
    {
        return false;
    }

    // P2-9 fix: replace raw new[]/delete[] pair with std::vector<char> for
    // 100% exception safety.  vector(size_t) value-initialises every char to
    // 0, so the explicit memset(0) is no longer needed.  If any C++ exception
    // is thrown later (WideCharToMultiByte helper wrappers, future /permissive
    // STL code that throws, std::bad_alloc on extreme resizing, etc.), the
    // vector destructor runs during stack unwinding and frees the buffer —
    // no leak possible.
    const size_t tempBufSize = wSrcLen * 3;
    std::vector<char> tempBuf(tempBufSize);

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

bool CPubFunc::CharToWChar(const char* cSrc, size_t cSrcLen, WCHAR* wDest, size_t wDestLen, UINT codePage)
{
    if (cSrc == nullptr || wDest == nullptr || cSrcLen == 0 || wDestLen == 0)
    {
        return false;
    }

    // P2-9 fix: replace raw new[]/delete[] pair with std::vector<WCHAR> for
    // 100% exception safety (see WCharToChar above for rationale).  Each
    // WCHAR is value-initialised to 0, so ZeroMemory is no longer needed.
    const size_t tempBufSize = cSrcLen;
    std::vector<WCHAR> tempBuf(tempBufSize);

    int convertLen = MultiByteToWideChar(
        codePage,
        0,
        cSrc,
        static_cast<int>(cSrcLen),
        tempBuf.data(),
        static_cast<int>(tempBufSize));

    if (convertLen == 0)
    {
        ZeroMemory(wDest, wDestLen * sizeof(WCHAR));
        return false;
    }

    if (static_cast<size_t>(convertLen) >= wDestLen)
    {
        memcpy(wDest, tempBuf.data(), wDestLen * sizeof(WCHAR));
    }
    else
    {
        memcpy(wDest, tempBuf.data(), convertLen * sizeof(WCHAR));
        ZeroMemory(wDest + convertLen, (wDestLen - convertLen) * sizeof(WCHAR));
    }

    return true;
}

std::string CPubFunc::CStringToAnsi(const CString& str)
{
    return CT2A(str);
}

CString CPubFunc::AnsiToCString(const char* szAnsi)
{
    return CA2T(szAnsi);
}

std::string CPubFunc::CStringToUtf8(const CString& str)
{
    return CT2A(str, CP_UTF8);
}

CString CPubFunc::Utf8ToCString(const char* szUtf8)
{
    return CA2T(szUtf8, CP_UTF8);
}

int CPubFunc::CStringToInt(const CString& str)
{
    return _ttoi(str);
}

UINT CPubFunc::CStringToUInt(const CString& str)
{
    return static_cast<UINT>(_tcstoul(str, nullptr, 10));
}

UINT CPubFunc::HexStringToUInt(const CString& strHex)
{
    UINT nVal = 0;
    _stscanf_s(strHex, _T("%x"), &nVal);
    return nVal;
}

bool CPubFunc::ReadSerialNumberRange(UINT& nSnStart, UINT& nSnEnd)
{
    // Use the same named mutex as acquire path to keep ini reads consistent with writes.
    HANDLE hMutex = CreateMutexW(nullptr, FALSE, L"Global\\SparkUfsPdt_AcquireSn_Lock");
    if (hMutex == nullptr)
    {
        return false;
    }

    DWORD wait = WaitForSingleObject(hMutex, 5000);
    if (wait != WAIT_OBJECT_0 && wait != WAIT_ABANDONED)
    {
        CloseHandle(hMutex);
        return false;
    }

    bool bRet = false;
    do
    {
        // Prefer going through the adapter to read base setting; fall back to default behavior
        PST_UFS_BASE_SETTING pBase = nullptr;
        // Some contexts may not have dialog pointer; keep existing behavior by reading the static
        // base option via CDialogBase instance if possible through a temporary adapter.
        ::DialogAdapter adapter(&SettingsService::Instance());
        pBase = adapter.GetBaseSetting();
        if (pBase == nullptr || !pBase->bSnSeparateIni || pBase->szRemoteSnPath[0] == '\0')
        {
            break;
        }

        CString iniPath = CA2T(pBase->szRemoteSnPath);
        if (iniPath.IsEmpty() || GetFileAttributes(iniPath) == INVALID_FILE_ATTRIBUTES)
        {
            break;
        }

        WCHAR snStartBuf[128] = {};
        WCHAR snEndBuf[128] = {};
        DWORD lenStart = GetPrivateProfileStringW(L"TEST", L"SerialNumber", L"", snStartBuf, _countof(snStartBuf), CT2W(iniPath));
        DWORD lenEnd = GetPrivateProfileStringW(L"TEST", L"SerialNumber_End", L"", snEndBuf, _countof(snEndBuf), CT2W(iniPath));
        if (lenStart == 0 || lenEnd == 0)
        {
            break;
        }

        CStringW currentSnStart(snStartBuf);
        CStringW currentSnEnd(snEndBuf);
        currentSnStart.Trim();
        currentSnEnd.Trim();
        if (currentSnStart.IsEmpty() || currentSnEnd.IsEmpty())
        {
            break;
        }

        errno = 0;
        wchar_t* endPtr = nullptr;
        nSnStart = wcstoull(currentSnStart, &endPtr, 16);
        if (errno != 0 || endPtr == currentSnStart.GetString() || *endPtr != L'\0')
        {
            break;
        }

        nSnEnd = wcstoull(currentSnEnd, &endPtr, 16);
        if (errno != 0 || endPtr == currentSnEnd.GetString() || *endPtr != L'\0')
        {
            break;
        }

        bRet = true;
    } while (0);

    ReleaseMutex(hMutex);
    CloseHandle(hMutex);
    return bRet;
}

bool CPubFunc::AcquireAndAdvanceSerialNumber(CString& allocatedSn)
{
    allocatedSn.Empty();

    // Use a named mutex to prevent cross-process read-modify-write races on the same ini.
    HANDLE hMutex = CreateMutexW(nullptr, FALSE, L"Global\\SparkUfsPdt_AcquireSn_Lock");
    if (hMutex == nullptr)
    {
        return false;
    }

    DWORD wait = WaitForSingleObject(hMutex, 5000);
    if (wait != WAIT_OBJECT_0 && wait != WAIT_ABANDONED)
    {
        CloseHandle(hMutex);
        return false;
    }

    bool bRet = false;
    do
    {
        // Prefer going through the adapter to read base setting; fall back to default behavior
        PST_UFS_BASE_SETTING pBase = nullptr;
        // Some contexts may not have dialog pointer; keep existing behavior by reading the static
        // base option via CDialogBase instance if possible through a temporary adapter.
        ::DialogAdapter adapter(&SettingsService::Instance());
        pBase = adapter.GetBaseSetting();
        if (pBase == nullptr || !pBase->bSnSeparateIni || pBase->szRemoteSnPath[0] == '\0')
        {
            break;
        }

        CString iniPath = CA2T(pBase->szRemoteSnPath);
        if (iniPath.IsEmpty() || GetFileAttributes(iniPath) == INVALID_FILE_ATTRIBUTES)
        {
            break;
        }

        WCHAR snBuf[128] = {};
        DWORD len = GetPrivateProfileStringW(L"TEST", L"SerialNumber", L"", snBuf, _countof(snBuf), CT2W(iniPath));
        if (len == 0)
        {
            break;
        }

        CStringW currentSn(snBuf);
        currentSn.Trim();
        if (currentSn.IsEmpty())
        {
            break;
        }

        errno = 0;
        wchar_t* endPtr = nullptr;
        unsigned long long currentValue = wcstoull(currentSn, &endPtr, 16);
        if (errno != 0 || endPtr == currentSn.GetString() || *endPtr != L'\0')
        {
            break;
        }

        allocatedSn = CString(currentSn);

        unsigned long long nextValue = currentValue + 1;
        int width = currentSn.GetLength();
        CStringW nextSn;
        nextSn.Format(L"%0*llX", width > 0 ? width : 1, nextValue);

        if (!WritePrivateProfileStringW(L"TEST", L"SerialNumber", nextSn, CT2W(iniPath)))
        {
            break;
        }

        bRet = true;
    } while (0);

    ReleaseMutex(hMutex);
    CloseHandle(hMutex);
    return bRet;
}

bool CPubFunc::ReadTextFileA(const CString& path, CStringA& content)
{
    CFile file;
    if (!file.Open(path, CFile::modeRead | CFile::typeBinary))
    {
        return false;
    }
    ULONGLONG length = file.GetLength();
    CStringA data;
    UINT toRead = static_cast<UINT>(length);
    char* buffer = data.GetBuffer(toRead);
    UINT read = file.Read(buffer, toRead);
    data.ReleaseBuffer(read);
    content = data;
    return true;
}

CString CPubFunc::GetGitVersionString()
{
    TCHAR modulePath[MAX_PATH] = {};
    GetModuleFileName(nullptr, modulePath, MAX_PATH);
    CString dir = modulePath;
    int pos = dir.ReverseFind(_T('\\'));
    if (pos >= 0)
    {
        dir = dir.Left(pos);
    }

    CStringA headContent;
    CString gitHash;
    CString searchDir = dir;
    for (int i = 0; i < 6 && !searchDir.IsEmpty(); ++i)
    {
        CString headPath = searchDir + _T("\\.git\\HEAD");
        if (GetFileAttributes(headPath) != INVALID_FILE_ATTRIBUTES && ReadTextFileA(headPath, headContent))
        {
            headContent.Trim();
            if (headContent.Left(4) == "ref:")
            {
                CStringA refPathA = headContent.Mid(4);
                refPathA.Trim();
                CString refPath = searchDir + _T("\\.git\\") + CString(refPathA);
                CStringA refContent;
                if (ReadTextFileA(refPath, refContent))
                {
                    refContent.Trim();
                    gitHash = CString(refContent);
                }
            }
            else
            {
                gitHash = CString(headContent);
            }
            break;
        }

        int lastSlash = searchDir.ReverseFind(_T('\\'));
        if (lastSlash < 0)
        {
            break;
        }
        searchDir = searchDir.Left(lastSlash);
    }

    if (gitHash.IsEmpty())
    {
        return _T("unknown");
    }

    if (gitHash.GetLength() > 8)
    {
        gitHash = gitHash.Left(8);
    }
    return gitHash;
}
