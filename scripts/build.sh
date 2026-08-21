#!/usr/bin/env bash
# ============================================================
# X-Phage Compiler Build Script v4.0.0
# Supports: Linux, macOS, Termux (Android)
# AeonCoreX Lab
# ============================================================
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$ROOT/build"
INSTALL_DIR="${INSTALL_PREFIX:-/usr/local}"

COLOR_RESET="\033[0m"
COLOR_GREEN="\033[1;32m"
COLOR_CYAN="\033[1;36m"
COLOR_YELLOW="\033[1;33m"
COLOR_RED="\033[1;31m"

info()    { echo -e "${COLOR_CYAN}[xphage build]${COLOR_RESET} $*"; }
success() { echo -e "${COLOR_GREEN}[ok]${COLOR_RESET} $*"; }
warn()    { echo -e "${COLOR_YELLOW}[warn]${COLOR_RESET} $*"; }
error()   { echo -e "${COLOR_RED}[error]${COLOR_RESET} $*" >&2; exit 1; }

# ── Detect platform ───────────────────────────────────────────
detect_platform() {
    case "$(uname -s)" in
        Linux*)
            if [ -n "$TERMUX_VERSION" ]; then
                echo "termux"
            else
                echo "linux"
            fi
            ;;
        Darwin*)  echo "macos" ;;
        MINGW*|MSYS*) echo "windows" ;;
        *)        echo "unknown" ;;
    esac
}

PLATFORM=$(detect_platform)
info "Platform: $PLATFORM"

# ── Check cmake ───────────────────────────────────────────────
check_deps() {
    local missing=()
    command -v cmake >/dev/null 2>&1 || missing+=("cmake")
    command -v c++   >/dev/null 2>&1 || \
    command -v g++   >/dev/null 2>&1 || \
    command -v clang++ >/dev/null 2>&1 || missing+=("c++ compiler (g++ or clang++)")
    if [ ${#missing[@]} -ne 0 ]; then
        error "Missing dependencies: ${missing[*]}"
    fi
}

# ── Parse args ────────────────────────────────────────────────
BUILD_TYPE="Release"
ENABLE_LLVM="OFF"
JOBS=$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
CLEAN=false
ARCH_FLAG=""

while [[ $# -gt 0 ]]; do
    case $1 in
        --debug)        BUILD_TYPE="Debug"; shift ;;
        --llvm)         ENABLE_LLVM="ON";   shift ;;
        --clean)        CLEAN=true;         shift ;;
        --jobs|-j)      JOBS=$2;            shift 2 ;;
        --prefix)       INSTALL_DIR=$2;     shift 2 ;;
        --x86_64)       ARCH_FLAG="-DCMAKE_OSX_ARCHITECTURES=x86_64"; shift ;;
        --arm64)        ARCH_FLAG="-DCMAKE_OSX_ARCHITECTURES=arm64";   shift ;;
        --help|-h)
            echo "Usage: build.sh [--debug] [--llvm] [--clean] [-j N] [--prefix DIR]"
            echo "       [--x86_64] [--arm64]"
            exit 0 ;;
        *) warn "Unknown arg: $1"; shift ;;
    esac
done

# ── macOS: use macos-latest compatible flag ───────────────────
# (macos-13 runner retired — see spec fix #6)
if [ "$PLATFORM" = "macos" ]; then
    export MACOSX_DEPLOYMENT_TARGET="${MACOSX_DEPLOYMENT_TARGET:-11.0}"
    info "macOS deployment target: $MACOSX_DEPLOYMENT_TARGET"
    # Strip LLVM flags that break C++ exception compilation
    if command -v llvm-config >/dev/null 2>&1; then
        RAW_CXXFLAGS=$(llvm-config --cxxflags 2>/dev/null)
        CLEAN_CXXFLAGS=$(echo "$RAW_CXXFLAGS" | \
            sed 's/-fno-exceptions//g; s/-fno-rtti//g')
        export CXXFLAGS="$CLEAN_CXXFLAGS"
    fi
fi

check_deps

# ── Clean ─────────────────────────────────────────────────────
if $CLEAN && [ -d "$BUILD_DIR" ]; then
    info "Cleaning $BUILD_DIR..."
    rm -rf "$BUILD_DIR"
fi

mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# ── Configure ─────────────────────────────────────────────────
info "Configuring (${BUILD_TYPE}, LLVM=${ENABLE_LLVM})..."
cmake "$ROOT" \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
    -DCMAKE_INSTALL_PREFIX="$INSTALL_DIR" \
    -DENABLE_LLVM="$ENABLE_LLVM" \
    $ARCH_FLAG \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

# ── Build ─────────────────────────────────────────────────────
info "Building with $JOBS jobs..."
cmake --build . --parallel "$JOBS"

success "Build complete! Binary: $BUILD_DIR/xphage"

# ── Quick smoke test ──────────────────────────────────────────
if [ -x "$BUILD_DIR/xphage" ]; then
    SMOKE=$(cat <<'SMOKE_EOF'
beam "Hello from X-Phage v4.0.0"
atom x: int = 42
beam f"x = {x}"
SMOKE_EOF
)
    echo "$SMOKE" > /tmp/smoke_test.xp0
    info "Running smoke test..."
    if "$BUILD_DIR/xphage" /tmp/smoke_test.xp0 2>&1; then
        success "Smoke test passed ✓"
    else
        warn "Smoke test failed — check generated C++"
    fi
    rm -f /tmp/smoke_test.xp0
fi

echo ""
echo -e "${COLOR_CYAN}To install:${COLOR_RESET}  sudo cmake --install $BUILD_DIR"
echo -e "${COLOR_CYAN}To use:${COLOR_RESET}      $BUILD_DIR/xphage --version"
echo -e "${COLOR_CYAN}REPL:${COLOR_RESET}        $BUILD_DIR/xphage repl"
echo ""
