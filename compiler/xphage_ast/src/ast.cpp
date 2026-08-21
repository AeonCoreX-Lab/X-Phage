// ============================================================
// xphage_ast — AST Implementation v3.5.0
// ============================================================
#include "../include/ast.hpp"
#include <iostream>
#include <sstream>
#include <cassert>

namespace xphage::ast_module {

// ── ASTBuilder ───────────────────────────────────────────────
static ASTNodePtr mk(NodeKind k, std::string v, uint32_t line) {
    auto n = std::make_shared<ASTNode>(k, std::move(v));
    n->span.line = line;
    return n;
}

ASTNodePtr ASTBuilder::make_pulse(std::string name, uint32_t line) {
    return mk(NodeKind::PulseDecl, std::move(name), line);
}
ASTNodePtr ASTBuilder::make_global(std::string name, std::string value, uint32_t line) {
    auto n = mk(NodeKind::GlobalDecl, std::move(name), line);
    n->extra = std::move(value);
    return n;
}
ASTNodePtr ASTBuilder::make_atom(std::string name, std::string value, uint32_t line) {
    auto n = mk(NodeKind::AtomDecl, std::move(name), line);
    n->children.push_back(make_string_lit(value, line));
    return n;
}
ASTNodePtr ASTBuilder::make_shadow(std::string name, std::string value, uint32_t line) {
    auto n = mk(NodeKind::ShadowDecl, std::move(name), line);
    n->children.push_back(make_string_lit(value, line));
    return n;
}
ASTNodePtr ASTBuilder::make_beam(std::string expr, uint32_t line) {
    auto n = mk(NodeKind::BeamStmt, "", line);
    n->children.push_back(make_identifier(std::move(expr), line));
    return n;
}
ASTNodePtr ASTBuilder::make_bypass(std::string target, uint32_t line) {
    return mk(NodeKind::BypassStmt, std::move(target), line);
}
ASTNodePtr ASTBuilder::make_quantum(std::string task, uint32_t line) {
    return mk(NodeKind::QuantumStmt, std::move(task), line);
}
ASTNodePtr ASTBuilder::make_link(std::string module_path, uint32_t line) {
    return mk(NodeKind::LinkStmt, std::move(module_path), line);
}
ASTNodePtr ASTBuilder::make_chronos(std::string ms, uint32_t line) {
    return mk(NodeKind::ChronosStmt, std::move(ms), line);
}
ASTNodePtr ASTBuilder::make_ether(std::string target, std::string data, uint32_t line) {
    auto n = mk(NodeKind::EtherStmt, std::move(target), line);
    n->extra = std::move(data);
    return n;
}
ASTNodePtr ASTBuilder::make_vortex(uint32_t line) {
    return mk(NodeKind::VortexStmt, "vortex", line);
}
ASTNodePtr ASTBuilder::make_void(uint32_t line) {
    return mk(NodeKind::VoidStmt, "void", line);
}
ASTNodePtr ASTBuilder::make_synapse(std::string id, std::string api, uint32_t line) {
    auto n = mk(NodeKind::SynapseStmt, std::move(id), line);
    n->extra = std::move(api);
    return n;
}
ASTNodePtr ASTBuilder::make_matrix(std::string id, std::string size, uint32_t line) {
    auto n = mk(NodeKind::MatrixStmt, std::move(id), line);
    n->extra = std::move(size);
    return n;
}
ASTNodePtr ASTBuilder::make_block(uint32_t line) {
    return mk(NodeKind::Block, "", line);
}
ASTNodePtr ASTBuilder::make_string_lit(std::string value, uint32_t line) {
    return mk(NodeKind::StringLit, std::move(value), line);
}
ASTNodePtr ASTBuilder::make_number_lit(std::string value, uint32_t line) {
    return mk(NodeKind::IntLit, std::move(value), line);
}
ASTNodePtr ASTBuilder::make_identifier(std::string name, uint32_t line) {
    return mk(NodeKind::Identifier, std::move(name), line);
}
ASTNodePtr ASTBuilder::make_binop(std::string op, ASTNodePtr lhs,
                                    ASTNodePtr rhs, uint32_t line) {
    auto n = mk(NodeKind::BinaryOp, std::move(op), line);
    n->children.push_back(std::move(lhs));
    n->children.push_back(std::move(rhs));
    return n;
}
ASTNodePtr ASTBuilder::make_config_block(uint32_t line) {
    return mk(NodeKind::ConfigBlock, "", line);
}
ASTNodePtr ASTBuilder::add_config_pair(ASTNodePtr block,
                                        std::string key, std::string value) {
    assert(block && block->kind == NodeKind::ConfigBlock);
    auto pair = mk(NodeKind::ConfigPair, std::move(key), block->span.line);
    pair->extra = std::move(value);
    block->children.push_back(pair);
    return block;
}

// ── Visitor walk ─────────────────────────────────────────────
void walk_node(const ASTNode& node, ASTVisitor& visitor, int depth) {
    visitor.visit_pre(node, depth);
    for (auto& child : node.children)
        if (child) walk_node(*child, visitor, depth + 1);
    visitor.visit_post(node, depth);
}

void walk(const Program& prog, ASTVisitor& visitor) {
    for (auto& node : prog)
        if (node) walk_node(*node, visitor, 0);
}

// ── Transformer ──────────────────────────────────────────────
ASTNodePtr transform_node(ASTNodePtr node, const NodeTransform& fn) {
    if (!node) return nullptr;
    // Transform children first (bottom-up)
    auto copy = std::make_shared<ASTNode>(*node);
    copy->children.clear();
    for (auto& child : node->children)
        copy->children.push_back(transform_node(child, fn));
    // Apply transform to the (copied) node
    return fn(copy);
}

Program transform_program(const Program& prog, const NodeTransform& fn) {
    Program result;
    for (auto& node : prog)
        if (auto t = transform_node(node, fn)) result.push_back(t);
    return result;
}

// ── Pretty-printer ───────────────────────────────────────────
std::string node_kind_name(NodeKind k) {
    switch (k) {
        // ── Top-level ──────────────────────────────────────
        case NodeKind::Program:       return "Program";
        case NodeKind::Block:         return "Block";

        // ── Declarations ───────────────────────────────────
        case NodeKind::PulseDecl:     return "PulseDecl";
        case NodeKind::AsyncPulseDecl:return "AsyncPulseDecl";
        case NodeKind::GlobalDecl:    return "GlobalDecl";
        case NodeKind::AtomDecl:      return "AtomDecl";
        case NodeKind::ShadowDecl:    return "ShadowDecl";
        case NodeKind::ConstDecl:     return "ConstDecl";
        case NodeKind::ForgeDecl:     return "ForgeDecl";
        case NodeKind::NexusDecl:     return "NexusDecl";
        case NodeKind::FluxDecl:      return "FluxDecl";
        case NodeKind::ImplDecl:      return "ImplDecl";
        case NodeKind::UseDecl:       return "UseDecl";
        case NodeKind::RealmDecl:     return "RealmDecl";

        // ── Statements ─────────────────────────────────────
        case NodeKind::IfStmt:        return "IfStmt";
        case NodeKind::ElifStmt:      return "ElifStmt";
        case NodeKind::ElseStmt:      return "ElseStmt";
        case NodeKind::WhileStmt:     return "WhileStmt";
        case NodeKind::ForStmt:       return "ForStmt";
        case NodeKind::ReturnStmt:    return "ReturnStmt";
        case NodeKind::BreakStmt:     return "BreakStmt";
        case NodeKind::ContinueStmt:  return "ContinueStmt";
        case NodeKind::BeamStmt:      return "BeamStmt";
        case NodeKind::BypassStmt:    return "BypassStmt";
        case NodeKind::QuantumStmt:   return "QuantumStmt";
        case NodeKind::ScanStmt:      return "ScanStmt";
        case NodeKind::LinkStmt:      return "LinkStmt";
        case NodeKind::ChronosStmt:   return "ChronosStmt";
        case NodeKind::EtherStmt:     return "EtherStmt";
        case NodeKind::VortexStmt:    return "VortexStmt";
        case NodeKind::VoidStmt:      return "VoidStmt";
        case NodeKind::SynapseStmt:   return "SynapseStmt";
        case NodeKind::MatrixStmt:    return "MatrixStmt";
        case NodeKind::ProbeStmt:     return "ProbeStmt";
        case NodeKind::ProbeArm:      return "ProbeArm";
        case NodeKind::EmitStmt:      return "EmitStmt";
        case NodeKind::AbsorbStmt:    return "AbsorbStmt";
        case NodeKind::YieldStmt:     return "YieldStmt";
        case NodeKind::ExprStmt:      return "ExprStmt";

        // ── UI Declarations ────────────────────────────────
        case NodeKind::FusionDecl:    return "FusionDecl";
        case NodeKind::UIComponent:   return "UIComponent";
        case NodeKind::WeaveExpr:     return "WeaveExpr";
        case NodeKind::StrandDecl:    return "StrandDecl";

        // ── Expressions ─────────────────────────────────────
        case NodeKind::Identifier:    return "Identifier";
        case NodeKind::PathExpr:      return "PathExpr";
        case NodeKind::StringLit:     return "StringLit";
        case NodeKind::FStringLit:    return "FStringLit";
        case NodeKind::IntLit:        return "IntLit";
        case NodeKind::FloatLit:      return "FloatLit";
        case NodeKind::BoolLit:       return "BoolLit";
        case NodeKind::NullLit:       return "NullLit";
        case NodeKind::BinaryOp:      return "BinaryOp";
        case NodeKind::UnaryOp:       return "UnaryOp";
        case NodeKind::AssignExpr:    return "AssignExpr";
        case NodeKind::CallExpr:      return "CallExpr";
        case NodeKind::IndexExpr:     return "IndexExpr";
        case NodeKind::MemberExpr:    return "MemberExpr";
        case NodeKind::PipelineExpr:  return "PipelineExpr";
        case NodeKind::RangeExpr:     return "RangeExpr";
        case NodeKind::LambdaExpr:    return "LambdaExpr";
        case NodeKind::ProcExpr:      return "ProcExpr";
        case NodeKind::EnvExpr:       return "EnvExpr";
        case NodeKind::GlobExpr:      return "GlobExpr";
        case NodeKind::SpawnExpr:     return "SpawnExpr";
        case NodeKind::CastExpr:      return "CastExpr";
        case NodeKind::TypeofExpr:    return "TypeofExpr";
        case NodeKind::SizeofExpr:    return "SizeofExpr";
        case NodeKind::AwaitExpr:     return "AwaitExpr";
        case NodeKind::PropagateExpr: return "PropagateExpr";

        // ── Type annotations ────────────────────────────────
        case NodeKind::TypeAnnot:     return "TypeAnnot";
        case NodeKind::OwnType:       return "OwnType";
        case NodeKind::RefType:       return "RefType";
        case NodeKind::MutRefType:    return "MutRefType";

        // ── Field in forge/nexus ─────────────────────────────
        case NodeKind::FieldDecl:     return "FieldDecl";
        case NodeKind::MethodDecl:    return "MethodDecl";

        // ── Config (deprecated, kept for compat) ─────────────
        case NodeKind::ConfigBlock:   return "ConfigBlock";
        case NodeKind::ConfigPair:    return "ConfigPair";

        default:                      return "Unknown";
    }
}

void print_node(const ASTNode& node, std::ostream& out,
                int depth, int indent_size) {
    std::string indent(depth * indent_size, ' ');
    out << indent
        << "[" << node_kind_name(node.kind) << "]";
    if (!node.value.empty()) out << " '" << node.value << "'";
    if (!node.extra.empty()) out << " | '" << node.extra << "'";
    if (node.span.line > 0)  out << " @L" << node.span.line;
    out << "\n";
    for (auto& c : node.children)
        if (c) print_node(*c, out, depth + 1, indent_size);
}

void print_ast(const Program& prog, std::ostream& out, int indent_size) {
    out << "; X-Phage AST dump — " << prog.size() << " top-level node(s)\n\n";
    for (auto& n : prog)
        if (n) print_node(*n, out, 0, indent_size);
}

// ── Symbol table ─────────────────────────────────────────────
void SymbolTable::enter_scope() {
    scopes_.push_back({});
}
void SymbolTable::exit_scope() {
    if (!scopes_.empty()) scopes_.pop_back();
}
bool SymbolTable::define(const Symbol& sym) {
    if (scopes_.empty()) scopes_.push_back({});
    auto& top = scopes_.back();
    if (top.count(sym.name)) return false; // already defined
    top[sym.name] = sym;
    return true;
}
bool SymbolTable::resolve(const std::string& name, Symbol& out) const {
    // Search from innermost scope outward
    for (int i = (int)scopes_.size() - 1; i >= 0; i--) {
        auto it = scopes_[i].find(name);
        if (it != scopes_[i].end()) { out = it->second; return true; }
    }
    return false;
}
bool SymbolTable::defined_in_current_scope(const std::string& name) const {
    if (scopes_.empty()) return false;
    return scopes_.back().count(name) > 0;
}
void SymbolTable::dump(std::ostream& out) const {
    out << "SymbolTable (" << scopes_.size() << " scope(s)):\n";
    for (size_t s = 0; s < scopes_.size(); s++) {
        out << "  [scope " << s << "]\n";
        for (auto& [name, sym] : scopes_[s])
            out << "    " << sym.kind << " " << name
                << (sym.constant ? " (const)" : "") << "\n";
    }
}

// ── Semantic analyser ─────────────────────────────────────────
class SemanticAnalyser : public ASTVisitor {
public:
    SemanticResult result;

