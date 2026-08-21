# X-Phage Standard Library

Nine canonical modules, matching the language book's Appendix C reference,
laid out the way Rust lays out its own standard library — one directory
per module, one `.xh` per module declaring its public signatures:

```
library/
├── io/io.xh
├── math/math.xh
├── string/string.xh
├── collections/collections.xh
├── net/net.xh
├── os/os.xh
├── crypt/crypt.xh
├── ai/ai.xh
└── solver/solver.xh
```

`fusion-ui` is **not** part of this directory — it is a Spore package
(installed via `xpm add fusion-ui`), not a built-in stdlib module, exactly
as the book and ecosystem specification describe.

## How `~link` resolves a module

`~link "math"` (or any of the other eight names above) is resolved by the
compiler at compile time:

1. The `.xh` file at `library/<name>/<name>.xh` is located and parsed,
   the same way any other source file is.
2. Its declarations (function signatures, `forge` types, `atom`
   constants) are merged into the program being compiled.
3. A real C++ implementation for the higher-value subset of each
   module's functions is inlined directly into the generated output —
   see "Implementation status" below for exactly which functions.

Module root resolution order (first match wins): `--library-path=` flag,
`$XPHAGE_HOME/library`, a path relative to the running executable, then
`./library`.

## Implementation status — honest, function-by-function

Matching the ecosystem specification's own discipline (Section 21.2):
distinguish clearly between "the signature exists" and "calling it does
something." Every function below either has a real, tested
implementation, or is forward-declared only (a clean linker error if
called, not silently wrong behavior).

| Module | Real implementation | Forward-declared only (not yet implemented) |
|---|---|---|
| **math** | All of it — arithmetic, trig, rounding, min/max/clamp, gcd/lcm, interpolation (lerp/smoothstep/bezier), random (rand/rand_int/rand_float/rand_gaussian), bit ops | vector/matrix/quaternion math (`vec2_*`, `vec3_*`, `mat4_*`, `quat_*`), statistics (`mean`, `median`, `stddev`, `variance`) |
| **string** | Search, transform (upper/lower/title/trim/reverse/repeat), slicing, split/join, replace, padding, type conversion (`str_to_int`, `int_to_str`, ...), regex (find/replace/split/match), formatting, UTF-8 helpers | Unicode normalization/case-folding beyond ASCII, locale-aware comparison |
| **io** | File read/write/append/copy/move/delete, directory listing/creation, path utilities (join/basename/dirname/extension), console I/O (`print`/`println`/`input`), environment variables, temp file/dir | Streaming/buffered readers, file watching, permissions |
| **collections** | Vec/Map/Set/Queue/Stack core operations, range/zip/enumerate/take/drop/chunk | Functions taking a function value as an argument (`vec_map`, `vec_filter`, `vec_reduce`, `vec_each`, `vec_sort_by`, `partition`, `take_while`, `drop_while`, `option_map`, `result_map`) — these need first-class function values, which is broader compiler support beyond this stdlib pass |
| **net** | — | Everything (`http_get`, `http_post`, TCP/UDP sockets, WebSocket) — needs libcurl integration |
| **crypt** | — | Everything (hashing, AES, RSA, base64) — needs OpenSSL integration |
| **ai** | — | Everything — needs a model-runtime integration (GGUF/ONNX or similar) |
| **solver** | — | Everything — Phase 10 per the language roadmap; this module exists only so `~link "solver"` resolves cleanly, not because an implementation is imminent |

`net`/`crypt`/`ai` are signature-complete (the full API surface from the
book is declared) but have no runtime behind them yet — they need an
external library dependency (libcurl, OpenSSL, a model runtime
respectively) that isn't available to link against in every environment
by default. Wiring them up is exactly the same mechanism described
below for reusing any other C/C++/Rust library: an `extern "C"` block
plus the appropriate `-l` flag.

## Reusing existing C/C++/Rust code

X-Phage is not limited to its own standard library. Any C-ABI-compatible
function — from a C library, a C++ library exposing an `extern "C"`
boundary, or a Rust `cdylib`/`staticlib` exporting functions as
`#[no_mangle] extern "C" fn ...` — can be called directly:

```xphage
extern "C" {
    pulse sqlite3_open(filename: str, db: *mut void) -> int
    pulse my_rust_function(a: int, b: int) -> int
}
```

Compile with the library linked in:

```bash
xphage build src/main.xp0 -L/path/to/libs -lmy_library
```

`str` parameters and return values crossing an `extern "C"` boundary are
automatically adapted to/from `const char*` — XPhage code keeps using
ordinary strings; the compiler inserts the `.c_str()` / wrapping for you.
Raw pointer types (`*mut T`, `*const T`) are supported for FFI signatures
that need them.
