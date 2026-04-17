# Getting Started with X-Phage v3.5.0

## Installation

### One-line install (all platforms)

```bash
curl -sL https://raw.githubusercontent.com/AeonCoreX-Lab/X-Phage/main/scripts/install.sh | bash
```

Supported: **Linux x64/ARM64 · macOS Universal · Windows x64/ARM64 · Android (Termux)**

### Verify installation

```bash
xphage --version
# XPM v3.5.0 (X-Phage Package Manager)
```

---

## Your first program

Create `hello.xp0`:

```xp0
~link io/console

pulse main {
    log_info("Hello, X-Phage!")
    beam "Version 3.5.0 is live."
}
```

Run it:

```bash
xphage run hello.xp0
```

Compile to native binary:

```bash
xphage build hello.xp0
./output_app
```

---

## Project setup

```bash
mkdir my-project && cd my-project
xphage init
```

This creates:

```
my-project/
├── src/main.xp0
└── xphage.pkg
```

Edit `xphage.pkg`:

```toml
[package]
name    = "my-project"
version = "0.1.0"
author  = "Your Name"

[dependencies]
net-http = "latest"
```

Install dependencies:

```bash
xphage install net-http
xphage lock
```

---

## Using the standard library

```xp0
~link net/http
~link data/json
~link io/console

pulse main {
    log_info("Fetching data...")
    atom res = http_get("https://api.github.com")

    log_info("Status: " + res.status)
    beam res.body
}
```

---

## Neural interface (BCI)

```xp0
~link neural/bci

pulse main {
    bci_connect("openbci", "/dev/ttyUSB0")
    bci_stream_start()
    chronos "3000"
    bci_stream_stop()
    bci_save_csv("eeg.csv")
    bci_disconnect()
}
```

---

## Building from source

```bash
git clone https://github.com/AeonCoreX-Lab/X-Phage
cd X-Phage
bash scripts/build.sh
```

Optional environment variables:

| Variable | Default | Description |
|----------|---------|-------------|
| `BUILD_TYPE` | `Release` | `Debug` / `Release` / `RelWithDebInfo` |
| `ENABLE_LLVM` | `ON` | Use LLVM backend |
| `RUN_TESTS` | `0` | Run test suite after build |
| `JOBS` | `nproc` | Parallel compile jobs |

---

## Docker

```bash
docker pull aeoncorex/xphage:latest
docker run --rm -v $(pwd):/workspace aeoncorex/xphage run /workspace/hello.xp0
```
