@echo off
setlocal EnableDelayedExpansion

set ROOT=%~dp0
pushd "%ROOT%" || exit /B 1

set CMD_RAW=%1
shift

set "FORWARD_ARGS="
:collect_forward_args
if "%~1"=="" goto forward_args_collected
set "FORWARD_ARGS=!FORWARD_ARGS! "%~1""
shift
goto collect_forward_args

:forward_args_collected

if "%CMD_RAW%"=="" (
    echo Usage: bld ^<command[d^|r]^> [args...]
    exit /B 1
)

set MODE=Release
set CMD=%CMD_RAW%

set LAST=!CMD_RAW:~-1!

if /I "!LAST!"=="d" (
    if /I "!CMD!" NEQ "d" (
        set MODE=Debug
        set CMD=!CMD_RAW:~0,-1!
    )
) else (
    if /I "!LAST!"=="r" (
        set MODE=Release
        set CMD=!CMD_RAW:~0,-1!
    )
)

REM ----------------------------
REM Alias routing
REM ----------------------------

if /I "!CMD!"=="b" goto build
if /I "!CMD!"=="build" goto build

if /I "!CMD!"=="cl" goto clean
if /I "!CMD!"=="clean" goto clean

if /I "!CMD!"=="t" goto test
if /I "!CMD!"=="test" goto test

if /I "!CMD!"=="bm" goto benchmark
if /I "!CMD!"=="bench" goto benchmark
if /I "!CMD!"=="benchmark" goto benchmark

if /I "!CMD!"=="cov" goto coverage
if /I "!CMD!"=="coverage" goto coverage

if /I "!CMD!"=="i" goto install
if /I "!CMD!"=="install" goto install

if /I "!CMD!"=="c" goto configure
if /I "!CMD!"=="configure" goto configure

if /I "!CMD!"=="g" goto codegen
if /I "!CMD!"=="cg" goto codegen
if /I "!CMD!"=="codegen" goto codegen

if /I "!CMD!"=="d" goto dockerbuild
if /I "!CMD!"=="dockerbuild" goto dockerbuild

if /I "!CMD!"=="dw" goto dockerbuild-web
if /I "!CMD!"=="dockerbuild-web" goto dockerbuild-web

echo Unknown command: !CMD!
exit /B 1

REM ----------------------------
REM Dispatch
REM ----------------------------

:build
call scripts\build.bat !MODE! !FORWARD_ARGS!
set EC=!ERRORLEVEL!
goto end

:clean
call scripts\clean.bat !MODE! !FORWARD_ARGS!
set EC=!ERRORLEVEL!
goto end

:test
call scripts\test.bat !MODE! unit !FORWARD_ARGS!
set EC=!ERRORLEVEL!
goto end

:benchmark
call scripts\test.bat !MODE! benchmark !FORWARD_ARGS!
set EC=!ERRORLEVEL!
goto end

:coverage
echo Code coverage is only supported on Linux with GCC. Run: ./bld.sh coverage
set EC=1
goto end

:install
call scripts\install.bat !MODE! !FORWARD_ARGS!
set EC=!ERRORLEVEL!
goto end

:configure
call scripts\configure.bat !MODE! !FORWARD_ARGS!
set EC=!ERRORLEVEL!
goto end

:codegen
call scripts\codegen.bat !MODE! !FORWARD_ARGS!
set EC=!ERRORLEVEL!
goto end

:dockerbuild
call scripts\dockerbuild.bat !MODE! !FORWARD_ARGS!
set EC=!ERRORLEVEL!
goto end

:dockerbuild-web
call scripts\dockerbuild-web.bat !MODE! !FORWARD_ARGS!
set EC=!ERRORLEVEL!
goto end

:end
popd
endlocal & exit /B %EC%
