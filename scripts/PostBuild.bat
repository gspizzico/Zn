@echo off

rem if [%1]==[] goto :ErrorEmpty
rem if [%2]==[] goto :ErrorEmpty

rem if "%~1" =="Clean" goto :Clean

rem if "%~1" =="Build" goto :Build


rem goto :ErrorEmpty

rem :Clean
rem echo ====== Running Clean ======
rem ..\Source\ThirdParty\SDL\VisualC\clean.sh
rem echo ====== Clean Completed ======

echo ====== Running Shader Compilation ======
py %~dp0vk_compile_shaders.py
echo ====== Completed Shader Compilation ======

:End