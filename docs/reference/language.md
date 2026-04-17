# X-Phage Language Reference v3.5.0

## Keywords

| Keyword | Syntax | Description |
|---------|--------|-------------|
| `pulse` | `pulse name { ... }` | Declare a function/block |
| `atom` | `atom id = value` | Immutable variable |
| `shadow` | `shadow id = value` | Mutable variable |
| `global` | `global id = value` | Global registry variable |
| `beam` | `beam expr` | Print value to stdout |
| `bypass` | `bypass target { k: v }` | Hardware/kernel call |
| `quantum` | `quantum "task"` | Spawn async thread |
| `vortex` | `vortex` | Clear local Shadow RAM |
| `void` | `void` | Full memory wipe (VOID Protocol) |
| `chronos` | `chronos "ms"` | Sleep N milliseconds |
| `ether` | `ether "target" data` | Cloud/network uplink |
| `synapse` | `synapse "id" "api"` | Neural API handshake |
| `matrix` | `matrix id [size]` | GPU matrix allocation |
| `scan` | `scan id { ... }` | Inspect/type-check value |
| `~link` | `~link module/path` | Import stdlib or module |
| `fusion` | `fusion { ... }` | Titan UI composition |

## Data Types

- **atom** — immutable, stack-like (similar to `const`)
- **shadow** — mutable, heap-like (similar to `var`)
- **global** — persists in global registry across pulse calls

## Standard Library Import Paths

```xp0
~link core/types
~link core/system
~link io/file
~link io/console
~link net/http
~link net/socket
~link data/json
~link data/string
~link math/basic
~link math/linalg
~link media/engine
~link media/stream
~link security/crypt
~link ui/fusion
~link neural/bci
~link neural/lsl
~link alloc/alloc
```

## bypass Config Block

```xp0
bypass target_name {
    key1: "value1",
    key2: "value2"
}
```

The `bypass` statement invokes a native hardware, kernel, or runtime driver.
Keys and values are always strings unless the driver interprets them otherwise.

## fusion UI

```xp0
fusion {
    Signal("title")
    Orbit {
        Vision("image.png")
        Trigger("click_handler")
    }
}
```

## Example Program

```xp0
~link io/console
~link math/basic

global APP_VER = "3.5.0"

pulse greet(name) {
    beam "Hello, "
    beam name
    beam " — X-Phage " + APP_VER
}

pulse main {
    atom pi = MATH_PI
    log_info("Pi = " + pi)
    greet("World")
    quantum "background_task"
    chronos "100"
    vortex
}
```
