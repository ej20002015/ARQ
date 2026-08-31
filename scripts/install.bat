@echo off
setlocal EnableDelayedExpansion

set "SCRIPT_DIR=%~dp0"
set CONFIG=Release
set CLEAN=1

if /I "%~1"=="dbg" set CONFIG=Debug
if /I "%~1"=="debug" set CONFIG=Debug
shift

set "FORWARD_ARGS="
:collect_forward_args
if "%~1"=="" goto run
if /I "%~1"=="noclean" (
    set CLEAN=0
) else (
    set "FORWARD_ARGS=!FORWARD_ARGS! "%~1""
)
shift
goto collect_forward_args

:run
pushd "!SCRIPT_DIR!..\"

if %CLEAN%==1 (
    if exist .install (
        rmdir /S /Q .install
    )
)

cmake --install .build --config %CONFIG% --prefix .install !FORWARD_ARGS!
set EC=%ERRORLEVEL%

popd


endlocal & exit /B %EC%
