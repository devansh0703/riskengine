#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ROOT_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$ROOT_DIR/build"

if [ ! -d "$BUILD_DIR" ]; then
    echo "Build directory not found. Running build first..."
    bash "$SCRIPT_DIR/build.sh"
fi

cd "$BUILD_DIR"

echo "=== Running C++ Tests ==="
ctest --output-on-failure -j$(nproc)

echo ""
echo "=== Running Greeks Test ==="
./test_greeks || true

echo ""
echo "=== All tests passed ==="
