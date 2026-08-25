// =============================================================================
// P2-8 TDD harness: CSparkUfsPdtDlg::OnDestroy cleanup bugs.
//
// TWO bugs in production code (SparkUfsPdtDlg.cpp L1094-L1143):
//
// [BUG #1] GDI double-delete.  Four GDI objects (font + 3 brushes) each get
//          DeleteObject() called TWO times:
//          - Pass 1 (L1118-L1133): if (GetSafeHandle()) { DeleteObject(); }  ✓ OK
//          - Pass 2 (L1135-L1138): DeleteObject();  NO if-check — REDUNDANT   ✗ BUG
//          MFC CFont/CBrush tolerate double-delete (return FALSE on 2nd call
//          and silently ignore it), but swapping for raw HGDIOBJ or custom
//          RAII wrappers would produce double-free undefined behaviour.
//
// [BUG #2] Wrong WM_NCDESTROY (OnDestroy) order.
//          CDialogEx::OnDestroy() (MFC default handler that tears down the
//          HWND, child controls and dialog state) is called at L1099 — the
//          VERY START of the function.  All of the following cleanup work
//          runs AFTER the window is already dead:
//            - m_factorySerial.Close()    (may post WM_* messages to the dialog)
//            - s_pool.reset()             (worker threads reference HWND via notifiers)
//            - SparkLog_Close()           (may queue UI flush messages)
//            - DeleteCriticalSection()
//            - EventBus::Unregister()    (Unregister AFTER HWND already torn down)
//          Correct canonical MFC order is: CLEAN UP FIRST (serial, pool, log,
//          critsec, eventbus, GDI), THEN call CDialogEx::OnDestroy() last.
//          Any other ordering risks posting to a dead HWND → handle leaks,
//          messages in the void, or (worst case) reuse-after-free of HWND data.
// =============================================================================

#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

typedef int (*TestCaseFn)();

// -----------------------------------------------------------------------------
// Instrumented GDI handle wrapper.  Counts every DeleteObject() call that
// targets a valid (non-NULL) handle.  Mirrors CFont/CBrush's MFC semantics:
//   GetSafeHandle() → returns the HGDIOBJ or NULL if already detached.
//   DeleteObject()  → calls ::DeleteObject() ONLY if the handle is valid,
//                     AND increments the global counter.  (We can't actually
//                     destroy a real HFONT/HBRUSH in the isolated test, so we
//                     simulate by setting m_hObject = NULL and bumping count.)
// -----------------------------------------------------------------------------
struct DummyH {};
typedef DummyH* HGDI;   // fake HGDI handle type for isolation

static int g_deleteCalls = 0;

struct InstrumentedGdi
{
    HGDI m_h = nullptr;

    InstrumentedGdi() { m_h = Create(); }   // simulate OnInitDialog CreateFont/CreateSolidBrush
    ~InstrumentedGdi() { if (m_h) { /* warn if leaked */ } }

    static HGDI Create() { static int s_id = 0; s_id++; return (HGDI)(uintptr_t)(s_id + 0x1000); }

    bool GetSafeHandle() const { return m_h != nullptr; }
    void DeleteObject()
    {
        if (m_h != nullptr) { ++g_deleteCalls; m_h = nullptr; }
        // If m_h is already NULL we MUST NOT increment the counter (matches
        // the post-condition of CFont::DeleteObject on an already-detached obj:
        // no underlying ::DeleteObject() call, no accounting).
    }
};

// -----------------------------------------------------------------------------
// Global cleanup counters used to verify ORDER (bug #2).  We capture the
// order in which the following "events" fire, then compare to canonical order.
// -----------------------------------------------------------------------------
enum CleanupEvent : int {
    EV_SERIAL_CLOSE = 1,
    EV_POOL_RESET   = 2,
    EV_LOG_CLOSE    = 3,
    EV_CRITSEC_DEL  = 4,
    EV_GDI_DELETE   = 5,
    EV_BUS_UNREG    = 6,
    EV_DLG_DESTROY  = 7,   // CDialogEx::OnDestroy() — the MFC handler that tears down HWND
};

