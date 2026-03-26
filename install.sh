#!/bin/bash
# ============================================================
# 🧬 X-Phage Universal Installer v3.5.0
# Supports: Linux x64/ARM64, macOS (Universal), Android/Termux,
#           Windows (Git Bash / WSL), iOS (Termux-style)
# ============================================================
set -e

GREEN='\033[1;32m'
CYAN='\033[1;36m'
YELLOW='\033[1;33m'
RED='\033[1;31m'
PURPLE='\033[1;35m'
NC='\033[0m'

REPO="AeonCoreX-Lab/X-Phage"
RELEASE_BASE="https://github.com/${REPO}/releases/latest/download"
API_BASE="https://api.github.com/repos/${REPO}/releases/latest"

# ============================================================
# Banner
# ============================================================
echo -e "${PURPLE}"
echo "  _  _  ____  __  __ "
echo " ( \/ )(  _ \(  \/  )"
echo "  )  (  )___/ )    ( "
echo " (_/\_)(__)  (_/\/_) ${GREEN}v3.5.0${PURPLE}"
echo ""
echo -e "${CYAN}🧬 X-Phage Universal Installer${NC}"
echo -e "${YELLOW}   Repository: https://github.com/${REPO}${NC}"
echo ""

# ============================================================
# Dependency check
# ============================================================
check_dep() {
    if ! command -v "$1" &>/dev/null; then
        echo -e "${RED}✖ Required: '$1' is not installed.${NC}"
        echo "  Install it and re-run this script."
        exit 1
    fi
}

check_dep curl

# sha256 verification — optional but recommended
SHA256_CMD=""
if command -v sha256sum &>/dev/null; then
    SHA256_CMD="sha256sum"
elif command -v shasum &>/dev/null; then
    SHA256_CMD="shasum -a 256"
elif command -v certutil &>/dev/null; then
    SHA256_CMD="certutil" # Windows fallback
fi

# ============================================================
# Environment detection
# ============================================================
OS="$(uname -s 2>/dev/null || echo 'Windows')"
ARCH="$(uname -m 2>/dev/null || echo 'x86_64')"

PLATFORM=""
BINARY_NAME=""
INSTALL_DIR=""
SUDO=""
IS_WINDOWS=false
IS_TERMUX=false

# --- Termux (Android) ---
if [ -d "/data/data/com.termux/files/usr/bin" ]; then
    IS_TERMUX=true
    INSTALL_DIR="/data/data/com.termux/files/usr/bin"
    SUDO=""
    PLATFORM="android"
    BINARY_NAME="xphage_android_arm64"
    echo -e "📱 Environment: ${GREEN}Termux (Android ARM64)${NC}"

# --- Windows (Git Bash / WSL) ---
elif [[ "$OS" == MINGW* || "$OS" == MSYS* || "$OS" == CYGWIN* || "$OS" == "Windows" ]]; then
    IS_WINDOWS=true
    INSTALL_DIR="${USERPROFILE}/AppData/Local/xphage/bin"
    SUDO=""
    if [[ "$ARCH" == "aarch64" || "$ARCH" == "arm64" ]]; then
        PLATFORM="windows-arm64"
        BINARY_NAME="xphage_arm64.exe"
    else
        PLATFORM="windows-x64"
        BINARY_NAME="xphage.exe"
    fi
    echo -e "🪟 Environment: ${GREEN}Windows ${ARCH}${NC}"

# --- WSL (Linux subsystem on Windows) ---
elif [[ "$OS" == "Linux" ]] && grep -qi microsoft /proc/version 2>/dev/null; then
    INSTALL_DIR="/usr/local/bin"
    SUDO="sudo"
    if [[ "$ARCH" == "aarch64" || "$ARCH" == "arm64" ]]; then
        PLATFORM="linux-arm64"
        BINARY_NAME="xphage_linux_arm64"
    else
        PLATFORM="linux-x64"
        BINARY_NAME="xphage_linux_x64"
    fi
    echo -e "🐧 Environment: ${GREEN}WSL (Linux on Windows) ${ARCH}${NC}"

