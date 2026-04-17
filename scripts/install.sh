#!/bin/bash
# ============================================================
# 🧬 X-Phage Universal Installer v3.5.0
# Supports: Linux x64/ARM64, macOS (Universal), Android/Termux,
#           Windows (Git Bash / WSL)
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
if ! command -v curl &>/dev/null; then
    echo -e "${RED}✖ Required: 'curl' is not installed.${NC}"
    echo "  Install it and re-run this script."
    exit 1
fi

SHA256_CMD=""
if   command -v sha256sum &>/dev/null; then SHA256_CMD="sha256sum"
elif command -v shasum    &>/dev/null; then SHA256_CMD="shasum -a 256"
elif command -v certutil  &>/dev/null; then SHA256_CMD="certutil"
fi

# ============================================================
# Environment detection
# ============================================================
OS="$(uname -s 2>/dev/null || echo 'Windows')"
ARCH="$(uname -m 2>/dev/null || echo 'x86_64')"

PLATFORM="" BINARY_NAME="" INSTALL_DIR="" SUDO=""
IS_WINDOWS=false IS_TERMUX=false

# --- Termux (Android) ---
if [ -d "/data/data/com.termux/files/usr/bin" ]; then
    IS_TERMUX=true
    INSTALL_DIR="/data/data/com.termux/files/usr/bin"
    PLATFORM="android"; BINARY_NAME="xphage_android_arm64"
    echo -e "📱 Environment: ${GREEN}Termux (Android ARM64)${NC}"

# --- Windows Git Bash / MSYS ---
elif [[ "$OS" == MINGW* || "$OS" == MSYS* || "$OS" == CYGWIN* || "$OS" == "Windows" ]]; then
    IS_WINDOWS=true
    INSTALL_DIR="${USERPROFILE}/AppData/Local/xphage/bin"
    if [[ "$ARCH" == "aarch64" || "$ARCH" == "arm64" ]]; then
        PLATFORM="windows-arm64"; BINARY_NAME="xphage_arm64.exe"
    else
        PLATFORM="windows-x64"; BINARY_NAME="xphage.exe"
    fi
    echo -e "🪟 Environment: ${GREEN}Windows ${ARCH}${NC}"

# --- WSL ---
elif [[ "$OS" == "Linux" ]] && grep -qi microsoft /proc/version 2>/dev/null; then
    INSTALL_DIR="/usr/local/bin"; SUDO="sudo"
    if [[ "$ARCH" == "aarch64" || "$ARCH" == "arm64" ]]; then
        PLATFORM="linux-arm64"; BINARY_NAME="xphage_linux_arm64"
    else
        PLATFORM="linux-x64"; BINARY_NAME="xphage_linux_x64"
    fi
    echo -e "🐧 Environment: ${GREEN}WSL ${ARCH}${NC}"

# --- macOS ---
elif [[ "$OS" == "Darwin" ]]; then
    INSTALL_DIR="/usr/local/bin"; SUDO="sudo"
    PLATFORM="macos-universal"; BINARY_NAME="xphage_mac_universal"
    echo -e "🍎 Environment: ${GREEN}macOS $(sw_vers -productVersion 2>/dev/null) ${ARCH}${NC}"
    echo -e "   ${CYAN}Using Universal Binary (arm64 + x64)${NC}"

# --- Linux ---
elif [[ "$OS" == "Linux" ]]; then
    INSTALL_DIR="/usr/local/bin"; SUDO="sudo"
    if   [[ "$ARCH" == "x86_64" ]];                     then PLATFORM="linux-x64";   BINARY_NAME="xphage_linux_x64"
    elif [[ "$ARCH" == "aarch64" || "$ARCH" == "arm64" ]]; then PLATFORM="linux-arm64"; BINARY_NAME="xphage_linux_arm64"
    else
        echo -e "${RED}✖ Unsupported architecture: ${ARCH}${NC}"; exit 1
    fi
    DISTRO=""; [ -f /etc/os-release ] && DISTRO=$(grep '^PRETTY_NAME=' /etc/os-release | cut -d= -f2 | tr -d '"')
    echo -e "🐧 Environment: ${GREEN}Linux ${ARCH}${NC}${DISTRO:+ (${DISTRO})}"

else
    echo -e "${RED}✖ Unsupported OS: ${OS}${NC}"; exit 1
fi

# ============================================================
# Resolve latest version
# ============================================================
echo ""
echo -e "${CYAN}[1/4] Resolving latest version...${NC}"

LATEST_TAG=$(curl -sL "${API_BASE}" -H "Accept: application/vnd.github+json" \
    | grep '"tag_name"' | head -1 \
    | sed 's/.*"tag_name": *"\([^"]*\)".*/\1/')

if [ -z "$LATEST_TAG" ]; then
    LATEST_TAG="latest"
    echo -e "${YELLOW}  ⚠ Could not resolve version, using 'latest'${NC}"
else
    echo -e "  ✔ Latest release: ${GREEN}${LATEST_TAG}${NC}"
fi

# ============================================================
# Temp directory
#
# FIX: Use ${TMPDIR:-/tmp} everywhere — single line, no conditionals.
#
# Why this was broken before:
#   The previous code had a separate Termux block that tried to use
#   /data/data/com.termux/files/usr/tmp directly. If that directory
#   didn't exist yet, mktemp failed with "Permission denied".
#
# Why this works now:
#   On Termux: $TMPDIR is always exported by Termux to its own tmp path.
#   On Linux/macOS: $TMPDIR may be set by the shell, fallback is /tmp.
#   On Windows Git Bash: $TMPDIR is set by Git Bash to a Windows temp path.
#   mkdir -p ensures the directory exists before mktemp runs.
# ============================================================
XP_TMPDIR="${TMPDIR:-/tmp}"
mkdir -p "$XP_TMPDIR" 2>/dev/null || true

