#!/bin/bash
# X-Phage Omni-Platform Modular Build Script v3.3.2
set -e 

GREEN='\033[1;32m'
PURPLE='\033[1;35m'
YELLOW='\033[1;33m'
RED='\033[1;31m'
NC='\033[0m'

echo -e "${PURPLE}🧬 AeonCoreX: Initiating X-Phage Build for Target: ${TARGET:-ALL}...${NC}"

# Clean and Prep
rm -rf bin
mkdir -p bin/linux bin/linux-arm64 bin/windows bin/android bin/macos bin/ios

# --- SOURCE DEFINITIONS ---
# Included llvm_compiler.cpp explicitly to fix Linker Errors
SOURCES="src/main.cpp src/lexer.cpp src/llvm_compiler.cpp src/runtime/xpm_core.cpp src/runtime/memory.cpp src/runtime/linker.cpp src/runtime/core_ops.cpp"
INCLUDES="-I./include"
STANDARD_FLAGS="-std=c++17 -O3 -pthread"

# Helper to get LLVM Flags only if llvm-config exists
function get_llvm_flags() {
    if command -v llvm-config &> /dev/null; then
        echo "$(llvm-config --cxxflags --ldflags --libs core)"
    else
        echo ""
    fi
}

# ---------------------------------------------------------
# 🐧 1. LINUX (x64)
# ---------------------------------------------------------
if [[ "$TARGET" == "linux" || -z "$TARGET" ]]; then
    echo -e "${PURPLE}🐧 Building for Linux (x64)...${NC}"
    LLVM_FLAGS=$(get_llvm_flags)
    
    if command -v clang++ &> /dev/null; then
        clang++ $SOURCES $INCLUDES -o bin/linux/xphage_linux_x64 $STANDARD_FLAGS $LLVM_FLAGS -ldl
        echo -e "${GREEN}✔ Linux Build Success${NC}"
    else
        echo -e "${RED}✘ clang++ not found!${NC}" && exit 1
    fi
fi

# ---------------------------------------------------------
# 🐧 1.5. LINUX (ARM64)
# ---------------------------------------------------------
if [[ "$TARGET" == "linux-arm64" || -z "$TARGET" ]]; then
    echo -e "${PURPLE}🐧 Building for Linux (ARM64)...${NC}"
    if command -v aarch64-linux-gnu-g++ &> /dev/null; then
        # Note: Excluding LLVM flags for cross-compile simple build
        aarch64-linux-gnu-g++ $SOURCES $INCLUDES -o bin/linux-arm64/xphage_linux_arm64 $STANDARD_FLAGS
        echo -e "${GREEN}✔ Linux ARM64 Build Success${NC}"
    else
        echo -e "${YELLOW}⚠ aarch64-linux-gnu-g++ not found! Skipping.${NC}"
    fi
fi

# ---------------------------------------------------------
# 🪟 2. WINDOWS (x64)
# ---------------------------------------------------------
if [[ "$TARGET" == "windows" || -z "$TARGET" ]]; then
    echo -e "${PURPLE}🪟 Building for Windows (x64)...${NC}"
    if command -v x86_64-w64-mingw32-g++ &> /dev/null; then
        x86_64-w64-mingw32-g++ $SOURCES $INCLUDES -o bin/windows/xphage.exe $STANDARD_FLAGS -static-libgcc -static-libstdc++
        echo -e "${GREEN}✔ Windows Build Success${NC}"
    else
        echo -e "${YELLOW}⚠ MinGW compiler not found. Skipping Windows.${NC}"
    fi
fi

# ---------------------------------------------------------
# 🤖 3. ANDROID (ARM64)
# ---------------------------------------------------------
if [[ "$TARGET" == "android" || -z "$TARGET" ]]; then
    echo -e "${PURPLE}🤖 Building for Android (ARM64)...${NC}"
    if command -v aarch64-linux-gnu-g++ &> /dev/null; then
        # -D__ANDROID__ helps disable LLVM in code
        aarch64-linux-gnu-g++ $SOURCES $INCLUDES -o bin/android/xphage_android_arm64 $STANDARD_FLAGS -pie -fPIE -D__ANDROID__
        echo -e "${GREEN}✔ Android Build Success${NC}"
    else
        echo -e "${YELLOW}⚠ Android cross-compiler not found. Skipping.${NC}"
    fi
fi

# ---------------------------------------------------------
# 🍎 4. macOS
# ---------------------------------------------------------
if [[ "$TARGET" == "macos" || -z "$TARGET" ]]; then
    echo -e "${PURPLE}🍎 Building for macOS...${NC}"
    if [[ "$OSTYPE" == "darwin"* ]]; then
        LLVM_FLAGS=$(get_llvm_flags)
        clang++ $SOURCES $INCLUDES -o bin/macos/xphage_mac $STANDARD_FLAGS -arch x86_64 -arch arm64 $LLVM_FLAGS
        echo -e "${GREEN}✔ macOS Build Success${NC}"
    else
        echo -e "${YELLOW}⚠ Not on macOS. Skipping.${NC}"
    fi
fi

# ---------------------------------------------------------
# 📱 5. iOS (ARM64)
# ---------------------------------------------------------
if [[ "$TARGET" == "ios" || -z "$TARGET" ]]; then
    echo -e "${PURPLE}📱 Building for iOS (ARM64)...${NC}"
    if [[ "$OSTYPE" == "darwin"* ]]; then
        SDK_PATH=$(xcrun --sdk iphoneos --show-sdk-path)
        # Using -DTARGET_OS_IPHONE to disable LLVM in code
        clang++ $SOURCES $INCLUDES -o bin/ios/xphage_ios_arm64 -arch arm64 -isysroot "$SDK_PATH" -miphoneos-version-min=14.0 $STANDARD_FLAGS
        echo -e "${GREEN}✔ iOS Build Success${NC}"
    else
        echo -e "${YELLOW}⚠ Not on macOS. Skipping iOS.${NC}"
    fi
fi
