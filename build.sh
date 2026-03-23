#!/bin/bash
# X-Phage Titan Ultimate Build Script v5.1 [LLVM DYNAMIC + SHA256 + WIN ARM64]
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
    # FIXED: Added double quotes around $COMPILER
    "$COMPILER" $SOURCES $INCLUDES -o "$OUTPUT" $FLAGS
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
        
        # macOS: Use Homebrew LLVM if available
        if [[ "$PLATFORM" == *"macOS"* || "$PLATFORM" == *"iOS"* ]]; then
            if [ -f "/opt/homebrew/opt/llvm/bin/llvm-config" ]; then
                LLVM_CONF="/opt/homebrew/opt/llvm/bin/llvm-config"
            elif [ -f "/usr/local/opt/llvm/bin/llvm-config" ]; then
                LLVM_CONF="/usr/local/opt/llvm/bin/llvm-config"
            fi
        fi

        # Windows: Hardcoded Aggressive LLVM Path Discovery
        if [[ "$PLATFORM" == *"Windows"* ]]; then
            # Force the MSYS/Git Bash path for LLVM
            export PATH="/c/Program Files/LLVM/bin:$PATH"
            
            if [ -f "/c/Program Files/LLVM/bin/llvm-config.exe" ]; then
                LLVM_CONF="/c/Program Files/LLVM/bin/llvm-config.exe"
            elif command -v llvm-config.exe &> /dev/null; then
                LLVM_CONF="llvm-config.exe"
            elif command -v llvm-config &> /dev/null; then
                LLVM_CONF="llvm-config"
            fi
            
            # Explicitly force Clang++ compiler path if available to avoid GCC collisions
            if [ -f "/c/Program Files/LLVM/bin/clang++.exe" ]; then
                COMPILER="/c/Program Files/LLVM/bin/clang++.exe"
            fi
        fi

        # FIXED: Added double quotes around $LLVM_CONF
        if ! command -v "$LLVM_CONF" &> /dev/null && [ ! -f "$LLVM_CONF" ]; then
            echo -e "${YELLOW}⚠ LLVM toolchain ($LLVM_CONF) not found. Using Titan Transpiler.${NC}"
            compile_transpiler "$PLATFORM" "$OUTPUT" "$FLAGS" "$COMPILER"
            return
        fi

        # FIXED: Added double quotes around $LLVM_CONF
        local LLVM_VERSION=$("$LLVM_CONF" --version)
        local L_CFLAGS=$("$LLVM_CONF" --cxxflags)
        local L_LDFLAGS=$("$LLVM_CONF" --ldflags)
        local L_LIBS=$("$LLVM_CONF" --libs all)
        local L_SYSLIBS=$("$LLVM_CONF" --system-libs)
        
        # Windows doesn't use -ldl, other OSes usually need it for LLVM JIT/dynamic loading
        local EXTRA_LIBS="-ldl"
        if [[ "$PLATFORM" == *"Windows"* ]]; then
            EXTRA_LIBS=""
        fi

        echo -e "${CYAN}      Using LLVM Config: $LLVM_CONF (version $LLVM_VERSION)${NC}"
        echo -e "${CYAN}      Compiler command execution generated.${NC}"
        
        set +e 
        # FIXED: Added double quotes around $COMPILER
        "$COMPILER" $SOURCES $INCLUDES -o "$OUTPUT" $FLAGS -DENABLE_LLVM $L_CFLAGS $L_LDFLAGS $L_LIBS $L_SYSLIBS $EXTRA_LIBS
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
# 🐧 1. LINUX (x64)
# ---------------------------------------------------------
if [[ "$TARGET" == "linux" || -z "$TARGET" ]]; then
    echo -e "${PURPLE}🐧 Building for Linux (x64)...${NC}"
    OUTPUT="bin/linux/${ARTIFACT_NAME:-xphage_linux_x64}"
    compile_smart "Linux x64" "$OUTPUT" "$STANDARD_FLAGS" "clang++" "true"
    generate_sha256 "$OUTPUT"
fi

# ---------------------------------------------------------
# 🐧 1.5. LINUX (ARM64)
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
# 🪟 2. WINDOWS (x64)
# ---------------------------------------------------------
if [[ "$TARGET" == "windows" || -z "$TARGET" ]]; then
    echo -e "${PURPLE}🪟 Building for Windows (x64)...${NC}"
    OUTPUT="bin/windows/${ARTIFACT_NAME:-xphage.exe}"
    
    if command -v clang++ &> /dev/null || [ -f "/c/Program Files/LLVM/bin/clang++.exe" ]; then
        compile_smart "Windows x64" "$OUTPUT" "$STANDARD_FLAGS" "clang++" "true"
    elif command -v x86_64-w64-mingw32-clang++ &> /dev/null; then
        compile_smart "Windows x64" "$OUTPUT" "$STANDARD_FLAGS -static" "x86_64-w64-mingw32-clang++" "true"
    elif command -v x86_64-w64-mingw32-g++ &> /dev/null; then
        compile_smart "Windows x64 (Transpiler)" "$OUTPUT" "$STANDARD_FLAGS -static-libgcc -static-libstdc++" "x86_64-w64-mingw32-g++" "false"
    fi
    generate_sha256 "$OUTPUT"
fi

# ---------------------------------------------------------
# 🪟 2.5 WINDOWS (ARM64)
# ---------------------------------------------------------
if [[ "$TARGET" == "windows-arm64" || -z "$TARGET" ]]; then
    echo -e "${PURPLE}🪟 Building for Windows (ARM64)...${NC}"
    OUTPUT="bin/windows-arm64/${ARTIFACT_NAME:-xphage_arm64.exe}"
    
    if command -v clang++ &> /dev/null || [ -f "/c/Program Files/LLVM/bin/clang++.exe" ]; then
        compile_smart "Windows ARM64" "$OUTPUT" "$STANDARD_FLAGS --target=aarch64-pc-windows-msvc" "clang++" "true"
    else
        echo -e "${YELLOW}⚠ Clang not found for Windows ARM64. Skipping.${NC}"
    fi
    generate_sha256 "$OUTPUT"
fi

# ---------------------------------------------------------
# 🤖 3. ANDROID (ARM64)
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
# 🍎 4. macOS
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
# 📱 5. iOS (ARM64)
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
