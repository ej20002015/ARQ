#!/bin/bash

CONFIG=Release
CLEAN=1

shopt -s nocasematch

for arg in "$@"; do
    case "$arg" in
        dbg|debug)
            CONFIG=Debug
            ;;
        noclean)
            CLEAN=0
            ;;
    esac
done

shopt -u nocasematch

SCRIPT_DIR=$(dirname "$(realpath "$0")")

pushd "$SCRIPT_DIR/.." > /dev/null

if [[ $CLEAN -eq 1 ]]; then
    rm -rf .install
fi

cmake --install .build --config "$CONFIG" --prefix .install

ec=$?

popd > /dev/null

exit $ec
