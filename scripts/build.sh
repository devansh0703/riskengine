#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ROOT_DIR="$(dirname "$SCRIPT_DIR")"

echo "=== Building RiskEngine ==="
mkdir -p "$ROOT_DIR/build"
cd "$ROOT_DIR/build"

cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_TESTS=ON \
    -DBUILD_TOOLS=ON \
    -DBUILD_PYTHON_BINDINGS=OFF

make -j"$(nproc)"

echo "=== Build complete ==="
echo "Binaries:"
ls -la riskengine risk_cli replay_tool export_tool 2>/dev/null || true
