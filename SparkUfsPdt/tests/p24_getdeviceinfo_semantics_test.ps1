# P2-4 TDD combo runner: GetDeviceInfo(UCHAR) wrapper semantic mismatch
# Steps:
#   1. Compile p24_getdeviceinfo_semantics_test.cpp to .exe
#   2. Run BUGGY mode -> expect: pass=4 fail=0 (reproduces the bug correctly)
#   3. Run FIXED mode -> expect: pass=4 fail=0 (harness shows fixed behavior)
#   4. Print overall verdict

param(
    [switch]$SkipBuild
)

$ErrorActionPreference = "Stop"
$TestDir = "c:\Users\offic\source\repos\xuchen95\SparkUfsPdt\SparkUfsPdt\tests"
$SrcFile = Join-Path $TestDir "p24_getdeviceinfo_semantics_test.cpp"
$ExeFile = Join-Path $TestDir "p24_getdeviceinfo_semantics_test.exe"

# Locate cl.exe (VS 2026 MSVC x86 host x86 target)
$cl = "C:\SoftWare\VS2026\Community\VC\Tools\MSVC\14.51.36231\bin\Hostx64\x86\cl.exe"
Write-Host "Using cl.exe: $cl"

# --- Step 1: build ----------------------------------------------------------------
if (-not $SkipBuild) {
    Push-Location $TestDir
    & $cl /nologo /std:c++17 /EHsc /O2 `
        /DWIN32 /D_WINDOWS /DUNICODE /D_UNICODE `
        $SrcFile `
        /link /OUT:p24_getdeviceinfo_semantics_test.exe 2>&1 | Select-Object -Last 10
    $rc = $LASTEXITCODE
    Pop-Location
    if ($rc -ne 0 -or -not (Test-Path $ExeFile)) {
        Write-Host "BUILD FAILED (exit=$rc)" -ForegroundColor Red
        exit 2
    }
    Write-Host "BUILD OK: $ExeFile" -ForegroundColor Green
}

# --- Step 2: BUGGY mode -----------------------------------------------------------
Write-Host ""
Write-Host "=================================" -ForegroundColor Cyan
Write-Host " BUGGY phase (expect 4/4 PASS)"    -ForegroundColor Cyan
Write-Host "=================================" -ForegroundColor Cyan
$buggyOut = & $ExeFile BUGGY 2>&1
$buggyOut | ForEach-Object { Write-Host $_ }
$buggyExit = $LASTEXITCODE
Write-Host "BUGGY exit=$buggyExit"

# --- Step 3: FIXED mode -----------------------------------------------------------
Write-Host ""
Write-Host "=================================" -ForegroundColor Cyan
Write-Host " FIXED phase (expect 4/4 PASS)"    -ForegroundColor Cyan
Write-Host "=================================" -ForegroundColor Cyan
$fixedOut = & $ExeFile FIXED 2>&1
$fixedOut | ForEach-Object { Write-Host $_ }
$fixedExit = $LASTEXITCODE
Write-Host "FIXED exit=$fixedExit"

# --- Step 4: aggregate ------------------------------------------------------------
Write-Host ""
Write-Host "=================================" -ForegroundColor Cyan
Write-Host " EXPECTATION CHECK"                 -ForegroundColor Cyan
Write-Host "=================================" -ForegroundColor Cyan
$buggyPass = -1; $buggyFail = -2
$fixedPass = -1; $fixedFail = -2
foreach ($line in $buggyOut) { if ($line -match "P24_CHECKSUMS: pass=(\d+) fail=(\d+)") { $buggyPass=[int]$Matches[1]; $buggyFail=[int]$Matches[2] } }
foreach ($line in $fixedOut) { if ($line -match "P24_CHECKSUMS: pass=(\d+) fail=(\d+)") { $fixedPass=[int]$Matches[1]; $fixedFail=[int]$Matches[2] } }
Write-Host "BUGGY checksums: pass=$buggyPass fail=$buggyFail"
Write-Host "FIXED checksums: pass=$fixedPass fail=$fixedFail"

$ok = ($buggyExit -eq 0 -and $fixedExit -eq 0 -and $buggyPass -eq 4 -and $buggyFail -eq 0 -and $fixedPass -eq 4 -and $fixedFail -eq 0)
if ($ok) {
    Write-Host ""
    Write-Host "ALL P2-4 TDD CHECKS PASSED" -ForegroundColor Green
    exit 0
} else {
    Write-Host ""
    Write-Host "P2-4 TDD FAILED: buggy or fixed phase returned wrong pass/fail counts" -ForegroundColor Red
    exit 1
}
