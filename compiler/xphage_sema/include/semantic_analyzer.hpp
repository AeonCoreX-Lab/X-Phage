#pragma once
// ============================================================
// X-Phage Semantic Analyzer v1.0.0
//
// A standalone compiler pass, run after parsing and before XIL
// lowering:
//
//   Parser → AST → Semantic Analyzer → Verified AST → XIL Builder → Backend
//
// Responsibility is deliberately narrow and separate from lowering:
// this pass only resolves names, checks types, and validates call
// arity/argument types — it does not build any IR. Keeping these
// concerns apart (rather than checking-while-lowering) is what lets
// Smart Ownership, generics, and Fusion UI-specific checks be added
// later as their own passes over the same Verified AST, without
// entangling them with code generation the way an in-lowering check
// would.
//
// On success, the analyzer returns the same Program it was given
// (the AST itself isn't transformed — "Verified AST" means "AST
// that has passed this pass", not a different data structure). On
// failure, it returns the diagnostics collected, and the caller
// (interface.cpp) should stop before transpilation/lowering, the
// same way a parser error stops before semantic analysis today.
// ============================================================
#include "xphage/ast.hpp"
#include "diagnostic.hpp"
#include "symbol_table.hpp"
#include <vector>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace xphage::sema {

struct AnalysisResult {
    bool                          ok = true;
    std::vector<diag::Diagnostic> diagnostics; // errors AND warnings together;
                                                 // caller filters by severity
};

class SemanticAnalyzer {
public:
    explicit SemanticAnalyzer(const std::string& source_text = "");

    // Runs the full analysis over a top-level program (the merged
    // AST that interface.cpp already produces — after stdlib
    // resolution, after `~link` merging, before transpile/lower).
    AnalysisResult analyze(const Program& prog);

private:
    SymbolTable    symtab_;
    std::vector<diag::Diagnostic> diags_;
    std::string    source_text_; // for source-line snippets in diagnostics;
                                  // empty when analyzing a merged multi-file
                                  // program (snippets are best-effort, not
                                  // load-bearing — the file:line:col is the
                                  // part that must always be correct)

    // ── Pass 1: declaration collection ─────────────────────────
    // Walks top-level decls (forge/nexus/pulse/global/atom/extern)
    // and registers them in the symbol table BEFORE checking any
    // function bodies, so forward references and mutual recursion
    // between top-level functions resolve correctly (matching how
    // every real compiler handles top-level declarations — order
    // in the source doesn't matter for visibility).
    void collect_declarations(const Program& prog);
    void collect_one_declaration(const ASTNodePtr& n);

    // ── Pass 2: body checking ──────────────────────────────────
    void check_program(const Program& prog);
    void check_pulse_body(const ASTNodePtr& pulse_decl);
    void check_stmt(const ASTNodePtr& stmt);
    void check_block(const ASTNodePtr& block);
    // Returns the inferred/declared type string for the expression,
    // or "auto" when it can't be determined (XPhage's lowering
    // already tolerates "auto" throughout, so an inference miss
    // degrades to "don't check this particular subexpression's
    // type" rather than a hard failure).
    std::string check_expr(const ASTNodePtr& expr);

    // ── Specific checks ─────────────────────────────────────────
    void check_identifier(const ASTNodePtr& n);
    void check_call(const ASTNodePtr& n);
    void check_assign(const ASTNodePtr& n);
    void check_binary_op(const ASTNodePtr& n);

    // ── Generics: basic bound validation ────────────────────────
    // type_name implements bound_nexus if an `impl bound_nexus for
    // type_name { ... }` was seen anywhere in the program (recorded
    // in impl_registry_ by a pre-pass — see collect_impls). Returns
    // true (permissively) when bound_nexus is empty (no bound to
    // check) or when type_name itself couldn't be determined (e.g.
    // an argument expression check_expr can't infer a type for) —
    // in both cases there's nothing concrete to validate against, and
    // silently accepting is preferable to a false-positive error on
    // code this pass simply doesn't have enough information about.
    bool satisfies_bound(const std::string& type_name, const std::string& bound_nexus) const;
    // Walks the whole program once, before any call-site checking,
    // recording every `impl Nexus for Type { ... }` into
    // impl_registry_.
    void collect_impls(const ASTNodePtr& n);
    std::unordered_map<std::string, std::unordered_set<std::string>> impl_registry_; // nexus -> {types implementing it}

    // ── Diagnostic helpers ──────────────────────────────────────
    void error(const std::string& code, const Span& span, const std::string& msg,
               uint32_t underline_len = 1,
               std::optional<std::string> help = std::nullopt);
    void warning(const std::string& code, const Span& span, const std::string& msg,
                 uint32_t underline_len = 1,
                 std::optional<std::string> help = std::nullopt);

    // Builtin/runtime function table: names the analyzer should
    // treat as always-resolved, since their real definitions are
    // injected by the transpiler/runtime rather than declared in
    // user XPhage source. Populated from the same name lists the
    // transpiler itself uses (math/string/io/collections runtime,
    // core_ops.cpp), so the two stay in sync by construction rather
    // than by two people remembering to update both places.
    void seed_builtins();
};

} // namespace xphage::sema
