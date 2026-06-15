#!/bin/bash
SCRIPT_DIR=$(dirname "$(realpath "$0")")
pushd "$SCRIPT_DIR/.." > /dev/null
docker build -t arq-web:latest -f infra/docker/arq-web/Dockerfile
popd > /dev/null