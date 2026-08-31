#!/bin/bash

set -euo pipefail

# Coverage always uses its own Debug build; consume the standard wrapper mode.
shift || true

if [[ "$(uname -s)" != "Linux" ]]; then
    echo "Code coverage is only supported on Linux with GCC."
    exit 1
fi

for required_command in cmake ctest ninja python3; do
    if ! command -v "$required_command" > /dev/null 2>&1; then
        echo "Missing required coverage command: $required_command"
        exit 1
    fi
done

SCRIPT_DIR=$(dirname "$(realpath "$0")")
PROJECT_ROOT=$(realpath "$SCRIPT_DIR/..")
BUILD_DIR="$PROJECT_ROOT/.build-coverage"
REPORT_DIR="$BUILD_DIR/coverage"
GCOVR_VERSION="8.6"
GCOVR_VENV="$BUILD_DIR/.gcovr-$GCOVR_VERSION-venv"
GCOVR="$GCOVR_VENV/bin/gcovr"
GCOVR_GCOV_OPTIONS=()

pushd "$PROJECT_ROOT" > /dev/null

if [[ ! -x "$GCOVR" ]]; then
    echo "Creating project-local gcovr $GCOVR_VERSION environment..."
    if ! python3 -m venv "$GCOVR_VENV"; then
        echo "Unable to create a Python virtual environment."
        echo "On Ubuntu, install the venv support package with: sudo apt-get install python3-venv"
        exit 1
    fi

    "$GCOVR_VENV/bin/python" -m pip install \
        --disable-pip-version-check \
        "gcovr==$GCOVR_VERSION"
fi

cmake \
    -B "$BUILD_DIR" \
    -S "$PROJECT_ROOT" \
    -G "Ninja Multi-Config" \
    -DARQ_ENABLE_COVERAGE=ON

HAS_EXPLICIT_GCOV=false
for argument in "$@"; do
    if [[ "$argument" == "--gcov-executable" || "$argument" == --gcov-executable=* ]]; then
        HAS_EXPLICIT_GCOV=true
        break
    fi
done

if [[ -z "${GCOV:-}" && "$HAS_EXPLICIT_GCOV" == false ]]; then
    TOOLCHAIN_FILE="$BUILD_DIR/arq-coverage-toolchain.txt"
    if [[ ! -f "$TOOLCHAIN_FILE" ]]; then
        echo "CMake did not write the coverage toolchain information."
        exit 1
    fi

    {
        IFS= read -r CXX_COMPILER
        IFS= read -r GCC_VERSION
    } < "$TOOLCHAIN_FILE"

    if [[ -z "$CXX_COMPILER" || ! -x "$CXX_COMPILER" ]]; then
        echo "Unable to determine the GCC executable used by the coverage build."
        exit 1
    fi

    if [[ -z "$GCC_VERSION" ]]; then
        echo "Unable to determine the GCC version used by the coverage build."
        exit 1
    fi

    GCC_MAJOR=${GCC_VERSION%%.*}
    GCOV_EXECUTABLE=$(command -v "gcov-$GCC_MAJOR" || true)

    if [[ -z "$GCOV_EXECUTABLE" ]]; then
        DEFAULT_GCOV=$(command -v gcov || true)
        if [[ -n "$DEFAULT_GCOV" ]] && [[ "$("$DEFAULT_GCOV" --version)" == *"$GCC_VERSION"* ]]; then
            GCOV_EXECUTABLE="$DEFAULT_GCOV"
        else
            echo "Coverage was compiled with GCC $GCC_VERSION, but a matching gcov was not found."
            echo "Install gcov-$GCC_MAJOR or pass: --gcov-executable /path/to/matching/gcov"
            exit 1
        fi
    fi

    if [[ "$("$GCOV_EXECUTABLE" --version)" != *"$GCC_VERSION"* ]]; then
        echo "$GCOV_EXECUTABLE does not match the configured GCC version $GCC_VERSION."
        exit 1
    fi

    echo "Using $GCOV_EXECUTABLE for coverage compiled with $CXX_COMPILER ($GCC_VERSION)."
    GCOVR_GCOV_OPTIONS=(--gcov-executable "$GCOV_EXECUTABLE")
fi

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

"$GCOVR" \
    "${GCOVR_GCOV_OPTIONS[@]}" \
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
