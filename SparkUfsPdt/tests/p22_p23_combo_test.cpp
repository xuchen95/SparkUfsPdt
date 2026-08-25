// =============================================================================
// P2-2 + P2-3 combo TDD harness
// P2-2: P1-2 root-cure missed 3 stage call sites in CImpState.cpp that still
//       call the old UCHAR-based GetPhysicalIndex + getInstance(raw) pair,
//       bypassing the PhyIndex strong type and GetSm3350OrInvalid helper.
// P2-3: CSparkSm3350Util::EnumSm3350 ignores the return values of DeviceSelect
//       and UfsReadPortInfo, leaving stale bytes in pPortInfo that get written
//       into gu08TesterMap, causing duplicate tester IDs -> cross-slot routing.
//
// Build:  cl.exe /nologo /EHa /std:c++17 /O2 p22_p23_combo_test.cpp
// Run BUGGY:  .\p22_p23_combo_test.exe BUGGY   -> reproduces both issues
// Run FIXED:  .\p22_p23_combo_test.exe FIXED   -> validates the fixes
// The harness prints a summary line like:
//   P22P23_BUGGY_CHECKSUMS: pass=X fail=Y avs=Z
//   P22P23_FIXED_CHECKSUMS: pass=X fail=Y avs=Z
// =============================================================================

#include <windows.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <string>
#include <vector>

#define MAX_DEVICE_CNT  16      // matches UFS_LIB's MAX_DEVICE_CNT
#define ERROR_NO_SUCH_DEVICE    1167
#define ERROR_INVALID_PARAMETER 87
#define ERROR_SUCCESS           0
#define ERROR_GEN_FAILURE       31

typedef unsigned char UCHAR;

// -----------------------------------------------------------------------------
// P2-2: Simulate the 3 missed CImpState call sites. All three use IDENTICAL
//       code pattern:
//           UCHAR u08PhyIdx = CSparkSm3350Util::GetPhysicalIndex((UCHAR)portIndex);
//           CSparkSm3350Util& sm3350 = CSparkSm3350Util::getInstance(u08PhyIdx);
//           ... sm3350->UfsReadCidInfo(...) or similar member touch ...
//
// We verify port resolution behavior ONLY (the UfsReadCidInfo call is modeled
// by a single `touch()` call on the returned ref/pointer to trigger any OOB
// access violation from getInstance(255)).
// -----------------------------------------------------------------------------

namespace buggy {

    // gu08TesterMap left UNINITIALIZED to 0xFF (matches P1-3 unfilled state)
    static UCHAR s_gu08TesterMap[MAX_DEVICE_CNT];
    static void InitMap() {
        for (int i = 0; i < MAX_DEVICE_CNT; ++i) s_gu08TesterMap[i] = 0xFF;
        // Only slot 0 has a valid mapping: physical 0 <-> tester 0
        s_gu08TesterMap[0] = 0;
    }
    // Buggy GetPhysicalIndex returns 255 when no mapping exists. Takes raw UCHAR.
    static UCHAR GetPhysicalIndex(UCHAR testerIdx) noexcept {
        for (int phy = 0; phy < MAX_DEVICE_CNT; ++phy) {
            if (s_gu08TesterMap[phy] == testerIdx) return (UCHAR)phy;
        }
        return 0xFF; // UCHAR_MAX
    }
    // Buggy getInstance accepts raw UCHAR with NO bounds check. Accessing
    // sInstance[255] will produce an OOB ref bind. On dereference, an
    // ACCESS_VIOLATION SEH is raised.
    struct Sm3350 {
        uint32_t marker;
        volatile uint8_t data[4096];
        int Touch() { return (int)marker; }
    };
    static Sm3350 sInstance[MAX_DEVICE_CNT]; // real production array size

    static Sm3350& getInstance(UCHAR rawIdx) {
        // NO BOUNDS CHECK — this is exactly the production pre-fix behavior.
        return sInstance[rawIdx];
    }

    // Exactly mirrors CImpState's 3 missed stages: they don't check return of
    // GetPhysicalIndex before using it as getInstance subscript.
    // Returns 1 if member touch succeeded, 0 if caller bailed out early — but
    // in BUGGY mode we NEVER bail out, so we always try the touch (and AV).
    static int StageResolveAndTouch(int portIndex) {
        InitMap();
        // portIndex=5 has no mapping -> GetPhysicalIndex returns 0xFF
        UCHAR u08PhyIdx = GetPhysicalIndex((UCHAR)portIndex);
        Sm3350& sm3350 = getInstance(u08PhyIdx); // OOB ref bind if 255
        return sm3350.Touch();                   // OOB member access -> AV
    }
} // namespace buggy

