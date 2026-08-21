#pragma once
// ============================================================
// xphage_parse — Pratt Parser v4.0.0
// Tokens → Full Phase 1-3 AST
// ============================================================
#include "xphage/ast.hpp"
#include <vector>
#include <memory>
#include <string>
#include <functional>

namespace xphage::parse {

struct ParseError {
    std::string message;
    uint32_t    line = 0;
    uint32_t    col  = 0;
};

// Pratt parser precedence levels
enum class Prec {
    None = 0,
    Assign,     // =  +=  -=  *=  /=
    Pipeline,   // |>
    Or,         // ||
    And,        // &&
    Equality,   // == !=
    Compare,    // < > <= >=
    Range,      // ..
    Add,        // + -
    Mul,        // * / %
    Unary,      // ! -  (prefix)
    Call,       // () [] . ? (postfix)
    Primary,
};

class Parser {
public:
    explicit Parser(std::vector<Token> tokens, std::string file = "<input>");

    Program parse();
    const std::vector<ParseError>& errors() const { return errors_; }
    bool has_errors() const { return !errors_.empty(); }

    // Parse a single standalone expression (used by f-string interpolation
    // re-lexing — ensures {expr} inside f"..." goes through the SAME
    // lex→parse→emit pipeline as every other expression in the language,
    // so reserved-keyword sanitization and all other transforms apply).
    ASTNodePtr parse_single_expression() { return parse_expr(); }

private:
    std::vector<Token>      tokens_;
    size_t                  pos_   = 0;
    std::string             file_;
    std::vector<ParseError> errors_;

    // ── Token utilities ──────────────────────────────────────
    const Token& peek(int off = 0) const;
    const Token& advance();
    bool          check(TokenType t) const;
    bool          check_any(std::initializer_list<TokenType>) const;
    bool          match(TokenType t);
    bool          match_any(std::initializer_list<TokenType>);
    const Token& expect(TokenType t, const std::string& msg);
    void          error(const std::string& msg);
    void          sync_to_next_stmt();
    bool          ensure_progress_or_recover(size_t pos_before, const char* context);
    ASTNodePtr    err_node(const std::string& msg);
    ASTNodePtr    span_node(NodeKind k, const std::string& v = "");

    // ── Top-level statements ─────────────────────────────────
    ASTNodePtr parse_stmt();
    ASTNodePtr parse_block();

    // ── Declaration parsers ──────────────────────────────────
    ASTNodePtr parse_pulse_decl(bool is_async = false);
    ASTNodePtr parse_global_decl();
    ASTNodePtr parse_atom_or_shadow();
    ASTNodePtr parse_const_decl();
    ASTNodePtr parse_forge_decl();
    ASTNodePtr parse_realm_decl();
    ASTNodePtr parse_nexus_decl();
    ASTNodePtr parse_flux_decl();
    ASTNodePtr parse_impl_decl();
    ASTNodePtr parse_use_decl();
    ASTNodePtr parse_extern_decl();
    ASTNodePtr parse_one_extern_pulse(const std::string& abi, uint32_t ln);
    ASTNodePtr parse_unsafe_block();
    ASTNodePtr parse_fusion_decl();
    ASTNodePtr parse_strand_decl();
    ASTNodePtr parse_enum_decl();
    // Parses a `<T>` / `<T: Bound>` / `<T, U: Bound>` generic
    // type-parameter list, if one is present at the current position
    // (returns an empty vector if the current token isn't `<` — this
    // is always optional, called speculatively right after a
    // pulse/forge/enum's name). Shared by parse_pulse_decl,
    // parse_forge_decl, and parse_enum_decl so the three declaration
    // kinds that can be generic all use identical syntax/parsing.
    std::vector<ASTNodePtr> parse_type_params();

    // ── Control flow parsers ─────────────────────────────────
    ASTNodePtr parse_if_stmt();
    ASTNodePtr parse_while_stmt();
    ASTNodePtr parse_for_stmt();
    ASTNodePtr parse_return_stmt();
    ASTNodePtr parse_break_stmt();
    ASTNodePtr parse_continue_stmt();
    ASTNodePtr parse_vortex_stmt();

    // ── Action statement parsers ─────────────────────────────
    ASTNodePtr parse_beam_stmt();
    ASTNodePtr parse_bypass_stmt();
    ASTNodePtr parse_quantum_stmt();
    ASTNodePtr parse_scan_stmt();
    ASTNodePtr parse_link_stmt();
    ASTNodePtr parse_chronos_stmt();
    ASTNodePtr parse_ether_stmt();
    ASTNodePtr parse_synapse_stmt();
    ASTNodePtr parse_matrix_stmt();
    ASTNodePtr parse_probe_stmt();
    ASTNodePtr parse_emit_stmt();
    ASTNodePtr parse_absorb_stmt();
    ASTNodePtr parse_yield_stmt();

    // ── Pratt expression parser ───────────────────────────────
    ASTNodePtr parse_expr(Prec min_prec = Prec::None);
    ASTNodePtr parse_prefix();
    ASTNodePtr parse_infix(ASTNodePtr left, Prec min_prec);
    ASTNodePtr parse_primary();
    ASTNodePtr parse_call(ASTNodePtr callee);
    ASTNodePtr parse_index(ASTNodePtr array);
    ASTNodePtr parse_member(ASTNodePtr obj);
    ASTNodePtr parse_lambda();
    ASTNodePtr parse_weave_expr();
    ASTNodePtr parse_spawn_expr();
    ASTNodePtr parse_proc_expr();
    ASTNodePtr parse_env_expr();
    ASTNodePtr parse_glob_expr();

    // ── Type annotation ───────────────────────────────────────
    ASTNodePtr parse_type_annot();    // : TypeName
    std::string parse_type_name();    // TypeName (possibly own/ref/mut_ref T)
    std::vector<ASTNodePtr> parse_param_list(); // (name: type, ...)

    // ── Field / method in forge/nexus ────────────────────────
    ASTNodePtr parse_field_decl();
    ASTNodePtr parse_method_decl();

    // ── Helper: get precedence ────────────────────────────────
    static Prec token_prec(TokenType t);
    static bool is_right_assoc(TokenType t);
};

} // namespace xphage::parse
