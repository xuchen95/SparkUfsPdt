// =============================================================================
// P2-5 TDD harness: EnumSm3350 TesterId uniqueness check
//  Issue: Two physical slots with the SAME tester-id jumper silently share
//         one gu08TesterMap entry. GetPhysicalIndex(TesterId(N)) always returns
//         the FIRST slot. Any task assigned to the second slot silently runs
//         on the first slot → device B overwrites device A's SN/ISP/CID.
//
//  COVERAGE (4 test cases):
//   [RED 1]   TC_buggy_dup_tester_silent_collision   — buggy enum (no dedup)
//                                             → two phys ↦ same tester →
//                                               GetPhysicalIndex both hit slot0
//   [RED 2]   TC_buggy_dup_tester_cnt_still_two     — gu08TesterCnt counts both
//                                             → UI shows 2 "valid" ports but
//                                               the 2nd never runs its tasks
//   [GREEN 1] TC_fixed_dup_tester_2nd_slot_invalid   — fixed enum (dedup check)
//                                             → 2nd slot = UCHAR_MAX, cnt=1,
//                                               GetPhysicalIndex only matches 1
//   [GREEN 2] TC_fixed_unique_testers_work_normally  — distinct ids ↦ distinct
//                                               mappings, cnt correct.
//
//  Build:
//    cl /nologo /std:c++17 /EHsc p25_unique_tester_test.cpp
// =============================================================================
#include <windows.h>
#include <stdio.h>
#include <string.h>

#define MAX_DEVICE_CNT 16

// ---- counters ----------------------------------------------------------------
static int s_pass = 0, s_fail = 0;
#define CHECK(expr) do { if (expr) s_pass++; else { s_fail++; printf("[FAIL %s:%d] %s\n",__FILE__,__LINE__,#expr); } } while(0)

#define RUN_CASE(fn) do { int pb=s_pass,fb=s_fail; printf("\n=== %s ===\n", #fn); fn(); printf("  pass=%d fail=%d\n", s_pass-pb, s_fail-fb); } while(0)

// ---- core arrays (copy of production's static gu08TesterMap/gu08DeviceCnt)
struct Ctx {
    UCHAR gu08TesterMap[MAX_DEVICE_CNT];
    UCHAR gu08DeviceCnt;
    UCHAR gu08TesterCnt;
    void reset() {
        memset(gu08TesterMap, 0xFF, sizeof(gu08TesterMap));   // UCHAR_MAX fill
        gu08DeviceCnt = 0;
        gu08TesterCnt = 0;
    }
};

// ---- simulated EnumSm3350 output: for phys slot i, "UfsReadPortInfo" returns tester[i]
// ---- both buggy and fixed share the same input array, but differ in dedup logic.

// ---- BUGGY: matches production pre-fix EnumSm3350 (no uniqueness check) ---------
static void BuggyEnum(Ctx& c, const UCHAR testerPerSlot[], int n)
{
    c.gu08DeviceCnt = (UCHAR)n;
    for (int i = 0; i < n; i++) {
        UCHAR u08Id = testerPerSlot[i];                 // from UfsReadPortInfo
        if (u08Id < MAX_DEVICE_CNT && c.gu08TesterMap[i] == 0xFF) {
            c.gu08TesterMap[i] = u08Id;
            c.gu08TesterCnt++;
        }
    }
}

// ---- FIXED: dedup scan gu08TesterMap[0..i-1] before write ----------------------
static void FixedEnum(Ctx& c, const UCHAR testerPerSlot[], int n)
{
    c.gu08DeviceCnt = (UCHAR)n;
    for (int i = 0; i < n; i++) {
        UCHAR u08Id = testerPerSlot[i];
        if (u08Id >= MAX_DEVICE_CNT) continue;          // invalid id → skip (UCHAR_MAX stays)
        if (c.gu08TesterMap[i] != 0xFF) continue;       // already mapped → skip

        // P2-5 uniqueness check: linear scan previous slots (0..i-1) for the same id.
        bool dup = false;
        for (int j = 0; j < i; j++) {
            if (c.gu08TesterMap[j] == u08Id) { dup = true; break; }
        }
        if (dup) {
            // Production: optionally SetLastError(ERROR_DUPLICATE_TAG) or log.
            // Must keep c.gu08TesterMap[i] = UCHAR_MAX — never count this slot.
            continue;
        }
        c.gu08TesterMap[i] = u08Id;
        c.gu08TesterCnt++;
    }
}

