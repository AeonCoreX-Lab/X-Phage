# X-Phage Language Manual v4.0.0
**AeonCoreX Lab** — https://github.com/AeonCoreX-Lab/X-Phage

> C++ speed + Rust safety model + Python expressiveness + Own cross-platform UI.
> One language: OS kernel to mobile app.

---

## Quick Start

```bash
# Build
cd X-Phage && bash scripts/build.sh

# Compile + run
./build/xphage hello.xp0

# REPL
./build/xphage repl

# Emit generated C++
./build/xphage --emit=cpp hello.xp0

# Show IR
./build/xphage --emit=ir hello.xp0
```

---

## File Extensions

| Extension | Layer | Purpose |
|-----------|-------|---------|
| `.xh` | Logic Layer | Declarations only: `atom`, `global`, `forge`, `nexus`, `pulse`, `flux`, `const` |
| `.xui` | UI Layer | `fusion` blocks + `~link` |
| `.xp0` | Engine Layer | Entry point + full execution |

---

## Phase 1 — Foundation

### Variable Declaration

```xphage
atom  x: int   = 42        // const (immutable)
shadow y: float = 3.14     // mutable
global APP_NAME = "X-Phage" // module-level static
const MAX: int = 1000       // compile-time constexpr
```

### Types

| X-Phage | C++ | Notes |
|---------|-----|-------|
| `int` | `long long` | 64-bit signed |
| `float` | `double` | 64-bit float |
| `bool` | `bool` | `true`/`false` |
| `str` | `std::string` | Unicode |
| `auto` | `auto` | Inferred |
| `void` | `void` | Null/no-value |

### Functions

```xphage
pulse greet(name: str) -> str {
    return f"Hello, {name}!"
}

// Recursive
pulse factorial(n: int) -> int {
    if n <= 1 { return 1 }
    return n * factorial(n - 1)
}
```

### Control Flow

```xphage
// if / elif / else
if score >= 90 {
    beam "A"
} elif score >= 80 {
    beam "B"
} else {
    beam "F"
}

// while
while i < 10 { i = i + 1 }

// for + range
for i in 0..10 { beam f"i={i}" }

// for over iterable
for item in my_list { beam item }

// break / continue
while true {
    if done { break }
    if skip { continue }
}
```

### Built-in Keywords

```xphage
beam f"output: {value}"       // print to stdout
scan username                 // read from stdin
bypass "mkdir -p build/"      // system command
chronos 500                   // sleep 500ms
quantum { /* thread body */ } // spawn thread
vortex { /* try */ } catch (e) { /* catch */ }
```

### f-Strings

```xphage
atom msg = f"Hello {name}, score: {score * 2}"
// → std::ostringstream stream; stream << "Hello " << name << ...
```

### Pipeline Operator

```xphage
atom result = "  hello  " |> str_trim |> str_upper
// → str_upper(str_trim("  hello  "))
```

---

## Phase 2 — Type System + Events

### forge (struct)

```xphage
forge User {
    name:  str  = ""
    email: str  = ""
    score: int  = 0
    admin: bool = false
}

shadow alice = spawn User {
    name:  "Alice"
    email: "alice@dev.com"
    score: 100
}
beam f"User: {alice.name}"
```

### nexus (interface/trait)

```xphage
nexus Drawable {
    draw()   -> void
    bounds() -> str
}
```

### flux (reactive state)

```xphage
flux counter: int = 0
flux username: str = "Guest"

// Any assignment automatically notifies observers
counter = counter + 1   // → triggers re-render in UI
username = "Alice"
```

### impl (implement nexus for forge)

```xphage
impl Drawable for Circle {
    pulse draw() -> void {
        beam f"Circle at {self.x},{self.y}"
    }
    pulse bounds() -> str {
        return f"({self.x},{self.y})"
    }
}
```

### probe / diverge (pattern match)

```xphage
probe status {
    diverge "ok"    -> beam "healthy"
    diverge "error" -> { beam "error!"; bypass "notify.sh" }
    diverge _       -> beam "unknown"
}
```

### emit / absorb (event bus)

```xphage
// Register handler
absorb "purchase" {
    beam "Processing order..."
}

// Fire event with data
emit "purchase" { product_id: "XP-001" quantity: "3" }
```

### Lambda

```xphage
atom double = |x: int| x * 2
atom add    = |a: int, b: int| a + b
atom greet  = |name: str| { return f"Hi, {name}!" }
```

### cast / typeof / sizeof

```xphage
shadow n = cast(3.14, int)      // 3
atom t  = typeof(n)             // → decltype(n)
atom s  = sizeof(int)           // → sizeof(long long)
```

---

## Phase 3 — Systems + Python/Bash Power

### proc (shell capture)

```xphage
atom branch = proc "git rev-parse --abbrev-ref HEAD"
atom files  = proc "ls src/"
atom commit = proc "git log --oneline -1"
beam f"On branch: {branch}"
```

### env (environment variables)

```xphage
atom home  = env.HOME
atom user  = env.USER
atom token = env.GITHUB_TOKEN
```

### glob (file patterns)

```xphage
atom xh_files  = glob "src/*.xh"
atom xp_files  = glob "examples/*.xp0"
```

### Ownership (Rust-inspired safety)

```xphage
own   T   // unique ownership, move semantics
ref   T   // immutable borrow
mut_ref T // mutable borrow

pulse process(data: ref str) -> int { /* read-only */ }
pulse mutate(data: mut_ref str)     { /* can write */ }
```

### async / await

```xphage
async pulse fetch(url: str) -> str {
    chronos 100   // simulate I/O
    return f"data from {url}"
}

shadow result = await fetch("https://api.example.com")
beam result
```

