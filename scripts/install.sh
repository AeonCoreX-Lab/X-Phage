#!/usr/bin/env bash
# ================================================================
# X-Phage Installer v3.5.0
# Compile-from-source installer — no pre-built binaries.
# Like Cargo: detects platform, installs deps, builds, installs.
# Connects to XPM (AeonCoreX-Lab/XPM) after install.
# ================================================================
set -euo pipefail

# ── Colours ──────────────────────────────────────────────────
RED='\033[1;31m'; GREEN='\033[1;32m'; YELLOW='\033[1;33m'
CYAN='\033[1;36m'; PURPLE='\033[1;35m'; NC='\033[0m'
BOLD='\033[1m'

info()    { echo -e "${CYAN}[xphage-install]${NC} $*"; }
success() { echo -e "${GREEN}[xphage-install]${NC} ✔ $*"; }
warn()    { echo -e "${YELLOW}[xphage-install]${NC} ⚠ $*"; }
die()     { echo -e "${RED}[xphage-install]${NC} ✘ $*" >&2; exit 1; }

# ── Config ────────────────────────────────────────────────────
XPHAGE_REPO="https://github.com/AeonCoreX-Lab/X-Phage.git"
XPM_REPO="https://github.com/AeonCoreX-Lab/XPM.git"
INSTALL_DIR="${XPHAGE_HOME:-$HOME/.xphage}"
BIN_DIR="$INSTALL_DIR/bin"
SRC_DIR="$INSTALL_DIR/src"
XPM_SRC_DIR="$INSTALL_DIR/xpm-src"
XPHAGE_VERSION="${XPHAGE_VERSION:-main}"

# ── Banner ────────────────────────────────────────────────────
echo -e "${PURPLE}"
echo "  ██╗  ██╗      ██████╗ ██╗  ██╗ █████╗  ██████╗ ███████╗"
echo "  ╚██╗██╔╝     ██╔══██╗██║  ██║██╔══██╗██╔════╝ ██╔════╝"
echo "   ╚███╔╝      ██████╔╝███████║███████║██║  ███╗█████╗  "
echo "   ██╔██╗      ██╔═══╝ ██╔══██║██╔══██║██║   ██║██╔══╝  "
echo "  ██╔╝ ██╗     ██║     ██║  ██║██║  ██║╚██████╔╝███████╗"
echo "  ╚═╝  ╚═╝     ╚═╝     ╚═╝  ╚═╝╚═╝  ╚═╝ ╚═════╝ ╚══════╝"
echo -e "${NC}"
echo -e "${BOLD}  X-Phage Language + XPM Installer v3.5.0${NC}"
echo -e "  Compiles from source — no pre-built binaries.\n"

# ── Detect OS + Architecture ──────────────────────────────────
OS="$(uname -s 2>/dev/null || echo Windows)"
ARCH="$(uname -m 2>/dev/null || echo x86_64)"

case "$OS" in
    Linux*)   PLATFORM="linux"  ;;
    Darwin*)  PLATFORM="macos"  ;;
    MINGW*|MSYS*|CYGWIN*|Windows*) PLATFORM="windows" ;;
    *)        die "Unsupported OS: $OS" ;;
esac

case "$ARCH" in
    x86_64|amd64) ARCH_TAG="x64"   ;;
    aarch64|arm64) ARCH_TAG="arm64" ;;
    armv7*)        ARCH_TAG="armv7" ;;
    *)             warn "Unknown arch: $ARCH, defaulting to x64"; ARCH_TAG="x64" ;;
esac

info "Detected: ${PLATFORM}/${ARCH_TAG}"

# ── Check requirements ────────────────────────────────────────
check_cmd() { command -v "$1" &>/dev/null || die "$1 is required but not found. Please install it first."; }
check_cmd git
check_cmd make || true  # not always needed

