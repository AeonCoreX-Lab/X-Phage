# Changelog: X-Phage Engine Evolution

All notable changes to the **X-Phage** project from its genesis to the current **Titan v3.3 [Docker]** edition are documented below.

---

## [3.3.0] - 2026-02-16 (Current: Titan Docker Edition)
### Added
- **Docker Hub Integration**: Official image hosted at `cybernahid/xphage` for zero-dependency setup.
- **Automated Docker CI/CD**: GitHub Actions now auto-builds and pushes images to Docker Hub on every release tag.
- **ARM64 Architecture Support**: Enhanced `build.sh` and `install.sh` to support Linux ARM64 (Termux/Raspberry Pi).
- **Universal Installer**: Redesigned `install.sh` with better OS detection and error handling.
- **Project Scaffolding**: Added `xphage init` command to generate standard project structure automatically.

## [3.2.0] - 2026-02-15 (Current: Titan Optimized Edition)
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
