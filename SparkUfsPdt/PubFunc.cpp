#include "pch.h"
#include "PubFunc.h"

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

bool CPubFunc::CharToWChar(const char* cSrc, size_t cSrcLen, WCHAR* wDest, size_t wDestLen, UINT codePage)
{
    if (cSrc == nullptr || wDest == nullptr || cSrcLen == 0 || wDestLen == 0)
    {
        return false;
    }

    size_t tempBufSize = cSrcLen;
    WCHAR* tempBuf = new WCHAR[tempBufSize];
    ZeroMemory(tempBuf, tempBufSize * sizeof(WCHAR));

    int convertLen = MultiByteToWideChar(
        codePage,
        0,
        cSrc,
        static_cast<int>(cSrcLen),
        tempBuf,
        static_cast<int>(tempBufSize));

    if (convertLen == 0)
    {
        delete[] tempBuf;
        ZeroMemory(wDest, wDestLen * sizeof(WCHAR));
        return false;
    }

    if (static_cast<size_t>(convertLen) >= wDestLen)
    {
        memcpy(wDest, tempBuf, wDestLen * sizeof(WCHAR));
    }
    else
    {
        memcpy(wDest, tempBuf, convertLen * sizeof(WCHAR));
        ZeroMemory(wDest + convertLen, (wDestLen - convertLen) * sizeof(WCHAR));
    }

    delete[] tempBuf;
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
