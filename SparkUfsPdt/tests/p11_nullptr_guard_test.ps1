# P1-1 null-pointer deref TDD driver.
# Builds BUGGY + FIXED variants of p11_nullptr_guard_test.cpp and runs both.
#
#   BUGGY (no P11_FIXED defined):
#       Each of the 6 null-input cases must trigger EXCEPTION_ACCESS_VIOLATION
#       which our SEH wrapper maps to ERROR_PROCESS_ABORTED (0x400). This
#       proves the bug EXISTS ("RED" in TDD cycle).
#
#   FIXED (/DP11_FIXED):
#       Each null input returns ERROR_INVALID_PARAMETER (87) and ZERO SEH
#       AVs. Valid inputs return 0. This proves the guard pattern works
#       before we port it into RunPdtTaskImpl.cpp ("GREEN" for the guard).

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$Src = Join-Path $ScriptDir "p11_nullptr_guard_test.cpp"
Push-Location $ScriptDir

# Locate cl.exe (prefer vswhere; fallback to PATH)
# Returns @(clExePath, vsInstallPath) — vsInstallPath may be null.
function Find-Toolchain() {
    $vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    if (Test-Path $vswhere) {
        $inst = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath 2>$null
        if ($inst) {
            $verFile = Join-Path $inst "VC\Auxiliary\Build\Microsoft.VCToolsVersion.default.txt"
            if (Test-Path $verFile) {
                $ver = (Get-Content $verFile -Raw).Trim()
                $cl = Join-Path $inst "VC\Tools\MSVC\$ver\bin\Hostx64\x64\cl.exe"
                if (Test-Path $cl) { return @($cl, $inst) }
            }
        }
    }
    $cmd = Get-Command cl.exe -ErrorAction SilentlyContinue
    if ($cmd) { return @($cmd.Source, $null) }
    return @($null, $null)
}

function Invoke-Native([string]$exe, [string[]]$argsList) {
    if (-not $argsList) { $argsList = @() }
    Write-Host "RUN> $exe $($argsList -join ' ')"
    if ($argsList.Count -eq 0) {
        $p = Start-Process -FilePath $exe -Wait -PassThru -NoNewWindow `
            -RedirectStandardOutput out.txt -RedirectStandardError err.txt
    } else {
        $p = Start-Process -FilePath $exe -ArgumentList $argsList `
            -Wait -PassThru -NoNewWindow -RedirectStandardOutput out.txt -RedirectStandardError err.txt
    }
    if (Test-Path out.txt) { $out = Get-Content out.txt -Raw; if ($out) { Write-Host $out } }
    if (Test-Path err.txt) { $err = Get-Content err.txt -Raw; if ($err) { Write-Host $err } }
    return $p.ExitCode
}

$toolchain = Find-Toolchain
$cl = $toolchain[0]
$vsInstall = $toolchain[1]
if (-not $cl) {
    Write-Host "SKIP: cl.exe not found. Install MSVC build tools or open a Developer Command Prompt."
    exit 0
}
Write-Host "Using cl.exe: $cl"

# ---- Load INCLUDE/LIB env from vcvars64.bat (needed for <windows.h> / CRT headers).
if ($vsInstall) {
    $vcvars = Join-Path $vsInstall "VC\Auxiliary\Build\vcvars64.bat"
    if (Test-Path $vcvars) {
        Write-Host "Loading MSVC env via $vcvars"
        $captured = cmd /c "`"$vcvars`" >nul 2>&1 && set"
        foreach ($line in $captured) {
            if ($line -match '^([^=]+)=(.*)$') {
                $k = $matches[1]; $v = $matches[2]
                [Environment]::SetEnvironmentVariable($k, $v, "Process")
            }
        }
    } else {
        Write-Host "WARNING: vcvars64.bat not found at $vcvars"
    }
} else {
    Write-Host "WARNING: vswhere didn't locate VS; relying on ambient INCLUDE/LIB env"
}

$totalFail = 0

# ---- 1. BUGGY mode ----
Write-Host "`n=== [1/2] BUILD + RUN: BUGGY mode (RED expected: 6 AVs, 0 failures in harness) ==="
$buggyExe = Join-Path $ScriptDir "p11_buggy.exe"
$buggyFe = "/Fe:" + $buggyExe
$rc = Invoke-Native $cl @("/nologo","/EHsc","/W3","/O2","/utf-8",$Src,$buggyFe,"/link","/INCREMENTAL:NO")
if ($rc -ne 0) { Write-Host "BUGGY compile failed, rc=$rc"; exit 2 }
if (-not (Test-Path $buggyExe)) { Write-Host "BUGGY compile produced no binary?"; exit 2 }
$rc = Invoke-Native $buggyExe @()
if ($rc -ne 0) { Write-Host "BUGGY mode FAILED (the harness itself must PASS — i.e. AVs must be caught). rc=$rc"; $totalFail += 1 }

# ---- 2. FIXED mode ----
Write-Host "`n=== [2/2] BUILD + RUN: FIXED mode (GREEN expected: 0 AVs, 0 failures) ==="
$fixedExe = Join-Path $ScriptDir "p11_fixed.exe"
$fixedFe = "/Fe:" + $fixedExe
$rc = Invoke-Native $cl @("/nologo","/EHsc","/W3","/O2","/utf-8","/DP11_FIXED",$Src,$fixedFe,"/link","/INCREMENTAL:NO")
if ($rc -ne 0) { Write-Host "FIXED compile failed, rc=$rc"; exit 2 }
if (-not (Test-Path $fixedExe)) { Write-Host "FIXED compile produced no binary?"; exit 2 }
$rc = Invoke-Native $fixedExe @()
if ($rc -ne 0) { Write-Host "FIXED mode FAILED (guard did not prevent AV). rc=$rc"; $totalFail += 1 }

Pop-Location
if ($totalFail -eq 0) { Write-Host "`n=== ALL P1-1 TDD CHECKS PASSED ==="; exit 0 }
Write-Host "`n=== P1-1 TDD FAILURES: $totalFail ==="
exit 1
