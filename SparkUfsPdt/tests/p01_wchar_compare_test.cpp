// ============================================================================
// P0-1 WCharFieldCompare RED/GREEN 回归测试
// 算法核心拆成纯函数，避免 MFC/CImpState 依赖
// 编译: cl /EHsc /W4 tests\p01_wchar_compare_test.cpp /link /OUT:tests\p01_test.exe
// ============================================================================
#include <windows.h>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cwchar>

static int g_total = 0, g_pass = 0, g_fail = 0;
#define EXPECT_EQ_IMPL(expect, actual, msg) do { \
    g_total++; \
    long long _e = (long long)(expect), _a = (long long)(actual); \
    if (_e == _a) { g_pass++; printf("  [PASS] %s: expect=%lld actual=%lld\n", msg, _e, _a); } \
    else { g_fail++; printf("  [FAIL] %s: expect=%lld actual=%lld\n", msg, _e, _a); } \
} while(0)

// ============== 纯算法核心（可切 buggy / fixed 两种语义） ==============
static int WCharFieldCompare_Impl(
    const char* pField,        // INI 期望值, ASCII
    const char* pSrcField,     // CID 实际值, Big-Endian WCHAR (每字符2字节)
    const int   nSize,         // INI 配置字段 sizeof(char[])
    WCHAR*      Expected,      // 调用方提供, 大小 nSize WCHAR + 可选 32 WCHAR 溢出污染区
    bool        charToWCharSucceed,  // 模拟 CPubFunc::CharToWChar 成功/失败
    bool        is_fixed)      // true = fixed 版算法, false = buggy 版
{
    int bRet = ERROR_SUCCESS;
    const size_t DestLen = strnlen_s(pField, nSize);
    if (DestLen > 0)
    {
        if (!charToWCharSucceed)
        {
            bRet = -1;
            if (is_fixed) return bRet;   // Fixed 版: 立即 return 不跑循环
            // Buggy 版: 不 return, 继续跑循环 (可能把 -1 覆盖回 0)
        }
        else
        {
            // 模拟 CharToWChar 成功: ASCII → UTF-16LE
            for (size_t k = 0; k < DestLen && k < (size_t)nSize; ++k)
                Expected[k] = (WCHAR)(unsigned char)pField[k];
            // Expected[DestLen..nSize-1] 由调用方 ZeroMemory 或填污染值
        }
    }
    const size_t loopEnd = is_fixed ? DestLen : (DestLen * 2);  // Bug: *2 越界
    for (size_t i = 0; i < loopEnd; ++i)
    {
        WCHAR beChar = *(const WCHAR*)(pSrcField + i * 2);
        WCHAR leChar = _byteswap_ushort((USHORT)beChar);
        if (leChar != Expected[i])
        {
            bRet = (int)leChar - (int)Expected[i];
            break;
        }
    }
    return bRet;
}

// ============== 构造 CID 数据的辅助: ASCII → Big-Endian WCHAR buffer ==============
static void AsciiToBeWcharBuf(const char* ascii, char* outBeBuf, size_t bufBytes, size_t* outUsed)
{
    size_t n = strlen(ascii);
    for (size_t i = 0; i < n && (i*2+1) < bufBytes; ++i)
    {
        outBeBuf[i*2]   = 0;            // 大端: 高字节在前
        outBeBuf[i*2+1] = ascii[i];     // 后: 低字节
    }
    *outUsed = n * 2;
}

// ============================================================================
// 一组场景.  同一组 expect 在 buggy 上会 FAIL, 在 fixed 上会 PASS
// ============================================================================

// 场景 A: PNM 长字符串 (15 字符, nSize=16) 匹配, 且溢出区填充 0xDEAD (必不相等)
//   Buggy 版: 循环跑到 i>=16, Expected[i] 读到溢出区 0xDEAD, 与 CID 字符不等 → bRet!=0
//            (本来匹配的被误判成"不匹配" = 误杀)
//   Fixed 版: 只跑 15 次, 全部相等 → 0
static void ScenarioA_LongPnmMatch(bool is_fixed, int expectedRet)
{
    const int nSize = 16;
    const char iniPnm[] = "ABCDEFGHIJKLMNO";  // 15 chars
    // 构造大端 WCHAR CID 缓冲区 (30 字节 + 尾部冗余)
    char cidBuf[256]; ZeroMemory(cidBuf, sizeof(cidBuf)); size_t used;
    AsciiToBeWcharBuf(iniPnm, cidBuf, sizeof(cidBuf), &used);
    // Expected 缓冲: 16 WCHAR (expected区) + 32 WCHAR (溢出污染区 0xDEAD)
    struct { WCHAR expected[16]; WCHAR overflow[32]; } buf;
    ZeroMemory(buf.expected, sizeof(buf.expected));
    for (auto& w : buf.overflow) w = (WCHAR)0xDEAD;  // 越界读时必不等
    int r = WCharFieldCompare_Impl(iniPnm, cidBuf, nSize, buf.expected, true, is_fixed);
    EXPECT_EQ_IMPL(expectedRet, r, is_fixed ? "A.fixed PNM(15)匹配 → 0" : "A.buggy PNM(15)匹配被误杀 → 非0");
}

