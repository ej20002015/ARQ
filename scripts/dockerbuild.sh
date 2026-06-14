#!/bin/bash
SCRIPT_DIR=$(dirname "$(realpath "$0")")
pushd "$SCRIPT_DIR/.." > /dev/null
docker build -t arq:latest -f infra/docker/Dockerfile .
popd > /dev/null