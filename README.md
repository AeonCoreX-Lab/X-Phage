<div align="center">

```
  _  _  ____  __  __
 ( \/ )(  _ \(  \/  )
  )  (  )___/ )    (
 (_/\_)(__)  (_/\/_) v3.5.0
```

# X-Phage Language

**Production-grade, LLVM-powered programming language**
*Neural interfaces · Cross-platform · Built-in package manager*

[![Build](https://github.com/AeonCoreX-Lab/X-Phage/actions/workflows/release-generator.yml/badge.svg)](https://github.com/AeonCoreX-Lab/X-Phage/actions)
[![Release](https://img.shields.io/github/v/release/AeonCoreX-Lab/X-Phage)](https://github.com/AeonCoreX-Lab/X-Phage/releases/latest)
[![License](https://img.shields.io/badge/license-MIT-blue)](LICENSE)

</div>

---

## Install

```bash
curl -sL https://raw.githubusercontent.com/AeonCoreX-Lab/X-Phage/main/scripts/install.sh | bash
```

**Supported platforms:** Linux x64/ARM64 · macOS Universal · Windows x64/ARM64 · Android (Termux)

---

## Quick Start

```xp0
~link io/console
~link net/http

pulse main {
    log_info("Hello from X-Phage v3.5.0!")

    atom res = http_get("https://api.github.com")
    beam res.status
}
```

```bash
xphage run main.xp0          # Run directly
xphage build main.xp0        # Compile to native binary
xphage --version             # Show version
```

---

## Repository Structure

```
X-Phage/
│
├── compiler/                    # Compiler pipeline (rustc-style)
│   ├── xphage_driver/           # CLI entry point & arg parsing
│   ├── xphage_lexer/            # Tokenisation
│   ├── xphage_parse/            # Parser → AST
│   ├── xphage_ast/              # AST node definitions
│   ├── xphage_middle/           # IR lowering & type system
│   ├── xphage_codegen_llvm/     # LLVM native backend
│   ├── xphage_codegen_transpiler/ # C++ transpiler backend
│   ├── xphage_linker/           # Symbol linking & module resolution
│   └── xphage_interface/        # Public compiler API (full pipeline)
│
├── library/                     # Standard library
│   ├── core/xh/                 # No-alloc core (types, system)
│   ├── alloc/xh/                # Memory allocation layer
│   └── std/xh/                  # Full standard library
│       ├── io/      console, file
│       ├── net/     http, socket
│       ├── data/    json, string
│       ├── math/    basic, linalg
│       ├── media/   engine, stream
│       ├── security/ crypt
│       ├── ui/      fusion
│       └── neural/  bci, lsl     ← NEW in v3.5.0
│
├── src/
│   ├── runtime/                 # Runtime engine (core_ops, memory)
│   └── tools/
│       └── xphage-fmt/          # Code formatter
│
├── tools/                       # Dev tooling
│   ├── xphage-lsp/              # Language Server (VS Code / Neovim)
│   ├── xphage-doc/              # Documentation generator
│   └── xphage-test/             # Test runner
│
├── tests/
│   ├── run-pass/                # Programs that must compile & run
│   └── compile-fail/            # Programs that must fail correctly
│
├── examples/                    # Example .xp0 programs
├── docs/                        # Language reference & guide
├── scripts/                     # build.sh, install.sh, release.sh
├── .github/workflows/           # CI/CD
└── CMakeLists.txt               # CMake build system
```

---

## Building from Source

**Requirements:** CMake ≥ 3.20 · Clang/GCC C++17 · LLVM ≥ 16 (optional)

```bash
git clone https://github.com/AeonCoreX-Lab/X-Phage
cd X-Phage

# Quick build (Release + LLVM auto-detect)
bash scripts/build.sh

# Debug build with tests
BUILD_TYPE=Debug RUN_TESTS=1 bash scripts/build.sh

# Without LLVM (transpiler only)
ENABLE_LLVM=OFF bash scripts/build.sh

# CMake directly
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

---

## Package Manager (XPM)

XPM is a **separate standalone tool** — install it independently:

```bash
curl -sL https://raw.githubusercontent.com/AeonCoreX-Lab/XPM/main/scripts/install.sh | bash
```

```bash
xpm init                    # Create project + xphage.pkg
xpm install net-http        # Install latest
xpm install data-json@^2.0  # Semver range
xpm update                  # Update all packages
xpm publish                 # Publish (fully automatic — no gh CLI needed)
xpm lock                    # Regenerate lockfile
xpm check                   # Verify SHA256 integrity
xpm search neural           # Search registry
xpm cache list              # Show local cache (~/.xpm/cache)
```

| Repo | Purpose |
|------|---------|
| [AeonCoreX-Lab/XPM](https://github.com/AeonCoreX-Lab/XPM) | Package manager tool |
| [AeonCoreX-Lab/xpm-registry](https://github.com/AeonCoreX-Lab/xpm-registry) | Package registry (serverless, TOML index) |

---

## Standard Library

| Module | Import | Description |
|--------|--------|-------------|
| `core/types` | `~link core/types` | Type system, constants, casting |
| `core/system` | `~link core/system` | OS, process, env vars, exec |
| `io/file` | `~link io/file` | File I/O, dir ops, watcher |
| `io/console` | `~link io/console` | Logging, prompts, progress, table |
| `net/http` | `~link net/http` | HTTP client + WebSocket |
| `net/socket` | `~link net/socket` | TCP/UDP/TLS/P2P/torrent |
| `data/json` | `~link data/json` | Parse, stringify, path query |
| `data/string` | `~link data/string` | Manipulation, encode, similarity |
| `math/basic` | `~link math/basic` | Trig, random, stats, bitwise |
| `math/linalg` | `~link math/linalg` | Matrix, vector, tensor, GPU |
| `media/engine` | `~link media/engine` | MPV player, playlist, audio EQ |
| `media/stream` | `~link media/stream` | HLS/DASH, recording, transcode |
| `security/crypt` | `~link security/crypt` | AES, SHA-3, RSA, Argon2, TOTP |
| `ui/fusion` | `~link ui/fusion` | Vulkan UI components |
| **`neural/bci`** | `~link neural/bci` | **OpenBCI + Neurosity Crown** |
| **`neural/lsl`** | `~link neural/lsl` | **LSL protocol (200+ devices)** |

---

## Dev Tools

| Tool | Command | Description |
|------|---------|-------------|
| `xphage-lsp` | auto (via editor) | LSP: completion, hover, diagnostics |
| `xphage-doc` | `xphage-doc library/ --out docs/` | Generate API docs (MD + HTML) |
| `xphage-test` | `xphage-test --jobs 8` | Run test suite in parallel |
| `xphage-fmt` | `xphage-fmt src/main.xp0` | Auto-format source files |

**VS Code:** Add to `.vscode/settings.json`:
```json
{
  "xphage.lsp.path": "./build/xphage-lsp"
}
```

---

## Docker

```bash
docker pull aeoncorex/xphage:latest
docker run --rm -it aeoncorex/xphage
docker run --rm -v $(pwd):/workspace aeoncorex/xphage run /workspace/main.xp0
```

---

## Language Reference

| Keyword | Description |
|---------|-------------|
| `pulse` | Declare a function/block |
| `atom` | Immutable variable |
| `shadow` | Mutable variable |
| `global` | Global registry variable |
| `beam` | Print to stdout |
| `bypass` | Hardware/kernel injection |
| `quantum` | Spawn async thread |
| `vortex` | Clear local memory |
| `void` | Full memory wipe (VOID Protocol) |
| `chronos` | Sleep / time dilation (ms) |
| `ether` | Cloud/network uplink |
| `synapse` | Neural API handshake |
| `matrix` | GPU matrix allocation |
| `scan` | Inspect / type-check value |
| `~link` | Import stdlib or module |
| `fusion` | Titan UI composition |

---

## License

MIT — © AeonCoreX Lab
