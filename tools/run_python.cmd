@echo off
setlocal
set "apex_python=%USERPROFILE%\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe"
if exist "%apex_python%" goto bundled
where python.exe >nul 2>nul
if errorlevel 1 goto launcher
python.exe %*
exit /b %errorlevel%
:launcher
py -3 %*
exit /b %errorlevel%
:bundled
"%apex_python%" %*
exit /b %errorlevel%