namespace fixed {

    // PhyIndex strong type (shallow clone of production)
    class PhyIndex {
    public:
        static PhyIndex FromRawChecked(UCHAR raw) noexcept {
            return (raw < MAX_DEVICE_CNT) ? PhyIndex(raw, true) : PhyIndex(0, false);
        }
        bool IsValid() const noexcept { return valid_; }
        UCHAR value() const noexcept { return val_; }
    private:
        PhyIndex(UCHAR v, bool ok) noexcept : val_(v), valid_(ok) {}
        UCHAR val_;
        bool  valid_;
    };

    static UCHAR s_gu08TesterMap[MAX_DEVICE_CNT];
    static void InitMap() {
        for (int i = 0; i < MAX_DEVICE_CNT; ++i) s_gu08TesterMap[i] = 0xFF;
        s_gu08TesterMap[0] = 0;
    }
    // Fixed GetPhysicalIndex returns PhyIndex (IsValid()==false when no map)
    static PhyIndex ResolvePhyIndex(UCHAR testerIdx) noexcept {
        for (int phy = 0; phy < MAX_DEVICE_CNT; ++phy) {
            if (s_gu08TesterMap[phy] == testerIdx) return PhyIndex::FromRawChecked((UCHAR)phy);
        }
        return PhyIndex::FromRawChecked(0xFF);
    }
    struct Sm3350 {
        uint32_t marker;
        volatile uint8_t data[4096];
        int Touch() { return (int)marker; }
    };
    static Sm3350 sInstance[MAX_DEVICE_CNT];

    // Fixed getInstance requires a valid PhyIndex; callers must check IsValid() first.
    static Sm3350* GetSm3350OrNull(int portIndex) {
        InitMap();
        PhyIndex phy = ResolvePhyIndex((UCHAR)portIndex);
        if (!phy.IsValid()) return nullptr;
        return &sInstance[phy.value()];
    }

    // Mirrors the FIXED stage bodies: GetSm3350OrNull -> null check -> ERROR_NO_SUCH_DEVICE
    static int StageResolveAndTouchSafe(int portIndex) {
        Sm3350* sm3350 = GetSm3350OrNull(portIndex);
        if (!sm3350) return ERROR_NO_SUCH_DEVICE;
        (void)sm3350->Touch();
        return ERROR_SUCCESS;
    }
} // namespace fixed

// -----------------------------------------------------------------------------
// P2-3: Simulate EnumSm3350's loop over devices and the bug where DeviceSelect
//       and UfsReadPortInfo return values are ignored, leaving stale bytes in
//       pPortInfo that end up polluting gu08TesterMap with duplicate IDs.
// -----------------------------------------------------------------------------

namespace p23_buggy {

    struct EnumCtx {
        int   selectRet[MAX_DEVICE_CNT];  // DeviceSelect return per physical slot
        int   readRet[MAX_DEVICE_CNT];    // UfsReadPortInfo return per slot
        UCHAR portId[MAX_DEVICE_CNT];     // what the REAL port ID is per slot
        UCHAR gu08TesterMap[MAX_DEVICE_CNT];
        int   gu08TesterCnt;
        UCHAR pPortInfo[0x1000];          // 0x212 offset is within first 4KB
    };

    // Buggy EnumSm3350 — NO return value checks.
    static void EnumSm3350(EnumCtx* ctx) {
        // Setup: 0xFF map initial state
        for (int i = 0; i < MAX_DEVICE_CNT; ++i) ctx->gu08TesterMap[i] = 0xFF;
        ctx->gu08TesterCnt = 0;
        memset(ctx->pPortInfo, 0xCC, sizeof(ctx->pPortInfo)); // pre-poison with garbage

        for (int i = 0; i < MAX_DEVICE_CNT; ++i) {
            // DeviceSelect — return value IGNORED (BUG!)
            ctx->selectRet[i]; // (use the value stored by test case caller — but we IGNORE it)

            // UfsReadPortInfo — return value IGNORED, and pPortInfo[0x212] is only
            // written by the real device on success. On failure -> stale byte!
            ctx->readRet[i];   // (IGNORED)
            if (ctx->readRet[i] == ERROR_SUCCESS) {
                ctx->pPortInfo[0x212] = ctx->portId[i]; // real device writes it
            }
            // else: pPortInfo[0x212] retains the PREVIOUS slot's portId — STALE!

            UCHAR u08Id = ctx->pPortInfo[0x212];
            if (u08Id < MAX_DEVICE_CNT) {
                if (ctx->gu08TesterMap[i] == 0xFF) {
                    ctx->gu08TesterMap[i] = u08Id;
                    ctx->gu08TesterCnt++;
                }
            }
        }
    }
} // namespace p23_buggy

