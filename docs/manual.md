## 🧬 X-Phage v3.4 Titan: The Definitive Technical Manual
Version: 3.4.0 "Stdlib" | Core: LLVM Optimized | Architecture: Neural-Native

---

## 🛰 1. Executive Summary
X-Phage is a high-performance, neural-native programming language built for the next generation of decentralized and hardware-accelerated applications. With version 3.4, it ships with a complete standard library (`stdlib`) and an enhanced package manager (XPM) for seamless module management.

Core Philosophy:
 * Decoupling: Separation of Logic (.xh), UI (.xui), and Execution (.xp0).
 * Zero Latency: Direct hardware access via the bypass protocol.
 * Portability: Run anywhere with zero dependencies using Docker or the stdlib.
 * Modularity: `~link` automatically resolves includes from `stdlib/` and `modules/`.

---

## 📦 2. Deployment & Environment

X-Phage is designed for instant setup across Linux, Windows, macOS, Android (Termux), and Cloud environments.

### 2.1 One‑Line Native Install
```bash
curl -sL https://raw.githubusercontent.com/AeonCoreX-Lab/X-Phage/main/install.sh | bash
```

2.2 Dockerized Execution (Recommended)

```bash
# Pull the latest engine
docker pull aeoncorex/x-phage:latest

# Check version
docker run --rm aeoncorex/x-phage --version

# Run your project
docker run --rm -v $(pwd):/app aeoncorex/x-phage run main.xp0
```

---

## 📂 3. Tri-File Modular System + Stdlib (Refined)

X-Phage enforces strict separation of concerns using a tri-file architecture:

| Extension | Layer Name   | Responsibility |
|----------|-------------|----------------|
| .xh      | Logic Layer  | Constants, global state, hardware hooks, core logic definitions |
| .xui     | UI Layer     | Declarative UI (Fusion Engine components like Vortex, Orbit, Vision) |
| .xp0     | Engine Layer | Entry point, execution flow, orchestration, async/quantum tasks |

### 🔗 Linking System
- `~link` resolves:
  - `stdlib/` (core official modules)
  - `modules/` (user + remote packages)

### 📁 Directory Structure

project/ ├── src/ │   ├── main.xp0 │   ├── app.xh │   └── layout.xui │ ├── stdlib/ │   ├── core/ │   ├── math/ │   ├── io/ │   ├── net/ │   ├── security/ │   └── ui/ │ ├── modules/ └── bin/

---

## 🧩 3.1 Stdlib Module Matrix (v3.4)

| Module Path        | Category   | Key Features |
|------------------|-----------|-------------|
| core/system.xh    | Core      | OS info, process control, memory stats |
| math/basic.xh     | Math      | Trigonometry, exponentials, logs |
| io/console.xh     | I/O       | Logging, colors, terminal control |
| net/http.xh       | Network   | REST APIs, GET/POST, async fetch |
| security/crypt.xh | Security  | AES-256, SHA-3, secure hashing |
| ui/fusion.xh      | UI        | Fusion Engine components |

### 🔬 Extended Capability Mapping

| Category  | Performance | Async Support | Hardware Access | Security Level |
|----------|------------|--------------|----------------|---------------|
| Core     | ⚡⚡⚡⚡⚡     | Yes          | Partial        | High          |
| Math     | ⚡⚡⚡⚡⚡     | Yes          | NPU Optimized  | Medium        |
| IO       | ⚡⚡⚡⚡       | Yes          | OS Layer       | Medium        |
| Network  | ⚡⚡⚡⚡       | Full Async   | No             | High          |
| Security | ⚡⚡⚡⚡⚡     | Yes          | Kernel Level   | Ultra         |
| UI       | ⚡⚡⚡⚡       | Reactive     | GPU (Vulkan)   | Medium        |


🛠 4. X‑Phage Package Manager (XPM)

XPM is built into the CLI. It manages both user‑created modules and the standard library.

Command Description
xphage sync <module> Downloads a module from the cloud into modules/ (supports subfolders like net/http).
xphage update-stdlib Downloads the entire standard library into stdlib/ with the full folder structure.
xphage init Creates a new project with src/, stdlib/, modules/, and bin/.

Example:

```bash
# Get the HTTP networking module
xphage sync net/http

# Update the whole standard library
xphage update-stdlib
```

---

⌨️ 5. Keyword & Function Lexicon

