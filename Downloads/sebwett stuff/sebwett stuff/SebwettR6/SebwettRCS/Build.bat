@echo off
setlocal EnableDelayedExpansion

:: ============================================================
::  xim.gg RCS - Build Script
::  Requires: Visual Studio 2022 (any edition) with C++ workload
:: ============================================================

set PROJECT_NAME=XimGG
set OUT_DIR=bin
set OBJ_DIR=obj\%PROJECT_NAME%

:: ── Locate Visual Studio via vswhere ────────────────────────
set VSWHERE="%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist %VSWHERE% (
    echo [ERROR] vswhere.exe not found. Is Visual Studio installed?
    goto :fail
)

for /f "usebackq tokens=*" %%i in (
    `%VSWHERE% -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`
) do set VS_PATH=%%i

if "!VS_PATH!"=="" (
    echo [ERROR] No Visual Studio installation with C++ tools found.
    goto :fail
)

:: ── Set up the MSVC x64 environment ─────────────────────────
set VCVARS="!VS_PATH!\VC\Auxiliary\Build\vcvars64.bat"
if not exist !VCVARS! (
    echo [ERROR] vcvars64.bat not found at: !VCVARS!
    goto :fail
)

echo [INFO] Using Visual Studio at: !VS_PATH!
call !VCVARS! >nul 2>&1

:: ── Paths (relative to this .bat's location) ────────────────
set ROOT=%~dp0
:: Strip trailing backslash
if "!ROOT:~-1!"=="\" set ROOT=!ROOT:~0,-1!

:: Source files live alongside Build.bat (no R6Recoil subfolder)
set SRC_DIR=!ROOT!
:: ImGui lives in the sibling SebwettRecoil project's libs folder
set IMGUI_DIR=!ROOT!\..\..\SebwettRecoil\libs\imgui
set OUT_EXE=!ROOT!\!OUT_DIR!\!PROJECT_NAME!.exe
set OBJ_PATH=!ROOT!\!OBJ_DIR!

:: ── Create output directories ────────────────────────────────
if not exist "!ROOT!\!OUT_DIR!" mkdir "!ROOT!\!OUT_DIR!"
if not exist "!OBJ_PATH!" mkdir "!OBJ_PATH!"

echo.
echo [BUILD] Compiling xim.gg RCS ...
echo.

:: ── Source files ─────────────────────────────────────────────
set SOURCES=
set SOURCES=!SOURCES! "!SRC_DIR!\main.cpp"
set SOURCES=!SOURCES! "!SRC_DIR!\Core\Application\Application.cpp"
set SOURCES=!SOURCES! "!SRC_DIR!\Core\Config\Config.cpp"
set SOURCES=!SOURCES! "!SRC_DIR!\Core\Input\Hotkey.cpp"
set SOURCES=!SOURCES! "!SRC_DIR!\Core\Input\Mouse.cpp"
set SOURCES=!SOURCES! "!SRC_DIR!\Core\Operators\OperatorData.cpp"
set SOURCES=!SOURCES! "!SRC_DIR!\Core\Recoil\RecoilEngine.cpp"
set SOURCES=!SOURCES! "!SRC_DIR!\Rendering\Overlay\Overlay.cpp"
set SOURCES=!SOURCES! "!SRC_DIR!\Rendering\Menu\Menu.cpp"

:: ImGui sources
set SOURCES=!SOURCES! "!IMGUI_DIR!\imgui.cpp"
set SOURCES=!SOURCES! "!IMGUI_DIR!\imgui_draw.cpp"
set SOURCES=!SOURCES! "!IMGUI_DIR!\imgui_tables.cpp"
set SOURCES=!SOURCES! "!IMGUI_DIR!\imgui_widgets.cpp"
set SOURCES=!SOURCES! "!IMGUI_DIR!\imgui_impl_win32.cpp"
set SOURCES=!SOURCES! "!IMGUI_DIR!\imgui_impl_dx11.cpp"

:: ── Include directories ───────────────────────────────────────
set INCLUDES=
set INCLUDES=!INCLUDES! /I"!IMGUI_DIR!"
set INCLUDES=!INCLUDES! /I"!SRC_DIR!"
set INCLUDES=!INCLUDES! /I"!ROOT!\..\..\SebwettRecoil\libs"

:: ── Compiler flags ────────────────────────────────────────────
set CFLAGS=/nologo /std:c++17 /O2 /W3 /EHsc /MT
set CFLAGS=!CFLAGS! /D WIN32 /D _WINDOWS /D NDEBUG
set CFLAGS=!CFLAGS! /D _CRT_SECURE_NO_WARNINGS
set CFLAGS=!CFLAGS! /D WIN32_LEAN_AND_MEAN
set CFLAGS=!CFLAGS! /D NOMINMAX

:: ── Linker flags ──────────────────────────────────────────────
set LIBS=
set LIBS=!LIBS! d3d11.lib
set LIBS=!LIBS! dxgi.lib
set LIBS=!LIBS! d3dcompiler.lib
set LIBS=!LIBS! dwmapi.lib
set LIBS=!LIBS! setupapi.lib
set LIBS=!LIBS! hid.lib
set LIBS=!LIBS! advapi32.lib
set LIBS=!LIBS! user32.lib
set LIBS=!LIBS! kernel32.lib
set LIBS=!LIBS! gdi32.lib
set LIBS=!LIBS! shell32.lib
set LIBS=!LIBS! ole32.lib
set LIBS=!LIBS! uuid.lib

set LFLAGS=/SUBSYSTEM:WINDOWS /MACHINE:X64

:: ── Compile + Link ────────────────────────────────────────────
cl.exe !CFLAGS! !INCLUDES! !SOURCES! ^
    /Fo"!OBJ_PATH!\\" ^
    /Fe"!OUT_EXE!" ^
    /link !LFLAGS! !LIBS!

if !ERRORLEVEL! neq 0 goto :fail

echo.
echo [SUCCESS] Built: !OUT_EXE!
echo.
goto :done

:fail
echo.
echo [FAILED] Build did not complete. Check errors above.
echo.
exit /b 1

:done
endlocal
exit /b 0
