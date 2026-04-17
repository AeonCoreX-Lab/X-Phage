// ============================================================
// xphage_parse — Parser v3.5.0
// Converts token stream → AST
// ============================================================
#include "../include/parser.hpp"
#include <stdexcept>
#include <iostream>

namespace xphage::parse {

Parser::Parser(std::vector<Token> tokens, std::string file)
    : tokens_(std::move(tokens)), file_(std::move(file)) {
    // Ensure EOF sentinel
    if (tokens_.empty() || tokens_.back().type != END_OF_FILE) {
        Token eof; eof.type = END_OF_FILE; eof.value = "";
        tokens_.push_back(eof);
    }
}

// ── Token utilities ─────────────────────────────────────────
const Token& Parser::peek(int off) const {
    size_t idx = pos_ + off;
    if (idx >= tokens_.size()) return tokens_.back();
    return tokens_[idx];
}

const Token& Parser::advance() {
    const Token& t = tokens_[pos_];
    if (pos_ + 1 < tokens_.size()) pos_++;
    return t;
}

bool Parser::check(TokenType t) const { return peek().type == t; }

bool Parser::match(TokenType t) {
    if (check(t)) { advance(); return true; }
    return false;
}

void Parser::expect(TokenType t, const std::string& msg) {
    if (!match(t)) error(msg + " (got '" + peek().value + "')");
}

void Parser::error(const std::string& msg) {
    errors_.push_back({msg, peek().line, peek().col});
}

void Parser::sync_to_next_stmt() {
    while (!check(END_OF_FILE) &&
           !check(PULSE) && !check(GLOBAL) && !check(ATOM) &&
           !check(SHADOW) && !check(BEAM) && !check(BYPASS) &&
           !check(QUANTUM) && !check(SCAN) && !check(LINK) &&
           !check(FUSION) && !check(R_BRACE)) {
        advance();
    }
}

// ── Top-level parse ──────────────────────────────────────────
Program Parser::parse() {
    Program stmts;
    while (!check(END_OF_FILE)) {
        try {
            auto stmt = parse_stmt();
            if (stmt) stmts.push_back(stmt);
        } catch (...) {
            sync_to_next_stmt();
        }
    }
    return stmts;
}

// ── Statement dispatcher ─────────────────────────────────────
ASTNodePtr Parser::parse_stmt() {
    switch (peek().type) {
        case PULSE:   return parse_pulse_decl();
        case GLOBAL:  return parse_global_decl();
        case ATOM:
        case SHADOW:  return parse_atom_or_shadow();
        case BEAM:    return parse_beam();
        case BYPASS:  return parse_bypass();
        case QUANTUM: return parse_quantum();
        case SCAN:    return parse_scan();
        case LINK:    return parse_link();
        case CHRONOS: return parse_chronos();
        case ETHER:   return parse_ether();
        case VORTEX:  return parse_vortex();
        case VOID:    return parse_void();
        case SYNAPSE: return parse_synapse();
        case MATRIX:  return parse_matrix();
        case FUSION:  return parse_fusion_decl();
        default:
            advance(); // skip unknown token
            return nullptr;
    }
}

// ── Individual parsers ────────────────────────────────────────

// pulse <name> { <body> }
ASTNodePtr Parser::parse_pulse_decl() {
    uint32_t ln = peek().line;
    advance(); // consume 'pulse'
    auto node = std::make_shared<ASTNode>(NodeKind::PulseDecl, "", ln);

    if (check(IDENTIFIER)) {
        node->value = advance().value;
    }
    // optional parameter list: pulse name(a, b)
    if (check(LPAREN)) {
        advance();
        while (!check(RPAREN) && !check(END_OF_FILE)) {
            if (check(IDENTIFIER))
                node->attrs["param_" + std::to_string(node->children.size())]
                    = advance().value;
            match(COMMA);
        }
        expect(RPAREN, "Expected ')'");
    }

    node->children.push_back(parse_block());
    return node;
}

// global <name> = "value"
ASTNodePtr Parser::parse_global_decl() {
    uint32_t ln = peek().line;
    advance(); // consume 'global'
    auto node = std::make_shared<ASTNode>(NodeKind::GlobalDecl, "", ln);
    if (check(IDENTIFIER)) node->value = advance().value;
    expect(EQUAL, "Expected '='");
    node->extra = parse_expr()->value;
    return node;
}

// atom/shadow <name> = <expr>
ASTNodePtr Parser::parse_atom_or_shadow() {
    uint32_t ln  = peek().line;
    bool is_atom = (peek().type == ATOM);
    advance();
    auto node = std::make_shared<ASTNode>(
        is_atom ? NodeKind::AtomDecl : NodeKind::ShadowDecl, "", ln);
    if (check(IDENTIFIER)) node->value = advance().value;
    if (match(EQUAL)) node->children.push_back(parse_expr());
    return node;
}

// beam <expr>
ASTNodePtr Parser::parse_beam() {
    uint32_t ln = peek().line;
    advance();
    auto node = std::make_shared<ASTNode>(NodeKind::BeamStmt, "", ln);
    node->children.push_back(parse_expr());
    return node;
}

// bypass <target> { key: val, ... }
ASTNodePtr Parser::parse_bypass() {
    uint32_t ln = peek().line;
    advance();
    auto node = std::make_shared<ASTNode>(NodeKind::BypassStmt, "", ln);
    if (check(IDENTIFIER)) node->value = advance().value;
    if (check(L_BRACE)) node->children.push_back(parse_config_block());
    return node;
}

// quantum "<task>"
ASTNodePtr Parser::parse_quantum() {
    uint32_t ln = peek().line;
    advance();
    auto node = std::make_shared<ASTNode>(NodeKind::QuantumStmt, "", ln);
    if (check(STRING) || check(IDENTIFIER))
        node->value = advance().value;
    return node;
}

// scan <target> { ... }
ASTNodePtr Parser::parse_scan() {
    uint32_t ln = peek().line;
    advance();
    auto node = std::make_shared<ASTNode>(NodeKind::ScanStmt, "", ln);
    if (check(IDENTIFIER)) node->value = advance().value;
    if (check(L_BRACE)) node->children.push_back(parse_config_block());
    return node;
}

// ~link "module/path"
ASTNodePtr Parser::parse_link() {
    uint32_t ln = peek().line;
    advance();
    auto node = std::make_shared<ASTNode>(NodeKind::LinkStmt, "", ln);
    if (check(STRING) || check(IDENTIFIER))
        node->value = advance().value;
    return node;
}

// chronos "<ms>"
ASTNodePtr Parser::parse_chronos() {
    uint32_t ln = peek().line;
    advance();
    auto node = std::make_shared<ASTNode>(NodeKind::ChronosStmt, "", ln);
    if (check(STRING) || check(NUMBER))
        node->value = advance().value;
    return node;
}

// ether "<target>" <data>
ASTNodePtr Parser::parse_ether() {
    uint32_t ln = peek().line;
    advance();
    auto node = std::make_shared<ASTNode>(NodeKind::EtherStmt, "", ln);
    if (check(STRING) || check(IDENTIFIER)) node->value = advance().value;
    if (check(STRING) || check(IDENTIFIER)) node->extra = advance().value;
    return node;
}

// vortex
ASTNodePtr Parser::parse_vortex() {
    uint32_t ln = peek().line;
    advance();
    return std::make_shared<ASTNode>(NodeKind::VortexStmt, "vortex", ln);
}

// void
ASTNodePtr Parser::parse_void() {
    uint32_t ln = peek().line;
    advance();
    return std::make_shared<ASTNode>(NodeKind::VoidStmt, "void", ln);
}

// synapse "<id>" "<api>"
ASTNodePtr Parser::parse_synapse() {
    uint32_t ln = peek().line;
    advance();
    auto node = std::make_shared<ASTNode>(NodeKind::SynapseStmt, "", ln);
    if (check(STRING) || check(IDENTIFIER)) node->value = advance().value;
    if (check(STRING) || check(IDENTIFIER)) node->extra = advance().value;
    return node;
}

// matrix <name> [<size>]
ASTNodePtr Parser::parse_matrix() {
    uint32_t ln = peek().line;
    advance();
    auto node = std::make_shared<ASTNode>(NodeKind::MatrixStmt, "", ln);
    if (check(IDENTIFIER)) node->value = advance().value;
    if (match(L_BRACKET)) {
        if (check(NUMBER) || check(IDENTIFIER))
            node->extra = advance().value;
        expect(R_BRACKET, "Expected ']'");
    }
    return node;
}

// fusion { ... } / @NeuralComposition(...) { ... }
ASTNodePtr Parser::parse_fusion_decl() {
    uint32_t ln = peek().line;
    advance();
    auto node = std::make_shared<ASTNode>(NodeKind::FusionDecl, "", ln);
    if (check(IDENTIFIER)) node->value = advance().value;
    if (check(L_BRACE))    node->children.push_back(parse_block());
    return node;
}

// { key: val, key: val, ... }
ASTNodePtr Parser::parse_config_block() {
    uint32_t ln = peek().line;
    expect(L_BRACE, "Expected '{'");
    auto block = std::make_shared<ASTNode>(NodeKind::ConfigBlock, "", ln);

    while (!check(R_BRACE) && !check(END_OF_FILE)) {
        auto pair = std::make_shared<ASTNode>(NodeKind::ConfigPair, "", ln);
        if (check(IDENTIFIER) || check(STRING))
            pair->value = advance().value;
        if (match(COLON) || match(EQUAL)) {
            pair->extra = parse_expr()->value;
        }
        match(COMMA);
        block->children.push_back(pair);
    }
    expect(R_BRACE, "Expected '}'");
    return block;
}

// Expression: string | number | identifier
ASTNodePtr Parser::parse_expr() {
    uint32_t ln = peek().line;
    if (check(STRING)) {
        return std::make_shared<ASTNode>(NodeKind::StringLit, advance().value, ln);
    }
    if (check(NUMBER)) {
        return std::make_shared<ASTNode>(NodeKind::NumberLit, advance().value, ln);
    }
    if (check(IDENTIFIER)) {
        auto id = std::make_shared<ASTNode>(NodeKind::Identifier, advance().value, ln);
        // simple binary: id + expr
        if (check(PLUS)) {
            advance();
            auto binop = std::make_shared<ASTNode>(NodeKind::BinaryOp, "+", ln);
            binop->children.push_back(id);
            binop->children.push_back(parse_expr());
            return binop;
        }
        return id;
    }
    // fallback: return empty node
    return std::make_shared<ASTNode>(NodeKind::Identifier, "", ln);
}

// Block: { <stmts>* }
ASTNodePtr Parser::parse_block() {
    uint32_t ln = peek().line;
    expect(L_BRACE, "Expected '{'");
    auto block = std::make_shared<ASTNode>(NodeKind::Block, "", ln);
    while (!check(R_BRACE) && !check(END_OF_FILE)) {
        auto stmt = parse_stmt();
        if (stmt) block->children.push_back(stmt);
    }
    expect(R_BRACE, "Expected '}'");
    return block;
}

} // namespace xphage::parse
