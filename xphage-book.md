# The X-Phage Programming Language

**AeonCoreX Lab | v3.5.0**

*By the X-Phage Team*

---

> *"Write once. Run everywhere. From silicon to the stars."*

---

## Foreword

X-Phage was born from a simple frustration: every language makes you choose. You can have speed or safety. Expressiveness or control. High-level abstractions or bare-metal access. Native performance or cross-platform UI.

X-Phage refuses to make that choice.

This book is for anyone who wants to write software that is fast, safe, expressive, and runs everywhere — from a space rover's onboard computer to a mobile app, from an AI inference engine to an OS kernel. No compromises.

Whether you are a C++ veteran tired of undefined behavior, a Python developer who needs more speed, a Rust programmer who wants simpler syntax, or a Kotlin developer who wants true native cross-platform UI — X-Phage is for you.

---

## How to Read This Book

This book progresses from beginner to expert:

- **Chapters 1-3**: Setup, first programs, basic syntax
- **Chapters 4-6**: Core language features: types, functions, control flow
- **Chapters 7-9**: The type system: forge, nexus, impl
- **Chapters 10-11**: Reactive programming: flux, emit, absorb
- **Chapters 12-13**: Systems programming: ownership, memory
- **Chapters 14-15**: Standard library deep dive
- **Chapters 16-18**: Fusion UI framework
- **Chapters 19-20**: Advanced topics: async, AI, embedded
- **Chapter 21**: Building real projects

---

# Part I — Getting Started

---

## Chapter 1: Installation and Hello World

### 1.1 Installing X-Phage

**Linux / macOS:**
```bash
curl -sL https://raw.githubusercontent.com/AeonCoreX-Lab/X-Phage/main/scripts/install.sh | bash
```

**Windows:**
```powershell
irm https://raw.githubusercontent.com/AeonCoreX-Lab/X-Phage/main/scripts/install.ps1 | iex
```

This installs:
- `xphage` — the compiler
- `xpm` — the package manager
- Standard library

Verify:
```bash
xphage --version
# X-Phage 3.5.0 (Titan Transpiler)

xpm --version
# XPM 1.0.0
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

X-Phage uses three file types. This is not optional — the compiler enforces it:

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
    beam "Hello from X-Phage!"
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
    beam "=== X-Phage Guessing Game ==="
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
=== X-Phage Guessing Game ===
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

X-Phage has two kinds of variables:

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
const APP:     str   = "X-Phage"
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
// Integer operations
atom a: int = 1_000_000    // underscores for readability
atom b: int = 0xFF          // hex literal
atom c: int = 0b1010        // binary literal

// Float operations
atom x: float = 1.0e9       // scientific notation
atom y: float = -0.001

// Boolean
atom flag: bool = true
atom other      = !flag     // false

// String
atom name: str = "X-Phage"
atom multi      = "Line 1\nLine 2\nLine 3"
atom tab        = "Column1\tColumn2"
```

### 3.3 String Interpolation (f-strings)

```xphage
atom name  = "Nahid"
atom level = 42
atom score = 9850.5

// Basic interpolation
beam f"Hello {name}"
beam f"Level: {level}, Score: {score}"

// Expressions inside {}
beam f"Double: {level * 2}"
beam f"Is high: {score > 9000}"

// Method calls inside {}
atom upper_name = str_upper(name)
beam f"Welcome {upper_name}!"
```

### 3.4 Type Conversion

```xphage
// String ↔ Number
atom n = str_to_int("42")        // str → int
atom f = str_to_float("3.14")    // str → float
atom s = int_to_str(1000)        // int → str
atom fs = float_to_str(3.14)     // float → str
atom fs2 = float_to_str_prec(3.14159, 2)  // "3.14"

// Explicit cast
atom x: int   = 42
atom y: float = x as float      // int → float (Phase 3)
atom z: int   = 3.9 as int      // float → int, truncates to 3
```

### 3.5 Shadowing vs Mutability

In X-Phage, you can redeclare a variable in the same scope:

```xphage
atom x = 5
beam x     // 5

atom x = x * 2     // new atom x, shadows the old one
beam x     // 10

atom x = f"Value is {x}"   // even change type!
beam x     // "Value is 10"
```

This is different from `shadow` (mutable). Shadowing creates a new binding. The old one ceases to exist in this scope.

---

## Chapter 4: Functions

### 4.1 Declaring Functions

```xphage
// Basic function
pulse greet() {
    beam "Hello!"
}

// Function with parameters
pulse greet_user(name: str) {
    beam f"Hello, {name}!"
}

// Function with return type
pulse add(a: int, b: int) -> int {
    return a + b
}

// Function with multiple parameters
pulse create_user(name: str, age: int, email: str) -> str {
    return f"{name}:{age}:{email}"
}
```

### 4.2 Calling Functions

```xphage
greet()                              // Hello!
greet_user("Nahid")                 // Hello, Nahid!
atom result = add(10, 20)           // 30
atom user   = create_user("Nahid", 25, "nahid@example.com")
```

### 4.3 Return Values

```xphage
// Explicit return
pulse max_val(a: int, b: int) -> int {
    if a > b {
        return a
    }
    return b
}

// Early return
pulse find_first_negative(nums: str) -> int {
    atom parts = str_split(nums, ",")
    for part in parts {
        atom n = str_to_int(part)
        if n < 0 {
            return n
        }
    }
    return 0   // none found
}

// No return (void — default)
pulse log_info(message: str) {
    atom timestamp = os_datetime()
    beam f"[{timestamp}] INFO: {message}"
}
```

### 4.4 Lambda Expressions

```xphage
// Single-expression lambda: |params| expression
atom double   = |x: int| x * 2
atom add_ten  = |x: int| x + 10
atom square   = |x: float| x * x
atom is_even  = |n: int| n % 2 == 0
atom greet_fn = |name: str| f"Hello {name}"

// Using lambdas
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

The `|>` operator passes the left value as the argument to the right function:

```xphage
// Without pipeline (nested, hard to read)
atom result = str_upper(str_trim(str_replace(input, ",", "")))

// With pipeline (left-to-right, easy to read)
atom result = input
    |> str_replace(",", "")
    |> str_trim
    |> str_upper

// Multiple operations
atom processed = raw_data
    |> validate
    |> normalize
    |> compute_average
    |> format_output

