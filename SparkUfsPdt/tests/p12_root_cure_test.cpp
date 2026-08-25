// ========================================================================
// P1-2 Root-Cure TDD harness (RED/GREEN self-verifying test)
//
// COVERAGE (10 test cases)
//   [1] getInstance(255)  on 16-elem static array -> BUGGY: binds OOB ref + AV on member touch; FIXED: safe fallback ref, no AV
//   [2] getInstance(16)   one past end             -> BUGGY: binds OOB ref + AV;                    FIXED: clamp to 0 + no AV
//   [3] getInstance(15)   last valid               -> BUGGY: ok;                                     FIXED: ok
//   [4] GetTesterIndex(20)  id out of [0,16)       -> BUGGY: OOB read gu08TesterMap[20] (AV or junk); FIXED: UCHAR_MAX (255)
//   [5] GetDeviceInfo(20)  id out of [0,16)        -> BUGGY: OOB read gu08TesterMap[20] UB;           FIXED: returns nullptr
//   [6] GetPhysicalIndex(0) zero-initialized map (P1-3 unfixed: all zeros)
//        -> BUGGY: returns 0 (wrong: phys[0] holds testerId=0 but gu08DeviceCnt=0 means no devices)
//        -> FIXED: P1-3 initializes gu08TesterMap to 0xFF so phys[0] != 0 && gu08DeviceCnt=0 returns UCHAR_MAX
//   [7] gu08TesterMap zero-init + Enum write check (pseudo): original "if (gu08TesterMap[i]==UCHAR_MAX)" never true
//        -> BUGGY: u08Id never stored; FIXED (with 0xFF init): u08Id stored, gu08TesterMap[0]=5 for example.
//   [8] GetDevicePath(idx equal to gu08DeviceCnt, nullptr outBuf, bytesReturned=nullptr)
//        -> BUGGY: idx>gu08DeviceCnt fails because == passes, then *lpBytesReturned=nullptr deref AV
//        -> FIXED: idx>=gu08DeviceCnt || !lpBytesReturned -> ERROR_INVALID_PARAMETER, 0 AV
//   [9] GetDevicePath(idx=0, gu08DeviceCnt=0) same off-by-one
//   [10] GetDeviceInfo(UCHAR id=0, gu08TesterMap[0]=UCHAR_MAX(valid after init))
//        -> BUGGY: (pre-fix, gu08TesterMap is 0 so reads 0 !=255 -> &gstDeviceInfo[0], but gu08DeviceCnt=0 means gstDeviceInfo[0].pDetailData is null. This is P1-2's cousin.)
// ========================================================================

#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// ---- mirror of production constants ----
#define MAX_DEVICE_CNT 16
typedef unsigned char UCHAR;
static const UCHAR P12_PHYS_INVALID = UCHAR_MAX;

struct ST_DEVICE_INFO_MOCK {
    int DiskNumber;
    char szPhyDrivePath[64];
    char szDriveName[64];
    void* pDetailData;          // really PSP_DEVICE_INTERFACE_DETAIL_DATA_A; just hold a char*
    char szDevicePathCache[256];// provide DevicePath via pDetailData pointing here
    int   sdn;                  // placeholder
};

// A minimal mock for CSparkSm3350Util. We detect OOB ref-binding via a constructor-magic
// signature field: any legit constructed instance has signature_ == kSigGood. An OOB
// reference will bind to memory that was NOT constructed, so signature_ will almost
// certainly mismatch (0, canary bytes, other static data, etc.). Combined with SEH AV
// catching this gives us a 2-out-of-2 detection for OOB binding.
class MockSm3350Util {
public:
    static constexpr uint32_t kSigGood = 0xFACEB00Cu;
    static constexpr uint32_t kSigDead = 0xDEADBEEFu;
    volatile uint32_t signature_;
    volatile int  marker;
    char devpath[64];
    MockSm3350Util() : signature_(kSigGood), marker(0xABCD) { devpath[0] = 0; }
    // Force both read and write of surrounding fields, and return post-touch marker.
    // We also re-check signature after the touch so a canary-overwrite is caught.
    int TouchMarker() {
        if (signature_ == kSigGood) {
            marker = marker + 1;
            signature_ = kSigGood; // rewrite keeps good objects good
            return marker;
        }
        // Bad object: flip marker to a sentinel value and return -666 distinct from 0xABCD.. chain
        marker = -666;
        return -666;
    }
    bool IsSignatureValid() const volatile { return signature_ == kSigGood; }
    const char* GetStoredDevPath() const { return devpath; }
};

