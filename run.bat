@echo off
setlocal

echo [SCRIPT] Setting up MSVC environment...

REM This script needs the Developer Command Prompt environment.
if not defined DevEnvDir (
    call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
    if %errorlevel% neq 0 (
        echo [ERROR] Could not find vcvars64.bat to set up the MSVC environment.
        exit /b 1
    )
)

echo [SCRIPT] Configuring project with CMake...
if not exist build (
    mkdir build
)
cmake -S . -B build -G "Ninja" -DCMAKE_C_COMPILER=clang-cl -DCMAKE_CXX_COMPILER=clang-cl
if %errorlevel% neq 0 (
    echo [ERROR] CMake configuration failed.
    exit /b %errorlevel%
)

echo [SCRIPT] Building project...
cmake --build build
if %errorlevel% neq 0 (
    echo [ERROR] Build failed.
    exit /b %errorlevel%
)

REM --- This is the updated section ---

REM Check if an example name was provided as an argument.
if "%~1"=="" (
    echo.
    echo [SCRIPT] Build successful. No example specified to run.
    echo Please provide the name of the example you want to run.
    echo For example: %0 00_Init
    goto :EOF
)

echo.
echo [SCRIPT] Build successful. Running '%1'...
echo -------------------------------------------------
echo.

REM The executables are placed in the 'build/bin' directory by CMake.
REM We run the specified example from within that directory.
cd build\bin
%1.exe
cd ..\..

echo.
echo -------------------------------------------------
echo [SCRIPT] Application '%1' finished.

endlocal
:EOF
