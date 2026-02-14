# Changelog: X-Phage Engine Evolution

All notable changes to the **X-Phage** project from its genesis to the current **Omni-God v3.1 [TITAN]** edition are documented below.

---

## [3.1.0] - 2026-02-14 (Current: Titan Fusion Edition)
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

## [1.3.0] - 2026-01-05 (Genesis Expansion)
### Added
- **Library Linking**: Initial support for `~link` to include external logic files.

## [1.0.0] - 2025-12-15 (The Beginning)
### Added
- **Core Architecture**: Initial release of the Lexer and Runtime with basic keywords (`pulse`, `atom`, `beam`).

---
*© 2026 AeonCoreX Intellectual Property. All Rights Reserved.*
