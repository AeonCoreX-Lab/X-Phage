#pragma once
// ============================================================
// xphage_ast — AST Module Header v3.5.0
//
// Owns:
//   • ASTNode tree construction helpers
//   • AST visitor interface
//   • AST pretty-printer
//   • Symbol table (pre-IR name resolution)
// ============================================================
#include "xphage/ast.hpp"
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <functional>
#include <ostream>

namespace xphage::ast_module {

// ── ASTBuilder — fluent factory helpers ─────────────────────
struct ASTBuilder {
    static ASTNodePtr make_pulse(std::string name, uint32_t line = 0);
    static ASTNodePtr make_global(std::string name, std::string value, uint32_t line = 0);
    static ASTNodePtr make_atom(std::string name, std::string value, uint32_t line = 0);
    static ASTNodePtr make_shadow(std::string name, std::string value, uint32_t line = 0);
    static ASTNodePtr make_beam(std::string expr, uint32_t line = 0);
    static ASTNodePtr make_bypass(std::string target, uint32_t line = 0);
    static ASTNodePtr make_quantum(std::string task, uint32_t line = 0);
    static ASTNodePtr make_link(std::string module_path, uint32_t line = 0);
    static ASTNodePtr make_chronos(std::string ms, uint32_t line = 0);
    static ASTNodePtr make_ether(std::string target, std::string data, uint32_t line = 0);
    static ASTNodePtr make_vortex(uint32_t line = 0);
    static ASTNodePtr make_void(uint32_t line = 0);
    static ASTNodePtr make_synapse(std::string id, std::string api, uint32_t line = 0);
    static ASTNodePtr make_matrix(std::string id, std::string size, uint32_t line = 0);
    static ASTNodePtr make_block(uint32_t line = 0);
    static ASTNodePtr make_string_lit(std::string value, uint32_t line = 0);
    static ASTNodePtr make_number_lit(std::string value, uint32_t line = 0);
    static ASTNodePtr make_identifier(std::string name, uint32_t line = 0);
    static ASTNodePtr make_binop(std::string op, ASTNodePtr lhs,
                                  ASTNodePtr rhs, uint32_t line = 0);
    static ASTNodePtr make_config_block(uint32_t line = 0);
    static ASTNodePtr add_config_pair(ASTNodePtr block,
                                       std::string key, std::string value);
};

// ── Visitor interface ────────────────────────────────────────
// Implement this to walk the AST without modifying it.
class ASTVisitor {
public:
    virtual ~ASTVisitor() = default;

    // Called before visiting children
    virtual void visit_pre(const ASTNode& node, int depth) {}
    // Called after visiting children
    virtual void visit_post(const ASTNode& node, int depth) {}
};

// Walk the entire program tree
void walk(const Program& prog, ASTVisitor& visitor);
void walk_node(const ASTNode& node, ASTVisitor& visitor, int depth = 0);

// ── ASTTransformer ───────────────────────────────────────────
// Returns a new transformed copy; original unchanged.
using NodeTransform = std::function<ASTNodePtr(ASTNodePtr)>;
ASTNodePtr transform_node(ASTNodePtr node, const NodeTransform& fn);
Program    transform_program(const Program& prog, const NodeTransform& fn);

// ── Pretty-printer ───────────────────────────────────────────
// Dumps the AST in a human-readable indented format.
void print_ast(const Program& prog, std::ostream& out = std::cout,
               int indent_size = 2);
void print_node(const ASTNode& node, std::ostream& out,
                int depth = 0, int indent_size = 2);
std::string node_kind_name(NodeKind k);

// ── Symbol table ─────────────────────────────────────────────
struct Symbol {
    std::string name;
    std::string kind;      // "atom", "shadow", "global", "pulse"
    std::string type_hint; // "string", "number", "unknown"
    bool        constant = false;
    uint32_t    line     = 0;
};

class SymbolTable {
public:
    void   enter_scope();
    void   exit_scope();
    bool   define(const Symbol& sym);
    bool   resolve(const std::string& name, Symbol& out) const;
    bool   defined_in_current_scope(const std::string& name) const;
    size_t scope_depth() const { return scopes_.size(); }
    void   dump(std::ostream& out = std::cout) const;

private:
    std::vector<std::unordered_map<std::string, Symbol>> scopes_;
};

// ── Semantic pass ─────────────────────────────────────────────
struct SemanticError {
    std::string message;
    uint32_t    line = 0;
};

struct SemanticResult {
    std::vector<SemanticError> errors;
    std::vector<SemanticError> warnings;
    SymbolTable                symbols;
    bool ok() const { return errors.empty(); }
};

// Run basic name-resolution and type-hint checks over the AST.
SemanticResult analyse(const Program& prog);

} // namespace xphage::ast_module
