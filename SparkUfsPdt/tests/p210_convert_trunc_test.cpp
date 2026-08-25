// =============================================================================
// P2-10 TDD harness: ConvertWCharDataToCharData L143-L146 truncation branch.
//
// BUG (before fix, production code L143-L146):
//   if (convertLen >= cDestLen) { memcpy(cDest, tempBuf, cDestLen); /* return true! */ }
//   → TWO bugs:
//     [1] Function returns TRUE (success), silently pretends the conversion
//         succeeded when in fact the output buffer was too small.  Caller
//         doesn't know the string got truncated (e.g. SN/model get silently
//         mangled, production traceability breaks).
//     [2] memcpy() fills the ENTIRE cDestLen bytes — there is NO room for a
//         trailing NUL terminator AND the code doesn't write one.  Any caller
//         that treats cDest as a C string (strlen / strcmp / printf("%s") /
//         strcpy_s without explicit length) reads BEYOND cDest until it hits
//         a random 0x00 byte on the stack → OOB read / garbage in logs.
//
// FIX (Plan A — strict failure, same behaviour as the convertLen==0 branch):
//   if (convertLen >= cDestLen) { memset(cDest, 0, cDestLen); return false; }
//   → callers that don't check the bool return value still get a SAFE output
//     (all zeros = empty C string), and callers that DO check get a clear
//     "too small" signal.  Zero semantic ambiguity.
// =============================================================================

#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <vector>

typedef int (*TestCaseFn)();

// -----------------------------------------------------------------------------
// Buggy implementation (mirrors production code BEFORE P2-10 fix, verbatim).
// -----------------------------------------------------------------------------
static bool ConvertWCharDataToCharData_BUGGY(
    const WCHAR* wSrc, size_t wSrcLen,
    char* cDest, size_t cDestLen,
    UINT codePage /* = CP_ACP */)
{
    if (wSrc == NULL || cDest == NULL || wSrcLen == 0 || cDestLen == 0) return false;
    size_t tempBufSize = wSrcLen * 3;
    std::vector<char> tempBuf(tempBufSize);
    memset(tempBuf.data(), 0, tempBufSize);

    int convertLen = WideCharToMultiByte(codePage, 0, wSrc, (int)wSrcLen,
        tempBuf.data(), (int)tempBufSize, NULL, NULL);
    if (convertLen == 0) { memset(cDest, 0, cDestLen); return false; }

    // BUG branch (production L143-L146 verbatim — NO NUL, NO failure).
    if ((size_t)convertLen >= cDestLen)
    {
        memcpy(cDest, tempBuf.data(), cDestLen);
        return true;  // ❌ bug 1: claims success
                      // ❌ bug 2: cDest has no trailing NUL
    }
    else
    {
        memcpy(cDest, tempBuf.data(), convertLen);
        memset(cDest + convertLen, 0, cDestLen - convertLen);
    }
    return true;
}

// -----------------------------------------------------------------------------
// FIXED implementation (Plan A — strict failure, matches proposed fix).
// -----------------------------------------------------------------------------
static bool ConvertWCharDataToCharData_FIXED(
    const WCHAR* wSrc, size_t wSrcLen,
    char* cDest, size_t cDestLen,
    UINT codePage /* = CP_ACP */)
{
    if (wSrc == NULL || cDest == NULL || wSrcLen == 0 || cDestLen == 0) return false;
    size_t tempBufSize = wSrcLen * 3;
    std::vector<char> tempBuf(tempBufSize);
    memset(tempBuf.data(), 0, tempBufSize);

    int convertLen = WideCharToMultiByte(codePage, 0, wSrc, (int)wSrcLen,
        tempBuf.data(), (int)tempBufSize, NULL, NULL);
    if (convertLen == 0) { memset(cDest, 0, cDestLen); return false; }

    // FIXED branch (Plan A): same policy as convertLen==0.
    if ((size_t)convertLen >= cDestLen)
    {
        memset(cDest, 0, cDestLen);   // ✅ safe output (empty C string, has NUL)
        return false;                  // ✅ signals "too small" to caller
    }
    else
    {
        memcpy(cDest, tempBuf.data(), convertLen);
        memset(cDest + convertLen, 0, cDestLen - convertLen);
    }
    return true;
}

// -----------------------------------------------------------------------------
// Helpers
// -----------------------------------------------------------------------------
#define CHECK(cond) do { if (!(cond)) { printf("  FAIL %s line %d: %s\n", __FUNCTION__, __LINE__, #cond); return 1; } } while (0)

static bool BufferIsAllZero(const char* p, size_t n)
{
    for (size_t i = 0; i < n; i++) if (p[i] != 0) return false;
    return true;
}
static bool HasTrailingNulAtOrAfter(const char* p, size_t startIdx, size_t cDestLen)
{
    // Returns true only if a NUL exists at index >= startIdx (i.e. the byte
    // sequence at [0..startIdx) is followed by NUL within the buffer).
    for (size_t i = startIdx; i < cDestLen; i++) if (p[i] == 0) return true;
    return false;
}

