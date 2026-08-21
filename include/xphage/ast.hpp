#pragma once
// ============================================================
// X-Phage AST — Token + AST Node Definitions v4.0.0
// Phases 1-3: Foundation + Type System + Systems Power
// AeonCoreX Lab
// ============================================================
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <cstdint>

// ============================================================
// TOKEN LAYER — used by Lexer + Parser
// ============================================================
enum TokenType {
    // ── Phase 1 Keywords ─────────────────────────────────────
    PULSE,      // function declaration
    ATOM,       // const variable
    SHADOW,     // mutable variable
    GLOBAL,     // module-level static
    RETURN,     // return value
    IF,         // if
    ELIF,       // elif
    ELSE,       // else
    WHILE,      // while loop
    FOR,        // for loop
    IN,         // in (for/in)
    BREAK,      // break
    CONTINUE,   // continue
    BEAM,       // print (std::cout)
    SCAN,       // input (std::cin)
    BYPASS,     // system call
    QUANTUM,    // thread
    CHRONOS,    // sleep
    ETHER,      // network
    MATRIX,     // array
    SYNAPSE,    // API connection
    LINK,       // ~link import
    VOID_KW,    // void type/null
    VORTEX,     // try/catch
    REALM,      // namespace block

    // ── Phase 2 Keywords ─────────────────────────────────────
    FORGE,      // struct/record
    NEXUS,      // interface/trait
    FLUX,       // reactive state
    PROBE,      // match/switch
    DIVERGE,    // match arm
    EMIT,       // event dispatch
    ABSORB,     // event handler
    WEAVE,      // UI modifier pipeline
    STRAND,     // animation block
    MESH,       // grid layout
    CAST,       // type cast
    SPAWN,      // struct instantiation
    IMPL,       // implement nexus for forge
    SELF,       // self reference

    // ── Phase 3 Keywords ─────────────────────────────────────
    OWN,        // unique ownership
    REF,        // immutable borrow
    MUT_REF,    // mutable borrow
    ASYNC,      // async pulse
    AWAIT,      // await result
    YIELD,      // generator yield
    USE,        // bring into scope
    PUB,        // public visibility
    PRIV,       // private visibility
    CONST,      // compile-time const
    STATIC,     // static method/field
    UNSAFE,     // unsafe block
    EXTERN,     // external linkage
    AS,         // type cast alias
    TYPEOF,     // type introspection
    SIZEOF,     // size of type
    LAMBDA,     // lambda keyword
    PROC,       // run process, capture output
    GLOB,       // glob file patterns
    ENV,        // environment variable

    // ── Phase 6 Keywords ──────────────────────────────────────
    ENUM,       // enum type declaration

    // ── Fusion UI Components ─────────────────────────────────
    FUSION,     // UI component declaration
    ORBIT,      // Vertical stack
    ORBIT_H,    // Horizontal stack
    CANVAS,     // Free-form box
    SCAFFOLD,   // Page layout
    LAYER,      // Z-stack
    VISION,     // Text
    SIGNAL,     // Surface/Card
    TRIGGER,    // Button
    INPUT_UI,   // Text field
    TOGGLE,     // Switch/checkbox
    SLIDER,     // Range slider
    IMAGE,      // Image
    SPACER,     // Empty space
    DIVIDER,    // Separator

    // ── Built-in Types ────────────────────────────────────────
    TYPE_INT,   // int
    TYPE_FLOAT, // float
    TYPE_BOOL,  // bool
    TYPE_STR,   // str
    TYPE_AUTO,  // auto

    // ── Literal Values ────────────────────────────────────────
    TRUE,
    FALSE,
    NULL_LIT,   // null/void literal

