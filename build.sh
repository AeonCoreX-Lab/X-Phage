#!/bin/bash
# X-Phage Omni-Platform Modular Build Script v3.3
# Fixed: Checks TARGET variable to build specific platform

# স্ট্রিক্ট মোড: কোনো কমান্ড ফেইল করলে স্ক্রিপ্ট থেমে যাবে
set -e 

GREEN='\033[1;32m'
PURPLE='\033[1;35m'
YELLOW='\033[1;33m'
RED='\033[1;31m'
NC='\033[0m'

echo -e "${PURPLE}🧬 AeonCoreX: Initiating X-Phage Build for Target: ${TARGET:-ALL}...${NC}"

# ফোল্ডার সেটআপ
rm -rf bin
mkdir -p bin/linux bin/windows bin/android bin/macos bin/ios

SOURCES="src/main.cpp src/lexer.cpp src/runtime/*.cpp"
INCLUDES="-I./include"
FLAGS="-std=c++17 -O3 -pthread"

# ---------------------------------------------------------
# 🐧 1. LINUX (x64)
# ---------------------------------------------------------
if [[ "$TARGET" == "linux" || -z "$TARGET" ]]; then
    echo -e "${PURPLE}🐧 Building for Linux (x64)...${NC}"
    if command -v clang++ &> /dev/null; then
        clang++ $SOURCES $INCLUDES -o bin/linux/xphage_linux_x64 $FLAGS
        echo -e "${GREEN}✔ Linux Build Success${NC}"
    else
        echo -e "${RED}✘ clang++ not found!${NC}" && exit 1
    fi
fi

# ---------------------------------------------------------
# 🪟 2. WINDOWS (x64)
# ---------------------------------------------------------
if [[ "$TARGET" == "windows" || -z "$TARGET" ]]; then
    echo -e "${PURPLE}🪟 Building for Windows (x64)...${NC}"
    if command -v x86_64-w64-mingw32-g++ &> /dev/null; then
        x86_64-w64-mingw32-g++ $SOURCES $INCLUDES -o bin/windows/xphage.exe $FLAGS -static
        echo -e "${GREEN}✔ Windows Build Success${NC}"
    else
        echo -e "${YELLOW}⚠ MinGW not found. Skipping.${NC}"
    fi
fi

# ---------------------------------------------------------
# 🤖 3. ANDROID (ARM64)
# ---------------------------------------------------------
if [[ "$TARGET" == "android" || -z "$TARGET" ]]; then
    echo -e "${PURPLE}🤖 Building for Android (ARM64)...${NC}"
    if [ -n "$ANDROID_NDK_HOME" ]; then
        HOST_TAG="linux-x86_64"
        [[ "$OSTYPE" == "darwin"* ]] && HOST_TAG="darwin-x86_64"
        
        NDK_CLANG="$ANDROID_NDK_HOME/toolchains/llvm/prebuilt/$HOST_TAG/bin/aarch64-linux-android34-clang++"
        
        if [ -f "$NDK_CLANG" ]; then
            $NDK_CLANG $SOURCES $INCLUDES -o bin/android/xphage_android_arm64 $FLAGS -static-libstdc++
            echo -e "${GREEN}✔ Android Build Success${NC}"
        else
            echo -e "${RED}✘ NDK Path invalid!${NC}" && exit 1
        fi
    else
        echo -e "${RED}✘ ANDROID_NDK_HOME not set!${NC}" && exit 1
    fi
fi

# ---------------------------------------------------------
# 🍎 4. MACOS (Universal/x64)
# ---------------------------------------------------------
if [[ "$TARGET" == "macos" || -z "$TARGET" ]]; then
    echo -e "${PURPLE}🍎 Building for macOS...${NC}"
    if [[ "$OSTYPE" == "darwin"* ]]; then
        # Mac Runner-এ সরাসরি clang ব্যবহার
        clang++ $SOURCES $INCLUDES -o bin/macos/xphage_mac $FLAGS -arch x86_64 -arch arm64
        echo -e "${GREEN}✔ macOS (Universal) Build Success${NC}"
    elif command -v o64-clang++ &> /dev/null; then
        # Cross-compile
        o64-clang++ $SOURCES $INCLUDES -o bin/macos/xphage_mac $FLAGS
        echo -e "${GREEN}✔ macOS (Cross) Build Success${NC}"
    else
        echo -e "${RED}✘ No macOS compiler found!${NC}" && exit 1
    fi
fi

# ---------------------------------------------------------
# 📱 5. iOS (ARM64)
# ---------------------------------------------------------
if [[ "$TARGET" == "ios" || -z "$TARGET" ]]; then
    echo -e "${PURPLE}📱 Building for iOS (ARM64)...${NC}"
    if [[ "$OSTYPE" == "darwin"* ]]; then
        SDK_PATH=$(xcrun --sdk iphoneos --show-sdk-path)
        # iOS-এর জন্য স্পেশাল ফ্ল্যাগস
        clang++ $SOURCES $INCLUDES \
            -o bin/ios/xphage_ios_arm64 \
            -arch arm64 \
            -isysroot "$SDK_PATH" \
            -miphoneos-version-min=14.0 \
            -fembed-bitcode \
            $FLAGS
        
        echo -e "${GREEN}✔ iOS Build Success${NC}"
    else
        echo -e "${RED}✘ iOS build requires macOS runner!${NC}" && exit 1
    fi
fi

echo -e "${GREEN}✨ Build Phase Complete.${NC}"
