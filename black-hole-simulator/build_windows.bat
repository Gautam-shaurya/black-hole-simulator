@echo off
title Black Hole Simulator - one-time setup
cd /d "%~dp0"

rem ---- Safety check: this file only works together with the other files ----
if not exist "%~dp0build_windows.ps1" goto missing
if not exist "%~dp0src\main.c" goto missing
if not exist "%~dp0shaders\blackhole.frag" goto missing
goto ok

:missing
echo.
echo  =====================================================
echo   THE OTHER PROGRAM FILES ARE MISSING
echo  =====================================================
echo   This file must stay inside the full extracted folder,
echo   next to the folders named  src  and  shaders  and the
echo   file  build_windows.ps1
echo.
echo   What to do:
echo    1. Find the downloaded file  BlackHoleSimulator.zip
echo    2. Right-click it, choose  Extract All...  then Extract
echo    3. Open the NEW folder that appears
echo    4. Double-click build_windows.bat INSIDE that folder
echo.
echo   Do not copy build_windows.bat out of the folder.
echo.
echo   This folder is:  %~dp0
echo  =====================================================
echo.
pause
exit /b 1

:ok
echo.
echo  =====================================================
echo   BLACK HOLE SIMULATOR - ONE-TIME SETUP
echo  =====================================================
echo   This will download two free tools and create
echo   BlackHoleSimulator.exe in this folder.
echo   It needs an internet connection and takes 2-5 minutes.
echo   Please do not close this window.
echo  =====================================================
echo.
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0build_windows.ps1"
if errorlevel 1 goto failed
echo.
echo  =====================================================
echo   DONE! Double-click BlackHoleSimulator.exe to start.
echo  =====================================================
echo.
pause
exit /b 0

:failed
echo.
echo  =====================================================
echo   SOMETHING WENT WRONG. Please read the red text above.
echo   A copy was saved in the file build_error.log
echo  =====================================================
echo.
pause
exit /b 1
