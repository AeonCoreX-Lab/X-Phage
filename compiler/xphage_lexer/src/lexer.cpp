// ============================================================
// X-Phage Lexer v4.0.0
// Complete Phase 1-3 tokenizer with f-strings, all operators
// AeonCoreX Lab
// ============================================================
#include "../include/lexer.hpp"
#include <cctype>
#include <stdexcept>

namespace xphage::lexer {

// ── Keyword table ────────────────────────────────────────────
static const std::unordered_map<std::string, TokenType> KEYWORDS = {
    // Phase 1
    {"pulse",     PULSE},    {"atom",      ATOM},
    {"shadow",    SHADOW},   {"global",    GLOBAL},
    {"return",    RETURN},   {"if",        IF},
    {"elif",      ELIF},     {"else",      ELSE},
    {"while",     WHILE},    {"for",       FOR},
    {"in",        IN},       {"break",     BREAK},
    {"continue",  CONTINUE}, {"beam",      BEAM},
    {"scan",      SCAN},     {"bypass",    BYPASS},
    {"quantum",   QUANTUM},  {"chronos",   CHRONOS},
    {"ether",     ETHER},    {"matrix",    MATRIX},
    {"synapse",   SYNAPSE},  {"void",      VOID_KW},
    {"vortex",    VORTEX},
    {"realm",     REALM},
    // Phase 2
    {"forge",     FORGE},    {"nexus",     NEXUS},
    {"flux",      FLUX},     {"probe",     PROBE},
    {"diverge",   DIVERGE},  {"emit",      EMIT},
    {"absorb",    ABSORB},   {"weave",     WEAVE},
    {"strand",    STRAND},   {"mesh",      MESH},
    {"cast",      CAST},     {"spawn",     SPAWN},
    {"impl",      IMPL},     {"self",      SELF},
    // Phase 3
    {"own",       OWN},      {"ref",       REF},
    {"mut_ref",   MUT_REF},  {"async",     ASYNC},
    {"await",     AWAIT},    {"yield",     YIELD},
    {"use",       USE},      {"pub",       PUB},
    {"priv",      PRIV},     {"const",     CONST},
    {"static",    STATIC},   {"unsafe",    UNSAFE},
    {"extern",    EXTERN},   {"as",        AS},
    {"typeof",    TYPEOF},   {"sizeof",    SIZEOF},
    {"lambda",    LAMBDA},   {"proc",      PROC},
    {"glob",      GLOB},     {"env",       ENV},
    // Phase 6
    {"enum",      ENUM},
    // Types
    {"int",       TYPE_INT}, {"float",     TYPE_FLOAT},
    {"bool",      TYPE_BOOL},{"str",       TYPE_STR},
    {"auto",      TYPE_AUTO},
    // Literals
    {"true",      TRUE},     {"false",     FALSE},
    {"null",      NULL_LIT},
    // UI Components
    {"fusion",    FUSION},   {"Orbit",     ORBIT},
    {"OrbitH",    ORBIT_H},  {"Canvas",    CANVAS},
    {"Scaffold",  SCAFFOLD}, {"Layer",     LAYER},
    {"Vision",    VISION},   {"Signal",    SIGNAL},
    {"Trigger",   TRIGGER},  {"Input",     INPUT_UI},
    {"Toggle",    TOGGLE},   {"Slider",    SLIDER},
    {"Image",     IMAGE},    {"Spacer",    SPACER},
    {"Divider",   DIVIDER},
};

// ── Constructor ──────────────────────────────────────────────
Lexer::Lexer(std::string src, std::string file)
    : src_(std::move(src)), file_(std::move(file)) {}

// ── Helpers ──────────────────────────────────────────────────
char Lexer::peek(int off) const {
    size_t idx = pos_ + off;
    return idx < src_.size() ? src_[idx] : '\0';
}

char Lexer::advance() {
    char c = src_[pos_++];
    if (c == '\n') { line_++; col_ = 1; } else { col_++; }
    return c;
}

Token Lexer::make(TokenType k, std::string text) const {
    Token t;
    t.type  = k;
    t.value = std::move(text);
    t.line  = line_;
    t.col   = col_;
    return t;
}

void Lexer::error(const std::string& msg, uint32_t line, uint32_t col) {
    errors_.push_back({msg, line, col});
}

void Lexer::skip_ws_and_comments() {
    while (pos_ < src_.size()) {
        char c = peek();
        // Whitespace
        if (isspace(c)) { advance(); continue; }
        // Line comment //
        if (c == '/' && peek(1) == '/') {
            while (pos_ < src_.size() && peek() != '\n') advance();
            continue;
        }
        // Block comment /* */
        if (c == '/' && peek(1) == '*') {
            uint32_t ln = line_, cl = col_;
            advance(); advance(); // consume /*
            bool closed = false;
            while (pos_ < src_.size()) {
                if (peek() == '*' && peek(1) == '/') {
                    advance(); advance();
                    closed = true;
                    break;
                }
                advance();
            }
            if (!closed) {
                error("Unterminated block comment (missing closing '*/')", ln, cl);
            }
            continue;
        }
        break;
    }
}

// ── f-string: f"Hello {name}, score: {score}" ──────────────
// Stored as FSTRING with raw interpolated text intact
Token Lexer::lex_fstring() {
    uint32_t ln = line_, cl = col_;
    advance(); // consume opening "
    std::string raw;
    while (pos_ < src_.size() && peek() != '"') {
        if (peek() == '\\' && pos_ + 1 < src_.size()) {
            advance();
            char esc = advance();
            switch(esc) {
                case 'n': raw += '\n'; break;
                case 't': raw += '\t'; break;
                case '"': raw += '"';  break;
                case '\\': raw += '\\'; break;
                case '{': raw += '{'; break;
                default:  raw += '\\'; raw += esc; break;
            }
        } else {
            raw += advance();
        }
    }
    if (pos_ < src_.size()) {
        advance(); // closing "
    } else {
        error("Unterminated f-string literal (missing closing '\"')", ln, cl);
    }
    Token t; t.type = FSTRING; t.value = raw; t.line = ln; t.col = cl;
    return t;
}

// ── String literal ───────────────────────────────────────────
Token Lexer::lex_string() {
    uint32_t ln = line_, cl = col_;
    advance(); // opening "
    std::string s;
    while (pos_ < src_.size() && peek() != '"') {
        if (peek() == '\\' && pos_ + 1 < src_.size()) {
            advance();
            char esc = advance();
            switch(esc) {
                case 'n':  s += '\n'; break;
                case 't':  s += '\t'; break;
                case '"':  s += '"';  break;
                case '\\': s += '\\'; break;
                default:   s += '\\'; s += esc; break;
            }
        } else {
            s += advance();
        }
    }
    if (pos_ < src_.size()) {
        advance(); // closing "
    } else {
        error("Unterminated string literal (missing closing '\"')", ln, cl);
    }
    Token t; t.type = STRING; t.value = s; t.line = ln; t.col = cl;
    return t;
}

// ── Number literal ───────────────────────────────────────────
Token Lexer::lex_number() {
    uint32_t ln = line_, cl = col_;
    std::string num;
    bool is_float = false;

    // Hex: 0x...
    if (peek() == '0' && (peek(1) == 'x' || peek(1) == 'X')) {
        num += advance(); num += advance();
        while (pos_ < src_.size() && isxdigit(peek())) num += advance();
        Token t; t.type = NUMBER_INT; t.value = num; t.line = ln; t.col = cl;
        return t;
    }

    while (pos_ < src_.size() && (isdigit(peek()) || peek() == '.')) {
        if (peek() == '.') {
            if (!isdigit(peek(1))) break; // .. range operator
            is_float = true;
        }
        num += advance();
    }
    // Scientific notation exponent: e/E, optional sign, digits.
    // Must actually be followed by digits (optionally after a sign)
    // to count as an exponent — otherwise a bare trailing 'e' would
    // incorrectly consume what should be a separate identifier
    // token (e.g. "5 e" as two tokens must stay two tokens; only
    // "5e10" or "5e+10" or "5e-10" is a single float literal).
    if (pos_ < src_.size() && (peek() == 'e' || peek() == 'E')) {
        size_t lookahead = 1;
        if (peek(1) == '+' || peek(1) == '-') lookahead = 2;
        if (isdigit(peek(lookahead))) {
            is_float = true;
            num += advance(); // e/E
            if (peek() == '+' || peek() == '-') num += advance();
            while (pos_ < src_.size() && isdigit(peek())) num += advance();
        }
    }
    // Optional suffix: f for float
    if (pos_ < src_.size() && (peek() == 'f' || peek() == 'F')) {
        num += advance(); is_float = true;
    }
    Token t;
    t.type  = is_float ? NUMBER_FLOAT : NUMBER_INT;
    t.value = num;
    t.line  = ln;
    t.col   = cl;
    return t;
}

// ── Word (keyword or identifier) ────────────────────────────
Token Lexer::lex_word() {
    uint32_t ln = line_, cl = col_;
    std::string word;
    while (pos_ < src_.size() &&
           (isalnum(peek()) || peek() == '_')) {
        word += advance();
    }
    Token t;
    auto it = KEYWORDS.find(word);
    t.type  = (it != KEYWORDS.end()) ? it->second : IDENTIFIER;
    t.value = word;
    t.line  = ln;
    t.col   = cl;
    return t;
}

// ── Main tokenize ────────────────────────────────────────────
std::vector<Token> Lexer::tokenize() {
    std::vector<Token> toks;

    while (true) {
        skip_ws_and_comments();
        if (pos_ >= src_.size()) break;

        uint32_t ln = line_, cl = col_;
        char c = peek();

        // ── f-string ───────────────────────────────────────────
        if (c == 'f' && peek(1) == '"') {
            advance(); // consume 'f'
            toks.push_back(lex_fstring());
            continue;
        }

        // ── ~link / ~ bitwise NOT ──────────────────────────────
        if (c == '~') {
            advance();
            std::string w;
            size_t saved_pos = pos_;
            while (pos_ < src_.size() && (isalnum(peek()) || peek() == '_'))
                w += advance();
            if (w == "link") {
                Token t; t.type = LINK; t.value = "~link";
                t.line = ln; t.col = cl;
                toks.push_back(t);
            } else {
                // Not ~link — treat ~ as bitwise NOT, put word back
                pos_ = saved_pos;
                toks.push_back(make(TILDE, "~"));
            }
            continue;
        }

        // ── String ────────────────────────────────────────────
        if (c == '"') { toks.push_back(lex_string()); continue; }

        // ── Number ────────────────────────────────────────────
        if (isdigit(c)) { toks.push_back(lex_number()); continue; }

        // ── Identifier / keyword ─────────────────────────────
        if (isalpha(c) || c == '_') { toks.push_back(lex_word()); continue; }

        // ── Lambda pipe: |x| or | ─────────────────────────────
        // We emit PIPE and let the parser figure out lambda context
        if (c == '|') {
            advance();
            if (peek() == '>') { advance(); toks.push_back(make(PIPE_GT, "|>")); }
            else if (peek() == '|') { advance(); toks.push_back(make(AND_AND, "||")); }
            else { toks.push_back(make(PIPE, "|")); }
            continue;
        }

        // ── Multi-char operators ──────────────────────────────
        advance(); // consume c

        switch (c) {
            case '+': {
                if (peek() == '=') { advance(); toks.push_back(make(PLUS_EQ,  "+=")); }
                else               toks.push_back(make(PLUS,    "+"));
                break;
            }
            case '-': {
                if (peek() == '>') { advance(); toks.push_back(make(ARROW,     "->")); }
                else if (peek() == '=') { advance(); toks.push_back(make(MINUS_EQ, "-=")); }
                else               toks.push_back(make(MINUS,   "-"));
                break;
            }
            case '*': {
                if (peek() == '=') { advance(); toks.push_back(make(STAR_EQ,  "*=")); }
                else               toks.push_back(make(STAR,    "*"));
                break;
            }
            case '/': {
                if (peek() == '=') { advance(); toks.push_back(make(SLASH_EQ, "/=")); }
                else               toks.push_back(make(SLASH,   "/"));
                break;
            }
            case '%': toks.push_back(make(PERCENT, "%")); break;
            case '=': {
                if (peek() == '=') { advance(); toks.push_back(make(EQ_EQ, "==")); }
                else if (peek() == '>') { advance(); toks.push_back(make(FAT_ARROW, "=>")); }
                else               toks.push_back(make(EQUAL,  "="));
                break;
            }
            case '!': {
                if (peek() == '=') { advance(); toks.push_back(make(BANG_EQ, "!=")); }
                else               toks.push_back(make(BANG, "!"));
                break;
            }
            case '<': {
                if (peek() == '=') { advance(); toks.push_back(make(LT_EQ,   "<=")); }
                else if (peek() == '<') { advance(); toks.push_back(make(LSHIFT, "<<")); }
                else               toks.push_back(make(LT,     "<"));
                break;
            }
            case '>': {
                if (peek() == '=') { advance(); toks.push_back(make(GT_EQ,   ">=")); }
                else if (peek() == '>') { advance(); toks.push_back(make(RSHIFT, ">>")); }
                else               toks.push_back(make(GT,     ">"));
                break;
            }
            case '&': {
                if (peek() == '&') { advance(); toks.push_back(make(AND_AND, "&&")); }
                else               toks.push_back(make(AMPERSAND, "&"));
                break;
            }
            case '^': toks.push_back(make(CARET,  "^")); break;
            case '.': {
                if (peek() == '.') { advance(); toks.push_back(make(DOT_DOT, "..")); }
                else               toks.push_back(make(DOT, "."));
                break;
            }
            case ':': {
                if (peek() == ':') { advance(); toks.push_back(make(COLON_COLON, "::")); }
                else               toks.push_back(make(COLON, ":"));
                break;
            }
            case '?': toks.push_back(make(QUESTION,  "?"));   break;
            case '(': toks.push_back(make(LPAREN,    "("));   break;
            case ')': toks.push_back(make(RPAREN,    ")"));   break;
            case '{': toks.push_back(make(L_BRACE,   "{"));   break;
            case '}': toks.push_back(make(R_BRACE,   "}"));   break;
            case '[': toks.push_back(make(L_BRACKET, "["));   break;
            case ']': toks.push_back(make(R_BRACKET, "]"));   break;
            case ',': toks.push_back(make(COMMA,     ","));   break;
            case ';': toks.push_back(make(SEMICOLON, ";"));   break;
            case '@': toks.push_back(make(AT,        "@"));   break;
            default: {
                Token t; t.type = TOK_ERROR;
                t.value = std::string(1, c);
                t.line = ln; t.col = cl;
                error("Unexpected character '" + std::string(1, c) + "'", ln, cl);
                toks.push_back(t);
                break;
            }
        }
    }

    // EOF sentinel
    Token eof; eof.type = END_OF_FILE; eof.value = "";
    eof.line = line_; eof.col = col_;
    toks.push_back(eof);
    return toks;
}

} // namespace xphage::lexer

// ============================================================
// Legacy XPhageLexer interface bridge (used by old runtime)
// ============================================================
#include "xphage/runtime.hpp"

std::vector<Token> XPhageLexer::tokenize(const std::string& source,
                                          const std::string& filename) {
    xphage::lexer::Lexer lex(source, filename);
    return lex.tokenize();
}

