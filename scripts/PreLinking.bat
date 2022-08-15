@echo off

if [%1]==[] goto :ErrorEmpty

if "%~1" =="Clean" goto :Clean

if "%~1" =="Build" goto :Build


goto :ErrorEmpty

:Clean
echo ====== Running Clean ======
..\SDL\VisualC\clean.sh
echo ====== Clean Completed ======

:Build
echo ====== Running SDL Build ======
py %~dp0\sdl_build.py %~2
echo ====== SDL Build Completed======
goto :End
	
:ErrorEmpty
echo Usage: "PreLink.bat [Clean | Build]"


:End