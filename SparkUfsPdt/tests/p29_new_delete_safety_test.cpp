// =============================================================================
// P2-9 TDD harness: 3 raw new[]/delete[] sites → exception-safety with std::vector.
//
// Instead of intercepting global operator new/delete (which corrupts std
// internals because vector also uses array-form new), we track "buffer
// outstanding count" manually inside the buggy/fixed variants via a local
// RAII tracker.  Buggy versions: manual new[] + manual delete[] on the
// non-throw path but SKIP delete[] when exception thrown → outstanding++
// (leak).  Fixed versions: std::vector destructor ALWAYS runs (both paths)
// → outstanding stays at 0 even when throw happens.
// =============================================================================

#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <vector>

typedef int (*TestCaseFn)();

// -----------------------------------------------------------------------------
// Manual counter: each new T[...] increments, each delete T[...] decrements.
// Net value > 0 after a call = buffer was leaked.
// -----------------------------------------------------------------------------
static int64_t g_outstanding = 0;

// -----------------------------------------------------------------------------
// SITE [a] WCharFieldCompare equivalent
// -----------------------------------------------------------------------------
static int BuggyWCharFieldCompare(int nSize, bool triggerThrow)
{
    int bRet = 0;
    WCHAR* Expected = new WCHAR[nSize]; g_outstanding++;
    memset(Expected, 0, nSize * sizeof(WCHAR));
    const int DestLen = nSize > 4 ? 4 : nSize;
    if (DestLen > 0)
    {
        if (triggerThrow) throw int(42);  // SKIPS delete[] below → leak 1
        bRet = -1;
        delete[] Expected; g_outstanding--;
        return bRet;
    }
    for (int i = 0; i < DestLen; i++) { bRet = (int)i; }
    delete[] Expected; g_outstanding--;
    return bRet;
}

static int FixedWCharFieldCompare(int nSize, bool triggerThrow)
{
    int bRet = 0;
    // Use std::vector<T>(n) + track outstanding via a wrapper so we can
    // still count.  The key invariant: vector dtor runs ON EVERY exit path.
    struct TrackVec : public std::vector<WCHAR> {
        TrackVec(size_t n) : std::vector<WCHAR>(n) { g_outstanding++; }
        ~TrackVec() { g_outstanding--; }
    };
    TrackVec Expected(nSize);  // value-initialised → 0
    const int DestLen = nSize > 4 ? 4 : nSize;
    if (DestLen > 0)
    {
        if (triggerThrow) throw int(42);  // TrackVec dtor → g_outstanding--
        bRet = -1;
        return bRet;
    }
    for (int i = 0; i < DestLen; i++) { bRet = (int)i; }
    return bRet;
}

// -----------------------------------------------------------------------------
// SITE [b] PubFunc CharToWChar equivalent (char buf)
// -----------------------------------------------------------------------------
static bool BuggyCharToWChar(size_t bufSize, bool triggerThrow)
{
    char* tempBuf = new char[bufSize]; g_outstanding++;
    memset(tempBuf, 0, bufSize);
    if (triggerThrow) throw int(42);
    bool ok = (tempBuf[0] == 0);
    delete[] tempBuf; g_outstanding--;
    return ok;
}

static bool FixedCharToWChar(size_t bufSize, bool triggerThrow)
{
    struct TrackVecC : public std::vector<char> {
        TrackVecC(size_t n) : std::vector<char>(n) { g_outstanding++; }
        ~TrackVecC() { g_outstanding--; }
    };
    TrackVecC tempBuf(bufSize);
    if (triggerThrow) throw int(42);
    bool ok = (tempBuf[0] == 0);
    return ok;
}

// -----------------------------------------------------------------------------
// SITE [c] PubFunc WCharToChar equivalent (WCHAR buf)
// -----------------------------------------------------------------------------
static bool BuggyWCharToChar(size_t bufSize, bool triggerThrow)
{
    WCHAR* tempBuf = new WCHAR[bufSize]; g_outstanding++;
    memset(tempBuf, 0, bufSize * sizeof(WCHAR));
    if (triggerThrow) throw int(42);
    bool ok = (tempBuf[0] == 0);
    delete[] tempBuf; g_outstanding--;
    return ok;
}

