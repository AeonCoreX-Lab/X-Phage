#!/bin/bash
# X-Phage Titan Ultimate Build Script v5.8 [WIN ARM64 INCLUDE/LIB ENV FIX]
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

# ---------------------------------------------------------
# Windows PATH setup — only inject MSYS2 for x64 target
# ARM64 uses MSVC ABI so MSYS2 MinGW headers would conflict
# ---------------------------------------------------------
IS_WINDOWS=false
if [[ "$RUNNER_OS" == "Windows" || "$OS" == "Windows_NT" ]]; then
    IS_WINDOWS=true
    if [[ "$TARGET" != "windows-arm64" ]]; then
        export PATH="/c/msys64/mingw64/bin:$PATH"
        echo -e "${CYAN}   [WIN] MSYS2 MinGW bin path added to PATH${NC}"
    else
        echo -e "${CYAN}   [WIN] MSYS2 skipped for ARM64 MSVC target${NC}"
    fi

    if command -v llvm-config &>/dev/null && llvm-config --version &>/dev/null 2>&1; then
        echo -e "${CYAN}   [WIN] llvm-config found: $(command -v llvm-config) ($(llvm-config --version))${NC}"
    else
        echo -e "${YELLOW}   [WIN] llvm-config not in PATH${NC}"
    fi

    if command -v clang++ &>/dev/null; then
        echo -e "${CYAN}   [WIN] clang++ found: $(command -v clang++)${NC}"
    fi
fi

# --- Helper: Find llvm-config ---
function find_llvm_config() {
    local PLATFORM=$1

    if [[ "$PLATFORM" == *"Windows"* ]]; then
        if command -v llvm-config &>/dev/null && llvm-config --version &>/dev/null 2>&1; then
            echo "llvm-config"; return
        fi
        if [ -f "/c/msys64/mingw64/bin/llvm-config.exe" ] && \
           "/c/msys64/mingw64/bin/llvm-config.exe" --version &>/dev/null 2>&1; then
            echo "/c/msys64/mingw64/bin/llvm-config.exe"; return
        fi
        echo ""; return
    elif [[ "$PLATFORM" == *"macOS"* || "$PLATFORM" == *"iOS"* ]]; then
        if [ -f "/opt/homebrew/opt/llvm/bin/llvm-config" ]; then
            echo "/opt/homebrew/opt/llvm/bin/llvm-config"; return
        elif [ -f "/usr/local/opt/llvm/bin/llvm-config" ]; then
            echo "/usr/local/opt/llvm/bin/llvm-config"; return
        fi
    fi

    if command -v llvm-config &>/dev/null; then
        echo "llvm-config"; return
    fi
    echo ""
}

# --- Helper: Find clang++ ---
function find_clangpp() {
    local PLATFORM=$1

    if [[ "$PLATFORM" == *"Windows"* ]]; then
        if command -v clang++ &>/dev/null; then
            echo "clang++"; return
        fi
        if [ -f "/c/msys64/mingw64/bin/clang++.exe" ]; then
            echo "/c/msys64/mingw64/bin/clang++.exe"; return
        fi
    elif [[ "$PLATFORM" == *"macOS"* || "$PLATFORM" == *"iOS"* ]]; then
        if [ -f "/opt/homebrew/opt/llvm/bin/clang++" ]; then
            echo "/opt/homebrew/opt/llvm/bin/clang++"; return
        elif [ -f "/usr/local/opt/llvm/bin/clang++" ]; then
            echo "/usr/local/opt/llvm/bin/clang++"; return
        fi
    fi

    if command -v clang++ &>/dev/null; then
        echo "clang++"; return
    fi
    echo ""
}

# --- Helper: Generate SHA256 ---
function generate_sha256() {
    local TARGET_FILE=$1
    if [ -f "$TARGET_FILE" ]; then
        if command -v sha256sum &>/dev/null; then
            sha256sum "$TARGET_FILE" | awk '{print $1}' > "${TARGET_FILE}.sha256"
        elif command -v shasum &>/dev/null; then
            shasum -a 256 "$TARGET_FILE" | awk '{print $1}' > "${TARGET_FILE}.sha256"
        fi
        echo -e "${GREEN}🔒 SHA256 generated: ${TARGET_FILE}.sha256${NC}"
    else
        echo -e "${YELLOW}⚠ SHA256 skipped: binary not found at $TARGET_FILE${NC}"
    fi
}

# --- Helper: Transpiler mode ---
function compile_transpiler() {
    local PLATFORM=$1
    local OUTPUT=$2
    local FLAGS=$3
    local COMPILER=$4
    echo -e "${CYAN}   -> Building with Titan Transpiler Engine...${NC}"
    "$COMPILER" $SOURCES $INCLUDES -o "$OUTPUT" $FLAGS
    echo -e "${GREEN}✔ $PLATFORM (Transpiler Mode) Build Success${NC}"
}

