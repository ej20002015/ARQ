#!/bin/bash

set -euo pipefail

# Coverage always uses its own Debug build; consume the standard wrapper mode.
shift || true

if [[ "$(uname -s)" != "Linux" ]]; then
    echo "Code coverage is only supported on Linux with GCC."
    exit 1
fi

for required_command in cmake ctest gcovr ninja; do
    if ! command -v "$required_command" > /dev/null 2>&1; then
        echo "Missing required coverage command: $required_command"
        if [[ "$required_command" == "gcovr" ]]; then
            echo "Install it with: python3 -m pip install gcovr"
        fi
        exit 1
    fi
done

SCRIPT_DIR=$(dirname "$(realpath "$0")")
PROJECT_ROOT=$(realpath "$SCRIPT_DIR/..")
BUILD_DIR="$PROJECT_ROOT/.build-coverage"
REPORT_DIR="$BUILD_DIR/coverage"

pushd "$PROJECT_ROOT" > /dev/null

cmake \
    -B "$BUILD_DIR" \
    -S "$PROJECT_ROOT" \
    -G "Ninja Multi-Config" \
    -DARQ_ENABLE_COVERAGE=ON

cmake --build "$BUILD_DIR" --config Debug --parallel

# Avoid merging data from an earlier local run into this report.
find "$BUILD_DIR" -type f -name '*.gcda' -delete

ctest \
    --test-dir "$BUILD_DIR" \
    -C Debug \
    -L unit \
    --parallel 1 \
    --output-on-failure

mkdir -p "$REPORT_DIR"

gcovr \
    --root "$PROJECT_ROOT" \
    --object-directory "$BUILD_DIR" \
    --filter "$PROJECT_ROOT/ARQLib/" \
    --filter "$PROJECT_ROOT/services/" \
    --exclude '.*/test/.*' \
    --exclude '.*/bench/.*' \
    --exclude-throw-branches \
    --exclude-unreachable-branches \
    --txt-summary \
    --html-details "$REPORT_DIR/index.html" \
    --html-title "ARQ C++ Code Coverage" \
    --cobertura-pretty \
    --cobertura "$REPORT_DIR/coverage.xml" \
    --json-summary-pretty \
    --json-summary "$REPORT_DIR/summary.json" \
    --markdown-summary "$REPORT_DIR/summary.md" \
    "$@"

echo "Coverage report: $REPORT_DIR/index.html"

popd > /dev/null
