# Build script for Kodi Multi Collections (Windows, Visual Studio 2022, x64)
#
# The repo lives at F:\work\Kodi\Multi Collections\master. CMake is configured
# against the stable junction F:\work\Kodi\builds\master (created via
# `mklink /J F:\work\Kodi\builds\master "F:\work\Kodi\Multi Collections\master"`)
# so the build survives future renames/moves of the "Multi Collections" folder —
# only the junction target needs to change, not the CMake cache. No drive
# substitution (subst) is used anywhere in this build - use the junction only.
#
# KNOWN ISSUE: Kodi's own Windows FFmpeg/MSYS build tooling (invoked by CMake's
# build-ffmpeg custom-build step) independently expects an "L:" drive to exist
# somewhere in that chain (root cause not isolated - it's created/torn down
# within a single build-ffmpeg invocation, too fast to catch live via polling,
# and isn't a literal `subst` call anywhere under tools/ or
# project/BuildDependencies/scripts). Without L: mapped, ffmpeg's ./configure
# bakes a stale absolute L:/... path into its generated Makefiles and the
# build-ffmpeg step fails. Fixing that for real means finding the actual call
# site; deleting project/BuildDependencies/build/src/ffmpeg-8.1 forces it to
# regenerate but does not avoid the underlying dependency on L:.
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

$buildRoot = "F:\work\Kodi\builds\master"
if (-not (Test-Path $buildRoot)) {
    Write-Error "Build junction not found: $buildRoot (run: mklink /J `"$buildRoot`" `"$PSScriptRoot`")"
    exit 1
}


# ── Step 2: Kill stale mspdbsrv.exe (prevents C1902 PDB mismatch errors) ────
# Also ensure Kodi is not running, as rebuilding while kodi.exe is active can
# keep files locked and produce misleading build/link failures.
$kodiProcesses = Get-Process -Name "kodi" -ErrorAction SilentlyContinue
while ($kodiProcesses)
{
    Write-Host "Kodi is currently running (PID(s): $($kodiProcesses.Id -join ', '))." -ForegroundColor Yellow
    $choice = Read-Host "Please close Kodi, then press Enter to continue (or type Q to cancel build)"
    if ($choice -match '^(q|Q)$')
    {
        Write-Host "Build canceled by user because Kodi is still running." -ForegroundColor Red
        exit 1
    }

    $kodiProcesses = Get-Process -Name "kodi" -ErrorAction SilentlyContinue
}

Stop-Process -Name "mspdbsrv" -Force -ErrorAction SilentlyContinue

# ── Step 3: Find MSBuild (VS 2022 required — toolset v143) ───────────────────
$msbuild = "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe"
if (-not (Test-Path $msbuild)) {
    $vsWhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    $msbuild = & $vsWhere -latest -requires Microsoft.Component.MSBuild -find MSBuild\**\Bin\MSBuild.exe |
               Select-Object -First 1
}

$parallelFlag = if ($Jobs -gt 0) { "/m:$Jobs" } else { "/m" }
$slnDir = Join-Path $buildRoot "kodi-build.x64"   # ← real cmake build dir, via the stable junction

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
