// ============================================================
// P2-4 TDD: GetDeviceInfo(UCHAR) wrapper semantic mismatch.
// Bug:  Legacy wrapper GetDeviceInfo(UCHAR idRaw) interprets idRaw
//       as TesterId (via TesterId::FromRawChecked), while the
//       callers (e.g. Scan button in SparkUfsPdtDlg) intend the
//       SAME loop variable i to be PHYSICAL SLOT index (it is
//       also passed to GetTesterIndex(UCHAR) which wraps to
//       PhyIndex).  When testerId != physIndex this yields
//       cross-slot routing / missing devices in the UI.
// Fix:  Add a PhyIndex overload GetDeviceInfo(PhyIndex), and
//       repoint the legacy UCHAR wrapper to PhyIndex semantics
//       (matching GetTesterIndex wrapper).  Keep GetDeviceInfo
//       (TesterId) but implement it via reverse lookup
//       GetPhysicalIndex first, then delegate to the PhyIndex
//       overload.
//
// Build:
//   cl /nologo /std:c++17 /EHsc p24_getdeviceinfo_semantics_test.cpp
// Usage:
//   p24_getdeviceinfo_semantics_test.exe BUGGY
//   p24_getdeviceinfo_semantics_test.exe FIXED
// Exit: 0 on success, nonzero on any test failure.
// ============================================================
#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <string>

static constexpr int MAX_DEVICE_CNT = 16;
static constexpr unsigned char UCHAR_MAX_ = 0xFF;

struct MockDeviceInfo {
    int         diskNumber;   // "physical slot identity" for verification
    const char* driveName;    // e.g. "D:"
    bool        populated;
};

// ----------------------------------------------------------------
// BUGGY namespace:  mirrors CURRENT (buggy) production semantics
// ----------------------------------------------------------------
namespace buggy {

    static inline uint8_t gu08DeviceCnt = 0;
    static inline uint8_t gu08TesterMap[MAX_DEVICE_CNT];
    static inline MockDeviceInfo gstDeviceInfo[MAX_DEVICE_CNT];

    struct PhyIndex {
        bool    valid_;
        uint8_t val_;
        static PhyIndex FromRawChecked(uint8_t raw) {
            PhyIndex r; r.val_ = (raw < MAX_DEVICE_CNT) ? raw : 0;
            r.valid_ = (raw < MAX_DEVICE_CNT); return r;
        }
        bool IsValid() const { return valid_; }
        uint8_t value() const { return val_; }
    };

    struct TesterId {
        uint8_t val_;
        static TesterId FromRawChecked(uint8_t raw) {
            TesterId r; r.val_ = (raw < MAX_DEVICE_CNT) ? raw : (uint8_t)MAX_DEVICE_CNT; return r;
        }
        bool IsValid() const { return val_ < MAX_DEVICE_CNT; }
        uint8_t value() const { return val_; }
    };

    static inline PhyIndex GetPhysicalIndex(TesterId tester) noexcept {
        if (!tester.IsValid()) { PhyIndex p; p.valid_ = false; p.val_ = 0; return p; }
        for (uint8_t i = 0; i < gu08DeviceCnt; ++i) {
            if (gu08TesterMap[i] == tester.value()) return PhyIndex::FromRawChecked(i);
        }
        PhyIndex p; p.valid_ = false; p.val_ = 0; return p;
    }

    // Legacy wrappers - PROBLEM: GetDeviceInfo(UCHAR) wraps to TesterId,
    // but GetTesterIndex(UCHAR) wraps to PhyIndex (the SAME i means TWO
    // different things to the call-site!)
    static inline MockDeviceInfo* GetDeviceInfo(uint8_t idRaw) noexcept {
        // --- BUG: idRaw treated as TesterId, then we index gu08TesterMap[testerId] ---
        TesterId id = TesterId::FromRawChecked(idRaw);
        if (!id.IsValid()) return nullptr;
        uint8_t mapped = gu08TesterMap[id.value()];  // indexed by testerId (wrong!)
        if (mapped == UCHAR_MAX_) return nullptr;
        if (mapped >= gu08DeviceCnt) return nullptr;
        return &gstDeviceInfo[mapped];
    }

    static inline uint8_t GetTesterIndex(uint8_t idRaw) noexcept {
        PhyIndex p = PhyIndex::FromRawChecked(idRaw);  // idRaw treated as PhyIndex
        if (!p.IsValid()) return UCHAR_MAX_;
        return gu08TesterMap[p.value()];
    }

