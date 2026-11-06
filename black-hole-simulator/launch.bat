@echo off
rem Backup launcher. Normally just double-click BlackHoleSimulator.exe instead.
cd /d "%~dp0"
if not exist "BlackHoleSimulator.exe" (
    echo.
    echo  BlackHoleSimulator.exe was not found in this folder.
    echo  If this is your first time: double-click  build_windows.bat  once,
    echo  wait until it says DONE, then try again.
    echo.
    pause
    exit /b 1
)
if not exist "shaders\blackhole.frag" (
    echo.
    echo  The "shaders" folder is missing next to the program.
    echo  Please right-click the ZIP file, choose Extract All, and open the NEW folder.
    echo.
    pause
    exit /b 1
)
start "" /wait "BlackHoleSimulator.exe"
if errorlevel 1 (
    echo.
    echo  The simulator closed because of a problem.
    echo  Details were saved in the file error.log in this folder.
    echo.
    pause
)
