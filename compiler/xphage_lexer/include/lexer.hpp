#pragma once
// ============================================================
// xphage_lexer — Tokenisation module v4.0.0
// ============================================================
#include "xphage/ast.hpp"
#include <string>
#include <vector>
#include <unordered_map>

namespace xphage::lexer {

struct LexError {
    std::string message;
    uint32_t    line = 0;
    uint32_t    col  = 0;
};

class Lexer {
public:
    explicit Lexer(std::string src, std::string file = "<input>");
    std::vector<Token> tokenize();

    const std::vector<LexError>& errors() const { return errors_; }
    bool has_errors() const { return !errors_.empty(); }

private:
    std::string src_;
    std::string file_;
    size_t      pos_  = 0;
    uint32_t    line_ = 1;
    uint32_t    col_  = 1;
    std::vector<LexError> errors_;

    char   peek(int off = 0) const;
    char   advance();
    void   skip_ws_and_comments();
    Token  make(TokenType k, std::string text) const;
    Token  lex_string();
    Token  lex_fstring();
    Token  lex_number();
    Token  lex_word();
    void   error(const std::string& msg, uint32_t line, uint32_t col);
};

} // namespace xphage::lexer
