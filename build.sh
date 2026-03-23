#!/bin/bash
# X-Phage Titan Ultimate Build Script v5.5 [WINDOWS LLVM FIX]
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
# ROOT FIX v5.5: Windows PATH — use 8.3 short paths (no spaces)
#
# Problem 1: Git Bash converts "C:\Program Files\LLVM\bin" to
# "/c/Program Files/LLVM/bin". The space in "Program Files" makes
# `command -v` skip that directory entirely when searching PATH.
#
# Problem 2: llvm-config.exe installed by Chocolatey requires MSVC
# runtime. Without Visual Studio, it crashes silently. So we CANNOT
# rely on llvm-config on GitHub-hosted Windows runners.
#
# Fix: On Windows, bypass llvm-config entirely. Use clang++ directly
# with MinGW runtime libraries. Pass LLVM include/lib paths manually
# using well-known Chocolatey install locations.
# ---------------------------------------------------------
IS_WINDOWS=false
if [[ "$RUNNER_OS" == "Windows" || "$OS" == "Windows_NT" ]]; then
    IS_WINDOWS=true
    export PATH="/c/PROGRA~1/LLVM/bin:/c/ProgramData/chocolatey/lib/llvm/tools/llvm/bin:/c/ProgramData/mingw64/mingw64/bin:$PATH"
    echo -e "${CYAN}   [WIN] LLVM + MinGW bin paths added to PATH (short path)${NC}"

    if command -v llvm-config &>/dev/null; then
        # Test if llvm-config actually runs (needs MSVC — usually fails)
        if llvm-config --version &>/dev/null 2>&1; then
            echo -e "${CYAN}   [WIN] llvm-config found and functional: $(command -v llvm-config)${NC}"
        else
            echo -e "${YELLOW}   [WIN] llvm-config found but NOT executable (MSVC missing). Will use manual flags.${NC}"
        fi
    else
        echo -e "${YELLOW}   [WIN] llvm-config not in PATH. Will use manual LLVM flags.${NC}"
    fi

    if command -v clang++ &>/dev/null; then
        echo -e "${CYAN}   [WIN] clang++ found: $(command -v clang++)${NC}"
    fi
fi

