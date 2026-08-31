@echo off
setlocal EnableDelayedExpansion

set "SCRIPT_DIR=%~dp0"
set CONFIG=Release

if /I "%~1"=="dbg" set CONFIG=Debug
if /I "%~1"=="debug" set CONFIG=Debug
shift

set "TEST_KIND=%~1"
shift

set "FORWARD_ARGS="
:collect_forward_args
if "%~1"=="" goto run
set "FORWARD_ARGS=!FORWARD_ARGS! "%~1""
shift
goto collect_forward_args

:run
pushd "!SCRIPT_DIR!..\"

if /I "!TEST_KIND!"=="benchmark" (
    ctest --test-dir .build -C %CONFIG% -L benchmark --output-on-failure !FORWARD_ARGS!
) else (
    ctest --test-dir .build -C %CONFIG% -LE benchmark --parallel --output-on-failure !FORWARD_ARGS!
)

set EC=%ERRORLEVEL%

popd

endlocal & exit /B %EC%