static bool FixedWCharToChar(size_t bufSize, bool triggerThrow)
{
    struct TrackVecW : public std::vector<WCHAR> {
        TrackVecW(size_t n) : std::vector<WCHAR>(n) { g_outstanding++; }
        ~TrackVecW() { g_outstanding--; }
    };
    TrackVecW tempBuf(bufSize);
    if (triggerThrow) throw int(42);
    bool ok = (tempBuf[0] == 0);
    return ok;
}

// -----------------------------------------------------------------------------
// Helpers
// -----------------------------------------------------------------------------
#define CHECK(cond) do { if (!(cond)) { printf("  FAIL %s line %d: %s (outstanding=%lld)\n", __FUNCTION__, __LINE__, #cond, (long long)g_outstanding); return 1; } } while (0)

template<typename Fn>
static void RunCatch(Fn&& fn) { try { fn(); } catch (...) {} }

// =============================================================================
// RED: 3 buggy sites + 3 throws → outstanding=3 (each leaks 1 buffer)
// =============================================================================
static int TC_RED_buggy_throw_outstanding_eq_3(void)
{
    g_outstanding = 0;
    RunCatch([] { BuggyWCharFieldCompare(16, true); });
    RunCatch([] { BuggyCharToWChar(128, true); });
    RunCatch([] { BuggyWCharToChar(128, true); });
    // Manual new[] without manual delete[] → 3 leaks.
    CHECK(g_outstanding == 3);
    return 0;
}

// =============================================================================
// GREEN: 3 fixed sites + 3 throws → outstanding=0 (vector dtor frees every path)
// =============================================================================
static int TC_GRN_fixed_throw_outstanding_eq_0(void)
{
    g_outstanding = 0;
    RunCatch([] { FixedWCharFieldCompare(16, true); });
    RunCatch([] { FixedCharToWChar(128, true); });
    RunCatch([] { FixedWCharToChar(128, true); });
    CHECK(g_outstanding == 0);
    return 0;
}

// Baseline (no throw): both buggy & fixed return identical values + 0 outstanding.
static int TC_GRN_baseline_same_return_zero_outstanding(void)
{
    g_outstanding = 0;
    int bB = BuggyWCharFieldCompare(16, false);
    int bF = FixedWCharFieldCompare(16, false);
    CHECK(bB == bF);
    CHECK(g_outstanding == 0);

    g_outstanding = 0;
    bool okB = BuggyCharToWChar(128, false);
    bool okF = FixedCharToWChar(128, false);
    CHECK(okB == okF);
    CHECK(g_outstanding == 0);

    g_outstanding = 0;
    okB = BuggyWCharToChar(128, false);
    okF = FixedWCharToChar(128, false);
    CHECK(okB == okF);
    CHECK(g_outstanding == 0);

    return 0;
}

// -----------------------------------------------------------------------------
// Runner
// -----------------------------------------------------------------------------
int main(void)
{
    struct { const char* name; TestCaseFn fn; bool expectPass; } cases[] = {
        { "RED  buggy throw  → outstanding=3", TC_RED_buggy_throw_outstanding_eq_3,         true },
        { "GRN1 fixed throw  → outstanding=0", TC_GRN_fixed_throw_outstanding_eq_0,         true },
        { "GRN2 baseline same + outstanding=0",TC_GRN_baseline_same_return_zero_outstanding,true },
    };
    int failed = 0;
    for (size_t i = 0; i < _countof(cases); i++)
    {
        int rc = cases[i].fn();
        bool pass = (rc == 0) == cases[i].expectPass;
        printf("[%s] %s\n", pass ? "PASS" : "FAIL", cases[i].name);
        if (!pass) failed++;
    }
    printf("\nP29 result: %d failed\n", failed);
    return failed ? 1 : 0;
}