# --- macOS ---
elif [[ "$OS" == "Darwin" ]]; then
    INSTALL_DIR="/usr/local/bin"
    SUDO="sudo"
    # Always use universal binary — works on both Apple Silicon and Intel
    PLATFORM="macos-universal"
    BINARY_NAME="xphage_mac_universal"
    echo -e "🍎 Environment: ${GREEN}macOS $(sw_vers -productVersion 2>/dev/null) ${ARCH}${NC}"
    echo -e "   ${CYAN}Using Universal Binary (arm64 + x64)${NC}"

# --- Linux ---
elif [[ "$OS" == "Linux" ]]; then
    INSTALL_DIR="/usr/local/bin"
    SUDO="sudo"
    if [[ "$ARCH" == "x86_64" ]]; then
        PLATFORM="linux-x64"
        BINARY_NAME="xphage_linux_x64"
    elif [[ "$ARCH" == "aarch64" || "$ARCH" == "arm64" ]]; then
        PLATFORM="linux-arm64"
        BINARY_NAME="xphage_linux_arm64"
    else
        echo -e "${RED}✖ Unsupported architecture: ${ARCH}${NC}"
        echo "  Supported: x86_64, aarch64/arm64"
        exit 1
    fi
    # Detect Linux distro for display
    DISTRO=""
    if [ -f /etc/os-release ]; then
        DISTRO=$(grep '^PRETTY_NAME=' /etc/os-release | cut -d= -f2 | tr -d '"')
    fi
    echo -e "🐧 Environment: ${GREEN}Linux ${ARCH}${NC} ${DISTRO:+(${DISTRO})}"

else
    echo -e "${RED}✖ Unsupported OS: ${OS}${NC}"
    echo "  Supported: Linux (x64/arm64), macOS, Android (Termux), Windows (Git Bash/WSL)"
    exit 1
fi

# ============================================================
# Resolve latest version tag from GitHub API
# ============================================================
echo ""
echo -e "${CYAN}[1/4] Resolving latest version...${NC}"

LATEST_TAG=""
if command -v curl &>/dev/null; then
    LATEST_TAG=$(curl -sL "${API_BASE}" \
        -H "Accept: application/vnd.github+json" \
        | grep '"tag_name"' \
        | head -1 \
        | sed 's/.*"tag_name": *"\([^"]*\)".*/\1/')
fi

if [ -z "$LATEST_TAG" ]; then
    LATEST_TAG="latest"
    echo -e "${YELLOW}  ⚠ Could not resolve version from API, using 'latest'${NC}"
else
    echo -e "  ✔ Latest release: ${GREEN}${LATEST_TAG}${NC}"
fi

# ============================================================
# Download binary
# ============================================================
echo ""
echo -e "${CYAN}[2/4] Downloading binary: ${BINARY_NAME}...${NC}"
echo -e "  URL: ${RELEASE_BASE}/${BINARY_NAME}"

TMP_BIN="$(mktemp /tmp/xphage_install.XXXXXX)"
TMP_SHA="$(mktemp /tmp/xphage_sha.XXXXXX)"

# Download binary with progress
if ! curl -L --progress-bar "${RELEASE_BASE}/${BINARY_NAME}" -o "$TMP_BIN"; then
    echo -e "${RED}✖ Download failed.${NC}"
    echo "  Check your internet connection or try:"
    echo "  curl -L ${RELEASE_BASE}/${BINARY_NAME} -o xphage"
    rm -f "$TMP_BIN" "$TMP_SHA"
    exit 1
fi

# Verify file is not an HTML error page (GitHub 404 returns HTML)
if file "$TMP_BIN" 2>/dev/null | grep -qi "HTML"; then
    echo -e "${RED}✖ Download returned an error page (binary not found in release).${NC}"
    echo "  Make sure ${LATEST_TAG} contains asset: ${BINARY_NAME}"
    rm -f "$TMP_BIN" "$TMP_SHA"
    exit 1
fi

# ============================================================
# SHA256 verification
# ============================================================
echo ""
echo -e "${CYAN}[3/4] Verifying integrity (SHA256)...${NC}"

SHA_URL="${RELEASE_BASE}/${BINARY_NAME}.sha256"
SHA_OK=false

