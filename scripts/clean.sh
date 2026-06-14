#!/bin/bash

CONFIG=Release

if [[ "${1,,}" == "dbg" || "${1,,}" == "debug" ]]; then
    CONFIG=Debug
fi

SCRIPT_DIR=$(dirname "$(realpath "$0")")

pushd "$SCRIPT_DIR/.." > /dev/null
cmake --build .build --config "$CONFIG" --parallel --target clean
ec=$?
popd > /dev/null

exit $ec
