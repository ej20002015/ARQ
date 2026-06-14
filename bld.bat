@echo off
setlocal EnableDelayedExpansion

set ROOT=%~dp0
pushd "%ROOT%" || exit /B 1

set CMD_RAW=%1
shift

if "%CMD_RAW%"=="" (
    echo Usage: bld ^<command[d^|r]^> [args...]
    exit /B 1
)

set MODE=Release
set CMD=%CMD_RAW%

set LAST=!CMD_RAW:~-1!

if /I "!LAST!"=="d" (
    set MODE=Debug
    set CMD=!CMD_RAW:~0,-1!
) else if /I "!LAST!"=="r" (
    set MODE=Release
    set CMD=!CMD_RAW:~0,-1!
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

if /I "!CMD!"=="i" goto install
if /I "!CMD!"=="install" goto install

if /I "!CMD!"=="c" goto configure
if /I "!CMD!"=="configure" goto configure

if /I "!CMD!"=="g" goto codegen
if /I "!CMD!"=="cg" goto codegen
if /I "!CMD!"=="codegen" goto codegen

if /I "!CMD!"=="d" goto dockerbuild
if /I "!CMD!"=="dockerbuild" goto dockerbuild

echo Unknown command: !CMD!
exit /B 1

REM ----------------------------
REM Dispatch
REM ----------------------------

:build
call scripts\build.bat !MODE! %*
goto end

:clean
call scripts\clean.bat !MODE! %*
goto end

:test
call scripts\test.bat !MODE! %*
goto end

:install
call scripts\install.bat !MODE! %*
goto end

:configure
call scripts\configure.bat !MODE! %*
goto end

:codegen
call scripts\codegen.bat !MODE! %*
goto end

:dockerbuild
call scripts\dockerbuild.bat !MODE! %*
goto end

:end
popd
endlocal