    // ── Operators ─────────────────────────────────────────────
    PLUS, MINUS, STAR, SLASH, PERCENT,       // + - * / %
    EQ_EQ, BANG_EQ,                           // == !=
    LT, GT, LT_EQ, GT_EQ,                    // < > <= >=
    AND_AND, PIPE_PIPE, BANG,                 // && || !
    EQUAL,                                    // =
    PLUS_EQ, MINUS_EQ, STAR_EQ, SLASH_EQ,    // += -= *= /=
    PIPE_GT,                                  // |> pipeline
    DOT_DOT,                                  // .. range
    QUESTION,                                 // ? error propagation
    ARROW,                                    // -> return type
    FAT_ARROW,                                // => (unused, future)
    PIPE,                                     // | (lambda |x|)
    DOT,                                      // . member access
    COLON_COLON,                              // :: namespace
    AMPERSAND,                                // & address-of / bitwise AND
    LSHIFT,                                   // << left shift
    RSHIFT,                                   // >> right shift
    CARET,                                    // ^ bitwise XOR
    TILDE,                                    // ~ bitwise NOT

    // ── Punctuation ───────────────────────────────────────────
    LPAREN, RPAREN,     // ( )
    L_BRACE, R_BRACE,   // { }
    L_BRACKET, R_BRACKET, // [ ]
    COLON,              // :
    COMMA,              // ,
    SEMICOLON,          // ;
    AT,                 // @

    // ── Token Literals ────────────────────────────────────────
    IDENTIFIER,
    STRING,
    FSTRING,        // f"..." interpolated string
    NUMBER_INT,     // integer literal
    NUMBER_FLOAT,   // float literal

    // ── Meta ──────────────────────────────────────────────────
    END_OF_FILE,
    TOK_ERROR       // lexer error token
};

// Token with source location
struct Token {
    TokenType   type;
    std::string value;
    uint32_t    line = 1;
    uint32_t    col  = 1;
};

// ============================================================
// AST NODE LAYER — used by Parser + IR Lowering + Codegen
// ============================================================
enum class NodeKind {
    // ── Top-level ─────────────────────────────────────────────
    Program, Block,

    // ── Declarations ─────────────────────────────────────────
    PulseDecl,      // pulse name(params) -> ret { body }
    AsyncPulseDecl, // async pulse ...
    GlobalDecl,     // global name = expr
    AtomDecl,       // atom name: type = expr
    ShadowDecl,     // shadow name: type = expr
    ConstDecl,      // const name: type = expr
    ForgeDecl,      // forge Name { fields }
    NexusDecl,      // nexus Name { methods }
    FluxDecl,       // flux name: type = expr
    ImplDecl,       // impl Nexus for Forge { methods }
    UseDecl,        // use module::name
    RealmDecl,      // realm Name { decls }  → C++ namespace
    ExternDecl,     // extern "C" pulse name(params) -> ret  (FFI declaration, no body)
    EnumDecl,       // enum Name { Variant1  Variant2(Type1, Type2) ... }
    EnumVariant,    // one variant inside an EnumDecl — value=name,
                    // children are payload type names (as FieldDecl-
                    // like TypeAnnot placeholders, see parse_enum_decl)
    TypeParamDecl,  // one <T> or <T: Bound> generic type parameter —
                    // value=parameter name (e.g. "T"), extra=bound
                    // name if present (e.g. "Numeric"), empty if
                    // unbounded. A PulseDecl/ForgeDecl/EnumDecl with
                    // one or more of these as children (see each
                    // parser's own comment for exactly where they're
                    // stored) is generic; monomorphization replaces
                    // every use of the parameter name with a concrete
                    // type before semantic analysis's concrete-type
                    // checks and before either backend's codegen ever
                    // sees the declaration.

