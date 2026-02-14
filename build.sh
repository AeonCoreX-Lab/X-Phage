#!/bin/bash
# X-Phage Omni-Platform Modular Build Script v3.1
# Targets: Windows, Linux, Android, macOS
# Mode: Omni-God Modular Architecture

echo -e "\033[1;35m🧬 AeonCoreX: Initiating X-Phage v3.1 Multi-Platform Build...\033[0m"

# ফোল্ডার সেটআপ
rm -rf bin
mkdir -p bin/linux bin/windows bin/android bin/macos

# সোর্স কনফিগারেশন (Wildcard automatically picks up xpm_core.cpp)
SOURCES="src/main.cpp src/lexer.cpp src/runtime/*.cpp"
INCLUDES="-I./include"
FLAGS="-std=c++17 -O3 -pthread"

# 🐧 LINUX (x64)
echo "🐧 Building for Linux..."
clang++ $SOURCES $INCLUDES -o bin/linux/xphage_x64 $FLAGS

# 🪟 WINDOWS (x64)
echo "🪟 Building for Windows..."
x86_64-w64-mingw32-g++ $SOURCES $INCLUDES -o bin/windows/xphage_x64.exe $FLAGS -static

# 🤖 ANDROID (ARM64)
if [ -n "$ANDROID_NDK_HOME" ]; then
    echo "🤖 Building for Android..."
    $ANDROID_NDK_HOME/toolchains/llvm/prebuilt/linux-x86_64/bin/aarch64-linux-android34-clang++ \
    $SOURCES $INCLUDES -o bin/android/xphage_arm64 $FLAGS
fi

# 🍎 MACOS (Universal)
echo "🍎 Building for macOS..."
o64-clang++ -target arm64-apple-macos11 $SOURCES $INCLUDES -o bin/macos/xphage_universal $FLAGS

echo -e "\033[1;32m✔ Build Complete. Binaries are in /bin folder.\033[0m"