// Pipeline with arguments using lambdas
atom result = numbers
    |> |v| vec_filter(v, |x| str_to_int(x) > 0)
    |> |v| vec_sort(v)
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
// Functions can be stored in variables
atom my_func = |x: int| x * x

// Pass functions as arguments
pulse apply(fn: auto, value: int) -> int {
    return fn(value)
}

atom result = apply(|x: int| x * 3, 7)   // 21

// Return functions from functions
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
// Basic if
if temperature > 30 {
    beam "Hot day!"
}

// if-else
if age >= 18 {
    beam "Adult"
} else {
    beam "Minor"
}

// if-elif-else chain
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

// if as expression (result used)
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
// Basic while
shadow i: int = 0
while i < 10 {
    beam i
    i = i + 1
}

// With break
shadow searching = true
shadow pos: int = 0
while searching {
    if data_at(pos) == target {
        searching = false
    }
    pos = pos + 1
    if pos > max_pos { break }
}

// With continue
shadow n: int = 0
while n < 20 {
    n = n + 1
    if n % 2 == 0 { continue }  // skip even numbers
    beam n    // prints only odd numbers
}
```

### 5.3 for / in Loops

```xphage
// Iterate over a range
for i in range(0, 10) {
    beam i    // 0, 1, 2, ..., 9
}

// Range with step
for i in range_step(0, 100, 10) {
    beam i    // 0, 10, 20, ..., 90
}

// Iterate over collection (comma-separated)
atom fruits = "apple,banana,cherry"
for fruit in str_split(fruits, ",") {
    beam f"I like {fruit}"
}

// Enumerate (get index + value)
atom languages = "X-Phage,Rust,C++,Python"
for entry in enumerate(str_split(languages, ",")) {
    // entry format: "0:X-Phage", "1:Rust", etc.
    atom parts = str_split(entry, ":")
    atom idx  = vec_get(parts, 0)
    atom lang = vec_get(parts, 1)
    beam f"{idx}. {lang}"
}
```

### 5.4 probe / diverge — Pattern Matching

`probe` is X-Phage's pattern matching — more powerful than `switch`:

```xphage
// Basic probe
probe command {
    diverge "quit"    -> os_exit(0)
    diverge "help"    -> show_help()
    diverge "version" -> beam "X-Phage v3.5.0"
    diverge _         -> beam f"Unknown command: {command}"
}

// Probe on numbers
probe error_code {
    diverge 0   -> beam "Success"
    diverge 1   -> beam "Permission denied"
    diverge 2   -> beam "File not found"
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
    diverge _ -> {
        log_unknown_action(action)
    }
}

// Probe on boolean
probe authenticated {
    diverge true  -> show_dashboard()
    diverge false -> show_login()
}
```

### 5.5 vortex — Error Handling

```xphage
// Basic error handling
vortex {
    atom data = parse_dangerous_file("config.xh")
    process(data)
}

// Error handling with recovery
vortex {
    atom connection = db_connect("localhost:5432")
    atom users = db_query(connection, "SELECT * FROM users")
    beam f"Found {vec_size(users)} users"
}

// Nested vortex
vortex {
    atom file = io_read("data.json")
    vortex {
        atom parsed = json_parse(file)
        use_data(parsed)
    }
    // outer vortex catches if json_parse fails
}
```

### 5.6 The ? Error Propagation Operator

```xphage
// Without ? — verbose
pulse load_and_process(path: str) -> str {
    vortex {
        atom raw  = io_read(path)
        atom data = json_parse(raw)
        return process(data)
    }
    return ""
}

// With ? — concise (Phase 3)
pulse load_and_process(path: str) -> str {
    atom raw  = io_read(path)?
    atom data = json_parse(raw)?
    return process(data)
}
// ? means: if error, return error immediately (propagate up)
```

---

## Chapter 6: Operators and Expressions

### 6.1 Arithmetic

```xphage
atom a: int = 10
atom b: int = 3

beam a + b     // 13  — addition
beam a - b     // 7   — subtraction
beam a * b     // 30  — multiplication
beam a / b     // 3   — integer division (truncates)
beam a % b     // 1   — remainder/modulo

// Float arithmetic
atom x: float = 10.0
atom y: float = 3.0
beam x / y     // 3.333...  — float division
```

### 6.2 Comparison

```xphage
beam 5 == 5     // true
beam 5 != 3     // true
beam 5 >  3     // true
beam 5 <  3     // false
beam 5 >= 5     // true
beam 5 <= 4     // false

// String comparison
beam "abc" == "abc"    // true
beam "abc" != "xyz"    // true
```

### 6.3 Logical

```xphage
beam true && false    // false  (and)
beam true || false    // true   (or)
beam !true            // false  (not)

// Word forms also work
beam true and false   // false
beam true or false    // true
beam not true         // false

// Short-circuit evaluation
atom safe_divide = b != 0 && a / b > 0  // b != 0 checked first
```

### 6.4 Bitwise

```xphage
atom a: int = 0b1010   // 10
atom b: int = 0b1100   // 12

beam a & b    // 0b1000 = 8  (AND)
beam a | b    // 0b1110 = 14 (OR)
beam a ^ b    // 0b0110 = 6  (XOR)
beam ~a       // bitwise NOT
beam a << 1   // 0b10100 = 20 (left shift)
beam a >> 1   // 0b0101 = 5  (right shift)
```

### 6.5 Compound Assignment

```xphage
shadow x: int = 10
x += 5    // x = x + 5  = 15
x -= 3    // x = x - 3  = 12
x *= 2    // x = x * 2  = 24
x /= 4    // x = x / 4  = 6
```

### 6.6 Operator Precedence

From highest to lowest:

```
1. Function calls, method calls, indexing
2. Unary: - !
3. * / %
4. + -
5. << >>
6. < > <= >=
7. == !=
8. &
9. ^
10. |
11. &&
12. ||
13. |> (pipeline)
14. = += -= *= /= (assignment)
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
    name:       str = ""
    email:      str = ""
    age:        int = 0
    score:      float = 0.0
    active:     bool = true
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

// Create with spawn
atom p = spawn Point { x: 3.0, y: 4.0 }

// Partial construction (uses defaults for missing fields)
atom red = spawn Color { r: 255 }    // g=0, b=0, a=255