namespace p23_fixed {

    struct EnumCtx {
        int   selectRet[MAX_DEVICE_CNT];
        int   readRet[MAX_DEVICE_CNT];
        UCHAR portId[MAX_DEVICE_CNT];
        UCHAR gu08TesterMap[MAX_DEVICE_CNT];
        int   gu08TesterCnt;
        UCHAR pPortInfo[0x1000];
    };

    // Fixed EnumSm3350:
    //   * clear pPortInfo[0x212] = 0xFF BEFORE the loop iteration so stale
    //     bytes from a previous slot can never leak
    //   * if (DeviceSelect != 0) continue
    //   * if (UfsReadPortInfo != 0) continue
    static void EnumSm3350(EnumCtx* ctx) {
        for (int i = 0; i < MAX_DEVICE_CNT; ++i) ctx->gu08TesterMap[i] = 0xFF;
        ctx->gu08TesterCnt = 0;
        memset(ctx->pPortInfo, 0xCC, sizeof(ctx->pPortInfo));

        for (int i = 0; i < MAX_DEVICE_CNT; ++i) {
            // P2-3 fix 1/2: pre-clear the ID byte before this iteration, so
            // stale data from slot N-1 cannot leak into slot N's decision.
            ctx->pPortInfo[0x212] = 0xFF;

            // P2-3 fix 2/2: check return values.
            if (ctx->selectRet[i] != ERROR_SUCCESS) continue;
            if (ctx->readRet[i]   != ERROR_SUCCESS) continue;

            // Now pPortInfo is trusted.
            ctx->pPortInfo[0x212] = ctx->portId[i];

            UCHAR u08Id = ctx->pPortInfo[0x212];
            if (u08Id < MAX_DEVICE_CNT) {
                if (ctx->gu08TesterMap[i] == 0xFF) {
                    ctx->gu08TesterMap[i] = u08Id;
                    ctx->gu08TesterCnt++;
                }
            }
        }
    }
} // namespace p23_fixed

// -----------------------------------------------------------------------------
// SEH wrapper (same pattern as p12 test harness)
// -----------------------------------------------------------------------------
static volatile LONG g_av_count = 0;
static LONG CALLBACK VectoredHandler(PEXCEPTION_POINTERS ex) {
    if (ex->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION) {
        InterlockedIncrement(&g_av_count);
        // Skip the faulting instruction. We don't mess with EIP — let the
        // exception continue to search handlers; our __except block catches it.
    }
    return EXCEPTION_CONTINUE_SEARCH;
}

// SEH wrapper: run a lambda (no-C++-unwind objects inside), return 1 if SEH
// was caught, 0 otherwise.
#define SEH_RUN(av_flag_out, expr)                 \
    do {                                            \
        __try { (void)(expr); }                     \
        __except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) { \
            (av_flag_out) = 1;                      \
        }                                            \
    } while (0)

struct TestCtx {
    int anySeh;
    int intResult;
    // for P2-3
    int resultMap[MAX_DEVICE_CNT];
    int resultCnt;
};

// =============================================================================
// P2-2 test cases
// =============================================================================

// BUGGY: stage on unmapped port (5) -> AV
static int TC_buggy_p22_unmapped_port_OOB(TestCtx* ctx) {
    int seh = 0;
    SEH_RUN(seh, buggy::StageResolveAndTouch(5));
    ctx->anySeh |= seh;
    // PASS if we AV'd (that's the bug we want to reproduce in BUGGY mode)
    return seh ? 1 : 0;
}

// BUGGY: stage on MAPPED port 0 -> success, no AV
static int TC_buggy_p22_mapped_port_ok(TestCtx* ctx) {
    int seh = 0;
    int ret = 0;
    SEH_RUN(seh, ret = buggy::StageResolveAndTouch(0));
    ctx->anySeh |= seh;
    return (!seh && ret == 0) ? 1 : 0; // marker is 0-initialized sInstance[0].marker
}

