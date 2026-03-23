#!/bin/bash
# X-Phage Titan Ultimate Build Script v5.0 [LLVM DYNAMIC + SHA256 + WIN ARM64]
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
mkdir -p bin/linux bin/linux-arm64 bin/windows bin/windows-arm64 bin/android bin/macos bin/ios

# --- SOURCE DEFINITIONS ---
SOURCES="src/main.cpp src/lexer.cpp src/llvm_compiler.cpp src/transpiler.cpp src/runtime/xpm_core.cpp src/runtime/memory.cpp src/runtime/linker.cpp src/runtime/core_ops.cpp"
INCLUDES="-I./include"
STANDARD_FLAGS="-std=c++17 -O3 -pthread"

# --- Helper: Generate SHA256 Checksum ---
function generate_sha256() {
    local TARGET_FILE=$1
    if [ -f "$TARGET_FILE" ]; then
        if command -v sha256sum &> /dev/null; then
            sha256sum "$TARGET_FILE" | awk '{print $1}' > "${TARGET_FILE}.sha256"
        elif command -v shasum &> /dev/null; then
            shasum -a 256 "$TARGET_FILE" | awk '{print $1}' > "${TARGET_FILE}.sha256"
        fi
        echo -e "${GREEN}🔒 SHA256 generated: ${TARGET_FILE}.sha256${NC}"
    fi
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

# --- Helper: Find correct LLVM include path (Fix for LLVM 17+) ---
function get_llvm_include_path() {
    local conf="$1"
    local version="$2"
    
    # Try the path from llvm-config first
    local inc=$($conf --includedir 2>/dev/null || echo "")
    if [ -n "$inc" ] && { [ -f "$inc/llvm/Support/Host.h" ] || [ -f "$inc/llvm/TargetParser/Host.h" ]; }; then
        echo "$inc"
        return 0
    fi
    
    # Otherwise search common locations (including Homebrew and Linux standard paths)
    local candidates=(
        "/usr/include/llvm-${version%%.*}"
        "/usr/lib/llvm-${version%%.*}/include"
        "/opt/homebrew/opt/llvm/include"
        "/usr/local/opt/llvm/include"
        "/usr/local/include"
        "/usr/include/llvm"
        "C:/Program Files/LLVM/include"
    )
    
    for dir in "${candidates[@]}"; do
        if [ -f "$dir/llvm/Support/Host.h" ] || [ -f "$dir/llvm/TargetParser/Host.h" ]; then
            echo "$dir"
            return 0
        fi
    done
    return 1
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
        
        # macOS: Use Homebrew LLVM if available
        if [[ "$PLATFORM" == "macOS" || "$PLATFORM" == "iOS" ]]; then
            if [ -f "/opt/homebrew/opt/llvm/bin/llvm-config" ]; then
                LLVM_CONF="/opt/homebrew/opt/llvm/bin/llvm-config"
            elif [ -f "/usr/local/opt/llvm/bin/llvm-config" ]; then
                LLVM_CONF="/usr/local/opt/llvm/bin/llvm-config"
            fi
        fi

        if ! command -v $LLVM_CONF &> /dev/null; then
            echo -e "${YELLOW}⚠ LLVM toolchain (llvm-config) not found. Using Titan Transpiler.${NC}"
            compile_transpiler "$PLATFORM" "$OUTPUT" "$FLAGS" "$COMPILER"
            return
        fi

        local LLVM_VERSION=$($LLVM_CONF --version)
        local INC_DIR=$(get_llvm_include_path "$LLVM_CONF" "$LLVM_VERSION")
        
        if [ -z "$INC_DIR" ]; then
            echo -e "${YELLOW}⚠ Could not find Host.h headers. Falling back to Transpiler.${NC}"
            compile_transpiler "$PLATFORM" "$OUTPUT" "$FLAGS" "$COMPILER"
            return
        fi

        # Get flags from llvm-config, replace include path with discovered directory
        local RAW_CFLAGS=$($LLVM_CONF --cxxflags)
        local CLEAN_CFLAGS=$(echo "$RAW_CFLAGS" | sed 's/-I[^ ]*//g')
        local L_CFLAGS="-I\"$INC_DIR\" $CLEAN_CFLAGS"
        local L_LDFLAGS="$($LLVM_CONF --ldflags) $($LLVM_CONF --libs all) $($LLVM_CONF --system-libs)"
        
        # In Windows Git Bash, paths might contain spaces, avoid quoting errors
        echo -e "${CYAN}      Using LLVM Config: $LLVM_CONF (version $LLVM_VERSION)${NC}"
        echo -e "${CYAN}      Compiler command execution generated.${NC}"
        
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
    generate_sha256 "$OUTPUT"
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
    generate_sha256 "$OUTPUT"
fi

# ---------------------------------------------------------
# 🪟 2. WINDOWS (x64) - [LLVM ENABLED]
# ---------------------------------------------------------
if [[ "$TARGET" == "windows" || -z "$TARGET" ]]; then
    echo -e "${PURPLE}🪟 Building for Windows (x64)...${NC}"
    OUTPUT="bin/windows/${ARTIFACT_NAME:-xphage.exe}"
    
    # Try Clang for LLVM support on Windows runner
    if command -v clang++ &> /dev/null; then
        compile_smart "Windows x64" "$OUTPUT" "$STANDARD_FLAGS" "clang++" "true"
    elif command -v x86_64-w64-mingw32-clang++ &> /dev/null; then
        compile_smart "Windows x64" "$OUTPUT" "$STANDARD_FLAGS -static" "x86_64-w64-mingw32-clang++" "true"
    elif command -v x86_64-w64-mingw32-g++ &> /dev/null; then
        compile_smart "Windows x64 (Transpiler)" "$OUTPUT" "$STANDARD_FLAGS -static-libgcc -static-libstdc++" "x86_64-w64-mingw32-g++" "false"
    fi
    generate_sha256 "$OUTPUT"
fi

# ---------------------------------------------------------
# 🪟 2.5 WINDOWS (ARM64) - [LLVM ENABLED]
# ---------------------------------------------------------
if [[ "$TARGET" == "windows-arm64" || -z "$TARGET" ]]; then
    echo -e "${PURPLE}🪟 Building for Windows (ARM64)...${NC}"
    OUTPUT="bin/windows-arm64/${ARTIFACT_NAME:-xphage_arm64.exe}"
    
    if command -v clang++ &> /dev/null; then
        # Use Clang to cross-compile for Windows ARM64
        compile_smart "Windows ARM64" "$OUTPUT" "$STANDARD_FLAGS --target=aarch64-pc-windows-msvc" "clang++" "true"
    else
        echo -e "${YELLOW}⚠ Clang not found for Windows ARM64. Skipping.${NC}"
    fi
    generate_sha256 "$OUTPUT"
fi

# ---------------------------------------------------------
# 🤖 3. ANDROID (ARM64) - [LLVM ENABLED]
# ---------------------------------------------------------
if [[ "$TARGET" == "android" || -z "$TARGET" ]]; then
    echo -e "${PURPLE}🤖 Building for Android (ARM64)...${NC}"
    OUTPUT="bin/android/${ARTIFACT_NAME:-xphage_android_arm64}"
    
    if command -v clang++ &> /dev/null; then
        compile_smart "Android" "$OUTPUT" "$STANDARD_FLAGS -pie -fPIE -D__ANDROID__" "clang++" "true"
    elif command -v aarch64-linux-gnu-g++ &> /dev/null; then
        compile_smart "Android (Transpiler)" "$OUTPUT" "$STANDARD_FLAGS -pie -fPIE -D__ANDROID__" "aarch64-linux-gnu-g++" "false"
    fi
    generate_sha256 "$OUTPUT"
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
        generate_sha256 "$OUTPUT"
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
        generate_sha256 "$OUTPUT"
    fi
fi
