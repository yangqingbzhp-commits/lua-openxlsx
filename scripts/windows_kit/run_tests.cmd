@echo off
rem ============================================================
rem  openxlsx Windows 全套测试（320 项断言）
rem  直接双击本文件即可运行
rem ============================================================
setlocal enabledelayedexpansion
cd /d "%~dp0"

set LUA_CPATH=%~dp0?.dll;;
set OX_TEST_DIR=%~dp0test
set OX_TEST_OUT=%TEMP%\openxlsx-tests
if not exist "%OX_TEST_OUT%" mkdir "%OX_TEST_OUT%"

set FAILED=0
for %%f in ("%OX_TEST_DIR%\0*.lua") do (
    echo --- %%~nxf
    lua.exe "%%f"
    if errorlevel 1 (
        echo     FAILED
        set FAILED=1
    )
)

echo.
if "!FAILED!"=="0" (
    echo ALL TESTS PASSED
) else (
    echo TEST FAILURES PRESENT
)
echo.
pause
exit /b !FAILED!
