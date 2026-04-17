#pragma once
// ============================================================
// xphage_lexer — Tokenisation module v3.5.0
// ============================================================
#include "xphage/ast.hpp"
#include <string>
#include <vector>

namespace xphage::lexer {

// Extended lexer with source location tracking
class Lexer {
public:
    explicit Lexer(std::string src, std::string file = "<input>");
    std::vector<Token> tokenize();

private:
    std::string src_;
    std::string file_;
    size_t      pos_  = 0;
    uint32_t    line_ = 1;
    uint32_t    col_  = 1;

    char   peek(int off = 0) const;
    char   advance();
    void   skip_ws_and_comments();
    Token  make(TokenType k, std::string text) const;
    Token  lex_string();
    Token  lex_number();
    Token  lex_word();
};

} // namespace xphage::lexer
