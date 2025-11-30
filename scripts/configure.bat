@echo off
pushd %~dp0\..\
cmake -B .build -S . -G "Visual Studio 18 2026"
popd