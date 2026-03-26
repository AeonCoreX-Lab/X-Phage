# ============================================================
# 🧬 X-Phage Official Docker Image v3.5.0
#
# Multi-arch: linux/amd64, linux/arm64
# Base:       Ubuntu 24.04 LTS (Noble)
# LLVM:       Auto-detects and installs latest available version
#
# Build:
#   docker build -t xphage:3.5.0 .
#   docker build --platform linux/arm64 -t xphage:3.5.0-arm64 .
#
# Run:
#   docker run --rm xphage:3.5.0 --version
#   docker run --rm -it xphage:3.5.0
#   docker run --rm -v $(pwd):/workspace xphage:3.5.0 run /workspace/main.xp0
# ============================================================

FROM ubuntu:24.04

LABEL org.opencontainers.image.title="X-Phage"
LABEL org.opencontainers.image.description="X-Phage Language Runtime v3.5.0"
LABEL org.opencontainers.image.version="3.5.0"
LABEL org.opencontainers.image.authors="AeonCoreX Lab"
LABEL org.opencontainers.image.source="https://github.com/AeonCoreX-Lab/X-Phage"

# ============================================================
# 1. System dependencies
# ============================================================
ENV DEBIAN_FRONTEND=noninteractive
ENV TZ=UTC

RUN apt-get update && apt-get install -y \
    # Build tools
    build-essential \
    cmake \
    ninja-build \
    # LLVM/Clang — versioned install
    lsb-release \
    wget \
    gnupg \
    software-properties-common \
    # Runtime utilities
    curl \
    git \
    ca-certificates \
    file \
    # SHA256 verification
    coreutils \
    # websocat dependency (for net/http WebSocket)
    openssl \
    && rm -rf /var/lib/apt/lists/*

# ============================================================
# 2. Install latest LLVM + Clang
#    Uses the official LLVM APT repository (llvm.sh script)
#    Automatically picks the latest stable version.
# ============================================================
RUN wget -qO /tmp/llvm.sh https://apt.llvm.org/llvm.sh && \
    chmod +x /tmp/llvm.sh && \
    /tmp/llvm.sh all && \
    rm /tmp/llvm.sh

# Create versioned symlinks → generic names
RUN LLVM_VER=$(ls /usr/bin/llvm-config-* 2>/dev/null \
        | grep -oP '\d+' | sort -n | tail -1) && \
    if [ -n "$LLVM_VER" ]; then \
        ln -sf /usr/bin/llvm-config-${LLVM_VER} /usr/bin/llvm-config && \
        ln -sf /usr/bin/clang++-${LLVM_VER}     /usr/bin/clang++     && \
        ln -sf /usr/bin/clang-${LLVM_VER}        /usr/bin/clang       && \
        ln -sf /usr/bin/lld-${LLVM_VER}          /usr/bin/lld         && \
        echo "LLVM ${LLVM_VER} symlinks created"; \
    else \
        echo "Warning: LLVM not found via llvm.sh, falling back to apt" && \
        apt-get install -y clang llvm lld; \
    fi

# ============================================================
# 3. Directory layout
# ============================================================
WORKDIR /xphage

# ============================================================
# 4. Copy source code
# ============================================================
COPY . .

# ============================================================
# 5. Build X-Phage for the current architecture
#    The build.sh auto-detects TARGET from the environment.
#    On linux/amd64 → builds linux (x64)
#    On linux/arm64 → builds linux-arm64 (native)
# ============================================================
RUN ARCH=$(uname -m) && \
    if [ "$ARCH" = "aarch64" ] || [ "$ARCH" = "arm64" ]; then \
        export TARGET=linux-arm64; \
        export ARTIFACT_NAME=xphage; \
    else \
        export TARGET=linux; \
        export ARTIFACT_NAME=xphage; \
    fi && \
    echo "Building for TARGET=${TARGET} on ${ARCH}" && \
    bash build.sh && \
    echo "Build complete."

# ============================================================
# 6. Install binary to global PATH
# ============================================================
RUN ARCH=$(uname -m) && \
    if [ "$ARCH" = "aarch64" ] || [ "$ARCH" = "arm64" ]; then \
        BIN_PATH="bin/linux-arm64/xphage"; \
    else \
        BIN_PATH="bin/linux/xphage"; \
    fi && \
    if [ -f "$BIN_PATH" ]; then \
        mv "$BIN_PATH" /usr/local/bin/xphage && \
        chmod +x /usr/local/bin/xphage && \
        echo "Installed: /usr/local/bin/xphage"; \
    else \
        echo "Error: binary not found at $BIN_PATH" && exit 1; \
    fi

# ============================================================
# 7. Download stdlib v3.5.0
#    Pre-bakes the standard library into the image so
#    users don't need 'xphage update-stdlib' after pull.
# ============================================================
ENV XP_STDLIB_DIR=/usr/local/share/xphage/stdlib

RUN mkdir -p ${XP_STDLIB_DIR} && \
    BASE="https://raw.githubusercontent.com/AeonCoreX-Lab/X-Phage/main/stdlib" && \
    MODULES=" \
        core/types.xh \
        core/system.xh \
        math/basic.xh \
        math/linalg.xh \
        io/file.xh \
        io/console.xh \
        net/http.xh \
        net/socket.xh \
        data/json.xh \
        data/string.xh \
        media/engine.xh \
        media/stream.xh \
        security/crypt.xh \
        ui/fusion.xh \
        neural/bci.xh \
        neural/lsl.xh \
    " && \
    for mod in $MODULES; do \
        dir="${XP_STDLIB_DIR}/$(dirname $mod)" && \
        mkdir -p "$dir" && \
        curl -sL "${BASE}/${mod}" -o "${XP_STDLIB_DIR}/${mod}" && \
        echo "  ✔ stdlib/${mod}"; \
    done && \
    echo "Stdlib v3.5.0 downloaded."

# Set stdlib path env so xphage can find it globally
ENV XPHAGE_STDLIB=${XP_STDLIB_DIR}

# ============================================================
# 8. Create workspace directory for user files
# ============================================================
RUN mkdir -p /workspace
WORKDIR /workspace

# ============================================================
# 9. Verify installation
# ============================================================
RUN xphage --version

# ============================================================
# 10. Metadata & entrypoint
# ============================================================
EXPOSE 8080

ENTRYPOINT ["xphage"]
CMD ["--version"]

# ============================================================
# Usage examples (in comments for reference):
#
# Interactive REPL:
#   docker run --rm -it xphage:3.5.0
#
# Run a script:
#   docker run --rm -v $(pwd):/workspace xphage:3.5.0 run /workspace/main.xp0
#
# Build a project:
#   docker run --rm -v $(pwd):/workspace xphage:3.5.0 build /workspace/main.xp0
#
# Install packages inside container:
#   docker run --rm -it -v $(pwd):/workspace xphage:3.5.0 install net-http
#
# Override stdlib path:
#   docker run --rm -e XPHAGE_STDLIB=/my/stdlib xphage:3.5.0
# ============================================================