if [ -n "$SHA256_CMD" ]; then
    if curl -sL "$SHA_URL" -o "$TMP_SHA" 2>/dev/null; then
        EXPECTED=$(cat "$TMP_SHA" | awk '{print $1}')
        if [ -n "$EXPECTED" ] && ! echo "$EXPECTED" | grep -qi "not found\|404\|<!"; then
            if [[ "$SHA256_CMD" == "certutil" ]]; then
                # Windows certutil
                ACTUAL=$(certutil -hashfile "$TMP_BIN" SHA256 2>/dev/null | grep -v ":" | tr -d ' \r\n')
            elif [[ "$SHA256_CMD" == "shasum -a 256" ]]; then
                ACTUAL=$(shasum -a 256 "$TMP_BIN" | awk '{print $1}')
            else
                ACTUAL=$(sha256sum "$TMP_BIN" | awk '{print $1}')
            fi

            if [[ "$ACTUAL" == "$EXPECTED" ]]; then
                echo -e "  ✔ ${GREEN}SHA256 verified.${NC}"
                SHA_OK=true
            else
                echo -e "  ${RED}✖ SHA256 mismatch!${NC}"
                echo "    Expected: $EXPECTED"
                echo "    Got:      $ACTUAL"
                echo "  The download may be corrupted. Aborting."
                rm -f "$TMP_BIN" "$TMP_SHA"
                exit 1
            fi
        else
            echo -e "  ${YELLOW}⚠ SHA256 file not available — skipping verification.${NC}"
        fi
    else
        echo -e "  ${YELLOW}⚠ Could not fetch SHA256 — skipping verification.${NC}"
    fi
else
    echo -e "  ${YELLOW}⚠ No sha256 tool found — skipping verification.${NC}"
    echo "    Install sha256sum or shasum for integrity checking."
fi

rm -f "$TMP_SHA"

# ============================================================
# Install binary
# ============================================================
echo ""
echo -e "${CYAN}[4/4] Installing to ${INSTALL_DIR}/xphage${IS_WINDOWS:+.exe}...${NC}"

chmod +x "$TMP_BIN"

# Create install dir if needed
if [ ! -d "$INSTALL_DIR" ]; then
    if [ -n "$SUDO" ]; then
        $SUDO mkdir -p "$INSTALL_DIR"
    else
        mkdir -p "$INSTALL_DIR"
    fi
fi

# Final binary name (keep .exe on Windows)
if $IS_WINDOWS; then
    DEST="${INSTALL_DIR}/xphage.exe"
else
    DEST="${INSTALL_DIR}/xphage"
fi

if [ -n "$SUDO" ]; then
    $SUDO mv "$TMP_BIN" "$DEST"
    $SUDO chmod +x "$DEST"
else
    mv "$TMP_BIN" "$DEST"
    chmod +x "$DEST"
fi

# Add to PATH for Windows (Git Bash) if not already there
if $IS_WINDOWS; then
    if [[ ":$PATH:" != *":${INSTALL_DIR}:"* ]]; then
        echo ""
        echo -e "${YELLOW}⚠ Add to PATH (run once):${NC}"
        echo "  export PATH=\"\$PATH:${INSTALL_DIR}\""
        echo "  Or add it permanently via System Properties → Environment Variables."
    fi
fi

# ============================================================
# Post-install
# ============================================================
echo ""
echo -e "${GREEN}╔══════════════════════════════════════════╗${NC}"
echo -e "${GREEN}║  ✅ X-Phage ${LATEST_TAG} installed successfully!  ║${NC}"
echo -e "${GREEN}╚══════════════════════════════════════════╝${NC}"
echo ""
echo -e "  Platform:  ${CYAN}${PLATFORM}${NC}"
echo -e "  Binary:    ${CYAN}${DEST}${NC}"
if $SHA_OK; then
echo -e "  Integrity: ${GREEN}✔ SHA256 verified${NC}"
fi
echo ""
echo -e "${YELLOW}Next steps:${NC}"
echo -e "  ${CYAN}xphage --version${NC}          Check installation"
echo -e "  ${CYAN}xphage init${NC}               Create a new project"
echo -e "  ${CYAN}xphage update-stdlib${NC}      Download standard library"
echo -e "  ${CYAN}xphage repl${NC}               Start interactive shell"
echo ""
echo -e "${PURPLE}Docs: https://github.com/${REPO}#readme${NC}"
