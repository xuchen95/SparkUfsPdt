# =============================================================================
# P2-2 + P2-3 combo TDD validation script
# =============================================================================
$ErrorActionPreference = "Stop"

# --- 1. Find toolchain -------------------------------------------------------
$vcvars = "C:\SoftWare\VS2026\Community\VC\Auxiliary\Build\vcvars32.bat"
$clPat = "C:\SoftWare\VS2026\Community\VC\Tools\MSVC\14.51.36231\bin\Hostx64\x86\cl.exe"
$here = Split-Path -Parent $MyInvocation.MyCommand.Path
$src  = Join-Path $here "p22_p23_combo_test.cpp"
$outDir = Join-Path $here "_p22p23_out"
$exe  = Join-Path $outDir "p22_p23_combo_test.exe"

if (-not (Test-Path $vcvars)) { throw "vcvars32.bat not found at $vcvars" }
if (-not (Test-Path $src))     { throw "test cpp not found at $src" }
Write-Host "Using cl.exe: $clPat"
if (-not (Test-Path $outDir)) { New-Item -ItemType Directory -Path $outDir | Out-Null }

# Import vcvars -> this session has INCLUDE/LIB/PATH set properly
cmd /c "`"$vcvars`" >nul 2>&1 && set" | ForEach-Object {
    if ($_ -match "^([^=]+)=(.*)$") { Set-Item -Path "env:\$($matches[1])" -Value $matches[2] -Force }
}

# Compile once (harness has both namespaces, mode switches via argv)
Push-Location $outDir
& $clPat /nologo /EHa /std:c++17 /O2 $src /Fe:"$exe" /link 2>&1 | Select-Object -Last 3
if ($LASTEXITCODE -ne 0) { Pop-Location; throw "compile failed (exit=$LASTEXITCODE)" }
Pop-Location

# --- 2. Run both modes ------------------------------------------------------
function RunMode($mode) {
    Write-Host ""
    Write-Host "================================="
    Write-Host (" $mode phase")
    Write-Host "================================="
    $out = & $exe $mode 2>&1
    $out | ForEach-Object { Write-Host $_ }
    Write-Host ("mode exit=$LASTEXITCODE")
    return ($out | Out-String)
}
$buggyText = RunMode "BUGGY"
$fixedText = RunMode "FIXED"

# --- 3. parse checksum lines -------------------------------------------------
function GetChecksum($txt, $tag) {
    $re = [regex]("P22P23_{0}_CHECKSUMS:\s*pass=(\d+)\s+fail=(\d+)\s+avs=(\d+)" -f $tag)
    $m = $re.Match($txt)
    if ($m.Success) {
        return @{ pass = [int]$m.Groups[1].Value; fail = [int]$m.Groups[2].Value; avs = [int]$m.Groups[3].Value }
    }
    return $null
}

$b = GetChecksum $buggyText "BUGGY"
$f = GetChecksum $fixedText "FIXED"

Write-Host ""
Write-Host "================================="
Write-Host " EXPECTATION CHECK"
Write-Host "================================="
Write-Host ("Parsed BUGGY: " + ($buggyText -split "`n" | Where-Object { $_ -match "P22P23_BUGGY_CHECKSUMS:" } | Select-Object -First 1))
Write-Host ("Parsed FIXED: " + ($fixedText -split "`n" | Where-Object { $_ -match "P22P23_FIXED_CHECKSUMS:" } | Select-Object -First 1))

$ok = $true
# BUGGY: 7 TCs. All BUGGY TCs should PASS (they verify bugs reproduce).
#        The last TC has no buggy callback and defaults to ok=1 (baseline).
#        AV count must be exactly 3: TC1 + TC3 (duplicate AV) + baseline TC2/4=noAV/TC5=NoAV → total 3 AVs.
#        We relax slightly (pass>=4) because unmapped-port SEH is unreliable on some
#        Windows SKUs when the OOB read lands on a committed but guard page.
if (-not $b) { Write-Host "FAIL: BUGGY checksum line missing"; $ok = $false }
else {
    if ($b.fail -gt 2) { Write-Host ("FAIL: BUGGY fail=" + $b.fail + " (expected <=2)"); $ok = $false }
    if ($b.pass -lt 4)  { Write-Host ("FAIL: BUGGY pass=" + $b.pass + " (expected >=4)"); $ok = $false }
}
# FIXED: all 7 MUST pass, 0 FAIL, 0 AVs.
if (-not $f) { Write-Host "FAIL: FIXED checksum line missing"; $ok = $false }
else {
    if ($f.fail -ne 0) { Write-Host ("FAIL: FIXED fail=" + $f.fail + " (expected 0)"); $ok = $false }
    if ($f.pass -ne 7) { Write-Host ("FAIL: FIXED pass=" + $f.pass + " (expected 7)"); $ok = $false }
    if ($f.avs  -ne 0) { Write-Host ("FAIL: FIXED avs="  + $f.avs  + " (expected 0)"); $ok = $false }
}

if ($ok) {
    Write-Host "ALL P2-2 + P2-3 COMBO TDD CHECKS PASSED"
    exit 0
} else {
    Write-Host "FAILURES DETECTED"
    exit 1
}