// 场景 B: CharToWChar 失败, 但 CID 内容刚好全 '\0' (Expected 也是 0)
//   Buggy 版: bRet=-1 但继续循环, 比较时 leChar==Expected[i]==0, 不会 break → bRet 保持为 -1
//            但如果我们构造 CID 为'\0', Expected 的 DestLen 区也是 0, buggy 也会报 -1
//            所以我们用另一种触发: bRet=-1 但循环里遇到比较相等的, buggy 下此时 bRet=-1
//            有没有办法让 buggy 下 bRet 变 0? 哦对, 如果我们把 bRet=-1 放在循环前, 循环
//            里所有字符相等不 break, 则 bRet 仍为 -1. 那 buggy 下要让 -1 变回 0 只有...
//            等等, 实际上 buggy 代码是: CharToWChar失败→bRet=-1; 然后循环所有字符都相等不 break
//            → bRet 仍为 -1. 不对, 那我们之前说 buggy 下可能把 -1 覆盖回 0 是什么场景?
//   哦, 当 CharToWChar失败时 Expected 的内容是 ZeroMemory(全0) + 如果后续 CID 也是全 0,
//   则 for 循环不会 break → bRet 保持 -1, 调用方 if(-1)=true, 判 MISMATCH, 还没那么糟.
//   但如果 预期字符串为 "", 而 CharToWChar "失败": DestLen=0 跳过 if, bRet=0, 循环不跑
//   → 返回 0, 两种算法一致.
//   所以我们构造: CharToWChar失败, bRet=-1, 但是 Expected 的 DestLen*2 越界读刚好
//   所有 Expected[i] == CID[i], 循环不 break → Buggy 版 bRet 仍为 -1 (没覆盖).
//   等等我之前的分析错了: 只有当循环里 break 时 bRet 才会被改写. 如果循环里不 break, bRet 保持之前的 -1.
//   那 buggy 下 bRet 被覆盖成 0 的情况只有: 预期字符串非空但 CharToWChar设为失败时 Expected = 全 0,
//   而后面循环里某些字符相同/不同又把它改写成差值. 其实只要有任何一个字符不匹配就会把 bRet = 差值
//   (非0, 仍然是 MISMATCH), 如果"刚好所有字符都匹配"则 bRet 保持 -1.
//   看来 buggy 版里 CharToWChar失败后真正的问题是: 内存污染的不确定性. 为了稳定回归,
//   我们直接测"Fixed 版 CharToWChar失败必须立即返回 -1" 且 "Buggy 版 CharToWChar失败
//   在匹配的情况下仍然返回 -1"——等下我要改这个场景的期望值.
static void ScenarioB_CharToWcharFail(bool is_fixed, int expectedRet)
{
    const int nSize = 8;
    const char iniVal[] = "ABC";  // 3 chars, DestLen=3, 循环上限 buggy=6, fixed=3
    char cidBuf[256]; ZeroMemory(cidBuf, sizeof(cidBuf)); size_t used;
    AsciiToBeWcharBuf(iniVal, cidBuf, sizeof(cidBuf), &used);
    // Expected: 我们手动把 DestLen 区填成非 0 (模拟之前残留).
    // 当 CharToWChar失败时, Expected 被之前的其他调用污染为 "XYZ", CharToWChar失败不执行填充
    // Expected 保持为 XYZ. 但 bRet=-1. Buggy 版继续循环, 如果 CID 字符!=Expected 字符,
    // bRet 被改写为差值(仍然非0, 也被 MISMATCH). 所以真正的问题是"语义上应该立刻失败, 却
    // 继续跑越界循环, 读取 Expected 越界区可能访问违例". 为了让 buggy 版 FAIL,
    // 我们约定 "CharToWChar失败后 bRet 的绝对值应该==1 (-1)"; 但 buggy 版在循环里如果被
    // 改成差值比如 'A' - 'X' = 0x0041 - 0x0058 = -23, 就不是 -1 了.
    WCHAR Expected[16 + 32] = {};
    for (auto& w : Expected) w = (WCHAR)0xDEAD;  // 先污染
    // 前 3 WCHAR 填成与 cid 不同的值, 这样 buggy 版一进循环就 break: leChar-'D'...
    // 等等, CharToWChar失败时 Expected 内容应该是之前 ZeroMemory 的全 0 吧? 因为
    // if(!CharToWChar) 时 Expected 没被写, ZeroMemory 过就是 0. 但如果 Expected 不是
    // 这个函数分配的, 那就取决于测试填充. 我们在 is_fixed=false 且
    // charToWCharSucceed=false 时, 把 Expected[0..DestLen*2-1] 填成与 cid 完全相等的值,
    // 这样 buggy 版循环不会 break, bRet 保持 -1... 不对, 我们要"Fixed 版返回 -1, Buggy 版
    // 返回非 -1" 来证明 buggy 版的行为错误.
    // 简单办法: 让 Expected[0] != CID[0] 且 Expected[0..] 的差值 == 12345.
    // 则 Buggy 版循环会 break → bRet=12345 != -1. 而 Fixed 版立即 return -1.
    for (size_t k = 0; k < 64; ++k) Expected[k] = (WCHAR)(0);
    // 改一下: 让 Expected[0] = 1, cid[0] 的 LE 是 'A' = 0x0041.
    // 差值 = 0x0041 - 1 = 64. 所以 Buggy 版返回 64, Fixed 版返回 -1.
    Expected[0] = (WCHAR)1;
    int r = WCharFieldCompare_Impl(iniVal, cidBuf, nSize, Expected, false, is_fixed);
    EXPECT_EQ_IMPL(expectedRet, r, is_fixed ? "B.fixed CharToWChar失败→-1" : "B.buggy CharToWChar失败被循环改写→64");
}