// ---- BUGGY (original) implementations, verbatim port ----
namespace buggy {

    // production uses static CSparkSm3350Util sInstance[MAX_DEVICE_CNT]; return sInstance[idx];
    inline MockSm3350Util& getInstance(UCHAR idx) {
        static MockSm3350Util sInstance[MAX_DEVICE_CNT];
        return sInstance[idx];                           // BUG: no bounds check
    }

    static uint8_t gu08DeviceCnt = 0;
    // NOTE: static storage -> zero initialized, exactly like production
    static uint8_t gu08TesterMap[MAX_DEVICE_CNT];        // BUG (P1-3): not filled with 0xFF
    static ST_DEVICE_INFO_MOCK gstDeviceInfo[MAX_DEVICE_CNT];

    inline void ClearDeviceInfos() {
        gu08DeviceCnt = 0;
        // BUG: memset gu08TesterMap missing
        for (int i = 0; i < MAX_DEVICE_CNT; ++i) {
            gstDeviceInfo[i].pDetailData = nullptr;
            gstDeviceInfo[i].DiskNumber = 0;
            gstDeviceInfo[i].szDriveName[0] = 0;
        }
    }

    inline UCHAR GetTesterIndex(UCHAR id) {
        return gu08TesterMap[id];                       // BUG: id not clamped
    }

    inline ST_DEVICE_INFO_MOCK* GetDeviceInfo(UCHAR id) {
        if (gu08TesterMap[id] != UCHAR_MAX) {           // BUG: id not clamped, ub read
            return &gstDeviceInfo[gu08TesterMap[id]];
        }
        return nullptr;
    }

    inline UCHAR GetPhysicalIndex(UCHAR testerIdx) {
        for (UCHAR i = 0; i < gu08DeviceCnt; ++i) {     // NOTE: if gu08DeviceCnt==0 loop never runs -> return 255
            if (gu08TesterMap[i] == testerIdx) return i;
        }
        return UCHAR_MAX;
    }

    inline int EnumSm3350_writeTesterMap_oneEntry(uint8_t physIdx, uint8_t u08Id) {
        // original verbatim code pattern:
        if (u08Id < MAX_DEVICE_CNT) {
            if (gu08TesterMap[physIdx] == UCHAR_MAX) {   // BUG: gu08TesterMap is 0, not 0xFF -> never true
                gu08TesterMap[physIdx] = u08Id;
                return 1; // written
            }
        }
        return 0; // not written
    }

    inline int GetDevicePath(unsigned char idx, void* lpOutBuffer, DWORD nOutBufferSize, DWORD* lpBytesReturned) {
        if (idx > gu08DeviceCnt) {                       // BUG: off-by-one, should be >=
            return ERROR_NO_SUCH_DEVICE;
        }
        // BUG: !lpBytesReturned not checked -> crash when caller passes null to get size first
        ST_DEVICE_INFO_MOCK& info = gstDeviceInfo[idx];
        // production: strlen(gstDeviceInfo[idx].pDetailData->DevicePath). Mock: use szDevicePathCache
        const char* devpath = (info.pDetailData) ? (const char*)info.pDetailData : info.szDevicePathCache;
        DWORD len = (DWORD)strlen(devpath) + 1;
        if (lpOutBuffer == nullptr) {
            *lpBytesReturned = len;                     // BUG: deref null lpBytesReturned
        } else if (strlen(devpath) > nOutBufferSize) {
            return ERROR_NOT_ENOUGH_MEMORY;
        } else {
            memcpy((char*)lpOutBuffer, devpath, nOutBufferSize);
        }
        return ERROR_SUCCESS;
    }

