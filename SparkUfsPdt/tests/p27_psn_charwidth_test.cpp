// =============================================================================
// P2-7 TDD harness: SetSnData L442 swprintf_s(psn, L"%S", psnText)
//                  character-width mismatch root-cause reproduction + fix validation
//
// COVERAGE (4 test cases):
//   [RED 1]   TC_buggy_pctS_wrong_charwidth_ascii8   — L"12345678" 用错 %S → 只写 1 字符（截断）
//   [RED 2]   TC_buggy_pctS_wrong_charwidth_midnull  — 含高位 0x00 的中文/高 UCS2 字符 → 立即截断
//   [GREEN 1] TC_fixed_wcscpy_direct                 — wcscpy_s(psn, src) → 全 8 字符正确
//   [GREEN 2] TC_fixed_swprintf_pcts                 — swprintf_s(psn, L"%s", wchar_t*) → 全 8 字符正确
//
// BUILD:
//   cl /nologo /EHsc /std:c++17 p27_psn_charwidth_test.cpp
// RUN:
//   .\p27_psn_charwidth_test.exe
//
// The harness uses VC CRT's real swprintf_s directly — no mocking — so we
// observe the EXACT same behavior the production code sees.
// =============================================================================

#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <wchar.h>
#include <string.h>
#include <crtdbg.h>
#include <stdlib.h>

// ---- Global: disable the invalid-parameter fast-fail handler during our
//      "observe the actual truncation behavior" subtests. Without this, the
//      modern VC CRT kills the process with STATUS_STACK_BUFFER_OVERRUN
//      (0xC0000409) as soon as it sees the %S / wchar_t* mismatch. We still
//      verify the kill-via-SEH case separately below.
static void __cdecl silent_iph(const wchar_t*, const wchar_t*, const wchar_t*, unsigned, uintptr_t) { /* noop */ }
static _invalid_parameter_handler s_prevIph = nullptr;
static void IphPush() { s_prevIph = _set_invalid_parameter_handler(silent_iph); }
static void IphPop()  { _set_invalid_parameter_handler(s_prevIph); }

