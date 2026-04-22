#!/bin/bash
# ============================================================
# 🧬 X-Phage Universal Installer v3.6.0
# Compiles from source for current platform & architecture.
# No pre-built binaries downloaded.
# Requires: git, cmake (≥3.20), a C++17 compiler
# ============================================================
set -e

GREEN='\033[1;32m'
CYAN='\033[1;36m'
YELLOW='\033[1;33m'
RED='\033[1;31m'
PURPLE='\033[1;35m'
NC='\033[0m'

REPO="AeonCoreX-Lab/X-Phage"
REPO_URL="https://github.com/${REPO}.git"

echo -e "${PURPLE}"
echo "  _  _  ____  __  __ "
echo " ( \/ )(  _ \(  \/  )"
echo "  )  (  )___/ )    ( "
echo " (_\/\_)(__)  (_/\/_) ${GREEN}v3.6.0${PURPLE}"
echo ""
echo -e "${CYAN}🧬 X-Phage — Build from Source Installer${NC}"
echo -e "${YELLOW}   Repository: https://github.com/${REPO}${NC}"
echo ""

# ============================================================
# Platform & architecture detection
# ============================================================
OS="$(uname -s 2>/dev/null || echo 'Unknown')"
ARCH="$(uname -m 2>/dev/null || echo 'x86_64')"
INSTALL_DIR="/usr/local/bin"
SUDO="sudo"
IS_TERMUX=false

if [ -d "/data/data/com.termux/files/usr" ]; then
    IS_TERMUX=true
    INSTALL_DIR="/data/data/com.termux/files/usr/bin"
    SUDO=""
    echo -e "📱 Platform: ${GREEN}Termux (Android ${ARCH})${NC}"
elif [[ "$OS" == "Darwin" ]]; then
    echo -e "🍎 Platform: ${GREEN}macOS ${ARCH}${NC}"
elif [[ "$OS" == "Linux" ]]; then
    DISTRO=""
    [ -f /etc/os-release ] && DISTRO=$(grep '^PRETTY_NAME=' /etc/os-release \
        | cut -d= -f2 | tr -d '"')
    echo -e "🐧 Platform: ${GREEN}Linux ${ARCH}${NC}${DISTRO:+ (${DISTRO})}"
elif [[ "$OS" == MINGW* || "$OS" == MSYS* || "$OS" == CYGWIN* ]]; then
    INSTALL_DIR="${USERPROFILE}/AppData/Local/xphage/bin"
    SUDO=""
    echo -e "🪟 Platform: ${GREEN}Windows (Git Bash) ${ARCH}${NC}"
else
    echo -e "${RED}✖ Unsupported OS: ${OS}${NC}"; exit 1
fi

# ============================================================
# Dependency check
# ============================================================
echo ""
echo -e "${CYAN}[1/5] Checking dependencies...${NC}"

MISSING=()
for dep in git cmake; do
    if ! command -v "$dep" &>/dev/null; then MISSING+=("$dep"); fi
done

# Need a C++17 compiler
CXX_FOUND=""
for cxx in clang++ g++ c++; do
    command -v "$cxx" &>/dev/null && CXX_FOUND="$cxx" && break
done
[ -z "$CXX_FOUND" ] && MISSING+=("clang++ or g++")

