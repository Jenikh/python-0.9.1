@echo off
setlocal
where gcc >nul 2>nul
if errorlevel 1 (
    echo MinGW-w64 GCC was not found in PATH.
    echo Install MinGW-w64 and add its bin directory to PATH.
    exit /b 1
)
where mingw32-make >nul 2>nul
if errorlevel 1 (
    echo mingw32-make was not found in PATH.
    echo Install MinGW-w64 and add its bin directory to PATH.
    exit /b 1
)

pushd "%~dp0src"
mingw32-make -f Makefile.win32 %*
set "status=%errorlevel%"
popd

if not "%status%"=="0" exit /b %status%
echo.
echo Built src\python.exe