    static inline void SetupScenario() {
        memset(gu08TesterMap, 0xFF, sizeof gu08TesterMap);
        memset(gstDeviceInfo, 0, sizeof gstDeviceInfo);
        // 5 physical devices, non-1:1 mapping (the exact scenario from the bug report)
        gu08DeviceCnt = 5;
        gu08TesterMap[0] = 5;   // phys 0 -> tester 5 (Port 6), drive "D:"
        gu08TesterMap[1] = 2;   // phys 1 -> tester 2 (Port 3), drive "E:"
        gu08TesterMap[2] = 9;   // phys 2 -> tester 9 (Port 10), drive "F:"
        gu08TesterMap[3] = 0;   // phys 3 -> tester 0 (Port 1), drive "G:"
        gu08TesterMap[4] = 1;   // phys 4 -> tester 1 (Port 2), drive "H:"
        const char* drives[] = {"D:","E:","F:","G:","H:"};
        for (int i = 0; i < 5; i++) {
            gstDeviceInfo[i].diskNumber  = i;            // diskNumber IS physical slot here
            gstDeviceInfo[i].driveName   = drives[i];
            gstDeviceInfo[i].populated   = true;
        }
    }
}

// ----------------------------------------------------------------
// FIXED namespace: mirrors FIXED production semantics
// ----------------------------------------------------------------
namespace fixed_ns {

    static inline uint8_t gu08DeviceCnt = 0;
    static inline uint8_t gu08TesterMap[MAX_DEVICE_CNT];
    static inline MockDeviceInfo gstDeviceInfo[MAX_DEVICE_CNT];

    struct PhyIndex {
        bool    valid_;
        uint8_t val_;
        static PhyIndex FromRawChecked(uint8_t raw) {
            PhyIndex r; r.val_ = (raw < MAX_DEVICE_CNT) ? raw : 0;
            r.valid_ = (raw < MAX_DEVICE_CNT); return r;
        }
        bool IsValid() const { return valid_; }
        uint8_t value() const { return val_; }
    };

    struct TesterId {
        uint8_t val_;
        static TesterId FromRawChecked(uint8_t raw) {
            TesterId r; r.val_ = (raw < MAX_DEVICE_CNT) ? raw : (uint8_t)MAX_DEVICE_CNT; return r;
        }
        bool IsValid() const { return val_ < MAX_DEVICE_CNT; }
        uint8_t value() const { return val_; }
    };

    static inline PhyIndex GetPhysicalIndex(TesterId tester) noexcept {
        if (!tester.IsValid()) { PhyIndex p; p.valid_ = false; p.val_ = 0; return p; }
        for (uint8_t i = 0; i < gu08DeviceCnt; ++i) {
            if (gu08TesterMap[i] == tester.value()) return PhyIndex::FromRawChecked(i);
        }
        PhyIndex p; p.valid_ = false; p.val_ = 0; return p;
    }

    // --- FIX 1/3: primary GetDeviceInfo takes PhyIndex (O(1) direct index) ---
    static inline MockDeviceInfo* GetDeviceInfo(PhyIndex phys) noexcept {
        if (!phys.IsValid()) return nullptr;
        uint8_t i = phys.value();
        if (i >= gu08DeviceCnt) return nullptr;
        return &gstDeviceInfo[i];
    }

    // --- FIX 2/3: GetDeviceInfo(TesterId) does reverse lookup via GetPhysicalIndex ---
    static inline MockDeviceInfo* GetDeviceInfo(TesterId id) noexcept {
        PhyIndex p = GetPhysicalIndex(id);
        if (!p.IsValid()) return nullptr;
        return GetDeviceInfo(p);
    }

    // --- FIX 3/3: legacy UCHAR wrapper now mirrors GetTesterIndex(UCHAR) - both wrap to PhyIndex ---
    static inline MockDeviceInfo* GetDeviceInfo(uint8_t idRaw) noexcept {
        return GetDeviceInfo(PhyIndex::FromRawChecked(idRaw));
    }

    static inline uint8_t GetTesterIndex(uint8_t idRaw) noexcept {
        PhyIndex p = PhyIndex::FromRawChecked(idRaw);
        if (!p.IsValid()) return UCHAR_MAX_;
        return gu08TesterMap[p.value()];
    }

    static inline void SetupScenario() {
        memset(gu08TesterMap, 0xFF, sizeof gu08TesterMap);
        memset(gstDeviceInfo, 0, sizeof gstDeviceInfo);
        gu08DeviceCnt = 5;
        gu08TesterMap[0] = 5;
        gu08TesterMap[1] = 2;
        gu08TesterMap[2] = 9;
        gu08TesterMap[3] = 0;
        gu08TesterMap[4] = 1;
        const char* drives[] = {"D:","E:","F:","G:","H:"};
        for (int i = 0; i < 5; i++) {
            gstDeviceInfo[i].diskNumber  = i;
            gstDeviceInfo[i].driveName   = drives[i];
            gstDeviceInfo[i].populated   = true;
        }
    }
}