// Full construction
atom user = spawn User {
    name:   "Nahid"
    email:  "nahid@example.com"
    age:    25
    score:  98.5
    active: true
}

// Default construction (all defaults)
atom origin = spawn Point {}
```

### 7.3 Accessing Fields

```xphage
atom p = spawn Point { x: 3.0, y: 4.0 }

beam p.x        // 3.0
beam p.y        // 4.0
beam f"Point: ({p.x}, {p.y})"

// Mutable field access
shadow u = spawn User { name: "Nahid", age: 25 }
u.age   = 26
u.score = 99.0
beam u.age    // 26
```

### 7.4 Nested forge

```xphage
forge Address {
    street: str = ""
    city:   str = ""
    country: str = ""
}

forge Person {
    name:    str = ""
    age:     int = 0
    address: Address = spawn Address {}
}

// Usage
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

// Pass by ref (immutable reference — Phase 3)
pulse display_user(ref u: User) {
    beam f"User: {u.name}"
    beam f"Email: {u.email}"
}

// Pass by mut_ref (mutable reference — Phase 3)
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
    draw() -> void
    bounds() -> Rectangle
}

nexus Serializable {
    to_json()           -> str
    from_json(s: str)   -> bool
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
forge Dog {
    name: str = ""
    breed: str = ""
}

forge Cat {
    name: str = ""
    indoor: bool = true
}
```

```xphage
// animals_impl.xh
impl Animal for Dog {
    speak() -> str {
        return "Woof!"
    }
    move() -> void {
        beam f"{self.name} runs"
    }
    name() -> str {
        return self.name
    }
}

impl Animal for Cat {
    speak() -> str {
        return "Meow!"
    }
    move() -> void {
        beam f"{self.name} slinks"
    }
    name() -> str {
        return self.name
    }
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
        return f"{\"title\":\"{self.title}\",\"content\":\"{self.content}\",\"author\":\"{self.author}\"}"
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

    // Default implementations
    log_info(msg: str) -> void {
        self.log(f"[INFO]  {msg}")
    }
    log_warn(msg: str) -> void {
        self.log(f"[WARN]  {msg}")
    }
    log_error(msg: str) -> void {
        self.log(f"[ERROR] {msg}")
    }
}
```

Any type implementing `Logger` only needs to define `log()`. The other methods come for free.

---

## Chapter 9: Enumerations and Variants

While X-Phage doesn't have a dedicated `enum` keyword yet (Phase 6), you can model enumerations using constants and `probe`:

```xphage
// status.xh
const STATUS_OK:      str = "ok"
const STATUS_WARN:    str = "warn"
const STATUS_ERROR:   str = "error"
const STATUS_LOADING: str = "loading"

// directions
const DIR_NORTH: int = 0
const DIR_EAST:  int = 1
const DIR_SOUTH: int = 2
const DIR_WEST:  int = 3
```

```xphage
// Usage
shadow status = STATUS_LOADING
load_data()
status = STATUS_OK

probe status {
    diverge STATUS_OK      -> beam "All good"
    diverge STATUS_WARN    -> beam "Warning"
    diverge STATUS_ERROR   -> beam "Failed"
    diverge STATUS_LOADING -> beam "Please wait..."
    diverge _              -> beam "Unknown"
}
```

**Option pattern (None / Some):**
```xphage
~link "collections"

// Result that may or may not exist
atom found = find_user(42)

if option_is_some(found) {
    atom user = option_unwrap(found)
    beam f"Found: {user}"
} else {
    beam "User not found"
}

// With default
atom name = option_unwrap_or(find_name(id), "Anonymous")
```

**Result pattern (Ok / Err):**
```xphage
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

---

# Part IV — Reactive Architecture

---

## Chapter 10: flux — Reactive State

### 10.1 What is Reactive State?

In traditional programming:
```xphage
shadow score: int = 0
score = 100
// UI doesn't know score changed
// You must manually update UI
```

With `flux`:
```xphage
flux score: int = 0
score = 100
// Fusion UI automatically re-renders all components that display score
// Zero manual synchronization
```

`flux` is the most important concept for building interactive applications in X-Phage.

### 10.2 Declaring flux

```xphage
// Single values
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
// Direct use — reads current value
beam counter
beam f"Hello {username}"
beam f"Progress: {progress}%"

// In conditions
if is_loading {
    beam "Please wait..."
}

if user_id > 0 {
    beam "Logged in"
} else {
    beam "Guest mode"
}
```

### 10.4 Writing flux

```xphage
// Direct assignment — triggers observers
counter    = counter + 1
username   = "Nahid"
is_loading = true
progress   = 75.5

// Compound assignment
counter += 10
counter -= 5
progress *= 2.0
```

### 10.5 flux in UI (Fusion UI integration)

```xphage
// layout.xui
~link "fusion-ui"

flux count:    int = 0
flux message:  str = "Ready"

fusion CounterScreen {
    Orbit(weave().padding(24)) {
        // These automatically re-render when flux changes
        Vision(f"Count: {count}")
        Vision(f"Status: {message}")

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
// Trigger logic when state changes (Phase 3)
// Advanced: observe pattern
flux temperature: float = 20.0

// When temperature changes:
// absorb pattern handles this naturally
absorb "temperature_update" {
    if temperature > 35.0 {
        send_alert("High temperature!")
    } elif temperature < 0.0 {
        send_alert("Freezing!")
    }
}

// Trigger update
temperature = read_sensor()
emit "temperature_update"
```

---

## Chapter 11: emit / absorb — The Event Bus

### 11.1 The Event Bus Pattern

Instead of directly calling functions between components, X-Phage uses an event bus:

**Without event bus:**
```xphage
// Tight coupling — every component knows about every other
pulse button_clicked() {
    update_counter()
    refresh_ui()
    save_to_database()
    send_analytics()
}
```

**With emit/absorb:**
```xphage
// Loose coupling — components only know about events
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
emit "user_login"    { username }
emit "purchase"      { product_id, price, quantity }
emit "error"         { code, message }
emit "progress"      { current, total }
emit "navigation"    { destination }
```

### 11.3 Absorbing Events

```xphage
// Handle simple event
absorb "app_started" {
    beam "Application started"
    load_config()
    init_database()
}

// Handle event with data access
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
    if code >= 500 {
        alert_admin(message)
    }
}
```

### 11.4 Event-Driven Architecture

```xphage
// A complete feature implemented with events

// Feature: User registration
absorb "register_start" {
    is_loading = true
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
    is_loading    = false
    status_message = "Account created!"
    emit "navigate" { "dashboard" }
}

absorb "register_error" {
    is_loading     = false
    status_message = f"Error: {error_message}"
}

// Trigger the flow
Trigger("Create Account") { emit "register_start"; emit "register_validate" }
```

### 11.5 Best Practices

**Name events clearly:**
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

**One responsibility per absorb:**
```xphage
// Good — each absorb does one thing
absorb "order_placed" { update_inventory(product_id) }
absorb "order_placed" { send_confirmation_email(user_email) }
absorb "order_placed" { notify_warehouse(order_id) }

// Bad — one absorb does everything
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

X-Phage has a lightweight ownership system inspired by Rust but simpler:

| Keyword | Meaning | Use when |
|---------|---------|----------|
| `own` | Unique ownership | Large data, file handles, connections |
| `ref` | Immutable borrow | Read-only access, passing to functions |
| `mut_ref` | Mutable borrow | Need to modify without transferring ownership |
| Normal | Copy semantics | Small data (int, float, bool, short strings) |

### 12.2 own — Unique Ownership

```xphage
// own = you own this, you are responsible for it
own shadow buffer = allocate(1024 * 1024)  // 1MB buffer
own atom file     = open_file("data.bin")
own shadow conn   = db_connect("localhost")

// When these go out of scope, resources are freed automatically
// No GC needed, no manual free()
```

```xphage
pulse process_file(path: str) {
    own atom f = open_file(path)
    // use f...
    // f is automatically closed when this function returns
}
// No file handle leaks
```

### 12.3 ref — Immutable Borrow

```xphage
forge Image {
    width:  int = 0
    height: int = 0
    pixels: str = ""   // pixel data
}

// Takes an immutable reference — cannot modify the image
pulse display_info(ref img: Image) {
    beam f"Image: {img.width}x{img.height}"
    beam f"Pixels: {str_length(img.pixels)}"
}

// Takes ownership — can do anything
pulse save_image(img: Image, path: str) {
    io_write(path, img.pixels)
}

atom photo = spawn Image { width: 1920, height: 1080 }
display_info(photo)   // photo not moved — ref borrow
// photo still valid here
save_image(photo, "photo.bin")  // photo moved
// photo no longer valid here (owned by save_image)
```

### 12.4 mut_ref — Mutable Borrow

```xphage
// Modify without taking ownership
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

For OS development, hardware access, and FFI:

```xphage
// Hardware register access (embedded/OS)
unsafe {
    atom uart_addr = 0x10000000 as *mut int
    *uart_addr = 72    // write 'H' to UART
}

// Direct memory (when you know exactly what you're doing)
unsafe {
    atom raw_ptr = allocate_raw(1024)
    write_raw(raw_ptr, data, size)
    // Manual cleanup required in unsafe blocks
    free_raw(raw_ptr)
}

// FFI — calling C functions
unsafe {
    extern "C" pulse malloc(size: int) -> *mut void
    extern "C" pulse free(ptr: *mut void)

    atom mem = malloc(256)
    // use mem...
    free(mem)
}
```

### 12.6 Memory Safety in Practice

X-Phage prevents these common bugs:

```xphage
// Null pointer — impossible with Option<T>
atom user = find_user(42)   // returns Option, not nullable
if option_is_some(user) {
    atom u = option_unwrap(user)
    beam u.name    // safe — we checked
}
// Never: beam user.name when user might be null

// Buffer overflow — bounds-checked
atom arr = vec_new()
vec_push(arr, "a")
vec_push(arr, "b")
atom item = vec_get(arr, 5)   // returns "" not crash
// Never: direct index without bounds check

// Use-after-free — ownership prevents this
own atom data = load_data()
process(data)   // data moved into process
// beam data   // compile error: data no longer valid
```

---

## Chapter 13: Process, Environment, and the Shell

### 13.1 proc — Running Processes

`proc` runs a shell command and captures its output:

```xphage
// Basic usage
atom output = proc "ls -la"
atom git_branch = proc "git rev-parse --abbrev-ref HEAD"
atom disk_usage = proc "df -h /"
atom cpu_info   = proc "cat /proc/cpuinfo | grep 'model name' | head -1"

beam f"Current branch: {git_branch}"
beam f"Disk usage:\n{disk_usage}"

// Multi-line output
atom log_lines = proc "git log --oneline -10"
for line in str_split_lines(log_lines) {
    beam f"  {line}"
}
```

```xphage
// Build system example
pulse build_project() {
    beam "Building X-Phage project..."

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

When you don't need the output:

```xphage
bypass "mkdir -p build/release"
bypass "chmod +x scripts/install.sh"
bypass "rm -rf /tmp/xphage_cache"
bypass "git add -A"
bypass "git commit -m 'Auto-commit'"
```

### 13.3 env — Environment Variables

```xphage
// Read environment variables
atom home     = env.HOME
atom path     = env.PATH
atom user     = env.USER
atom api_key  = env.OPENAI_API_KEY
atom port     = env.PORT
atom debug    = env.DEBUG

// Use in logic
if debug == "true" {
    beam "Debug mode enabled"
    verbose_logging = true
}

atom server_port = if str_length(port) > 0 {
    str_to_int(port)
} else {
    8080    // default port
}

beam f"Starting server on port {server_port}"
```

```xphage
// Practical: Loading API credentials
pulse init_api_client() -> str {
    atom key = env.API_KEY
    if str_length(key) == 0 {
        beam "Error: API_KEY environment variable not set"
        os_exit(1)
    }
    return create_client(key)
}
```

### 13.4 glob — File Pattern Matching

```xphage
~link "io"

// Find all X-Phage source files
atom xp0_files = glob "src/*.xp0"
atom xh_files  = glob "src/*.xh"
atom all_src   = glob "src/**/*.xp0"

// Process all config files
for config_file in str_split(glob "config/*.toml", ",") {
    atom content = io_read(config_file)
    parse_config(content)
    beam f"Loaded: {config_file}"
}

// Count source files
atom files = glob "**/*.xp0"
atom count = vec_size(str_split(files, ","))
beam f"Project has {count} .xp0 files"
```

### 13.5 Command-Line Arguments

```xphage
// In main.xp0
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
        diverge _        -> {
            beam f"Unknown command: {command}"
            beam "Run 'myapp help' for usage"
            os_exit(1)
        }
    }
}

