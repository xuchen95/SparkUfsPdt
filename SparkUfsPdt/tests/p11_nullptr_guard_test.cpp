// --------------------------------------------------------------------------
// P1-1 TDD harness: pBase/pOpt nullptr dereference guard for RunFtTaskImpl / RunQcTaskImpl
//
// This file inline-replicates the EXACT dereference sites from RunPdtTaskImpl.cpp that
// are vulnerable to the P1-1 bug. It does NOT link against MFC or the real DLLs.
//
// Compile-time toggle:
//   * /DP11_FIXED   -> emit the null guard we will add to production (safe path)
//   * no P11_FIXED    -> emit buggy path (should AV on SEH on null inputs; we wrap with
//                         __try/__except and report EXCEPTION_ACCESS_VIOLATION)
//
// Run with:  cl /EHsc /nologo /W3 p11_nullptr_guard_test.cpp
//          cl /EHsc /nologo /W3 /DP11_FIXED /Fepad /Fep11_nullptr_guard_test_fixed.exe p11_nullptr_guard_test.cpp
// --------------------------------------------------------------------------
#include <windows.h>
#include <cstdio>
#include <cstring>
#include <cstdint>

// ---- Mirror the exact POD structs used in production so field offsets / types match.
// Do NOT change field order or types; copied verbatim from CDialogBase.h lines 4..73
// (typedefs were expanded to plain struct tags so the compiler doesn't need afx stuff.
struct main_param {
    int bBurnInTest;          // BOOL -> int for ABI compat
    char szFlowName[32];
    int funcSel;
    int bDLTesterFW;
    char strTesterFwPath[1024];
    int bDLISP;
    char strIspPath[1024];
    int bDLCID;                // BOOL (P1-1 deref site in FT L166)
    unsigned int bankIdx;
    char mid[2];
    char oid[31];
    char pnm[16];
    unsigned int psn_start;
    unsigned int psn_end;
    char psn_mask[31];
    char mdt[4];
    char prv[4];
    char mnm[8];
    char meto1[4];
    char meto2[4];
};
struct qc_param {
    int bCheckDiskInfo;
    unsigned long n4KBCnt;
    int bCheckPnm;
    char pnm[16];
    int bCheckMidOid;
    unsigned int bankIdx;
    char mid[2];
    char oid[31];
    int bCheckMnm;
    char mnm[8];
    int bCheckPrv;
    char prv[4];
    int bCheckMdt;
    char mdt[4];
    int bCheckIsp;              // BOOL (P1-1 deref site in QC L281)
    char isp[32];
    int bCheckSramTest;
    char szSramTestPath[1024];
};
struct ufs_option {
    struct main_param mainPrm;
    struct qc_param   qcPrm;
};
struct UFS_BASE_SETTING {
    int PortBaseSel;
    int PortMappingSel;
    int ForceRomMode;           // int (P1-1 deref site: FT L149/L154; QC L262/L267/L286/L292)
    int bSnSeparateIni;
    char szRemoteSnPath[1024];
    char szReportPath[1024];
    char szComName[64];
    unsigned int uBaudRate;
    unsigned int uByteSize;
    unsigned int uParity;
    unsigned int uStopBits;
};

// Constants for ForceRomMode (copied from CImpState.h L5-6)
#define UPIU_FORCE_ROM_MODE FALSE   // 0
#define VCC_FORCE_ROM_MODE  TRUE    // 1

static int g_av_caught;   // 1 if SEH handler caught EXCEPTION_ACCESS_VIOLATION during this call
static int g_av_count;   // total AVs caught in the whole process (for assertion)

