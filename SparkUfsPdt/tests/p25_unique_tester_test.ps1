# =============================================================================
# P2-5 TDD validation script
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

$src = Join-Path $TestDir "p25_unique_tester_test.cpp"
$exe = Join-Path $BuildDir "p25_unique_tester_test.exe"
Write-Host ""
Write-Host "--- building $src ---"
& $Cl /nologo /std:c++17 /EHsc /Fe:$exe $src 2>&1
if ($LASTEXITCODE -ne 0) { throw "BUILD FAILED exit=$LASTEXITCODE" }
if (!(Test-Path $exe)) { throw "BUILD FAILED exe missing" }
Write-Host "BUILD OK: $exe"

Write-Host ""
Write-Host "================================="
Write-Host " P2-5 TDD run (expect 4/4 PASS)"
Write-Host "================================="
& $exe
$runExit = $LASTEXITCODE
Write-Host "RUN exit=$runExit"
if ($runExit -ne 0) { throw "RUN FAILED exit=$runExit" }
Write-Host ""
Write-Host "ALL P2-5 TDD CHECKS PASSED"
exit 0
