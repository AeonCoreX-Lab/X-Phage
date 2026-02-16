#!/bin/bash
# 🧬 X-Phage Titan Universal Installer v3.3.1
set -e

GREEN='\033[1;32m'
CYAN='\033[1;36m'
YELLOW='\033[1;33m'
RED='\033[1;31m'
NC='\033[0m'

echo -e "${CYAN}--- X-Phage Titan Installation ---${NC}"

if ! command -v curl &> /dev/null; then
    echo -e "${RED}Error: 'curl' is not installed. Please install curl first.${NC}"
    exit 1
fi

OS="$(uname -s)"
ARCH="$(uname -m)"

if [ -d "/data/data/com.termux/files/usr/bin" ]; then
    INSTALL_DIR="/data/data/com.termux/files/usr/bin"
    SUDO=""
    PLATFORM="android_arm64"
    echo -e "📱 Environment: Termux detected."
else
    INSTALL_DIR="/usr/local/bin"
    SUDO="sudo"
    
    if [ "$OS" == "Darwin" ]; then
        PLATFORM="macos_universal"
    elif [ "$OS" == "Linux" ]; then
        if [ "$ARCH" == "x86_64" ]; then
            PLATFORM="linux_x64"
        elif [ "$ARCH" == "aarch64" ] || [ "$ARCH" == "arm64" ]; then
            PLATFORM="linux_arm64"
        else
            echo -e "${RED}Error: Unsupported architecture: $ARCH${NC}"
            exit 1
        fi
    else
        echo -e "${RED}Error: Unsupported OS: $OS${NC}"
        exit 1
    fi
    echo -e "💻 Environment: Desktop ${OS} detected."
fi

REPO_URL="https://github.com/AeonCoreX-Lab/X-Phage/releases/latest/download"
BINARY_NAME="xphage_${PLATFORM}"

echo -e "[1/2] Fetching the latest Titan binary: ${BINARY_NAME}..."
curl -sL "${REPO_URL}/${BINARY_NAME}" -o xphage_temp

echo -e "[2/2] Integrating with system PATH..."
chmod +x xphage_temp
$SUDO mv xphage_temp $INSTALL_DIR/xphage

echo -e "\n${GREEN}✅ X-Phage Titan is now installed globally!${NC}"
echo -e "Try running: ${CYAN}xphage --version${NC}"
