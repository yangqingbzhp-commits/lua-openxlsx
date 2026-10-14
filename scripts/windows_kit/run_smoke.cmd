@echo off
rem ============================================================
rem  openxlsx Windows 自检（最小流程）
rem  直接双击本文件即可运行
rem ============================================================
setlocal
cd /d "%~dp0"
set LUA_CPATH=%~dp0?.dll;;
echo.
echo Using LUA_CPATH=%LUA_CPATH%
echo.
lua.exe smoke.lua
echo.
echo ---- 退出码: %ERRORLEVEL% ----
pause