pulse cmd_build() {
    atom target = if os_argc() > 2 { os_arg(2) } else { "debug" }
    beam f"Building for target: {target}"
    // build logic...
}
```

---

# Part VI — Standard Library

---

## Chapter 14: Collections

### 14.1 Vec — Dynamic Arrays

```xphage
~link "collections"

// Create
shadow v = vec_new()
v = vec_push(v, "apple")
v = vec_push(v, "banana")
v = vec_push(v, "cherry")

// Access
beam vec_size(v)        // 3
beam vec_get(v, 0)      // "apple"
beam vec_get(v, -1)     // "cherry" (negative indexing!)
beam vec_get(v, 1)      // "banana"

// Modify
v = vec_set(v, 1, "blueberry")  // replace index 1
v = vec_push_front(v, "avocado")  // prepend
v = vec_pop(v)           // remove last
v = vec_pop_front(v)     // remove first

// Search
atom found = vec_contains(v, "apple")   // true/false
atom idx   = vec_index_of(v, "apple")   // index or -1

// Transform
v = vec_sort(v)                         // alphabetical
v = vec_reverse(v)
atom unique_v = vec_unique(v)           // remove duplicates
atom sliced   = vec_slice(v, 1, 3)      // v[1..3]

// Numeric operations
shadow nums = vec_new()
nums = vec_push(nums, "10")
nums = vec_push(nums, "20")
nums = vec_push(nums, "30")
beam vec_sum(nums)        // 60.0
beam vec_min(nums)        // 10.0
beam vec_max(nums)        // 30.0