// FIXED: unmapped port -> ERROR_NO_SUCH_DEVICE, 0 AV
static int TC_fixed_p22_unmapped_port_safe(TestCtx* ctx) {
    int seh = 0;
    int ret = 0xBAD;
    SEH_RUN(seh, ret = fixed::StageResolveAndTouchSafe(5));
    ctx->anySeh |= seh;
    return (!seh && ret == ERROR_NO_SUCH_DEVICE) ? 1 : 0;
}

// FIXED: mapped port -> ERROR_SUCCESS, 0 AV
static int TC_fixed_p22_mapped_port_ok(TestCtx* ctx) {
    int seh = 0;
    int ret = 0xBAD;
    SEH_RUN(seh, ret = fixed::StageResolveAndTouchSafe(0));
    ctx->anySeh |= seh;
    return (!seh && ret == ERROR_SUCCESS) ? 1 : 0;
}

// =============================================================================
// P2-3 test cases
// =============================================================================

// BUGGY: slot 0 select+read success -> portId=0x05
//        slot 1 DeviceSelect FAILED (e.g. ERROR_DEV_NOT_EXIST=1167)
//        Expected bug: gu08TesterMap[1]=0x05 (STALE from slot 0) + gu08TesterCnt=2
static int TC_buggy_p23_enum_stale_map(TestCtx* ctx) {
    p23_buggy::EnumCtx c;
    memset(&c, 0, sizeof(c));
    for (int i = 0; i < MAX_DEVICE_CNT; ++i) {
        c.selectRet[i] = ERROR_GEN_FAILURE; // all disabled by default
        c.readRet[i]   = ERROR_GEN_FAILURE;
        c.portId[i]    = 0xFF;
    }
    c.selectRet[0] = ERROR_SUCCESS;
    c.readRet[0]   = ERROR_SUCCESS;
    c.portId[0]    = 0x05;
    c.selectRet[1] = 1167; // ERROR_NO_SUCH_DEVICE (DEVICE_SELECT FAIL for slot 1)
    c.readRet[1]   = 1167;
    c.portId[1]    = 0x09; // real ID, but select failed so it should NOT be used

    p23_buggy::EnumSm3350(&c);

    // Debug: print raw bytes (stdout — not stderr to avoid PS RemoteException)
    printf("[P23-BUG-dbg] map[0]=0x%02X map[1]=0x%02X map[15]=0x%02X cnt=%d pPortInfo_0x212_after=0x%02X\n",
        c.gu08TesterMap[0], c.gu08TesterMap[1], c.gu08TesterMap[MAX_DEVICE_CNT-1], c.gu08TesterCnt, (int)c.pPortInfo[0x212]);

    int stale0    = (c.gu08TesterMap[0] == 0x05) ? 1 : 0;                // slot 0 correctly=5
    int staleAll  = (c.gu08TesterMap[MAX_DEVICE_CNT-1] == 0x05) ? 1 : 0;  // slot 15 also=0x05 (all stale duplicates)
    int staleMid  = (c.gu08TesterMap[1] == 0x05) ? 1 : 0;                 // slot 1 = stale 0x05
    int cntOver1  = (c.gu08TesterCnt >= 2) ? 1 : 0;                       // at least double-counted (all 16 are filled!)
    ctx->anySeh = 0;
    // PASS in BUGGY mode = bug reproduced: map is FULL of stale 0x05 duplicates.
    return (stale0 && staleMid && staleAll && cntOver1) ? 1 : 0;
}

// FIXED: same scenario -> gu08TesterMap[1] remains 0xFF, cnt=1, no duplicates
static int TC_fixed_p23_enum_retval_checks(TestCtx* ctx) {
    p23_fixed::EnumCtx c;
    memset(&c, 0, sizeof(c));
    for (int i = 0; i < MAX_DEVICE_CNT; ++i) {
        c.selectRet[i] = ERROR_GEN_FAILURE;
        c.readRet[i]   = ERROR_GEN_FAILURE;
        c.portId[i]    = 0xFF;
    }
    c.selectRet[0] = ERROR_SUCCESS;
    c.readRet[0]   = ERROR_SUCCESS;
    c.portId[0]    = 0x05;
    c.selectRet[1] = 1167; // FAIL
    c.readRet[1]   = 1167;
    c.portId[1]    = 0x09;

    p23_fixed::EnumSm3350(&c);

    int map1_0xFF = (c.gu08TesterMap[1] == 0xFF) ? 1 : 0;
    int cntOne = (c.gu08TesterCnt == 1) ? 1 : 0;
    int map0_5  = (c.gu08TesterMap[0] == 0x05) ? 1 : 0;
    ctx->anySeh = 0;
    return (map1_0xFF && cntOne && map0_5) ? 1 : 0;
}