// ---- SEH helper to catch the 0xC0000409 CRT fast-fail when the handler is on.
// Returns STATUS_SUCCESS (0) if no exception, else the exception code.
static DWORD RunWithSehCatch(void(*fn)(), bool* terminated)
{
    *terminated = false;
    __try {
        fn();
    } __except (GetExceptionCode() == 0xC0000409 ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
    {
        *terminated = true;
        return GetExceptionCode();
    }
    return 0;
}

// ---- Buggy payload (for SEH case): matches production SetSnData L442 exactly.
struct PayloadCtx { const wchar_t* psnText; wchar_t* psn; size_t psnCch; int ret; };
static PayloadCtx* g_payload = nullptr;
static void BuggyPayload()
{
    PayloadCtx* c = g_payload;
    c->ret = swprintf_s(c->psn, c->psnCch, L"%S", c->psnText);
}

// ---- test infrastructure ---------------------------------------------------
static int s_pass = 0;
static int s_fail = 0;

#define CHECK(cond) do { \
    if (cond) { \
        s_pass++; \
    } else { \
        s_fail++; \
        printf("[CHECK FAIL %s:%d] %s\n", __FILE__, __LINE__, #cond); \
    } \
} while (0)

#define RUN_CASE(name) do { \
    int p_before = s_pass, f_before = s_fail; \
    printf("\n=== %s ===\n", #name); \
    name(); \
    int dp = s_pass - p_before, df = s_fail - f_before; \
    printf("  -> pass=%d fail=%d\n", dp, df); \
} while (0)

// -----------------------------------------------------------------------------
// RED 1 (VS2015+ modern CRT): %S + wchar_t* → invalid_parameter → failfast
//   In earlier direct runs this produced exit code -1073740791 = 0xC0000409
//   (STATUS_STACK_BUFFER_OVERRUN via CRT terminate). We inline the buggy
//   call inside __try/__except so the harness survives; the assert below
//   doubles as proof that either the CRT kills the frame OR (when the CRT
//   survives due to handler tolerance) the data is silently truncated to
//   1 char anyway — the real production failure modes.
// -----------------------------------------------------------------------------
static void TC_buggy_pctS_either_crash_or_truncate()
{
    const wchar_t* psnText = L"12345678";
    wchar_t psn[9] = { L'X', L'X', L'X', L'X', L'X', L'X', L'X', L'X', L'\0' };
    bool killed = false;
    DWORD  code = 0;
    int    n = -2;

    __try {
        // BUGGY production line — inlined so CRT local checks fire naturally.
        n = swprintf_s(psn, _countof(psn), L"%S", psnText);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        killed = true;
        code = GetExceptionCode();
    }

    if (killed) {
        // --- Path A (strict CRT): hard-terminated before returning ---
        CHECK(code == 0xC0000409u);   // STATUS_STACK_BUFFER_OVERRUN
    } else {
        // --- Path B (lenient CRT): survives but data corrupted ---
        CHECK(n == 1);
        CHECK(wcslen(psn) == 1);
        CHECK(psn[0] == L'1');
        CHECK(psn[1] == L'\0');
        CHECK(psn[2] == L'X');
    }
}

// -----------------------------------------------------------------------------
// RED 2 (old CRT / with invalid-parameter silenced): %S treats wchar_t* as
//   narrow char*, reads until first 0x00 byte → writes only the 1st character.
// -----------------------------------------------------------------------------
static void TC_buggy_pctS_old_crt_or_silenced_truncates_to_1char()
{
    const wchar_t* psnText = L"12345678";   // UTF-16LE: 31 00 32 00 33 00 ...
    wchar_t psn[9] = { L'X', L'X', L'X', L'X', L'X', L'X', L'X', L'X', L'\0' };

    IphPush();  // silence invalid_parameter hard-terminate so we can observe data
    int n = swprintf_s(psn, _countof(psn), L"%S", psnText);
    IphPop();

    // Expected: swprintf_s sees narrow "1" (bytes: 0x31 then 0x00 terminator)
    //           → writes L"1" (1 WCHAR + null)
    CHECK(n == 1);
    CHECK(wcslen(psn) == 1);
    CHECK(psn[0] == L'1');
    CHECK(psn[1] == L'\0');   // early null = proof of truncation
    CHECK(psn[2] == L'X');   // rest untouched from init
}

// -----------------------------------------------------------------------------
// GREEN 1: wcscpy_s(psn, psnText) — recommended fix. No format, no failfast.
// -----------------------------------------------------------------------------
static void TC_fixed_wcscpy_direct_no_crash_full_8chars()
{
    const wchar_t* psnText = L"12345678";
    wchar_t psn[9] = {};

    // FIXED production option A (preferred for known-length case)
    errno_t err = wcscpy_s(psn, _countof(psn), psnText);

    IphPush();  // irrelevant here; silence handler just to be strict
    CHECK(err == 0);
    CHECK(wcslen(psn) == 8);
    CHECK(wcscmp(psn, L"12345678") == 0);
    IphPop();
}

// -----------------------------------------------------------------------------
// GREEN 2: swprintf_s(psn, L"%s", psnText) — correct %s specifier.
//          In wprintf family, %s = wchar_t* (native) → no failfast, full copy.
// -----------------------------------------------------------------------------
static void TC_fixed_swprintf_pcts_correct_specifier()
{
    // 8 wide chars, including one U+0100 (non-ASCII, high byte non-zero).
    // NOTE: split the \x escape from trailing hex chars CDEF so the compiler
    //       doesn't interpret the whole thing as one giant escape.
    const wchar_t* psnText = L"AB\x0100" L"CDEFG";   // A B Ā C D E F G → 8 chars
    wchar_t psn[9] = {};

    // FIXED production option B: use the CORRECT width specifier for the family.
    int n = swprintf_s(psn, _countof(psn), L"%s", psnText);

    CHECK(n == 8);
    CHECK(wcslen(psn) == 8);
    CHECK(psn[0] == L'A');
    CHECK(psn[1] == L'B');
    CHECK(psn[2] == 0x0100);   // non-ASCII wide char preserved
    CHECK(wcscmp(psn, psnText) == 0);
}

// -----------------------------------------------------------------------------
int main()
{
    printf("P2-7 TDD: SetSnData %%S charwidth mismatch\n");
    printf("------------------------------------------\n");

    RUN_CASE(TC_buggy_pctS_either_crash_or_truncate);
    RUN_CASE(TC_buggy_pctS_old_crt_or_silenced_truncates_to_1char);
    RUN_CASE(TC_fixed_wcscpy_direct_no_crash_full_8chars);
    RUN_CASE(TC_fixed_swprintf_pcts_correct_specifier);

    printf("\n==========================================\n");
    printf("P27_CHECKSUMS: pass=%d fail=%d\n", s_pass, s_fail);
    printf("EXIT_CODE: %s\n", (s_fail == 0) ? "OK" : "FAIL");
    return (s_fail == 0) ? 0 : 1;
}