// --------------------------------------------------------------------------
// runFTSnippet: replicates RunFtTaskImpl.cpp L148-L177 core pBase/pOpt dereference
// (the code block that fires only after a hypothetical DeviceSelect()==SUCCESS).
// Returns 0 on success, non-zero Windows error on failure.
// --------------------------------------------------------------------------
static int runFTSnippet(const struct UFS_BASE_SETTING* pBase, const struct ufs_option* pOpt)
{
    // ----- P1-1 FIX (compile-time toggle) -----
#ifdef P11_FIXED
    if (!pBase || !pOpt) return ERROR_INVALID_PARAMETER; // 87
#endif

    volatile int touched = 0;
    // FT L149: if (pBase->ForceRomMode == UPIU_FORCE_ROM_MODE)
    if (pBase->ForceRomMode == UPIU_FORCE_ROM_MODE)        { touched |= 1; }
    // FT L154: else if (pBase->ForceRomMode == VCC_FORCE_ROM_MODE)
    else if (pBase->ForceRomMode == VCC_FORCE_ROM_MODE)  { touched |= 2; }
    // FT L166: if (pOpt->mainPrm.bDLCID)
    if (pOpt->mainPrm.bDLCID)                            { touched |= 4; }
    // Do not return 'touched' — caller only cares ERROR_SUCCESS vs error code.
    (void)touched;
    return ERROR_SUCCESS;
}

// --------------------------------------------------------------------------
// runQCSnippet: replicates RunQcTaskImpl.cpp L261-L294 core pBase/pOpt dereference
// --------------------------------------------------------------------------
static int runQCSnippet(const struct UFS_BASE_SETTING* pBase, const struct ufs_option* pOpt)
{
#ifdef P11_FIXED
    if (!pBase || !pOpt) return ERROR_INVALID_PARAMETER;
#endif

    volatile int touched = 0;
    // QC L262/L267 first ForceRom block
    if (pBase->ForceRomMode == UPIU_FORCE_ROM_MODE)         { touched |= 1; }
    else if (pBase->ForceRomMode == VCC_FORCE_ROM_MODE)     { touched |= 2; }
    // QC L281
    if (pOpt->qcPrm.bCheckIsp)                             { touched |= 4; }
    // QC L286/L292 second ForceRom block (the "作者明明在 L240 缓存了 bForceRomMode 但又忘了用的第二处)
    if (pBase->ForceRomMode == UPIU_FORCE_ROM_MODE)         { touched |= 8; }
    else if (pBase->ForceRomMode == VCC_FORCE_ROM_MODE)     { touched |= 16; }
    (void)touched;
    return ERROR_SUCCESS;
}

// --------------------------------------------------------------------------
// SEH wrapper: invokes fn() and translates any EXCEPTION_ACCESS_VIOLATION into
// a synthetic return value ERROR_PROCESS_ABORTED (0x400) plus increments g_av_caught.
// --------------------------------------------------------------------------
typedef int (*snippet_fn)(const struct UFS_BASE_SETTING*, const struct ufs_option*);
static int SehRun(snippet_fn fn, const struct UFS_BASE_SETTING* pB, const struct ufs_option* pO)
{
    g_av_caught = 0;
    __try {
        return fn(pB, pO);
    } __except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION
                ? EXCEPTION_EXECUTE_HANDLER
                : EXCEPTION_CONTINUE_SEARCH) {
        g_av_caught = 1;
        ++g_av_count;
        return ERROR_PROCESS_ABORTED;   // 1024 = 0x400  (marker, synthetic)
    }
}

struct Case { const char* name; int expectRet; int expectAv; const struct UFS_BASE_SETTING* pB; const struct ufs_option* pO; };

static int RunCase(const char* tag, snippet_fn fn, const struct Case* c)
{
    int r = SehRun(fn, c->pB, c->pO);
    int retOk = (r == c->expectRet);
    int avOk  = (g_av_caught == c->expectAv);
    const char* verdict = (retOk && avOk) ? "PASS" : "FAIL";
    printf("[%s] %-5s %-28s ret=%5d (want %5d) av=%d (want %d) %s\n",
        tag, verdict, c->name, r, c->expectRet, g_av_caught, c->expectAv,
        retOk ? "" : (retOk ? "" : avOk ? "ret-mismatch" : "both-mismatch"));
    return (retOk && avOk) ? 0 : 1;
}