# ---------------------------------------------------------
# Windows LLVM Manual Build (MSYS2 MinGW paths, x64 only)
# ---------------------------------------------------------
function build_windows_llvm_manual() {
    local PLATFORM=$1
    local OUTPUT=$2
    local FLAGS=$3
    local COMPILER=$4

    local LLVM_INCLUDE="/c/msys64/mingw64/include"
    local LLVM_LIB="/c/msys64/mingw64/lib"

    if [ ! -d "$LLVM_INCLUDE" ]; then
        echo -e "${YELLOW}   [WIN] LLVM include dir not found. Falling back to Transpiler.${NC}"
        compile_transpiler "$PLATFORM" "$OUTPUT" "$FLAGS" "$COMPILER"
        return
    fi

    local LLVM_CORE_LIBS=(
        -lLLVMX86CodeGen -lLLVMX86AsmParser -lLLVMX86Desc -lLLVMX86Disassembler -lLLVMX86Info
        -lLLVMAsmPrinter -lLLVMDebugInfoCodeView -lLLVMDebugInfoDWARF -lLLVMCFGuard
        -lLLVMGlobalISel -lLLVMSelectionDAG -lLLVMCodeGen -lLLVMObjCARCOpts -lLLVMipo
        -lLLVMVectorize -lLLVMLinker -lLLVMInstrumentation -lLLVMScalarOpts
        -lLLVMAggressiveInstCombine -lLLVMInstCombine -lLLVMTarget -lLLVMTransformUtils
        -lLLVMAnalysis -lLLVMProfileData -lLLVMObject -lLLVMMCParser -lLLVMMCAsmParser
        -lLLVMMC -lLLVMDebugInfoMSF -lLLVMBitReader -lLLVMAsmParser -lLLVMCore
        -lLLVMRemarks -lLLVMBitstreamReader -lLLVMBinaryFormat -lLLVMTargetParser
        -lLLVMSupport -lLLVMDemangle
    )
    local WIN_SYSLIBS=(-lpsapi -lshell32 -lole32 -luuid -ladvapi32)

    echo -e "${CYAN}   -> Attempting LLVM Manual Build (Windows MinGW)...${NC}"
    set +e
    "$COMPILER" \
        $SOURCES $INCLUDES \
        -I"$LLVM_INCLUDE" \
        -o "$OUTPUT" \
        $FLAGS \
        -DENABLE_LLVM \
        -L"$LLVM_LIB" \
        "${LLVM_CORE_LIBS[@]}" \
        "${WIN_SYSLIBS[@]}" \
        -Wno-unused-command-line-argument
    local RES=$?
    set -e

    if [[ $RES -eq 0 ]]; then
        echo -e "${GREEN}✔ $PLATFORM (LLVM Manual Mode) Build Success${NC}"
        return 0
    else
        echo -e "${YELLOW}   [WIN] LLVM Manual Build failed (exit $RES). Falling back to Transpiler.${NC}"
        compile_transpiler "$PLATFORM" "$OUTPUT" "$FLAGS" "$COMPILER"
    fi
}

