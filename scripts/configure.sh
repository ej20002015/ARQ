#!/bin/bash
shift || true

SCRIPT_DIR=$(dirname "$(realpath "$0")")
pushd "$SCRIPT_DIR/.." > /dev/null
cmake -B .build -S . -G "Ninja Multi-Config" "$@"
ec=$?
popd > /dev/null
exit $ec