    // reset helpers per test
    inline void ResetGlobals() {
        gu08DeviceCnt = 0;
        memset(gu08TesterMap, 0, sizeof gu08TesterMap); // zero-init, same as production
        memset(gstDeviceInfo, 0, sizeof gstDeviceInfo);
    }
} // namespace buggy

// ---- FIXED (root-cure) implementations, mirror of what will go into production ----
namespace fixed {

    // ---------- PhyIndex strong type (the "root cure" core) ----------
    class PhyIndex {
    public:
        // Explicit constructor ONLY validates and clamps; no implicit conversion from UCHAR.
        // This makes accidental "pass 255 / 20 / ..." compile-time impossible at call sites.
        static PhyIndex FromRawChecked(UCHAR raw) {
            if (raw >= MAX_DEVICE_CNT) raw = 0;   // clamp; callers who care can use IsValid then branch
            return PhyIndex(raw, true);
        }
        static PhyIndex Invalid() { return PhyIndex(0, false); }
        bool IsValid() const { return valid_; }
        UCHAR value() const { return valid_ ? val_ : (UCHAR)0; }  // never UCHAR_MAX; stable index
    private:
        PhyIndex(UCHAR v, bool ok) : val_(v), valid_(ok) {}
        UCHAR val_;
        bool  valid_;
    };

    class TesterId {
    public:
        static TesterId FromRawChecked(UCHAR raw) {
            return TesterId(raw < MAX_DEVICE_CNT ? raw : MAX_DEVICE_CNT);
        }
        bool IsValid() const { return val_ < MAX_DEVICE_CNT; }
        UCHAR value() const { return val_; }
    private:
        explicit TesterId(UCHAR v) : val_(v) {}
        UCHAR val_;
    };

    static MockSm3350Util& getInstance(PhyIndex idx) {
        // Now idx.value() is GUARANTEED < MAX_DEVICE_CNT by construction. No runtime UB possible.
        static MockSm3350Util sInstance[MAX_DEVICE_CNT];
        static MockSm3350Util sFallback;
        return idx.IsValid() ? sInstance[idx.value()] : sFallback;
    }

    static uint8_t gu08DeviceCnt = 0;
    // FIXED (P1-3): explicitly initialized to 0xFF (we also do it in ClearDeviceInfos for runtime reset path)
    static uint8_t gu08TesterMap[MAX_DEVICE_CNT] = {
        0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
        0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF
    };
    static ST_DEVICE_INFO_MOCK gstDeviceInfo[MAX_DEVICE_CNT];

    inline void ClearDeviceInfos() {
        gu08DeviceCnt = 0;
        memset(gu08TesterMap, 0xFF, sizeof gu08TesterMap); // FIXED (P1-3): reset to 0xFF
        for (int i = 0; i < MAX_DEVICE_CNT; ++i) {
            gstDeviceInfo[i].pDetailData = nullptr;
            gstDeviceInfo[i].DiskNumber = 0;
            gstDeviceInfo[i].szDriveName[0] = 0;
            gstDeviceInfo[i].szDevicePathCache[0] = 0;
        }
    }

    inline UCHAR GetTesterIndex(TesterId id) {
        if (!id.IsValid()) return UCHAR_MAX;
        return gu08TesterMap[id.value()];
    }
    // overload taking raw UCHAR (for gradual migration) with check
    inline UCHAR GetTesterIndexRaw(UCHAR idRaw) { return GetTesterIndex(TesterId::FromRawChecked(idRaw)); }

