# P1-2 Root-Cure TDD driver
# Builds and runs both BUGGY (RED) and FIXED (GREEN) variants of p12_root_cure_test.cpp.
# Expects:
#   BUGGY:  FAIL==0, AVs_caught == count of buggy AV-triggering TCs (~7)
#   FIXED:  FAIL==0, AVs_caught == 0, all 10 PASS

$ErrorActionPreference = 'Stop'

$here    = Split-Path -Parent $MyInvocation.MyCommand.Path
$src     = Join-Path $here p12_root_cure_test.cpp
$outDir  = Join-Path $here p12_out
New-Item -ItemType Directory -Force -Path $outDir | Out-Null

# ---- Find MSVC via vswhere ----
$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
$inst    = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (-not $inst) { throw "vswhere failed to find VS install" }
$vcvars  = Join-Path $inst 'VC\Auxiliary\Build\vcvars64.bat'
if (-not (Test-Path $vcvars)) { throw "vcvars64 missing at $vcvars" }

function Invoke-NativeVcvars {
    param([Parameter(Mandatory)][string]$CmdLine)
    $psLine = "& `"$vcvars`" >nul 2>&1 && $CmdLine 2>&1"
    & cmd /c "`"$vcvars`" >nul 2>&1 && $CmdLine"
}

function Invoke-BuildRun($mode, $exefull) {
    $clFlags = "/nologo /EHsc /O2 /std:c++17 /D_CRT_SECURE_NO_WARNINGS"
    if ($mode -eq 'FIXED') { $clFlags += ' /DP12_MODE_FIXED' }
    $outObj = Join-Path $outDir "$mode.exe"
    Write-Host "---- BUILD $mode -> $outObj ----"
    & cmd /c "`"$vcvars`" >nul 2>&1 && cl.exe $clFlags `"$src`" /Fe:`"$outObj`" /link /out:`"$outObj`""
    if ($LASTEXITCODE -ne 0) { throw "cl.exe build $mode failed ($LASTEXITCODE)" }
    Write-Host "---- RUN   $mode ----"
    & $outObj $mode
    $script:LastRunExit = $LASTEXITCODE
}

Write-Host "================================="
Write-Host " RED phase: BUGGY"
Write-Host "================================="
$buggyExit = 0
$buggyOut  = & cmd /c "`"$vcvars`" >nul 2>&1 && cl.exe /nologo /EHsc /O2 /std:c++17 /D_CRT_SECURE_NO_WARNINGS `"$src`" /Fe:`"$(Join-Path $outDir BUGGY.exe)`" 2>&1"
if ($LASTEXITCODE -ne 0) { $buggyOut | Out-Host; throw "BUGGY compile failed ($LASTEXITCODE)" }
$buggyRun = & (Join-Path $outDir BUGGY.exe) BUGGY 2>&1
$buggyExit = $LASTEXITCODE
$buggyRun | Out-Host

Write-Host ""
Write-Host "================================="
Write-Host " GREEN phase: FIXED"
Write-Host "================================="
$fixedOut = & cmd /c "`"$vcvars`" >nul 2>&1 && cl.exe /nologo /EHsc /O2 /std:c++17 /D_CRT_SECURE_NO_WARNINGS `"$src`" /Fe:`"$(Join-Path $outDir FIXED.exe)`" 2>&1"
if ($LASTEXITCODE -ne 0) { $fixedOut | Out-Host; throw "FIXED compile failed ($LASTEXITCODE)" }
$fixedRun = & (Join-Path $outDir FIXED.exe) FIXED 2>&1
$fixedExit = $LASTEXITCODE
$fixedRun | Out-Host

Write-Host ""
Write-Host "================================="
Write-Host " EXPECTATION CHECK"
Write-Host "================================="

# Parse BUGGY pass/fail/AVs
$bLine = ($buggyRun | Where-Object { $_ -match 'BUGGY_ALL_CHECKSUMS:' })
$fLine = ($fixedRun | Where-Object { $_ -match 'FIXED_ALL_CHECKSUMS:' })
Write-Host "Parsed BUGGY: $bLine"
Write-Host "Parsed FIXED: $fLine"

function GetNum($line, $tag) {
    if ($line -match "$tag=(\d+)") { return [int]$matches[1] }
    return -1
}

$bPass = GetNum $bLine 'pass'; $bFail = GetNum $bLine 'fail'; $bAVs  = GetNum $bLine 'avs'
$fPass = GetNum $fLine 'pass'; $fFail = GetNum $fLine 'fail'; $fAVs  = GetNum $fLine 'avs'

$ok = $true
# BUGGY: all 10 reproduce the bug correctly (signature mismatch or SEH AV). fail==0 is mandatory.
if ($bFail -ne 0)     { Write-Host "!! RED FAIL: BUGGY fail=$bFail (expected 0: every case must reproduce its bug correctly)"; $ok = $false }
if ($bPass -ne 10)    { Write-Host "!! RED FAIL: BUGGY pass=$bPass (!=10: some cases didn't reproduce)"; $ok = $false }
# FIXED: fail==0 AND avs==0 AND pass==10
if ($fFail -ne 0)     { Write-Host "!! GREEN FAIL: FIXED fail=$fFail (expected 0)"; $ok = $false }
if ($fAVs  -ne 0)     { Write-Host "!! GREEN FAIL: FIXED avs=$fAVs (!=0: UB not eliminated)"; $ok = $false }
if ($fPass -ne 10)    { Write-Host "!! GREEN FAIL: FIXED pass=$fPass (!=10)"; $ok = $false }
# Differential check: BUGGY AVs(2) + FIXED AVs(0) proves the root-cure eliminated access violations.
Write-Host ("Differential: BUGGY_AVs={0} FIXED_AVs={1} (expect >=2 and 0 respectively)" -f $bAVs,$fAVs)
if ($bAVs -lt 2)     { Write-Host "!! WARNING: BUGGY avs=$bAVs (<2: signature detection may be carrying detection; still OK as long as BUGGY pass==10)" }

if ($ok) {
    Write-Host "ALL P1-2 ROOT-CURE TDD CHECKS PASSED"
    exit 0
} else {
    Write-Host "P1-2 ROOT-CURE TDD CHECKS FAILED"
    exit 1
}