# ── Install build dependencies ────────────────────────────────
install_deps_linux() {
    info "Installing build dependencies (Linux)..."
    if command -v apt-get &>/dev/null; then
        sudo apt-get update -qq
        # Detect latest available LLVM
        LLVM_VER=$(apt-cache search '^llvm-[0-9]+$' \
            | grep -oP 'llvm-\K[0-9]+' | sort -n | tail -1)
        [ -z "$LLVM_VER" ] && LLVM_VER=18
        info "Using LLVM version: $LLVM_VER"
        sudo apt-get install -y --no-install-recommends \
            llvm-${LLVM_VER} llvm-${LLVM_VER}-dev \
            clang-${LLVM_VER} lld-${LLVM_VER} \
            libstdc++-12-dev git curl ca-certificates
        sudo ln -sf /usr/bin/clang++-${LLVM_VER} /usr/bin/clang++ 2>/dev/null || true
        sudo ln -sf /usr/bin/llvm-config-${LLVM_VER} /usr/bin/llvm-config 2>/dev/null || true
    elif command -v dnf &>/dev/null; then
        sudo dnf install -y clang llvm llvm-devel git curl
    elif command -v pacman &>/dev/null; then
        sudo pacman -Sy --noconfirm clang llvm git curl
    elif command -v apk &>/dev/null; then
        # Alpine / Termux-like
        sudo apk add --no-cache clang llvm-dev git curl bash
    else
        warn "Unknown package manager. Please install clang++ and llvm-dev manually."
    fi
}

install_deps_macos() {
    info "Installing build dependencies (macOS)..."
    if ! command -v brew &>/dev/null; then
        info "Installing Homebrew..."
        /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
    fi
    brew update
    brew install llvm git
    # Add Homebrew LLVM to PATH for this session
    BREW_LLVM_PREFIX="$(brew --prefix llvm 2>/dev/null || echo /opt/homebrew/opt/llvm)"
    export PATH="$BREW_LLVM_PREFIX/bin:$PATH"
}

install_deps_windows() {
    info "Installing build dependencies (Windows)..."
    if ! command -v clang++ &>/dev/null; then
        if command -v pacman &>/dev/null; then
            pacman -Sy --noconfirm mingw-w64-x86_64-llvm mingw-w64-x86_64-clang
        elif command -v winget &>/dev/null; then
            winget install -e --id LLVM.LLVM
            winget install -e --id Git.Git
        else
            die "Please install LLVM and Git manually from https://releases.llvm.org and https://git-scm.com"
        fi
    fi
}

case "$PLATFORM" in
    linux)   install_deps_linux   ;;
    macos)   install_deps_macos   ;;
    windows) install_deps_windows ;;
esac

# ── Verify compiler ───────────────────────────────────────────
CXX="${CXX:-}"
for try_cxx in clang++ g++ c++; do
    if command -v "$try_cxx" &>/dev/null; then
        CXX="$try_cxx"; break
    fi
done
[ -z "$CXX" ] && die "No C++ compiler found after dependency install."
CXX_VER=$("$CXX" --version 2>&1 | head -1)
success "Compiler: $CXX — $CXX_VER"

# ── Clone / update X-Phage source ────────────────────────────
mkdir -p "$INSTALL_DIR" "$BIN_DIR"

if [ -d "$SRC_DIR/.git" ]; then
    info "Updating X-Phage source..."
    git -C "$SRC_DIR" fetch --quiet origin
    git -C "$SRC_DIR" checkout --quiet "$XPHAGE_VERSION" 2>/dev/null || \
        git -C "$SRC_DIR" reset --hard "origin/$XPHAGE_VERSION"
else
    info "Cloning X-Phage source..."
    rm -rf "$SRC_DIR"
    git clone --depth=1 --branch "$XPHAGE_VERSION" "$XPHAGE_REPO" "$SRC_DIR" \
        2>/dev/null || git clone --depth=1 "$XPHAGE_REPO" "$SRC_DIR"
fi
success "Source ready at $SRC_DIR"

