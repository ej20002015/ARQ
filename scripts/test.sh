#!/bin/bash

CONFIG=Release

if [[ "${1,,}" == "dbg" || "${1,,}" == "debug" ]]; then
    CONFIG=Debug
fi
shift || true

TEST_KIND=$1
shift || true

SCRIPT_DIR=$(dirname "$(realpath "$0")")

pushd "$SCRIPT_DIR/.." > /dev/null

if [[ "$TEST_KIND" == "benchmark" ]]; then
    ctest --test-dir .build -C "$CONFIG" -L benchmark --output-on-failure "$@"
else
    ctest --test-dir .build -C "$CONFIG" -LE benchmark --parallel --output-on-failure "$@"
fi
ec=$?

popd > /dev/null

exit $ec