// Join
beam vec_join(v, ", ")    // "apple, banana, cherry"
```

### 14.2 Map — Key-Value Storage

```xphage
// Create
shadow m = map_new()
m = map_set(m, "name",    "Nahid")
m = map_set(m, "country", "Bangladesh")
m = map_set(m, "lang",    "X-Phage")
m = map_set(m, "level",   "42")

// Access
beam map_get(m, "name")             // "Nahid"
beam map_get_or(m, "city", "N/A")   // "N/A" (not set)
beam map_has(m, "lang")             // true
beam map_size(m)                    // 4

// Keys and values
atom keys   = map_keys(m)           // comma-separated
atom values = map_values(m)         // comma-separated
for key in str_split(keys, ",") {
    beam f"{key}: {map_get(m, key)}"
}

// Remove
m = map_remove(m, "level")
beam map_has(m, "level")    // false

// Merge two maps (second overwrites first on conflict)
atom combined = map_merge(m, other_map)
```

### 14.3 Set — Unique Collections

```xphage
// Create
shadow s = set_new()
s = set_add(s, "red")
s = set_add(s, "green")
s = set_add(s, "blue")
s = set_add(s, "red")    // duplicate — ignored

beam set_size(s)          // 3 (not 4)
beam set_contains(s, "red")    // true
beam set_contains(s, "yellow") // false

// Set operations
atom a = "apple,banana,cherry"  // as comma-sep for set construction
atom b = "banana,cherry,date"

// (In practice, build from vecs)
atom union = set_union(s, other_set)
atom inter = set_intersect(s, other_set)
atom diff  = set_difference(s, other_set)

s = set_remove(s, "blue")
atom v = set_to_vec(s)    // convert to vec
```

### 14.4 Queue and Stack

```xphage
// Queue — FIFO (First In, First Out)
shadow q = queue_new()
q = queue_push(q, "first")
q = queue_push(q, "second")
q = queue_push(q, "third")

beam queue_peek(q)         // "first" (don't remove)
beam queue_size(q)         // 3

q = queue_pop(q)           // removes "first"
beam queue_peek(q)         // "second"

// Stack — LIFO (Last In, First Out)
shadow s = stack_new()
s = stack_push(s, "bottom")
s = stack_push(s, "middle")
s = stack_push(s, "top")

beam stack_peek(s)         // "top" (don't remove)
s = stack_pop(s)           // removes "top"
beam stack_peek(s)         // "middle"
```

### 14.5 Range and Functional Operations

```xphage
// Range generation
atom r = range(0, 10)       // "0,1,2,3,4,5,6,7,8,9"
atom r = range_step(0, 20, 3) // "0,3,6,9,12,15,18"
atom r = range(10, 0)       // empty (start > end)

// Using range
for i in str_split(range(0, 5), ",") {
    beam f"Item {i}"
}

// Zip two lists
atom names = "Alice,Bob,Charlie"
atom scores = "95,87,92"
atom zipped = zip(names, scores)
// "Alice:95,Bob:87,Charlie:92"

for entry in str_split(zipped, ",") {
    atom parts = str_split(entry, ":")
    beam f"{vec_get(parts,0)} scored {vec_get(parts,1)}"
}

// Enumerate (add indices)
atom fruits = "apple,banana,cherry"
for entry in str_split(enumerate(fruits), ",") {
    atom parts = str_split(entry, ":")
    beam f"{vec_get(parts,0)}. {vec_get(parts,1)}"
}
// 0. apple
// 1. banana
// 2. cherry
```

---

## Chapter 15: String Processing

```xphage
~link "string"

// The fundamentals
atom s = "  Hello, X-Phage World!  "

beam str_length(s)                // 26
beam str_trim(s)                  // "Hello, X-Phage World!"
beam str_upper(s)                 // "  HELLO, X-PHAGE WORLD!  "
beam str_lower(s)                 // "  hello, x-phage world!  "
beam str_reverse(str_trim(s))     // "!dlroW egahP-X ,olleH"

// Search
atom t = str_trim(s)
beam str_contains(t, "X-Phage")  // true
beam str_find(t, "X-Phage")      // 7 (index)
beam str_starts_with(t, "Hello") // true
beam str_ends_with(t, "!")       // true
beam str_count(t, "l")           // 3

// Slice (like Python s[2:5])
beam str_slice(t, 0, 5)          // "Hello"
beam str_slice_from(t, 7)        // "X-Phage World!"
beam str_slice_to(t, 5)          // "Hello"
beam str_char_at(t, -1)          // "!" (last character)