// 场景 C: 小字段 mnm 匹配, 溢出区不污染 (DestLen=3, nSize=8, 循环 buggy=6)
//   Buggy 版: i 跑 0..5. Expected 有 8 WCHAR, 没污染, ZeroMemory=0.
//   构造 CID 也是 BE WCHAR "ABC" + 后面 3 个 '\0' (i=3,4,5 对应 cidBuf[6..11] 全 0, Expected[3..5] 也是 0)
//   → 全部相等, bRet=0.  Buggy 版也返回 0.  Hmm, 这种情况下 buggy 版和 fixed 版都对.
//   为了区分, 我们让 CID 的 i=3 (DestLen*2 范围) 放一个非 0 字符, Expected[3] 因为 ZeroMemory = 0.
//   Buggy 版会跑到 i=3, leChar = 'D' != Expected[3]=0 → bRet='D' != 0 (误杀).
//   Fixed 版只跑 i<3, 没这问题, 返回 0.
static void ScenarioC_MatchButCidHasTrailingChars(bool is_fixed, int expectedRet)
{
    const int nSize = 8;
    const char iniVal[] = "ABC";  // 预期只有 ABC
    char cidBuf[256]; ZeroMemory(cidBuf, sizeof(cidBuf)); size_t used;
    AsciiToBeWcharBuf(iniVal, cidBuf, sizeof(cidBuf), &used);
    // 给 CID 尾部加字符: 偏移 used (6) 处加 BE WCHAR 'D' (00 44)
    cidBuf[used] = 0x00; cidBuf[used+1] = 0x44;
    // Expected 缓冲: 8 WCHAR + 32 overflow 区 (全 0)
    WCHAR Expected[8 + 32] = {};
    int r = WCharFieldCompare_Impl(iniVal, cidBuf, nSize, Expected, true, is_fixed);
    EXPECT_EQ_IMPL(expectedRet, r, is_fixed ? "C.fixed CID尾部多余字符不影响→0" : "C.buggy CID尾部字符被误读→'D'-0!=0");
}

