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
    exit /b 1
)
mingw32-make -f src\Makefile.win32 %*
if errorlevel 1 exit /b %errorlevel%
echo.
echo Built src\python.exe
