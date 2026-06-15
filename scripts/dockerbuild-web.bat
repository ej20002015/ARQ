@echo off
pushd %~dp0\..\
docker build -t arq-web:latest -f infra\docker\arq-web\Dockerfile .
popd