// ---- simulated GetPhysicalIndex(TesterId) — exactly mirrors production (first-fit)
static int GetPhysicalIndex(const Ctx& c, UCHAR testerWant)
{
    if (testerWant >= MAX_DEVICE_CNT) return -1;
    for (UCHAR i = 0; i < c.gu08DeviceCnt; i++) {
        if (c.gu08TesterMap[i] == testerWant) return (int)i;
    }
    return -1;
}

// =============================================================================
// RED cases
// =============================================================================
static void TC_buggy_dup_tester_silent_collision()
{
    Ctx c; c.reset();
    // Two phys devices (n=2), both jumpers claim tester=5
    const UCHAR ids[] = { 5, 5 };
    BuggyEnum(c, ids, 2);

    // Both slots got mapped to tester=5 → silent corruption set up.
    CHECK(c.gu08TesterMap[0] == 5);
    CHECK(c.gu08TesterMap[1] == 5);   // <-- BUG: same tester-id, two phys slots

    // Now GetPhysicalIndex(tester=5) always hit phys=0.
    // If UI loop "for each tester port → run task": port for tester=5 maps to phys=0,
    // AND if phys=1 is also somehow used, it maps to tester=5 → again phys=0.
    int phyForTester5 = GetPhysicalIndex(c, 5);
    CHECK(phyForTester5 == 0);            // only phys=0 ever found

    // A naive "iterate gu08DeviceCnt and GetPhysicalIndex for tester=map[i]" pattern:
    int p0 = GetPhysicalIndex(c, c.gu08TesterMap[0]);
    int p1 = GetPhysicalIndex(c, c.gu08TesterMap[1]);
    CHECK(p0 == 0);
    CHECK(p1 == 0);                       // <-- second task ALSO routed to phys=0: slot 1串槽!
}

static void TC_buggy_dup_tester_cnt_still_two()
{
    Ctx c; c.reset();
    const UCHAR ids[] = { 5, 5 };
    BuggyEnum(c, ids, 2);

    // Buggy: gu08TesterCnt=2 → UI shows "2 tester ports", but only 1 physical
    // device actually receives work.  Users see "count correct" but output is wrong.
    CHECK(c.gu08TesterCnt == 2);
}

// =============================================================================
// GREEN cases
// =============================================================================
static void TC_fixed_dup_tester_2nd_slot_invalid()
{
    Ctx c; c.reset();
    const UCHAR ids[] = { 5, 5 };
    FixedEnum(c, ids, 2);

    // FIXED: only first slot accepted; second left UCHAR_MAX; cnt=1.
    CHECK(c.gu08TesterMap[0] == 5);
    CHECK(c.gu08TesterMap[1] == 0xFF);          // invalidated → no portIndex will map here
    CHECK(c.gu08TesterCnt == 1);

    // GetPhysicalIndex(tester=5) still works → only returns phys=0 (the only valid one)
    int phy5 = GetPhysicalIndex(c, 5);
    CHECK(phy5 == 0);
    int phyInvalid = GetPhysicalIndex(c, c.gu08TesterMap[1]); // UCHAR_MAX tester
    CHECK(phyInvalid == -1);                   // → port won't be used, zero 串槽
}

static void TC_fixed_unique_testers_work_normally()
{
    Ctx c; c.reset();
    const UCHAR ids[] = { 5, 9, 12 };
    FixedEnum(c, ids, 3);

    CHECK(c.gu08TesterMap[0] == 5);
    CHECK(c.gu08TesterMap[1] == 9);
    CHECK(c.gu08TesterMap[2] == 12);
    CHECK(c.gu08TesterCnt == 3);

    CHECK(GetPhysicalIndex(c, 5) == 0);
    CHECK(GetPhysicalIndex(c, 9) == 1);
    CHECK(GetPhysicalIndex(c, 12) == 2);
    CHECK(GetPhysicalIndex(c, 0xFF) == -1);
    CHECK(GetPhysicalIndex(c, 3) == -1);         // never assigned → invalid
}

// =============================================================================
int main()
{
    printf("P2-5 TDD: EnumSm3350 TesterId uniqueness dedup\n");
    printf("----------------------------------------------\n");

    RUN_CASE(TC_buggy_dup_tester_silent_collision);
    RUN_CASE(TC_buggy_dup_tester_cnt_still_two);
    RUN_CASE(TC_fixed_dup_tester_2nd_slot_invalid);
    RUN_CASE(TC_fixed_unique_testers_work_normally);

    printf("\n==========================================\n");
    printf("P25_CHECKSUMS: pass=%d fail=%d\n", s_pass, s_fail);
    printf("EXIT_CODE: %s\n", (s_fail == 0) ? "OK" : "FAIL");
    return (s_fail == 0) ? 0 : 1;
}