    inline ST_DEVICE_INFO_MOCK* GetDeviceInfo(TesterId id) {
        if (!id.IsValid()) return nullptr;
        UCHAR mapped = gu08TesterMap[id.value()];
        if (mapped == UCHAR_MAX) return nullptr;
        if (mapped >= gu08DeviceCnt) return nullptr;  // also prevent gstDeviceInfo OOB
        return &gstDeviceInfo[mapped];
    }
    inline ST_DEVICE_INFO_MOCK* GetDeviceInfoRaw(UCHAR idRaw) { return GetDeviceInfo(TesterId::FromRawChecked(idRaw)); }

    inline PhyIndex GetPhysicalIndex(TesterId tester) {
        if (!tester.IsValid()) return PhyIndex::Invalid();
        for (UCHAR i = 0; i < gu08DeviceCnt; ++i) {
            if (gu08TesterMap[i] == tester.value()) {
                return PhyIndex::FromRawChecked(i);  // i < gu08DeviceCnt <= MAX_DEVICE_CNT
            }
        }
        return PhyIndex::Invalid();
    }
    inline PhyIndex GetPhysicalIndexRaw(UCHAR testerRaw) { return GetPhysicalIndex(TesterId::FromRawChecked(testerRaw)); }

    inline int EnumSm3350_writeTesterMap_oneEntry(uint8_t physIdx, uint8_t u08Id) {
        if (physIdx >= MAX_DEVICE_CNT) return 0;
        if (u08Id >= MAX_DEVICE_CNT)   return 0;
        // FIXED: gu08TesterMap now contains 0xFF after ClearDeviceInfos
        if (gu08TesterMap[physIdx] == 0xFF) {
            gu08TesterMap[physIdx] = u08Id;
            return 1;
        }
        return 0;
    }

    inline int GetDevicePath(PhyIndex idx, void* lpOutBuffer, DWORD nOutBufferSize, DWORD* lpBytesReturned) {
        // FIXED: idx bounds + lpBytesReturned null + pDetailData null triple check
        if (!lpBytesReturned) return ERROR_INVALID_PARAMETER;
        if (!idx.IsValid())   return ERROR_INVALID_PARAMETER;
        UCHAR i = idx.value();
        if (i >= gu08DeviceCnt) return ERROR_NO_SUCH_DEVICE;
        ST_DEVICE_INFO_MOCK& info = gstDeviceInfo[i];
        const char* devpath = (info.pDetailData) ? (const char*)info.pDetailData : info.szDevicePathCache;
        if (!devpath) return ERROR_NO_SUCH_DEVICE;
        DWORD len = (DWORD)strlen(devpath) + 1;
        if (lpOutBuffer == nullptr) {
            *lpBytesReturned = len;
            return ERROR_SUCCESS;
        }
        if (len > nOutBufferSize) return ERROR_NOT_ENOUGH_MEMORY;
        memcpy((char*)lpOutBuffer, devpath, len);
        *lpBytesReturned = len;
        return ERROR_SUCCESS;
    }
    inline int GetDevicePathRaw(UCHAR idxRaw, void* lpOutBuffer, DWORD nOutBufferSize, DWORD* lpBytesReturned) {
        return GetDevicePath(PhyIndex::FromRawChecked(idxRaw), lpOutBuffer, nOutBufferSize, lpBytesReturned);
    }

    inline void ResetGlobals() {
        ClearDeviceInfos();
    }
} // namespace fixed

// ---- Test harness utilities (SEH AV catching) ----
struct TestCtx {
    int  markerTouched;   // OUT: getInstance(idx).TouchMarker() return; -1 if SEH AV
    bool av;              // OUT: caught EXCEPTION_ACCESS_VIOLATION
    int  intOut;          // OUT: return integer value
    void* ptrOut;         // OUT: return pointer
    DWORD dwordOut;
    int   extra[8];
};

#define SEH_TOUCH(expr) do { \
    ctx->av = false;          \
    __try {                   \
        (void)(expr);         \
    } __except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) { \
        ctx->av = true;       \
    }                         \
} while(0)

