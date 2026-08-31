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

if [[ "$LAST_CHAR" == "d" ]] && [[ "$CMD" != "d" ]]; then
    MODE="Debug"
    CMD="${CMD_RAW::-1}"
elif [[ "$LAST_CHAR" == "r" ]]; then
    MODE="Release"
    CMD="${CMD_RAW::-1}"
fi

# ----------------------------
# Aliases
# ----------------------------
SCRIPT_ARGS=()
SCRIPT_RUNNER=()
case "$CMD" in
    b|build)             SCRIPT="scripts/build.sh" ;;
    cl|clean)            SCRIPT="scripts/clean.sh" ;;
    t|test)              SCRIPT="scripts/test.sh"; SCRIPT_ARGS=(unit) ;;
    bm|bench|benchmark)  SCRIPT="scripts/test.sh"; SCRIPT_ARGS=(benchmark) ;;
    cov|coverage)        SCRIPT="scripts/coverage.sh"; SCRIPT_RUNNER=(bash) ;;
    i|install)           SCRIPT="scripts/install.sh" ;;
    c|configure)         SCRIPT="scripts/configure.sh" ;;
    g|cg|codegen)        SCRIPT="scripts/codegen.sh" ;;
    d|dockerbuild)       SCRIPT="scripts/dockerbuild.sh" ;;
    dw|dockerbuild-web)  SCRIPT="scripts/dockerbuild-web.sh" ;;
    *)
        echo "Unknown command: $CMD"
        exit 1
        ;;
esac

# ----------------------------
# Execute
# ----------------------------
exec "${SCRIPT_RUNNER[@]}" "$SCRIPT" "$MODE" "${SCRIPT_ARGS[@]}" "$@"
EC=$?

popd > /dev/null
exit $EC
