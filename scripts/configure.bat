@echo off
pushd %~dp0\..\
cmake -B .build -S . -G "Visual Studio 17 2022"
popd