#!/bin/bash

CONFIG=Release
CLEAN=1

if [[ "${1,,}" == "dbg" || "${1,,}" == "debug" ]]; then
    CONFIG=Debug
fi
shift || true

shopt -s nocasematch

FORWARD_ARGS=()
for arg in "$@"; do
    case "$arg" in
        noclean)
            CLEAN=0
            ;;
        *)
            FORWARD_ARGS+=( "$arg" )
            ;;
    esac
done

shopt -u nocasematch

SCRIPT_DIR=$(dirname "$(realpath "$0")")

pushd "$SCRIPT_DIR/.." > /dev/null

if [[ $CLEAN -eq 1 ]]; then
    rm -rf .install
fi

cmake --install .build --config "$CONFIG" --prefix .install "${FORWARD_ARGS[@]}"

ec=$?

popd > /dev/null

exit $ec
