@echo off

set CONFIG=Release
set CLEAN=1

if /I "%1"=="dbg" set CONFIG=Debug
if /I "%1"=="debug" set CONFIG=Debug

if /I "%1"=="noclean" set CLEAN=0
if /I "%2"=="noclean" set CLEAN=0

pushd %~dp0\..\

if %CLEAN%==1 (
    if exist .install (
        rmdir /S /Q .install
    )
)

cmake --install .build --config %CONFIG% --prefix .install

popd