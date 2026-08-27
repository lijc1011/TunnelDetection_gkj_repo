@echo off
setlocal
cd /d "%~dp0"
set "LOG=%CD%\field_self_test.log"
echo ==== TunnelDetection field self-test %DATE% %TIME% ==== > "%LOG%"

if not exist "TunnelDetection.exe" (
  echo FAIL: TunnelDetection.exe missing>>"%LOG%"
  type "%LOG%"
  exit /b 1
)
if not exist "TsZarrBridge.dll" (
  echo FAIL: TsZarrBridge.dll missing>>"%LOG%"
  type "%LOG%"
  exit /b 1
)

echo [1/2] TensorStore Zarr write/read and overwrite guard>>"%LOG%"
"%CD%\TsZarrBench.exe" --self-test >>"%LOG%" 2>&1
if errorlevel 1 (
  type "%LOG%"
  exit /b 1
)

echo [2/2] normalization rule validation>>"%LOG%"
"%CD%\ZarrRulesSelfTest.exe" >>"%LOG%" 2>&1
if errorlevel 1 (
  type "%LOG%"
  exit /b 1
)

echo FIELD SELF-TEST PASS>>"%LOG%"
type "%LOG%"
exit /b 0
