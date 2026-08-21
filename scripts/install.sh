#!/usr/bin/env bash
# ============================================================
# X-Phage Install Script v4.0.0
# Compiles from source and installs
# AeonCoreX Lab
# ============================================================
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(dirname "$SCRIPT_DIR")"
PREFIX="${INSTALL_PREFIX:-/usr/local}"

echo "[install] X-Phage v4.0.0 → $PREFIX"

bash "$SCRIPT_DIR/build.sh" "$@"

cmake --install "$ROOT/build" --prefix "$PREFIX"

echo "[install] Done. Run: xphage --version"
