@echo off
setlocal EnableExtensions EnableDelayedExpansion

echo Starting Windows test suite...

REM Check if cl.exe (MSVC) is available
echo Checking for MSVC cl.exe...
where cl.exe >nul 2>&1 || (
    echo ERROR: MSVC cl.exe not found in PATH
    echo Please run this from a Visual Studio Developer Command Prompt
    exit /b 1
)

REM Check if clang is available
echo Checking for clang...
where clang.exe >nul 2>&1 || (
    echo ERROR: clang.exe not found in PATH
    echo Please install clang and add it to your PATH
    exit /b 1
)

REM Create output directory if it doesn't exist
if not exist "tests\bin" mkdir "tests\bin"

echo.
echo Finding test files...
dir /b /a-d "tests\test_*.c" >nul 2>&1 || (
    echo No test files found starting with "test_" in tests directory
    exit /b 1
)

for %%f in ("tests\test_*.c") do (
    echo.
    echo === Testing %%~nxf ===

    REM Base filename without extension
    set "base_name=%%~nf"

    REM Test 1: MSVC cl with no optimization
    echo.
    echo Compiling: !base_name! with MSVC cl (no optimization)...
    cl.exe /nologo /TC /Od /Zi "%%~ff" /Fe:"tests\bin\!base_name!_msvc_debug.exe" >nul 2>&1
    if !errorlevel! neq 0 (
        echo ERROR: Failed to compile %%~nxf with MSVC (no optimization)
        exit /b 1
    )
    "tests\bin\!base_name!_msvc_debug.exe"
    if !errorlevel! neq 0 (
        echo ERROR: Test %%~nxf failed with MSVC (no optimization)
        exit /b 1
    )

    REM Test 2: MSVC cl with highest optimization and native features
    echo.
    echo Compiling: !base_name! with MSVC cl (O2 + native)...
    cl.exe /nologo /TC /O2 /arch:AVX2 /favor:INTEL64 "%%~ff" /Fe:"tests\bin\!base_name!_msvc_release.exe" >nul 2>&1
    if !errorlevel! neq 0 (
        echo ERROR: Failed to compile %%~nxf with MSVC (O2 + native)
        exit /b 1
    )
    "tests\bin\!base_name!_msvc_release.exe"
    if !errorlevel! neq 0 (
        echo ERROR: Test %%~nxf failed with MSVC (O2 + native)
        exit /b 1
    )

    REM Test 3: Clang with O0
    echo.
    echo Compiling: !base_name! with clang (O0)...
    clang.exe -std=c11 -O0 -g "%%~ff" -o "tests\bin\!base_name!_clang_debug.exe" >nul 2>&1
    if !errorlevel! neq 0 (
        echo ERROR: Failed to compile %%~nxf with clang (O0)
        exit /b 1
    )
    "tests\bin\!base_name!_clang_debug.exe"
    if !errorlevel! neq 0 (
        echo ERROR: Test %%~nxf failed with clang (O0)
        exit /b 1
    )

    REM Test 4: Clang with O3 and march=native
    echo.
    echo Compiling: !base_name! with clang (O3 + march=native)...
    clang.exe -std=c11 -O3 -march=native "%%~ff" -o "tests\bin\!base_name!_clang_release.exe" >nul 2>&1
    if !errorlevel! neq 0 (
        echo ERROR: Failed to compile %%~nxf with clang (O3 + march=native)
        exit /b 1
    )
    "tests\bin\!base_name!_clang_release.exe"
    if !errorlevel! neq 0 (
        echo ERROR: Test %%~nxf failed with clang (O3 + march=native)
        exit /b 1
    )
)

echo.
exit /b 0