if [ ${#MISSING[@]} -gt 0 ]; then
    echo -e "${RED}✖ Missing dependencies: ${MISSING[*]}${NC}"
    if [[ "$OS" == "Linux" ]] && command -v apt-get &>/dev/null; then
        echo -e "${YELLOW}  Try: sudo apt-get install -y git cmake clang${NC}"
    elif [[ "$OS" == "Darwin" ]]; then
        echo -e "${YELLOW}  Try: brew install git cmake llvm${NC}"
    elif $IS_TERMUX; then
        echo -e "${YELLOW}  Try: pkg install git cmake clang${NC}"
    fi
    exit 1
fi

CMAKE_VER=$(cmake --version | head -1 | grep -oP '\d+\.\d+' | head -1)
echo -e "  ✔ cmake ${CMAKE_VER}, ${CXX_FOUND}, git found"

# ============================================================
# Clone repository
# ============================================================
echo ""
echo -e "${CYAN}[2/5] Cloning X-Phage source...${NC}"

SAFE_TMP="${TMPDIR:-/tmp}"
CLONE_DIR="${SAFE_TMP}/xphage-src-$$"
trap "rm -rf '$CLONE_DIR'" EXIT

git clone --depth=1 "$REPO_URL" "$CLONE_DIR"
echo -e "  ✔ Cloned to ${CLONE_DIR}"

# ============================================================
# Configure
# ============================================================
echo ""
echo -e "${CYAN}[3/5] Configuring (CMake)...${NC}"

BUILD_DIR="${CLONE_DIR}/build"
JOBS=$(nproc 2>/dev/null || sysctl -n hw.logicalcpu 2>/dev/null || echo 4)

# Auto-detect LLVM
LLVM_FLAG="-DXPHAGE_ENABLE_LLVM=ON"
if ! (cmake --find-package -DNAME=LLVM -DCOMPILER_ID=Clang \
    -DLANGUAGE=CXX -DMODE=EXIST 2>/dev/null | grep -q "found") && \
    ! command -v llvm-config &>/dev/null; then
    echo -e "  ${YELLOW}⚠ LLVM not found — building with Transpiler engine${NC}"
    LLVM_FLAG="-DXPHAGE_ENABLE_LLVM=OFF"
fi

cmake -S "$CLONE_DIR" -B "$BUILD_DIR" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
    $LLVM_FLAG \
    -DXPHAGE_ENABLE_TESTS=OFF \
    -DXPHAGE_ENABLE_TOOLS=ON

# ============================================================
# Build
# ============================================================
echo ""
echo -e "${CYAN}[4/5] Building (${JOBS} parallel jobs)...${NC}"
cmake --build "$BUILD_DIR" --parallel "$JOBS" --config Release

# ============================================================
# Install
# ============================================================
echo ""
echo -e "${CYAN}[5/5] Installing to ${INSTALL_DIR}...${NC}"

mkdir -p "$INSTALL_DIR" 2>/dev/null || \
    { [ -n "$SUDO" ] && $SUDO mkdir -p "$INSTALL_DIR"; }

for bin in xphage xphage-fmt xphage-doc xphage-lsp xphage-test; do
    BIN_PATH="${BUILD_DIR}/${bin}"
    if [ -f "$BIN_PATH" ]; then
        if [ -n "$SUDO" ]; then
            $SUDO install -m 755 "$BIN_PATH" "${INSTALL_DIR}/${bin}"
        else
            install -m 755 "$BIN_PATH" "${INSTALL_DIR}/${bin}"
        fi
        echo -e "  ✔ Installed: ${INSTALL_DIR}/${bin}"
    fi
done

# Install stdlib
STDLIB_DEST="/usr/local/share/xphage"
$IS_TERMUX && STDLIB_DEST="/data/data/com.termux/files/usr/share/xphage"
[ -n "$SUDO" ] && $SUDO mkdir -p "$STDLIB_DEST" || mkdir -p "$STDLIB_DEST"
if [ -n "$SUDO" ]; then
    $SUDO cp -r "$CLONE_DIR/library" "$STDLIB_DEST/"
else
    cp -r "$CLONE_DIR/library" "$STDLIB_DEST/"
fi
echo -e "  ✔ Stdlib installed: ${STDLIB_DEST}/library"

# ============================================================
# PATH hint for Termux / Windows Git Bash
# ============================================================
if [[ ":$PATH:" != *":${INSTALL_DIR}:"* ]]; then
    echo ""
    echo -e "${YELLOW}⚠ Add to PATH:${NC}"
    echo -e "   export PATH=\"\$PATH:${INSTALL_DIR}\""
    echo -e "   (Add to ~/.bashrc or ~/.zshrc)"
fi

# ============================================================
# Done
# ============================================================
echo ""
echo -e "${GREEN}╔══════════════════════════════════════════╗${NC}"
echo -e "${GREEN}║  ✅ X-Phage installed from source!       ║${NC}"
echo -e "${GREEN}╚══════════════════════════════════════════╝${NC}"
echo ""
echo -e "  Platform : ${CYAN}${OS} ${ARCH}${NC}"
echo -e "  Compiler : ${CYAN}${CXX_FOUND}${NC}"
echo ""
echo -e "${YELLOW}Next steps:${NC}"
echo -e "  ${CYAN}xphage --version${NC}    Check installation"
echo -e "  ${CYAN}xphage init${NC}         Create a new project"
echo -e "  ${CYAN}xphage${NC}              Start interactive REPL"
echo ""
echo -e "${PURPLE}Docs: https://github.com/${REPO}#readme${NC}"
