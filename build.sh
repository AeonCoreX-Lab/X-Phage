#!/bin/bash
# X-Phage Titan Ultimate Build Script v3.3
# Supports: Linux (x64/ARM64), Windows, Android, macOS, iOS
set -e 

GREEN='\033[1;32m'
PURPLE='\033[1;35m'
YELLOW='\033[1;33m'
RED='\033[1;31m'
NC='\033[0m'

echo -e "${PURPLE}🧬 AeonCoreX: Initiating X-Phage Build for Target: ${TARGET:-ALL}...${NC}"

# Clean
rm -rf bin
mkdir -p bin/linux bin/linux-arm64 bin/windows bin/android bin/macos bin/ios

# --- SOURCE DEFINITIONS (Includes transpiler.cpp now) ---
SOURCES="src/main.cpp src/lexer.cpp src/llvm_compiler.cpp src/transpiler.cpp src/runtime/xpm_core.cpp src/runtime/memory.cpp src/runtime/linker.cpp src/runtime/core_ops.cpp"
INCLUDES="-I./include"
STANDARD_FLAGS="-std=c++17 -O3 -pthread"

# Helper to get LLVM Flags
function get_llvm_flags() {
    if command -v llvm-config &> /dev/null; then
        echo "$(llvm-config --cxxflags --ldflags --libs core)"
    else
        echo ""
    fi
}

# ---------------------------------------------------------
# 🐧 1. LINUX (x64) - Try enabling LLVM
# ---------------------------------------------------------
if [[ "$TARGET" == "linux" || -z "$TARGET" ]]; then
    echo -e "${PURPLE}🐧 Building for Linux (x64)...${NC}"
    
    if command -v llvm-config &> /dev/null; then
        LLVM_FLAGS=$(get_llvm_flags)
        FEATURE_FLAGS="-DENABLE_LLVM"
        echo "   -> LLVM Detected: Enabling Titan Native Core."
        clang++ $SOURCES $INCLUDES -o bin/linux/xphage_linux_x64 $STANDARD_FLAGS $FEATURE_FLAGS $LLVM_FLAGS -ldl
    else
        echo -e "${YELLOW}   -> LLVM Not Found: Building with Transpiler Engine.${NC}"
        clang++ $SOURCES $INCLUDES -o bin/linux/xphage_linux_x64 $STANDARD_FLAGS
    fi
    echo -e "${GREEN}✔ Linux Build Success${NC}"
fi

# ---------------------------------------------------------
# 🐧 1.5. LINUX (ARM64) - Transpiler Mode (Raspberry Pi/Servers)
# ---------------------------------------------------------
if [[ "$TARGET" == "linux-arm64" || -z "$TARGET" ]]; then
    echo -e "${PURPLE}🐧 Building for Linux (ARM64)...${NC}"
    
    # We deliberately DO NOT pass -DENABLE_LLVM here usually to ensure stability on ARM,
    # unless the user has a full LLVM dev setup. Transpiler is safer here.
    
    if command -v aarch64-linux-gnu-g++ &> /dev/null; then
        # Cross-compiling
        aarch64-linux-gnu-g++ $SOURCES $INCLUDES -o bin/linux-arm64/xphage_linux_arm64 $STANDARD_FLAGS
        echo -e "${GREEN}✔ Linux ARM64 (Cross) Build Success${NC}"
    elif [[ $(uname -m) == "aarch64" ]]; then
        # Native ARM64 compilation (e.g. on the device itself)
        g++ $SOURCES $INCLUDES -o bin/linux-arm64/xphage_linux_arm64 $STANDARD_FLAGS
        echo -e "${GREEN}✔ Linux ARM64 (Native) Build Success${NC}"
    else
        echo -e "${YELLOW}⚠ ARM64 compiler not found. Skipping.${NC}"
    fi
fi

# ---------------------------------------------------------
# 🪟 2. WINDOWS (x64) - Transpiler Mode
# ---------------------------------------------------------
if [[ "$TARGET" == "windows" || -z "$TARGET" ]]; then
    echo -e "${PURPLE}🪟 Building for Windows (x64)...${NC}"
    if command -v x86_64-w64-mingw32-g++ &> /dev/null; then
        x86_64-w64-mingw32-g++ $SOURCES $INCLUDES -o bin/windows/xphage.exe $STANDARD_FLAGS -static-libgcc -static-libstdc++
        echo -e "${GREEN}✔ Windows Build Success${NC}"
    fi
fi

# ---------------------------------------------------------
# 🤖 3. ANDROID (ARM64) - Transpiler Mode
# ---------------------------------------------------------
if [[ "$TARGET" == "android" || -z "$TARGET" ]]; then
    echo -e "${PURPLE}🤖 Building for Android (ARM64)...${NC}"
    if command -v aarch64-linux-gnu-g++ &> /dev/null; then
        aarch64-linux-gnu-g++ $SOURCES $INCLUDES -o bin/android/xphage_android_arm64 $STANDARD_FLAGS -pie -fPIE -D__ANDROID__
        echo -e "${GREEN}✔ Android Build Success${NC}"
    fi
fi

# ---------------------------------------------------------
# 🍎 4. macOS - LLVM Enabled
# ---------------------------------------------------------
if [[ "$TARGET" == "macos" || -z "$TARGET" ]]; then
    echo -e "${PURPLE}🍎 Building for macOS...${NC}"
    if [[ "$OSTYPE" == "darwin"* ]]; then
        LLVM_FLAGS=$(get_llvm_flags)
        FEATURE_FLAGS="-DENABLE_LLVM"
        clang++ $SOURCES $INCLUDES -o bin/macos/xphage_mac $STANDARD_FLAGS -arch x86_64 -arch arm64 $FEATURE_FLAGS $LLVM_FLAGS
        echo -e "${GREEN}✔ macOS Build Success${NC}"
    fi
fi

# ---------------------------------------------------------
# 📱 5. iOS (ARM64) - Transpiler Mode
# ---------------------------------------------------------
if [[ "$TARGET" == "ios" || -z "$TARGET" ]]; then
    echo -e "${PURPLE}📱 Building for iOS (ARM64)...${NC}"
    if [[ "$OSTYPE" == "darwin"* ]]; then
        SDK_PATH=$(xcrun --sdk iphoneos --show-sdk-path)
        clang++ $SOURCES $INCLUDES -o bin/ios/xphage_ios_arm64 -arch arm64 -isysroot "$SDK_PATH" -miphoneos-version-min=14.0 $STANDARD_FLAGS
        echo -e "${GREEN}✔ iOS Build Success${NC}"
    fi
fi
