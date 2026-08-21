# Changelog: X-Phage Engine Evolution

All notable changes to the **X-Phage** project from its genesis to the current **Titan v3.4 [Stdlib]** edition are documented below.

---

## [3.5.2] - 2026-07-02 (Diagnostic Engine + Semantic Analyzer)
### Added
- **Diagnostic Engine** (`xphage_sema/diagnostic.{hpp,cpp}`): structured diagnostics with a stable error code (`XP1000`-`XP9999` ranges, matching the language's own design spec), severity, source span, and rustc-style multi-line rendering — location line, source snippet, `^^^^` underline sized to the actual offending token, and an optional `help:` suggestion line.
- **Semantic Analyzer** (`xphage_sema/semantic_analyzer.{hpp,cpp}`), a standalone compiler pass — `Parser → AST → Semantic Analyzer → Verified AST → (LLVM backend | Transpiler)` — deliberately kept separate from XIL lowering (name resolution and type checking are a different responsibility than building IR; this also keeps the door open for generics/ownership/Fusion UI checks to become their own passes later without re-entangling them with codegen). Two-pass design: declaration collection (so forward references between top-level functions resolve correctly), then body checking with a real scoped `SymbolTable` (`xphage_sema/symbol_table.{hpp,cpp}`) and Levenshtein-based "did you mean?" suggestions.
- Catches, as clean XPhage-level diagnostics instead of leaked C++ compiler errors: undefined variable (`XP2001`), undefined function with a `~link` hint (`XP2002`), calling a non-function (`XP2003`), argument-count mismatch (`XP2004`), duplicate function/type definition with a "previous definition here" secondary location (`XP2010`/`XP2011`), same-scope shadowing warning (`XP2020`), assignment to an immutable `atom` (`XP2030`).
- Wired into **every** compile entry point (`compile()`, `compile_xp()`, `compile_multi()`), and — new in this release — into the **LLVM backend** too via a shared `try_llvm_backend()` helper. Previously `cfg.backend == Backend::LLVM` was checked in exactly one place in the entire codebase, inside `compile()` only, meaning `--backend=llvm` was silently ignored for a plain `.xp` file (the primary, developer-facing format) and for any Mixed-mode merge — only the direct multi-file `.xh`/`.xui`/`.xp0` path could ever reach LLVM codegen. All three compile paths now route through the same semantic-analysis-then-backend-selection sequence.

### Fixed (found while building and testing the above)
- Scientific notation float literals (`1e308`, `6.022e23`) were completely unlexed — silently mis-tokenized as an integer immediately followed by an identifier (`1` then `e308`), with no error.
- `flux` (Phase 2 reactive-state) declarations and the bodies of `absorb`/`emit` blocks were invisible to semantic analysis — every `flux`-declared variable was reported as undefined, and assignments inside an `absorb` body were never checked.
- **Critical**: 7 separate infinite-loop vulnerabilities across the parser's delimiter-separated list-parsing loops (function parameters, extern parameters, call arguments, lambda parameters, weave modifier arguments, struct/spawn fields, interface method parameters) — any malformed input where an element parser could legitimately consume zero tokens (e.g. writing `Type.Member` instead of `Type::Member` in a parameter type) would hang the compiler forever with no timeout and no error. Fixed with a shared `ensure_progress_or_recover()` guard applied at all 7 sites.
- A false-positive "shadows a previous declaration" warning on every top-level `atom`/`shadow`, caused by the semantic analyzer's own two-pass design: Pass 1 (declaration collection) registers top-level `atom`/`shadow` symbols so later top-level code can reference them, but Pass 2 (body checking) was re-declaring the same symbol and finding its own Pass-1 registration, reporting it as shadowing itself.

### XIL status (honest assessment, not a claim of completion)
X-Phage has a real, working internal IR (`xphage_middle/ir_lower.cpp`, `include/xphage/ir.hpp`) with a language-aware instruction set (`Emit`, `Absorb`, `Flux`, `Quantum`, `Chronos`, `Await`, `Yield` alongside conventional `Alloc`/`Load`/`Store`/`Call`/branches) and a functioning AST→IR lowering pass, verified working via `--emit=ir`. **This is not yet a production code-generation path.** Both `transpile_ir()` (`xphage_codegen_transpiler`) and `compile_ir()` (`xphage_codegen_llvm`) — the functions that would consume this IR to emit C++ or LLVM IR respectively — are explicit, self-documenting stubs: `transpile_ir` always emits an empty `int main(){return 0;}` regardless of input ("IR → C++ re-uses the same transpiler (via AST path)" per its own comment), and `compile_ir` unconditionally returns the error "IR module path in Phase 5." Neither is called from anywhere in the real compile pipeline — both real backends (the C++ transpiler and the LLVM backend) still perform their own independent AST-to-output walk, exactly as before this release. Making the IR a real, mandatory part of code generation means writing two new, complete backends over `IRModule`/`IRFunction`/`IRBlock`/`IRInstr` covering the full instruction set and control flow — comparable in scope to writing a second transpiler and a second LLVM codegen pass. This is correctly scoped as future, multi-session work, not something to claim as done.

## [3.5.0] - 2026-06-28 (Stdlib Reality Pass + FFI)
### Added
- **9-module standard library, matching the language book's Appendix C exactly**: `io`, `math`, `string`, `collections`, `net`, `os`, `crypt`, `ai`, `solver` — replacing the previous `core`/`alloc`/`std` scaffold directories, which contained invalid syntax (`#define` with non-constexpr values, undefined `bypass` targets) and were never actually parseable.
- **Real module resolver**: `~link "math"` (and the other 7 linkable modules) now genuinely locates `library/<name>/<name>.xh` on disk, parses it, and merges its declarations into the program being compiled — this did not exist before; `~link` was previously a no-op comment in the generated output regardless of which module name was given.
- **Real runtime implementations**, inlined into generated output and verified end-to-end: full `math` (trig, rounding, interpolation, random, bit ops), most of `string` (search/transform/split/replace/regex/format), most of `io` (file/path/console/env), most of `collections` (Vec/Map/Set/Queue/Stack). `net`/`crypt`/`ai` remain signature-complete but unimplemented pending an external library dependency (libcurl/OpenSSL/a model runtime respectively); `solver` is Phase 10 per the roadmap and intentionally a placeholder. See `library/README.md` for the exact function-by-function status.
- **`extern "C"` / `unsafe {}` FFI**, fully wired through lexer → parser → AST → transpiler → IR lowering → LLVM backend, with `-l`/`-L`/direct `.a`/`.so` linker passthrough — verified by linking and calling into independently-compiled native libraries (proving any C-ABI-compatible code, including a Rust `cdylib` exporting `#[no_mangle] extern "C" fn`, can be called from X-Phage). `str` parameters/return values crossing the FFI boundary are automatically adapted to/from `const char*`.
- **Raw pointer types** (`*mut T`, `*const T`) in type position, for FFI signatures that need them.
- **Compiler error system hardening**: unterminated string/comment literals now report a real error (previously silent corruption); lexer errors are surfaced through every entry point (`compile()`, `compile_xp()`, REPL, f-string interpolation); error-cascade flooding is capped; `sync_to_next_stmt()` recognizes the full statement-starting keyword set.

### Fixed
- `pulse main` with an explicit `-> int`/`-> void` (or no return type at all) now always transpiles to a literal C++ `int main()` — previously emitted `void main()` or `long long main()`, both invalid, silently breaking any program that declared a return type on `main`.
- A latent bug in `ir_lower.cpp`: `cur_fn`/`cur_blk` were never reset after a function finished lowering, so top-level statements following *any* function declaration would silently leak into that function's IR body instead of their own implicit `main()`.
- `~link core/types` (bare slash-path syntax, used throughout `examples/`) now parses; previously only quoted-string `~link` targets worked.
- Top-level `atom NAME: type = <literal>` now correctly becomes a real file-scope C++ constant (with a macro-collision guard for names like `NAN` that collide with `<cmath>`), instead of being silently dropped or duplicated depending on which code path saw it first.

> **Note on the previous [3.4.0] entry below**: it describes a stdlib module set (`core`, `media`, `security`, `ui`) and an `update-stdlib` XPM command that were not present and not functional in the repository at the time this entry was written. The 3.5.0 work above reflects what has actually been built and verified to work, including the test commands used to verify each claim.

## [3.4.0] - 2026-03-24 (Current: Stdlib & XPM Edition)
### Added
- **Full Standard Library (stdlib)**: Production‑ready modules for `core`, `math`, `io`, `net`, `data`, `media`, `security`, and `ui`.  
- **XPM (X-Phage Package Manager)**: Enhanced with `update-stdlib` command to fetch the entire standard library from GitHub.  
- **Subdirectory Support**: `sync_module` now handles nested modules (e.g., `net/http`).  
- **Improved Error Handling**: Clear color‑coded feedback and iOS sandbox protection.  
- **REPL Banner**: Interactive shell now shows ASCII art, version, developer info, and `help` command.

### Changed
- **Linker Intelligence**: The `~link` directive now searches both `stdlib/` and `modules/` for includes.  
- **Project Init**: `xphage init` creates the `stdlib/` folder structure automatically.  
- **Documentation**: Added in‑line comments to all stdlib files for better IDE support.

### Fixed
- **Windows ARM64**: Correct MSVC environment setup via `ilammy/msvc-dev-cmd`.  
- **Linux ARM64**: Native builds on Ubuntu 24.04 ARM runners.  
- **LLVM Compatibility**: Supports LLVM 16 through 22+ with version‑aware code.


## [3.3.0] - 2026-02-16 (Current: Titan Docker Edition)
### Added
- **Docker Hub Integration**: Official image hosted at `cybernahid/xphage` for zero-dependency setup.
- **Automated Docker CI/CD**: GitHub Actions now auto-builds and pushes images to Docker Hub on every release tag.
- **ARM64 Architecture Support**: Enhanced `build.sh` and `install.sh` to support Linux ARM64 (Termux/Raspberry Pi).
- **Universal Installer**: Redesigned `install.sh` with better OS detection and error handling.
- **Project Scaffolding**: Added `xphage init` command to generate standard project structure automatically.

## [3.2.0] - 2026-02-15 (Titan Optimized Edition)
### Added
- **LLVM Native Backend**: Integrated Clang/LLVM toolchain as the primary compiler backend, enabling `-O3` level machine code optimization for maximum performance.
- **SHA256 Integrity Verification**: Automated security checksum generation for all distributed binaries to ensure supply-chain protection.
- **Hardware Acceleration Hooks**: Added native triggers for Vulkan-based GPU pipelines and NPU neural synchronization within `core_ops`.
- **Multi-Platform CI/CD**: Updated GitHub Actions workflow to automatically provision LLVM environments and build for Windows, Linux, macOS, Android (ARM64), and iOS.
- **Unified Build System**: Refined `build.sh` to handle cross-compilation and native LLVM flag management across all five supported architectures.

### Changed
- **Performance Tuning**: Transitioned from standard GCC flags to LLVM-specific optimization flags for faster execution of `.xp0` logic.
- **Workflow Permissions**: Fixed 403 errors in GitHub Actions by implementing explicit `contents: write` permissions for release generation.

## [3.1.0] - 2026-02-14 (Titan Fusion Edition)
### Added
- **Tri-File Ecosystem**: Formal separation of concerns into `.xp0` (Execution), `.xh` (Logic/Headers), and `.xui` (UI Schematics).
- **Titan Fusion UI Engine**: Implementation of a Declarative UI Tree (Virtual DOM) with GPU direct rendering capabilities.
- **Recursive UI Parsing**: Added `parse_ui_block` in the core engine to support deeply nested UI components and complex layouts.
- **Enhanced Linker**: Intelligent file resolution system with support for local, module-based, and stdlib paths.
- **XPM Cloud Sync**: Automated fetching of missing modules from the AeonCoreX-Lab global registry using `curl`.
- **New UI Tokens**: Added support for `Z_Plane`, `Input`, and `@NeuralComposition` in the Lexer.

### Changed
- **Lexer 3.1**: Updated to handle structural tokens like `:`, `,`, and recursive braces `{}` for UI mapping.
- **Memory 2.0**: Enhanced Dual-Layer memory to support a Global Universal Registry alongside local Shadow RAM.

## [3.0.1] - 2026-02-12 (Omni-God Edition)
### Added
- **Quantum Threading**: Multi-threaded parallel processing via `launch_quantum_process`.
- **Chronos Sync**: Real-time hardware clock synchronization for zero-latency streaming.
- **Ether Protocol**: Direct decentralized cloud uplink for encrypted data relay.
- **Vortex/Void Wipe**: Implementation of Blackhole protocols for instant memory purging and session security.
- **Omni Panic System**: Detailed reporting for `atom` (immutable) memory violation attempts.

## [3.0.0] - 2026-02-05 (Singularity Upgrade)
### Added
- **Hardware Bypass**: Kernel-level direct hardware access skipping OS overhead.
- **Full ALU Core**: Native support for complex arithmetic operations.
- **Matrix Buffer**: High-speed data structures for processing large-scale media streams.

## [2.1.0] - 2026-01-20 (Ghost Memory Update)
### Added
- **Ghost Memory Protocol**: Introduction of `shadow` (mutable) and `atom` (strict immutable) memory types.

## [1.0.0] - 2025-12-15 (The Beginning)
### Added
- **Core Architecture**: Initial release of the Lexer, Parser, and Memory Management systems.