# ---------------------------------------------------------
# Windows ARM64 MSVC Build
#
# ROOT FIX v5.8:
# ilammy/msvc-dev-cmd sets INCLUDE and LIB as semicolon-separated
# Windows paths and exports them into the bash environment.
# We read these directly — no need to locate paths manually.
#
# Previous approach used -imsvc which is a clang-cl flag, NOT a
# clang++ flag. clang++ uses -isystem for system include dirs
# and -L for library search paths.
# ---------------------------------------------------------
function build_windows_arm64_msvc() {
    local OUTPUT=$1

    # Find clang++ from choco LLVM (already in PATH via ilammy env)
    local CLANGPP=""
    if command -v clang++ &>/dev/null; then
        CLANGPP="clang++"
    elif [ -f "/c/PROGRA~1/LLVM/bin/clang++.exe" ]; then
        CLANGPP="/c/PROGRA~1/LLVM/bin/clang++.exe"
    else
        echo -e "${RED}✘ clang++ not found for Windows ARM64. Skipping.${NC}"
        return 1
    fi

    echo -e "${CYAN}   [WIN ARM64] Using compiler: $CLANGPP${NC}"

    # Read INCLUDE env var set by ilammy/msvc-dev-cmd
    # Format: "C:\path1;C:\path2;..." — convert to bash -isystem flags
    local INCLUDE_FLAGS=()
    if [ -n "$INCLUDE" ]; then
        echo -e "${CYAN}   [WIN ARM64] Reading INCLUDE from VS environment${NC}"
        # Split on semicolon, convert Windows backslash paths to forward slash
        IFS=';' read -ra WIN_INCLUDES <<< "$INCLUDE"
        for inc in "${WIN_INCLUDES[@]}"; do
            [ -z "$inc" ] && continue
            # Convert Windows path to Git Bash path: C:\foo -> /c/foo
            local bash_inc
            bash_inc=$(echo "$inc" | sed 's|\\|/|g' | sed 's|^\([A-Za-z]\):|/\L\1|')
            INCLUDE_FLAGS+=(-isystem "$bash_inc")
        done
    else
        echo -e "${YELLOW}   [WIN ARM64] INCLUDE env var not set — VS environment may not be active${NC}"
    fi

    # Read LIB env var set by ilammy/msvc-dev-cmd
    # Format: "C:\path1;C:\path2;..." — convert to -L flags
    local LIB_FLAGS=()
    if [ -n "$LIB" ]; then
        echo -e "${CYAN}   [WIN ARM64] Reading LIB from VS environment${NC}"
        IFS=';' read -ra WIN_LIBS <<< "$LIB"
        for lib in "${WIN_LIBS[@]}"; do
            [ -z "$lib" ] && continue
            local bash_lib
            bash_lib=$(echo "$lib" | sed 's|\\|/|g' | sed 's|^\([A-Za-z]\):|/\L\1|')
            LIB_FLAGS+=(-L"$bash_lib")
        done
    else
        echo -e "${YELLOW}   [WIN ARM64] LIB env var not set — VS environment may not be active${NC}"
    fi

    echo -e "${CYAN}   -> Attempting Windows ARM64 MSVC Build...${NC}"
    set +e
    "$CLANGPP" \
        $SOURCES \
        $INCLUDES \
        "${INCLUDE_FLAGS[@]}" \
        -o "$OUTPUT" \
        -std=c++17 -O3 \
        --target=aarch64-pc-windows-msvc \
        "${LIB_FLAGS[@]}" \
        -Wno-unused-command-line-argument
    local RES=$?
    set -e

    if [[ $RES -eq 0 ]]; then
        echo -e "${GREEN}✔ Windows ARM64 (MSVC Mode) Build Success${NC}"
        return 0
    else
        echo -e "${YELLOW}   [WIN ARM64] MSVC Build failed (exit $RES). Falling back to Transpiler.${NC}"
        compile_transpiler "Windows ARM64" "$OUTPUT" "$STANDARD_FLAGS" "$CLANGPP"
    fi
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

        local LLVM_CONF
        LLVM_CONF=$(find_llvm_config "$PLATFORM")

        if [[ "$PLATFORM" == *"Windows"* || "$PLATFORM" == *"macOS"* || "$PLATFORM" == *"iOS"* ]]; then
            local DETECTED_CXX
            DETECTED_CXX=$(find_clangpp "$PLATFORM")
            if [ -n "$DETECTED_CXX" ]; then
                COMPILER="$DETECTED_CXX"
            fi
        fi

        if [[ "$PLATFORM" == *"Windows"* && -z "$LLVM_CONF" ]]; then
            echo -e "${YELLOW}   [WIN] llvm-config not usable. Using manual LLVM flags with MinGW.${NC}"
            build_windows_llvm_manual "$PLATFORM" "$OUTPUT" "$FLAGS" "$COMPILER"
            return
        fi

        if [ -z "$LLVM_CONF" ]; then
            echo -e "${YELLOW}⚠ LLVM toolchain not found. Using Titan Transpiler.${NC}"
            compile_transpiler "$PLATFORM" "$OUTPUT" "$FLAGS" "$COMPILER"
            return
        fi

        local LLVM_VERSION
        LLVM_VERSION=$("$LLVM_CONF" --version 2>/dev/null || echo "unknown")
        echo -e "${CYAN}      Using LLVM Config: $LLVM_CONF (version $LLVM_VERSION)${NC}"

        local L_CFLAGS L_LDFLAGS L_LIBS L_SYSLIBS
        L_CFLAGS=$("$LLVM_CONF" --cxxflags 2>/dev/null || echo "")
        L_LDFLAGS=$("$LLVM_CONF" --ldflags 2>/dev/null || echo "")
        L_LIBS=$("$LLVM_CONF" --libs all 2>/dev/null || echo "")

        if [[ "$PLATFORM" == *"Windows"* ]]; then
            L_SYSLIBS=""
        else
            L_SYSLIBS=$("$LLVM_CONF" --system-libs 2>/dev/null || echo "")
        fi

        local EXTRA_LIBS="-ldl"
        if [[ "$PLATFORM" == *"Windows"* ]]; then
            EXTRA_LIBS=""
        fi

        local CMD=("$COMPILER")
        # shellcheck disable=SC2206
        CMD+=($SOURCES $INCLUDES -o "$OUTPUT" $FLAGS -DENABLE_LLVM)
        [ -n "$L_CFLAGS"   ] && CMD+=($L_CFLAGS)
        [ -n "$L_LDFLAGS"  ] && CMD+=($L_LDFLAGS)
        [ -n "$L_LIBS"     ] && CMD+=($L_LIBS)
        [ -n "$L_SYSLIBS"  ] && CMD+=($L_SYSLIBS)
        [ -n "$EXTRA_LIBS" ] && CMD+=($EXTRA_LIBS)

        set +e
        "${CMD[@]}"
        RES=$?
        set -e

        if [[ $RES -eq 0 ]]; then
            echo -e "${GREEN}✔ $PLATFORM (LLVM Mode) Build Success${NC}"
            return 0
        else
            echo -e "${YELLOW}⚠ LLVM Build Failed (exit $RES). Falling back to Titan Transpiler.${NC}"
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

    # Use aarch64-linux-gnu-g++ directly — has its own ARM64 sysroot.
    # LLVM mode disabled: host LLVM libs are x64, cannot link into ARM64 binary.
    if command -v aarch64-linux-gnu-g++ &>/dev/null; then
        echo -e "${CYAN}   -> Using aarch64-linux-gnu-g++ cross-compiler${NC}"
        compile_smart "Linux ARM64" "$OUTPUT" "$STANDARD_FLAGS" "aarch64-linux-gnu-g++" "false"
    elif [[ $(uname -m) == "aarch64" ]]; then
        compile_smart "Linux ARM64 (Native)" "$OUTPUT" "$STANDARD_FLAGS" "g++" "true"
    else
        echo -e "${YELLOW}⚠ aarch64-linux-gnu-g++ not found. Skipping Linux ARM64.${NC}"
    fi
    generate_sha256 "$OUTPUT"
fi

# ---------------------------------------------------------
# 🪟 2. WINDOWS (x64)
# ---------------------------------------------------------
if [[ "$TARGET" == "windows" || -z "$TARGET" ]]; then
    echo -e "${PURPLE}🪟 Building for Windows (x64)...${NC}"
    OUTPUT="bin/windows/${ARTIFACT_NAME:-xphage.exe}"

    WIN_CXX=$(find_clangpp "Windows x64")
    if [ -n "$WIN_CXX" ]; then
        compile_smart "Windows x64" "$OUTPUT" "$STANDARD_FLAGS" "$WIN_CXX" "true"
    elif command -v x86_64-w64-mingw32-clang++ &>/dev/null; then
        compile_smart "Windows x64" "$OUTPUT" "$STANDARD_FLAGS -static" "x86_64-w64-mingw32-clang++" "true"
    elif command -v x86_64-w64-mingw32-g++ &>/dev/null; then
        compile_smart "Windows x64 (Transpiler)" "$OUTPUT" "$STANDARD_FLAGS -static-libgcc -static-libstdc++" "x86_64-w64-mingw32-g++" "false"
    else
        echo -e "${RED}✘ No suitable compiler found for Windows x64. Skipping.${NC}"
    fi
    generate_sha256 "$OUTPUT"
fi

# ---------------------------------------------------------
# 🪟 2.5. WINDOWS (ARM64)
# ---------------------------------------------------------
if [[ "$TARGET" == "windows-arm64" || -z "$TARGET" ]]; then
    echo -e "${PURPLE}🪟 Building for Windows (ARM64)...${NC}"
    OUTPUT="bin/windows-arm64/${ARTIFACT_NAME:-xphage_arm64.exe}"
    build_windows_arm64_msvc "$OUTPUT"
    generate_sha256 "$OUTPUT"
fi

# ---------------------------------------------------------
# 🤖 3. ANDROID (ARM64)
# ---------------------------------------------------------
if [[ "$TARGET" == "android" || -z "$TARGET" ]]; then
    echo -e "${PURPLE}🤖 Building for Android (ARM64)...${NC}"
    OUTPUT="bin/android/${ARTIFACT_NAME:-xphage_android_arm64}"

    if command -v clang++ &>/dev/null; then
        compile_smart "Android" "$OUTPUT" "$STANDARD_FLAGS -pie -fPIE -D__ANDROID__" "clang++" "true"
    elif command -v aarch64-linux-gnu-g++ &>/dev/null; then
        compile_smart "Android (Transpiler)" "$OUTPUT" "$STANDARD_FLAGS -pie -fPIE -D__ANDROID__" "aarch64-linux-gnu-g++" "false"
    else
        echo -e "${YELLOW}⚠ No suitable compiler for Android. Skipping.${NC}"
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
        MACOS_CXX=$(find_clangpp "macOS")
        MACOS_CXX="${MACOS_CXX:-clang++}"
        ARCH_FLAG="-arch $(uname -m)"
        compile_smart "macOS" "$OUTPUT" "$STANDARD_FLAGS $ARCH_FLAG" "$MACOS_CXX" "true"
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