#define MAXEV 16
static CleanupEvent g_order[MAXEV];
static int g_orderIdx = 0;
static void Fire(CleanupEvent e)
{
    if (g_orderIdx >= MAXEV) return;
    g_order[g_orderIdx++] = e;
}
static void ResetOrder() { memset(g_order, 0, sizeof(g_order)); g_orderIdx = 0; }

// ---- Helpers ----
#define CHECK(cond) do { if (!(cond)) { printf("  FAIL %s line %d: %s\n", __FUNCTION__, __LINE__, #cond); return 1; } } while (0)

static bool OrderAt(int idx, CleanupEvent want)
{
    return idx < g_orderIdx && g_order[idx] == want;
}

// -----------------------------------------------------------------------------
// BUGGY implementation (verbatim copy of production L1094-L1143 order + GDI calls).
// -----------------------------------------------------------------------------
static void BuggyOnDestroy(
    InstrumentedGdi& font, InstrumentedGdi& brushName,
    InstrumentedGdi& brushGreen, InstrumentedGdi& brushRed)
{
    // BUG #2: CDialogEx::OnDestroy() called FIRST — HWND torn down IMMEDIATELY.
    Fire(EV_DLG_DESTROY);

    Fire(EV_SERIAL_CLOSE);       // m_factorySerial.Close()
    Fire(EV_POOL_RESET);         // s_pool.reset()
    Fire(EV_LOG_CLOSE);          // SparkLog_Close()
    Fire(EV_CRITSEC_DEL);        // DeleteCriticalSection(g_logLock)

    // GDI Pass 1 (correct — guarded)
    if (font.GetSafeHandle())        font.DeleteObject();
    if (brushName.GetSafeHandle())   brushName.DeleteObject();
    if (brushGreen.GetSafeHandle())  brushGreen.DeleteObject();
    if (brushRed.GetSafeHandle())    brushRed.DeleteObject();
    Fire(EV_GDI_DELETE);

    // BUG #1: GDI Pass 2 (unguarded — DOUBLE-DELETE).
    brushName.DeleteObject();
    brushGreen.DeleteObject();
    brushRed.DeleteObject();
    font.DeleteObject();
    // (note: production code also fires GDI twice, we count each real call via instrumentation)

    Fire(EV_BUS_UNREG);          // EventBus::Unregister() — AFTER HWND already dead
}

// -----------------------------------------------------------------------------
// FIXED implementation:
//   (a) GDI DeleteObject() called exactly ONCE per handle (remove Pass 2).
//   (b) CDialogEx::OnDestroy() called LAST, after all other cleanup finished.
// -----------------------------------------------------------------------------
static void FixedOnDestroy(
    InstrumentedGdi& font, InstrumentedGdi& brushName,
    InstrumentedGdi& brushGreen, InstrumentedGdi& brushRed)
{
    Fire(EV_SERIAL_CLOSE);
    Fire(EV_POOL_RESET);
    Fire(EV_LOG_CLOSE);
    Fire(EV_CRITSEC_DEL);

    // GDI cleaned exactly ONCE — remove the redundant 2nd pass.
    if (font.GetSafeHandle())        font.DeleteObject();
    if (brushName.GetSafeHandle())   brushName.DeleteObject();
    if (brushGreen.GetSafeHandle())  brushGreen.DeleteObject();
    if (brushRed.GetSafeHandle())    brushRed.DeleteObject();
    Fire(EV_GDI_DELETE);

    Fire(EV_BUS_UNREG);

    // FIX #2: CDialogEx::OnDestroy() LAST (after HWND no longer needed by any cleanup).
    Fire(EV_DLG_DESTROY);
}

