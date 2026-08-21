# The XPhage Programming Language

**AeonCoreX Lab | v3.5.0**

*By the XPhage Team*

---

> *"Write once. Run everywhere. From silicon to the stars."*

---

## Foreword

XPhage was born from a simple frustration: every language makes you choose. You can have speed or safety. Expressiveness or control. High-level abstractions or bare-metal access. Native performance or cross-platform UI.

XPhage refuses to make that choice.

This book is for anyone who wants to write software that is fast, safe, expressive, and runs everywhere — from a space rover's onboard computer to a mobile app, from an AI inference engine to an OS kernel. No compromises.

Whether you are a C++ veteran tired of undefined behavior, a Python developer who needs more speed, a Rust programmer who wants simpler syntax, or a Kotlin developer who wants true native cross-platform UI — XPhage is for you.

### XPhage is a Multi-Generation Language

XPhage is the only language that gives you **3rd + 4th + 5th Generation** power in one coherent system:

| Generation | What it means | In XPhage |
|---|---|---|
| **3GL** — Systems | Full memory control, native speed, hardware access | `own/ref/mut_ref`, `unsafe`, LLVM, `@register` |
| **4GL** — Declarative | High-level abstractions, less code, more clarity | `flux/emit/absorb`, `select from where`, `filter/map` |
| **5GL** — Intelligence | Constraints, auto-optimization, AI-native compute | `@differentiable`, `solve {}`, `@gpu_kernel` |

**The golden rule:** 3GL and 4GL features are always available — no opt-in needed. 5GL features are completely opt-in using `@annotation` or special blocks. **Existing code never breaks. Never.**

```
┌─────────────────────────────────────────────────────┐
│              XPhage Power Levels                    │
├─────────────────────────────────────────────────────┤
│  🟢 3GL + 4GL — Always Available, Zero Setup        │
│                                                      │
│  Memory, speed, ownership, reactive UI,             │
│  query syntax, data pipelines                        │
├─────────────────────────────────────────────────────┤
│  🔴 5GL — Opt-In Only, Zero Cost When Unused        │
│                                                      │
│  @differentiable  — gradient auto-generation         │
│  @gpu_kernel      — GPU parallel dispatch            │
│  solve {}         — constraint-based solving         │
│  @smart_ownership — ownership inference              │
│                                                      │
│  Activate: add @annotation or write solve {}         │
│  Existing code: unchanged, unaffected                │
└─────────────────────────────────────────────────────┘
```

### A Honest Note on Current Status

XPhage is currently at **v3.5.0**, which is the version this book documents and the next release to ship. Phases 1–4 — the lexer, parser, AST, both compiler backends (the LLVM native backend and the C++17 transpiler fallback), and the full core language (variables, functions, control flow, `forge`/`nexus`/`impl`, `flux`/`emit`/`absorb`, ownership, async/await) — are **complete and stable** as of this version. Fusion UI's console backend is production-ready today. The GPU rendering backends (Vulkan, Metal, WebGPU) are in active development as part of Phase 5 — Chapter 16 shows the exact, honest status of each. Features marked with a phase number beyond 5 (e.g., "Phase 6," "Phase 7.5," "Phase 10") are planned and fully specified in this book, but not yet implemented — they describe where the language is going, not what v3.5.0 can do today.

---

## How to Read This Book

This book progresses from beginner to expert:

- **Chapters 1–3**: Setup, first programs, basic syntax
- **Chapters 4–6**: Core language features: types, functions, control flow
- **Chapters 7–9**: The type system: forge, nexus, impl, realm, enum
- **Chapters 10–11**: Reactive programming: flux, emit, absorb
- **Chapters 12–13**: Systems programming: ownership, memory
- **Chapters 14–15**: Standard library deep dive
- **Chapters 16–18**: Fusion UI framework
- **Chapters 19–20**: Advanced topics: async, AI/ML
- **Chapters 21–25**: 5th Generation Power (opt-in features)
- **Chapter 26**: Building real projects

---

# Part I — Getting Started

---

## Chapter 1: Installation and Hello World

### 1.1 Installing XPhage

**Linux / macOS:**
```bash
curl -sL https://xphage.dev/install | sh
```

**Windows:**
```powershell
irm https://xphage.dev/install | iex
```

This installs:
- `xphage` — the compiler
- `xpm` — the package manager
- `xforge` — the toolchain version manager
- Standard library

Verify:
```bash
xphage --version
# XPhage 3.5.0 (Titan Transpiler)

xpm --version
# XPM 1.0.0

xforge --version
# xforge 1.0.0
```

### 1.2 Your First Program

Create a file `hello.xp0`:

```xphage
beam "Hello, World!"
```

Run it:
```bash
xphage run hello.xp0
```

Output:
```
Hello, World!
```

Compile to a native binary:
```bash
xphage build hello.xp0
./hello
```

### 1.3 The Tri-Modular File System

XPhage's primary project layout uses three file types, each holding one layer of a program:

| Extension | Layer | Purpose |
|-----------|-------|---------|
| `.xh` | Logic Layer | Declarations: types, constants, function signatures |
| `.xui` | UI Layer | Fusion UI components |
| `.xp0` | Engine Layer | Execution, entry point, program logic |

Think of it like this:
- `.xh` is your API contract
- `.xui` is your visual design
- `.xp0` is your runtime behavior

```
my-app/
├── src/
│   ├── main.xp0       ← entry point
│   ├── models.xh      ← forge structs, nexus interfaces
│   ├── logic.xh       ← function signatures
│   └── screen.xui     ← UI
└── xpm.toml           ← package config
```

The compiler enforces layer discipline *within* each of these three extensions — the checks in §1.3.1 below (an execution statement in a `.xh` file, for instance, produces a warning) exist specifically because `.xh`/`.xui`/`.xp0` each promise to hold only one layer. This three-file split is the language's core, intended architecture for a real project — it stays the right structure to reach for as a codebase grows, since it's what keeps types, UI, and execution logic from tangling together. §1.3.1 covers a fourth extension, `.xp`, whose purpose is different: it exists specifically so a beginner (or anyone writing something small — a script, a one-off example, code for this book) doesn't have to learn or set up the three-file structure before writing their first working program. Reach for `.xp` while you're learning or prototyping; reach for the `.xh`/`.xui`/`.xp0` split once a project is real enough to benefit from the separation — `.xp` was not designed to replace it.

### 1.3.1 .xp — Single-File Programs, for Getting Started

`.xp` exists to remove a barrier, not to offer a second permanent architecture alongside the Tri-Modular one. The three-file split above is how a real XPhage project is meant to be organized as it grows — `.xp` is what you reach for before that split is worth doing: your first program, a script, a self-contained example, code while you're still learning the language. Nothing about `.xp` is a lesser or restricted version of the language (every feature in this book works the same way in a `.xp` file), and there's no requirement to "graduate" a working `.xp` file to three files by any particular point — but if a `.xp` file grows into something with real UI, a real API surface other code depends on, and real execution logic all tangled together, that's the signal that it's outgrown the format it's in, and §1.3's three-file structure is where it should move to.

