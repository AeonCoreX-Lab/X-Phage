#!/bin/bash
# X-Phage Titan Ultimate Build Script v4.2 [LLVM FIXED]
# Supports: Linux (x64/ARM64 - LLVM), macOS (LLVM), Android/iOS/Windows (Transpiler)
set -e 

GREEN='\033[1;32m'
PURPLE='\033[1;35m'
YELLOW='\033[1;33m'
RED='\033[1;31m'
CYAN='\033[1;36m'
NC='\033[0m'

echo -e "${PURPLE}🧬 AeonCoreX: Initiating X-Phage Build for Target: ${TARGET:-ALL}...${NC}"
echo -e "${PURPLE}   Artifact name: ${ARTIFACT_NAME:-not set}${NC}"

# Clean and create directories
rm -rf bin
mkdir -p bin/linux bin/linux-arm64 bin/windows bin/android bin/macos bin/ios

# --- SOURCE DEFINITIONS ---
SOURCES="src/main.cpp src/lexer.cpp src/llvm_compiler.cpp src/transpiler.cpp src/runtime/xpm_core.cpp src/runtime/memory.cpp src/runtime/linker.cpp src/runtime/core_ops.cpp"
INCLUDES="-I./include"
STANDARD_FLAGS="-std=c++17 -O3 -pthread"

# --- Helper: Try to find LLVM include directory containing llvm/Support/Host.h ---
function find_llvm_include() {
    local conf="$1"
    local version="$2"
    local candidates=(
        "$($conf --includedir)"
        "/usr/include/llvm-${version%%.*}"
        "/usr/lib/llvm-${version%%.*}/include"
        "/opt/homebrew/include"
        "/usr/local/include"
    )
    for dir in "${candidates[@]}"; do
        if [ -f "$dir/llvm/Support/Host.h" ]; then
            echo "$dir"
            return 0
        fi
    done
    return 1
}

# --- Helper: Transpiler mode ---
function compile_transpiler() {
    local PLATFORM=$1
    local OUTPUT=$2
    local FLAGS=$3
    local COMPILER=$4
    echo -e "${CYAN}   -> Building with Titan Transpiler Engine...${NC}"
    $COMPILER $SOURCES $INCLUDES -o $OUTPUT $FLAGS
    echo -e "${GREEN}✔ $PLATFORM (Transpiler Mode) Build Success${NC}"
}

# --- Main compile function with LLVM fallback ---
function compile_smart() {
    local PLATFORM=$1
    local OUTPUT=$2
    local FLAGS=$3
    local COMPILER=$4
    local TRY_LLVM=$5

    if [[ "$TRY_LLVM" == "true" ]]; then
        echo -e "${CYAN}   -> Attempting LLVM Native Core Build...${NC}"
        
        local LLVM_CONF="llvm-config"
        
        if [[ "$PLATFORM" == "macOS" ]]; then
            if [ -f "/opt/homebrew/opt/llvm/bin/llvm-config" ]; then
                LLVM_CONF="/opt/homebrew/opt/llvm/bin/llvm-config"
            elif [ -f "/usr/local/opt/llvm/bin/llvm-config" ]; then
                LLVM_CONF="/usr/local/opt/llvm/bin/llvm-config"
            fi
        fi

        if ! $LLVM_CONF --version &> /dev/null; then
            echo -e "${YELLOW}⚠ LLVM toolchain not found. Using Titan Transpiler.${NC}"
            compile_transpiler "$PLATFORM" "$OUTPUT" "$FLAGS" "$COMPILER"
            return
        fi

        local LLVM_VERSION=$($LLVM_CONF --version)
        local INC_DIR=$(find_llvm_include "$LLVM_CONF" "$LLVM_VERSION")
        
        if [ -z "$INC_DIR" ]; then
            echo -e "${YELLOW}⚠ Could not find llvm/Support/Host.h. Falling back to Transpiler.${NC}"
            compile_transpiler "$PLATFORM" "$OUTPUT" "$FLAGS" "$COMPILER"
            return
        fi

        # Build flags
        local L_CFLAGS="-I$INC_DIR"
        # Add other flags from llvm-config (strip out any -I that might conflict)
        local ADD_CFLAGS=$($LLVM_CONF --cxxflags | sed 's/-I[^ ]*//g')
        L_CFLAGS="$L_CFLAGS $ADD_CFLAGS"
        
        local L_LDFLAGS="$($LLVM_CONF --ldflags) $($LLVM_CONF --libs all) $($LLVM_CONF --system-libs)"
        
        echo -e "${CYAN}      Using LLVM Config: $LLVM_VERSION${NC}"
        echo -e "${CYAN}      Include dir: $INC_DIR${NC}"
        echo -e "${CYAN}      Compiler command: $COMPILER $SOURCES $INCLUDES -o $OUTPUT $FLAGS -DENABLE_LLVM $L_CFLAGS $L_LDFLAGS -ldl${NC}"
        
        set +e 
        $COMPILER $SOURCES $INCLUDES -o $OUTPUT $FLAGS -DENABLE_LLVM $L_CFLAGS $L_LDFLAGS -ldl
        RES=$?
        set -e 

        if [[ $RES -eq 0 ]]; then
            echo -e "${GREEN}✔ $PLATFORM (LLVM Mode) Build Success${NC}"
            return 0
        else
            echo -e "${YELLOW}⚠ LLVM Build Failed. Falling back to Titan Transpiler.${NC}"
        fi
    fi
    
    compile_transpiler "$PLATFORM" "$OUTPUT" "$FLAGS" "$COMPILER"
}

