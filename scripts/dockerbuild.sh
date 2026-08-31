#!/bin/bash
shift || true

SCRIPT_DIR=$(dirname "$(realpath "$0")")
pushd "$SCRIPT_DIR/.." > /dev/null
docker build -t arq-local:latest -f infra/docker/arq/Dockerfile "$@" .
ec=$?
popd > /dev/null
exit $ec
