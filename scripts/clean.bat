@echo off
set CONFIG=Release

if /I "%1"=="dbg" set CONFIG=Debug
if /I "%1"=="debug" set CONFIG=Debug

pushd %~dp0\..\
cmake --build .build --config %CONFIG% --parallel --target clean
popd