🟢 Basic: Data & Memory
 * global: Declares a variable in the Universal Registry. Accessible by all linked modules.
 * atom: An immutable constant. Once set, the memory address is locked at the hardware level.
 * shadow: Volatile, high-speed RAM allocation for temporary calculations.
 * beam: The primary output stream (Console/Log).

🟡 Intermediate: Fusion UI (UI/UX)
 * fusion: Initiates a UI rendering context.
 * Vortex / Orbit: Recursive layout components that handle 3D-style transitions.
 * Vision: Dedicated container for image processing and camera streams.
 * Input: Captures user interactions with neural-latency response.

🔴 Advanced: Titan Engine (Low-Level)
 * bypass: The "God Mode." Directly triggers kernel calls to communicate with hardware, skipping OS overhead.
 * quantum: Spawns asynchronous, multi-threaded processes for heavy background computation.
 * void: The Blackhole security protocol. It forces a hardware-level wipe of specific memory segments to prevent data leaks.
 * ~link: The intelligent module resolver. It connects .xh and .xui files and can auto-fetch missing libraries from the cloud.


## 🆕 New Stdlib Modules

· core/system.xh: OS info, process control.
· math/basic.xh: Trigonometry, exponents, logarithms.
· io/console.xh: Colored logging, terminal control.
· net/http.xh: REST API, GET/POST.
· security/crypt.xh: AES‑256, SHA‑3, Void protocol.
· ui/fusion.xh: UI components for the Fusion engine.

---

🚀 6. Mastering the Engine: Practical Examples

Example A: Using the Standard Library

```xphage
~link "core/system.xh"
~link "net/http.xh"
~link "ui/fusion.xh"

pulse main() {
    // Get system info
    sysgetinfo()
    
    // Fetch data from API
    shadow data = httpget("https://api.example.com/data")
    
    // Render a UI component
    fusion {
        Vortex(direction: "in") {
            Vision(data)
        }
    }
}
```

Example B: Project Setup with XPM

```bash
# Create project
xphage init
cd myproject

# Sync required modules
xphage sync net/http
xphage sync media/stream

# Build your app
xphage build src/main.xp0
```

---

## 📊 7. Feature Evolution Matrix (Refined)

| Feature            | v1.0        | v3.2 Titan        | v3.3 Titan Global     | v3.4 Stdlib (Current) |
|------------------|------------|------------------|----------------------|----------------------|
| Compiler         | Native C++ | LLVM -O3         | LLVM + Distributed   | LLVM + Stdlib Opt    |
| Architecture     | Monolithic | Modular          | Container-based      | Modular + Stdlib     |
| UI Engine        | None       | Recursive v2     | Fusion UI v3         | Fusion UI v4 (Vortex)|
| Multithreading   | No         | Basic Threads    | Distributed Compute  | Quantum Async Engine |
| Security         | None       | SHA-256          | Cloud Sandbox        | Void + Crypto Stdlib |
| Latency          | High       | Low              | Near Instant         | Zero-Latency Bypass  |
| Package Manager  | None       | Basic XPM        | Cloud Sync           | Full Stdlib + XPM    |
| Hardware Access  | None       | Limited          | GPU Enabled          | GPU + NPU + Bypass   |
| Portability      | Low        | Medium           | High (Docker)        | Universal Runtime    |

---

## 🧠 7.1 Feature Capability Matrix (Advanced View)

| Capability        | Support Level | Engine Component |
|------------------|-------------|------------------|
| Async Execution  | Full        | quantum          |
| Direct Hardware  | Full        | bypass           |
| Memory Control   | Ultra       | atom / shadow    |
| UI Rendering     | Advanced    | fusion           |
| Security Wipe    | Hardware    | void             |
| Module Linking   | Smart Auto  | ~link            |

---

🛡 8. Benefits of X-Phage Titan v3.4

· Extreme Speed: Direct LLVM machine code generation ensures your app runs at the speed of C++.
· Ironclad Security: The void protocol and atom isolation prevent 99% of memory‑based exploits.
· No Installation Hell: With the Docker image or the built‑in update-stdlib, your team starts coding in seconds.
· Hardware Native: Built‑in hooks for GPU (Vulkan) and NPU (Neural Sync) processing.
· Complete Standard Library: Ready‑to‑use modules for math, networking, crypto, UI, and more.

© 2026 AeonCoreX Intellectual Property. All Rights Reserved.
Next Phase: Real‑time Neural Synthesis Integration...