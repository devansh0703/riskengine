#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ROOT_DIR="$(dirname "$SCRIPT_DIR")"

echo "=== Setting up Python environment ==="

if [ ! -d "$ROOT_DIR/.venv" ]; then
    python3 -m venv "$ROOT_DIR/.venv"
fi

source "$ROOT_DIR/.venv/bin/activate"

pip install --upgrade pip
pip install -e "$ROOT_DIR[dev]"

echo "=== Python environment ready ==="
echo "Activate with: source $ROOT_DIR/.venv/bin/activate"
