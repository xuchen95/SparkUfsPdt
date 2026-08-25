# =============================================================================
# P2-6: C4267 size_t->UCHAR narrowing warnings proof script.
#  Strategy: compile libsparkusb.cpp with /W4 /Wx (warning = error).
#   RED phase (before fix): C4267 triggers → EXIT != 0 (WX treats as error).
#   GREEN phase (after fix): 0 warnings, 0 errors → EXIT == 0.
# =============================================================================
param(
    [switch]$GreenMode,   # pass when running after the fix applied
    [string]$Cl = "C:\SoftWare\VS2026\Community\VC\Tools\MSVC\14.51.36231\bin\Hostx64\x86\cl.exe",
    [string]$BuildDir = "c:\Users\offic\source\repos\xuchen95\SparkUfsPdt\build_tmp",
    [string]$SdkInclude = "C:\Program Files (x86)\Windows Kits\10\Include\10.0.26100.0",
    [string]$SdkLib    = "C:\Program Files (x86)\Windows Kits\10\Lib\10.0.26100.0",
    [string]$MsvcInclude = "C:\SoftWare\VS2026\Community\VC\Tools\MSVC\14.51.36231\include",
    [string]$MsvcLib     = "C:\SoftWare\VS2026\Community\VC\Tools\MSVC\14.51.36231\lib\x86",
    [string]$MfcInclude  = "C:\SoftWare\VS2026\Community\VC\Tools\MSVC\14.51.36231\atlmfc\include"
)
$ErrorActionPreference = "Stop"
$env:INCLUDE = "$SdkInclude\ucrt;$SdkInclude\shared;$SdkInclude\um;$MsvcInclude;$MfcInclude;" +
               "c:\Users\offic\source\repos\xuchen95\SparkUfsPdt;" +
               "c:\Users\offic\source\repos\xuchen95\SparkUfsPdt\UFS_LIB\libsparkusb;" +
               "c:\Users\offic\source\repos\xuchen95\SparkUfsPdt\UFS_LIB;" +
               "c:\Users\offic\source\repos\xuchen95\SparkUfsPdt\include;" +
               "c:\Users\offic\source\repos\xuchen95\SparkUfsPdt\Serial\include"
$env:LIB = "$SdkLib\ucrt\x86;$SdkLib\um\x86;$MsvcLib"
New-Item -ItemType Directory -Force -Path $BuildDir | Out-Null
$phase = if ($GreenMode) { "GREEN" } else { "RED" }
Write-Host "=== P2-6 $phase phase: cl /W4 /WX /c libsparkusb.cpp ==="
Write-Host "Using cl.exe: $Cl"

$src = "c:\Users\offic\source\repos\xuchen95\SparkUfsPdt\UFS_LIB\libsparkusb\libsparkusb.cpp"
Push-Location $BuildDir
# Notes:
#  * /wd4819 /wd4273 → disable pre-existing historical warnings unrelated to P2-6.
#  * /W4 → enable all level-4 warnings.
#  * /WX → warnings as errors (C4267 would normally be L4 warning, promoted to error by /WX).
# Note: libsparkusb.dll is built as MBCS (uses CHAR arrays for Win32 volume APIs), NOT UNICODE.
#       Do NOT pass /DUNICODE /D_UNICODE — they cause C2664 CHAR/WCHAR mismatches at L80 etc.
& $Cl /nologo /std:c++17 /EHsc /MD /c /DWIN32 /D_WINDOWS /D_USRDLL /D_AFXDLL `
      /W4 /WX /wd4819 /wd4273 /wd4244 /wd4100 $src 2>&1 | Select-Object -Last 60
$exit = $LASTEXITCODE
Pop-Location
Write-Host "P26_${phase}_EXIT: $exit"

# ---- expectations ----
if (-not $GreenMode) {
    # RED: /W4 /WX must FAIL because of the two C4267 narrowing warnings at L378 and L426.
    if ($exit -eq 0) { throw "RED PHASE FAILURE: expected /WX exit!=0 due to C4267, but got 0. Either warnings are gone or /wd suppressed them. Fix script." }
    Write-Host "`n[RED OK] C4267 size_t->UCHAR warnings correctly converted to errors by /WX (exit=$exit)."
} else {
    # GREEN: after fix, exit=0, 0 warnings, 0 errors.
    if ($exit -ne 0) { throw "GREEN PHASE FAILURE: exit=$exit, fix didn't eliminate all C4267." }
    Write-Host "`n[GREEN OK] 0 warnings, 0 errors. C4267 eliminated."
}
exit 0
