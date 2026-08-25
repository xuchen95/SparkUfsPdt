# =============================================================================
# P2-1 SparkLog root-cure TDD driver script
# Usage: pwsh -File p21_sparklog_test.ps1
# =============================================================================
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$here = Split-Path -Parent $MyInvocation.MyCommand.Path
$src = Join-Path $here "p21_sparklog_test.cpp"
$outDir = Join-Path $here "_p21_out"
$exe = Join-Path $outDir "p21_sparklog_test.exe"
$cl = $null

# --- 1. find cl.exe ---------------------------------------------------------
$vswhere = "C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe"
$vsRoot = $null
if (Test-Path $vswhere) {
    $vsRoot = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath 2>$null
    if ($vsRoot) {
        $candidates = @(
            (Join-Path $vsRoot "VC\Tools\MSVC\*\bin\Hostx64\x86\cl.exe"),
            (Join-Path $vsRoot "VC\Tools\MSVC\*\bin\Hostx86\x86\cl.exe")
        )
        foreach ($pat in $candidates) {
            $found = Get-ChildItem $pat -ErrorAction SilentlyContinue | Sort-Object FullName -Descending | Select-Object -First 1
            if ($found) { $cl = $found.FullName; break }
        }
    }
}
if (-not $cl) {
    $g = Get-Command cl.exe -ErrorAction SilentlyContinue
    if ($g) { $cl = $g.Source }
}

# --- 1.5 Apply VC dev env (INCLUDE/LIB/PATH) via vcvars32.bat ---------------
if ($vsRoot) {
    $vcvars = Join-Path $vsRoot "VC\Auxiliary\Build\vcvars32.bat"
    if (Test-Path $vcvars) {
        $dumpFile = Join-Path $outDir "vcvars_env.txt"
        New-Item -ItemType Directory -Force -Path $outDir | Out-Null
        & cmd /c "`"$vcvars`" >nul 2>&1 && set > `"$dumpFile`"" 2>&1 | Out-Null
        if (Test-Path $dumpFile) {
            $lines = Get-Content $dumpFile
            foreach ($line in $lines) {
                $eq = $line.IndexOf('=')
                if ($eq -gt 0) {
                    $k = $line.Substring(0, $eq)
                    $v = $line.Substring($eq + 1)
                    if ($k -match '^(INCLUDE|LIB|LIBPATH|PATH)$') {
                        [Environment]::SetEnvironmentVariable($k, $v, "Process")
                    }
                }
            }
        }
    }
}

if (-not $cl) {
    Write-Host "WARN: cl.exe not found via vswhere or PATH; skipping compile + run."
    exit 0
}

# --- 2. compile -------------------------------------------------------------
New-Item -ItemType Directory -Force -Path $outDir | Out-Null
Write-Host "Using cl.exe: $cl"
# Do NOT wipe INCLUDE. If empty, try to locate Windows Kits via vswhere's installationPath
# by running vcvars*.bat in a subshell and capturing the env block (too complex); instead
# just invoke cl.exe with INCLUDE/LIB inherited from the calling process. If vswhere already
# gave us cl.exe under VS2026, rely on the caller having the correct toolset env.
& $cl /nologo /EHa /std:c++17 /O2 "$src" /Fe:"$exe" /link 2>&1 | Select-Object -Last 40
if ($LASTEXITCODE -ne 0) {
    Write-Host "ERROR: compile failed with exit=$LASTEXITCODE"
    exit 1
}
Write-Host ""

function RunMode($mode) {
    Write-Host "================================="
    Write-Host " $mode phase"
    Write-Host "================================="
    Push-Location $outDir
    try {
        & $exe $mode 2>&1 | Tee-Object -Variable out
        Write-Host "mode exit=$LASTEXITCODE"
    } finally { Pop-Location }
    $text = ($out | Out-String)
    return $text
}

$buggyText = RunMode "BUGGY"
$fixedText = RunMode "FIXED"

# --- 3. parse checksum lines ------------------------------------------------
function GetChecksum($txt, $tag) {
    # Match CHECKSUMS line anywhere. Use .* around line so \r\n mid-file is fine, no strict ^$.
    $re = [regex]("{0}_CHECKSUMS:\s*pass=(\d+)\s+fail=(\d+)\s+avs=(\d+)" -f $tag)
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
Write-Host ("Parsed BUGGY: " + ($buggyText -split "`n" | Where-Object { $_ -match "BUGGY_CHECKSUMS:" } | Select-Object -First 1))
Write-Host ("Parsed FIXED: " + ($fixedText -split "`n" | Where-Object { $_ -match "FIXED_CHECKSUMS:" } | Select-Object -First 1))

$ok = $true
# BUGGY mode has 4 cases: TC1 (reproduce close-order UB -> PASS if touched or SEH) + TC3 (reproduce interleave -> not 100% on all Windows volumes due to NTFS append-file atomicity so PASS if either interleave OR size correct) + X1/X2 dummies.
if (-not $b) { Write-Host "FAIL: BUGGY checksum line missing"; $ok = $false }
else {
    if ($b.fail -gt 1) { Write-Host ("FAIL: BUGGY fail=" + $b.fail + " (expected <=1)"); $ok = $false }
    if ($b.pass -lt 3)  { Write-Host ("FAIL: BUGGY pass=" + $b.pass + " (expected >=3)"); $ok = $false }
}
# FIXED mode: all 7 cases MUST pass, 0 AVs.
if (-not $f) { Write-Host "FAIL: FIXED checksum line missing"; $ok = $false }
else {
    if ($f.fail -ne 0) { Write-Host ("FAIL: FIXED fail=" + $f.fail + " (expected 0)"); $ok = $false }
    if ($f.pass -ne 7) { Write-Host ("FAIL: FIXED pass=" + $f.pass + " (expected 7)"); $ok = $false }
    if ($f.avs  -ne 0) { Write-Host ("FAIL: FIXED avs="  + $f.avs  + " (expected 0)"); $ok = $false }
}

if ($ok) {
    Write-Host "ALL P2-1 ROOT-CURE TDD CHECKS PASSED"
    exit 0
} else {
    Write-Host "FAILURES DETECTED"
    exit 1
}