# ── Determine build target ────────────────────────────────────
case "${PLATFORM}-${ARCH_TAG}" in
    linux-x64)    BUILD_TARGET="linux"         ;;
    linux-arm64)  BUILD_TARGET="linux-arm64"   ;;
    macos-arm64)  BUILD_TARGET="macos-arm64"   ;;
    macos-x64)    BUILD_TARGET="macos-x64"     ;;
    windows-x64)  BUILD_TARGET="windows"       ;;
    windows-arm64) BUILD_TARGET="windows-arm64" ;;
    *)            BUILD_TARGET="linux"          ;;
esac

# ── Compile X-Phage ──────────────────────────────────────────
info "Compiling X-Phage for ${BUILD_TARGET}..."

cd "$SRC_DIR"
TARGET="$BUILD_TARGET" ARTIFACT_NAME="xphage" bash scripts/build.sh

# Find produced binary
BUILT_BIN=$(find "$SRC_DIR/bin/$BUILD_TARGET" -type f -name "xphage*" \
             ! -name "*.sha256" 2>/dev/null | head -1)
[ -z "$BUILT_BIN" ] && die "Build failed — binary not produced. Check errors above."
success "Build complete: $BUILT_BIN"

# ── Install X-Phage binary ────────────────────────────────────
XPHAGE_BIN="$BIN_DIR/xphage"
[[ "$PLATFORM" == "windows" ]] && XPHAGE_BIN="$BIN_DIR/xphage.exe"

cp "$BUILT_BIN" "$XPHAGE_BIN"
chmod +x "$XPHAGE_BIN"
success "Installed xphage → $XPHAGE_BIN"

# ── Clone / update XPM source ─────────────────────────────────
info "Setting up XPM (X-Phage Package Manager)..."
XPM_VERSION="${XPM_VERSION:-main}"

if [ -d "$XPM_SRC_DIR/.git" ]; then
    info "Updating XPM source..."
    git -C "$XPM_SRC_DIR" fetch --quiet origin
    git -C "$XPM_SRC_DIR" reset --hard "origin/$XPM_VERSION" --quiet 2>/dev/null || true
else
    info "Cloning XPM source..."
    rm -rf "$XPM_SRC_DIR"
    git clone --depth=1 --branch "$XPM_VERSION" "$XPM_REPO" "$XPM_SRC_DIR" \
        2>/dev/null || git clone --depth=1 "$XPM_REPO" "$XPM_SRC_DIR"
fi

# ── Compile XPM ──────────────────────────────────────────────
info "Compiling XPM..."
cd "$XPM_SRC_DIR"

# XPM uses the same build pattern
if [ -f "scripts/build.sh" ]; then
    TARGET="$BUILD_TARGET" ARTIFACT_NAME="xpm" bash scripts/build.sh
    XPM_BUILT=$(find "$XPM_SRC_DIR/bin/$BUILD_TARGET" -type f -name "xpm*" \
                 ! -name "*.sha256" 2>/dev/null | head -1)
else
    # Fallback: compile XPM directly
    XPM_SOURCES=$(find "$XPM_SRC_DIR/src" -name "*.cpp" 2>/dev/null | tr '\n' ' ')
    XPM_BIN_OUT="$XPM_SRC_DIR/xpm_built"
    [[ "$PLATFORM" == "windows" ]] && XPM_BIN_OUT="${XPM_BIN_OUT}.exe"
    if [ -n "$XPM_SOURCES" ]; then
        "$CXX" $XPM_SOURCES \
            -I"$XPM_SRC_DIR/include" \
            -std=c++17 -O2 -pthread \
            -o "$XPM_BIN_OUT" 2>/dev/null || true
        XPM_BUILT="$XPM_BIN_OUT"
    fi
fi

XPM_BIN="$BIN_DIR/xpm"
[[ "$PLATFORM" == "windows" ]] && XPM_BIN="$BIN_DIR/xpm.exe"

if [ -n "${XPM_BUILT:-}" ] && [ -f "${XPM_BUILT}" ]; then
    cp "$XPM_BUILT" "$XPM_BIN"
    chmod +x "$XPM_BIN"
    success "Installed xpm → $XPM_BIN"
else
    warn "XPM build failed or binary not found. XPM will not be available yet."
