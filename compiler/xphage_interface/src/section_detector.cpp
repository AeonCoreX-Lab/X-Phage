// ============================================================
// X-Phage Section Detector v4.1.0
// Classifies AST nodes → Logic / UI / Execution
// AeonCoreX Lab
// ============================================================
#include "../include/section_detector.hpp"
#include <sstream>
#include <unordered_map>

namespace xphage::interface {

bool SectionDetector::is_pure_literal(const ASTNodePtr& expr) {
    if (!expr) return true;
    switch (expr->kind) {
        case NodeKind::IntLit: case NodeKind::FloatLit:
        case NodeKind::BoolLit: case NodeKind::StringLit:
        case NodeKind::NullLit:
            return true;
        case NodeKind::UnaryOp:
            return !expr->children.empty() && is_pure_literal(expr->children[0]);
        default:
            return false;
    }
}

Section SectionDetector::classify(const ASTNode& n) {
    switch (n.kind) {
        case NodeKind::ForgeDecl:
        case NodeKind::NexusDecl:
        case NodeKind::ConstDecl:
        case NodeKind::UseDecl:
        case NodeKind::RealmDecl:
        case NodeKind::EnumDecl:
            return Section::Logic;

        case NodeKind::AtomDecl: {
            if (n.children.empty() || is_pure_literal(n.children[0]))
                return Section::Logic;
            return Section::Execution;
        }

        case NodeKind::PulseDecl:
        case NodeKind::AsyncPulseDecl: {
            bool has_body = false;
            for (auto& c : n.children)
                if (c && c->kind == NodeKind::Block) { has_body = true; break; }
            return has_body ? Section::Execution : Section::Logic;
        }

        case NodeKind::FusionDecl:
        case NodeKind::StrandDecl:
            return Section::UI;

        case NodeKind::LinkStmt:
            if (n.value == "fusion-ui" || n.value == "fusion") return Section::UI;
            return Section::Logic;

        case NodeKind::ImplDecl:
        case NodeKind::GlobalDecl:
        case NodeKind::FluxDecl:
        case NodeKind::ShadowDecl:
        case NodeKind::AbsorbStmt:
        case NodeKind::EmitStmt:
        case NodeKind::BeamStmt:
        case NodeKind::BypassStmt:
        case NodeKind::QuantumStmt:
        case NodeKind::ScanStmt:
        case NodeKind::ChronosStmt:
        case NodeKind::EtherStmt:
        case NodeKind::IfStmt:
        case NodeKind::WhileStmt:
        case NodeKind::ForStmt:
        case NodeKind::ReturnStmt:
        case NodeKind::BreakStmt:
        case NodeKind::ContinueStmt:
        case NodeKind::ProbeStmt:
        case NodeKind::VortexStmt:
        case NodeKind::YieldStmt:
        case NodeKind::ExprStmt:
        case NodeKind::Block:
            return Section::Execution;

        default:
            return Section::Execution;
    }
}

std::vector<SectionedNode> SectionDetector::classify_all(const Program& prog) {
    std::vector<SectionedNode> result;
    result.reserve(prog.size());
    for (auto& node : prog) {
        if (!node) continue;
        result.push_back({node, classify(*node)});
    }
    return result;
}

SectionDetector::SplitResult SectionDetector::split(const Program& prog) {
    SplitResult res;
    for (auto& node : prog) {
        if (!node) continue;
        switch (classify(*node)) {
            case Section::Logic:     res.logic.push_back(node);     break;
            case Section::UI:        res.ui.push_back(node);        break;
            case Section::Execution: res.execution.push_back(node); break;
        }
    }
    return res;
}

// ── Minimal but compilable re-serialiser ──────────────────────
static std::string indent_str(int n) { return std::string(n * 4, ' '); }
static std::string expr_to_src(const ASTNodePtr& e, int ind = 0);
static std::string node_to_source(const ASTNode& n, int indent = 0);
static std::string block_to_src(const ASTNode& blk, int ind);

static std::string expr_to_src(const ASTNodePtr& e, int ind) {
    if (!e) return "/* ? */";
    switch (e->kind) {
        case NodeKind::IntLit: case NodeKind::FloatLit: case NodeKind::BoolLit:
            return e->value;
        case NodeKind::NullLit:   return "null";
        case NodeKind::StringLit: return "\"" + e->value + "\"";
        case NodeKind::FStringLit:return "f\"" + e->value + "\"";
        case NodeKind::Identifier:return e->value;
        case NodeKind::PathExpr:  return e->value;
        case NodeKind::BinaryOp:
            if (e->children.size() < 2) return e->value;
            return expr_to_src(e->children[0]) + " " + e->value + " " + expr_to_src(e->children[1]);
        case NodeKind::UnaryOp:
            if (e->children.empty()) return e->value;
            return e->value + expr_to_src(e->children[0]);
        case NodeKind::AssignExpr:
            if (e->children.size() < 2) return "/* assign */";
            return expr_to_src(e->children[0]) + " " + e->value + " " + expr_to_src(e->children[1]);
        case NodeKind::CallExpr: {
            if (e->children.empty()) return "()";
            std::ostringstream s;
            s << expr_to_src(e->children[0]) << "(";
            for (size_t i = 1; i < e->children.size(); i++) {
                if (i > 1) s << ", ";
                s << expr_to_src(e->children[i]);
            }
            s << ")";
            return s.str();
        }
        case NodeKind::MemberExpr:
            if (e->children.empty()) return "." + e->value;
            return expr_to_src(e->children[0]) + "." + e->value;
        case NodeKind::IndexExpr:
            if (e->children.size() < 2) return "/* idx */";
            return expr_to_src(e->children[0]) + "[" + expr_to_src(e->children[1]) + "]";
        case NodeKind::PipelineExpr:
            if (e->children.size() < 2) return "/* pipe */";
            return expr_to_src(e->children[0]) + " |> " + expr_to_src(e->children[1]);
        case NodeKind::RangeExpr:
            if (e->children.size() < 2) return "/* range */";
            return expr_to_src(e->children[0]) + ".." + expr_to_src(e->children[1]);
        case NodeKind::LambdaExpr: {
            std::ostringstream s;
            s << "|";
            bool first = true;
            for (size_t i = 0; i + 1 < e->children.size(); i++) {
                auto& p = e->children[i];
                if (!p || p->kind != NodeKind::FieldDecl) continue;
                if (!first) s << ", ";
                s << p->value;
                if (!p->extra.empty()) s << ": " << p->extra;
                first = false;
            }
            s << "| ";
            if (!e->children.empty()) {
                auto& body = e->children.back();
                if (body->kind == NodeKind::Block) s << node_to_source(*body, ind);
                else s << expr_to_src(body);
            }
            return s.str();
        }
        case NodeKind::SpawnExpr: {
            std::ostringstream s;
            s << "spawn " << e->value << " {";
            for (auto& ch : e->children) {
                if (!ch) continue;
                std::string fname = ch->attrs.count("field") ? ch->attrs.at("field") : "_";
                s << "\n" << indent_str(ind+1) << fname << ": " << expr_to_src(ch);
            }
            s << "\n" << indent_str(ind) << "}";
            return s.str();
        }
        case NodeKind::ProcExpr:
            if (e->children.empty()) return "proc \"\"";
            return "proc " + expr_to_src(e->children[0]);
        case NodeKind::EnvExpr: return "env." + e->value;
        case NodeKind::CastExpr:
            if (e->children.empty()) return "/* cast */";
            return "cast(" + expr_to_src(e->children[0]) + ", " + e->extra + ")";
        case NodeKind::AwaitExpr:
            if (e->children.empty()) return "/* await */";
            return "await " + expr_to_src(e->children[0]);
        default:
            return "/* expr:" + std::to_string((int)e->kind) + " */";
    }
}

static std::string node_to_source(const ASTNode& n, int ind) {
    std::string I  = indent_str(ind);
    std::string I1 = indent_str(ind + 1);

    switch (n.kind) {
        case NodeKind::AtomDecl: {
            std::string s = I + "atom " + n.value;
            if (!n.extra.empty()) s += ": " + n.extra;
            if (!n.children.empty()) s += " = " + expr_to_src(n.children[0], ind);
            return s;
        }
        case NodeKind::ShadowDecl: {
            std::string s = I + "shadow " + n.value;
            if (!n.extra.empty()) s += ": " + n.extra;
            if (!n.children.empty()) s += " = " + expr_to_src(n.children[0], ind);
            return s;
        }
        case NodeKind::ConstDecl: {
            std::string s = I + "const " + n.value;
            if (!n.extra.empty()) s += ": " + n.extra;
            if (!n.children.empty()) s += " = " + expr_to_src(n.children[0], ind);
            return s;
        }
        case NodeKind::GlobalDecl: {
            std::string s = I + "global " + n.value;
            if (!n.children.empty()) s += " = " + expr_to_src(n.children[0], ind);
            return s;
        }
        case NodeKind::FluxDecl: {
            std::string s = I + "flux " + n.value;
            if (!n.extra.empty()) s += ": " + n.extra;
            if (!n.children.empty()) s += " = " + expr_to_src(n.children[0], ind);
            return s;
        }
        case NodeKind::LinkStmt:
            return I + "~link \"" + n.value + "\"";

        case NodeKind::ForgeDecl: {
            std::ostringstream s;
            s << I << "forge " << n.value << " {\n";
            for (auto& f : n.children) {
                if (!f || f->kind != NodeKind::FieldDecl) continue;
                s << I1 << f->value;
                if (!f->extra.empty()) s << ": " << f->extra;
                if (!f->children.empty()) s << " = " << expr_to_src(f->children[0], ind+1);
                s << "\n";
            }
            s << I << "}";
            return s.str();
        }
        case NodeKind::RealmDecl: {
            std::ostringstream s;
            s << I << "realm " << n.value << " {\n";
            for (auto& member : n.children) {
                if (!member) continue;
                s << node_to_source(*member, ind + 1) << "\n";
            }
            s << I << "}";
            return s.str();
        }
        case NodeKind::NexusDecl: {
            std::ostringstream s;
            s << I << "nexus " << n.value << " {\n";
            for (auto& m : n.children) {
                if (!m) continue;
                s << I1 << m->value << "(";
                bool first = true;
                for (auto& p : m->children) {
                    if (!p || p->kind != NodeKind::FieldDecl) continue;
                    if (!first) s << ", ";
                    s << p->value;
                    if (!p->extra.empty()) s << ": " << p->extra;
                    first = false;
                }
                s << ")";
                if (!m->extra.empty()) s << " -> " << m->extra;
                s << "\n";
            }
            s << I << "}";
            return s.str();
        }
        case NodeKind::PulseDecl:
        case NodeKind::AsyncPulseDecl: {
            std::ostringstream s;
            if (n.kind == NodeKind::AsyncPulseDecl) s << I << "async ";
            else s << I;
            s << "pulse " << n.value << "(";
            bool first = true;
            for (auto& c : n.children) {
                if (!c || c->kind != NodeKind::FieldDecl) continue;
                if (!first) s << ", ";
                s << c->value;
                if (!c->extra.empty()) s << ": " << c->extra;
                first = false;
            }
            s << ")";
            if (!n.extra2.empty()) s << " -> " << n.extra2;
            for (auto& c : n.children) {
                if (c && c->kind == NodeKind::Block) { s << " " << block_to_src(*c, ind); break; }
            }
            return s.str();
        }
        case NodeKind::ImplDecl: {
            std::ostringstream s;
            s << I << "impl " << n.value << " for " << n.extra << " {\n";
            for (auto& m : n.children) {
                if (!m) continue;
                s << node_to_source(*m, ind + 1) << "\n";
            }
            s << I << "}";
            return s.str();
        }
        case NodeKind::FusionDecl: {
            std::ostringstream s;
            s << I << "fusion " << n.value << " ";
            for (auto& c : n.children)
                if (c && c->kind == NodeKind::Block) { s << block_to_src(*c, ind); break; }
            return s.str();
        }
        case NodeKind::StrandDecl: {
            std::ostringstream s;
            s << I << "strand " << n.value << " ";
            for (auto& c : n.children)
                if (c && c->kind == NodeKind::Block) { s << block_to_src(*c, ind); break; }
            return s.str();
        }
        case NodeKind::IfStmt: {
            if (n.children.size() < 2) return I + "// malformed if";
            std::ostringstream s;
            s << I << "if " << expr_to_src(n.children[0], ind) << " " << block_to_src(*n.children[1], ind);
            for (size_t i = 2; i < n.children.size(); i++) {
                auto& ch = n.children[i];
                if (!ch) continue;
                if (ch->kind == NodeKind::ElifStmt) {
                    s << " elif " << expr_to_src(ch->children[0], ind) << " " << block_to_src(*ch->children[1], ind);
                } else if (ch->kind == NodeKind::ElseStmt) {
                    s << " else " << block_to_src(*ch->children[0], ind);
                }
            }
            return s.str();
        }
        case NodeKind::WhileStmt:
            if (n.children.size() < 2) return I + "// malformed while";
            return I + "while " + expr_to_src(n.children[0], ind) + " " + block_to_src(*n.children[1], ind);
        case NodeKind::ForStmt:
            if (n.children.size() < 2) return I + "// malformed for";
            return I + "for " + n.value + " in " + expr_to_src(n.children[0], ind) + " " + block_to_src(*n.children[1], ind);
        case NodeKind::ReturnStmt:
            if (n.children.empty()) return I + "return";
            return I + "return " + expr_to_src(n.children[0], ind);
        case NodeKind::BreakStmt:    return I + "break";
        case NodeKind::ContinueStmt: return I + "continue";
        case NodeKind::BeamStmt:
            if (n.children.empty()) return I + "beam";
            return I + "beam " + expr_to_src(n.children[0], ind);
        case NodeKind::BypassStmt:
            if (n.children.empty()) return I + "bypass \"\"";
            return I + "bypass " + expr_to_src(n.children[0], ind);
        case NodeKind::ChronosStmt:
            if (n.children.empty()) return I + "chronos 0";
            return I + "chronos " + expr_to_src(n.children[0], ind);
        case NodeKind::QuantumStmt: {
            std::ostringstream s;
            s << I << "quantum ";
            if (!n.children.empty() && n.children[0]->kind == NodeKind::Block)
                s << block_to_src(*n.children[0], ind);
            return s.str();
        }
        case NodeKind::ScanStmt: return I + "scan " + n.value;
        case NodeKind::EmitStmt: {
            std::ostringstream s;
            s << I << "emit \"" << n.value << "\"";
            if (!n.attrs.empty()) {
                s << " {";
                bool first = true;
                for (auto& [k, v] : n.attrs) {
                    if (!first) s << " ";
                    s << k;
                    if (!v.empty()) s << ": " << v;
                    first = false;
                }
                s << "}";
            }
            return s.str();
        }
        case NodeKind::AbsorbStmt: {
            std::ostringstream s;
            s << I << "absorb \"" << n.value << "\" ";
            if (!n.children.empty() && n.children[0]) s << block_to_src(*n.children[0], ind);
            return s.str();
        }
        case NodeKind::ProbeStmt: {
            std::ostringstream s;
            if (!n.children.empty()) s << I << "probe " << expr_to_src(n.children[0]) << " {\n";
            for (size_t i = 1; i < n.children.size(); i++) {
                auto& arm = n.children[i];
                if (!arm || arm->kind != NodeKind::ProbeArm) continue;
                s << I1 << "diverge " << arm->value << " -> ";
                if (!arm->children.empty()) {
                    if (arm->children[0]->kind == NodeKind::Block) s << block_to_src(*arm->children[0], ind + 1);
                    else s << expr_to_src(arm->children[0]);
                }
                s << "\n";
            }
            s << I << "}";
            return s.str();
        }
        case NodeKind::VortexStmt: {
            std::ostringstream s;
            s << I << "vortex ";
            if (!n.children.empty()) s << block_to_src(*n.children[0], ind);
            if (n.children.size() > 1) {
                s << " catch (" << (n.value.empty() ? "e" : n.value) << ") " << block_to_src(*n.children[1], ind);
            }
            return s.str();
        }
        case NodeKind::EnumDecl: {
            std::ostringstream s;
            s << I << "enum " << n.value << " {\n";
            for (auto& v : n.children) {
                if (!v || v->kind != NodeKind::EnumVariant) continue;
                s << I1 << v->value;
                if (!v->children.empty()) {
                    s << "(";
                    bool first = true;
                    for (auto& p : v->children) {
                        if (!p) continue;
                        if (!first) s << ", ";
                        s << p->extra;
                        first = false;
                    }
                    s << ")";
                }
                s << "\n";
            }
            s << I << "}";
            return s.str();
        }
        case NodeKind::ExprStmt:
            if (n.children.empty()) return I + ";";
            return I + expr_to_src(n.children[0], ind);
        case NodeKind::Block:
            return block_to_src(n, ind);
        default:
            return I + "// node:" + std::to_string((int)n.kind);
    }
}

static std::string block_to_src(const ASTNode& blk, int ind) {
    std::ostringstream s;
    s << "{\n";
    for (auto& child : blk.children) {
        if (!child) continue;
        s << node_to_source(*child, ind + 1) << "\n";
    }
    s << indent_str(ind) << "}";
    return s.str();
}

std::string SectionDetector::emit_xh(const Program& nodes, const std::string& orig) {
    std::ostringstream out;
    out << "// X-Phage Logic Layer — generated from: " << orig << "\n";
    out << "// AeonCoreX Lab\n\n";
    for (auto& n : nodes) { if (n) out << node_to_source(*n, 0) << "\n"; }
    return out.str();
}

std::string SectionDetector::emit_xui(const Program& nodes, const std::string& orig) {
    std::ostringstream out;
    out << "// X-Phage UI Layer — generated from: " << orig << "\n";
    out << "// AeonCoreX Lab\n\n";
    for (auto& n : nodes) { if (n) out << node_to_source(*n, 0) << "\n"; }
    return out.str();
}

std::string SectionDetector::emit_xp0(const Program& nodes,
                                       const std::vector<std::string>& links,
                                       const std::string& orig) {
    std::ostringstream out;
    out << "// X-Phage Execution Layer — generated from: " << orig << "\n";
    out << "// AeonCoreX Lab\n\n";
    for (auto& l : links) out << "~link \"" << l << "\"\n";
    if (!links.empty()) out << "\n";
    for (auto& n : nodes) { if (n) out << node_to_source(*n, 0) << "\n"; }
    return out.str();
}

// ── Duplicate declaration detector ────────────────────────────
std::vector<std::string> SectionDetector::check_duplicates(const Program& merged) {
    std::vector<std::string> errors;

    // forge/nexus: ANY duplicate top-level name is a genuine error
    // (struct/abstract-struct redefinition is never valid in C++).
    std::unordered_map<std::string, std::pair<std::string, uint32_t>> type_seen;
    // pulse: only flag if BOTH occurrences have a body. A signature-only
    // declaration (.xh) followed by a full definition (.xp0) is the
    // normal, expected split workflow — not a duplicate.
    std::unordered_map<std::string, std::pair<bool, uint32_t>> pulse_seen;

    for (auto& n : merged) {
        if (!n) continue;

        if (n->kind == NodeKind::ForgeDecl || n->kind == NodeKind::NexusDecl ||
            n->kind == NodeKind::EnumDecl) {
            if (n->value.empty()) continue;
            std::string kind = (n->kind == NodeKind::ForgeDecl) ? "forge"
                              : (n->kind == NodeKind::NexusDecl) ? "nexus"
                              : "enum";
            auto it = type_seen.find(n->value);
            if (it != type_seen.end()) {
                errors.push_back("duplicate " + kind + " '" + n->value +
                    "' — first declared at line " + std::to_string(it->second.second) +
                    ", redefined at line " + std::to_string(n->span.line));
            } else {
                type_seen[n->value] = {kind, n->span.line};
            }
        }

        if (n->kind == NodeKind::PulseDecl || n->kind == NodeKind::AsyncPulseDecl) {
            if (n->value.empty() || n->value == "main") continue;
            bool has_body = false;
            for (auto& c : n->children)
                if (c && c->kind == NodeKind::Block) { has_body = true; break; }

            auto it = pulse_seen.find(n->value);
            if (it != pulse_seen.end()) {
                if (it->second.first && has_body) {
                    errors.push_back("duplicate pulse '" + n->value +
                        "' — both occurrences have a body (first at line " +
                        std::to_string(it->second.second) + ", redefined at line " +
                        std::to_string(n->span.line) + ")");
                }
                // Keep the "has body" state if either occurrence has one,
                // so a third duplicate is still caught correctly.
                it->second.first = it->second.first || has_body;
            } else {
                pulse_seen[n->value] = {has_body, n->span.line};
            }
        }
    }

    return errors;
}

} // namespace xphage::interface
