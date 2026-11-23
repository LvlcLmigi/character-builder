@echo off
rem Builds Character Studio. Double-click this file, or run "build.bat" from any terminal.
rem Optional argument: debug or release (default: release).
setlocal
cd /d "%~dp0"

set "PRESET=%~1"
if "%PRESET%"=="" set "PRESET=release"

rem Find Visual Studio 2022 with the C++ tools and load its 64-bit build environment.
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" (
    echo Could not find the Visual Studio Installer. Is Visual Studio 2022 installed?
    goto :fail
)
set "VSDIR="
for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSDIR=%%i"
if not defined VSDIR (
    echo Could not find Visual Studio with the "Desktop development with C++" workload.
    goto :fail
)
call "%VSDIR%\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 goto :fail

echo === Configuring (%PRESET%) ===
cmake --preset %PRESET%
if errorlevel 1 goto :fail

echo === Building (%PRESET%) ===
cmake --build --preset %PRESET%
if errorlevel 1 goto :fail

echo.
echo Build succeeded: build\%PRESET%\CharacterStudio.dll
if defined CHARACTER_STUDIO_MO2_MOD echo Copied to: %CHARACTER_STUDIO_MO2_MOD%\SKSE\Plugins
call :maybe_pause
exit /b 0

:fail
echo.
echo Build FAILED. Scroll up for the first error.
call :maybe_pause
exit /b 1

:maybe_pause
rem Keep the window open when the script was double-clicked.
echo %cmdcmdline% | find /i "%~0" >nul && pause
exit /b 0
