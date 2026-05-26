@echo off

set CONFIG=Release

if /I "%1"=="dbg" set CONFIG=Debug
if /I "%1"=="debug" set CONFIG=Debug

pushd %~dp0\..\

ctest --test-dir .build -C %CONFIG% --parallel --output-on-failure

set EC=%ERRORLEVEL%

popd

exit /B %EC%