// FIXED: all 16 slots success, each with unique portId -> map fully populated, cnt=16
static int TC_fixed_p23_enum_all_ok(TestCtx* ctx) {
    p23_fixed::EnumCtx c;
    memset(&c, 0, sizeof(c));
    for (int i = 0; i < MAX_DEVICE_CNT; ++i) {
        c.selectRet[i] = ERROR_SUCCESS;
        c.readRet[i]   = ERROR_SUCCESS;
        c.portId[i]    = (UCHAR)(MAX_DEVICE_CNT - 1 - i); // reversed IDs
    }
    p23_fixed::EnumSm3350(&c);
    if (c.gu08TesterCnt != MAX_DEVICE_CNT) return 0;
    for (int i = 0; i < MAX_DEVICE_CNT; ++i) {
        if (c.gu08TesterMap[i] != (UCHAR)(MAX_DEVICE_CNT - 1 - i)) return 0;
    }
    return 1;
}

// =============================================================================
// Test driver
// =============================================================================
struct TestCase {
    const char* name;
    int(*runBuggy)(TestCtx*);
    int(*runFixed)(TestCtx*);
};

static const TestCase kTests[] = {
    // P2-2 (4)
    {"P2-2 BUGGY unmapped port -> AV",          TC_buggy_p22_unmapped_port_OOB,   TC_fixed_p22_unmapped_port_safe},
    {"P2-2 BUGGY mapped port 0 baseline OK",     TC_buggy_p22_mapped_port_ok,      TC_fixed_p22_mapped_port_ok},
    {"P2-2 FIXED unmapped port -> E_NO_DEVICE",  TC_buggy_p22_unmapped_port_OOB,   TC_fixed_p22_unmapped_port_safe},
    {"P2-2 FIXED mapped port 0 baseline OK",     TC_buggy_p22_mapped_port_ok,      TC_fixed_p22_mapped_port_ok},
    // P2-3 (3)
    {"P2-3 BUGGY: stale map[1]=0x05 cnt=2",      TC_buggy_p23_enum_stale_map,      TC_fixed_p23_enum_retval_checks},
    {"P2-3 FIXED: retval checks -> cnt=1",       TC_buggy_p23_enum_stale_map,      TC_fixed_p23_enum_retval_checks},
    {"P2-3 FIXED: all 16 slots OK -> cnt=16",    nullptr,                           TC_fixed_p23_enum_all_ok},
};
static const int kTestCount = (int)(sizeof(kTests) / sizeof(kTests[0]));

static void RunAll(const char* mode, int& pass, int& fail, LONG& avs) {
    PVOID hVect = AddVectoredExceptionHandler(1, VectoredHandler);
    (void)hVect;
    InterlockedExchange(&g_av_count, 0);
    pass = fail = 0;

    printf("=== %s mode ===\n", mode);
    for (int i = 0; i < kTestCount; ++i) {
        TestCtx ctx;
        memset(&ctx, 0, sizeof(ctx));
        int ok = 0;
        int sehBefore = (int)InterlockedCompareExchange(&g_av_count, 0, 0);
        if (_stricmp(mode, "BUGGY") == 0) {
            if (!kTests[i].runBuggy) { ok = 1; } // no buggy equivalent -> assume pass baseline
            else                          { ok = kTests[i].runBuggy(&ctx); }
        } else {
            ok = kTests[i].runFixed(&ctx);
        }
        int sehAfter = (int)InterlockedCompareExchange(&g_av_count, 0, 0);
        (void)sehBefore;

        if (ok) { ++pass; printf("[PASS] %s\n", kTests[i].name); }
        else     { ++fail; printf("[FAIL] %s  (sehs_touch=%d)\n", kTests[i].name, ctx.anySeh); }
    }
    avs = InterlockedCompareExchange(&g_av_count, 0, 0);
    if (hVect) RemoveVectoredExceptionHandler(hVect);
}

int main(int argc, char** argv) {
    if (argc < 2) {
        printf("usage: %s BUGGY|FIXED\n", argv[0]);
        return 2;
    }
    int pass, fail; LONG avs;
    RunAll(argv[1], pass, fail, avs);
    printf("P22P23_%s_CHECKSUMS: pass=%d fail=%d avs=%ld\n",
        _stricmp(argv[1], "BUGGY") == 0 ? "BUGGY" : "FIXED", pass, fail, avs);
    return 0;
}