// =============================================================================
// RED TCs
// =============================================================================
static int TC_buggy_4_objects_delete_8_times(void)
{
    g_deleteCalls = 0;
    {
        InstrumentedGdi font, bName, bGreen, bRed;
        BuggyOnDestroy(font, bName, bGreen, bRed);
    }
    // Bug #1: pass 1 deletes all 4 (counter=4), pass 2 tries to delete again —
    //         but GetSafeHandle returns false after pass 1 → second pass does
    //         NOT bump counter (MFC semantics).  Wait — this means the counter
    //         is still 4!  How to expose the bug then?
    // We expose the BUG differently: we instrument DeleteObject to COUNT EVEN
    // UNGUARDED CALLS (the L1135-L1138 calls — whether or not handle is
    // valid).  Add a separate counter for "attempted DeleteObject calls
    // without GetSafeHandle guard first".  Redesign below — use a separate
    // "unguarded attempt" counter.
    return 0; // placeholder, rewritten inline in the real harness below
}

// ---- Better instrumentation: count ALL DeleteObject() INVOCATIONS, not only
//      ones on valid handles.  The double-delete bug is "code calls the
//      method twice per object", regardless of MFC tolerating the 2nd call.
static int g_deleteInvocations = 0;     // every time DeleteObject() is entered
static int g_guardedInvocations = 0;    // times we called it AFTER a GetSafeHandle() guard

struct InstrumentedGdiV2
{
    HGDI m_h = nullptr;
    InstrumentedGdiV2() { m_h = InstrumentedGdi::Create(); }
    bool GetSafeHandle() const { return m_h != nullptr; }
    void DeleteObject(bool guarded)
    {
        g_deleteInvocations++;
        if (guarded) g_guardedInvocations++;
        if (m_h != nullptr) m_h = nullptr;
    }
};

static void BuggyOnDestroyV2(
    InstrumentedGdiV2& font, InstrumentedGdiV2& bn,
    InstrumentedGdiV2& bg, InstrumentedGdiV2& br)
{
    g_order[g_orderIdx++] = EV_DLG_DESTROY;
    g_order[g_orderIdx++] = EV_SERIAL_CLOSE;
    g_order[g_orderIdx++] = EV_POOL_RESET;
    g_order[g_orderIdx++] = EV_LOG_CLOSE;
    g_order[g_orderIdx++] = EV_CRITSEC_DEL;

    // Pass 1: GUARDED
    if (font.GetSafeHandle())        font.DeleteObject(true);
    if (bn.GetSafeHandle())          bn.DeleteObject(true);
    if (bg.GetSafeHandle())          bg.DeleteObject(true);
    if (br.GetSafeHandle())          br.DeleteObject(true);
    g_order[g_orderIdx++] = EV_GDI_DELETE;

    // Pass 2: UNGUARDED (buggy lines L1135-L1138).  These calls always execute,
    //         no GetSafeHandle() if-check in front.
    bn.DeleteObject(false);
    bg.DeleteObject(false);
    br.DeleteObject(false);
    font.DeleteObject(false);

    g_order[g_orderIdx++] = EV_BUS_UNREG;
}

static void FixedOnDestroyV2(
    InstrumentedGdiV2& font, InstrumentedGdiV2& bn,
    InstrumentedGdiV2& bg, InstrumentedGdiV2& br)
{
    g_order[g_orderIdx++] = EV_SERIAL_CLOSE;
    g_order[g_orderIdx++] = EV_POOL_RESET;
    g_order[g_orderIdx++] = EV_LOG_CLOSE;
    g_order[g_orderIdx++] = EV_CRITSEC_DEL;

    if (font.GetSafeHandle())        font.DeleteObject(true);
    if (bn.GetSafeHandle())          bn.DeleteObject(true);
    if (bg.GetSafeHandle())          bg.DeleteObject(true);
    if (br.GetSafeHandle())          br.DeleteObject(true);
    g_order[g_orderIdx++] = EV_GDI_DELETE;

    g_order[g_orderIdx++] = EV_BUS_UNREG;
    g_order[g_orderIdx++] = EV_DLG_DESTROY;   // LAST ✅
}