// ----------------------------------------------------------------
// Test framework helpers
// ----------------------------------------------------------------
struct TestCtx { int pass; int fail; char msg[200]; };
typedef int(*TestFn)(TestCtx*);

#define TEST_MSG(...) _snprintf_s(ctx->msg, _countof(ctx->msg), _TRUNCATE, __VA_ARGS__)
#define RUN(fn) do { int r = (fn)(&ctx); if (r) { ctx.pass++; printf("[PASS] %s\n", #fn); } else { ctx.fail++; printf("[FAIL] %s: %s\n", #fn, ctx.msg[0] ? ctx.msg : "(no message)"); } } while(0)

// Expectation for Scan loop (i=0..MAX_DEVICE_CNT-1):
// For each iteration, we verify GetDeviceInfo(i) diskNumber against expected.
// If expected == -1 we expect GetDeviceInfo to return nullptr.
static int verifyScanLoop(
    TestCtx* ctx,
    const char* mode,
    MockDeviceInfo* (*getInfo)(uint8_t),
    uint8_t(*getTester)(uint8_t),
    const int expectedDiskPerIter[16],
    const int expectedTesterPerIter[16])
{
    int ok = 1;
    for (int i = 0; i < MAX_DEVICE_CNT && ok; i++) {
        MockDeviceInfo* pInfo = getInfo((uint8_t)i);
        int actualDisk = pInfo ? pInfo->diskNumber : -1;
        if (actualDisk != expectedDiskPerIter[i]) {
            TEST_MSG("%s iter i=%d: GetDeviceInfo diskNumber: expected %d got %d", mode, i, expectedDiskPerIter[i], actualDisk);
            ok = 0; break;
        }
        uint8_t actualTester = getTester((uint8_t)i);
        if ((int)actualTester != expectedTesterPerIter[i]) {
            TEST_MSG("%s iter i=%d: GetTesterIndex: expected %d got %d", mode, i, expectedTesterPerIter[i], (int)actualTester);
            ok = 0; break;
        }
    }
    return ok;
}

// ---------------- BUGGY mode expected misrouting (the failure we want to detect) ----------------
// BUGGY: GetDeviceInfo(UCHAR i) -> TesterId(i) -> gu08TesterMap[i] = mapped tester value, then gstDeviceInfo[mapped]
static int TC_buggy_scan_misroute(TestCtx* ctx) {
    buggy::SetupScenario();
    // Expected for BUGGY behavior (misrouting confirmed by manual earlier walkthrough):
    // i=0: TesterId(0)->gu08TesterMap[0]=5 -> gstDeviceInfo[5] unpopulated (gu08DeviceCnt=5) -> nullptr (-1)
    // i=1: TesterId(1)->gu08TesterMap[1]=2 -> gstDeviceInfo[2] -> disk=2 (phys2 F:)
    // i=2: TesterId(2)->gu08TesterMap[2]=9 -> gstDeviceInfo[9] out of range (gu08DeviceCnt=5) -> -1
    // i=3: TesterId(3)->gu08TesterMap[3]=0 -> gstDeviceInfo[0] -> disk=0 (phys0 D:)
    // i=4: TesterId(4)->gu08TesterMap[4]=1 -> gstDeviceInfo[1] -> disk=1 (phys1 E:)
    // i=5..15: TesterId(i)->gu08TesterMap[i] = 0xFF -> -1
    int expDisk[16]   = {-1, 2,-1, 0, 1, -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1};
    int expTester[16] = { 5, 2, 9, 0, 1, 255,255,255,255,255,255,255,255,255,255,255 };  // GetTesterIndex(PhyIndex(i)) always ok
    return verifyScanLoop(ctx, "BUGGY", buggy::GetDeviceInfo, buggy::GetTesterIndex, expDisk, expTester);
}

// ---------------- FIXED mode expected correct routing ----------------
static int TC_fixed_scan_correct(TestCtx* ctx) {
    fixed_ns::SetupScenario();
    // FIXED: GetDeviceInfo(UCHAR i) -> PhyIndex(i) -> gstDeviceInfo[i] (O(1) direct)
    // So diskNumber == i == physical slot identity for 0..4, nullptr elsewhere
    int expDisk[16]   = { 0, 1, 2, 3, 4, -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1 };
    int expTester[16] = { 5, 2, 9, 0, 1, 255,255,255,255,255,255,255,255,255,255,255 };
    return verifyScanLoop(ctx, "FIXED", fixed_ns::GetDeviceInfo, fixed_ns::GetTesterIndex, expDisk, expTester);
}

