#!/bin/bash

CONFIG=Release

if [[ "$1" == "dbg" || "$1" == "debug" ]]; then
    CONFIG=Debug
fi

SCRIPT_DIR=$(dirname "$(realpath "$0")")

pushd "$SCRIPT_DIR/.." > /dev/null

ctest --test-dir .build -C "$CONFIG" --parallel --output-on-failure
ec=$?

popd > /dev/null

exit $ec