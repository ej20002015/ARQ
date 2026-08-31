@echo off
setlocal EnableDelayedExpansion

set "SCRIPT_DIR=%~dp0"
shift

set "FORWARD_ARGS="
:collect_forward_args
if "%~1"=="" goto run
set "FORWARD_ARGS=!FORWARD_ARGS! "%~1""
shift
goto collect_forward_args

:run
pushd "!SCRIPT_DIR!..\"
docker build -t arq-local:latest -f infra\docker\arq\Dockerfile !FORWARD_ARGS! .
set EC=%ERRORLEVEL%
popd

endlocal & exit /B %EC%
