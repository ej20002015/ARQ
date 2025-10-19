@echo off
pushd %~dp0\..\
python codegen\script\gen.py --definitions-dir codegen\definitions --template-dir codegen\templates --output-dir .
popd