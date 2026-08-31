@echo off
setlocal EnableDelayedExpansion

set "SCRIPT_DIR=%~dp0"
set CONFIG=Release

if /I "%~1"=="dbg" set CONFIG=Debug
if /I "%~1"=="debug" set CONFIG=Debug
shift

set "FORWARD_ARGS="
:collect_forward_args
if "%~1"=="" goto run
set "FORWARD_ARGS=!FORWARD_ARGS! "%~1""
shift
goto collect_forward_args

:run
pushd "!SCRIPT_DIR!..\"
cmake --build .build --config %CONFIG% --parallel --target clean !FORWARD_ARGS!
set EC=%ERRORLEVEL%
popd

endlocal & exit /B %EC%
