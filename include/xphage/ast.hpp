#pragma once
// ============================================================
// X-Phage AST — Token + AST Node Definitions v3.5.0
// ============================================================
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <cstdint>

// ============================================================
// Token layer (used by lexer + parser)
// ============================================================
enum TokenType {
    // Core keywords
    PULSE, SHADOW, ATOM, BEAM, SCAN, LINK, MATRIX, MATH,
    BYPASS, QUANTUM, VORTEX, SYNAPSE, CHRONOS, ETHER, VOID, GLOBAL,

    // Fusion UI
    FUSION, SIGNAL, VISION, ORBIT, TRIGGER, INPUT, Z_PLANE,

    // Syntax / punctuation
    IDENTIFIER, STRING, NUMBER, EQUAL, PLUS, MINUS, MULTIPLY, DIVIDE,
    L_BRACE, R_BRACE, L_BRACKET, R_BRACKET, LPAREN, RPAREN,
    COLON, COMMA, ARROW, TILDE_LINK, HASH_DEFINE, AT_SIGN,
    UNKNOWN, END_OF_FILE
};

struct Token {
    TokenType   type;
    std::string value;
    uint32_t    line = 0;
    uint32_t    col  = 0;
};

// ============================================================
// AST Node layer (used by parser + IR lowering)
// ============================================================
enum class NodeKind {
    // Top-level
    Program, Block,
    // Declarations
    PulseDecl, GlobalDecl, AtomDecl, ShadowDecl,
    // Statements
    BeamStmt, BypassStmt, QuantumStmt, ScanStmt,
    MatrixStmt, ChronosStmt, EtherStmt, VortexStmt,
    VoidStmt, SynapseStmt, LinkStmt,
    // Fusion UI
    FusionDecl, UIComponent,
    // Expressions
    Identifier, StringLit, NumberLit, BinaryOp, Call,
    // Config block {key: val, ...}
    ConfigBlock, ConfigPair,
};

struct Span {
    uint32_t    line = 0;
    uint32_t    col  = 0;
    std::string file;
};

struct ASTNode {
    NodeKind    kind;
    Span        span;
    std::string value;   // token text / name
    std::string extra;   // secondary value (op type, type name…)
    std::unordered_map<std::string, std::string> attrs;
    std::vector<std::shared_ptr<ASTNode>> children;

    ASTNode() = default;
    explicit ASTNode(NodeKind k, std::string v = "", uint32_t line = 0)
        : kind(k), value(std::move(v)) { span.line = line; }
};

using ASTNodePtr = std::shared_ptr<ASTNode>;
using Program    = std::vector<ASTNodePtr>;

// Fusion UI node (runtime-level, separate from AST)
struct FusionNode {
    std::string type;
    std::string id;
    std::unordered_map<std::string, std::string> props;
    std::vector<std::shared_ptr<FusionNode>> children;

    FusionNode() = default;
    explicit FusionNode(std::string t) : type(std::move(t)) {}
};