# ---------------------------------------------------------
# 🐧 1. LINUX (x64) - [LLVM ENABLED]
# ---------------------------------------------------------
if [[ "$TARGET" == "linux" || -z "$TARGET" ]]; then
    echo -e "${PURPLE}🐧 Building for Linux (x64)...${NC}"
    OUTPUT="bin/linux/${ARTIFACT_NAME:-xphage_linux_x64}"
    compile_smart "Linux x64" "$OUTPUT" "$STANDARD_FLAGS" "clang++" "true"
fi

# ---------------------------------------------------------
# 🐧 1.5. LINUX (ARM64) - [LLVM ENABLED]
# ---------------------------------------------------------
if [[ "$TARGET" == "linux-arm64" || -z "$TARGET" ]]; then
    echo -e "${PURPLE}🐧 Building for Linux (ARM64)...${NC}"
    OUTPUT="bin/linux-arm64/${ARTIFACT_NAME:-xphage_linux_arm64}"
    
    if command -v clang++ &> /dev/null; then
        compile_smart "Linux ARM64" "$OUTPUT" "$STANDARD_FLAGS" "clang++" "true"
    elif command -v aarch64-linux-gnu-g++ &> /dev/null; then
        compile_smart "Linux ARM64" "$OUTPUT" "$STANDARD_FLAGS" "aarch64-linux-gnu-g++" "true"
    elif [[ $(uname -m) == "aarch64" ]]; then
        compile_smart "Linux ARM64 (Native)" "$OUTPUT" "$STANDARD_FLAGS" "g++" "true"
    else
        echo -e "${YELLOW}⚠ ARM64 compiler not found. Skipping.${NC}"
    fi
fi

# ---------------------------------------------------------
# 🪟 2. WINDOWS (x64) - [TRANSPILER ONLY]
# ---------------------------------------------------------
if [[ "$TARGET" == "windows" || -z "$TARGET" ]]; then
    echo -e "${PURPLE}🪟 Building for Windows (x64)...${NC}"
    OUTPUT="bin/windows/${ARTIFACT_NAME:-xphage.exe}"
    if command -v x86_64-w64-mingw32-g++ &> /dev/null; then
        compile_smart "Windows" "$OUTPUT" "$STANDARD_FLAGS -static-libgcc -static-libstdc++" "x86_64-w64-mingw32-g++" "false"
    fi
fi

# ---------------------------------------------------------
# 🤖 3. ANDROID (ARM64) - [TRANSPILER ONLY]
# ---------------------------------------------------------
if [[ "$TARGET" == "android" || -z "$TARGET" ]]; then
    echo -e "${PURPLE}🤖 Building for Android (ARM64)...${NC}"
    OUTPUT="bin/android/${ARTIFACT_NAME:-xphage_android_arm64}"
    if command -v aarch64-linux-gnu-g++ &> /dev/null; then
        compile_smart "Android" "$OUTPUT" "$STANDARD_FLAGS -pie -fPIE -D__ANDROID__" "aarch64-linux-gnu-g++" "false"
    fi
fi

# ---------------------------------------------------------
# 🍎 4. macOS - [LLVM ENABLED]
# ---------------------------------------------------------
if [[ "$TARGET" == "macos" || -z "$TARGET" ]]; then
    echo -e "${PURPLE}🍎 Building for macOS...${NC}"
    OUTPUT="bin/macos/${ARTIFACT_NAME:-xphage_mac}"
    if [[ "$OSTYPE" == "darwin"* ]]; then
        COMPILER="clang++"
        if [ -f "/opt/homebrew/opt/llvm/bin/clang++" ]; then
            COMPILER="/opt/homebrew/opt/llvm/bin/clang++"
        elif [ -f "/usr/local/opt/llvm/bin/clang++" ]; then
            COMPILER="/usr/local/opt/llvm/bin/clang++"
        fi
        ARCH_FLAG="-arch $(uname -m)"
        compile_smart "macOS" "$OUTPUT" "$STANDARD_FLAGS $ARCH_FLAG" "$COMPILER" "true"
    fi
fi

# ---------------------------------------------------------
# 📱 5. iOS (ARM64) - [TRANSPILER ONLY]
# ---------------------------------------------------------
if [[ "$TARGET" == "ios" || -z "$TARGET" ]]; then
    echo -e "${PURPLE}📱 Building for iOS (ARM64)...${NC}"
    OUTPUT="bin/ios/${ARTIFACT_NAME:-xphage_ios_arm64}"
    if [[ "$OSTYPE" == "darwin"* ]]; then
        SDK_PATH=$(xcrun --sdk iphoneos --show-sdk-path)
        compile_smart "iOS" "$OUTPUT" "-arch arm64 -isysroot $SDK_PATH -miphoneos-version-min=14.0 $STANDARD_FLAGS" "clang++" "false"
    fi
fi