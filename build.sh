#!/bin/bash
# X-Phage Omni-Platform Modular Build Script v3.2
# Targets: Windows, Linux, Android, macOS, iOS
# Mode: Titan Fusion Architecture

# কালার কোড
GREEN='\033[1;32m'
PURPLE='\033[1;35m'
YELLOW='\033[1;33m'
RED='\033[1;31m'
NC='\033[0m' # No Color

echo -e "${PURPLE}🧬 AeonCoreX: Initiating X-Phage v3.2 Omni-Platform Build...${NC}"

# ফোল্ডার ক্লিনআপ এবং তৈরি
rm -rf bin
mkdir -p bin/linux bin/windows bin/android bin/macos bin/ios

# সোর্স এবং ফ্ল্যাগ কনফিগারেশন
SOURCES="src/main.cpp src/lexer.cpp src/runtime/*.cpp"
INCLUDES="-I./include"
FLAGS="-std=c++17 -O3 -pthread"

# ---------------------------------------------------------
# 🐧 1. LINUX (x64)
# ---------------------------------------------------------
echo -e "${PURPLE}🐧 Building for Linux (x64)...${NC}"
if command -v clang++ &> /dev/null; then
    clang++ $SOURCES $INCLUDES -o bin/linux/xphage_linux_x64 $FLAGS
    echo -e "${GREEN}✔ Linux Build Success${NC}"
else
    echo -e "${RED}✘ clang++ not found. Skipping Linux build.${NC}"
fi

# ---------------------------------------------------------
# 🪟 2. WINDOWS (x64)
# ---------------------------------------------------------
echo -e "${PURPLE}🪟 Building for Windows (x64)...${NC}"
if command -v x86_64-w64-mingw32-g++ &> /dev/null; then
    x86_64-w64-mingw32-g++ $SOURCES $INCLUDES -o bin/windows/xphage.exe $FLAGS -static
    echo -e "${GREEN}✔ Windows Build Success${NC}"
else
    echo -e "${YELLOW}⚠ MinGW compiler not found. Skipping Windows build.${NC}"
fi

# ---------------------------------------------------------
# 🤖 3. ANDROID (ARM64)
# ---------------------------------------------------------
echo -e "${PURPLE}🤖 Building for Android (ARM64)...${NC}"
if [ -n "$ANDROID_NDK_HOME" ]; then
    # হোস্ট ওএস ডিটেকশন (Linux or Darwin/Mac)
    HOST_TAG="linux-x86_64"
    if [[ "$OSTYPE" == "darwin"* ]]; then
        HOST_TAG="darwin-x86_64"
    fi

    NDK_CLANG="$ANDROID_NDK_HOME/toolchains/llvm/prebuilt/$HOST_TAG/bin/aarch64-linux-android34-clang++"

    if [ -f "$NDK_CLANG" ]; then
        $NDK_CLANG $SOURCES $INCLUDES -o bin/android/xphage_android_arm64 $FLAGS -static-libstdc++
        echo -e "${GREEN}✔ Android Build Success${NC}"
    else
        echo -e "${RED}✘ NDK Compiler not found at expected path. Check ANDROID_NDK_HOME.${NC}"
    fi
else
    echo -e "${YELLOW}⚠ ANDROID_NDK_HOME not set. Skipping Android build.${NC}"
fi

# ---------------------------------------------------------
# 🍎 4. MACOS (Universal/x64)
# ---------------------------------------------------------
echo -e "${PURPLE}🍎 Building for macOS...${NC}"
# যদি লোকাল ম্যাকে রান করা হয়
if [[ "$OSTYPE" == "darwin"* ]]; then
    clang++ $SOURCES $INCLUDES -o bin/macos/xphage_mac $FLAGS
    echo -e "${GREEN}✔ macOS (Native) Build Success${NC}"
# যদি লিনাক্সে ক্রস-কম্পাইল করা হয় (osxcross)
elif command -v o64-clang++ &> /dev/null; then
    o64-clang++ $SOURCES $INCLUDES -o bin/macos/xphage_mac $FLAGS
    echo -e "${GREEN}✔ macOS (Cross-Compile) Build Success${NC}"
else
    echo -e "${YELLOW}⚠ No macOS compiler found. Skipping.${NC}"
fi

# ---------------------------------------------------------
# 📱 5. iOS (ARM64) - [NEW]
# ---------------------------------------------------------
echo -e "${PURPLE}📱 Building for iOS (ARM64)...${NC}"
if [[ "$OSTYPE" == "darwin"* ]] && command -v xcrun &> /dev/null; then
    # ম্যাক থেকে iOS SDK পাথ বের করা
    SDK_PATH=$(xcrun --sdk iphoneos --show-sdk-path)
    
    clang++ -arch arm64 -isysroot "$SDK_PATH" -miphoneos-version-min=14.0 \
    $SOURCES $INCLUDES -o bin/ios/xphage_ios_arm64 $FLAGS
    
    echo -e "${GREEN}✔ iOS Build Success${NC}"
else
    echo -e "${YELLOW}⚠ iOS build requires running on macOS with Xcode installed. Skipping.${NC}"
fi

# ---------------------------------------------------------
echo -e "${GREEN}✨ All Tasks Completed. Check /bin directory.${NC}"