fi

# ── Write XPM config pointing to registry ─────────────────────
XPM_CONFIG_DIR="$HOME/.xpm"
mkdir -p "$XPM_CONFIG_DIR"
cat > "$XPM_CONFIG_DIR/config.toml" << XPMCFG
# XPM Configuration — auto-generated by X-Phage installer
[registry]
index_url = "https://raw.githubusercontent.com/AeonCoreX-Lab/xpm-registry/main/index"
api_url   = "https://api.github.com/repos/AeonCoreX-Lab/xpm-registry"

[paths]
xphage_bin = "$XPHAGE_BIN"
xpm_home   = "$INSTALL_DIR"
cache_dir  = "$XPM_CONFIG_DIR/cache"
XPMCFG
success "XPM config written → $XPM_CONFIG_DIR/config.toml"

# ── PATH setup ────────────────────────────────────────────────
setup_path() {
    local SHELL_RC=""
    case "${SHELL:-}" in
        */zsh)  SHELL_RC="$HOME/.zshrc"   ;;
        */fish) SHELL_RC="$HOME/.config/fish/config.fish" ;;
        *)      SHELL_RC="$HOME/.bashrc"  ;;
    esac

    local PATH_LINE=""
    if [[ "${SHELL:-}" == */fish ]]; then
        PATH_LINE="fish_add_path $BIN_DIR"
    else
        PATH_LINE="export PATH=\"$BIN_DIR:\$PATH\""
    fi

    local MARKER="# X-Phage tools"
    if [ -f "$SHELL_RC" ] && grep -q "$MARKER" "$SHELL_RC" 2>/dev/null; then
        info "PATH already configured in $SHELL_RC"
    else
        echo "" >> "$SHELL_RC"
        echo "$MARKER" >> "$SHELL_RC"
        echo "$PATH_LINE" >> "$SHELL_RC"
        success "Added $BIN_DIR to PATH in $SHELL_RC"
    fi
}

export PATH="$BIN_DIR:$PATH"

if [[ "$PLATFORM" != "windows" ]]; then
    setup_path
else
    warn "Add $BIN_DIR to your Windows PATH manually."
fi

# ── Verify installation ───────────────────────────────────────
echo ""
info "Verifying installation..."

if "$XPHAGE_BIN" --version &>/dev/null 2>&1; then
    XPHAGE_VER=$("$XPHAGE_BIN" --version 2>&1 | head -1)
    success "xphage: $XPHAGE_VER"
else
    warn "xphage --version check failed. The binary may still work."
fi

if [ -f "$XPM_BIN" ]; then
    if "$XPM_BIN" --version &>/dev/null 2>&1; then
        XPM_VER=$("$XPM_BIN" --version 2>&1 | head -1)
        success "xpm: $XPM_VER"
    else
        warn "xpm --version check failed. The binary may still work."
    fi
fi

# ── Done ─────────────────────────────────────────────────────
echo ""
echo -e "${GREEN}${BOLD}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo -e "${GREEN}${BOLD}  ✔ X-Phage v3.5.0 installed successfully!${NC}"
echo -e "${GREEN}${BOLD}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo ""
echo -e "  ${BOLD}Installed to:${NC} $BIN_DIR"
echo -e "  ${BOLD}Source at:${NC}    $SRC_DIR"
echo ""
echo -e "  ${BOLD}Quick start:${NC}"
echo -e "    ${CYAN}xphage --version${NC}"
echo -e "    ${CYAN}xpm init my-project${NC}"
echo -e "    ${CYAN}xpm add net-http${NC}"
echo -e "    ${CYAN}xphage build${NC}"
echo ""
echo -e "  ${BOLD}Docs:${NC} https://github.com/AeonCoreX-Lab/X-Phage"
echo ""

if [[ "$PLATFORM" != "windows" ]]; then
    echo -e "  ${YELLOW}Reload your shell or run:${NC}"
    echo -e "    ${CYAN}source ~/.bashrc${NC}   (or ~/.zshrc)"
    echo ""
fi