// ---------------- Additional: ensure GetDeviceInfo(TesterId) does reverse lookup correctly ----------------
static int TC_fixed_testerid_reverse_lookup(TestCtx* ctx) {
    fixed_ns::SetupScenario();
    // TesterId(0) -> phys3 -> disk 3 (G:)
    MockDeviceInfo* p0 = fixed_ns::GetDeviceInfo(fixed_ns::TesterId::FromRawChecked(0));
    if (!p0 || p0->diskNumber != 3) { TEST_MSG("tester0: expected disk 3 got %p/%d", p0, p0?p0->diskNumber:-1); return 0; }
    // TesterId(1) -> phys4 -> disk 4 (H:)
    MockDeviceInfo* p1 = fixed_ns::GetDeviceInfo(fixed_ns::TesterId::FromRawChecked(1));
    if (!p1 || p1->diskNumber != 4) { TEST_MSG("tester1: expected disk 4 got %p/%d", p1, p1?p1->diskNumber:-1); return 0; }
    // TesterId(5) -> phys0 -> disk 0 (D:)
    MockDeviceInfo* p5 = fixed_ns::GetDeviceInfo(fixed_ns::TesterId::FromRawChecked(5));
    if (!p5 || p5->diskNumber != 0) { TEST_MSG("tester5: expected disk 0 got %p/%d", p5, p5?p5->diskNumber:-1); return 0; }
    // TesterId(9) -> phys2 -> disk 2 (F:)
    MockDeviceInfo* p9 = fixed_ns::GetDeviceInfo(fixed_ns::TesterId::FromRawChecked(9));
    if (!p9 || p9->diskNumber != 2) { TEST_MSG("tester9: expected disk 2 got %p/%d", p9, p9?p9->diskNumber:-1); return 0; }
    // TesterId(12) -> unallocated -> nullptr
    MockDeviceInfo* p12 = fixed_ns::GetDeviceInfo(fixed_ns::TesterId::FromRawChecked(12));
    if (p12 != nullptr) { TEST_MSG("tester12: expected nullptr got disk %d", p12->diskNumber); return 0; }
    return 1;
}

// ---------------- Additional: BUGGY GetDeviceInfo(TesterId) would ALSO misroute, we must confirm the current harness buggy impl matches production (no TesterId overload - use the explicit UCHAR wrapper only) ----------------
static int TC_fixed_both_semantics_distinct_types(TestCtx* ctx) {
    // In FIXED, passing PhyIndex vs TesterId MUST produce different return values
    // for the SAME raw value when phys != tester.  Eg:
    //   GetDeviceInfo(PhyIndex(0)) => phys0 => disk 0
    //   GetDeviceInfo(TesterId(0)) => tester0 => phys3 => disk 3
    fixed_ns::SetupScenario();
    auto* pPhy0 = fixed_ns::GetDeviceInfo(fixed_ns::PhyIndex::FromRawChecked(0));
    auto* pTst0 = fixed_ns::GetDeviceInfo(fixed_ns::TesterId::FromRawChecked(0));
    if (!pPhy0 || !pTst0) { TEST_MSG("both should be non-null"); return 0; }
    if (pPhy0->diskNumber != 0)  { TEST_MSG("PhyIndex(0) disk: expected 0 got %d", pPhy0->diskNumber); return 0; }
    if (pTst0->diskNumber != 3)  { TEST_MSG("TesterId(0) disk: expected 3 got %d", pTst0->diskNumber); return 0; }
    // Compile-time guarantee: these are DIFFERENT overloads; callers who pass a PhyIndex
    // object can never accidentally hit the TesterId path (and vice-versa).
    return 1;
}

int main(int argc, char** argv) {
    const char* mode = (argc > 1) ? argv[1] : "BUGGY";
    TestCtx ctx = {0,0,{0}};
    int mismatch = _stricmp(mode, "fixed") == 0 ? 0 : 1;

    if (mismatch) {
        // BUGGY phase: confirm buggy behavior (the test passes if misrouting matches expectations)
        RUN(TC_buggy_scan_misroute);
        // Also sanity-check fixed harness works in BUGGY phase (it should still behave correctly)
        RUN(TC_fixed_scan_correct);
        RUN(TC_fixed_testerid_reverse_lookup);
        RUN(TC_fixed_both_semantics_distinct_types);
    } else {
        // FIXED phase: all four should pass
        RUN(TC_buggy_scan_misroute);
        RUN(TC_fixed_scan_correct);
        RUN(TC_fixed_testerid_reverse_lookup);
        RUN(TC_fixed_both_semantics_distinct_types);
    }

    printf("P24_CHECKSUMS: pass=%d fail=%d\n", ctx.pass, ctx.fail);
    return (ctx.fail == 0) ? 0 : 1;
}
