@echo off
pushd %~dp0\..\
docker build -t arq:latest -f infra\docker\Dockerfile .
popd