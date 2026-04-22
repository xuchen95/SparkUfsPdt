#pragma once
#include "pch.h"
#include <afxwin.h>
#include <atlconv.h>
#include <string>

class CPubFunc
{
public:
    // ==============================================
    // 1. 数字 → 十六进制字符串（最常用）
    // ==============================================
    static CString IntToHex(UINT nValue, BOOL bUppercase = TRUE, int nDigits = 0);

    // ==============================================
    // 2. 字节数组 → 十六进制字符串
    // ==============================================
    static CString BytesToHex(const BYTE* pData, int nLen, BOOL bUppercase = TRUE);

    // ==============================================
    // 3. 十六进制字符串 → 字节数组
    // ==============================================
    static int HexToBytes(LPCTSTR lpszHex, BYTE* pOut, int nOutMaxLen);

    // ==============================================
    // 4. WCHAR数组 → char数组
    // ==============================================
    static bool WCharToChar(const WCHAR* wSrc, size_t wSrcLen,
        char* cDest, size_t cDestLen,
        UINT codePage = CP_ACP);

    // ==============================================
    // 5. char数组 → WCHAR数组
    // ==============================================
    static bool CharToWChar(const char* cSrc, size_t cSrcLen,
        WCHAR* wDest, size_t wDestLen,
        UINT codePage = CP_ACP);

    // ==============================================
    // 6. CString 转 char* (ANSI)
    // ==============================================
    static std::string CStringToAnsi(const CString& str);

    // ==============================================
    // 7. char* → CString
    // ==============================================
    static CString AnsiToCString(const char* szAnsi);

    // ==============================================
    // 8. CString 转 UTF-8
    // ==============================================
    static std::string CStringToUtf8(const CString& str);

    // ==============================================
    // 9. UTF-8 → CString
    // ==============================================
    static CString Utf8ToCString(const char* szUtf8);

    // ==============================================
    // 10. CString 转 int / UINT
    // ==============================================
    static int CStringToInt(const CString& str);

    static UINT CStringToUInt(const CString& str);

    // ==============================================
    // 11. 十六进制字符串 → 整数
    // ==============================================
    static UINT HexStringToUInt(const CString& strHex);

    static bool AcquireAndAdvanceSerialNumber(CString& allocatedSn);
    static bool ReadTextFileA(const CString& path, CStringA& content);
    static CString GetGitVersionString();
};