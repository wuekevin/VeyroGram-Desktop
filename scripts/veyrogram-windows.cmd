@echo off
setlocal EnableExtensions
set "REPO=%~dp0.."
for %%I in ("%REPO%") do set "REPO=%%~fI"
set "MODE=%~1"
if "%MODE%"=="" set "MODE=all"
if /I not "%MODE%"=="prepare" if /I not "%MODE%"=="build" if /I not "%MODE%"=="all" (
  echo Usage: scripts\veyrogram-windows.cmd [prepare^|build^|all]
  exit /b 2
)
if not exist "%REPO%\Telegram\build\prepare\win.bat" (
  echo ERROR: Missing upstream Windows preparation script.
  exit /b 1
)
if not exist "%REPO%\Telegram\configure.bat" (
  echo ERROR: Missing Telegram\configure.bat.
  exit /b 1
)
where git >nul 2>&1 || (echo ERROR: Git missing from PATH. & exit /b 1)
where python >nul 2>&1 || (echo ERROR: Python missing from PATH. & exit /b 1)
where cmake >nul 2>&1 || (echo ERROR: CMake missing from PATH. & exit /b 1)

if /I not "%VSCMD_ARG_TGT_ARCH%"=="x64" (
  if exist "%ProgramFiles%\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat" (
    call "%ProgramFiles%\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64
  ) else (
    echo ERROR: Start an x64 Native Tools Command Prompt or install VS2022 Community.
    exit /b 1
  )
)
if errorlevel 1 exit /b 1
where cl >nul 2>&1 || (echo ERROR: MSVC cl.exe unavailable. & exit /b 1)

cd /d "%REPO%" || exit /b 1
echo [VeyroGram] Synchronizing recursive submodules...
git submodule sync --recursive || exit /b 1
git submodule update --init --recursive || exit /b 1

for %%I in ("%REPO%\..") do set "PARENT=%%~fI"
if not exist "%PARENT%\ThirdParty" mkdir "%PARENT%\ThirdParty"
if not exist "%PARENT%\Libraries" mkdir "%PARENT%\Libraries"

if /I "%MODE%"=="build" goto :build
echo [VeyroGram] Preparing pinned upstream Windows dependencies (including RNNoise)...
call "%REPO%\Telegram\build\prepare\win.bat" qt6
if errorlevel 1 exit /b 1
if /I "%MODE%"=="prepare" exit /b 0

:build
if not defined TDESKTOP_API_ID (
  echo ERROR: Set TDESKTOP_API_ID in this CMD session.
  exit /b 1
)
if not defined TDESKTOP_API_HASH (
  echo ERROR: Set TDESKTOP_API_HASH in this CMD session.
  exit /b 1
)
echo [VeyroGram] Configuring using upstream entry point...
cd /d "%REPO%\Telegram" || exit /b 1
call configure.bat x64 qt6 -D TDESKTOP_API_ID=%TDESKTOP_API_ID% -D TDESKTOP_API_HASH=%TDESKTOP_API_HASH%
if errorlevel 1 exit /b 1
echo [VeyroGram] Building Release x64...
cd /d "%REPO%" || exit /b 1
cmake --build out --config Release --target Telegram --parallel 8
if errorlevel 1 exit /b 1
echo [VeyroGram] Build finished. Inspect "%REPO%\out\Release".
exit /b 0