    // ── Statements ───────────────────────────────────────────
    IfStmt,         // if cond { body } elif { } else { }
    ElifStmt,       // elif cond { body }  (child of IfStmt)
    ElseStmt,       // else { body }       (child of IfStmt)
    WhileStmt,      // while cond { body }
    ForStmt,        // for var in expr { body }
    ReturnStmt,     // return expr
    BreakStmt,
    ContinueStmt,
    BeamStmt,       // beam expr
    BypassStmt,     // bypass "cmd"
    QuantumStmt,    // quantum { body }
    ScanStmt,       // scan var
    LinkStmt,       // ~link "module"
    ChronosStmt,    // chronos duration
    EtherStmt,      // ether url data
    VortexStmt,     // vortex { body } catch(e) { }
    VoidStmt,       // void
    SynapseStmt,    // synapse id api
    MatrixStmt,     // matrix name[size]
    ProbeStmt,      // probe expr { arms }
    ProbeArm,       // diverge pattern -> expr
    EmitStmt,       // emit "event" { data }
    AbsorbStmt,     // absorb "event" { body }
    YieldStmt,      // yield expr
    ExprStmt,       // standalone expression
    UnsafeBlock,    // unsafe { body }  (raw pointer / FFI block, transparent to codegen)

    // ── UI Declarations ───────────────────────────────────────
    FusionDecl,     // fusion Name { body }
    UIComponent,    // Orbit/Signal/Vision/... { body }
    WeaveExpr,      // weave().modifier().modifier()
    StrandDecl,     // strand name { animations }

    // ── Expressions ──────────────────────────────────────────
    Identifier,
    PathExpr,       // Realm::member  (scope-resolved access)
    StringLit,
    FStringLit,     // f"text {expr} text"
    IntLit,
    FloatLit,
    BoolLit,
    NullLit,
    BinaryOp,       // left op right
    UnaryOp,        // op expr
    AssignExpr,     // name = expr (also +=, -=, etc.)
    CallExpr,       // name(args)
    IndexExpr,      // expr[idx]
    MemberExpr,     // expr.field
    PipelineExpr,   // expr |> func
    RangeExpr,      // start .. end
    LambdaExpr,     // |params| body
    ProcExpr,       // proc "cmd"
    EnvExpr,        // env.VARNAME
    GlobExpr,       // glob "pattern"
    SpawnExpr,      // spawn TypeName { fields }
    TupleExpr,      // (e0, e1, ...) — a tuple literal/construction.
                     // children = the element expressions, in order.
    TupleDestructure, // atom (a, b, ...) = expr — value = "atom" or
                     // "shadow" (matches which keyword introduced the
                     // pattern); children[0..n-1] = one Identifier
                     // per bound name, children[n] = the initializer
                     // expression being destructured.
    CastExpr,       // cast(expr, Type)
    TypeofExpr,     // typeof(expr)
    SizeofExpr,     // sizeof(Type)
    AwaitExpr,      // await expr
    PropagateExpr,  // expr? error propagation

    // ── Type annotations ─────────────────────────────────────
    TypeAnnot,      // : typename
    OwnType,        // own T
    RefType,        // ref T
    MutRefType,     // mut_ref T

    // ── Field in forge/nexus ──────────────────────────────────
    FieldDecl,      // name: Type = default
    MethodDecl,     // pulse-like method signature

    // ── Config (deprecated, kept for compat) ─────────────────
    ConfigBlock,
    ConfigPair,
};

// Source span
struct Span {
    uint32_t    line = 0;
    uint32_t    col  = 0;
    std::string file;
};

// Universal AST node
struct ASTNode {
    NodeKind    kind;
    Span        span;
    std::string value;      // primary text (name, operator, etc.)
    std::string extra;      // secondary text (type name, op, etc.)
    std::string extra2;     // tertiary (return type annotation)
    std::unordered_map<std::string, std::string> attrs;
    std::vector<std::shared_ptr<ASTNode>> children;

    ASTNode() = default;
    explicit ASTNode(NodeKind k, std::string v = "",
                     uint32_t line = 0, uint32_t col = 0)
        : kind(k), value(std::move(v)) {
        span.line = line;
        span.col  = col;
    }
};

using ASTNodePtr = std::shared_ptr<ASTNode>;
using Program    = std::vector<ASTNodePtr>;

// Fusion UI node (runtime render tree)
struct FusionNode {
    std::string type;
    std::string id;
    std::unordered_map<std::string, std::string> props;
    std::vector<std::shared_ptr<FusionNode>> children;

    FusionNode() = default;
    explicit FusionNode(std::string t) : type(std::move(t)) {}
};

