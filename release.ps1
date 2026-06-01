# release.ps1 — Build Kodi Multi Collections and produce a Windows installer (.exe)
#
# Usage:
#   .\release.ps1                    # build Release, then package
#   .\release.ps1 -SkipBuild         # skip MSBuild, just package existing Release output
#   .\release.ps1 -OutDir "D:\dist"  # write installer to a custom directory
#
# Output: installer\KodiMultiCollections-<version>-setup.exe
# Requires: NSIS 3.x installed (makensis.exe in PATH or default location)

param(
    [switch]$SkipBuild,
    [string]$OutDir = (Join-Path $PSScriptRoot "installer")
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

# ── Resolve paths ────────────────────────────────────────────────────────────
$root      = $PSScriptRoot
$buildDir  = Join-Path $root "kodi-build.x64\Release"
$assetsDir = Join-Path $root "tools\windows\packaging\media"
$nsiScript = Join-Path $root "installer\kodi-multicollections.nsi"

# ── Read version ─────────────────────────────────────────────────────────────
$versionFile = Join-Path $root "version.txt"
$ver = @{}
Get-Content $versionFile | ForEach-Object {
    if ($_ -match '^\s*(\w+)\s+(.+)$') { $ver[$Matches[1]] = $Matches[2].Trim() }
}
$vMajor = $ver['VERSION_MAJOR']
$vMinor = $ver['VERSION_MINOR']
$vTag   = $ver['VERSION_TAG']
$vCode  = $ver['VERSION_CODE']
$versionStr = "${vMajor}.${vMinor}-${vTag}"
Write-Host "Version: $versionStr  ($vCode)" -ForegroundColor Cyan

# ── Find NSIS ────────────────────────────────────────────────────────────────
$makensis = Get-Command makensis -ErrorAction SilentlyContinue | Select-Object -ExpandProperty Source
if (-not $makensis) {
    $makensis = "C:\Program Files (x86)\NSIS\makensis.exe"
}
if (-not (Test-Path $makensis)) {
    Write-Error "makensis.exe not found. Install NSIS 3.x from https://nsis.sourceforge.io/"
}
Write-Host "NSIS: $makensis" -ForegroundColor DarkGray

# ── Step 1: Build (unless skipped) ───────────────────────────────────────────
if (-not $SkipBuild) {
    Write-Host "`nBuilding Release..." -ForegroundColor Cyan
    $buildScript = Join-Path $root "build.ps1"
    & $buildScript Release
    if ($LASTEXITCODE -ne 0) {
        Write-Error "Build failed (exit $LASTEXITCODE). Fix errors before packaging."
    }
    Write-Host "Build succeeded." -ForegroundColor Green
} else {
    Write-Host "Skipping build (-SkipBuild)." -ForegroundColor Yellow
}

# ── Step 2: Verify release output ────────────────────────────────────────────
if (-not (Test-Path (Join-Path $buildDir "kodi.exe"))) {
    Write-Error "kodi.exe not found in $buildDir — build may have failed or -SkipBuild used without a prior build."
}

# ── Step 3: Run NSIS ─────────────────────────────────────────────────────────
$null = New-Item -ItemType Directory -Path $OutDir -Force
$outFile = Join-Path $OutDir "KodiMultiCollections-${versionStr}-setup.exe"

Write-Host "`nPackaging installer -> $outFile" -ForegroundColor Cyan

& $makensis /V2 `
    /DVERSION_MAJOR=$vMajor `
    /DVERSION_MINOR=$vMinor `
    /DVERSION_TAG=$vTag `
    /DVERSION_CODE=$vCode `
    /DBUILD_DIR=$buildDir `
    /DASSETS_DIR=$assetsDir `
    /DOUTFILE=$outFile `
    $nsiScript

if ($LASTEXITCODE -ne 0) {
    Write-Error "NSIS packaging failed (exit $LASTEXITCODE)."
}

# ── Done ─────────────────────────────────────────────────────────────────────
$size = [math]::Round((Get-Item $outFile).Length / 1MB, 1)
Write-Host "`nInstaller ready: $outFile ($size MB)" -ForegroundColor Green