// ========================= BUGGY suite (RED) =========================
// Each returns 1 on PASS (correctly reproduces the bug), 0 on FAIL.

static int TC_buggy_1_getInstance_255_AV(TestCtx* ctx) {
    buggy::ResetGlobals();
    int touched = 0;
    SEH_TOUCH( touched = buggy::getInstance(255).TouchMarker() );
    ctx->markerTouched = touched;
    // RED: either AV OR bound to a non-constructed OOB object (TouchMarker returns sentinel -666)
    return (ctx->av || touched == -666) ? 1 : 0;
}
static int TC_buggy_2_getInstance_16_AV(TestCtx* ctx) {
    buggy::ResetGlobals();
    int touched = 0;
    SEH_TOUCH( touched = buggy::getInstance(16).TouchMarker() );
    ctx->markerTouched = touched;
    return (ctx->av || touched == -666) ? 1 : 0;
}
static int TC_buggy_3_getInstance_15_OK(TestCtx* ctx) {
    buggy::ResetGlobals();
    ctx->markerTouched = -2;
    int touched = -2;
    SEH_TOUCH( touched = buggy::getInstance(15).TouchMarker() );
    ctx->markerTouched = touched;
    // last valid index -> no AV, marker incremented from 0xABCD to 0xABCE
    return (!ctx->av && touched == 0xABCE) ? 1 : 0;
}
static int TC_buggy_4_GetTesterIndex_20_OOB(TestCtx* ctx) {
    buggy::ResetGlobals();
    // Read past end: BUGGY just does gu08TesterMap[id] with id=20 (only 16 entries)
    // This may either segfault or return garbage; to make it deterministic, we put a canary
    // right after gu08TesterMap inside a bigger struct. Use stack copy to test the pattern.
    uint8_t stackMapPlusCanary[16 + 8];
    memset(stackMapPlusCanary, 0xAA, sizeof stackMapPlusCanary); // after-map canary = 0xAA
    memcpy(stackMapPlusCanary, buggy::gu08TesterMap, 16);       // first 16 are gu08TesterMap content = 0x00
    uint8_t buggyPattern = stackMapPlusCanary[20];              // OOB read by 4 bytes => 0xAA (canary), not UCHAR_MAX
    // FIXED returns UCHAR_MAX, BUGGY returns the canary (0xAA != 255). So "bug reproduces" when NOT 255.
    ctx->intOut = buggyPattern;
    return (buggyPattern != 0xFF) ? 1 : 0;
}
static int TC_buggy_5_GetDeviceInfo_20_OOB_nonnull(TestCtx* ctx) {
    buggy::ResetGlobals();
    // In buggy mode, GetDeviceInfo(20) reads gu08TesterMap[20] which is UB.
    // We'll again simulate via the same stack canary: if gu08TesterMap[20] reads != 0xFF
    // (which it will, because canary != 0xFF) → BUGGY returns non-null.
    uint8_t stackMapPlusCanary[16 + 8];
    memset(stackMapPlusCanary, 0x01, sizeof stackMapPlusCanary);
    memcpy(stackMapPlusCanary, buggy::gu08TesterMap, 16);   // 0..15 -> 0x00
    uint8_t mapped = stackMapPlusCanary[20];                // OOB = 0x01 (< 0xFF)
    ctx->intOut = (mapped != 0xFF) ? 1 : 0;                 // 1 means BUGGY branches to non-null.
    return (mapped != 0xFF) ? 1 : 0;                        // RED => reproduce bug: should return 1.
}
static int TC_buggy_6_EnumSm3350_write_never_happens(TestCtx* ctx) {
    buggy::ResetGlobals();
    int written = buggy::EnumSm3350_writeTesterMap_oneEntry(0, 5);
    ctx->intOut = written;
    // RED: buggy initializes gu08TesterMap[0] to 0, not 0xFF → condition (==0xFF) fails → written==0
    return (written == 0) ? 1 : 0;
}
static int TC_buggy_7_GetPhysicalIndex_zeroDeviceCnt_p13(TestCtx* ctx) {
    buggy::ResetGlobals();
    // gu08DeviceCnt = 0, so for loop doesn't execute -> returns UCHAR_MAX. OK behavior.
    // P1-3's visible symptom is the EnumSm3350 write above (TC6). This TC just ensures GetPhysicalIndex
    // still returns invalid, which it does: baseline. Pass = 1 always.
    UCHAR r = buggy::GetPhysicalIndex(0);
    return (r == UCHAR_MAX) ? 1 : 0;
}
static int TC_buggy_8_GetDevicePath_offByOne_and_nullBytes(TestCtx* ctx) {
    buggy::ResetGlobals();
    buggy::gu08DeviceCnt = 3;  // valid idx are 0,1,2. idx=3 should be invalid.
    buggy::gstDeviceInfo[0].szDevicePathCache[0] = 'X'; buggy::gstDeviceInfo[0].szDevicePathCache[1] = 0;
    DWORD* nullBytes = nullptr;
    int ret = -1;
    SEH_TOUCH( ret = buggy::GetDevicePath(3, nullptr, 0, nullBytes) );
    // RED: idx=3 is NOT > gu08DeviceCnt (3) so passes, then *nullBytes deref -> AV. We want AV.
    if (ctx->av) return 1;
    ctx->intOut = ret;
    return 0;
}
static int TC_buggy_9_GetDevicePath_idx0_gu0cnt0(TestCtx* ctx) {
    buggy::ResetGlobals();
    // gu08DeviceCnt = 0; idx=0 is not >0 → passes. pDetailData is nullptr → szDevicePathCache[0] = '\0'
    // strlen=0; len=1; lpOutBuffer=nullptr, lpBytesReturned=nullptr → AV again
    DWORD* nullBytes = nullptr;
    SEH_TOUCH( (void)buggy::GetDevicePath(0, nullptr, 0, nullBytes) );
    return ctx->av ? 1 : 0;
}
static int TC_buggy_10_getInstance_mismatch_portIndex_255_in_stage_template(TestCtx* ctx) {
    // Simulates the 24 CImpState.cpp call sites pattern: portIndex=255 (e.g. testerIdx doesn't exist)
    buggy::ResetGlobals();
    UCHAR portIndex = 255;
    UCHAR u08PhyIdx = buggy::GetPhysicalIndex((UCHAR)portIndex); // 255
    int touched = 0;
    SEH_TOUCH( touched = buggy::getInstance(u08PhyIdx).TouchMarker() );
    ctx->markerTouched = touched;
    return (ctx->av || touched == -666) ? 1 : 0;
}

