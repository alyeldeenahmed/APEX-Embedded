@echo off
setlocal
cd /d "%~dp0"
echo Close Arduino Serial Monitor and other COM3 windows. Disconnect motor power.
call "%~dp0tools\run_python.cmd" tools\check_live_link.py --port COM3
if errorlevel 1 echo The link check did not pass. Keep the message above.
pause
