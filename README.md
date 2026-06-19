<div align="center">

<img src="https://raw.githubusercontent.com/xphage-lang/.github/main/profile/banner.png" alt="XPhage Banner" width="100%">

<br>

```
██╗  ██╗██████╗ ██╗  ██╗ █████╗  ██████╗ ███████╗
╚██╗██╔╝██╔══██╗██║  ██║██╔══██╗██╔════╝ ██╔════╝
 ╚███╔╝ ██████╔╝███████║███████║██║  ███╗█████╗  
 ██╔██╗ ██╔═══╝ ██╔══██║██╔══██║██║   ██║██╔══╝  
██╔╝ ██╗██║     ██║  ██║██║  ██║╚██████╔╝███████╗
╚═╝  ╚═╝╚═╝     ╚═╝  ╚═╝╚═╝  ╚═╝ ╚═════╝ ╚══════╝
```

### *From Silicon to the Stars*

<br>

[![Version](https://img.shields.io/badge/version-4.0.0-6C63FF?style=for-the-badge&logo=data:image/svg+xml;base64,PHN2ZyB4bWxucz0iaHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmciIHZpZXdCb3g9IjAgMCAyNCAyNCI+PHBhdGggZmlsbD0id2hpdGUiIGQ9Ik0xMiAyQzYuNDggMiAyIDYuNDggMiAxMnM0LjQ4IDEwIDEwIDEwIDEwLTQuNDggMTAtMTBTMTcuNTIgMiAxMiAyem0tMiAxNWwtNS01IDEuNDEtMS40MUwxMCAxNC4xN2w3LjU5LTcuNTlMMTkgOGwtOSA5eiIvPjwvc3ZnPg==)](https://github.com/xphage-lang/xphage/releases)
[![License](https://img.shields.io/badge/license-Apache%202.0-00E5FF?style=for-the-badge)](LICENSE)
[![Platform](https://img.shields.io/badge/platform-Linux%20%7C%20macOS%20%7C%20Windows%20%7C%20Android%20%7C%20iOS%20%7C%20Web-E040FB?style=for-the-badge)](https://xphage.dev)
[![Build](https://img.shields.io/badge/build-passing-22C55E?style=for-the-badge)](https://github.com/xphage-lang/xphage/actions)
[![Phase](https://img.shields.io/badge/phase-4%20complete%20%7C%205%20ongoing-F97316?style=for-the-badge)](ROADMAP.md)

<br>

**[Documentation](https://xphage.dev/docs) · [Language Book](https://xphage.dev/book) · [Playground](https://xphage.dev/play) · [Packages](https://xphage.dev/packages) · [Changelog](CHANGELOG.md)**

<br>

</div>

---

## What is XPhage?

**XPhage** is a compiled, statically-typed, multi-paradigm programming language built on **LLVM**. It targets every computing environment — from bare-metal OS kernels to AI inference engines to cross-platform mobile apps — without making you choose between speed, safety, or expressiveness.

```xphage
// XPhage — one language, every platform

~link "ai"
~link "net"

flux score: int = 0

forge User {
    name:  str   = ""
    level: int   = 1
    score: float = 0.0
}

pulse main() {
    atom user = spawn User { name: "Developer", level: 42 }

    // Bash power inside the language
    atom branch = proc "git rev-parse --abbrev-ref HEAD"
    atom key    = env.API_KEY

    // Pattern matching on any type
    probe user.level {
        diverge 1  -> beam "Beginner"
        diverge 42 -> beam f"Expert: {user.name} on {branch}"
        diverge _  -> beam "Learning..."
    }

    // Reactive state + event-driven architecture
    score = score + 100
    emit "score_updated" { score }

    // AI inference built-in
    atom llm  = llm_load("llama-3.2-3b.gguf", 4096, 32)
    atom resp = llm_generate(llm, f"Greet {user.name}:", 100)
    beam resp
}
```

---

## Why XPhage?

Every language forces you to choose. XPhage refuses.

| What You Need | What Others Force You To Use | XPhage |
|--------------|------------------------------|--------|
| C++ speed | C++ (unsafe, complex) | ✅ LLVM native, same speed |
| Memory safety | Rust (borrow checker complexity) | ✅ `own`/`ref` — simpler model |
| Python expressiveness | Python (slow, no types) | ✅ f-strings, pipeline, lambda |
| Bash system power | Bash (not a real language) | ✅ `proc`, `env`, `bypass` built-in |
| Cross-platform UI | Compose + SwiftUI + HTML (3 languages) | ✅ Fusion UI — one codebase |
| AI/ML built-in | Python + C++ bindings | ✅ `~link "ai"` — first-class |
| OS/Embedded | C (no safety) | ✅ Phase 8 bare metal mode |
| Reactive state | Redux + MobX + hooks chaos | ✅ `flux` — one keyword |

---

## Core Design

```
XPhage = C++ Speed
       + Rust Safety Model (simpler)
       + Python Expressiveness
       + Bash System Power
       + Own Cross-Platform GPU UI (Fusion UI)
       + AI/ML First-Class
```

---

## Tri-Modular File System

XPhage enforces separation of concerns at the file level. The compiler validates this at parse time.

```
.xh   →  Logic Layer    →  declarations: types, constants, functions
.xui  →  UI Layer       →  Fusion UI components only
.xp0  →  Engine Layer   →  execution, entry point, everything
```

```
my-app/
├── src/
│   ├── main.xp0        ← entry point + event handlers
│   ├── models.xh       ← forge structs, nexus interfaces
│   ├── logic.xh        ← function signatures (API contract)
│   └── screen.xui      ← Fusion UI layout
├── xpm.toml            ← project manifest
└── xpm.lock            ← pinned dependencies (commit this)
```

---

## Language Features

### Variables

```xphage
atom  name  = "XPhage"      // immutable — const auto
shadow count = 0             // mutable — auto
global app   = "XPhage"     // module-level static
const  PI    = 3.14159      // compile-time constexpr
flux   score: int = 0       // reactive — auto-notifies UI
own    buffer = alloc(1024) // owned resource — freed at scope end
```

### Functions

```xphage
pulse add(a: int, b: int) -> int {
    return a + b
}

async pulse fetch(url: str) -> str {
    atom resp = await http_get(url)
    return resp.body
}

// Lambda
atom double = |x: int| x * 2

// Pipeline
atom result = "  hello  " |> str_trim |> str_upper   // "HELLO"
```

### Types

```xphage
forge User {
    name:   str   = ""
    age:    int   = 0
    active: bool  = true
}

nexus Drawable {
    draw()   -> void
    area()   -> float
}

impl Drawable for User {
    draw()  -> void  { beam f"User: {self.name}" }
    area()  -> float { return 0.0 }
}

atom user = spawn User { name: "Nahid", age: 25 }
user.draw()
```

### Pattern Matching

```xphage
probe status {
    diverge "ok"    -> beam "All systems go"
    diverge "error" -> { log_error(); alert_admin() }
    diverge 404     -> beam "Not found"
    diverge _       -> beam f"Unknown: {status}"
}
```

### Reactive State + Events

```xphage
flux counter: int = 0

emit "increment"
emit "purchase" { product_id, price }

absorb "increment" { counter = counter + 1 }
absorb "purchase"  { process_order(product_id) }
absorb "purchase"  { send_receipt(price) }    // multiple handlers
```

### System Power

```xphage
// Process capture — like bash $()
atom branch = proc "git rev-parse --abbrev-ref HEAD"
atom log    = proc "git log --oneline -5"

// Environment variables
atom key  = env.API_KEY
atom home = env.HOME
atom port = str_to_int(env.PORT)

// System commands
bypass "mkdir -p build/release"
bypass "chmod +x scripts/deploy.sh"

// File glob
atom files = glob "src/**/*.xp0"
```

### Ownership

```xphage
own atom file   = open_file("data.bin")     // freed at scope end
own shadow conn = db_connect("localhost")   // no manual close needed

pulse display(ref u: User)      { beam u.name }    // immutable borrow
pulse birthday(mut_ref u: User) { u.age += 1 }     // mutable borrow
```

### Error Handling

```xphage
// vortex — try/catch
vortex {
    atom data = parse_config("config.xh")
    apply(data)
}

// ? — propagate errors up
pulse load(path: str) -> str {
    atom raw  = io_read(path)?
    atom valid = validate(raw)?
    return parsed
}
```

---

## Standard Library

All stdlib modules are `.xh` files — written in XPhage, not C++.

```xphage
~link "io"           // file, console, path, glob, proc, env
~link "math"         // arithmetic, trig, stats, Vec2/Vec3/Mat4, random
~link "string"       // split, join, trim, regex, encode, format
~link "collections"  // Option, Result, Vec, Map, Set, Queue, Stack
~link "net"          // HTTP, WebSocket, TCP, UDP, DNS, JSON
~link "os"           // platform, threads, time, mutex, signals
~link "crypt"        // AES-256, SHA-3, RSA, Ed25519, Argon2id, UUID
~link "ai"           // Tensor, neural ops, LLM, NPU dispatch, vector DB
~link "fusion-ui"    // full cross-platform GPU UI framework
```

### io

```xphage
~link "io"

atom content  = io_read("config.xh")
io_write("output.txt", data)
atom exists   = io_exists("bin/app")
atom files    = glob "src/**/*.xp0"
atom name     = input("Enter name: ")
atom branch   = proc "git branch --show-current"
atom home     = env.HOME
atom full     = path_join(home, "projects/myapp")
```

### math

```xphage
~link "math"

atom r = sqrt(144.0)                          // 12.0
atom v = lerp(0.0, 100.0, 0.5)               // 50.0
atom c = clamp(150.0, 0.0, 100.0)            // 100.0
atom n = rand_int(1, 100)
atom g = rand_gaussian(0.0, 1.0)
atom avg = mean_list("10,20,30,40,50")        // 30.0

atom v1 = spawn Vec3 { x: 1.0, y: 0.0, z: 0.0 }
atom v2 = spawn Vec3 { x: 0.0, y: 1.0, z: 0.0 }
atom cross = vec3_cross(v1, v2)               // (0, 0, 1)
```

### crypt

```xphage
~link "crypt"

atom id   = crypt_uuid()
atom hash = hash_sha256("my data")
atom key  = crypt_rand_key()
atom enc  = aes_encrypt("secret message", key)
atom dec  = aes_decrypt(enc, key)

atom pw_hash = argon2_hash("user_password")
atom ok      = argon2_verify("user_password", pw_hash)

atom keys = ed25519_generate()
atom sig  = ed25519_sign("document", priv_key)
```

### ai

```xphage
~link "ai"

// Tensor compute
atom t = tensor_rand("128x768")
atom g = tensor_to_gpu(t)
atom r = tensor_matmul(a, b)
atom o = nn_attention(q, k, v, mask)

// LLM inference — llama.cpp backend
atom llm  = llm_load("llama-3.2-3b.gguf", 4096, 32)
atom resp = llm_generate(llm, "Explain XPhage:", 512)
atom emb  = llm_embed(llm, "semantic search query")

// Vector DB for RAG
atom db   = vecdb_new(768)
vecdb_add(db, "doc1", embedding, "metadata")
atom hits = vecdb_search(db, query_emb, 5)
```

---

## Fusion UI

XPhage's native cross-platform GPU rendering framework. **Same `.xui` file → identical pixels everywhere.**

```
Same source →  Android  (Vulkan 1.2)
            →  iOS      (Metal 2)
            →  macOS    (Metal 2)
            →  Windows  (DirectX 12 / Vulkan)
            →  Linux    (Vulkan / OpenGL)
            →  Web      (WebGPU)
            →  Smart TV (Vulkan)
            →  WatchOS  (OpenGL ES)
```

**No Jetpack Compose. No SwiftUI. No HTML. No JVM. Own GPU engine.**

```xphage
// screen.xui
~link "fusion-ui"

flux count:    int  = 0
flux username: str  = "Guest"

atom card = weave()
    .fill_width()
    .corner_radius(16)
    .elevation(3)
    .padding(20)
    .background("#1A1A2E")

fusion HomeScreen {
    Scaffold {
        top_bar: OrbitH(weave().background("#6C63FF").padding(16)) {
            Vision(f"Hello {username}")
            Spacer(weight: 1)
            Vision(f"Score: {count}")
        }

        content: Orbit(weave().padding(16)) {
            Signal(card) {
                Orbit {
                    Vision("Counter Demo")
                    Spacer(8)
                    OrbitH {
                        Trigger("-") { emit "decrement" }
                        Spacer(weight: 1)
                        Vision(f"{count}")
                        Spacer(weight: 1)
                        Trigger("+") { emit "increment" }
                    }
                }
            }
            Spacer(16)
            Mesh(cols: 2, gap: 12) {
                Signal(card) { Vision("Users: 1,234") }
                Signal(card) { Vision("Revenue: $5K") }
                Signal(card) { Vision("Orders: 890") }
                Signal(card) { Vision("Rating: 4.9") }
            }
        }

        fab: Trigger("+", weave().corner_radius(28)) {
            emit "create_new"
        }
    }
}
```

```xphage
// main.xp0
absorb "increment" { count = count + 1 }
absorb "decrement" { if count > 0 { count = count - 1 } }
absorb "create_new" { navigate("create") }
```

### weave Modifier System

```xphage
weave().width(200).height(100)
weave().fill_width().fill_height()
weave().weight(1)               // flex
weave().padding(16)
weave().padding(8, 16)          // vertical, horizontal
weave().background("#1A1A2E")
weave().corner_radius(12)
weave().corner_radius(16, 16, 0, 0)  // tl, tr, br, bl
weave().elevation(4)
weave().alpha(0.8)
weave().rotate(45).scale(1.5)
weave().clip()
weave().scrollable_v()
weave().clickable { emit "tapped" }
```

### strand Animations

```xphage
strand fade_in { tween(alpha: 0.0 -> 1.0, duration: 300, easing: ease_out) }
strand slide_up { tween(offset_y: 100 -> 0, duration: 350, easing: ease_in_out) }
strand bounce { spring(scale: 0.6 -> 1.0, stiffness: 400, damping: 28) }
strand enter { spring(offset_y: 50 -> 0, preset: gentle) }
// presets: gentle | wobbly | stiff | snappy | slow
```

---

## XPM — Package Manager

Cargo-style developer experience + Go-style decentralized distribution.

```bash
xpm new my-app          # new project
xpm add fusion-ui       # install package
xpm add ai@0.9.0        # specific version
xpm remove fusion-ui    # uninstall
xpm update              # update all (MVS algorithm)
xpm build               # build project
xpm run                 # build + run
xpm test                # run tests
xpm publish             # publish package
xpm search "http"       # search registry
```

**xpm.toml**

```toml
[package]
name        = "my-app"
version     = "1.0.0"
author      = "Your Name <email>"

[dependencies]
fusion-ui   = "1.0"
ai          = "0.9"
net         = "1.0"

[build]
entry       = "src/main.xp0"
opt         = "O2"
targets     = ["android", "ios", "web", "linux"]
```

**Architecture:** Zero central code server. Git IS the distribution. Every package verified with SHA256 + Ed25519 signature. Works with private corporate git servers.

---

## Install

```bash
# Linux / macOS
curl -sL https://xphage.dev/install | sh

# Windows
irm https://xphage.dev/install | iex

# Verify
xphage --version   # XPhage 4.0.0
xpm --version      # XPM 1.0.0
```

**First project:**

```bash
xpm new hello-world
cd hello-world
xphage run src/main.xp0
```

---

## Build

```bash
# Run
xphage run src/main.xp0

# Build native binary
xphage build -O2 src/main.xp0 -o bin/app

# Platform targets
xphage build --target linux    src/main.xp0
xphage build --target windows  src/main.xp0
xphage build --target macos    src/main.xp0
xphage build --target android  src/main.xp0
xphage build --target ios      src/main.xp0
xphage build --target web      src/main.xp0

# Optimization
xphage build -O3 --lto src/main.xp0     # maximum performance
xphage build -Oz src/main.xp0           # minimum size (embedded)
xphage build -g src/main.xp0            # debug info (DWARF)
xphage build --emit-ir src/main.xp0     # output LLVM IR
```

---

## Roadmap

| Phase | Version | Features | Status |
|-------|---------|----------|--------|
| **1** | v1.0 | Lexer, Pratt parser, AST, C++17 transpiler, types, control flow, f-string, `\|>` pipeline, lambda | ✅ Complete |
| **2** | v2.0 | `forge`, `nexus`, `impl`, `flux`, `probe/diverge`, `emit/absorb`, `weave`, `strand` | ✅ Complete |
| **3** | v3.5 | `own/ref/mut_ref`, `async/await`, `proc`, `env`, `const`, `unsafe`, `extern`, `?` propagation | ✅ Complete |
| **4** | v4.0 | **LLVM native backend**, IR lowering, O0-O3 optimization, DWARF debug, LTO, PGO, smaller binaries | ✅ Complete |
| **5** | v4.5 | **Fusion UI GPU backends** — Vulkan (Android/Linux), Metal (iOS/macOS), WebGPU (Web), OpenGL fallback | 🔄 Ongoing |
| **6** | v5.0 | **Full Generics** — `pulse max<T>`, `forge Stack<T>`, `nexus Container<T>`, trait bounds, typed `Vec<T>`/`Map<K,V>` | 🔜 ~9 months |
| **7** | v6.0 | **NPU/GPU Compute** — `accelerate` keyword, SIMD auto-vectorization, GPU kernels | 🔜 ~18 months |
| **8** | v7.0 | **Bare Metal** — no-std, OS kernel mode, interrupt handlers, hardware registers, page tables | 🔜 ~24 months |
| **9** | v8.0 | **Self-Hosting** — XPhage compiler written in XPhage | 🔜 3+ years |

### Production Readiness

| Use Case | Status |
|----------|--------|
| CLI tools & automation | ✅ **Production Ready Now** |
| Backend / REST API | ✅ **Production Ready Now** |
| Data processing | ✅ **Production Ready Now** |
| AI/ML inference (CLI) | ✅ **Production Ready Now** |
| Desktop GUI app | 🔄 Phase 5 — ~6 months |
| Web app (WASM) | 🔄 Phase 5 — ~9 months |
| Android / iOS mobile | 🔄 Phase 5 — ~12 months |
| OS kernel / embedded | 🔜 Phase 8 — ~24 months |

---

## Ecosystem

| Tool | Description | Status |
|------|-------------|--------|
| `xphage` | Compiler, build tool, LLVM backend | ✅ v4.0.0 |
| `xpm` | Package manager | ✅ v1.0.0 |
| `xpm-registry` | Sparse TOML package index | ✅ Live |
| `xforge` | Universal installer + version manager | 🔄 In Progress |
| `xphage-lsp` | Language Server Protocol — IDE integration | 🔄 In Progress |
| `xphage-fmt` | Opinionated code formatter | 🔜 Planned |
| `xphage-doc` | Documentation generator | 🔜 Planned |
| `xphage-test` | Built-in test runner | 🔜 Planned |
| VS Code Extension | Syntax, IntelliSense, debug, format | 🔄 In Progress |

---

## Repository Structure

```
xphage-lang/              ← GitHub organization
├── xphage                ← compiler + runtime + stdlib (this repo)
│   ├── compiler/
│   │   ├── xphage_lexer/
│   │   ├── xphage_parse/
│   │   ├── xphage_ast/
│   │   ├── xphage_middle/        ← IR lowering
│   │   ├── xphage_codegen_llvm/  ← LLVM backend (Phase 4)
│   │   ├── xphage_codegen_transpiler/ ← C++17 fallback
│   │   ├── xphage_interface/
│   │   ├── xphage_linker/
│   │   └── xphage_driver/        ← main()
│   ├── include/xphage/
│   │   ├── ast.hpp               ← tokens + node kinds
│   │   ├── runtime.hpp           ← class declarations
│   │   └── fusion/fusion.hpp     ← Fusion UI bridge
│   ├── library/                  ← stdlib .xh modules
│   │   ├── io/io.xh
│   │   ├── math/math.xh
│   │   ├── string/string.xh
│   │   ├── collections/collections.xh
│   │   ├── net/net.xh
│   │   ├── os/os.xh
│   │   ├── crypt/crypt.xh
│   │   └── ai/ai.xh
│   ├── scripts/
│   │   ├── build.sh
│   │   └── install.sh
│   └── docs/
│
├── xpm                   ← package manager
├── xpm-registry          ← package index (TOML)
├── fusion-ui             ← UI framework (separate repo)
│   ├── include/xp/       ← flux, modifier, node, theme, animation
│   ├── engine/           ← layout, paint, diff engines
│   ├── backends/         ← vulkan, metal, webgpu, opengl, console
│   └── codegen/          ← android, ios, web packagers
└── xforge                ← installer tool
```

---

## Project & Ownership

<table>
<tr>
<td><b>Language</b></td>
<td>XPhage</td>
</tr>
<tr>
<td><b>Organization</b></td>
<td><a href="https://github.com/xphage-lang">xphage-lang</a></td>
</tr>
<tr>
<td><b>Core Developer</b></td>
<td><a href="https://github.com/cybernahid-dev">cybernahid-dev</a></td>
</tr>
<tr>
<td><b>Brand / Company</b></td>
<td>AeonCoreX</td>
</tr>
<tr>
<td><b>License</b></td>
<td>Apache 2.0</td>
</tr>
<tr>
<td><b>Website</b></td>
<td><a href="https://xphage.dev">xphage.dev</a></td>
</tr>
</table>

> XPhage is the flagship language of **AeonCoreX** — designed, built, and maintained by **[cybernahid-dev](https://github.com/cybernahid-dev)**. The project is hosted under the **[xphage-lang](https://github.com/xphage-lang)** GitHub organization.

---

## Contributing

Contributions are welcome. Please read [CONTRIBUTING.md](CONTRIBUTING.md) before submitting a PR.

```bash
# Clone
git clone https://github.com/xphage-lang/xphage
cd xphage

# Build compiler
bash scripts/build.sh

# Run tests
xphage test tests/

# Submit PR against: main branch
```

**Areas that need help:**
- Fusion UI GPU backends (Vulkan / Metal / WebGPU)
- Language Server Protocol implementation
- Standard library expansion
- Documentation and examples
- Platform testing (Android, iOS, embedded)

See [CONTRIBUTING.md](CONTRIBUTING.md) and [open issues](https://github.com/xphage-lang/xphage/issues).

---

## Security

Found a vulnerability? Please disclose responsibly:

**Email:** `security@xphage.dev`  
**Response time:** 72 hours  
**Policy:** [SECURITY.md](SECURITY.md)

Do **not** open public issues for security vulnerabilities.

---

## License

```
Copyright 2026 AeonCoreX / cybernahid-dev

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0
```

See [LICENSE](LICENSE) for full text.

---

<div align="center">

**Built by [cybernahid-dev](https://github.com/cybernahid-dev) · Owned by [AeonCoreX](https://aeoncorex.dev) · Hosted at [xphage-lang](https://github.com/xphage-lang)**

<br>

*"From Silicon to the Stars"*

</div>