### Error propagation

```xphage
// ? operator: if error, propagate up
atom data = io_read("config.xh")?
```

### yield (generators)

```xphage
pulse count_up(n: int) {
    for i in 0..n {
        yield i
    }
}
```

---

## Standard Library

| Module | `~link` | Key exports |
|--------|---------|-------------|
| IO | `"io"` | `io_read`, `io_write`, `io_exists`, `path_join`, `env_get` |
| Math | `"math"` | `sin/cos/sqrt/pow`, `Vec2/Vec3`, `PI`, `rand_int` |
| String | `"string"` | `str_trim/upper/lower/split/join/replace` |
| Collections | `"collections"` | `range`, `vec_map/filter`, `Option`, `Result` |
| Network | `"net"` | `http_get/post`, `WebSocket`, `json_parse` |
| OS | `"os"` | `os_platform`, `thread_spawn`, `mutex_new` |
| Cryptography | `"crypt"` | `hash_sha256`, `aes_encrypt`, `rsa_generate`, `crypt_uuid` |
| AI/ML | `"ai"` | `Tensor`, `nn_linear`, `llm_load/generate` |

---

## Fusion UI (Phase 5)

```xphage
~link "fusion-ui"

flux count: int = 0

fusion HomeScreen {
    Scaffold {
        top_bar: OrbitH(weave().background("#6C63FF").padding(16)) {
            Vision("X-Phage")
            Spacer(weight: 1)
            Vision(f"Count: {count}")
        }
        content: Orbit(weave().padding(16)) {
            Signal(weave().corner_radius(12).elevation(2)) {
                Trigger("Increment") { emit "inc" }
            }
        }
        fab: Trigger("+") { emit "create" }
    }
}

absorb "inc" { count = count + 1 }
```

---

## Compiler CLI

```
xphage <file.xp0>              Compile and run
xphage build <file.xp0>        Compile to binary
xphage repl                    Interactive REPL
xphage --emit=cpp <file.xp0>   Show generated C++
xphage --emit=ir  <file.xp0>   Show XPIR text
xphage --emit=ast <file.xp0>   Dump AST
xphage --libs                  List stdlib modules
xphage -O <file.xp0>           Optimize build
xphage -v <file.xp0>           Verbose output
xphage --version               Show version
```

---

## Phase Roadmap

| Phase | Status | Features |
|-------|--------|----------|
| 1 | ✅ Complete | Lexer, Pratt parser, AST, C++ transpiler, control flow, types, f-string |
| 2 | ✅ Complete | forge/nexus/flux, probe/diverge, emit/absorb, pipeline `\|>`, impl |
| 3 | ✅ Complete | lambda `\|x\|`, proc/env, own/ref, async/await, `?`, yield |
| 4 | 🔲 Planned | LLVM backend (native binary, no C++ intermediate) |
| 5 | 🔲 Planned | Fusion UI GPU backends (Vulkan/Metal/WebGPU) |
| 6 | 🔲 Planned | Generics `pulse max<T>`, trait bounds |
| 7 | 🔲 Planned | NPU/GPU `accelerate` keyword, SIMD dispatch |
| 8 | 🔲 Planned | Bare metal / no-std / OS kernel mode |
| 9 | 🔲 Long-term | Self-hosting: compiler written in X-Phage |

---

## Phase 4 — LLVM Native Backend

Phase 4 adds a direct AST → LLVM IR → native binary path. No C++ intermediate.

### Build with LLVM

```bash
# Linux
sudo apt install llvm-17-dev
bash scripts/build_llvm.sh

# macOS
brew install llvm
bash scripts/build_llvm.sh

# Arch
sudo pacman -S llvm
bash scripts/build_llvm.sh
```

### CLI flags

```
xphage --backend=llvm file.xp0     Compile + run via LLVM
xphage --emit=llvm    file.xp0     Emit LLVM IR (.ll file)
xphage --emit=obj     file.xp0     Emit native object (.o)
xphage --emit=ir      file.xp0     Emit X-Phage IR (.xpir)
xphage -O --backend=llvm file.xp0  Optimised native binary
```

### Architecture

```
Source (.xp0)
  → Lexer
  → Pratt Parser
  → AST
  → ASTCodegen (xphage_codegen_llvm)
      ├─ ASTNode → llvm::Value*  (expression visitor)
      ├─ ASTNode → BasicBlock    (control flow)
      ├─ forge   → StructType    (type system)
      └─ pulse   → Function      (function codegen)
  → llvm::Module
  → OptimizationPasses (O0-O3, new pass manager)
  → TargetMachine (x86_64 / arm64 / wasm32)
  → native .o
  → cc + xprt.a → binary
```

### xprt Runtime Library

`xprt` is a compact C library (~400 lines) providing:

| Symbol | Purpose |
|--------|---------|
| `xprt_beam_int/float/str` | Print values |
| `xprt_str_new/concat/upper/trim` | String ops |
| `xprt_emit/absorb` | Event bus |
| `xprt_flux_*` | Reactive state |
| `xprt_proc(cmd)` | Shell capture |
| `xprt_env_get(key)` | Env vars |
| `xprt_chronos(ms)` | Sleep |
| `xprt_bypass(cmd)` | System call |

### LLVM Version Compatibility

| LLVM | Status | Notes |
|------|--------|-------|
| 21   | ✅ | Latest |
| 20   | ✅ | |
| 19   | ✅ | |
| 18   | ✅ | CodeGenFileType moved to llvm/Support/CodeGen.h |
| 17   | ✅ | Opaque pointers, TargetParser split |
| 16   | ✅ | Minimum supported |
| ≤ 15 | ⚠️ | Untested, likely broken |
