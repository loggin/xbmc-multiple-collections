# Build script for Kodi Multi Collections (Windows, Visual Studio 2022, x64)
#
# *** CRITICAL: Run this script FIRST before every build session ***
# The project was originally configured on L: drive. This machine has the source
# on F:\work\Kodi\Multi Collections. Without the L: subst, ZERO_CHECK fails and
# the build will not start.
#
# This script maps L: → F:\work\Kodi\Multi Collections automatically.
# If the drive mapping is lost (e.g. after reboot), it is re-applied each run.
#
# Build output: kodi-build.x64\Release\kodi.exe  (or \Debug\kodi.exe)
#
# If build fails with C1902 (PDB manager mismatch): a stale mspdbsrv.exe is
# running. This script kills it automatically before building.
#
# Usage:
#   .\build.ps1                     # Release (default), full kodi target
#   .\build.ps1 Debug               # Debug build
#   .\build.ps1 Release libkodi     # Fast compile check — just xbmc/** sources

param(
    [string]$Config = "Release",
    [string]$Target = "kodi",    # "kodi" = full build; "libkodi" = fast compile-only
    [int]$Jobs = 0               # 0 = let MSBuild choose (/m)
)

# ── Step 1: Map L: drive (required — project was configured on L:/) ──────────
$lMapped = (subst L: 2>$null | Select-String "L:\\:" | Measure-Object).Count -gt 0
if (-not $lMapped) {
    subst L: "F:\work\Kodi\Multi Collections" | Out-Null
    Write-Host "L: drive mapped to F:\work\Kodi\Multi Collections" -ForegroundColor Yellow
} else {
    Write-Host "L: drive already mapped" -ForegroundColor DarkGray
}

# ── Step 2: Kill stale mspdbsrv.exe (prevents C1902 PDB mismatch errors) ────
Stop-Process -Name "mspdbsrv" -Force -ErrorAction SilentlyContinue

# ── Step 3: Find MSBuild (VS 2022 required — toolset v143) ───────────────────
$msbuild = "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe"
if (-not (Test-Path $msbuild)) {
    $vsWhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    $msbuild = & $vsWhere -latest -requires Microsoft.Component.MSBuild -find MSBuild\**\Bin\MSBuild.exe |
               Select-Object -First 1
}

$parallelFlag = if ($Jobs -gt 0) { "/m:$Jobs" } else { "/m" }
$slnDir = Join-Path $PSScriptRoot "kodi-build.x64"   # ← real cmake build dir

# ── Step 4: Build ─────────────────────────────────────────────────────────────
if ($Target -eq "libkodi") {
    # Fast compile of xbmc/** sources — verifies code changes compile without
    # waiting for the full link. Skips project references (FFmpeg, SWIG etc.)
    $proj = Join-Path $slnDir "libkodi.vcxproj"
    Write-Host "Building $Config (libkodi — fast compile check)" -ForegroundColor Cyan
    & $msbuild $proj `
        /p:Configuration=$Config /p:Platform=x64 `
        /p:BuildProjectReferences=false `
        $parallelFlag /nologo /clp:Summary `
        2>&1 | Where-Object { $_ -match "error C|Build succeeded|FAILED|error MSB" }
} else {
    $sln = Join-Path $slnDir "kodi.sln"
    Write-Host "Building $Config via kodi-build.x64\kodi.sln /t:kodi" -ForegroundColor Cyan
    & $msbuild $sln `
        /p:Configuration=$Config /p:Platform=x64 `
        $parallelFlag /nologo /clp:Summary /t:kodi `
        2>&1 | Where-Object { $_ -match "error C|Build succeeded|FAILED|error MSB" }
}

exit $LASTEXITCODE
