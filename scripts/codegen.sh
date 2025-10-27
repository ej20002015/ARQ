#!/bin/bash
# Exit immediately if a command exits with a non-zero status
set -e

# --- Configuration ---
SCRIPT_DIR=$(dirname "$(realpath "$0")")
PROJECT_ROOT="$SCRIPT_DIR/.."
VENV_DIR=".venv"
REQS_FILE="codegen/script/requirements.txt"

# --- Main execution ---
# Change to the project root directory
pushd "$PROJECT_ROOT" > /dev/null

# Check if the virtual environment directory does NOT exist
if [ ! -d "$VENV_DIR" ]; then
  echo "--- Creating virtual environment at $VENV_DIR... ---"
  python3 -m venv "$VENV_DIR"
  
  echo "--- Activating environment to install dependencies... ---"
  source "$VENV_DIR/bin/activate"
  
  echo "--- Installing dependencies from $REQS_FILE... ---"
  python3 -m pip install -r "$REQS_FILE"
else
  # If venv exists, just activate it
  echo "--- Activating existing virtual environment... ---"
  source "$VENV_DIR/bin/activate"
fi

echo "--- Running codegen script... ---"
python3 codegen/script/gen.py \
  --definitions-dir codegen/definitions \
  --template-dir codegen/templates \
  --output-dir .

echo "--- Deactivating virtual environment. ---"
deactivate

# Return to the original directory
popd > /dev/null

echo "--- Codegen complete. ---"