    void visit_pre(const ASTNode& node, int depth) override {
        switch (node.kind) {
        case NodeKind::PulseDecl:
            result.symbols.enter_scope();
            if (!node.value.empty()) {
                Symbol s;
                s.name = node.value; s.kind = "pulse"; s.line = node.span.line;
                result.symbols.define(s);
            }
            break;

        case NodeKind::GlobalDecl: {
            Symbol s;
            s.name = node.value; s.kind = "global";
            s.type_hint = "string"; s.line = node.span.line;
            result.symbols.define(s);
            break;
        }

        case NodeKind::AtomDecl: {
            Symbol s;
            s.name = node.value; s.kind = "atom";
            s.constant = true; s.line = node.span.line;
            if (result.symbols.defined_in_current_scope(node.value)) {
                result.errors.push_back(
                    {"Redefinition of atom '" + node.value + "'", node.span.line});
            } else {
                result.symbols.define(s);
            }
            break;
        }

        case NodeKind::ShadowDecl: {
            Symbol s;
            s.name = node.value; s.kind = "shadow";
            s.constant = false; s.line = node.span.line;
            result.symbols.define(s);
            break;
        }

        case NodeKind::BeamStmt: {
            // Warn if beaming an empty expression
            if (node.children.empty() && node.value.empty()) {
                result.warnings.push_back({"beam with no expression", node.span.line});
            }
            break;
        }

        case NodeKind::BypassStmt: {
            if (node.value.empty()) {
                result.errors.push_back(
                    {"bypass requires a target name", node.span.line});
            }
            break;
        }

        default: break;
        }
    }

    void visit_post(const ASTNode& node, int) override {
        if (node.kind == NodeKind::PulseDecl)
            result.symbols.exit_scope();
    }
};

SemanticResult analyse(const Program& prog) {
    SemanticAnalyser analyser;
    analyser.result.symbols.enter_scope(); // global scope
    walk(prog, analyser);
    analyser.result.symbols.exit_scope();
    return analyser.result;
}

} // namespace xphage::ast_module
