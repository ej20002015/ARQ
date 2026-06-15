@echo off
pushd %~dp0\..\
docker build -t arq-local:latest -f infra\docker\arq\Dockerfile .
popd