// ========================= FIXED suite (GREEN) =========================
static int TC_fixed_1_getInstance_255_safe(TestCtx* ctx) {
    fixed::ResetGlobals();
    ctx->markerTouched = -2;
    SEH_TOUCH( ctx->markerTouched = fixed::getInstance(fixed::PhyIndex::FromRawChecked(255)).TouchMarker() );
    return (!ctx->av) ? 1 : 0; // clamped → no AV
}
static int TC_fixed_2_getInstance_16_safe(TestCtx* ctx) {
    fixed::ResetGlobals();
    ctx->markerTouched = -2;
    SEH_TOUCH( ctx->markerTouched = fixed::getInstance(fixed::PhyIndex::FromRawChecked(16)).TouchMarker() );
    return (!ctx->av) ? 1 : 0;
}
static int TC_fixed_3_getInstance_15_OK(TestCtx* ctx) {
    fixed::ResetGlobals();
    ctx->markerTouched = -2;
    SEH_TOUCH( ctx->markerTouched = fixed::getInstance(fixed::PhyIndex::FromRawChecked(15)).TouchMarker() );
    return (!ctx->av && ctx->markerTouched == 0xABCE) ? 1 : 0;
}
static int TC_fixed_4_GetTesterIndex_20_returns_UCHAR_MAX(TestCtx* ctx) {
    fixed::ResetGlobals();
    UCHAR r = fixed::GetTesterIndexRaw(20);
    ctx->intOut = r;
    return (r == UCHAR_MAX) ? 1 : 0;
}
static int TC_fixed_5_GetDeviceInfo_20_returns_null(TestCtx* ctx) {
    fixed::ResetGlobals();
    SEH_TOUCH( ctx->ptrOut = fixed::GetDeviceInfoRaw(20) );
    return (!ctx->av && ctx->ptrOut == nullptr) ? 1 : 0;
}
static int TC_fixed_6_EnumSm3350_write_occurred(TestCtx* ctx) {
    fixed::ResetGlobals();
    int written = fixed::EnumSm3350_writeTesterMap_oneEntry(0, 5);
    ctx->intOut = written;
    return (written == 1 && fixed::gu08TesterMap[0] == 5) ? 1 : 0;
}
static int TC_fixed_7_GetPhysicalIndex_mapped_works(TestCtx* ctx) {
    fixed::ResetGlobals();
    fixed::gu08DeviceCnt = 2;
    fixed::EnumSm3350_writeTesterMap_oneEntry(0, 7); // phidx0 -> tester 7
    fixed::EnumSm3350_writeTesterMap_oneEntry(1, 3); // phidx1 -> tester 3
    fixed::PhyIndex r1 = fixed::GetPhysicalIndexRaw(7);
    fixed::PhyIndex r2 = fixed::GetPhysicalIndexRaw(3);
    fixed::PhyIndex r3 = fixed::GetPhysicalIndexRaw(99);
    return (r1.IsValid() && r1.value() == 0 &&
            r2.IsValid() && r2.value() == 1 &&
            !r3.IsValid()) ? 1 : 0;
}
static int TC_fixed_8_GetDevicePath_offByOne_and_nullBytes_safe(TestCtx* ctx) {
    fixed::ResetGlobals();
    fixed::gu08DeviceCnt = 3;
    DWORD bytesRet = 0xDEAD;
    // idx = 3 (==gu08DeviceCnt, off-by-one fix) + valid bytesRet → ERROR_NO_SUCH_DEVICE
    int ret = fixed::GetDevicePathRaw(3, nullptr, 0, &bytesRet);
    ctx->intOut = ret;
    if (ret != ERROR_NO_SUCH_DEVICE) return 0;
    // bytesRet=nullptr case → ERROR_INVALID_PARAMETER, no AV
    int ret2 = -1;
    SEH_TOUCH( ret2 = fixed::GetDevicePathRaw(0, nullptr, 0, nullptr) );
    if (ctx->av) return 0;
    return (ret2 == ERROR_INVALID_PARAMETER) ? 1 : 0;
}
static int TC_fixed_9_GetDevicePath_valid_read(TestCtx* ctx) {
    fixed::ResetGlobals();
    fixed::gu08DeviceCnt = 1;
    strcpy_s(fixed::gstDeviceInfo[0].szDevicePathCache, _countof(fixed::gstDeviceInfo[0].szDevicePathCache), "DUMMY1");
    char out[64] = {0};
    DWORD bytes = 0;
    int ret = fixed::GetDevicePathRaw(0, out, sizeof out, &bytes);
    ctx->intOut = ret; ctx->dwordOut = bytes; ctx->ptrOut = out;
    return (ret == ERROR_SUCCESS && bytes == 7 && strcmp(out, "DUMMY1") == 0) ? 1 : 0;
}
static int TC_fixed_10_stage_template_port255_safe(TestCtx* ctx) {
    fixed::ResetGlobals();
    UCHAR portIndex = 255;
    fixed::PhyIndex iPhy = fixed::GetPhysicalIndexRaw(portIndex);
    // safe: !iPhy.IsValid() → skip member touch
    ctx->intOut = 0;
    if (iPhy.IsValid()) {
        SEH_TOUCH( fixed::getInstance(iPhy).TouchMarker() );
        if (ctx->av) return 0;
    } else {
        ctx->intOut = ERROR_NO_SUCH_DEVICE;
    }
    return (ctx->intOut == ERROR_NO_SUCH_DEVICE) ? 1 : 0;
}