// Replace
beam str_replace(t, "World", "Universe")  // "Hello, X-Phage Universe!"
beam str_replace_first(t, "l", "L")       // "HeLlo, X-Phage World!"

// Split and join
atom parts = str_split(t, ", ")   // "Hello\nX-Phage World!"
atom joined = str_join(parts, " | ") // "Hello | X-Phage World!"
atom lines  = str_split_lines("Line1\nLine2\nLine3")

// Padding
beam str_pad_left("42", 6, '0')   // "000042"
beam str_pad_right("hi", 10)      // "hi        "
beam str_center("TITLE", 20, '=') // "=======TITLE========"

// Type conversion
beam str_to_int("42")              // 42
beam str_to_float("3.14")          // 3.14
beam str_to_bool("true")           // true
beam int_to_str(1000000)           // "1000000"
beam float_to_str_prec(3.14159, 2) // "3.14"
beam bool_to_str(true)             // "true"

// Character checks
beam str_is_alpha("hello")         // true
beam str_is_digit("12345")         // true
beam str_is_alnum("abc123")        // true
beam str_is_upper("HELLO")         // true

// Regex
beam str_matches("abc123", "[a-z]+[0-9]+")   // true
beam str_regex_replace("Hello 2026", "\\d+", "YEAR") // "Hello YEAR"
atom emails = str_regex_find_all(text, "[\\w.]+@[\\w.]+\\.\\w+")

// Encoding
beam str_to_hex("AB")              // "4142"
beam base64_encode("Hello")        // "SGVsbG8="
atom decoded = base64_decode("SGVsbG8=")  // "Hello"
```

---

# Part VII — Fusion UI

---

## Chapter 16: Introduction to Fusion UI

### 16.1 Philosophy

Fusion UI is X-Phage's native cross-platform UI framework. It is built from first principles:

**No dependencies on platform UI:**
- No Jetpack Compose (no JVM)
- No SwiftUI (no Objective-C runtime)
- No HTML/CSS/JavaScript (no browser engine)
- No React Native (no Node.js)

**Own everything:**
- Layout engine (flex/grid/stack)
- Diff engine (partial re-renders)
- Paint engine (draw calls)
- GPU backends per platform

**Write once, run everywhere:**
```
Same .xui file →
    Android  (Vulkan)   identical pixels
    iOS      (Metal)    identical pixels
    macOS    (Metal)    identical pixels
    Windows  (Vulkan)   identical pixels
    Linux    (Vulkan)   identical pixels
    Web      (WebGPU)   identical pixels
    Smart TV (Vulkan)   identical pixels
    Watch    (GLES)     identical pixels
```

### 16.2 Setting Up

Install Fusion UI:
```bash
xpm add fusion-ui
```

Add to your `.xui` file:
```xphage
~link "fusion-ui"
```

### 16.3 Your First UI

```xphage
// hello_ui.xui
~link "fusion-ui"

