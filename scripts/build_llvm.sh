#!/usr/bin/env bash
# ============================================================
# X-Phage Phase 4 — LLVM Native Backend Build Script v4.0.0
# Detects LLVM, strips forbidden flags, builds with native backend
# AeonCoreX Lab
# ============================================================
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$ROOT/build_llvm"
INSTALL_PREFIX="${INSTALL_PREFIX:-/usr/local}"

COLOR_RESET="\033[0m"
COLOR_GREEN="\033[1;32m"
COLOR_CYAN="\033[1;36m"
COLOR_YELLOW="\033[1;33m"
COLOR_RED="\033[1;31m"

info()    { echo -e "${COLOR_CYAN}[xphage/llvm]${COLOR_RESET} $*"; }
success() { echo -e "${COLOR_GREEN}[ok]${COLOR_RESET} $*"; }
warn()    { echo -e "${COLOR_YELLOW}[warn]${COLOR_RESET} $*"; }
die()     { echo -e "${COLOR_RED}[error]${COLOR_RESET} $*" >&2; exit 1; }

# ── Detect LLVM ───────────────────────────────────────────────
detect_llvm() {
    # Try common llvm-config names in order of preference
    for name in llvm-config-21 llvm-config-20 llvm-config-19 \
                llvm-config-18 llvm-config-17 llvm-config-16 llvm-config; do
        if command -v "$name" >/dev/null 2>&1; then
            echo "$name"; return 0
        fi
    done
    return 1
}

LLVM_CONFIG=$(detect_llvm) || die "LLVM not found.
Install LLVM 16-21:
  Ubuntu/Debian : sudo apt install llvm-17-dev
  macOS         : brew install llvm
  Arch          : sudo pacman -S llvm
  Fedora        : sudo dnf install llvm-devel"

LLVM_VER=$("$LLVM_CONFIG" --version)
LLVM_PREFIX=$("$LLVM_CONFIG" --prefix)
LLVM_CMAKE_DIR=$(find "$LLVM_PREFIX" -name "LLVMConfig.cmake" 2>/dev/null | head -1)
LLVM_CMAKE_DIR=$(dirname "$LLVM_CMAKE_DIR" 2>/dev/null)

info "Found LLVM $LLVM_VER at $LLVM_PREFIX"

if [ -z "$LLVM_CMAKE_DIR" ]; then
    # Try standard locations
    for d in "$LLVM_PREFIX/lib/cmake/llvm" \
              "/usr/lib/llvm-17/lib/cmake/llvm" \
              "/usr/local/lib/cmake/llvm" \
              "$(brew --prefix llvm 2>/dev/null)/lib/cmake/llvm"; do
        [ -f "$d/LLVMConfig.cmake" ] && { LLVM_CMAKE_DIR="$d"; break; }
    done
fi

[ -n "$LLVM_CMAKE_DIR" ] || die "Cannot find LLVMConfig.cmake. Set LLVM_DIR manually."
info "LLVM CMake dir: $LLVM_CMAKE_DIR"

# ── Strip LLVM flags that break C++ exceptions/RTTI ──────────
# (spec fix #6 — llvm-config --cxxflags may include -fno-exceptions -fno-rtti)
RAW_CXXFLAGS=$("$LLVM_CONFIG" --cxxflags 2>/dev/null || true)
CLEAN_CXXFLAGS=$(echo "$RAW_CXXFLAGS" | \
    tr ' ' '\n' | \
    grep -v -e '-fno-exceptions' -e '-fno-rtti' -e '-fno-unwind-tables' | \
    tr '\n' ' ')
export CXXFLAGS="$CLEAN_CXXFLAGS"
info "CXX flags (stripped): $(echo "$CLEAN_CXXFLAGS" | cut -c1-80)"

# ── macOS cross-compile fix ───────────────────────────────────
ARCH_FLAG=""
if [ "$(uname)" = "Darwin" ]; then
    export MACOSX_DEPLOYMENT_TARGET="${MACOSX_DEPLOYMENT_TARGET:-11.0}"
    # Detect host arch; macos-latest supports both
    ARCH=$(uname -m)
    info "macOS arch: $ARCH  deployment: $MACOSX_DEPLOYMENT_TARGET"
    ARCH_FLAG="-DCMAKE_OSX_ARCHITECTURES=$ARCH"
fi

# ── Parse CLI args ────────────────────────────────────────────
BUILD_TYPE="Release"
JOBS=$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
CLEAN=false

while [[ $# -gt 0 ]]; do
    case $1 in
        --debug)   BUILD_TYPE="Debug"; shift ;;
        --clean)   CLEAN=true;         shift ;;
        --jobs|-j) JOBS=$2;            shift 2 ;;
        --prefix)  INSTALL_PREFIX=$2;  shift 2 ;;
        *) warn "Unknown arg: $1"; shift ;;
    esac
done

$CLEAN && [ -d "$BUILD_DIR" ] && { info "Cleaning..."; rm -rf "$BUILD_DIR"; }
mkdir -p "$BUILD_DIR"

# ── Configure ─────────────────────────────────────────────────
info "Configuring with LLVM ($BUILD_TYPE)..."
cmake "$ROOT" \
    -B "$BUILD_DIR" \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
    -DCMAKE_INSTALL_PREFIX="$INSTALL_PREFIX" \
    -DENABLE_LLVM=ON \
    -DLLVM_DIR="$LLVM_CMAKE_DIR" \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
    $ARCH_FLAG

# ── Build ─────────────────────────────────────────────────────
info "Building ($JOBS jobs)..."
cmake --build "$BUILD_DIR" --parallel "$JOBS"

success "LLVM build complete!"

# ── Verify LLVM backend active ────────────────────────────────
BIN="$BUILD_DIR/xphage"
if [ -x "$BIN" ]; then
    info "Checking LLVM backend..."
    VER_OUT=$("$BIN" --version 2>&1)
    if echo "$VER_OUT" | grep -q "LLVM"; then
        success "LLVM backend active: $(echo "$VER_OUT" | grep LLVM)"
    else
        warn "LLVM backend not detected in version string"
    fi

    # Smoke test Phase 4
    SMOKE_FILE="/tmp/xp_llvm_smoke.xp0"
    cat > "$SMOKE_FILE" << 'SMOKE'
beam "Hello from LLVM backend!"
atom x: int = 21
atom y: int = 21
beam f"The answer is: {x + y}"
SMOKE

    info "Running Phase 4 smoke test..."
    if "$BIN" --backend=llvm "$SMOKE_FILE" 2>&1; then
        success "Phase 4 LLVM smoke test passed ✓"
    else
        warn "Phase 4 smoke test failed — check output above"
    fi
    rm -f "$SMOKE_FILE"
fi

echo ""
echo -e "${COLOR_CYAN}Usage:${COLOR_RESET}"
echo "  $BIN --backend=llvm file.xp0       Compile with LLVM"
echo "  $BIN --emit=llvm    file.xp0       Emit LLVM IR (.ll)"
echo "  $BIN --emit=obj     file.xp0       Emit native .o"
echo "  $BIN --version                     Check LLVM version"
echo ""
