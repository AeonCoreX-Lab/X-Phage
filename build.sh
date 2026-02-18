#!/bin/bash
# X-Phage Titan Ultimate Build Script v3.5 [STABLE]
# Supports: Linux (x64/ARM64), Windows, Android, macOS, iOS
# Fixes: Auto-fallback to Transpiler if LLVM fails.
set -e 

GREEN='\033[1;32m'
PURPLE='\033[1;35m'
YELLOW='\033[1;33m'
RED='\033[1;31m'
CYAN='\033[1;36m'
NC='\033[0m'

echo -e "${PURPLE}🧬 AeonCoreX: Initiating X-Phage Build for Target: ${TARGET:-ALL}...${NC}"

# Clean
rm -rf bin
mkdir -p bin/linux bin/linux-arm64 bin/windows bin/android bin/macos bin/ios

# --- SOURCE DEFINITIONS ---
SOURCES="src/main.cpp src/lexer.cpp src/llvm_compiler.cpp src/transpiler.cpp src/runtime/xpm_core.cpp src/runtime/memory.cpp src/runtime/linker.cpp src/runtime/core_ops.cpp"
INCLUDES="-I./include"
STANDARD_FLAGS="-std=c++17 -O3 -pthread"

# --- HELPER: COMPILE WITH FALLBACK ---
# Tries to compile with LLVM; if it fails, falls back to Transpiler mode.
function compile_smart() {
    local PLATFORM=$1
    local OUTPUT=$2
    local FLAGS=$3
    local COMPILER=$4
    local TRY_LLVM=$5

    # 1. Try LLVM Build (If requested and available)
    if [[ "$TRY_LLVM" == "true" ]]; then
        echo -e "${CYAN}   -> Attempting LLVM Native Core Build...${NC}"
        
        # specific header/lib search for LLVM
        local LLVM_CONF="llvm-config"
        
        # macOS Homebrew Path Fix
        if [[ "$PLATFORM" == "macos" ]]; then
            if [ -d "/opt/homebrew/opt/llvm/bin" ]; then
                LLVM_CONF="/opt/homebrew/opt/llvm/bin/llvm-config"
            elif [ -d "/usr/local/opt/llvm/bin" ]; then
                LLVM_CONF="/usr/local/opt/llvm/bin/llvm-config"
            fi
        fi

        if command -v $LLVM_CONF &> /dev/null; then
            # Capture specific flags
            local L_CFLAGS=$($LLVM_CONF --cxxflags)
            local L_LDFLAGS=$($LLVM_CONF --ldflags --libs core)
            local L_INC="-I$($LLVM_CONF --includedir)"
            
            # Attempt Compilation
            set +e # Temporarily allow failure
            $COMPILER $SOURCES $INCLUDES $L_INC -o $OUTPUT $FLAGS -DENABLE_LLVM $L_CFLAGS $L_LDFLAGS -ldl 2>/dev/null
            RES=$?
            set -e # Re-enable strict mode

            if [[ $RES -eq 0 ]]; then
                echo -e "${GREEN}✔ $PLATFORM (LLVM Mode) Build Success${NC}"
                return 0
            else
                echo -e "${YELLOW}⚠ LLVM Build Failed (Missing headers/libs). Falling back to Titan Transpiler.${NC}"
            fi
        else
            echo -e "${YELLOW}⚠ LLVM toolchain not found. Using Titan Transpiler.${NC}"
        fi
    fi

    # 2. Transpiler Mode (Fallback / Default)
    echo -e "${CYAN}   -> Building with Titan Transpiler Engine...${NC}"
    $COMPILER $SOURCES $INCLUDES -o $OUTPUT $FLAGS
    echo -e "${GREEN}✔ $PLATFORM (Transpiler Mode) Build Success${NC}"
}

# ---------------------------------------------------------
# 🐧 1. LINUX (x64)
# ---------------------------------------------------------
if [[ "$TARGET" == "linux" || -z "$TARGET" ]]; then
    echo -e "${PURPLE}🐧 Building for Linux (x64)...${NC}"
    compile_smart "Linux x64" "bin/linux/xphage" "$STANDARD_FLAGS" "clang++" "true"
fi

# ---------------------------------------------------------
# 🐧 1.5. LINUX (ARM64)
# ---------------------------------------------------------
if [[ "$TARGET" == "linux-arm64" || -z "$TARGET" ]]; then
    echo -e "${PURPLE}🐧 Building for Linux (ARM64)...${NC}"
    if command -v aarch64-linux-gnu-g++ &> /dev/null; then
        compile_smart "Linux ARM64" "bin/linux-arm64/xphage_arm64" "$STANDARD_FLAGS" "aarch64-linux-gnu-g++" "false"
    elif [[ $(uname -m) == "aarch64" ]]; then
        compile_smart "Linux ARM64 (Native)" "bin/linux-arm64/xphage_arm64" "$STANDARD_FLAGS" "g++" "false"
    else
        echo -e "${YELLOW}⚠ ARM64 compiler not found. Skipping.${NC}"
    fi
fi

# ---------------------------------------------------------
# 🪟 2. WINDOWS (x64)
# ---------------------------------------------------------
if [[ "$TARGET" == "windows" || -z "$TARGET" ]]; then
    echo -e "${PURPLE}🪟 Building for Windows (x64)...${NC}"
    if command -v x86_64-w64-mingw32-g++ &> /dev/null; then
        compile_smart "Windows" "bin/windows/xphage.exe" "$STANDARD_FLAGS -static-libgcc -static-libstdc++" "x86_64-w64-mingw32-g++" "false"
    fi
fi

# ---------------------------------------------------------
# 🤖 3. ANDROID (ARM64)
# ---------------------------------------------------------
if [[ "$TARGET" == "android" || -z "$TARGET" ]]; then
    echo -e "${PURPLE}🤖 Building for Android (ARM64)...${NC}"
    if command -v aarch64-linux-gnu-g++ &> /dev/null; then
        compile_smart "Android" "bin/android/xphage_android" "$STANDARD_FLAGS -pie -fPIE -D__ANDROID__" "aarch64-linux-gnu-g++" "false"
    fi
fi

# ---------------------------------------------------------
# 🍎 4. macOS
# ---------------------------------------------------------
if [[ "$TARGET" == "macos" || -z "$TARGET" ]]; then
    echo -e "${PURPLE}🍎 Building for macOS...${NC}"
    if [[ "$OSTYPE" == "darwin"* ]]; then
        compile_smart "macOS" "bin/macos/xphage_mac" "$STANDARD_FLAGS -arch x86_64 -arch arm64" "clang++" "true"
    fi
fi

# ---------------------------------------------------------
# 📱 5. iOS (ARM64)
# ---------------------------------------------------------
if [[ "$TARGET" == "ios" || -z "$TARGET" ]]; then
    echo -e "${PURPLE}📱 Building for iOS (ARM64)...${NC}"
    if [[ "$OSTYPE" == "darwin"* ]]; then
        SDK_PATH=$(xcrun --sdk iphoneos --show-sdk-path)
        compile_smart "iOS" "bin/ios/xphage_ios" "-arch arm64 -isysroot $SDK_PATH -miphoneos-version-min=14.0 $STANDARD_FLAGS" "clang++" "false"
    fi
fi
