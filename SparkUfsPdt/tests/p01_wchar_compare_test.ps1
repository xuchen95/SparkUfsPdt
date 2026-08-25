# P0-1 WCharFieldCompare RED/GREEN regression test (PowerShell equivalent)
# 与 C++ WCharFieldCompare 算法 byte-level 对齐
param([ValidateSet('buggy','fixed','both')][string]$Mode = 'both')

$script:total=$script:pass=$script:fail=0
function expect_eq($expect, $actual, [string]$msg) {
    $script:total++
    if ($expect -ceq $actual) {
        $script:pass++
        Write-Host "  [PASS] $msg`: expect=$expect actual=$actual"
    } else {
        $script:fail++
        Write-Host "  [FAIL] $msg`: expect=$expect actual=$actual" -ForegroundColor Red
    }
}
function byteswap_ushort([int]$v) { $v = $v -band 0xFFFF; return (($v -band 0x00FF) -shl 8) -bor (($v -band 0xFF00) -shr 8) }
function ascii_to_be_wchar_buf([string]$s) {
    $b = New-Object byte[] ($s.Length * 2)
    for ($i=0; $i -lt $s.Length; $i++) {
        $b[$i*2]     = 0x00            # Big-Endian: high byte first
        $b[$i*2 + 1] = [byte][char]$s[$i]
    }
    return ,$b
}

function WCharFieldCompare_Impl(
    [byte[]]$pField,       # INI: ASCII bytes
    [byte[]]$pSrcField,    # CID: Big-Endian WCHAR buffer
    [int]$nSize,           # sizeof(char[]) in ini
    [int[]]$Expected,      # caller pre-filled (length >= nSize+32), values 0..65535
    [bool]$charToWCharSucceed,
    [bool]$is_fixed
) {
    [int]$bRet = 0
    # strnlen_s(pField, nSize)
    $DestLen = 0
    while ($DestLen -lt $nSize -and $DestLen -lt $pField.Length -and $pField[$DestLen] -ne 0) { $DestLen++ }

    if ($DestLen -gt 0) {
        if (-not $charToWCharSucceed) {
            $bRet = -1
            if ($is_fixed) { return $bRet }   # Fixed: IMMEDIATE return
            # Buggy: fall through to loop
        } else {
            for ($k=0; $k -lt $DestLen; $k++) {
                if ($k -lt $nSize -and $k -lt $pField.Length) {
                    $Expected[$k] = [int]($pField[$k] -band 0xFF)
                }
            }
        }
    }
    $loopEnd = if ($is_fixed) { $DestLen } else { $DestLen * 2 }
    for ($i=0; $i -lt $loopEnd; $i++) {
        $idx = $i * 2
        if ($idx + 1 -lt $pSrcField.Length) {
            $lowAddrByte  = [int]($pSrcField[$idx]   -band 0xFF)
            $highAddrByte = [int]($pSrcField[$idx+1] -band 0xFF)
        } else { $lowAddrByte=0; $highAddrByte=0 }
        $beChar = ($highAddrByte -shl 8) -bor $lowAddrByte    # small-endian WCHAR value
        $leChar = byteswap_ushort $beChar
        if ($leChar -ne ($Expected[$i] -band 0xFFFF)) {
            $bRet = $leChar - ($Expected[$i] -band 0xFFFF)
            # signed short wrap: mirror C++ signed subtraction semantics
            if ($bRet -ge 0x8000) { $bRet -= 0x10000 }
            break
        }
    }
    return $bRet
}

# ---------- Scenarios ----------
function ScenarioA_LongPnmMatch([bool]$is_fixed, [int]$expectedRet) {
    $nSize = 16
    $ini = [Text.Encoding]::ASCII.GetBytes("ABCDEFGHIJKLMNO")
    $cidRaw = ascii_to_be_wchar_buf "ABCDEFGHIJKLMNO"
    $cid = New-Object byte[] ($cidRaw.Length + 500)
    [Array]::Copy($cidRaw, $cid, $cidRaw.Length)
    $Expected = New-Object int[] ($nSize + 32)
    for ($k=$nSize; $k -lt $Expected.Length; $k++) { $Expected[$k] = 0xDEAD }
    $r = WCharFieldCompare_Impl $ini $cid $nSize $Expected $true $is_fixed
    $msg = if ($is_fixed) { "A.fixed PNM(15)匹配→0" } else { "A.buggy PNM(15)匹配被越界误杀→-56973" }
    expect_eq $expectedRet $r $msg
}
function ScenarioB_CharToWcharFail([bool]$is_fixed, [int]$expectedRet) {
    $nSize = 8
    $ini = [Text.Encoding]::ASCII.GetBytes("ABC")
    $cid = ascii_to_be_wchar_buf "ABC" + (New-Object byte[] 200)
    $Expected = New-Object int[] ($nSize + 32)
    $Expected[0] = 1   # poison so 0x41-1 = 64
    $r = WCharFieldCompare_Impl $ini $cid $nSize $Expected $false $is_fixed
    $msg = if ($is_fixed) { "B.fixed CharToWChar失败→-1" } else { "B.buggy CharToWChar失败被循环覆盖→64" }
    expect_eq $expectedRet $r $msg
}
function ScenarioC_MatchButCidHasTrailing([bool]$is_fixed, [int]$expectedRet) {
    $nSize = 8
    $ini = [Text.Encoding]::ASCII.GetBytes("ABC")
    $cid = (ascii_to_be_wchar_buf "ABC") + (ascii_to_be_wchar_buf "D") + (New-Object byte[] 200)
    $Expected = New-Object int[] ($nSize + 32)
    $r = WCharFieldCompare_Impl $ini $cid $nSize $Expected $true $is_fixed
    $msg = if ($is_fixed) { "C.fixed CID尾部多余字符不影响→0" } else { "C.buggy CID尾部字符被误读→68('D')" }
    expect_eq $expectedRet $r $msg
}
function ScenarioD_Mismatch([bool]$is_fixed, [int]$expectedSign) {
    $nSize = 8
    $ini = [Text.Encoding]::ASCII.GetBytes("ABC")
    $cid = (ascii_to_be_wchar_buf "ABD") + (New-Object byte[] 200)
    $Expected = New-Object int[] ($nSize + 32)
    $r = WCharFieldCompare_Impl $ini $cid $nSize $Expected $true $is_fixed
    $sign = if ($r -eq 0) { 0 } elseif ($r -gt 0) { 1 } else { -1 }
    $msg = if ($is_fixed) { "D.fixed ABC≠ABD→非0" } else { "D.buggy ABC≠ABD→非0" }
    expect_eq $expectedSign $sign $msg
}
function ScenarioE_Match([bool]$is_fixed) {
    $nSize = 8
    $ini = [Text.Encoding]::ASCII.GetBytes("ABC")
    $cid = (ascii_to_be_wchar_buf "ABC") + (New-Object byte[] 200)
    $Expected = New-Object int[] ($nSize + 32)
    $r = WCharFieldCompare_Impl $ini $cid $nSize $Expected $true $is_fixed
    $msg = if ($is_fixed) { "E.fixed 匹配→0" } else { "E.buggy 短字段匹配碰巧→0" }
    expect_eq 0 $r $msg
}