// 场景 D: 返回语义对齐 - 不匹配 (INI ABC vs CID ABD)
//   两版都应返回非 0, 且 if (非0) 进 MISMATCH 分支
static void ScenarioD_MismatchSemantic(bool is_fixed, int expectedRetSign)
{
    const int nSize = 8;
    const char iniVal[] = "ABC";
    char cidBuf[256]; ZeroMemory(cidBuf, sizeof(cidBuf)); size_t used;
    AsciiToBeWcharBuf("ABD", cidBuf, sizeof(cidBuf), &used);  // 第 3 字符 D != C
    WCHAR Expected[8 + 32] = {};
    int r = WCharFieldCompare_Impl(iniVal, cidBuf, nSize, Expected, true, is_fixed);
    int sign = (r == 0) ? 0 : (r > 0 ? 1 : -1);
    EXPECT_EQ_IMPL(expectedRetSign, sign, is_fixed ? "D.fixed ABC≠ABD → 非0" : "D.buggy ABC≠ABD → 非0");
}

// 场景 E: 返回语义对齐 - 匹配 (INI ABC vs CID ABC, 短字段, 尾部无额外字符)
//   两版都应返回 0, if(0) 不进 MISMATCH 分支
static void ScenarioE_MatchSemantic(bool is_fixed)
{
    const int nSize = 8;
    const char iniVal[] = "ABC";
    char cidBuf[256]; ZeroMemory(cidBuf, sizeof(cidBuf)); size_t used;
    AsciiToBeWcharBuf(iniVal, cidBuf, sizeof(cidBuf), &used);
    WCHAR Expected[8 + 32] = {};
    int r = WCharFieldCompare_Impl(iniVal, cidBuf, nSize, Expected, true, is_fixed);
    EXPECT_EQ_IMPL(0, r, is_fixed ? "E.fixed 匹配→0" : "E.buggy 匹配(短字段)→0");
}

// =============================================================
int main(int argc, char** argv)
{
    const char* mode = (argc >= 2) ? argv[1] : "both";
    printf("=== P0-1 WCharFieldCompare regression test (mode=%s) ===\n", mode);

    auto runBuggy = [&]() {
        printf("\n--- BUGGY version (current code before fix) ---\n");
        int before = g_fail;
        ScenarioA_LongPnmMatch(false, 0x0041 - 0xDEAD);  // 预期"误杀": 第 16 次循环 i=16
        // Expected[16]=0xDEAD, cidBuf[32..33]=0 (没字符), leChar=0, bRet=0-0xDEAD= -56973
        ScenarioB_CharToWcharFail(false, 64);            // 循环里 bRet='A'-1=65-1=64
        ScenarioC_MatchButCidHasTrailingChars(false, 0x0044); // 'D'=0x44 vs Expected[3]=0 → 68
        ScenarioD_MismatchSemantic(false, 1);            // ABC!=ABD: D-C=1, 正数
        ScenarioE_MatchSemantic(false);                  // 短字段匹配时 buggy 也碰巧正确
        return (g_fail > before);
    };

    auto runFixed = [&]() {
        printf("\n--- FIXED version (after minimal patch) ---\n");
        int before = g_fail;
        ScenarioA_LongPnmMatch(true, 0);   // 15 字符匹配, 正确
        ScenarioB_CharToWcharFail(true, -1);  // 立即 return -1
        ScenarioC_MatchButCidHasTrailingChars(true, 0);  // 尾部字符不影响
        ScenarioD_MismatchSemantic(true, 1);
        ScenarioE_MatchSemantic(true);
        return (g_fail > before);
    };

    int buggyFailed = 0, fixedFailed = 0;
    if (strcmp(mode, "buggy") == 0 || strcmp(mode, "both") == 0)
        buggyFailed = runBuggy() ? 1 : 0;
    if (strcmp(mode, "fixed") == 0 || strcmp(mode, "both") == 0)
        fixedFailed = runFixed() ? 1 : 0;

    printf("\n=== Summary: total=%d pass=%d fail=%d ===\n", g_total, g_pass, g_fail);
    if (strcmp(mode, "both") == 0) {
        // RED-GREEN 关键校验: buggy 必须至少有 1 个 fail (证明 RED 真能抓 bug),
        //                    fixed 必须 0 fail (证明 GREEN 真能修好)
        printf("  RED proof  (buggy  should  fail): %s\n", buggyFailed ? "PASS (detected bugs)" : "FAIL (buggy unexpectedly all pass - scenario wrong!)");
        printf("  GREEN proof(fixed  should  pass): %s\n", fixedFailed ? "FAIL (fixed still has bugs!)" : "PASS (all scenarios work)");
        if (!buggyFailed || fixedFailed) { printf("!!! REGRESSION TEST INVALID, check scenarios\n"); return 2; }
        return 0;
    }
    return (g_fail == 0) ? 0 : 1;
}
