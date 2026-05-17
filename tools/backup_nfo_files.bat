@echo off
setlocal EnableExtensions EnableDelayedExpansion

if "%~1"=="" goto :usage
if "%~2"=="" goto :usage

set "SOURCE_ROOT=%~1"
set "ARCHIVE_FILE=%~2"

if not exist "%SOURCE_ROOT%" (
  echo Source root not found: "%SOURCE_ROOT%"
  exit /b 1
)

where 7z >nul 2>nul
if errorlevel 1 (
  echo 7z.exe was not found on PATH.
  exit /b 1
)

for %%I in ("%ARCHIVE_FILE%") do set "ARCHIVE_DIR=%%~dpI"
if not exist "%ARCHIVE_DIR%" mkdir "%ARCHIVE_DIR%" >nul 2>nul

pushd "%SOURCE_ROOT%" || exit /b 1

for /r %%F in (*.nfo) do goto :has_nfo

echo No .nfo files were found under "%SOURCE_ROOT%".
popd
exit /b 2

:has_nfo

echo Creating archive: "%ARCHIVE_FILE%"
7z a -tzip "%ARCHIVE_FILE%" "*.nfo" -r
set "RC=%ERRORLEVEL%"

popd
exit /b %RC%

:usage
echo Usage: %~nx0 ^<source_root^> ^<archive.zip^>
echo Example: %~nx0 "D:\KodiLibrary" "D:\Backups\KodiLibrary_NFOs.zip"
exit /b 64