fusion HelloScreen {
    Orbit(weave().padding(24)) {
        Vision("Hello from Fusion UI!")
        Spacer(16)
        Trigger("Click Me") {
            emit "button_clicked"
        }
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

**Orbit — Vertical Stack (like Column):**
```xphage
Orbit {
    Vision("Item 1")
    Vision("Item 2")
    Vision("Item 3")
}
// Items stacked vertically, top to bottom
```

**OrbitH — Horizontal Stack (like Row):**
```xphage
OrbitH {
    Vision("Left")
    Spacer(weight: 1)   // flexible space
    Vision("Right")
}
// Items side by side, left to right
```

**Canvas — Free-form Box:**
```xphage
Canvas(weave().width(200).height(200)) {
    Vision("Top-left")
    Vision("Center", weave().offset(75, 90))
    Vision("Bottom", weave().offset(50, 180))
}
```

**Layer — Z-stack (overlapping):**
```xphage
Layer {
    Image("background.jpg", weave().fill())
    Orbit(weave().fill()) {      // overlaid on image
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
// 3-column grid, 8dp gap between cells
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

**Vision — Text:**
```xphage
Vision("Simple text")
Vision("Styled text", weave().padding(8))
Vision(f"Dynamic: {score}")
Vision(long_text, weave().fill_width())
```

**Signal — Card/Surface:**
```xphage
Signal(weave().corner_radius(12).elevation(2).padding(16)) {
    Vision("Card content")
    Trigger("Action")
}
```

**Image:**
```xphage
Image("assets/logo.png", weave().width(100).height(100))
Image("https://example.com/photo.jpg", weave().fill_width().corner_radius(8))
```

**Spacer:**
```xphage
Spacer(16)              // 16dp fixed space
Spacer(weight: 1)       // flexible — takes remaining space
Spacer()                // minimal space
```

**Divider:**
```xphage
Divider()
Divider(weave().padding_h(16))   // with horizontal padding
```

### 17.3 Interactive Components

**Trigger — Button:**
```xphage
// Simple
Trigger("Click Me") { emit "clicked" }

// Styled
Trigger("Primary", weave()
    .background("#6C63FF")
    .corner_radius(8)
    .padding(12, 24)) {
    emit "primary_action"
}

// Icon button
Trigger("X", weave()
    .width(40).height(40)
    .corner_radius(20)
    .background("#FF4444")) {
    emit "close"
}
```

**Input — Text Field:**
```xphage
// Basic
Input("Enter text here") {
    absorb "on_change" { text_value = input_value }
}

// Styled
Input("Search...", weave()
    .fill_width()
    .corner_radius(24)
    .padding(12, 20)
    .background("#F0F0F0")) {
    absorb "on_change" { search_query = input_value }
}
```

**Toggle — Switch:**
```xphage
flux dark_mode: bool = false

Toggle(dark_mode) {
    absorb "on_toggle" { dark_mode = !dark_mode }
}
```

**Slider:**
```xphage
flux volume: float = 50.0

Slider(volume, 0, 100) {
    absorb "on_slide" { volume = slider_value }
}

Vision(f"Volume: {volume}%")
```

### 17.4 The weave Modifier System

Every component accepts a `weave()` modifier chain:

```xphage
// Size
weave().width(200)
weave().height(100)
weave().fill_width()            // match parent width
weave().fill_height()           // match parent height
weave().fill()                  // fill both
weave().weight(1)               // flex weight
weave().min_width(100)
weave().max_height(300)
weave().aspect_ratio(16.0/9.0)

// Spacing
weave().padding(16)             // all sides
weave().padding(8, 16)          // vertical, horizontal
weave().padding(4, 8, 4, 8)     // top, right, bottom, left
weave().padding_h(16)           // horizontal only
weave().padding_v(8)            // vertical only
weave().offset(10, -5)          // x, y offset

// Appearance
weave().background("#1A1A2E")
weave().background("#FFFFFF")
weave().alpha(0.8)              // 80% opacity
weave().corner_radius(12)       // all corners
weave().corner_radius(16, 16, 0, 0) // tl, tr, br, bl
weave().border_width(1)
weave().border_color("#DDDDDD")
weave().elevation(4)            // shadow
weave().shadow(...)
weave().rotate(45)              // degrees
weave().scale(1.5)

// Interaction
weave().clickable { emit "clicked" }
weave().focusable()
weave().scrollable_v()
weave().scrollable_h()

// Layout control
weave().z_index(10)
weave().clip()

// Composition — combine modifiers
weave()
    .fill_width()
    .corner_radius(16)
    .elevation(3)
    .padding(16, 24)
    .background("#6C63FF")

// Store and reuse
atom button_style = weave()
    .corner_radius(8)
    .padding(12, 24)
    .background("#6C63FF")

Trigger("Primary", button_style)
Trigger("Secondary", button_style)
Trigger("Cancel",    button_style)
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

// Easing functions
// ease_in      — starts slow, ends fast
// ease_out     — starts fast, ends slow (most natural for UI)
// ease_in_out  — slow start and end, fast middle
// linear       — constant speed
// bounce       — bounces at the end
// elastic      — overshoots and springs back
```

### 18.2 Theme System

```xphage
// Default light theme
xphage run MyApp with Theme.light()

// Dark theme
xphage run MyApp with Theme.dark()

// AeonCoreX theme (futuristic dark)
xphage run MyApp with Theme.aeon()

// Custom theme
atom my_theme = spawn Theme {
    colors: spawn ColorScheme {
        primary:     "#FF6B6B"
        secondary:   "#4ECDC4"
        background:  "#1A1A2E"
        on_surface:  "#EEEEEE"
    }
    dark_mode: true
    font_scale: 1.1
}

xphage run MyApp with my_theme
```

### 18.3 Responding to Theme

```xphage
flux app_theme: str = "dark"

// Platform-specific and theme-specific UI
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
// Mark a function as async
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
    // Await results via events
    absorb "user_data_ready"       { update_user_section(data) }
    absorb "analytics_ready"       { update_charts(analytics) }
    absorb "notifications_ready"   { update_badge(count) }
}
```

---

## Chapter 20: AI and Machine Learning

### 20.1 Tensor Operations

```xphage
~link "ai"

// Create tensors
atom zeros   = tensor_zeros("3x4")
atom ones    = tensor_ones("128x768")
atom random  = tensor_rand("512x512")
atom data    = tensor_from_data("2x3", "1.0,2.0,3.0,4.0,5.0,6.0")

// Math
atom sum     = tensor_add(a, b)
atom product = tensor_matmul(a, b)    // matrix multiplication
atom scaled  = tensor_scale(a, 0.5)
atom normed  = tensor_normalize(a)

// GPU/NPU acceleration
atom fast_a  = tensor_to_gpu(a)       // move to GPU
atom npu_a   = tensor_to_npu(a)       // move to NPU (if available)
atom cpu_r   = tensor_to_cpu(result)  // move back to CPU

// Neural operations
atom relu    = nn_relu(input)
atom softmax = nn_softmax(logits, 1)
atom out     = nn_linear(input, weight, bias)
atom attn    = nn_attention(q, k, v, mask)
```

### 20.2 Loading Models

```xphage
// ONNX model
atom model = model_load("sentiment.onnx", "gpu")
atom result = model_run(model, text_embedding)

// Named I/O
atom result = model_run_named(model,
    "input=text_tensor,output=logits")
```

### 20.3 LLM Inference

```xphage
~link "ai"

// Load LLaMA model (GGUF format — llama.cpp backend)
atom llm = llm_load("llama-3.2-3b-instruct.gguf",
    4096,    // context length
    32       // GPU layers (-1 = all on GPU)
)

// Generate text
atom response = llm_generate(llm,
    "Explain quantum computing in simple terms:",
    512     // max tokens
)
beam response

// With parameters
atom creative = llm_generate_ex(llm,
    "Write a haiku about X-Phage:",
    100,    // max tokens
    0.9,    // temperature (higher = more creative)
    0.95    // top_p
)
beam creative

// Chat
atom messages = "[{\"role\":\"user\",\"content\":\"Hello!\"}]"
atom reply    = llm_chat(llm, messages)
beam reply

// Embeddings (for semantic search, RAG)
atom embedding = llm_embed(llm, "X-Phage is a systems language")
```

### 20.4 Vector Database (RAG)

```xphage
// Create in-memory vector DB for semantic search
atom db = vecdb_new(768)   // 768-dimensional embeddings

// Index documents
atom docs = [
    "X-Phage is a compiled language",
    "Fusion UI uses Vulkan on Android",
    "forge creates struct types"
]

for doc in str_split(docs, ",") {
    atom embed = llm_embed(llm, doc)
    vecdb_add(db, doc, embed, doc)
}

// Semantic search
atom query   = llm_embed(llm, "how to create a structure?")
atom results = vecdb_search(db, query, 3)  // top 3 results
beam results

// Save/load for persistence
vecdb_save(db, "knowledge_base.vdb")
atom db2 = vecdb_load("knowledge_base.vdb")
```

---

# Part IX — Building Real Projects

---

## Chapter 21: Complete Projects

### Project 1: Command-Line Tool

A file statistics tool:

```xphage
// file_stats.xp0
~link "io"
~link "string"
~link "math"

forge FileStats {
    path:       str = ""
    size:       int = 0
    lines:      int = 0
    words:      int = 0
    chars:      int = 0
    ext:        str = ""
}

pulse analyze_file(path: str) -> FileStats {
    atom content  = io_read(path)
    atom all_lines = str_split_lines(content)
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
        // Analyze all files in directory
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
        // Analyze single file
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
    // Returns comma-separated repo summaries
    // (until typed generics land in Phase 6)
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
    id:        int  = 0
    title:     str  = ""
    done:      bool = false
    priority:  int  = 1     // 1=low, 2=med, 3=high
}
```

```xphage
// layout.xui
~link "fusion-ui"
~link "models.xh"

flux tasks:      str  = ""     // JSON array of tasks
flux new_title:  str  = ""
flux filter:     str  = "all"  // all, active, done

atom priority_color = |p: int| -> str {
    probe p {
        diverge 3 -> "#FF4444"   // high
        diverge 2 -> "#FF9800"   // medium
        diverge _ -> "#4CAF50"   // low
    }
}

fusion TaskApp {
    Scaffold {
        top_bar: OrbitH(weave().background("#6C63FF").padding(16)) {
            Vision("X-Phage Tasks")
            Spacer(weight: 1)
            Trigger("Clear Done") { emit "clear_done" }
        }

        content: Orbit(weave().padding(16)) {
            // Add new task
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

            // Filter tabs
            OrbitH(weave().fill_width()) {
                Trigger("All",    weave().weight(1)) { filter = "all" }
                Trigger("Active", weave().weight(1)) { filter = "active" }
                Trigger("Done",   weave().weight(1)) { filter = "done" }
            }

            Spacer(8)

            // Task list (renders from flux `tasks` state)
            // Real impl: parse tasks JSON and render each
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
~link "crypt"

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
    // toggle done state
    tasks = toggle_task_in_list(task_id, task_list)
}

absorb "clear_done" {
    task_list = vec_filter(task_list, |t| !str_contains(t, ":true"))
    tasks     = vec_join(task_list, "|")
}

pulse main() {
    beam f"X-Phage Task App on {os_platform()}"
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
| `scan` | 1 | Read input |
| `bypass` | 1 | System command |
| `quantum` | 1 | Spawn thread |
| `chronos` | 1 | Sleep |
| `ether` | 1 | Network |
| `matrix` | 1 | Array |
| `vortex` | 1 | Try/catch |
| `~link` | 1 | Import |
| `if/elif/else` | 1 | Conditional |
| `while` | 1 | Loop |
| `for/in` | 1 | Iterator |
| `break/continue` | 1 | Loop control |
| `forge` | 2 | Struct type |
| `nexus` | 2 | Interface |
| `impl` | 2 | Implement |
| `self` | 2 | Self reference |
| `flux` | 2 | Reactive state |
| `probe/diverge` | 2 | Pattern match |
| `emit` | 2 | Event dispatch |
| `absorb` | 2 | Event handler |
| `weave` | 2 | UI modifier |
| `strand` | 2 | Animation |
| `mesh` | 2 | Grid layout |
| `spawn` | 2 | Create instance |
| `own` | 3 | Unique ownership |
| `ref` | 3 | Immutable borrow |
| `mut_ref` | 3 | Mutable borrow |
| `async` | 3 | Async function |
| `await` | 3 | Await result |
| `lambda` | 3 | Lambda keyword |
| `proc` | 3 | Process capture |
| `env` | 3 | Environment var |
| `unsafe` | 3 | Unsafe block |
| `use` | 3 | Scope import |
| `pub/priv` | 3 | Visibility |
| `extern` | 3 | External linkage |

---

## Appendix B: Operator Quick Reference

```
Arithmetic:     + - * / %
Comparison:     == != < > <= >=
Logical:        && || !  (and or not)
Bitwise:        & | ^ ~ << >>
Assignment:     = += -= *= /=
Pipeline:       |>
Error prop:     ?
Range:          ..
f-string:       f"text {expr}"
Member access:  object.field
Index:          array[i]
Lambda:         |params| expression
```

---

## Appendix C: Standard Library Reference

```
~link "io"           File, process, env, path, glob
~link "math"         Arithmetic, trig, stats, Vec2/3, Mat4
~link "string"       String processing, regex, encoding
~link "collections"  Option, Result, Vec, Map, Set, Queue, Stack
~link "net"          HTTP, WebSocket, TCP, DNS, JSON
~link "os"           Platform, threads, time, signals
~link "crypt"        AES, RSA, SHA, Argon2, UUID
~link "ai"           Tensor, neural ops, LLM, NPU
~link "fusion-ui"    Full UI framework
```

---

## Appendix D: Compiler Flags

```bash
# Build commands
xphage build src/main.xp0                    # debug build
xphage build -O2 src/main.xp0               # optimized
xphage build -o myapp src/main.xp0           # custom output
xphage build --target android src/main.xp0   # Android
xphage build --target ios src/main.xp0       # iOS
xphage build --target web src/main.xp0       # WebAssembly

# Run
xphage run src/main.xp0                      # compile + run

# Package management
xpm add fusion-ui                            # install package
xpm add net                                  # install stdlib module
xpm remove fusion-ui                         # remove package
xpm list                                     # list installed
xpm update                                   # update all
```

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
→ Missing closing parenthesis in function call or params.

[error] .xh only allows declarations
→ Remove function calls, if statements, loops from .xh files.

[vortex] <error message>
→ Runtime error caught by vortex block.
→ Check the error message for details.
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

### Library
```
my-lib/
├── src/
│   ├── lib.xh          ← public API
│   └── internal.xh     ← internal types
├── tests/
│   └── test_main.xp0
├── xpm.toml
└── README.md
```

### Mobile App (with Fusion UI)
```
my-app/
├── src/
│   ├── main.xp0        ← entry point + event handlers
│   ├── models.xh       ← data types
│   ├── logic.xh        ← business logic signatures
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

### System/OS Project
```
my-os/
├── src/
│   ├── boot.xp0        ← bootloader entry
│   ├── kernel.xp0      ← kernel main
│   ├── interrupts.xh   ← IRQ handlers
│   ├── memory.xh       ← memory management
│   ├── drivers/
│   │   ├── uart.xh
│   │   ├── disk.xh
│   │   └── network.xh
│   └── fs/
│       ├── vfs.xh
│       └── ext4.xh
├── linker.ld
└── Makefile
```

---

*The X-Phage Programming Language is developed by AeonCoreX Lab.*
*© 2026 AeonCoreX Lab. All Rights Reserved.*

*"From silicon to the stars."*
