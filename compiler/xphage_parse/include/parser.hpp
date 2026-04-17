#pragma once
// ============================================================
// xphage_parse — Parser v3.5.0
// Tokens → AST
// ============================================================
#include "xphage/ast.hpp"
#include <vector>
#include <memory>
#include <string>

namespace xphage::parse {

struct ParseError {
    std::string message;
    uint32_t    line = 0;
    uint32_t    col  = 0;
};

class Parser {
public:
    explicit Parser(std::vector<Token> tokens, std::string file = "<input>");

    // Returns program AST; errors collected in errors()
    Program parse();
    const std::vector<ParseError>& errors() const { return errors_; }
    bool has_errors() const { return !errors_.empty(); }

private:
    std::vector<Token>       tokens_;
    size_t                   pos_   = 0;
    std::string              file_;
    std::vector<ParseError>  errors_;

    // Token utilities
    const Token& peek(int off = 0) const;
    const Token& advance();
    bool          check(TokenType t) const;
    bool          match(TokenType t);
    void          expect(TokenType t, const std::string& msg);

    // Error recovery
    void error(const std::string& msg);
    void sync_to_next_stmt();

    // Parsers
    ASTNodePtr parse_stmt();
    ASTNodePtr parse_pulse_decl();
    ASTNodePtr parse_global_decl();
    ASTNodePtr parse_atom_or_shadow();
    ASTNodePtr parse_beam();
    ASTNodePtr parse_bypass();
    ASTNodePtr parse_quantum();
    ASTNodePtr parse_scan();
    ASTNodePtr parse_link();
    ASTNodePtr parse_chronos();
    ASTNodePtr parse_ether();
    ASTNodePtr parse_vortex();
    ASTNodePtr parse_void();
    ASTNodePtr parse_synapse();
    ASTNodePtr parse_matrix();
    ASTNodePtr parse_fusion_decl();
    ASTNodePtr parse_config_block();
    ASTNodePtr parse_expr();
    ASTNodePtr parse_block();
};

} // namespace xphage::parse
