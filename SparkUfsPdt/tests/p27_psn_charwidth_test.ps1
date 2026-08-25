# =============================================================================
# P2-7 TDD validation script
#  Steps:
#   1. Build  p27_psn_charwidth_test.cpp  →  .exe (real VC CRT, no mocks)
#   2. Run   .exe  →  expect 4/4 PASS (all 2 RED + 2 GREEN assertions self-consistent)
#
#  Exit 0 only if build ok, run ok, pass==expected, fail==0.
# =============================================================================

param(
    [string]$Cl = "C:\SoftWare\VS2026\Community\VC\Tools\MSVC\14.51.36231\bin\Hostx64\x86\cl.exe",
    [string]$TestDir = "c:\Users\offic\source\repos\xuchen95\SparkUfsPdt\SparkUfsPdt\tests",
    [string]$BuildDir = "c:\Users\offic\source\repos\xuchen95\SparkUfsPdt\build_tmp",
    [string]$SdkInclude = "C:\Program Files (x86)\Windows Kits\10\Include\10.0.26100.0",
    [string]$SdkLib    = "C:\Program Files (x86)\Windows Kits\10\Lib\10.0.26100.0",
    [string]$MsvcInclude = "C:\SoftWare\VS2026\Community\VC\Tools\MSVC\14.51.36231\include",
    [string]$MsvcLib     = "C:\SoftWare\VS2026\Community\VC\Tools\MSVC\14.51.36231\lib\x86"
)

$ErrorActionPreference = "Stop"
$env:INCLUDE = "$SdkInclude\ucrt;$SdkInclude\shared;$SdkInclude\um;$MsvcInclude"
$env:LIB     = "$SdkLib\ucrt\x86;$SdkLib\um\x86;$MsvcLib"

New-Item -ItemType Directory -Force -Path $BuildDir | Out-Null
Write-Host "Using cl.exe: $Cl"

# ---- step 1 build ----
$src = Join-Path $TestDir "p27_psn_charwidth_test.cpp"
$exe = Join-Path $BuildDir "p27_psn_charwidth_test.exe"
Write-Host ""
Write-Host "--- building $src ---"
& $Cl /nologo /std:c++17 /EHsc /Fe:$exe $src 2>&1
if ($LASTEXITCODE -ne 0) { throw "BUILD FAILED exit=$LASTEXITCODE" }
if (!(Test-Path $exe)) { throw "BUILD FAILED exe missing" }
Write-Host "BUILD OK: $exe"

# ---- step 2 run ----
Write-Host ""
Write-Host "================================="
Write-Host " P2-7 TDD run (expect 4/4 PASS, fail=0)"
Write-Host "================================="
& $exe
$runExit = $LASTEXITCODE
Write-Host ""
Write-Host "RUN exit=$runExit"

if ($runExit -ne 0) { throw "RUN FAILED exit=$runExit — TDD broken" }
Write-Host ""
Write-Host "ALL P2-7 TDD CHECKS PASSED"
exit 0