# --- Helper: Find llvm-config ---
function find_llvm_config() {
    local PLATFORM=$1

    if [[ "$PLATFORM" == *"Windows"* ]]; then
        # On Windows: only trust llvm-config if it actually executes successfully.
        # Chocolatey's llvm-config.exe needs MSVC and crashes without it.
        if command -v llvm-config &>/dev/null; then
            if llvm-config --version &>/dev/null 2>&1; then
                echo "llvm-config"; return
            fi
            # llvm-config exists but can't run — check the .exe directly
            if [ -f "/c/PROGRA~1/LLVM/bin/llvm-config.exe" ]; then
                if "/c/PROGRA~1/LLVM/bin/llvm-config.exe" --version &>/dev/null 2>&1; then
                    echo "/c/PROGRA~1/LLVM/bin/llvm-config.exe"; return
                fi
            fi
        fi
        # llvm-config is not usable on this runner — return empty
        # build_windows_llvm_manual() will handle the manual path approach
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
        if [ -f "/c/PROGRA~1/LLVM/bin/clang++.exe" ]; then
            echo "/c/PROGRA~1/LLVM/bin/clang++.exe"; return
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
# NEW v5.5: Windows LLVM Manual Build
#
# When llvm-config.exe is broken (no MSVC), we build with LLVM
# headers + MinGW libs directly. Chocolatey installs LLVM to a
# predictable location so we hard-code the paths using 8.3 format.
#
# Steps:
#  1. Find LLVM include dir from Chocolatey install
#  2. Find LLVM lib dir
#  3. Detect LLVM version from llvm/Config/llvm-config.h
#  4. Compile with -DENABLE_LLVM and manual -I / -L / -l flags
# ---------------------------------------------------------
function build_windows_llvm_manual() {
    local PLATFORM=$1
    local OUTPUT=$2
    local FLAGS=$3
    local COMPILER=$4

    # Well-known Chocolatey LLVM install paths (8.3 short form, no spaces)
    local LLVM_INCLUDE_SHORT="/c/PROGRA~1/LLVM/include"
    local LLVM_LIB_SHORT="/c/PROGRA~1/LLVM/lib"
    local LLVM_INCLUDE_LONG="/c/Program Files/LLVM/include"
    local LLVM_LIB_LONG="/c/Program Files/LLVM/lib"

    local LLVM_INCLUDE=""
    local LLVM_LIB=""

    # Prefer short path; fall back to quoted long path
    if [ -d "$LLVM_INCLUDE_SHORT" ]; then
        LLVM_INCLUDE="$LLVM_INCLUDE_SHORT"
        LLVM_LIB="$LLVM_LIB_SHORT"
    elif [ -d "$LLVM_INCLUDE_LONG" ]; then
        LLVM_INCLUDE="$LLVM_INCLUDE_LONG"
        LLVM_LIB="$LLVM_LIB_LONG"
    else
        echo -e "${YELLOW}   [WIN] LLVM include dir not found at expected paths. Falling back to Transpiler.${NC}"
        compile_transpiler "$PLATFORM" "$OUTPUT" "$FLAGS" "$COMPILER"
        return
    fi

    echo -e "${CYAN}   [WIN] LLVM include: $LLVM_INCLUDE${NC}"
    echo -e "${CYAN}   [WIN] LLVM lib:     $LLVM_LIB${NC}"

    # Detect LLVM version from the installed header
    local LLVM_VER_HEADER="$LLVM_INCLUDE/llvm/Config/llvm-config.h"
    local LLVM_VERSION="unknown"
    if [ -f "$LLVM_VER_HEADER" ]; then
        LLVM_VERSION=$(grep -m1 'LLVM_VERSION_STRING' "$LLVM_VER_HEADER" | grep -oP '"\K[^"]+')
    fi
    echo -e "${CYAN}   [WIN] Detected LLVM version: $LLVM_VERSION${NC}"

    # Core LLVM libs needed for IR + target emission (static, MinGW-compatible)
    # Order matters for static linking — dependencies must come after dependents.
    local LLVM_CORE_LIBS=(
        -lLLVMX86CodeGen
        -lLLVMX86AsmParser
        -lLLVMX86Desc
        -lLLVMX86Disassembler
        -lLLVMX86Info
        -lLLVMAsmPrinter
        -lLLVMDebugInfoCodeView
        -lLLVMDebugInfoDWARF
        -lLLVMCFGuard
        -lLLVMGlobalISel
        -lLLVMSelectionDAG
        -lLLVMCodeGen
        -lLLVMObjCARCOpts
        -lLLVMipo
        -lLLVMVectorize
        -lLLVMLinker
        -lLLVMInstrumentation
        -lLLVMScalarOpts
        -lLLVMAggressiveInstCombine
        -lLLVMInstCombine
        -lLLVMTarget
        -lLLVMTransformUtils
        -lLLVMAnalysis
        -lLLVMProfileData
        -lLLVMObject
        -lLLVMMCParser
        -lLLVMMCAsmParser
        -lLLVMMC
        -lLLVMDebugInfoMSF
        -lLLVMBitReader
        -lLLVMAsmParser
        -lLLVMCore
        -lLLVMRemarks
        -lLLVMBitstreamReader
        -lLLVMBinaryFormat
        -lLLVMTargetParser
        -lLLVMSupport
        -lLLVMDemangle
    )

    # Windows system libs required by LLVM
    local WIN_SYSLIBS=(-lpsapi -lshell32 -lole32 -luuid -ladvapi32)

    echo -e "${CYAN}   -> Attempting LLVM Manual Build (Windows, MinGW)...${NC}"

    set +e
    "$COMPILER" \
        $SOURCES \
        $INCLUDES \
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
        echo -e "${YELLOW}   [WIN] LLVM Manual Build failed (exit $RES). Falling back to Titan Transpiler.${NC}"
        compile_transpiler "$PLATFORM" "$OUTPUT" "$FLAGS" "$COMPILER"
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

        # Re-detect compiler for Windows/macOS
        if [[ "$PLATFORM" == *"Windows"* || "$PLATFORM" == *"macOS"* || "$PLATFORM" == *"iOS"* ]]; then
            local DETECTED_CXX
            DETECTED_CXX=$(find_clangpp "$PLATFORM")
            if [ -n "$DETECTED_CXX" ]; then
                COMPILER="$DETECTED_CXX"
            fi
        fi

        # --- Windows special path: llvm-config broken → use manual build ---
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

        # --system-libs errors out on Windows — skip it
        if [[ "$PLATFORM" == *"Windows"* ]]; then
            L_SYSLIBS=""
        else
            L_SYSLIBS=$("$LLVM_CONF" --system-libs 2>/dev/null || echo "")
        fi

        # -ldl not available on Windows
        local EXTRA_LIBS="-ldl"
        if [[ "$PLATFORM" == *"Windows"* ]]; then
            EXTRA_LIBS=""
        fi

        # Use bash array — handles any remaining spaces in compiler path
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

    if command -v clang++ &>/dev/null; then
        compile_smart "Linux ARM64" "$OUTPUT" "$STANDARD_FLAGS --target=aarch64-linux-gnu" "clang++" "true"
    elif command -v aarch64-linux-gnu-g++ &>/dev/null; then
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

    WIN_CXX=$(find_clangpp "Windows ARM64")
    if [ -n "$WIN_CXX" ]; then
        compile_smart "Windows ARM64" "$OUTPUT" "$STANDARD_FLAGS --target=aarch64-pc-windows-msvc" "$WIN_CXX" "true"
    elif command -v clang++ &>/dev/null; then
        compile_smart "Windows ARM64" "$OUTPUT" "$STANDARD_FLAGS --target=aarch64-pc-windows-msvc" "clang++" "true"
    else
        echo -e "${RED}✘ Clang not found for Windows ARM64. Skipping.${NC}"
    fi
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
