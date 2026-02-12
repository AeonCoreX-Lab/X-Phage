## 🧬 X-Phage (.xp0) Technical Manual - v3.0 Singularity
Official Documentation for AeonCoreX Intellectual Property

## 🛰 Introduction

X-Phage is a high-performance, hybrid-safety programming language designed to bridge the gap between C++ execution speed and Rust memory safety. Developed by AeonCoreX, it features the Ghost Memory Protocol, which ensures that sensitive data remains immutable and secure within next-gen applications like StreamX Ultra.

## 📂 Project Structure
 * /src: Contains the core engine source code (xphage_core.cpp)—the heart of the Singularity.
 * /tests: Storage for neural logic validation files and performance stress tests.
 * /bin: Target directory for hardware-optimized binary executables.
 * /lib: System libraries (.xh) and hardware abstraction hooks.

## ⌨️ Complete Keyword Reference & Functions
1. Entry Point
 * pulse core { ... }: Defines the primary execution block. Every X-Phage program must initiate from the neural center.
2. Memory Management (Ghost Memory Protocol)
 * atom (New/Updated): Declares an Immutable Constant. Once assigned, the memory address is locked. Any attempt to reassign an atom will trigger a security crash to prevent unauthorized data manipulation (similar to Rust’s safety).
 * shadow: Declares Mutable Ghost Memory. These variables can be updated during runtime for dynamic data processing.
 * matrix (New): A high-speed data structure designed for complex datasets, neural mapping, and audio/video buffering.
3. Logic & Input/Output
 * beam: Streams data directly to the output interface (terminal).
 * scan: A high-speed conditional gate. It evaluates a variable; if the value is 0 or null, the subsequent logic block is bypassed.
 * ~link: Establishes a secure handshake with external libraries or AeonCore modules (e.g., ~link "aeon.core").
4. Mathematical Operations (ALU Integration)
V3.0 Singularity now supports inline arithmetic directly on the hardware level:
 * Operators: + (Addition), - (Subtraction), * (Multiplication), / (Division).
 * Usage: result = value1 * value2.

## 🔒 Security Protocol: Ghost Memory
X-Phage employs a unique memory locking mechanism. When a variable is declared as an atom, the engine marks that memory address as ReadOnly. This prevents buffer overflow attacks and ensures the integrity of the AeonCoreX ecosystem, making it the ideal engine for high-performance streaming services.

## 🚀 Execution Guide
To build and run your X-Phage project using the optimized compiler:
 * Compile the Engine:
   bash build.sh

 * Execute Neural Logic:
   ./bin/xphage tests/your_file.xp0

## 🛰 Mission Statement
X-Phage is NOT intended to replace C++ or Rust. Instead, it serves as a high-level performance bridge designed to eliminate the complexities of memory management while retaining raw execution power. 

Our goal is to empower global developers and future AI OS systems with a language that provides:
* **Safety without Complexity**: Rust-level security with a simplified syntax.
* **Hardware Synergy**: Native hooks for AI-driven operating systems and high-bandwidth applications like StreamX Ultra.
* **Unique Sovereignty**: Features like 'Ghost Memory' and 'Neural Gating' that are not present in traditional low-level languages.


Copyright © 2026 AeonCoreX. All rights reserved. 🤐
Summary of New Features:
 * Rust-like Immutability: The atom keyword now strictly enforces memory locking.
 * Matrix Support: New matrix keyword for handling large data arrays.
 * Full ALU Support: Inline math operations (+, -, *, /) are now fully functional within the engine.
 * Enhanced Splash Screen: The global identity system now identifies as X-Phage Singularity Engine v3.0.