TMP_BIN="$(mktemp "${XP_TMPDIR}/xphage_install.XXXXXX")"
TMP_SHA="$(mktemp "${XP_TMPDIR}/xphage_sha.XXXXXX")"

# ============================================================
# Download binary
# ============================================================
echo ""
echo -e "${CYAN}[2/4] Downloading binary: ${BINARY_NAME}...${NC}"
echo -e "  URL: ${RELEASE_BASE}/${BINARY_NAME}"

if ! curl -L --progress-bar "${RELEASE_BASE}/${BINARY_NAME}" -o "$TMP_BIN"; then
    echo -e "${RED}✖ Download failed.${NC}"
    echo "  Try: curl -L ${RELEASE_BASE}/${BINARY_NAME} -o xphage"
    rm -f "$TMP_BIN" "$TMP_SHA"; exit 1
fi

# Guard against GitHub 404 HTML page
if file "$TMP_BIN" 2>/dev/null | grep -qi "HTML"; then
    echo -e "${RED}✖ Got HTML instead of binary — asset not found in release.${NC}"
    echo "  Expected asset name: ${BINARY_NAME}"
    rm -f "$TMP_BIN" "$TMP_SHA"; exit 1
fi

# ============================================================
# SHA256 verification
# ============================================================
echo ""
echo -e "${CYAN}[3/4] Verifying integrity (SHA256)...${NC}"

SHA_OK=false
if [ -n "$SHA256_CMD" ]; then
    if curl -sL "${RELEASE_BASE}/${BINARY_NAME}.sha256" -o "$TMP_SHA" 2>/dev/null; then
        EXPECTED=$(awk '{print $1}' "$TMP_SHA")
        if [ -n "$EXPECTED" ] && ! echo "$EXPECTED" | grep -qi "not found\|404\|<!"; then
            if   [[ "$SHA256_CMD" == "certutil" ]];     then ACTUAL=$(certutil -hashfile "$TMP_BIN" SHA256 2>/dev/null | grep -v ":" | tr -d ' \r\n')
            elif [[ "$SHA256_CMD" == "shasum -a 256" ]]; then ACTUAL=$(shasum -a 256 "$TMP_BIN" | awk '{print $1}')
            else                                              ACTUAL=$(sha256sum "$TMP_BIN" | awk '{print $1}')
            fi
            if [[ "$ACTUAL" == "$EXPECTED" ]]; then
                echo -e "  ✔ ${GREEN}SHA256 verified.${NC}"; SHA_OK=true
            else
                echo -e "  ${RED}✖ SHA256 mismatch — download may be corrupted.${NC}"
                echo "    Expected: $EXPECTED"; echo "    Got:      $ACTUAL"
                rm -f "$TMP_BIN" "$TMP_SHA"; exit 1
            fi
        else
            echo -e "  ${YELLOW}⚠ SHA256 file not available — skipping.${NC}"
        fi
    else
        echo -e "  ${YELLOW}⚠ Could not fetch SHA256 — skipping.${NC}"
    fi
else
    echo -e "  ${YELLOW}⚠ No sha256 tool — skipping. Install sha256sum for integrity checking.${NC}"
fi
rm -f "$TMP_SHA"

# ============================================================
# Install
# ============================================================
echo ""
echo -e "${CYAN}[4/4] Installing...${NC}"

chmod +x "$TMP_BIN"

if [ ! -d "$INSTALL_DIR" ]; then
    [ -n "$SUDO" ] && $SUDO mkdir -p "$INSTALL_DIR" || mkdir -p "$INSTALL_DIR"
fi

DEST="${INSTALL_DIR}/xphage"
$IS_WINDOWS && DEST="${INSTALL_DIR}/xphage.exe"

if [ -n "$SUDO" ]; then
    $SUDO mv "$TMP_BIN" "$DEST" && $SUDO chmod +x "$DEST"
else
    mv "$TMP_BIN" "$DEST" && chmod +x "$DEST"
fi

if $IS_WINDOWS && [[ ":$PATH:" != *":${INSTALL_DIR}:"* ]]; then
    echo ""
    echo -e "${YELLOW}⚠ Add to PATH: export PATH=\"\$PATH:${INSTALL_DIR}\"${NC}"
fi

# ============================================================
# Done
# ============================================================
echo ""
echo -e "${GREEN}╔══════════════════════════════════════════╗${NC}"
echo -e "${GREEN}║  ✅ X-Phage ${LATEST_TAG} installed successfully!  ║${NC}"
echo -e "${GREEN}╚══════════════════════════════════════════╝${NC}"
echo ""
echo -e "  Platform:  ${CYAN}${PLATFORM}${NC}"
echo -e "  Binary:    ${CYAN}${DEST}${NC}"
$SHA_OK && echo -e "  Integrity: ${GREEN}✔ SHA256 verified${NC}"
echo ""
echo -e "${YELLOW}Next steps:${NC}"
echo -e "  ${CYAN}xphage --version${NC}        Check installation"
echo -e "  ${CYAN}xphage init${NC}             Create a new project"
echo -e "  ${CYAN}xphage update-stdlib${NC}    Download standard library"
echo -e "  ${CYAN}xphage${NC}                  Start interactive REPL"
echo ""
echo -e "${PURPLE}Docs: https://github.com/${REPO}#readme${NC}"