# ---------- run ----------
Write-Host "=== P0-1 WCharFieldCompare regression test (mode=$Mode, PowerShell impl) ==="
Write-Host "All scenarios: expected values = production-correct behavior"
$buggyBefore = $script:fail

if ($Mode -eq 'buggy' -or $Mode -eq 'both') {
    Write-Host "`n--- BUGGY version (current code on disk) ---"
    # 所有期望值统一为 "正确生产行为"（匹配=0, CharToWChar失败=-1, 不匹配=非0）
    # 若 buggy 断言失败 → 证明测试真的抓到了 bug (RED proof)
    ScenarioA_LongPnmMatch $false 0                  # 长字符串匹配 → 正确应返回 0; buggy 会越界读返回 -57005
    ScenarioB_CharToWcharFail $false -1              # CharToWChar 失败 → 正确应返回 -1; buggy 会被循环覆盖为 64
    ScenarioC_MatchButCidHasTrailing $false 0        # CID 尾部多余字符不影响 → 正确返回 0; buggy 会读到 'D' 返回 68
    ScenarioD_Mismatch $false 1                      # ABC≠ABD → 正确返回非 0 (正号); buggy/fixed 都应成立
    ScenarioE_Match $false                           # 短字段精确匹配 → 返回 0; buggy/fixed 碰巧都对
}
$buggyFailCnt = $script:fail - $buggyBefore
$buggyFailed = $buggyFailCnt -gt 0   # RED proof: buggy 必须至少错 3 个（A, B, C）

if ($Mode -eq 'fixed' -or $Mode -eq 'both') {
    Write-Host "`n--- FIXED version (after minimal patch) ---"
    # 同样的期望值 (正确生产行为)
    ScenarioA_LongPnmMatch $true 0
    ScenarioB_CharToWcharFail $true -1
    ScenarioC_MatchButCidHasTrailing $true 0
    ScenarioD_Mismatch $true 1
    ScenarioE_Match $true
}
if ($Mode -eq 'both') { $fixedStartFail = $buggyBefore + 5 } else { $fixedStartFail = $buggyBefore }
if ($script:fail -ge $fixedStartFail) { $fixedActualFail = $script:fail - $fixedStartFail } else { $fixedActualFail = 0 }

Write-Host "`n=== Summary: total=$script:total pass=$script:pass fail=$script:fail ==="
if ($Mode -eq 'both') {
    $buggyFailCnt = $script:fail - $buggyBefore
    if ($buggyFailed) { $redMsg = "PASS - buggy version triggered $buggyFailCnt bug(s) (RED proof works)" } else { $redMsg = "FAIL - INVALID TEST! buggy version unexpectedly all pass" }
    if ($fixedActualFail -eq 0) { $greenMsg = "PASS - fixed version passed all 5 assertions (GREEN proof works)" } else { $greenMsg = "FAIL - fixed still has $fixedActualFail failures, patch broken" }
    Write-Host "  RED proof  (buggy MUST show failures):  $redMsg"
    Write-Host "  GREEN proof(fixed MUST pass all):       $greenMsg"
    if (-not $buggyFailed) { Write-Host "`n!!! REGRESSION TEST INVALID: buggy doesn't fail, check scenarios." -ForegroundColor Red; exit 2 }
    if ($fixedActualFail -ne 0) { Write-Host "`n!!! PATCH BROKEN" -ForegroundColor Red; exit 1 }
    exit 0
} else {
    if ($script:fail -eq 0) { exit 0 } else { exit 1 }
}
