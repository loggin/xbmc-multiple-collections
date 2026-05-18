# Build script for Kodi Multi Collections (Windows, Visual Studio 2022, x64)
#
# The workspace is an in-source CMake build directory configured with:
#   cmake -G "Visual Studio 17 2022" -A x64 .
#
# Known pre-existing environment issues (unrelated to code changes):
#   1. libnfs-config.cmake had a hardcoded L: drive path → fixed to F: in place.
#   2. CMake ZERO_CHECK triggers a reconfigure when generate.stamp.depend is stale.
#      Run .\touch-stamps.ps1 to refresh stamp files if ZERO_CHECK blocks the build.
#   3. build-ffmpeg.vcxproj tries to download+build FFmpeg → pre-built expected.
#   4. python_binding.vcxproj needs SWIG-generated .i.cpp files → must run swig first.
#
# For code change verification (fast):
#   .\build.ps1 -Target libkodi     # compiles xbmc/** source only, no dependency build
#
# For full build:
#   .\build.ps1                     # Debug, full kodi.sln /t:kodi
#   .\build.ps1 Release             # Release
#
# Usage:
#   .\build.ps1 [Debug|Release] [-Target libkodi|kodi] [-Jobs N]

param(
    [string]$Config = "Debug",
    [string]$Target = "kodi",    # "kodi" = full build via kodi.sln; "libkodi" = fast compile-only
    [int]$Jobs = 0               # 0 = let MSBuild choose (/m with no number)
)

$msbuild = "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe"
if (-not (Test-Path $msbuild)) {
    $vsWhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    $msbuild = & $vsWhere -latest -requires Microsoft.Component.MSBuild -find MSBuild\**\Bin\MSBuild.exe |
               Select-Object -First 1
}

$parallelFlag = if ($Jobs -gt 0) { "/m:$Jobs" } else { "/m" }

if ($Target -eq "libkodi") {
    # Fast compile of xbmc/** sources only — no FFmpeg/SWIG/packaging dependency.
    # Expected failures: python_binding.vcxproj (missing SWIG .i.cpp) — pre-existing.
    $proj = Join-Path $PSScriptRoot "libkodi.vcxproj"
    Write-Host "Building $Config (libkodi only — fast compile check) via $proj" -ForegroundColor Cyan
    & $msbuild $proj `
        /p:Configuration=$Config /p:Platform=x64 `
        /p:BuildProjectReferences=false `
        $parallelFlag /nologo /clp:Summary `
        2>&1 | Where-Object { $_ -match "error C|Build succeeded|FAILED|error MSB" }
} else {
    $sln = Join-Path $PSScriptRoot "kodi.sln"
    Write-Host "Building $Config via $sln /t:kodi" -ForegroundColor Cyan
    & $msbuild $sln `
        /p:Configuration=$Config /p:Platform=x64 `
        $parallelFlag /nologo /clp:Summary /t:kodi `
        2>&1 | Where-Object { $_ -match "error C|Build succeeded|FAILED|error MSB" }
}

exit $LASTEXITCODE
