@echo off
setlocal
cd /d "%~dp0"
if not exist "TunnelDetection.exe" (
  echo TunnelDetection.exe not found.
  pause
  exit /b 1
)
start "TunnelDetection Field Trial" /wait "%CD%\TunnelDetection.exe"
exit /b %ERRORLEVEL%