// -----------------------------------------------------------------------------
// RED: prove the bug exists in the BUGGY implementation.
// -----------------------------------------------------------------------------
static int TC_buggy_trunc_returns_true_no_nul(void)
{
    // Input: 8 ASCII WCHARs → CP_ACP converts to exactly 8 bytes.
    const WCHAR wSrc[] = L"12345678";
    const size_t wSrcLen = wcslen(wSrc);
    char cDest[5] = { 0xDD, 0xDD, 0xDD, 0xDD, 0xDD };  // poison

    bool ok = ConvertWCharDataToCharData_BUGGY(wSrc, wSrcLen, cDest, _countof(cDest), CP_ACP);

    // Bug 1/2: BUGGY version returns TRUE (claims success).
    CHECK(ok == true);

    // Bug 2/2: BUGGY version fills ALL 5 bytes with "12345" — NO NUL terminator
    // anywhere in the buffer (cDest has length 5 and every byte is non-zero).
    CHECK(0 == memcmp(cDest, "12345", 5));
    CHECK(!HasTrailingNulAtOrAfter(cDest, 0, _countof(cDest)));

    return 0;  // RED PASS: bug reproduced
}

// -----------------------------------------------------------------------------
// GREEN 1: fixed version returns FALSE and zeroes buffer when buffer too small.
// -----------------------------------------------------------------------------
static int TC_fixed_trunc_returns_false_zeroed(void)
{
    const WCHAR wSrc[] = L"12345678";
    const size_t wSrcLen = wcslen(wSrc);
    char cDest[5] = { 0xDD, 0xDD, 0xDD, 0xDD, 0xDD };

    bool ok = ConvertWCharDataToCharData_FIXED(wSrc, wSrcLen, cDest, _countof(cDest), CP_ACP);

    CHECK(ok == false);                 // ✅ signals FAILURE
    CHECK(BufferIsAllZero(cDest, _countof(cDest)));  // ✅ safe empty C-string output
    return 0;
}

// -----------------------------------------------------------------------------
// GREEN 2: fixed version also FAILS when convertLen == cDestLen exactly (no
// room for NUL → still invalid C-string output).  Plan A strict policy.
// -----------------------------------------------------------------------------
static int TC_fixed_equal_size_also_fail(void)
{
    const WCHAR wSrc[] = L"12345678";
    const size_t wSrcLen = wcslen(wSrc);  // 8 WCHARs → 8 bytes in CP_ACP
    char cDest[8];
    memset(cDest, 0xDD, _countof(cDest));

    bool ok = ConvertWCharDataToCharData_FIXED(wSrc, wSrcLen, cDest, _countof(cDest), CP_ACP);

    // 8-byte buffer cannot hold 8-byte payload + mandatory NUL → must FAIL.
    CHECK(ok == false);
    CHECK(BufferIsAllZero(cDest, _countof(cDest)));
    return 0;
}

// -----------------------------------------------------------------------------
// Baseline: normal path (convertLen < cDestLen) — both impls behave identically.
// No regression in the success case.
// -----------------------------------------------------------------------------
static int TC_fixed_baseline_normal_convert(void)
{
    const WCHAR wSrc[] = L"12345678";
    const size_t wSrcLen = wcslen(wSrc);

    char cDestBug[16]; memset(cDestBug, 0xDD, sizeof(cDestBug));
    char cDestFix[16]; memset(cDestFix, 0xDD, sizeof(cDestFix));

    bool okB = ConvertWCharDataToCharData_BUGGY(wSrc, wSrcLen, cDestBug, _countof(cDestBug), CP_ACP);
    bool okF = ConvertWCharDataToCharData_FIXED(wSrc, wSrcLen, cDestFix, _countof(cDestFix), CP_ACP);

    CHECK(okB == true);
    CHECK(okF == true);
    CHECK(0 == strcmp(cDestBug, "12345678"));   // buggy baseline OK (has room for NUL)
    CHECK(0 == strcmp(cDestFix, "12345678"));   // fixed same
    CHECK(0 == memcmp(cDestBug, cDestFix, _countof(cDestBug)));  // byte-identical output
    return 0;
}

// -----------------------------------------------------------------------------
// Runner
// -----------------------------------------------------------------------------
int main(void)
{
    struct { const char* name; TestCaseFn fn; bool expectPass; } cases[] = {
        // RED: bug reproduced (TC passes = buggy code misbehaves as designed)
        { "RED  TC_buggy_trunc_returns_true_no_nul",     TC_buggy_trunc_returns_true_no_nul,     true  },
        // GREEN
        { "GRN1 TC_fixed_trunc_returns_false_zeroed",    TC_fixed_trunc_returns_false_zeroed,    true  },
        { "GRN2 TC_fixed_equal_size_also_fail",          TC_fixed_equal_size_also_fail,          true  },
        { "GRN3 TC_fixed_baseline_normal_convert",       TC_fixed_baseline_normal_convert,       true  },
    };
    int failed = 0;
    for (size_t i = 0; i < _countof(cases); i++)
    {
        int rc = cases[i].fn();
        bool pass = (rc == 0) == cases[i].expectPass;
        printf("[%s] %s\n", pass ? "PASS" : "FAIL", cases[i].name);
        if (!pass) failed++;
    }
    printf("\nP210 result: %d failed\n", failed);
    return failed ? 1 : 0;
}