int main()
{
    // Build valid (stack) valid opt;
    struct UFS_BASE_SETTING base; std::memset(&base, 0, sizeof(base));
    base.ForceRomMode = UPIU_FORCE_ROM_MODE;
    struct ufs_option opt;  std::memset(&opt, 0, sizeof(opt));
    opt.mainPrm.bDLCID = TRUE;
    opt.qcPrm.bCheckIsp = TRUE;

    const char* tag =
#ifdef P11_FIXED
        "FIXED";
#else
        "BUGGY";
#endif

    int fail = 0;
    g_av_count = 0;

    // ---- FT cases ----
    struct Case ft_cases[] = {
    //  name                           expectRet            expectAv  pB        pO
        {"FT_both_null",              ERROR_INVALID_PARAMETER,  0,  nullptr,  nullptr},  // BUGGY: AV(0x400) — FIXED: 87 no-AV
        {"FT_pBase_null_opt_valid",   ERROR_INVALID_PARAMETER,  0,  nullptr,  &opt   },  // BUGGY: AV — FIXED: 87
        {"FT_pBase_valid_opt_null",   ERROR_INVALID_PARAMETER,  0,  &base,    nullptr},  // BUGGY: AV (on L166 opt) — FIXED: 87
        {"FT_both_valid",             ERROR_SUCCESS,            0,  &base,    &opt   },  // both: 0, no AV
    };
    // Override BUGGY expectations: without P1-1 bug means we EXPECT an AV on any null input
#ifndef P11_FIXED
    ft_cases[0].expectRet = ERROR_PROCESS_ABORTED; ft_cases[0].expectAv = 1;
    ft_cases[1].expectRet = ERROR_PROCESS_ABORTED; ft_cases[1].expectAv = 1;
    ft_cases[2].expectRet = ERROR_PROCESS_ABORTED; ft_cases[2].expectAv = 1;
#endif

    printf("--- FT snippet (%s mode) ---\n", tag);
    for (size_t i = 0; i < sizeof(ft_cases)/sizeof(ft_cases[0]); ++i)
        fail += RunCase(tag, runFTSnippet, &ft_cases[i]);

    // ---- QC cases ----
    struct Case qc_cases[] = {
        {"QC_both_null",              ERROR_INVALID_PARAMETER,  0,  nullptr,  nullptr},
        {"QC_pBase_null_opt_valid",   ERROR_INVALID_PARAMETER, 0,  nullptr,  &opt   },
        {"QC_pBase_valid_opt_null",    ERROR_INVALID_PARAMETER, 0,  &base,    nullptr},
        {"QC_both_valid",              ERROR_SUCCESS,           0,  &base,    &opt   },
    };
#ifndef P11_FIXED
    qc_cases[0].expectRet = ERROR_PROCESS_ABORTED; qc_cases[0].expectAv = 1;
    qc_cases[1].expectRet = ERROR_PROCESS_ABORTED; qc_cases[1].expectAv = 1;
    qc_cases[2].expectRet = ERROR_PROCESS_ABORTED; qc_cases[2].expectAv = 1;
#endif

    printf("--- QC snippet (%s mode) ---\n", tag);
    for (size_t i = 0; i < sizeof(qc_cases)/sizeof(qc_cases[0]); ++i)
        fail += RunCase(tag, runQCSnippet, &qc_cases[i]);

    printf("=== TOTAL %s-mode failures: %d (AVs caught during run: %d)\n", tag, fail, g_av_count);

    // BUGGY mode must have triggered exactly 6 AVs (FT[0..2] + QC[0..2])
#ifdef P11_FIXED
    // FIXED mode must have triggered ZERO AVs.
    if (g_av_count != 0) { printf("FAIL: FIXED mode must have ZERO AVs (got %d)\n", g_av_count); ++fail; }
#else
    if (g_av_count != 6) { printf("FAIL: BUGGY mode must have 6 AVs (got %d)\n", g_av_count); ++fail; }
#endif

    return (fail == 0) ? 0 : 1;
}
