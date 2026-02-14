# 🧬 X-Phage Technical Manual - v3.1 [OMNI-GOD FINAL BUILD]
Official Documentation for AeonCoreX Modular Ecosystem

## 🛰 Introduction
X-Phage v3.1 "Omni-God" is a high-performance modular engine designed for next-gen decentralized applications. In this version, logic, design, and execution are decoupled into three distinct layers, ensuring maximum scalability and performance.

## 📂 File Ecosystem & Extensions
X-Phage now utilizes a specialized triple-file architecture:

1.  **.xp0 (Main Engine):** The core execution entry point. It contains the `pulse core` block which drives the entire system.
2.  **.xh (Logic Library):** Developer modules containing `#define` constants, `global` variables, and `~hook` hardware mappings.
3.  **.xui (UI Design):** User Interface schematics. Uses the `@NeuralComposition` decorator to define visual layouts.



## 📂 Project Structure
 * `/src`: Engine Core (Lexer, Linker, Runtime, Memory).
 * `/modules`: External libraries synced via XPM (Package Manager).
 * `/stdlib`: System default libraries (Aeon Core, StreamX, etc.).
 * `/bin`: Platform-optimized binaries (Linux, Windows, Android, macOS).

## ⌨️ Power Keywords & Functions

### 1. Global & Memory Phase (Logic Layer - .xh)
* **`global`**: Stores data in the Universal Matrix, accessible across all modules.
* **`atom`**: Declares an immutable constant (locked memory).
* **`shadow`**: Volatile local memory for high-speed dynamic processing.
* **`~link`**: Injects external modules or UI designs into the main engine.

### 2. Titan Fusion UI (Design Layer - .xui)
* **`@NeuralComposition`**: Defines the root of a UI design block.
* **`Signal`**: Text or status display component.
* **`Vision`**: Image, video, or camera feed container.
* **`Vortex / Orbit`**: Real-time animation and layout transition effects.
* **`Input`**: User interaction and data entry fields.

### 3. Advanced Hardware Control (Engine Layer - .xp0)
* **`bypass`**: Directly triggers the kernel to communicate with hardware, skipping OS overhead.
* **`quantum`**: Initiates multi-threaded neural processing.
* **`synapse`**: Establishes a secure bridge to external APIs (Spotify, Live TV, etc.).
* **`void`**: Security protocol that instantly purges sensitive data from RAM.

## 🚀 Execution Guide

### 1. Modular Linking Example:

// config.xh (Logic)
#define VERSION "3.1"
global SYS_MODE = "PERFORMANCE"
~hook gpu -> "matrix_accelerator"

// layout.xui (UI)
@NeuralComposition(MainHUD) {
    Signal "Omni-God Engine v3.1"
    Vortex(direction: "rotate", speed: "fast")
}

// main.xp0 (Execution)
~link "config.xh"
~link "layout.xui"

pulse core {
    beam "System Mode: " + SYS_MODE
    quantum "init_neural_sync"
    bypass hardware_auth { mode: "direct" }
}


### 2. Terminal Commands:
Compile: bash build.sh

Run: ./bin/xphage main.xp0

© 2026 AeonCoreX Intellectual Property. All Rights Reserved.