// RED: buggy → 8 DeleteObject invocations, 4 guarded + 4 unguarded.
static int TC_RED_buggy_8_delete_invocations(void)
{
    g_deleteInvocations = 0;
    g_guardedInvocations = 0;
    ResetOrder();
    {
        InstrumentedGdiV2 font, bn, bg, br;
        BuggyOnDestroyV2(font, bn, bg, br);
    }
    CHECK(g_deleteInvocations  == 8);   // 4+4 = 8 calls to DeleteObject
    CHECK(g_guardedInvocations == 4);  // exactly 4 were preceded by GetSafeHandle guard
    return 0; // RED pass = bug reproduced (8 calls is the buggy behaviour)
}

// GREEN: fixed → 4 DeleteObject invocations, all 4 guarded.
static int TC_GRN1_fixed_4_delete_invocations(void)
{
    g_deleteInvocations = 0;
    g_guardedInvocations = 0;
    ResetOrder();
    {
        InstrumentedGdiV2 font, bn, bg, br;
        FixedOnDestroyV2(font, bn, bg, br);
    }
    CHECK(g_deleteInvocations  == 4);
    CHECK(g_guardedInvocations == 4);  // every call guarded
    return 0;
}

// RED: buggy order → DLG_DESTROY event is the FIRST event (idx 0).
static int TC_RED_buggy_dialog_destroy_first(void)
{
    ResetOrder();
    {
        InstrumentedGdiV2 font, bn, bg, br;
        BuggyOnDestroyV2(font, bn, bg, br);
    }
    CHECK(OrderAt(0, EV_DLG_DESTROY));  // bug: HWND torn down before serial/pool/log cleanup
    return 0; // RED pass
}

// GREEN: fixed order → DLG_DESTROY event is LAST (after SERIAL → POOL → LOG →
//                       CRITSEC → GDI → BUS).  Unregister runs before HWND dies.
static int TC_GRN2_fixed_dialog_destroy_last(void)
{
    ResetOrder();
    {
        InstrumentedGdiV2 font, bn, bg, br;
        FixedOnDestroyV2(font, bn, bg, br);
    }
    // Order should be: SERIAL_CLOSE POOL_RESET LOG_CLOSE CRITSEC_DEL GDI_DELETE BUS_UNREG DLG_DESTROY
    CHECK(OrderAt(0, EV_SERIAL_CLOSE));
    CHECK(OrderAt(1, EV_POOL_RESET));
    CHECK(OrderAt(2, EV_LOG_CLOSE));
    CHECK(OrderAt(3, EV_CRITSEC_DEL));
    CHECK(OrderAt(4, EV_GDI_DELETE));
    CHECK(OrderAt(5, EV_BUS_UNREG));
    CHECK(OrderAt(6, EV_DLG_DESTROY));
    CHECK(g_orderIdx == 7);   // exactly 7 events, no extras (no duplicate GDI passes)
    return 0;
}

// -----------------------------------------------------------------------------
// Runner
// -----------------------------------------------------------------------------
int main(void)
{
    struct { const char* name; TestCaseFn fn; bool expectPass; } cases[] = {
        { "RED  buggy 8 DeleteObject invocations",  TC_RED_buggy_8_delete_invocations,       true  },
        { "RED  buggy dialog_destroy FIRST",         TC_RED_buggy_dialog_destroy_first,        true  },
        { "GRN1 fixed 4 DeleteObject invocations",   TC_GRN1_fixed_4_delete_invocations,       true  },
        { "GRN2 fixed dialog_destroy LAST",          TC_GRN2_fixed_dialog_destroy_last,        true  },
    };
    int failed = 0;
    for (size_t i = 0; i < _countof(cases); i++)
    {
        int rc = cases[i].fn();
        bool pass = (rc == 0) == cases[i].expectPass;
        printf("[%s] %s\n", pass ? "PASS" : "FAIL", cases[i].name);
        if (!pass) failed++;
    }
    printf("\nP28 result: %d failed\n", failed);
    return failed ? 1 : 0;
}