A `.xp` file can freely mix all three layers — types, UI, and execution logic together in one file — and the compiler is not stricter about the mixing than that: there is no warning for combining a `forge` declaration with a `beam` statement in the same `.xp` file, the way there would be for putting a `beam` statement in a `.xh` file. Every `.xp` example used elsewhere in this book (and in the compiler's own golden test suite) is written this way:

```xphage
// greet.xp — types, functions, and execution all in one file
forge Person {
    name: str = ""
    age: int = 0
}

pulse greet(p: Person) -> str {
    return f"Hello, {p.name}! You are {p.age} years old."
}

atom nahid = spawn Person { name: "Nahid"   age: 28 }
beam greet(nahid)
```

```bash
xphage build greet.xp -o greet
./greet
```

**How the compiler handles a `.xp` file internally.** Even though nothing in the file is separated by extension, the compiler still classifies every top-level declaration into the same three layers `.xh`/`.xui`/`.xp0` represent (a `forge`/`pulse` signature is Logic, a `weave`/`strand` UI declaration is UI, a `beam`/loop/`if`/top-level statement is Execution) and reassembles them in Logic → UI → Execution order before generating code — the same ordering a three-file project would naturally have. This classification is purely internal bookkeeping for code generation; you don't write anything differently because of it, and declaration order in the source file doesn't matter (see §14 on declaration-order independence, which applies identically to `.xp` files and to `.xh`/`.xui`/`.xp0` projects).

**A `.xp` file that starts leaning on Tri-Modular siblings.** If a `.xp` file has no execution entry point of its own — just type and function declarations, nothing that would actually run — the compiler looks in the same directory for a file with the same base name that could supply the missing piece(s): `name.xh` for a missing Logic layer, `name.xui` for a missing UI layer, `name.xp0` for a missing Execution layer. If exactly one such file exists per missing layer, and none of its declared names collide with anything the `.xp` file already declares itself, it's merged in automatically:

```
project/
├── shapes.xp     ← forge Circle, forge Square (no execution entry point)
└── shapes.xp0    ← beam statements calling into shapes.xp's functions
```

```bash
xphage build shapes.xp    # automatically finds and merges shapes.xp0
```

If a `.xp` file already has its own execution entry point, this discovery step is skipped entirely — it compiles standalone even if a same-named `.xh`/`.xui`/`.xp0` happens to exist alongside it. And if a same-named sibling file *is* found but declares a symbol with the same name as something the `.xp` file already declares, the compiler treats that as two independent, unrelated files that happen to share a name (not a genuine multi-layer project) and refuses to guess — it prints a warning explaining the collision and compiles the `.xp` file alone, rather than silently picking one declaration over the other.

In practice, this sibling-discovery behavior mostly matters as a stepping stone: it lets a project move its execution logic into a real `.xp0` (or its types into a real `.xh`) one piece at a time, without having to relocate everything at once, while `shapes.xp` still works as the thing you `build`. Once every layer has moved out into its own file, there's nothing left mixed in `shapes.xp` and the project has arrived at the ordinary Tri-Modular layout from §1.3 — at that point you'd typically build the `.xp0` directly (`xphage build shapes.xp0`) rather than continue through the now-empty `.xp`. You can also always name every file explicitly instead of relying on discovery:

```bash
xphage build shapes.xp shapes.xp0
```

which is exactly what the block form of `extern "C"`, multi-file `.xh`+`.xp0` projects, and every other multi-file invocation in this book already does.

### 1.4 Your First Project

```bash
xpm new my-app
cd my-app
```

This creates:
```
my-app/
├── src/
│   └── main.xp0
├── xpm.toml
└── .gitignore
```

`src/main.xp0`:
```xphage
pulse main() {
    beam "Hello from XPhage!"
}
```

Build and run:
```bash
xphage build src/main.xp0 -o bin/my-app
bin/my-app
```

### 1.5 Comments

```xphage
// Single line comment

/*
   Multi-line comment
   spanning several lines
*/

// Comments can appear anywhere
atom x = 42  // inline comment
```

---

## Chapter 2: A Guessing Game

Before diving into details, let's build something fun — a number guessing game. This introduces variables, user input, conditionals, and loops.

```xphage
// guessing_game.xp0
~link "math"
~link "io"

pulse main() {
    beam "=== XPhage Guessing Game ==="
    beam "I'm thinking of a number between 1 and 100."
    beam ""

    atom secret = rand_int(1, 100)
    shadow attempts: int = 0
    shadow won: bool = false

    while !won {
        atom guess_str = input("Your guess: ")
        atom guess = str_to_int(guess_str)
        attempts = attempts + 1

        if guess < secret {
            beam "Too low! Try higher."
        } elif guess > secret {
            beam "Too high! Try lower."
        } else {
            won = true
            beam f"Correct! You got it in {attempts} attempts!"
        }
    }
}
```

Run it:
```bash
xphage run guessing_game.xp0
```

```
=== XPhage Guessing Game ===
I'm thinking of a number between 1 and 100.

Your guess: 50
Too high! Try lower.
Your guess: 25
Too low! Try higher.
Your guess: 37
Correct! You got it in 3 attempts!
```

Let's analyze what we used:
- `~link "math"` — imports the math stdlib for `rand_int`
- `~link "io"` — imports I/O for `input`, `str_to_int`
- `pulse main()` — the entry point function
- `atom` — immutable variable
- `shadow` — mutable variable with explicit type `: int`
- `while` loop with `!won` condition
- `if/elif/else` for comparison
- `f"..."` — string interpolation

---

# Part II — Core Language Features

---

## Chapter 3: Variables and Data Types

### 3.1 Variables

XPhage has two kinds of variables:

**`atom` — immutable (cannot be changed)**
```xphage
atom name    = "Nahid"
atom pi      = 3.14159
atom max_val = 1000
atom active  = true
```

Once set, `atom` values cannot be reassigned. This is the default — prefer `atom` when the value doesn't need to change.

**`shadow` — mutable (can be changed)**
```xphage
shadow count: int = 0
shadow message    = "Loading..."
shadow score      = 0.0

count   = count + 1
message = "Done!"
score   = score + 9.5
```

**Why this naming?**
- `atom` — atomic, immutable, fundamental like an atom
- `shadow` — like a shadow that shifts and changes

**Type inference:**
```xphage
atom x = 42        // inferred: int
atom y = 3.14      // inferred: float
atom z = "hello"   // inferred: str
atom b = true      // inferred: bool
```

**Explicit types:**
```xphage
atom x: int   = 42
atom y: float = 3.14
atom z: str   = "hello"
atom b: bool  = true
```

**`global` — module-level variable:**
```xphage
// In a .xh file
global version: str = "3.5.0"
global max_connections: int = 1024
```

Available everywhere in the module without passing as parameter.

**`const` — compile-time constant:**
```xphage
const PI:      float = 3.14159265358979
const MAX_BUF: int   = 65536
const APP:     str   = "XPhage"
```

Computed at compile time. Zero runtime cost.

### 3.2 The Type System

| Type | Description | Size | Example |
|------|-------------|------|---------|
| `int` | 64-bit signed integer | 8 bytes | `42`, `-100`, `0` |
| `float` | 64-bit floating point | 8 bytes | `3.14`, `-0.001` |
| `bool` | Boolean | 1 byte | `true`, `false` |
| `str` | String | dynamic | `"hello"` |
| `void` | No value | — | return type only |
| `auto` | Inferred | varies | compiler determines |

```xphage
atom a: int = 1_000_000    // underscores for readability
atom b: int = 0xFF          // hex literal
atom c: int = 0b1010        // binary literal
atom x: float = 1.0e9       // scientific notation
atom flag: bool = true
atom other      = !flag     // false
atom name: str = "XPhage"
atom multi      = "Line 1\nLine 2\nLine 3"
```

### 3.3 String Interpolation (f-strings)

```xphage
atom name  = "Nahid"
atom level = 42
atom score = 9850.5

beam f"Hello {name}"
beam f"Level: {level}, Score: {score}"
beam f"Double: {level * 2}"
beam f"Is high: {score > 9000}"

atom upper_name = str_upper(name)
beam f"Welcome {upper_name}!"
```

### 3.4 Type Conversion

```xphage
atom n   = str_to_int("42")
atom f   = str_to_float("3.14")
atom s   = int_to_str(1000)
atom fs  = float_to_str(3.14)
atom fs2 = float_to_str_prec(3.14159, 2)  // "3.14"

// Explicit cast
atom x: int   = 42
atom y: float = x as float
atom z: int   = 3.9 as int    // truncates to 3
```

### 3.5 Shadowing vs Mutability

```xphage
atom x = 5
beam x         // 5

atom x = x * 2     // new atom — shadows old one
beam x         // 10

atom x = f"Value is {x}"   // even change type
beam x         // "Value is 10"
```

---

## Chapter 4: Functions

### 4.1 Declaring Functions

```xphage
pulse greet() {
    beam "Hello!"
}

pulse greet_user(name: str) {
    beam f"Hello, {name}!"
}

pulse add(a: int, b: int) -> int {
    return a + b
}

pulse create_user(name: str, age: int, email: str) -> str {
    return f"{name}:{age}:{email}"
}
```

### 4.2 Calling Functions

```xphage
greet()
greet_user("Nahid")
atom result = add(10, 20)
atom user   = create_user("Nahid", 25, "nahid@example.com")
```

### 4.3 Return Values

```xphage
pulse max_val(a: int, b: int) -> int {
    if a > b { return a }
    return b
}

pulse find_first_negative(nums: str) -> int {
    atom parts = str_split(nums, ",")
    for part in parts {
        atom n = str_to_int(part)
        if n < 0 { return n }
    }
    return 0
}

pulse log_info(message: str) {
    atom timestamp = os_datetime()
    beam f"[{timestamp}] INFO: {message}"
}
```

### 4.4 Lambda Expressions

```xphage
atom double   = |x: int| x * 2
atom add_ten  = |x: int| x + 10
atom square   = |x: float| x * x
atom is_even  = |n: int| n % 2 == 0
atom greet_fn = |name: str| f"Hello {name}"

beam double(5)         // 10
beam add_ten(32)       // 42
beam greet_fn("World") // Hello World

// Multi-line lambda
atom process = |text: str| -> str {
    shadow result = str_trim(text)
    result = str_upper(result)
    return result
}

// Lambda with pipeline
atom clean_text = "  hello world  " |> str_trim |> str_upper
beam clean_text    // "HELLO WORLD"
```

### 4.5 The Pipeline Operator |>

```xphage
// Without pipeline (nested, hard to read)
atom result = str_upper(str_trim(str_replace(input, ",", "")))

// With pipeline (left-to-right, easy to read)
atom result = input
    |> str_replace(",", "")
    |> str_trim
    |> str_upper

// Phase 6: typed pipeline combinators
atom result: Vec<int> = numbers
    |> filter(|x| x > 0)
    |> map(|x| x * 2)
    |> sort()
    |> take(10)
```

### 4.6 Recursive Functions

```xphage
pulse factorial(n: int) -> int {
    if n <= 1 { return 1 }
    return n * factorial(n - 1)
}

pulse fibonacci(n: int) -> int {
    if n <= 1 { return n }
    return fibonacci(n - 1) + fibonacci(n - 2)
}

beam factorial(10)   // 3628800
beam fibonacci(10)   // 55
```

### 4.7 Functions as Values

```xphage
atom my_func = |x: int| x * x

pulse apply(fn: auto, value: int) -> int {
    return fn(value)
}

atom result = apply(|x: int| x * 3, 7)   // 21

pulse make_adder(n: int) -> auto {
    return |x: int| x + n
}

atom add5 = make_adder(5)
beam add5(10)    // 15
beam add5(100)   // 105
```

---

## Chapter 5: Control Flow

### 5.1 if / elif / else

```xphage
if temperature > 30 {
    beam "Hot day!"
}

if age >= 18 {
    beam "Adult"
} else {
    beam "Minor"
}

if score >= 90 {
    beam "A"
} elif score >= 80 {
    beam "B"
} elif score >= 70 {
    beam "C"
} elif score >= 60 {
    beam "D"
} else {
    beam "F"
}

// if as expression
atom category = if temperature > 30 {
    "hot"
} elif temperature > 20 {
    "warm"
} else {
    "cool"
}
beam f"Weather: {category}"
```

### 5.2 while Loops

```xphage
shadow i: int = 0
while i < 10 {
    beam i
    i = i + 1
}

// With break
shadow searching = true
shadow pos: int = 0
while searching {
    if data_at(pos) == target { searching = false }
    pos = pos + 1
    if pos > max_pos { break }
}

// With continue
shadow n: int = 0
while n < 20 {
    n = n + 1
    if n % 2 == 0 { continue }
    beam n
}
```

### 5.3 for / in Loops

```xphage
for i in range(0, 10) {
    beam i
}

for i in range_step(0, 100, 10) {
    beam i    // 0, 10, 20, ..., 90
}

atom fruits = "apple,banana,cherry"
for fruit in str_split(fruits, ",") {
    beam f"I like {fruit}"
}

// Enumerate (get index + value)
atom languages = "XPhage,Rust,C++,Python"
for entry in enumerate(str_split(languages, ",")) {
    atom parts = str_split(entry, ":")
    atom idx  = vec_get(parts, 0)
    atom lang = vec_get(parts, 1)
    beam f"{idx}. {lang}"
}
```

**Direct range syntax.** `range(start, end)` above is a standard-library function; XPhage also has a `..` range operator built directly into `for`, with no function call needed:

```xphage
for i in 0..10 {
    beam i    // 0, 1, 2, ..., 9 — same as range(0, 10)
}

pulse fib(n: int) -> int {
    if n <= 1 { return n }
    shadow a: int = 0
    shadow b: int = 1
    for i in 2..n {
        shadow t: int = a + b
        a = b
        b = t
    }
    return b
}
```

`start..end` is **half-open**: it includes `start` and every integer up to but not including `end`, exactly like `range(start, end)` — `2..n` above runs for `n - 2` iterations, not `n - 1`. Both bounds can be arbitrary integer expressions, not just literals (`for i in a..(b + 1)` is valid). There is currently no `..=` inclusive-range variant — write `start..(end + 1)` if you need the upper bound included.

### 5.4 probe / diverge — Pattern Matching

`probe` is XPhage's pattern matching — more powerful than `switch`:

```xphage
probe command {
    diverge "quit"    -> os_exit(0)
    diverge "help"    -> show_help()
    diverge "version" -> beam "XPhage v3.5.0"
    diverge _         -> beam f"Unknown command: {command}"
}

probe error_code {
    diverge 0   -> beam "Success"
    diverge 1   -> beam "Permission denied"
    diverge 404 -> beam "Not found"
    diverge 500 -> beam "Server error"
    diverge _   -> beam f"Error: {error_code}"
}

// Probe with blocks
probe action {
    diverge "create" -> {
        atom id = generate_id()
        create_item(id)
        beam f"Created item {id}"
    }
    diverge "delete" -> {
        confirm_deletion()
        delete_item(current_id)
        beam "Item deleted"
    }
    diverge _ -> { log_unknown_action(action) }
}

probe authenticated {
    diverge true  -> show_dashboard()
    diverge false -> show_login()
}
```

### 5.5 vortex — Error Handling

```xphage
// Basic
vortex {
    atom data = parse_dangerous_file("config.xh")
    process(data)
} catch(err) {
    beam f"Error: {err.message}"
    use_defaults()
} finally {
    cleanup_temp_files()
}

// Nested vortex
vortex {
    atom file = io_read("data.json")
    vortex {
        atom parsed = json_parse(file)
        use_data(parsed)
    }
}
```

### 5.6 The ? Error Propagation Operator

```xphage
// With ? — concise error propagation
pulse load_and_process(path: str) -> str {
    atom raw  = io_read(path)?
    atom data = json_parse(raw)?
    return process(data)
}
// ? means: if error, return error immediately
```

---

## Chapter 6: Operators and Expressions

### 6.1 Arithmetic

```xphage
atom a: int = 10
atom b: int = 3

beam a + b     // 13
beam a - b     // 7
beam a * b     // 30
beam a / b     // 3  (integer division)
beam a % b     // 1  (modulo)

atom x: float = 10.0
atom y: float = 3.0
beam x / y     // 3.333...
```

### 6.2 Comparison

```xphage
beam 5 == 5     // true
beam 5 != 3     // true
beam 5 >  3     // true
beam 5 <  3     // false
beam 5 >= 5     // true
beam 5 <= 4     // false
beam "abc" == "abc"    // true
```

### 6.3 Logical

```xphage
beam true && false    // false
beam true || false    // true
beam !true            // false

beam true and false   // false
beam true or false    // true
beam not true         // false

atom safe_divide = b != 0 && a / b > 0
```

### 6.4 Bitwise

```xphage
atom a: int = 0b1010   // 10
atom b: int = 0b1100   // 12

beam a & b    // 8  (AND)
beam a | b    // 14 (OR)
beam a ^ b    // 6  (XOR)
beam ~a       // bitwise NOT
beam a << 1   // 20 (left shift)
beam a >> 1   // 5  (right shift)
```

### 6.5 Compound Assignment

```xphage
shadow x: int = 10
x += 5    // 15
x -= 3    // 12
x *= 2    // 24
x /= 4    // 6
```

### 6.6 Operator Precedence

```
1. Function calls, method calls, indexing
2. Unary: - !
3. * / %
4. + -
5. << >>
6. < > <= >=
7. == !=
8. &  ^  |
9. && ||
10. |> (pipeline)
11. = += -= *= /=
```

---

# Part III — The Type System

---

## Chapter 7: forge — Structured Data

### 7.1 Declaring a forge

`forge` creates a structured data type (like a struct):

```xphage
// models.xh
forge Point {
    x: float = 0.0
    y: float = 0.0
}

forge Color {
    r: int = 0
    g: int = 0
    b: int = 0
    a: int = 255
}

forge User {
    name:   str   = ""
    email:  str   = ""
    age:    int   = 0
    score:  float = 0.0
    active: bool  = true
}
```

**Rules for forge (in `.xh`):**
- All fields must have a type annotation
- Default values are optional but recommended
- Cannot contain executable code

### 7.2 Creating Instances

```xphage
// main.xp0
~link "models.xh"

atom p    = spawn Point { x: 3.0, y: 4.0 }
atom red  = spawn Color { r: 255 }    // g=0, b=0, a=255
atom user = spawn User {
    name:   "Nahid"
    email:  "nahid@example.com"
    age:    25
    score:  98.5
    active: true
}
atom origin = spawn Point {}    // all defaults
```

### 7.3 Accessing Fields

```xphage
atom p = spawn Point { x: 3.0, y: 4.0 }
beam p.x
beam p.y
beam f"Point: ({p.x}, {p.y})"

shadow u = spawn User { name: "Nahid", age: 25 }
u.age   = 26
u.score = 99.0
beam u.age    // 26
```

### 7.4 Nested forge

```xphage
forge Address {
    street:  str = ""
    city:    str = ""
    country: str = ""
}

forge Person {
    name:    str     = ""
    age:     int     = 0
    address: Address = spawn Address {}
}

shadow person = spawn Person {
    name: "Nahid"
    age:  25
    address: spawn Address {
        street:  "123 Main St"
        city:    "Dhaka"
        country: "Bangladesh"
    }
}

beam person.address.city    // "Dhaka"
person.address.city = "Chittagong"
```

### 7.5 forge in Functions

```xphage
// Pass by value (copy)
pulse print_user(u: User) {
    beam f"User: {u.name} (age {u.age})"
}

// Pass by ref (immutable reference)
pulse display_user(ref u: User) {
    beam f"User: {u.name}"
    beam f"Email: {u.email}"
}

// Pass by mut_ref (mutable reference)
pulse birthday(mut_ref u: User) {
    u.age = u.age + 1
}

// Return forge
pulse create_user(name: str, email: str) -> User {
    return spawn User {
        name:  name
        email: email
        age:   0
    }
}

atom u = create_user("Nahid", "nahid@example.com")
print_user(u)
birthday(u)
beam u.age    // 1
```

### 7.6 Methods with impl

```xphage
// models.xh
forge Rectangle {
    width:  float = 0.0
    height: float = 0.0
}
```

```xphage
// logic.xh
impl Rectangle {
    area() -> float {
        return self.width * self.height
    }
    perimeter() -> float {
        return 2.0 * (self.width + self.height)
    }
    is_square() -> bool {
        return self.width == self.height
    }
    scale(factor: float) -> Rectangle {
        return spawn Rectangle {
            width:  self.width  * factor
            height: self.height * factor
        }
    }
}
```

```xphage
// main.xp0
atom rect = spawn Rectangle { width: 5.0, height: 3.0 }
beam rect.area()         // 15.0
beam rect.perimeter()    // 16.0
beam rect.is_square()    // false

atom big = rect.scale(2.0)
beam big.area()          // 60.0
```

---

## Chapter 8: nexus — Interfaces and Traits

### 8.1 Declaring a nexus

`nexus` defines a contract — what methods a type must provide:

```xphage
// interfaces.xh
nexus Drawable {
    draw()   -> void
    bounds() -> Rectangle
}

nexus Serializable {
    to_json()         -> str
    from_json(s: str) -> bool
}

nexus Comparable {
    compare_to(other: str) -> int   // -1, 0, 1
    equals(other: str)     -> bool
}

nexus Animal {
    speak() -> str
    move()  -> void
    name()  -> str
}
```

### 8.2 Implementing a nexus

```xphage
// animals.xh
forge Dog { name: str = ""  breed: str = "" }
forge Cat { name: str = ""  indoor: bool = true }
```

```xphage
// animals_impl.xh
impl Animal for Dog {
    speak() -> str  { return "Woof!" }
    move()  -> void { beam f"{self.name} runs" }
    name()  -> str  { return self.name }
}

impl Animal for Cat {
    speak() -> str  { return "Meow!" }
    move()  -> void { beam f"{self.name} slinks" }
    name()  -> str  { return self.name }
}
```

```xphage
// main.xp0
atom dog = spawn Dog { name: "Rex", breed: "Labrador" }
atom cat = spawn Cat { name: "Whiskers" }

beam dog.speak()    // Woof!
beam cat.speak()    // Meow!
dog.move()          // Rex runs
cat.move()          // Whiskers slinks
```

### 8.3 Multiple nexus Implementations

```xphage
forge Document {
    title:   str = ""
    content: str = ""
    author:  str = ""
}

impl Serializable for Document {
    to_json() -> str {
        return f"{{\"title\":\"{self.title}\",\"content\":\"{self.content}\",\"author\":\"{self.author}\"}}"
    }
    from_json(s: str) -> bool {
        self.title   = json_get(s, "title")
        self.content = json_get(s, "content")
        self.author  = json_get(s, "author")
        return true
    }
}

impl Drawable for Document {
    draw() -> void {
        beam f"=== {self.title} ==="
        beam f"By: {self.author}"
        beam ""
        beam self.content
    }
    bounds() -> Rectangle {
        return spawn Rectangle { width: 800.0, height: 600.0 }
    }
}
```

### 8.4 nexus with Default Implementations

```xphage
nexus Logger {
    // Abstract — must be implemented
    log(message: str) -> void

    // Default implementations — free for implementors
    log_info(msg: str)  -> void { self.log(f"[INFO]  {msg}") }
    log_warn(msg: str)  -> void { self.log(f"[WARN]  {msg}") }
    log_error(msg: str) -> void { self.log(f"[ERROR] {msg}") }
}
```

Any type implementing `Logger` only needs to define `log()`. The other methods come for free.

---

## Chapter 8.5: realm — Namespaces

> **A note on documentation status:** `realm` is a real, working v3.5.0 feature — the parser, semantic analyzer, and both C++ code generation backends (§Appendix D.1) all support it fully, including nested realms and qualified references. It has simply never been written up in this book or the ecosystem specification before now; the ecosystem specification's package-management section even lists "a namespace/realm system" among *future* language design concerns, which is no longer accurate — `realm` already exists and works today. This chapter fills that documentation gap.

`realm` groups related declarations — `forge` types, `pulse` functions, `const` values, `impl` blocks, and even other `realm`s — under a shared name, so they don't have to compete for global names and can be referred to as a qualified group.

### 8.5.1 Declaring a realm

```xphage
realm Geometry {
    const PI: float = 3.14159

    forge Vector2 {
        x: float = 0.0
        y: float = 0.0
    }

    pulse length_sq(v: Geometry::Vector2) -> float {
        return v.x * v.x + v.y * v.y
    }

    pulse circle_area(r: float) -> float {
        return PI * r * r
    }
}
```

Everything declared inside `realm Geometry { }` is qualified with the realm's name — the type is `Geometry::Vector2`, the function is `Geometry::length_sq`, and so on. Notice that `circle_area`'s body refers to `PI` unqualified — a `const` declared inside a realm is visible without its qualifier to other code *in the same realm*; code outside the realm must qualify it as `Geometry::PI`.

### 8.5.2 Using a realm's members from outside

Refer to a realm member with `::`, the same way the type is referenced in `length_sq`'s own parameter above:

```xphage
atom v = spawn Geometry::Vector2 { x: 3.0   y: 4.0 }
beam f"len_sq={Geometry::length_sq(v)}"    // len_sq=25
beam f"area={Geometry::circle_area(2.0)}"  // area=12.5664
```

### 8.5.3 use — Importing an Unqualified Name

Writing the full `Realm::name` path every time is sometimes more than you want. `use` brings a single qualified name into scope so you can refer to it unqualified afterward:

```xphage
use Geometry::circle_area

beam f"area={circle_area(2.0)}"   // no "Geometry::" needed anymore
```

`use` only affects the one name it names — it does not import every member of the realm. There is currently no wildcard `use Realm::*` form.

### 8.5.4 Nested realms

A `realm` can contain another `realm`, for grouping that's more than one level deep:

```xphage
realm App {
    realm Models {
        forge User { name: str = ""   score: int = 0 }
    }

    pulse rank(u: App::Models::User) -> str {
        if u.score >= 100 { return "champion" }
        return "player"
    }
}

atom u = spawn App::Models::User { name: "Nahid"   score: 150 }
beam App::rank(u)   // champion
```

Each level of nesting adds another `::`-separated segment — `App::Models::User` is fully qualified with both levels, while `App::rank` (declared directly inside `App`, not inside the nested `Models` realm) only needs one.

### 8.5.5 impl inside a realm

An `impl` block for a realm-declared type is written the same way as any other `impl`, and is itself scoped to the realm:

```xphage
realm Geometry {
    forge Vector2 { x: float = 0.0   y: float = 0.0 }

    impl Vector2 {
        length(self) -> float {
            return (self.x * self.x + self.y * self.y) as float
        }
    }
}
```



### 9.1 The enum Keyword (Phase 6)

XPhage has a first-class `enum` keyword for type-safe enumerations. Before Phase 6, `const str` workarounds were used — `enum` replaces those completely with compile-time safety.

```xphage
// status.xh
enum Status {
    Ok
    Loading
    Warning
    Error(str)      // data-carrying variant
    NotFound(int)   // error code
}

enum Direction {
    North
    South
    East
    West
}

enum Color {
    Red
    Green
    Blue
    Custom(int, int, int)   // RGB
}
```

### 9.2 Using enum with probe

```xphage
shadow status: Status = Status.Loading
load_data()
status = Status.Ok

probe status {
    diverge Status.Ok             -> beam "All good"
    diverge Status.Loading        -> beam "Please wait..."
    diverge Status.Warning        -> beam "Warning issued"
    diverge Status.Error(msg)     -> beam f"Error: {msg}"
    diverge Status.NotFound(code) -> beam f"Not found: {code}"
}
```

**Why enum beats const str:**
```xphage
// Old way — NOT type-safe (pre-Phase 6):
const STATUS_OK:      str = "ok"
const STATUS_WARN:    str = "warn"
const STATUS_ERROR:   str = "error"
const STATUS_LOADING: str = "loading"

probe status {
    diverge "okkk" -> beam "typo! compiler won't catch this"
}

// New way (Phase 6) — compile-time safe:
probe status {
    diverge Status.Okkk -> ...
    // [error] No variant 'Okkk' in enum Status
    //         Did you mean: Status.Ok?
}
```

> **Note:** Until Phase 6 lands, use `const str` pattern shown above — it works correctly and is backward compatible. The `enum` keyword will be a drop-in addition.

### 9.3 enum with Generics (Phase 6)

```xphage
// Built-in Option<T> — replaces option_is_some/option_unwrap
enum Option<T> {
    Some(T)
    None
}

// Built-in Result<T, E> — replaces result_is_ok/result_unwrap
enum Result<T, E> {
    Ok(T)
    Err(E)
}

// Usage
atom found: Option<User> = find_user(42)

probe found {
    diverge Option.Some(user) -> beam f"Found: {user.name}"
    diverge Option.None       -> beam "User not found"
}

atom result: Result<Config, str> = read_config("config.xh")

probe result {
    diverge Result.Ok(config)  -> apply_config(config)
    diverge Result.Err(reason) -> beam f"Config failed: {reason}"
}
```

**Current (pre-Phase 6) — using helper functions:**
```xphage
// Option pattern
atom found = find_user(42)
if option_is_some(found) {
    atom user = option_unwrap(found)
    beam f"Found: {user}"
} else {
    beam "User not found"
}
atom name = option_unwrap_or(find_name(id), "Anonymous")

// Result pattern
atom result = read_config("config.xh")
if result_is_ok(result) {
    atom config = result_unwrap(result)
    apply_config(config)
} else {
    atom err = result_error(result)
    beam f"Config error: {err}"
    use_defaults()
}
```

### 9.4 Tuple Types (Phase 6)

Phase 6 also introduces lightweight, anonymous tuple types alongside generics. A tuple groups a fixed number of values of (possibly different) types without requiring a named `forge` declaration — useful for quick, local groupings such as a coordinate pair or a labeled sample, where defining a whole `forge` would be unnecessary ceremony.

```xphage
// Declaring a tuple type
atom point: (float, float) = (3.0, 4.0)

// Positional field access with .0, .1, .2, ...
beam point.0   // 3.0
beam point.1   // 4.0

// Tuples as function return types
pulse min_max(values: Vec<int>) -> (int, int) {
    return (vec_min(values), vec_max(values))
}

atom (lo, hi) = min_max(numbers)   // destructuring on assignment
beam f"Range: {lo} to {hi}"

// Tuples inside collections (used throughout Chapter 21's training-loop examples)
atom dataset: Vec<(float, float)> = vec_from([(1.0, 2.0), (3.0, 6.0)])
for sample in dataset {
    atom x = sample.0
    atom y = sample.1
}
```

Tuples are intentionally minimal — they have no named fields and no methods. Reach for a `forge` instead of a tuple as soon as a grouping of values needs a name, a default, or behavior attached to it via `impl`; tuples are meant only for small, local, self-explanatory groupings like the ones shown above.

---

# Part IV — Reactive Architecture

---

## Chapter 10: flux — Reactive State

### 10.1 What is Reactive State?

```xphage
// Without flux — manual UI sync:
shadow score: int = 0
score = 100
// UI doesn't know score changed — must manually update

// With flux — automatic:
flux score: int = 0
score = 100
// Fusion UI automatically re-renders all components using score
// Zero manual synchronization
```

`flux` is the most important concept for building interactive applications in XPhage.

### 10.2 Declaring flux

```xphage
flux counter:    int   = 0
flux username:   str   = "Guest"
flux is_loading: bool  = false
flux progress:   float = 0.0

// In .xh files (shared state)
global flux app_theme: str = "dark"
global flux user_id:   int = -1
```

### 10.3 Reading flux

```xphage
beam counter
beam f"Hello {username}"
beam f"Progress: {progress}%"

if is_loading { beam "Please wait..." }
if user_id > 0 { beam "Logged in" } else { beam "Guest mode" }
```

### 10.4 Writing flux

```xphage
counter    = counter + 1
username   = "Nahid"
is_loading = true
progress   = 75.5

counter  += 10
counter  -= 5
progress *= 2.0
```

### 10.5 flux in Fusion UI

```xphage
// layout.xui
~link "fusion-ui"

flux count:   int = 0
flux message: str = "Ready"

fusion CounterScreen {
    Orbit(weave().padding(24)) {
        Vision(f"Count: {count}")      // auto re-renders
        Vision(f"Status: {message}")   // auto re-renders
        Trigger("Increment") { emit "inc" }
        Trigger("Reset")     { emit "reset" }
    }
}
```

```xphage
// main.xp0
absorb "inc" {
    count   = count + 1
    message = f"Count is now {count}"
}

absorb "reset" {
    count   = 0
    message = "Reset!"
}
```

The UI re-renders **only the nodes that use `count` and `message`** — not the whole screen. This is the diff engine at work.

### 10.6 Observing flux in Logic

```xphage
flux temperature: float = 20.0

absorb "temperature_update" {
    if temperature > 35.0 {
        send_alert("High temperature!")
    } elif temperature < 0.0 {
        send_alert("Freezing!")
    }
}

temperature = read_sensor()
emit "temperature_update"
```

---

## Chapter 11: emit / absorb — The Event Bus

### 11.1 The Event Bus Pattern

```xphage
// Without event bus — tight coupling:
pulse button_clicked() {
    update_counter()
    refresh_ui()
    save_to_database()
    send_analytics()
}

// With emit/absorb — loose coupling:
Trigger("Click") { emit "button_clicked" }

absorb "button_clicked" { update_counter() }
absorb "button_clicked" { refresh_ui() }
absorb "button_clicked" { save_to_database() }
absorb "button_clicked" { send_analytics() }
```

Adding or removing behavior is trivial — just add/remove `absorb`.

### 11.2 Emitting Events

```xphage
// Simple event (no data)
emit "app_started"
emit "user_logged_out"
emit "refresh_requested"

// Event with data
emit "user_login"  { username }
emit "purchase"    { product_id, price, quantity }
emit "error"       { code, message }
emit "progress"    { current, total }
emit "navigation"  { destination }
```

### 11.3 Absorbing Events

```xphage
absorb "app_started" {
    beam "Application started"
    load_config()
    init_database()
}

absorb "user_login" {
    beam f"Welcome {username}"
    load_user_profile(username)
    logged_in = true
}

absorb "purchase" {
    beam f"Processing order: {product_id}"
    atom receipt = process_payment(price)
    send_email(receipt)
    update_inventory(product_id, quantity)
}

absorb "error" {
    beam f"Error {code}: {message}"
    log_error(code, message)
    if code >= 500 { alert_admin(message) }
}
```

### 11.4 Event-Driven Architecture

```xphage
// Feature: User registration
absorb "register_start" {
    is_loading     = true
    status_message = "Creating account..."
}

absorb "register_validate" {
    atom valid = true
    if str_length(username) < 3 { valid = false; emit "register_error" { "Username too short" } }
    if str_length(password) < 8 { valid = false; emit "register_error" { "Password too weak" } }
    if valid { emit "register_submit" }
}

absorb "register_submit" {
    atom result = api_create_user(username, email, password)
    if result_is_ok(result) {
        emit "register_success"
    } else {
        emit "register_error" { result_error(result) }
    }
}

absorb "register_success" {
    is_loading     = false
    status_message = "Account created!"
    emit "navigate" { "dashboard" }
}

absorb "register_error" {
    is_loading     = false
    status_message = f"Error: {error_message}"
}

Trigger("Create Account") { emit "register_start"; emit "register_validate" }
```

### 11.5 Best Practices

```xphage
// Good — verb + noun
emit "user_logout"
emit "file_saved"
emit "message_sent"
emit "data_loaded"

// Bad — vague
emit "done"
emit "update"
emit "click"
```

```xphage
// Good — one responsibility per absorb
absorb "order_placed" { update_inventory(product_id) }
absorb "order_placed" { send_confirmation_email(user_email) }
absorb "order_placed" { notify_warehouse(order_id) }

// Bad — one absorb doing everything
absorb "order_placed" {
    update_inventory(product_id)
    send_confirmation_email(user_email)
    notify_warehouse(order_id)
    update_analytics()
    charge_payment()
}
```

---

# Part V — Systems Programming

---

## Chapter 12: Memory and Ownership

### 12.1 The Ownership Model

XPhage has a lightweight ownership system inspired by Rust but simpler:

| Keyword | Meaning | Use when |
|---------|---------|----------|
| `own` | Unique ownership | Large data, file handles, connections |
| `ref` | Immutable borrow | Read-only access, passing to functions |
| `mut_ref` | Mutable borrow | Need to modify without transferring ownership |
| Normal | Copy semantics | Small data (int, float, bool, short strings) |

### 12.2 own — Unique Ownership

```xphage
own shadow buffer = allocate(1024 * 1024)
own atom file     = open_file("data.bin")
own shadow conn   = db_connect("localhost")
// Automatically freed when out of scope — no GC, no manual free()
```

```xphage
pulse process_file(path: str) {
    own atom f = open_file(path)
    // use f...
    // f automatically closed when this function returns
}
```

### 12.3 ref — Immutable Borrow

```xphage
forge Image {
    width:  int = 0
    height: int = 0
    pixels: str = ""
}

// ref — cannot modify
pulse display_info(ref img: Image) {
    beam f"Image: {img.width}x{img.height}"
    beam f"Pixels: {str_length(img.pixels)}"
}

// Takes ownership
pulse save_image(img: Image, path: str) {
    io_write(path, img.pixels)
}

atom photo = spawn Image { width: 1920, height: 1080 }
display_info(photo)     // photo not moved — ref borrow
save_image(photo, "photo.bin")  // photo moved
```

### 12.4 mut_ref — Mutable Borrow

```xphage
pulse scale_image(mut_ref img: Image, factor: float) {
    img.width  = img.width  * factor as int
    img.height = img.height * factor as int
}

shadow photo = spawn Image { width: 1920, height: 1080 }
scale_image(photo, 0.5)
beam f"New size: {photo.width}x{photo.height}"  // 960x540
// photo still owned here
```

### 12.5 unsafe — When You Need Full Control

```xphage
// Hardware register access (embedded/OS)
unsafe {
    atom uart_addr = 0x10000000 as *mut int
    *uart_addr = 72    // write 'H' to UART
}

// Direct memory
unsafe {
    atom raw_ptr = allocate_raw(1024)
    write_raw(raw_ptr, data, size)
    free_raw(raw_ptr)
}

// FFI — calling C functions
unsafe {
    extern "C" pulse malloc(size: int) -> *mut void
    extern "C" pulse free(ptr: *mut void)

    atom mem = malloc(256)
    free(mem)
}
```

### 12.5.1 extern "C" — Calling Into C, C++, and Rust Libraries

`extern "C"` declares a function whose implementation lives outside the current program, in a compiled library — a C library, a C++ library exposing a C-compatible interface, or a Rust library built as a `cdylib`/`staticlib` with `#[no_mangle] extern "C"` functions. XPhage does not compile or link the library itself; it only declares the function's signature so the compiler knows how to call it, and you supply the actual library file (or `-l`/`-L` flags) on the command line.

**Declaring a single function:**

```xphage
extern "C" pulse native_add(a: int, b: int) -> int
```

**Declaring several at once (block form)** — equivalent to writing each one separately, just more convenient for a library with many entry points:

```xphage
extern "C" {
    pulse native_add(a: int, b: int) -> int
    pulse native_mul(a: float, b: float) -> float
    pulse native_greet(name: str) -> str
    pulse native_strlen(s: str) -> int
}
```

Both forms can appear at the top level of a file (they don't have to be wrapped in `unsafe { }` — the `unsafe` block in the example above groups an FFI declaration together with raw pointer use, but a plain `extern "C" pulse ...` declaration by itself doesn't require one).

**Calling an extern function** looks exactly like calling any other `pulse` — there is no special call syntax:

```xphage
extern "C" {
    pulse native_add(a: int, b: int) -> int
    pulse native_greet(name: str) -> str
}

atom sum = native_add(6, 7)
atom greeting = native_greet("Nahid")
beam f"sum={sum}"                 // sum=13
beam f"greeting={greeting}"       // greeting=Hello, Nahid!
```

**Type mapping across the FFI boundary.** Most XPhage types map directly to their obvious C equivalent (`int` → a C integer type, `float` → a C floating-point type, `bool` → a C `bool`/`int`). `str` is the one type that needs special handling: XPhage strings are not C strings internally, so the compiler automatically converts a `str` argument to a C-compatible `const char*` at the call site, and converts a `str`-typed return value back into a normal XPhage string — this happens transparently, so `native_greet("Nahid")` above just works without any manual conversion on your part. The corresponding C (or Rust) function signature should use `const char*` (Rust: `*const c_char`, typically read with `CStr::from_ptr`) for any `str`-typed parameter or return value.

**Linking the library.** An `extern "C"` declaration only tells the compiler the function's signature — it does not link the library that actually implements it. Pass the compiled library on the command line:

```bash
# Static library
xphage build main.xp0 libnative.a

# Dynamic library, via -l/-L (matches how you'd link it with any C compiler)
xphage build main.xp0 -L. -lnative
```

A Rust library built with `cargo build --release --crate-type=cdylib` (producing a `.so`/`.dylib`/`.dll`) or `--crate-type=staticlib` (producing a `.a`/`.lib`) links the same way — `extern "C"` is a contract about calling-convention and symbol naming, not about which language produced the library, so the XPhage side of the declaration is identical whether the library was written in C, C++ (with `extern "C"` on the C++ side to disable name mangling), or Rust.

```rust
// Rust side — must be #[no_mangle] and extern "C" so the symbol name
// and calling convention match what XPhage's declaration expects.
#[no_mangle]
pub extern "C" fn native_add(a: i32, b: i32) -> i32 {
    a + b
}
```

**A function name declared via `extern "C"` is never renamed or namespaced** by the compiler, even if the declaration appears inside a `realm` — the name must match the library's real exported symbol exactly, since that's what the linker looks for.

### 12.6 Memory Safety in Practice

```xphage
// Null pointer — impossible with Option<T>
atom user = find_user(42)   // returns Option, not nullable
if option_is_some(user) {
    atom u = option_unwrap(user)
    beam u.name    // safe — we checked
}

// Buffer overflow — bounds-checked
atom arr = vec_new()
vec_push(arr, "a")
atom item = vec_get(arr, 5)   // returns "" not crash

// Use-after-free — ownership prevents this
own atom data = load_data()
process(data)   // data moved into process
// beam data   // compile error: data no longer valid
```

---

## Chapter 13: Process, Environment, and the Shell

### 13.1 proc — Running Processes

```xphage
atom output     = proc "ls -la"
atom git_branch = proc "git rev-parse --abbrev-ref HEAD"
atom disk_usage = proc "df -h /"

beam f"Current branch: {git_branch}"

atom log_lines = proc "git log --oneline -10"
for line in str_split_lines(log_lines) {
    beam f"  {line}"
}
```

```xphage
pulse build_project() {
    beam "Building XPhage project..."

    atom cmake_out = proc "cmake -B build -S . -DCMAKE_BUILD_TYPE=Release"
    if str_contains(cmake_out, "Error") {
        beam f"CMake failed: {cmake_out}"
        return
    }

    atom make_out = proc "cmake --build build -j 8"
    beam f"Build output: {make_out}"
    beam "Build complete!"
}
```

### 13.2 bypass — Fire-and-Forget Commands

```xphage
bypass "mkdir -p build/release"
bypass "chmod +x scripts/install.sh"
bypass "rm -rf /tmp/xphage_cache"
bypass "git add -A"
bypass "git commit -m 'Auto-commit'"
```

### 13.3 env — Environment Variables

```xphage
atom home    = env.HOME
atom path    = env.PATH
atom api_key = env.OPENAI_API_KEY
atom port    = env.PORT
atom debug   = env.DEBUG

if debug == "true" {
    beam "Debug mode enabled"
    verbose_logging = true
}

atom server_port = if str_length(port) > 0 {
    str_to_int(port)
} else {
    8080
}
```

### 13.4 glob — File Pattern Matching

```xphage
~link "io"

atom xp0_files = glob "src/*.xp0"
atom xh_files  = glob "src/*.xh"
atom all_src   = glob "src/**/*.xp0"

for config_file in str_split(glob "config/*.toml", ",") {
    atom content = io_read(config_file)
    parse_config(content)
    beam f"Loaded: {config_file}"
}
```

### 13.5 Command-Line Arguments

```xphage
pulse main() {
    atom argc = os_argc()

    if argc < 2 {
        beam "Usage: myapp <command> [args]"
        os_exit(1)
    }

    atom command = os_arg(1)

    probe command {
        diverge "build"  -> cmd_build()
        diverge "run"    -> cmd_run()
        diverge "test"   -> cmd_test()
        diverge "clean"  -> cmd_clean()
        diverge "help"   -> show_help()
        diverge _ -> {
            beam f"Unknown command: {command}"
            beam "Run 'myapp help' for usage"
            os_exit(1)
        }
    }
}
```

---

# Part VI — Standard Library

---

## Chapter 14: Collections

> **Phase Note:** In the current version (Phase 1–5), collections store values as strings internally. Phase 6 introduces typed generics (`Vec<T>`, `Map<K,V>`) that eliminate this limitation. Both APIs are shown where relevant.

### 14.1 Vec — Dynamic Arrays

```xphage
~link "collections"

// Create and populate
shadow v = vec_new()
v = vec_push(v, "apple")
v = vec_push(v, "banana")
v = vec_push(v, "cherry")

// Access
beam vec_size(v)           // 3
beam vec_get(v, 0)         // "apple"
beam vec_get(v, -1)        // "cherry" (negative indexing)
beam vec_get(v, 1)         // "banana"
beam vec_contains(v, "banana")  // true

// Modify
v = vec_remove(v, 1)       // removes index 1
v = vec_insert(v, 0, "avocado")
v = vec_sort(v)
v = vec_reverse(v)
v = vec_slice(v, 1, 3)

// Numeric operations (current — numbers as strings)
shadow nums = vec_new()
nums = vec_push(nums, "10")
nums = vec_push(nums, "20")
nums = vec_push(nums, "30")
beam vec_sum(nums)     // 60.0
beam vec_min(nums)     // 10.0
beam vec_max(nums)     // 30.0

// Join
beam vec_join(v, ", ")    // "apple, banana, cherry"
```

### 14.2 Map — Key-Value Storage

```xphage
shadow m = map_new()
m = map_set(m, "name",    "Nahid")
m = map_set(m, "country", "Bangladesh")
m = map_set(m, "lang",    "XPhage")
m = map_set(m, "level",   "42")

beam map_get(m, "name")            // "Nahid"
beam map_get_or(m, "city", "N/A")  // "N/A"
beam map_has(m, "lang")            // true
beam map_size(m)                   // 4

atom keys   = map_keys(m)
atom values = map_values(m)
for key in str_split(keys, ",") {
    beam f"{key}: {map_get(m, key)}"
}

m = map_remove(m, "level")
atom combined = map_merge(m, other_map)
```

### 14.3 Set — Unique Collections

```xphage
shadow s = set_new()
s = set_add(s, "red")
s = set_add(s, "green")
s = set_add(s, "blue")
s = set_add(s, "red")    // duplicate — ignored

beam set_size(s)               // 3
beam set_contains(s, "red")    // true

atom u = set_union(s, other_set)
atom i = set_intersect(s, other_set)
atom d = set_difference(s, other_set)
s = set_remove(s, "blue")
atom v = set_to_vec(s)
```

### 14.4 Queue and Stack

```xphage
// Queue — FIFO
shadow q = queue_new()
q = queue_push(q, "first")
q = queue_push(q, "second")
q = queue_push(q, "third")

beam queue_peek(q)     // "first"
beam queue_size(q)     // 3
q = queue_pop(q)
beam queue_peek(q)     // "second"

// Stack — LIFO
shadow s = stack_new()
s = stack_push(s, "bottom")
s = stack_push(s, "middle")
s = stack_push(s, "top")

beam stack_peek(s)     // "top"
s = stack_pop(s)
beam stack_peek(s)     // "middle"
```

### 14.5 Range and Functional Operations (Current)

```xphage
// Range generation
atom r  = range(0, 10)
atom r2 = range_step(0, 20, 3)

// Zip two lists
atom names  = "Alice,Bob,Charlie"
atom scores = "95,87,92"
atom zipped = zip(names, scores)
// "Alice:95,Bob:87,Charlie:92"

for entry in str_split(zipped, ",") {
    atom parts = str_split(entry, ":")
    beam f"{vec_get(parts,0)} scored {vec_get(parts,1)}"
}

// Enumerate
atom fruits = "apple,banana,cherry"
for entry in str_split(enumerate(fruits), ",") {
    atom parts = str_split(entry, ":")
    beam f"{vec_get(parts,0)}. {vec_get(parts,1)}"
}
// 0. apple
// 1. banana
// 2. cherry
```

### 14.6 Typed Pipeline Combinators (Phase 6)

Phase 6 adds fully typed functional combinators that work with the `|>` operator:

```xphage
~link "collections"

// filter — keep elements where predicate is true
atom positives = numbers |> filter(|x| x > 0)

// map — transform each element
atom doubled = numbers |> map(|x| x * 2)

// flat_map — map + flatten
atom words = sentences |> flat_map(|s| str_split(s, " "))

// reduce — fold to single value
atom total = numbers |> reduce(0, |acc, x| acc + x)

// sort, sort_by, sort_by_desc
atom sorted    = numbers |> sort()
atom by_age    = users   |> sort_by(|u| u.age)
atom by_score  = users   |> sort_by_desc(|u| u.score)

// take, skip, take_while, skip_while
atom first10   = numbers |> take(10)
atom rest      = numbers |> skip(5)
atom small     = numbers |> take_while(|x| x < 100)

// deduplicate, group_by, zip, enumerate
atom unique    = names   |> sort() |> deduplicate()
atom by_dept   = users   |> group_by(|u| u.department)
atom pairs     = names   |> zip(scores)
atom indexed   = items   |> enumerate()

// aggregates
atom cnt   = numbers |> count()
atom sum   = numbers |> sum()
atom min   = numbers |> min()
atom max   = numbers |> max()
atom first = numbers |> first()
atom last  = numbers |> last()

// any, all
atom has_neg = numbers |> any(|x| x < 0)
atom all_pos = numbers |> all(|x| x > 0)

// Complex pipeline example
atom report = raw_logs
    |> filter(|log| log.level == "ERROR")
    |> map(|log| f"[{log.time}] {log.message}")
    |> sort_by(|s| s.timestamp)
    |> deduplicate()
    |> take(100)
```

### 14.7 Native Query Syntax (Phase 6.5)

Phase 6.5 adds SQL-style query syntax that compiles to Phase 6 combinators at zero runtime cost:

```xphage
// Basic query
shadow active_users =
    select from users
    where u.status == "active" and u.age > 18
    order by u.name asc
    limit 50

// With computed fields
shadow report =
    select f"[{e.level}] {e.message}"
    from error_log as e
    where e.severity > 2
    order by e.timestamp desc
    limit 20

// Aggregation
shadow total =
    select sum(o.amount)
    from orders as o
    where o.status == "completed"

// What the compiler generates internally (developer never sees this):
// users |> filter(|u| u.status == "active" && u.age > 18)
//       |> sort_by(|u| u.name) |> take(50)
// Zero runtime overhead — pure compile-time transformation
```

---

## Chapter 15: String Processing

```xphage
~link "string"

atom s = "  Hello, XPhage World!  "

beam str_length(s)
beam str_trim(s)
beam str_upper(s)
beam str_lower(s)
beam str_reverse(str_trim(s))

// Search
atom t = str_trim(s)
beam str_contains(t, "XPhage")
beam str_find(t, "XPhage")
beam str_starts_with(t, "Hello")
beam str_ends_with(t, "!")
beam str_count(t, "l")

// Slice
beam str_slice(t, 0, 5)         // "Hello"
beam str_slice_from(t, 7)       // "XPhage World!"
beam str_slice_to(t, 5)         // "Hello"
beam str_char_at(t, -1)         // "!" (last character)

// Replace and split
beam str_replace(t, "World", "Universe")
atom parts  = str_split(t, ", ")
atom joined = str_join(parts, " | ")
atom lines  = str_split_lines("Line1\nLine2\nLine3")

// Padding
beam str_pad_left("42", 6, '0')    // "000042"
beam str_pad_right("hi", 10)       // "hi        "
beam str_center("TITLE", 20, '=')  // "=======TITLE========"

// Type conversion
beam str_to_int("42")
beam str_to_float("3.14")
beam str_to_bool("true")
beam int_to_str(1000000)
beam float_to_str_prec(3.14159, 2)  // "3.14"
beam bool_to_str(true)

// Character checks
beam str_is_alpha("hello")    // true
beam str_is_digit("12345")    // true
beam str_is_alnum("abc123")   // true
beam str_is_upper("HELLO")    // true

// Regex
beam str_matches("abc123", "[a-z]+[0-9]+")
beam str_regex_replace("Hello 2026", "\\d+", "YEAR")
atom emails = str_regex_find_all(text, "[\\w.]+@[\\w.]+\\.\\w+")

// Encoding
beam str_to_hex("AB")
beam base64_encode("Hello")
atom decoded = base64_decode("SGVsbG8=")
```

---

# Part VII — Fusion UI

---

## Chapter 16: Introduction to Fusion UI

### 16.1 Philosophy

Fusion UI is XPhage's native cross-platform UI framework built from first principles:

- No Jetpack Compose (no JVM)
- No SwiftUI (no Objective-C runtime)
- No HTML/CSS/JavaScript (no browser engine)
- No React Native (no Node.js)

XPhage owns everything: the layout engine, diff engine, paint engine, and GPU backends per platform.

**The goal:** the same `.xui` file produces pixel-identical output on every platform.

### 16.2 Current Backend Status

> ⚠️ **Honest status as of v3.5.0:** The API is stable and fully specified. The GPU rendering backends are in active development. Every code example in this book runs today on the console backend.

| Backend | Platforms | Status | What Remains |
|---------|-----------|--------|--------------|
| **Console** | Development, CI, testing | ✅ Production ready | None |
| **Vulkan** | Android, Linux, Windows, Smart TV | 🔄 ~70% | GPU draw calls, SDF text, shadow blur |
| **Metal** | iOS, macOS, visionOS | 🔄 ~60% | SDF shaders, font atlas, CADisplayLink |
| **WebGPU** | Web (WASM) | 🔄 ~50% | Emscripten glue, JS event bridge, PWA |
| **OpenGL** | Older Android, Linux fallback | 🔄 ~40% | VBO management, GLSL pipeline |
| **OpenGL ES 3.0** | WatchOS, legacy Android | 🔜 Not started | Full implementation |
| **DirectX 12** | Windows native | 🔜 Not started | Stretch goal |

**What this means:**
- CLI tools and console apps: ✅ Production ready today
- Android apps: available when Vulkan reaches 100%
- iOS apps: available when Metal reaches 100%
- Web apps: available when WebGPU reaches 100%

### 16.3 Setting Up

```bash
xpm add fusion-ui
```

```xphage
~link "fusion-ui"
```

### 16.4 Your First UI

```xphage
// hello_ui.xui
~link "fusion-ui"

fusion HelloScreen {
    Orbit(weave().padding(24)) {
        Vision("Hello from Fusion UI!")
        Spacer(16)
        Trigger("Click Me") { emit "button_clicked" }
    }
}
```

```xphage
// main.xp0
~link "hello_ui.xui"

absorb "button_clicked" {
    beam "Button was clicked!"
}

pulse main() {
    xphage run HelloScreen
}
```

---

## Chapter 17: Layout and Components

### 17.1 Layout Components

**Orbit — Vertical Stack:**
```xphage
Orbit {
    Vision("Item 1")
    Vision("Item 2")
    Vision("Item 3")
}
```

**OrbitH — Horizontal Stack:**
```xphage
OrbitH {
    Vision("Left")
    Spacer(weight: 1)
    Vision("Right")
}
```

**Canvas — Free-form Box:**
```xphage
Canvas(weave().width(200).height(200)) {
    Vision("Top-left")
    Vision("Center", weave().offset(75, 90))
}
```

**Layer — Z-stack (overlapping):**
```xphage
Layer {
    Image("background.jpg", weave().fill())
    Orbit(weave().fill()) {
        Vision("Overlay text")
        Trigger("Click")
    }
}
```

**Mesh — Grid:**
```xphage
Mesh(cols: 3, gap: 8) {
    Vision("A")  Vision("B")  Vision("C")
    Vision("D")  Vision("E")  Vision("F")
    Vision("G")  Vision("H")  Vision("I")
}
```

**Scaffold — Page layout:**
```xphage
Scaffold {
    top_bar: OrbitH(weave().background("#6C63FF").padding(16)) {
        Vision("My App")
    }
    content: Orbit(weave().padding(16)) {
        Vision("Main content here")
    }
    bottom_bar: OrbitH(weave().background("#F5F5F5").padding(8)) {
        Trigger("Home")
        Trigger("Search")
        Trigger("Profile")
    }
    fab: Trigger("+", weave().corner_radius(28))
}
```

### 17.2 Content Components

```xphage
// Vision — Text
Vision("Simple text")
Vision(f"Dynamic: {score}")
Vision(long_text, weave().fill_width())

// Signal — Card/Surface
Signal(weave().corner_radius(12).elevation(2).padding(16)) {
    Vision("Card content")
    Trigger("Action")
}

// Image
Image("assets/logo.png", weave().width(100).height(100))
Image("https://example.com/photo.jpg", weave().fill_width().corner_radius(8))

// Spacer and Divider
Spacer(16)
Spacer(weight: 1)
Divider()
Divider(weave().padding_h(16))
```

### 17.3 Interactive Components

```xphage
// Trigger — Button
Trigger("Click Me") { emit "clicked" }

Trigger("Primary", weave()
    .background("#6C63FF")
    .corner_radius(8)
    .padding(12, 24)) {
    emit "primary_action"
}

// Input — Text Field
Input("Search...", weave()
    .fill_width()
    .corner_radius(24)
    .padding(12, 20)
    .background("#F0F0F0")) {
    absorb "on_change" { search_query = input_value }
}

// Toggle
flux dark_mode: bool = false
Toggle(dark_mode) {
    absorb "on_toggle" { dark_mode = !dark_mode }
}

// Slider
flux volume: float = 50.0
Slider(volume, 0, 100) {
    absorb "on_slide" { volume = slider_value }
}
Vision(f"Volume: {volume}%")
```

### 17.4 The weave Modifier System

```xphage
// Size
weave().width(200)
weave().height(100)
weave().fill_width()
weave().fill_height()
weave().fill()
weave().weight(1)
weave().min_width(100)
weave().max_height(300)
weave().aspect_ratio(16.0/9.0)

// Spacing
weave().padding(16)
weave().padding(8, 16)       // vertical, horizontal
weave().padding(4, 8, 4, 8)  // top, right, bottom, left
weave().padding_h(16)
weave().padding_v(8)
weave().offset(10, -5)

// Appearance
weave().background("#1A1A2E")
weave().alpha(0.8)
weave().corner_radius(12)
weave().corner_radius(16, 16, 0, 0)
weave().border_width(1)
weave().border_color("#DDDDDD")
weave().elevation(4)
weave().rotate(45)
weave().scale(1.5)

// Interaction
weave().clickable { emit "clicked" }
weave().focusable()
weave().scrollable_v()
weave().scrollable_h()

// Composition and reuse
atom card_style = weave()
    .fill_width()
    .corner_radius(16)
    .elevation(3)
    .padding(16, 24)
    .background("#FFFFFF")

Signal(card_style) { Vision("Card 1") }
Signal(card_style) { Vision("Card 2") }
```

---

## Chapter 18: Animation and Themes

### 18.1 strand — Animations

```xphage
// Tween: animate from value A to B over time
strand fade_in {
    tween(alpha: 0.0 -> 1.0, duration: 300, easing: ease_out)
}

strand slide_up {
    tween(offset_y: 100 -> 0, duration: 350, easing: ease_in_out)
}

strand grow {
    tween(scale: 0.8 -> 1.0, duration: 200, easing: ease_out)
}

// Spring: physics-based, feels natural
strand bounce_in {
    spring(scale: 0.6 -> 1.0, stiffness: 400, damping: 28)
}

strand wobble {
    spring(rotation: -5 -> 0, stiffness: 200, damping: 10)
}

// Presets: gentle, wobbly, stiff, snappy, slow
strand pop {
    spring(scale: 0.8 -> 1.0, preset: snappy)
}

// Easing functions:
// ease_in      — starts slow, ends fast
// ease_out     — starts fast, ends slow (most natural)
// ease_in_out  — slow start and end
// linear       — constant speed
// bounce       — bounces at the end
// elastic      — overshoots and springs back
```

### 18.2 Theme System

```xphage
// Built-in themes
xphage run MyApp with Theme.light()
xphage run MyApp with Theme.dark()
xphage run MyApp with Theme.aeon()    // AeonCoreX futuristic dark

// Custom theme
atom my_theme = spawn Theme {
    colors: spawn ColorScheme {
        primary:    "#FF6B6B"
        secondary:  "#4ECDC4"
        background: "#1A1A2E"
        on_surface: "#EEEEEE"
    }
    dark_mode:  true
    font_scale: 1.1
}

xphage run MyApp with my_theme
```

### 18.3 Responding to Theme

```xphage
flux app_theme: str = "dark"

probe app_theme {
    diverge "dark" -> Orbit(weave().background("#121212")) {
        Vision("Dark mode", weave().color("#EEEEEE"))
    }
    diverge "light" -> Orbit(weave().background("#FAFAFA")) {
        Vision("Light mode", weave().color("#121212"))
    }
}
```

---

# Part VIII — Advanced Topics

---

## Chapter 19: Async Programming

### 19.1 async pulse

```xphage
async pulse fetch_data(url: str) -> str {
    atom response = await http_get(url)
    return response.body
}

async pulse load_user(id: int) -> User {
    atom data = await http_get(f"https://api.example.com/users/{id}")
    atom json  = data.body
    return spawn User {
        name:  json_get(json, "name")
        email: json_get(json, "email")
        age:   str_to_int(json_get(json, "age"))
    }
}
```

### 19.2 await — Waiting for Results

```xphage
async pulse main_async() {
    // Sequential — each waits for previous
    atom user    = await load_user(42)
    atom profile = await load_profile(user.name)
    atom posts   = await load_posts(user.name)

    beam f"User: {user.name}"
    beam f"Posts: {vec_size(posts)}"
}
```

### 19.3 Concurrent Operations

```xphage
async pulse load_dashboard() {
    // Start all concurrently with quantum
    quantum "load_user_data"
    quantum "load_analytics"
    quantum "load_notifications"

    // All three run in parallel
    absorb "user_data_ready"     { update_user_section(data) }
    absorb "analytics_ready"     { update_charts(analytics) }
    absorb "notifications_ready" { update_badge(count) }
}
```

---

## Chapter 20: AI and Machine Learning

### 20.1 Tensor Operations

```xphage
~link "ai"

atom zeros   = tensor_zeros("3x4")
atom ones    = tensor_ones("128x768")
atom random  = tensor_rand("512x512")
atom data    = tensor_from_data("2x3", "1.0,2.0,3.0,4.0,5.0,6.0")

atom sum     = tensor_add(a, b)
atom product = tensor_matmul(a, b)
atom scaled  = tensor_scale(a, 0.5)
atom normed  = tensor_normalize(a)

// GPU/NPU acceleration
atom fast_a  = tensor_to_gpu(a)
atom npu_a   = tensor_to_npu(a)
atom cpu_r   = tensor_to_cpu(result)

// Neural operations
atom relu    = nn_relu(input)
atom softmax = nn_softmax(logits, 1)
atom out     = nn_linear(input, weight, bias)
atom attn    = nn_attention(q, k, v, mask)
```

### 20.2 Loading Models

```xphage
// ONNX model
atom model  = model_load("sentiment.onnx", "gpu")
atom result = model_run(model, text_embedding)

atom result2 = model_run_named(model,
    "input=text_tensor,output=logits")
```

### 20.3 LLM Inference

```xphage
~link "ai"

atom llm = llm_load("llama-3.2-3b-instruct.gguf",
    4096,    // context length
    32       // GPU layers
)

atom response = llm_generate(llm,
    "Explain quantum computing in simple terms:",
    512
)
beam response

atom creative = llm_generate_ex(llm,
    "Write a haiku about XPhage:",
    100,
    0.9,    // temperature
    0.95    // top_p
)
beam creative

// Chat
atom messages = "[{\"role\":\"user\",\"content\":\"Hello!\"}]"
atom reply    = llm_chat(llm, messages)
beam reply

// Embeddings
atom embedding = llm_embed(llm, "XPhage is a systems language")
```

### 20.4 Vector Database (RAG)

```xphage
atom db = vecdb_new(768)

atom docs = [
    "XPhage is a compiled language",
    "Fusion UI uses Vulkan on Android",
    "forge creates struct types"
]

for doc in str_split(docs, ",") {
    atom embed = llm_embed(llm, doc)
    vecdb_add(db, doc, embed, doc)
}

atom query   = llm_embed(llm, "how to create a structure?")
atom results = vecdb_search(db, query, 3)
beam results

vecdb_save(db, "knowledge_base.vdb")
atom db2 = vecdb_load("knowledge_base.vdb")
```

### 20.5 GPU Compute with @gpu_kernel (Phase 7)

> **5GL Opt-In Feature** — requires `~link "ai"`. Existing code is completely unaffected.

```xphage
~link "ai"

// @gpu_kernel — runs this function on GPU
@gpu_kernel
pulse matrix_multiply(
    a: Tensor<float>,
    b: Tensor<float>
) -> Tensor<float> {
    atom row = gpu_thread_x()
    atom col = gpu_thread_y()
    shadow sum: float = 0.0
    for k in range(0, a.cols()) {
        sum = sum + a[row, k] * b[k, col]
    }
    return sum
}

// accelerate — SIMD auto-vectorization on CPU
accelerate
pulse dot_product(a: Vec<float>, b: Vec<float>) -> float {
    return zip(a, b)
        |> map(|(x, y)| x * y)
        |> sum()
    // Compiler generates AVX-512 on x86, NEON on ARM automatically
}

// NPU dispatch with fallback chain
pulse run_inference(model: Model, input: Tensor<float>) -> Tensor<float> {
    accelerate npu {
        return model.forward(input)
        // NPU → GPU → CPU (automatic fallback)
    }
}
```

---

# Part IX — 5th Generation Power (Opt-In)

> **Important:** Everything in this Part is completely opt-in. None of these features affect existing code. A developer can use XPhage for years without ever needing anything in this section. 5GL features activate only when you explicitly write the annotation or block.

---

## Chapter 21: @differentiable — Automatic Differentiation

> **5GL Opt-In Feature — Phase 7.5.** Requires `~link "ai"`. No effect on any function that does not carry the `@differentiable` annotation.

### 21.1 The Problem Without @differentiable

Training ML models requires gradients. Without auto-diff, you compute them manually:

```xphage
// Manual gradient — one mistake ruins training forever
pulse loss(w: float, x: float, y: float) -> float {
    atom pred = w * x
    return (pred - y) * (pred - y)
}

// Must derive and implement manually — error-prone:
// d/dw [(w*x - y)^2] = 2*(w*x - y)*x
pulse loss_gradient_MANUAL(w: float, x: float, y: float) -> float {
    return 2.0 * (w * x - y) * x
    // One sign error here → model never converges
}
```

### 21.2 @differentiable — Opt-In Gradient Generation

```xphage
~link "ai"

// Just add @differentiable — that is all
@differentiable
pulse loss(w: float, x: float, y: float) -> float {
    atom pred = w * x
    return (pred - y) * (pred - y)
}

// Compiler automatically generates:
// loss_grad_w(w, x, y) -> float  — d(loss)/d(w)
// loss_grad_x(w, x, y) -> float  — d(loss)/d(x)
// loss_grad_y(w, x, y) -> float  — d(loss)/d(y)

// Training loop using auto-generated gradient:
pulse train(own data: Vec<(float, float)>, epochs: int) -> float {
    shadow w:  float = 0.0
    atom   lr: float = 0.01

    for epoch in range(0, epochs) {
        for sample in data {
            atom x    = sample.0
            atom y    = sample.1
            atom grad = loss_grad_w(w, x, y)  // auto-generated
            w         = w - lr * grad
        }
    }
    return w
}
```

### 21.3 Neural Network with @differentiable

```xphage
~link "ai"

@differentiable
pulse linear_layer(
    own weights: Vec<Vec<float>>,
    own bias:    Vec<float>,
    input:       Vec<float>
) -> Vec<float> {
    return matmul(weights, input)
        |> zip(bias) |> map(|(a, b)| a + b)
        |> map(|x| relu(x))
}
// Compiler auto-generates: linear_layer_grad_weights, linear_layer_grad_bias

@differentiable
pulse full_network(
    own w1: Vec<Vec<float>>,
    own w2: Vec<Vec<float>>,
    input:  Vec<float>
) -> Vec<float> {
    atom h1  = linear_layer(w1, zeros(64), input)
    atom out = linear_layer(w2, zeros(10), h1)
    return nn_softmax(out, 0)
}

pulse train_network(own dataset: Vec<(Vec<float>, int)>) {
    shadow w1 = tensor_rand("64x784") as Vec<Vec<float>>
    shadow w2 = tensor_rand("10x64")  as Vec<Vec<float>>
    atom lr   = 0.001

    for epoch in range(0, 10) {
        for (x, label) in dataset {
            atom pred     = full_network(w1, w2, x)
            atom loss_val = cross_entropy_loss(pred, label)

            // All gradients auto-generated
            atom dw1 = full_network_grad_w1(w1, w2, x)
            atom dw2 = full_network_grad_w2(w1, w2, x)

            w1 = w1 |> zip(dw1) |> map(|(w, g)| w - lr * g)
            w2 = w2 |> zip(dw2) |> map(|(w, g)| w - lr * g)
        }
        beam f"Epoch {epoch}: loss = {loss_val}"
    }
}
```

### 21.4 Functions Without @differentiable — Zero Change

```xphage
// No @differentiable = completely normal function = zero overhead
pulse loss(w: float, x: float, y: float) -> float {
    atom pred = w * x
    return (pred - y) * (pred - y)
}
// Compiles exactly as before. No gradient code generated.
// Adding @differentiable to other functions does not affect this.
```

### 21.5 Supported Differentiable Operations

| Expression | Derivative rule applied |
|---|---|
| `a + b` | `da + db` |
| `a - b` | `da - db` |
| `a * b` | `a*db + b*da` |
| `a / b` | `(da*b - a*db) / b²` |
| `relu(x)` | `x > 0 ? dx : 0` |
| `sigmoid(x)` | `sigmoid(x) * (1 - sigmoid(x)) * dx` |
| `tanh(x)` | `(1 - tanh(x)²) * dx` |
| `log(x)` | `dx / x` |
| `exp(x)` | `exp(x) * dx` |
| `matmul(A, B)` | `dA·Bᵀ + Aᵀ·dB` |

---

## Chapter 22: solve {} — Constraint Solving

> **5GL Opt-In Feature — Phase 10 (long-term, alongside formal verification).** Requires `~link "solver"` and `solver` in `xpm.toml`. This is the most distant feature on the entire roadmap described in this book — it is documented here so the design is recorded, not because it is coming soon.

### 22.1 The Problem solve {} Solves

Some problems are naturally expressed as constraints, not as step-by-step algorithms:

```xphage
// Without solve: developer writes Dijkstra manually (~80 lines)
pulse shortest_path_manual(graph: Graph, start: Node, end: Node) -> Vec<Node> {
    shadow dist  = map_new()
    shadow prev  = map_new()
    shadow queue = priority_queue_new()
    // ... 75 more lines of algorithm ...
}

// With solve: express what you want, not how to get it
pulse shortest_path(graph: Graph, start: Node, end: Node) -> Vec<Node> {
    ~link "solver"
    solve {
        find: path as Vec<Node>
        subject_to {
            path.first() == start
            path.last()  == end
            all n in path -> graph.contains(n)
            all i in range(0, path.len() - 1) ->
                graph.has_edge(path[i], path[i + 1])
        }
        minimize: path |> map(|i| graph.edge_cost(path[i], path[i+1])) |> sum()
    }
}
```

### 22.2 solve {} Syntax

```xphage
~link "solver"    // explicit opt-in required

solve {
    // What to find — typed variable
    find: <variable_name> as <Type>

    // All constraints must be satisfied
    subject_to {
        <constraint_expression>
        <constraint_expression>
    }

    // Optimization objective — optional
    minimize: <expression>   // or: maximize: <expression>

    // Performance hints — optional
    hints {
        timeout:  5000                   // max milliseconds
        strategy: "branch_and_bound"
    }
}
```

### 22.3 Real-World Examples

**Job Scheduling:**
```xphage
~link "solver"

pulse schedule_jobs(jobs: Vec<Job>, workers: Vec<Worker>) -> Map<Job, Worker> {
    solve {
        find: assignment as Map<Job, Worker>
        subject_to {
            all j in jobs -> map_has(assignment, j)
            all j in jobs ->
                map_get(assignment, j).skills |> any(|s| s == j.required_skill)
            all w in workers ->
                jobs |> filter(|j| map_get(assignment, j) == w)
                     |> map(|j| j.hours)
                     |> sum() <= w.max_hours
        }
        minimize: jobs |> map(|j| map_get(assignment, j).cost * j.hours) |> sum()
    }
}
```

**Resource Allocation:**
```xphage
~link "solver"

pulse allocate_servers(jobs: Vec<Job>, servers: Vec<Server>) -> Map<Job, Server> {
    solve {
        find: placement as Map<Job, Server>
        subject_to {
            all j in jobs -> map_has(placement, j)
            all s in servers ->
                jobs |> filter(|j| map_get(placement, j) == s)
                     |> map(|j| j.memory_mb) |> sum() <= s.total_memory
            all s in servers ->
                jobs |> filter(|j| map_get(placement, j) == s)
                     |> map(|j| j.cpu_cores) |> sum() <= s.total_cpu
        }
        minimize: servers |> filter(|s|
            jobs |> any(|j| map_get(placement, j) == s)) |> count()
    }
}
```

### 22.4 Manual and solve {} Always Coexist

```xphage
// Option A: manual — always available, full control
pulse find_path_manual(g: Graph, s: Node, e: Node) -> Vec<Node> {
    // Your own Dijkstra, A*, BFS — unchanged 3GL power
}

// Option B: constraint-based — opt-in
pulse find_path_auto(g: Graph, s: Node, e: Node) -> Vec<Node> {
    ~link "solver"
    solve {
        find: path as Vec<Node>
        subject_to { path.first() == s  path.last() == e }
        minimize: path_cost(path, g)
    }
}
// Both are valid. Developer decides which to use.
```

### 22.5 Missing Dependency Error

```
// If ~link "solver" is missing:
[error E5002] solve {} requires ~link "solver"
              Add to xpm.toml: [dependencies] solver = "1.0"
              Then add: ~link "solver" to your .xp0 file
              Documentation: docs.xphage.dev/solver
```

---

## Chapter 23: @requires / @ensures — Design by Contract

> **Advanced 3GL Feature — Phase 8.** No opt-in import needed. Contracts are scheduled alongside bare-metal support (Chapter 25) because both rely on the same static-analysis pass the compiler gains in Phase 8 — but `@requires`/`@ensures` are general-purpose and just as useful in ordinary application code as in embedded code; they are not limited to bare-metal targets.

### 23.1 The Problem

```xphage
// Silent contracts — developer must read docs:
pulse binary_search(arr: Vec<int>, target: int) -> int {
    // arr unsorted → wrong result, no error
    // arr empty → crash
    // No compiler warning for either
}
```

### 23.2 @requires and @ensures

```xphage
@requires(arr.len() > 0,       "Array cannot be empty")
@requires(vec_is_sorted(arr),  "Array must be sorted for binary search")
@ensures(result >= -1,         "Returns -1 if not found")
@ensures(result < arr.len(),   "Index must be within bounds")
pulse binary_search(arr: Vec<int>, target: int) -> int {
    // implementation
}

@requires(b != 0.0,            "Divisor cannot be zero")
@ensures(result * b ~= a,      "Result satisfies a/b relationship")
pulse divide(a: float, b: float) -> float {
    return a / b
}

@requires(str_length(email) > 0,       "Email required")
@requires(str_contains(email, "@"),    "Valid email required")
@requires(age >= 0 and age <= 150,     "Age must be realistic")
@ensures(result.id > 0,               "Created user gets valid ID")
pulse create_user(name: str, email: str, age: int) -> User {
    // implementation
}
```

### 23.3 Debug vs Release Behavior

```
Debug mode (xphage build):
    @requires → runtime assertion at function entry
    @ensures  → runtime assertion at function exit
    Violation → clear error with line number and message

Release mode (xphage build --release):
    @requires → compiler attempts static proof
    Simple cases → proven, removed from binary (zero overhead)
    Complex cases → lightweight runtime check
    @ensures  → removed from binary entirely

Phase 10 (Formal Verification):
    Proven → never in binary (zero overhead, mathematically correct)
    Unproven → compile warning + lightweight check
```

---

## Chapter 24: @smart_ownership — Ownership Inference

> **5GL Opt-In Feature** — available from Phase 8.5. Explicit `own/ref/mut_ref` always remains valid and is never removed.

### 24.1 Explicit vs Inferred

```xphage
// Current (always valid — explicit):
pulse display(ref img: Image) {
    beam f"Size: {img.width}x{img.height}"
}

pulse resize(mut_ref img: Image, scale: float) {
    img.width  = img.width  * scale as int
    img.height = img.height * scale as int
}

pulse process(own data: Buffer) -> Buffer {
    transform(data)
    return data
}

// Phase 8.5 (opt-in — compiler infers):
@smart_ownership
pulse display(img: Image) {
    beam f"Size: {img.width}x{img.height}"
    // Compiler: img not modified, not returned → infers ref
}

@smart_ownership
pulse resize(img: Image, scale: float) {
    img.width = img.width * scale as int
    // Compiler: img modified, not returned → infers mut_ref
}

@smart_ownership
pulse process(data: Buffer) -> Buffer {
    transform(data)
    return data
    // Compiler: data returned → infers own (move)
}
```

### 24.2 Ambiguous Cases — Helpful Errors

```xphage
@smart_ownership
pulse send_and_keep(data: Buffer) {
    network_send(data)   // Does send() consume data or borrow?
    beam data.size       // data used after send — ambiguous
}
// [error] Cannot infer ownership for 'data'
//         network_send() signature is ambiguous from context
//         Please specify explicitly: own / ref / mut_ref
//         Hint: if network_send borrows: use 'ref data: Buffer'
//               if network_send consumes: use 'own data: Buffer'
```

### 24.3 Both Styles Always Work Together

```xphage
// Explicit and inferred functions freely call each other

pulse explicit_fn(ref img: Image) {   // explicit — always valid
    beam img.width
}

@smart_ownership
pulse inferred_fn(img: Image) {       // inferred — Phase 8.5
    beam img.width
}

pulse main() {
    atom img = load_image("photo.jpg")
    explicit_fn(img)   // works
    inferred_fn(img)   // works
    // Both are identical at runtime
}
```

---

## Chapter 25: @register — Declarative Hardware (Phase 8)

> **Advanced 3GL Feature** — for embedded and OS development. Makes `unsafe` hardware access safe and readable.

### 25.1 The Problem

```xphage
// Raw hardware access — verbose, error-prone, hard to read:
unsafe {
    atom uart = 0x10000000 as *mut int
    *uart = (*uart & !0xFF) | (baudrate & 0xFF)
    atom status = *(0x10000008 as *const int)
    if status & 0x01 != 0 { /* TX ready */ }
}
```

### 25.2 @register — Declarative Layout

```xphage
// hardware.xh — declare hardware once, cleanly
@register UART0 at 0x10000000 {
    TX_DATA:   write_only  bit[8]  at 0x00
    RX_DATA:   read_only   bit[8]  at 0x04
    BAUD_RATE: read_write  bit[16] at 0x08
    STATUS:    read_only   bit[32] at 0x0C {
        TX_READY: bit[0]
        RX_FULL:  bit[1]
        ERROR:    bit[2]
    }
    CONTROL:   read_write  bit[8]  at 0x10 {
        ENABLE:     bit[0]
        INT_ENABLE: bit[1]
        PARITY:     bit[2:3]
    }
}

@register GPIO at 0x20000000 {
    PIN_OUT:   read_write bit[32] at 0x00
    PIN_IN:    read_only  bit[32] at 0x04
    PIN_DIR:   read_write bit[32] at 0x08
    PIN_SET:   write_only bit[32] at 0x0C
    PIN_CLEAR: write_only bit[32] at 0x10
}
```

```xphage
// main.xp0 — clean, safe, readable
// Compiler generates all unsafe pointer arithmetic automatically

pulse init_uart(baud: int) {
    UART0.BAUD_RATE       = baud
    UART0.CONTROL.ENABLE  = true
    UART0.CONTROL.PARITY  = 0
}

pulse send_byte(byte: int) {
    while !UART0.STATUS.TX_READY {}
    UART0.TX_DATA = byte
}

pulse recv_byte() -> int {
    while !UART0.STATUS.RX_FULL {}
    return UART0.RX_DATA
}

pulse set_pin_output(pin: int) {
    GPIO.PIN_DIR = GPIO.PIN_DIR | (1 << pin)
    GPIO.PIN_SET = 1 << pin
}
```

**Compiler generates all unsafe code automatically:**
```
UART0.TX_DATA = byte    →    unsafe { *(0x10000000 as *mut int) = byte & 0xFF }
UART0.STATUS.TX_READY  →    unsafe { (*(0x1000000C as *const int) >> 0) & 0x01 != 0 }
```

---

# Part X — Building Real Projects

---

## Chapter 26: Complete Projects

### Project 1: Command-Line Tool

A file statistics tool:

```xphage
// file_stats.xp0
~link "io"
~link "string"
~link "math"

forge FileStats {
    path:  str = ""
    size:  int = 0
    lines: int = 0
    words: int = 0
    chars: int = 0
    ext:   str = ""
}

pulse analyze_file(path: str) -> FileStats {
    atom content    = io_read(path)
    atom all_lines  = str_split_lines(content)
    atom line_count = vec_size(str_split(all_lines, "\n"))
    atom word_list  = str_split(content, " ")
    atom clean_words = vec_filter(word_list,
        |w| str_length(str_trim(w)) > 0)

    return spawn FileStats {
        path:  path
        size:  io_size(path)
        lines: line_count
        words: vec_size(clean_words)
        chars: str_length(content)
        ext:   path_extension(path)
    }
}

pulse print_stats(s: FileStats) {
    beam f"File:  {path_basename(s.path)}"
    beam f"Size:  {s.size} bytes"
    beam f"Lines: {s.lines}"
    beam f"Words: {s.words}"
    beam f"Chars: {s.chars}"
    beam f"Type:  {s.ext}"
}

pulse main() {
    atom argc = os_argc()

    if argc < 2 {
        beam "Usage: file_stats <file> [<file2> ...]"
        beam "       file_stats --dir <directory>"
        os_exit(1)
    }

    atom first_arg = os_arg(1)

    if first_arg == "--dir" && argc >= 3 {
        atom dir   = os_arg(2)
        atom files = io_list_dir(dir)
        shadow total_lines: int = 0
        shadow total_words: int = 0

        for file in str_split(files, "\n") {
            if str_length(file) == 0 { continue }
            vortex {
                atom stats = analyze_file(file)
                print_stats(stats)
                total_lines = total_lines + stats.lines
                total_words = total_words + stats.words
                beam "---"
            }
        }
        beam f"TOTAL: {total_lines} lines, {total_words} words"
    } else {
        atom stats = analyze_file(first_arg)
        print_stats(stats)
    }
}
```

### Project 2: REST API Client

```xphage
// api_client.xp0
~link "net"
~link "string"
~link "io"

forge GitHubRepo {
    name:        str = ""
    description: str = ""
    stars:       int = 0
    language:    str = ""
    url:         str = ""
}

pulse fetch_repos(username: str) -> str {
    atom url  = f"https://api.github.com/users/{username}/repos"
    atom resp = http_get(url)

    if !http_is_ok(resp) {
        beam f"Error: {resp.status}"
        return ""
    }
    return resp.body
}

pulse parse_repos(json_array: str) -> str {
    atom names = json_array(json_array, "[*].name")
    atom stars = json_array(json_array, "[*].stargazers_count")
    return zip(names, stars)
}

pulse main() {
    atom username = if os_argc() > 1 {
        os_arg(1)
    } else {
        "AeonCoreX-Lab"
    }

    beam f"Fetching repos for: {username}"
    beam ""

    atom json = fetch_repos(username)
    if str_length(json) == 0 { return }

    atom repos = parse_repos(json)
    shadow repo_count: int = 0

    for entry in str_split(repos, ",") {
        atom parts = str_split(entry, ":")
        atom name  = vec_get(parts, 0)
        atom stars = vec_get(parts, 1)
        beam f"  ★ {stars}  {name}"
        repo_count = repo_count + 1
    }

    beam ""
    beam f"Total: {repo_count} repositories"
}
```

### Project 3: Full Mobile App (with Fusion UI)

```xphage
// models.xh
forge Task {
    id:       int  = 0
    title:    str  = ""
    done:     bool = false
    priority: int  = 1     // 1=low, 2=med, 3=high
}
```

```xphage
// layout.xui
~link "fusion-ui"
~link "models.xh"

flux tasks:     str = ""
flux new_title: str = ""
flux filter:    str = "all"

atom priority_color = |p: int| -> str {
    probe p {
        diverge 3 -> "#FF4444"
        diverge 2 -> "#FF9800"
        diverge _ -> "#4CAF50"
    }
}

fusion TaskApp {
    Scaffold {
        top_bar: OrbitH(weave().background("#6C63FF").padding(16)) {
            Vision("XPhage Tasks")
            Spacer(weight: 1)
            Trigger("Clear Done") { emit "clear_done" }
        }

        content: Orbit(weave().padding(16)) {
            Signal(weave().corner_radius(12).elevation(1).margin(8)) {
                Orbit(weave().padding(16)) {
                    Vision("Add New Task")
                    Spacer(8)
                    Input("Task title...", weave().fill_width()) {
                        absorb "on_change" { new_title = input_value }
                    }
                    Spacer(8)
                    OrbitH {
                        Trigger("Low",  weave().background("#4CAF50").corner_radius(6).padding(8,16)) { emit "add_task" { "1" } }
                        Spacer(8)
                        Trigger("Med",  weave().background("#FF9800").corner_radius(6).padding(8,16)) { emit "add_task" { "2" } }
                        Spacer(8)
                        Trigger("High", weave().background("#FF4444").corner_radius(6).padding(8,16)) { emit "add_task" { "3" } }
                    }
                }
            }

            Spacer(16)

            OrbitH(weave().fill_width()) {
                Trigger("All",    weave().weight(1)) { filter = "all" }
                Trigger("Active", weave().weight(1)) { filter = "active" }
                Trigger("Done",   weave().weight(1)) { filter = "done" }
            }

            Spacer(8)
            Vision(f"{vec_size(str_split(tasks,\"|\"))} tasks")
        }

        fab: Trigger("+", weave().corner_radius(28).elevation(6)) {
            emit "scroll_to_top"
        }
    }
}
```

```xphage
// main.xp0
~link "models.xh"
~link "layout.xui"
~link "collections"

shadow task_list = vec_new()
shadow next_id: int = 1

absorb "add_task" {
    if str_length(new_title) == 0 { return }
    atom id    = next_id
    next_id    = next_id + 1
    task_list  = vec_push(task_list, f"{id}:{new_title}:{priority}:false")
    tasks      = vec_join(task_list, "|")
    new_title  = ""
    emit "task_added"
}

absorb "toggle_task" {
    tasks = toggle_task_in_list(task_id, task_list)
}

absorb "clear_done" {
    task_list = vec_filter(task_list, |t| !str_contains(t, ":true"))
    tasks     = vec_join(task_list, "|")
}

pulse main() {
    beam f"XPhage Task App on {os_platform()}"
    xphage run TaskApp
}
```

---

# Appendix

---

## Appendix A: Keyword Quick Reference

| Keyword | Phase | Role |
|---------|-------|------|
| `pulse` | 1 | Function declaration |
| `atom` | 1 | Immutable variable |
| `shadow` | 1 | Mutable variable |
| `global` | 1 | Module variable |
| `const` | 3 | Compile-time constant |
| `return` | 1 | Return value |
| `beam` | 1 | Print output |
| `bypass` | 1 | System command (fire-and-forget) |
| `quantum` | 1 | Spawn concurrent task |
| `vortex` | 1 | Error handling block |
| `catch` | 1 | Error catch clause |
| `finally` | 1 | Always-run cleanup |
| `~link` | 1 | Import module or file |
| `if/elif/else` | 1 | Conditional |
| `while` | 1 | While loop |
| `for/in` | 1 | Iterator loop |
| `break/continue` | 1 | Loop control |
| `forge` | 2 | Struct/record type |
| `nexus` | 2 | Interface/trait |
| `impl` | 2 | Method implementation |
| `self` | 2 | Self reference in impl |
| `realm` | 2 | Namespace grouping |
| `use` | 2 | Import a qualified name into unqualified scope |
| `flux` | 2 | Reactive state variable |
| `probe/diverge` | 2 | Pattern matching |
| `emit` | 2 | Fire event |
| `absorb` | 2 | Handle event |
| `weave` | 2 | UI modifier chain |
| `strand` | 2 | Animation definition |
| `spawn` | 2 | Create struct instance |
| `own` | 3 | Unique ownership |
| `ref` | 3 | Immutable borrow |
| `mut_ref` | 3 | Mutable borrow |
| `async` | 3 | Async function |
| `await` | 3 | Await async result |
| `proc` | 3 | Run shell command + capture output |
| `env` | 3 | Environment variables |
| `unsafe` | 3 | Unsafe block |
| `extern` | 3 | External linkage (FFI) |
| `enum` | 6 | Type-safe enumeration |
| `select/from/where` | 6.5 | Native query syntax |
| `solve` | 10 | Constraint solver block |

### Annotations

| Annotation | Phase | Purpose |
|---|---|---|
| `@differentiable` | 7.5 | Opt-in auto gradient generation |
| `@gpu_kernel` | 7 | Opt-in GPU dispatch |
| `@smart_ownership` | 8.5 | Opt-in ownership inference |
| `@requires(cond, msg)` | 8 | Precondition contract |
| `@ensures(cond, msg)` | 8 | Postcondition contract |
| `@register NAME at ADDR {}` | 8 | Hardware register map |
| `@interrupt NAME {}` | 8 | Interrupt handler |

---

## Appendix B: Operator Quick Reference

```
Arithmetic:       + - * / %
Comparison:       == != < > <= >=
Logical:          && || !  (and or not)
Bitwise:          & | ^ ~ << >>
Assignment:       = += -= *= /=
Pipeline:         |>
Error prop:       ?
f-string:         f"text {expr}"
Member access:    object.field
Index:            array[i]
Lambda:           |params| expression
Cast:             value as Type
Range:            ..
```

---

## Appendix C: Standard Library Reference

```
~link "io"           File I/O, path, glob, process I/O
~link "math"         Arithmetic, trig, stats, Vec2/3, Mat4
~link "string"       String processing, regex, encoding
~link "collections"  Vec, Map, Set, Queue, Stack + Phase 6 combinators
~link "net"          HTTP, WebSocket, TCP, DNS, JSON
~link "os"           Platform, threads, time, signals
~link "crypt"        AES, RSA, SHA, Argon2, UUID
~link "ai"           Tensor, neural ops, LLM, NPU, @differentiable, @gpu_kernel
~link "solver"       Constraint solver for solve {} blocks (Phase 10)
~link "fusion-ui"    Full UI framework
```

---

## Appendix D: Compiler Flags

```bash
# Build commands
xphage build src/main.xp0                         # debug build
xphage build src/main.xp0 --release               # optimized
xphage build src/main.xp0 --release -O3           # max optimization
xphage build src/main.xp0 --release --lto         # link-time optimization
xphage build src/main.xp0 --release --pgo         # profile-guided optimization
xphage build src/main.xp0 -o bin/myapp            # custom output name

# Backend selection (see Appendix D.1 for what each backend is)
xphage build src/main.xp0 --backend=llvm          # LLVM native (default for --release)
xphage build src/main.xp0 --backend=transpiler    # C++17 transpiler, AST-driven
xphage build src/main.xp0 --backend=xil           # C++17 transpiler, IR-driven

# Cross-compilation
xphage build --target android-aarch64 src/main.xp0
xphage build --target ios-arm64       src/main.xp0
xphage build --target wasm32-unknown  src/main.xp0
xphage build --target riscv32-none-elf src/boot.xp0   # bare metal

# Run
xphage run src/main.xp0
xphage run src/main.xp0 -- --arg1 --arg2

# Package management
xpm add fusion-ui
xpm add solver          # required for solve {} blocks
xpm remove fusion-ui
xpm list
xpm update
xpm publish             # publish library to Spore registry
```

### Appendix D.1 — The Three Compiler Backends

XPhage source passes through the same lexer, parser, and semantic analyzer regardless of backend — the backend only decides how the validated program becomes an executable. There are three:

| Backend | Flag | Pipeline | When it's used |
|---|---|---|---|
| LLVM native | `--backend=llvm` | AST → LLVM IR → native machine code | Default for `--release` builds; full optimization levels, DWARF debug info, LTO, PGO (see Appendix D) |
| C++17 transpiler (AST) | `--backend=transpiler` | AST → generated C++17 → system C++ compiler | Default for debug builds; the original, most heavily-tested code generation path |
| C++17 transpiler (XIL) | `--backend=xil` | AST → XIL (XPhage Intermediate Representation) → generated C++17 → system C++ compiler | An alternative C++ codegen path that goes through an explicit intermediate representation instead of generating C++ directly from the AST |

**What XIL is, and why there are two C++ transpiler paths.** "XIL" (XPhage Intermediate Representation) is a lowered, SSA-register-style representation that sits between the AST and generated C++ — every expression becomes a sequence of typed instructions (`Alloca`, `Store`, `Call`, `GEP`, arithmetic/comparison ops, branches) operating on named registers, closer to what a traditional compiler's middle-end works with than to XPhage source syntax itself. `--backend=transpiler` skips this step and walks the AST directly to produce C++; `--backend=xil` lowers to XIL first (`xphage build --emit=ir` prints this representation directly, if you want to see it) and generates C++ from that instead.

The practical difference for day-to-day use is small — both `--backend=transpiler` and `--backend=xil` produce working programs and, for any given source file, should produce output that behaves identically. The AST-driven transpiler is the more heavily used and tested of the two C++ paths; XIL exists because an explicit intermediate representation is what the LLVM native backend was always going to need anyway (LLVM IR generation lowers from the same kind of intermediate form), so XIL doubles as a C++-backed way to exercise and validate that lowering step independently of LLVM itself. If you don't have a specific reason to pick one of the C++ paths over the other, `--backend=transpiler` (the default for debug builds) is the safer choice today; reach for `--backend=xil` if you're specifically working on or debugging the IR lowering step itself.

All three backends accept the same linking flags for external libraries — see §12.5.1 for `extern "C"` and linking against C/C++/Rust libraries, which works identically regardless of which backend you build with.

---

## Appendix E: Error Messages Reference

```
[error] .xh: declarations only
→ You put executable code in a .xh file.
→ Move execution to a .xp0 file.

[error] .xui: fusion blocks only
→ You put non-UI code in a .xui file.
→ Move logic to .xh or .xp0 files.

[error] Expected '}'
→ Missing closing brace. Check your blocks.

[error] Expected ')'
→ Missing closing parenthesis.

[error E5001] @differentiable requires ~link "ai"
→ Add ~link "ai" to your imports.

[error E5002] solve {} requires ~link "solver"
→ Add solver = "1.0" to xpm.toml [dependencies]
→ Add ~link "solver" to your .xp0 file.

[error E5003] @gpu_kernel requires ~link "ai"
→ Add ~link "ai" to your imports.

[vortex] <error message>
→ Runtime error caught by vortex block.
```

---

## Appendix F: Project Structure Templates

### Simple CLI Tool
```
my-tool/
├── src/
│   ├── main.xp0
│   └── helpers.xh
├── xpm.toml
└── README.md
```

### Library (Spore Package)
```
my-lib/
├── src/
│   ├── lib.xh           ← public API
│   └── internal.xh      ← internal types
├── tests/
│   └── test_main.xp0
├── xpm.toml
└── README.md
```

### Mobile App (with Fusion UI)
```
my-app/
├── src/
│   ├── main.xp0         ← entry point + event handlers
│   ├── models.xh        ← data types
│   ├── logic.xh         ← business logic signatures
│   ├── screens/
│   │   ├── home.xui
│   │   ├── profile.xui
│   │   └── settings.xui
│   └── components/
│       ├── button.xui
│       └── card.xui
├── assets/
│   ├── icons/
│   └── images/
├── xpm.toml
└── README.md
```

### System / OS Project
```
my-os/
├── src/
│   ├── boot.xp0         ← #![bare_metal] entry
│   ├── kernel.xp0       ← kernel main
│   ├── interrupts.xh    ← @interrupt handlers
│   ├── hardware.xh      ← @register declarations
│   ├── memory.xh        ← memory management
│   ├── drivers/
│   │   ├── uart.xh
│   │   ├── uart.xp0
│   │   ├── gpio.xh
│   │   └── gpio.xp0
│   └── fs/
│       ├── vfs.xh
│       └── ext4.xh
├── linker.ld
└── xpm.toml
```

### Embedded / Microcontroller
```
my-device/
├── src/
│   ├── main.xp0         ← #![bare_metal] #![no_main]
│   └── hardware.xh      ← @register blocks
├── xpm.toml
└── README.md
```

---

## Appendix G: Generation Feature Summary

```
┌──────────────────────────────────────────────────────────────┐
│              XPhage 3GL + 4GL + 5GL Features                 │
├──────────────────────────────────────────────────────────────┤
│  3GL — Systems (Always Available)                            │
│  atom/shadow/const/global/flux — variables                   │
│  pulse/forge/nexus/impl        — functions and types         │
│  realm/use                     — namespaces                  │
│  own/ref/mut_ref               — ownership model             │
│  unsafe {}                     — raw memory access           │
│  extern "C"                    — C/C++/Rust FFI              │
│  proc/bypass/env               — system integration          │
│  async/await/quantum           — concurrency                 │
│  @register (Ph 8)              — declarative HW registers    │
│  @interrupt (Ph 8)             — interrupt handlers          │
│  @requires/@ensures (Ph 8)     — design by contract          │
│  #![bare_metal] (Ph 8)         — no OS, no stdlib            │
├──────────────────────────────────────────────────────────────┤
│  4GL — Declarative (Always Available)                        │
│  flux/emit/absorb              — reactive state + event bus  │
│  Fusion UI (weave/strand)      — cross-platform UI           │
│  enum (Ph 6)                   — type-safe enumerations      │
│  (T, U) tuples (Ph 6)          — anonymous grouped values     │
│  filter/map/reduce (Ph 6)      — typed pipeline combinators  │
│  select from where (Ph 6.5)    — native query syntax         │
│  group_by/zip/sort_by (Ph 6)   — collection combinators      │
├──────────────────────────────────────────────────────────────┤
│  5GL — Intelligence (Opt-In Only)                            │
│  @differentiable (Ph 7.5)  → gradient auto-generation        │
│  @gpu_kernel (Ph 7)        → GPU parallel dispatch           │
│  accelerate (Ph 7)         → SIMD auto-vectorization         │
│  accelerate npu (Ph 7)     → NPU dispatch + CPU fallback     │
│  @smart_ownership (Ph 8.5) → ownership inference             │
│  solve {} (Ph 10)          → constraint-based solving        │
│                                                              │
│  Activate: @annotation or solve {} or ~link                  │
│  Cost if unused: exactly zero bytes, zero nanoseconds        │
│  Existing code: never affected, never breaks                 │
└──────────────────────────────────────────────────────────────┘
```

---

*The XPhage Programming Language is developed by AeonCoreX Lab.*
*© 2026 AeonCoreX Lab. All Rights Reserved.*

*"From silicon to the stars."*