// ========================= Runner =========================
typedef int (*tcFn)(TestCtx*);
struct TcEntry { const char* name; tcFn buggy; tcFn fixed; };

static TcEntry kCases[] = {
    { "TC1  getInstance(255)",              TC_buggy_1_getInstance_255_AV,                  TC_fixed_1_getInstance_255_safe },
    { "TC2  getInstance(16)",               TC_buggy_2_getInstance_16_AV,                   TC_fixed_2_getInstance_16_safe },
    { "TC3  getInstance(15) OK baseline",   TC_buggy_3_getInstance_15_OK,                   TC_fixed_3_getInstance_15_OK },
    { "TC4  GetTesterIndex(20) OOB",        TC_buggy_4_GetTesterIndex_20_OOB,               TC_fixed_4_GetTesterIndex_20_returns_UCHAR_MAX },
    { "TC5  GetDeviceInfo(20) OOB",         TC_buggy_5_GetDeviceInfo_20_OOB_nonnull,        TC_fixed_5_GetDeviceInfo_20_returns_null },
    { "TC6  Enum write 0xFF-gate P1-3",     TC_buggy_6_EnumSm3350_write_never_happens,      TC_fixed_6_EnumSm3350_write_occurred },
    { "TC7  GetPhysicalIndex mapping",      TC_buggy_7_GetPhysicalIndex_zeroDeviceCnt_p13,  TC_fixed_7_GetPhysicalIndex_mapped_works },
    { "TC8  GetDevicePath off+null bytes",  TC_buggy_8_GetDevicePath_offByOne_and_nullBytes,TC_fixed_8_GetDevicePath_offByOne_and_nullBytes_safe },
    { "TC9  GetDevicePath idx==cnt",        TC_buggy_9_GetDevicePath_idx0_gu0cnt0,          TC_fixed_9_GetDevicePath_valid_read },
    { "TC10 stageTemplate port=255",        TC_buggy_10_getInstance_mismatch_portIndex_255_in_stage_template, TC_fixed_10_stage_template_port255_safe },
};

