@echo off
setlocal
cd /d "%~dp0"
echo Close Arduino Serial Monitor and other COM3 windows. Press Ctrl+C to end recording.
call "%~dp0tools\run_python.cmd" slow_monitor.py --port COM3 --display-hz 1 --record "slow_view_%RANDOM%_%RANDOM%.jsonl"
pause
