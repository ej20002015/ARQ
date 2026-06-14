#!/bin/bash

set -e

SCRIPT_DIR="$(dirname "$(realpath "$0")")"
ROOT="$SCRIPT_DIR"

pushd "$ROOT" > /dev/null

CMD_RAW=$1
shift || true

if [[ -z "$CMD_RAW" ]]; then
    echo "Usage: bld <command>[d|r] [args...]"
    exit 1
fi

# ----------------------------
# Parse mode suffix
# ----------------------------
MODE="Release"
CMD="$CMD_RAW"

LAST_CHAR="${CMD_RAW: -1}"

if [[ "$LAST_CHAR" == "d" ]]; then
    MODE="Debug"
    CMD="${CMD_RAW::-1}"
elif [[ "$LAST_CHAR" == "r" ]]; then
    MODE="Release"
    CMD="${CMD_RAW::-1}"
fi

# ----------------------------
# Aliases
# ----------------------------
case "$CMD" in
    b|build)        SCRIPT="scripts/build.sh" ;;
    cl|clean)       SCRIPT="scripts/clean.sh" ;;
    t|test)         SCRIPT="scripts/test.sh" ;;
    i|install)      SCRIPT="scripts/install.sh" ;;
    c|configure)    SCRIPT="scripts/configure.sh" ;;
    g|cg|codegen)   SCRIPT="scripts/codegen.sh" ;;
    d|dockerbuild)  SCRIPT="scripts/dockerbuild.sh" ;;
    *)
        echo "Unknown command: $CMD"
        exit 1
        ;;
esac

# ----------------------------
# Execute
# ----------------------------
exec "$SCRIPT" "$MODE" "$@"
EC=$?

popd > /dev/null
exit $EC