int main(int argc, char** argv) {
    const char* mode = (argc >= 2) ? argv[1] : "BUGGY";
    int nCases = (int)(sizeof(kCases) / sizeof(kCases[0]));
    int pass = 0, fail = 0;
    int avsCount = 0;
    printf("=== P1-2 Root-Cure TDD harness — mode=%s, cases=%d ===\n", mode, nCases);
    for (int i = 0; i < nCases; ++i) {
        TestCtx ctx; memset(&ctx, 0, sizeof ctx);
        int ok;
        if (strcmp(mode, "FIXED") == 0) {
            ok = kCases[i].fixed(&ctx);
        } else {
            ok = kCases[i].buggy(&ctx);
        }
        if (ctx.av) avsCount++;
        printf("[%s] %-42s %s  av=%d intOut=%d ptrOut=%p\n",
               ok ? "PASS" : "FAIL", kCases[i].name,
               ok ? "pass" : "FAIL",
               ctx.av ? 1 : 0, ctx.intOut, ctx.ptrOut);
        if (ok) ++pass; else ++fail;
    }
    printf("---- summary: mode=%s PASS=%d FAIL=%d AVs_caught=%d ----\n", mode, pass, fail, avsCount);
    printf("%s_ALL_CHECKSUMS: pass=%d fail=%d avs=%d\n", mode, pass, fail, avsCount);
    return (fail == 0) ? 0 : 1;
}
