// ============================================================
// xphage_parse — Pratt Parser v4.0.0
// Phase 1-3 Complete: Control flow, types, events, systems
// AeonCoreX Lab
// ============================================================
#include "../include/parser.hpp"
#include <stdexcept>
#include <iostream>
#include <sstream>

namespace xphage::parse {

// ── Constructor ──────────────────────────────────────────────
Parser::Parser(std::vector<Token> tokens, std::string file)
    : tokens_(std::move(tokens)), file_(std::move(file)) {
    if (tokens_.empty() || tokens_.back().type != END_OF_FILE) {
        Token eof; eof.type = END_OF_FILE; eof.value = "";
        tokens_.push_back(eof);
    }
}

// ── Token utilities ──────────────────────────────────────────
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

bool Parser::check_any(std::initializer_list<TokenType> types) const {
    for (auto t : types) if (check(t)) return true;
    return false;
}

bool Parser::match(TokenType t) {
    if (check(t)) { advance(); return true; }
    return false;
}

bool Parser::match_any(std::initializer_list<TokenType> types) {
    for (auto t : types) if (match(t)) return true;
    return false;
}

const Token& Parser::expect(TokenType t, const std::string& msg) {
    if (!check(t)) {
        error(msg + " (got '" + peek().value + "')");
    }
    return advance();
}

void Parser::error(const std::string& msg) {
    constexpr size_t kMaxErrors = 100;
    if (errors_.size() >= kMaxErrors) return;
    // Suppress an exact duplicate at the same position immediately
    // following the previous error — prevents tight-loop cascades
    // from flooding the output with repeats of the same diagnostic.
    if (!errors_.empty()) {
        const auto& last = errors_.back();
        if (last.line == peek().line && last.col == peek().col && last.message == msg)
            return;
    }
    errors_.push_back({msg, peek().line, peek().col});
    if (errors_.size() == kMaxErrors) {
        errors_.push_back({"Too many errors, stopping further diagnostics", peek().line, peek().col});
    }
}

void Parser::sync_to_next_stmt() {
    while (!check(END_OF_FILE)) {
        if (check_any({PULSE, GLOBAL, ATOM, SHADOW, BEAM, BYPASS,
                       QUANTUM, SCAN, LINK, FUSION, R_BRACE, IF,
                       WHILE, FOR, RETURN, FORGE, NEXUS, FLUX,
                       EMIT, ABSORB, PROBE, IMPL, ASYNC, VORTEX,
                       REALM, CONST, USE, STRAND, MESH, CHRONOS,
                       ETHER, MATRIX, SYNAPSE, BREAK, CONTINUE,
                       YIELD, UNSAFE, EXTERN})) return;
        advance();
    }
}

// Call at the end of any delimiter-separated list-parsing loop body
// (function params, call arguments, struct/spawn fields, lambda
// params, weave modifier args, etc.) right after the element parse
// + optional-delimiter-match. If the cursor hasn't moved since
// `pos_before` — meaning the element parser didn't recognize
// whatever token it started on and consumed nothing, and there was
// no delimiter to match either — the loop would otherwise spin
// forever on the same token. This reports it as an error and forces
// one token of progress, the same recovery strategy used for
// top-level statements (see Program Parser::parse()'s safety net)
// and the same root cause class of bug found there.
//
// Returns true if the caller's enclosing while-loop should stop
// (only relevant at EOF, where forcing "progress" isn't possible
// and the loop's own !check(END_OF_FILE) condition will exit it on
// the next iteration regardless — this return value just avoids an
// extra redundant error from advance() being called at EOF).
bool Parser::ensure_progress_or_recover(size_t pos_before, const char* context) {
    if (pos_ != pos_before) return false; // real progress was made; nothing to do
    error(std::string("Unexpected token in ") + context + ": '" + peek().value + "'");
    if (!check(END_OF_FILE)) advance();
    return check(END_OF_FILE);
}

ASTNodePtr Parser::err_node(const std::string& msg) {
    error(msg);
    return std::make_shared<ASTNode>(NodeKind::NullLit, "", peek().line, peek().col);
}

ASTNodePtr Parser::span_node(NodeKind k, const std::string& v) {
    return std::make_shared<ASTNode>(k, v, peek().line, peek().col);
}

// ── Precedence table ─────────────────────────────────────────
Prec Parser::token_prec(TokenType t) {
    switch (t) {
        case EQUAL: case PLUS_EQ: case MINUS_EQ:
        case STAR_EQ: case SLASH_EQ:   return Prec::Assign;
        case PIPE_GT:                  return Prec::Pipeline;
        case PIPE_PIPE:                return Prec::Or;
        case PIPE:                     return Prec::Or;     // bitwise OR
        case CARET:                    return Prec::Equality; // bitwise XOR
        case AND_AND:                  return Prec::And;
        case AMPERSAND:                return Prec::And;    // bitwise AND
        case LSHIFT: case RSHIFT:      return Prec::Add;    // bit shifts
        case EQ_EQ:   case BANG_EQ:    return Prec::Equality;
        case LT: case GT:
        case LT_EQ:   case GT_EQ:      return Prec::Compare;
        case DOT_DOT:                  return Prec::Range;
        case PLUS:    case MINUS:      return Prec::Add;
        case STAR:    case SLASH:
        case PERCENT:                  return Prec::Mul;
        case LPAREN:  case L_BRACKET:
        case DOT:     case QUESTION:   return Prec::Call;
        default:                       return Prec::None;
    }
}

bool Parser::is_right_assoc(TokenType t) {
    return t == EQUAL || t == PLUS_EQ || t == MINUS_EQ ||
           t == STAR_EQ || t == SLASH_EQ;
}

// ── Top-level parse ───────────────────────────────────────────
// Stamps span.file on every node in a subtree. The parser already
// records line/col accurately at each of its ~66 ASTNode
// construction sites, but threading the filename through every one
// of them individually would be repetitive and easy to miss on
// future additions — instead, since a single Parser instance always
// parses exactly one file, a single post-parse pass stamps the
// (uniform) filename onto every node it produced.
static void stamp_file(const ASTNodePtr& n, const std::string& file) {
    if (!n) return;
    n->span.file = file;
    for (auto& c : n->children) stamp_file(c, file);
}

Program Parser::parse() {
    Program stmts;
    while (!check(END_OF_FILE)) {
        size_t pos_before = pos_;
        try {
            auto stmt = parse_stmt();
            if (stmt) stmts.push_back(stmt);
        } catch (...) {
            sync_to_next_stmt();
        }
        // Safety net: if a statement parse consumed no tokens at all
        // (a parser bug or unhandled token kind), force progress so
        // we never spin forever on malformed input.
        if (pos_ == pos_before && !check(END_OF_FILE)) {
            error("Unable to parse token: '" + peek().value + "'");
            advance();
        }
    }
    for (auto& s : stmts) stamp_file(s, file_);
    return stmts;
}

// ── Statement dispatcher ──────────────────────────────────────
ASTNodePtr Parser::parse_stmt() {
    // Optional semicolons
    while (match(SEMICOLON)) {}
    if (check(END_OF_FILE)) return nullptr;

    switch (peek().type) {
        // Phase 1
        case ASYNC:    { advance(); return parse_pulse_decl(true); }
        case PULSE:    return parse_pulse_decl(false);
        case GLOBAL:   return parse_global_decl();
        case ATOM:     return parse_atom_or_shadow();
        case SHADOW:   return parse_atom_or_shadow();
        case CONST:    return parse_const_decl();
        case IF:       return parse_if_stmt();
        case WHILE:    return parse_while_stmt();
        case FOR:      return parse_for_stmt();
        case RETURN:   return parse_return_stmt();
        case BREAK:    return parse_break_stmt();
        case CONTINUE: return parse_continue_stmt();
        case BEAM:     return parse_beam_stmt();
        case BYPASS:   return parse_bypass_stmt();
        case QUANTUM:  return parse_quantum_stmt();
        case SCAN:     return parse_scan_stmt();
        case LINK:     return parse_link_stmt();
        case CHRONOS:  return parse_chronos_stmt();
        case ETHER:    return parse_ether_stmt();
        case SYNAPSE:  return parse_synapse_stmt();
        case MATRIX:   return parse_matrix_stmt();
        case VORTEX:   return parse_vortex_stmt();
        // Phase 2
        case FORGE:    return parse_forge_decl();
        case REALM:    return parse_realm_decl();
        case NEXUS:    return parse_nexus_decl();
        case FLUX:     return parse_flux_decl();
        case IMPL:     return parse_impl_decl();
        case PROBE:    return parse_probe_stmt();
        case EMIT:     return parse_emit_stmt();
        case ABSORB:   return parse_absorb_stmt();
        case FUSION:   return parse_fusion_decl();
        case STRAND:   return parse_strand_decl();
        // Phase 3
        case USE:      return parse_use_decl();
        case YIELD:    return parse_yield_stmt();
        case EXTERN:   return parse_extern_decl();
        case UNSAFE:   return parse_unsafe_block();
        // Phase 6
        case ENUM:     return parse_enum_decl();
        // Expression statement
        default: {
            auto expr = parse_expr();
            match(SEMICOLON);
            auto node = span_node(NodeKind::ExprStmt);
            node->children.push_back(expr);
            return node;
        }
    }
}

// ── Block: { stmts* } ────────────────────────────────────────
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

// ── pulse [name] [(params)] [-> type] { body } ───────────────
ASTNodePtr Parser::parse_pulse_decl(bool is_async) {
    uint32_t ln = peek().line;
    advance(); // consume 'pulse'
    auto node = std::make_shared<ASTNode>(
        is_async ? NodeKind::AsyncPulseDecl : NodeKind::PulseDecl, "", ln);

    // Name (optional for anonymous pulse)
    if (check(IDENTIFIER)) node->value = advance().value;

    // Generic type parameters: pulse identity<T>(x: T) -> T
    // Stored as leading TypeParamDecl children, before the real
    // parameter FieldDecls — this is safe to distinguish later
    // (monomorphization / semantic analysis) since a TypeParamDecl
    // and a FieldDecl are different NodeKinds, not by position.
    for (auto& tp : parse_type_params()) node->children.push_back(tp);

    // Parameters: (name: type, ...)
    if (check(LPAREN)) {
        advance();
        int pi = 0;
        while (!check(RPAREN) && !check(END_OF_FILE)) {
            size_t pos_before_param = pos_;
            auto param = parse_field_decl();
            if (param) {
                param->attrs["index"] = std::to_string(pi++);
                node->attrs["param_" + std::to_string(pi-1)] = param->value;
                node->attrs["param_type_" + std::to_string(pi-1)] = param->extra;
                node->children.push_back(param);
            }
            match(COMMA);
            if (ensure_progress_or_recover(pos_before_param, "parameter list")) break;
        }
        expect(RPAREN, "Expected ')'");
    }

    // Return type: -> type
    if (match(ARROW)) {
        node->extra2 = parse_type_name();
    }

    // Body
    if (check(L_BRACE)) node->children.push_back(parse_block());
    return node;
}

// ── global name = expr ────────────────────────────────────────
ASTNodePtr Parser::parse_global_decl() {
    uint32_t ln = peek().line;
    advance();
    auto node = std::make_shared<ASTNode>(NodeKind::GlobalDecl, "", ln);
    if (check(IDENTIFIER)) node->value = advance().value;
    expect(EQUAL, "Expected '='");
    node->children.push_back(parse_expr());
    return node;
}

// ── atom/shadow [name] [: type] = expr ───────────────────────
ASTNodePtr Parser::parse_atom_or_shadow() {
    uint32_t ln  = peek().line;
    bool is_atom = (peek().type == ATOM);
    advance();
    // Tuple destructuring: atom (lo, hi) = min_max(numbers) — detected
    // by a '(' where a plain identifier name would otherwise be. Each
    // bound name becomes its own Identifier child; the last child is
    // the initializer expression being destructured. Requires an
    // initializer (there's no meaningful "declare an empty tuple
    // destructure with no value" form), unlike a plain atom/shadow
    // where the initializer is optional.
    if (check(LPAREN)) {
        advance();
        auto node = std::make_shared<ASTNode>(NodeKind::TupleDestructure,
                                               is_atom ? "atom" : "shadow", ln);
        while (!check(RPAREN) && !check(END_OF_FILE)) {
            size_t pos_before = pos_;
            if (check(IDENTIFIER)) {
                node->children.push_back(
                    std::make_shared<ASTNode>(NodeKind::Identifier, advance().value, ln));
            }
            if (!match(COMMA)) break;
            if (ensure_progress_or_recover(pos_before, "tuple destructure pattern")) break;
        }
        expect(RPAREN, "Expected ')' after tuple destructure pattern");
        expect(EQUAL, "Expected '=' after tuple destructure pattern");
        node->children.push_back(parse_expr());
        match(SEMICOLON);
        return node;
    }
    auto node = std::make_shared<ASTNode>(
        is_atom ? NodeKind::AtomDecl : NodeKind::ShadowDecl, "", ln);
    if (check(IDENTIFIER)) node->value = advance().value;
    // Optional type
    if (check(COLON)) {
        advance();
        node->extra = parse_type_name();
    }
    if (match(EQUAL)) {
        node->children.push_back(parse_expr());
    }
    match(SEMICOLON);
    return node;
}

// ── const name: type = expr ──────────────────────────────────
ASTNodePtr Parser::parse_const_decl() {
    uint32_t ln = peek().line;
    advance();
    auto node = std::make_shared<ASTNode>(NodeKind::ConstDecl, "", ln);
    if (check(IDENTIFIER)) node->value = advance().value;
    if (check(COLON)) { advance(); node->extra = parse_type_name(); }
    expect(EQUAL, "Expected '='");
    node->children.push_back(parse_expr());
    match(SEMICOLON);
    return node;
}

// ── if cond { body } [elif cond { body }]* [else { body }] ───
ASTNodePtr Parser::parse_if_stmt() {
    uint32_t ln = peek().line;
    advance(); // 'if'
    auto node = std::make_shared<ASTNode>(NodeKind::IfStmt, "", ln);
    node->children.push_back(parse_expr());    // condition
    node->children.push_back(parse_block());   // then-body

    // elif chains
    while (check(ELIF)) {
        uint32_t eln = peek().line;
        advance();
        auto elif = std::make_shared<ASTNode>(NodeKind::ElifStmt, "", eln);
        elif->children.push_back(parse_expr());
        elif->children.push_back(parse_block());
        node->children.push_back(elif);
    }
    // else
    if (check(ELSE)) {
        uint32_t eln = peek().line;
        advance();
        auto el = std::make_shared<ASTNode>(NodeKind::ElseStmt, "", eln);
        el->children.push_back(parse_block());
        node->children.push_back(el);
    }
    return node;
}

// ── while cond { body } ──────────────────────────────────────
ASTNodePtr Parser::parse_while_stmt() {
    uint32_t ln = peek().line;
    advance();
    auto node = std::make_shared<ASTNode>(NodeKind::WhileStmt, "", ln);
    node->children.push_back(parse_expr());
    node->children.push_back(parse_block());
    return node;
}

// ── for var in expr { body } ─────────────────────────────────
ASTNodePtr Parser::parse_for_stmt() {
    uint32_t ln = peek().line;
    advance();
    auto node = std::make_shared<ASTNode>(NodeKind::ForStmt, "", ln);
    if (check(IDENTIFIER)) node->value = advance().value;
    expect(IN, "Expected 'in'");
    node->children.push_back(parse_expr());    // iterable
    node->children.push_back(parse_block());   // body
    return node;
}

// ── return [expr] ─────────────────────────────────────────────
ASTNodePtr Parser::parse_return_stmt() {
    uint32_t ln = peek().line;
    advance();
    auto node = std::make_shared<ASTNode>(NodeKind::ReturnStmt, "", ln);
    if (!check_any({R_BRACE, SEMICOLON, END_OF_FILE}))
        node->children.push_back(parse_expr());
    match(SEMICOLON);
    return node;
}

ASTNodePtr Parser::parse_break_stmt() {
    uint32_t ln = peek().line; advance();
    match(SEMICOLON);
    return std::make_shared<ASTNode>(NodeKind::BreakStmt, "break", ln);
}

ASTNodePtr Parser::parse_continue_stmt() {
    uint32_t ln = peek().line; advance();
    match(SEMICOLON);
    return std::make_shared<ASTNode>(NodeKind::ContinueStmt, "continue", ln);
}

// ── beam expr ────────────────────────────────────────────────
ASTNodePtr Parser::parse_beam_stmt() {
    uint32_t ln = peek().line; advance();
    auto node = std::make_shared<ASTNode>(NodeKind::BeamStmt, "", ln);
    node->children.push_back(parse_expr());
    match(SEMICOLON);
    return node;
}

// ── bypass "cmd" ─────────────────────────────────────────────
ASTNodePtr Parser::parse_bypass_stmt() {
    uint32_t ln = peek().line; advance();
    auto node = std::make_shared<ASTNode>(NodeKind::BypassStmt, "", ln);
    node->children.push_back(parse_expr());
    match(SEMICOLON);
    return node;
}

// ── quantum { body } ─────────────────────────────────────────
ASTNodePtr Parser::parse_quantum_stmt() {
    uint32_t ln = peek().line; advance();
    auto node = std::make_shared<ASTNode>(NodeKind::QuantumStmt, "", ln);
    if (check(L_BRACE)) node->children.push_back(parse_block());
    else if (check(STRING) || check(IDENTIFIER)) node->value = advance().value;
    match(SEMICOLON);
    return node;
}

// ── scan var ─────────────────────────────────────────────────
ASTNodePtr Parser::parse_scan_stmt() {
    uint32_t ln = peek().line; advance();
    auto node = std::make_shared<ASTNode>(NodeKind::ScanStmt, "", ln);
    if (check(IDENTIFIER)) node->value = advance().value;
    match(SEMICOLON);
    return node;
}

// ── ~link "module" ───────────────────────────────────────────
ASTNodePtr Parser::parse_link_stmt() {
    uint32_t ln = peek().line; advance();
    auto node = std::make_shared<ASTNode>(NodeKind::LinkStmt, "", ln);
    if (check(STRING)) {
        node->value = advance().value;
    } else if (check(IDENTIFIER)) {
        // Bare module path: identifier(/identifier)*  e.g. core/types, net/http
        std::string path = advance().value;
        while (check(SLASH) && peek(1).type == IDENTIFIER) {
            advance(); // consume '/'
            path += "/" + advance().value;
        }
        node->value = path;
    }
    match(SEMICOLON);
    return node;
}

// ── chronos ms ───────────────────────────────────────────────
ASTNodePtr Parser::parse_chronos_stmt() {
    uint32_t ln = peek().line; advance();
    auto node = std::make_shared<ASTNode>(NodeKind::ChronosStmt, "", ln);
    node->children.push_back(parse_expr());
    match(SEMICOLON);
    return node;
}

// ── ether url data ───────────────────────────────────────────
ASTNodePtr Parser::parse_ether_stmt() {
    uint32_t ln = peek().line; advance();
    auto node = std::make_shared<ASTNode>(NodeKind::EtherStmt, "", ln);
    node->children.push_back(parse_expr());
    if (!check_any({SEMICOLON, R_BRACE, END_OF_FILE}))
        node->children.push_back(parse_expr());
    match(SEMICOLON);
    return node;
}

// ── synapse id api ───────────────────────────────────────────
ASTNodePtr Parser::parse_synapse_stmt() {
    uint32_t ln = peek().line; advance();
    auto node = std::make_shared<ASTNode>(NodeKind::SynapseStmt, "", ln);
    if (check(IDENTIFIER) || check(STRING)) node->value  = advance().value;
    if (check(IDENTIFIER) || check(STRING)) node->extra  = advance().value;
    match(SEMICOLON);
    return node;
}

// ── matrix name[size] ────────────────────────────────────────
ASTNodePtr Parser::parse_matrix_stmt() {
    uint32_t ln = peek().line; advance();
    auto node = std::make_shared<ASTNode>(NodeKind::MatrixStmt, "", ln);
    if (check(IDENTIFIER)) node->value = advance().value;
    if (match(L_BRACKET)) {
        node->children.push_back(parse_expr());
        expect(R_BRACKET, "Expected ']'");
    }
    match(SEMICOLON);
    return node;
}

// ── vortex { body } [catch (e) { body }] ─────────────────────
ASTNodePtr Parser::parse_vortex_stmt() {
    uint32_t ln = peek().line; advance();
    auto node = std::make_shared<ASTNode>(NodeKind::VortexStmt, "", ln);
    node->children.push_back(parse_block()); // try body
    // Optional catch block
    if (check(IDENTIFIER) && peek().value == "catch") {
        advance();
        if (match(LPAREN)) {
            if (check(IDENTIFIER)) node->value = advance().value; // error var
            expect(RPAREN, "Expected ')'");
        }
        node->children.push_back(parse_block()); // catch body
    }
    return node;
}

// ── forge Name { fields } ────────────────────────────────────
ASTNodePtr Parser::parse_forge_decl() {
    uint32_t ln = peek().line; advance();
    auto node = std::make_shared<ASTNode>(NodeKind::ForgeDecl, "", ln);
    if (check(IDENTIFIER)) node->value = advance().value;
    // Generic type parameters: forge Box<T> { value: T }
    for (auto& tp : parse_type_params()) node->children.push_back(tp);
    expect(L_BRACE, "Expected '{'");
    while (!check(R_BRACE) && !check(END_OF_FILE)) {
        auto field = parse_field_decl();
        if (field) node->children.push_back(field);
        match(SEMICOLON);
    }
    expect(R_BRACE, "Expected '}'");
    return node;
}

// ── enum Name { Variant  Variant(Type, Type)  ... } ───────────
// A simple variant (no payload) is just a bare identifier line;
// a data-carrying variant looks like a tuple-style call:
// `Error(str)`, `NotFound(int)`. Each variant becomes an
// EnumVariant child node: .value = variant name, .children = one
// TypeAnnot-shaped node per payload slot (.extra = the type name,
// reusing parse_type_name() so payload types can be anything a
// normal type annotation can be, including realm-qualified types).
// ── <T> / <T: Bound> / <T, U: Bound> generic type-parameter list ──
// Called speculatively right after a pulse/forge/enum's name; a
// position where '<' can only mean "a type-parameter list starts
// here" (never a less-than comparison), so no lookahead/backtracking
// is needed the way an expression-context '<' would require.
std::vector<ASTNodePtr> Parser::parse_type_params() {
    std::vector<ASTNodePtr> params;
    if (!check(LT)) return params;
    advance();
    while (!check(GT) && !check(END_OF_FILE)) {
        size_t pos_before = pos_;
        if (check(IDENTIFIER)) {
            uint32_t ln = peek().line;
            auto p = std::make_shared<ASTNode>(NodeKind::TypeParamDecl, "", ln);
            p->value = advance().value;
            if (check(COLON)) {
                advance();
                if (check(IDENTIFIER)) p->extra = advance().value;
            }
            params.push_back(p);
        } else {
            advance();
        }
        match(COMMA);
        if (ensure_progress_or_recover(pos_before, "type parameter list")) break;
    }
    expect(GT, "Expected '>' after type parameter list");
    return params;
}

ASTNodePtr Parser::parse_enum_decl() {
    uint32_t ln = peek().line; advance(); // consume 'enum'
    auto node = std::make_shared<ASTNode>(NodeKind::EnumDecl, "", ln);
    if (check(IDENTIFIER)) node->value = advance().value;
    // Generic type parameters: enum Option<T> { Some(T)  None }
    for (auto& tp : parse_type_params()) node->children.push_back(tp);
    expect(L_BRACE, "Expected '{'");
    while (!check(R_BRACE) && !check(END_OF_FILE)) {
        size_t pos_before_variant = pos_;
        if (!check(IDENTIFIER)) { advance(); continue; }
        uint32_t vln = peek().line;
        auto variant = std::make_shared<ASTNode>(NodeKind::EnumVariant, "", vln);
        variant->value = advance().value;
        if (check(LPAREN)) {
            advance();
            while (!check(RPAREN) && !check(END_OF_FILE)) {
                auto payload_type = std::make_shared<ASTNode>(NodeKind::TypeAnnot, "", peek().line);
                payload_type->extra = parse_type_name();
                variant->children.push_back(payload_type);
                if (!match(COMMA)) break;
            }
            expect(RPAREN, "Expected ')' after enum variant payload types");
        }
        node->children.push_back(variant);
        match(COMMA);
        if (ensure_progress_or_recover(pos_before_variant, "enum variant list")) break;
    }
    expect(R_BRACE, "Expected '}'");
    return node;
}

// ── realm Name { decl* } → C++ namespace ──────────────────────
// A realm groups forge/nexus/const/pulse/flux/impl/nested realm
// declarations under a named scope, accessed via Name::member
ASTNodePtr Parser::parse_realm_decl() {
    uint32_t ln = peek().line; advance(); // consume 'realm'
    auto node = std::make_shared<ASTNode>(NodeKind::RealmDecl, "", ln);
    if (check(IDENTIFIER)) node->value = advance().value;
    expect(L_BRACE, "Expected '{'");
    while (!check(R_BRACE) && !check(END_OF_FILE)) {
        auto stmt = parse_stmt();
        if (stmt) node->children.push_back(stmt);
    }
    expect(R_BRACE, "Expected '}'");
    return node;
}

// ── nexus Name { methods } ───────────────────────────────────
ASTNodePtr Parser::parse_nexus_decl() {
    uint32_t ln = peek().line; advance();
    auto node = std::make_shared<ASTNode>(NodeKind::NexusDecl, "", ln);
    if (check(IDENTIFIER)) node->value = advance().value;
    expect(L_BRACE, "Expected '{'");
    while (!check(R_BRACE) && !check(END_OF_FILE)) {
        auto method = parse_method_decl();
        if (method) node->children.push_back(method);
        match(SEMICOLON);
    }
    expect(R_BRACE, "Expected '}'");
    return node;
}

// ── flux name: type = expr ───────────────────────────────────
ASTNodePtr Parser::parse_flux_decl() {
    uint32_t ln = peek().line; advance();
    auto node = std::make_shared<ASTNode>(NodeKind::FluxDecl, "", ln);
    if (check(IDENTIFIER)) node->value = advance().value;
    if (check(COLON)) { advance(); node->extra = parse_type_name(); }
    if (match(EQUAL)) node->children.push_back(parse_expr());
    match(SEMICOLON);
    return node;
}

// ── impl Nexus for Forge { methods } ─────────────────────────
ASTNodePtr Parser::parse_impl_decl() {
    uint32_t ln = peek().line; advance();
    auto node = std::make_shared<ASTNode>(NodeKind::ImplDecl, "", ln);
    // Generic impl: impl<T> Box<T> { ... }. The type-parameter list,
    // if present, comes immediately after `impl` — stored as leading
    // TypeParamDecl children (same convention pulse/forge/enum use),
    // consumed by monomorphize.cpp the same way a generic forge's own
    // TypeParamDecl children are.
    for (auto& tp : parse_type_params()) node->children.push_back(tp);
    // First identifier: ambiguous until we see whether `for` follows.
    // `impl Nexus for Type { }` — this identifier is the nexus name.
    // `impl Type { }` (inherent impl, no nexus) — this identifier IS
    // the type name. 'for' lexes as the dedicated FOR token (the same
    // one used by for-loops), never as a plain IDENTIFIER, so the
    // previous check here (`check(IDENTIFIER) && peek().value ==
    // "for"`) could never be true — every `impl Nexus for Type`
    // parsed as if "Nexus" were the type name instead (node->extra
    // left empty, "for" then rejected as an unexpected token), and
    // conversely the inherent-impl form `impl Type { }` left the type
    // name sitting in node->value with node->extra empty — neither
    // form's method bodies were ever reachable by anything looking
    // for the type name in node->extra (semantic analysis's
    // check_program, or codegen's impl_methods_ registry) the way
    // both AST transpiler and IR lowering already expect.
    std::string first_ident;
    if (check(IDENTIFIER)) first_ident = advance().value;
    // A generic receiver type repeats its type parameter(s) in angle
    // brackets here: `impl<T> Box<T> { ... }` — first_ident is "Box",
    // and this consumes the trailing "<T>" to produce the full
    // "Box<T>" flat-string type name monomorphize.cpp already expects
    // from every other type-name-bearing field (matches
    // parse_spawn_expr's identical generic-argument suffix parsing).
    auto consume_generic_suffix = [&](std::string base) -> std::string {
        if (check(LT)) {
            advance();
            base += "<" + parse_type_name();
            while (match(COMMA)) base += ", " + parse_type_name();
            expect(GT, "Expected '>' after generic type argument(s)");
            base += ">";
        }
        return base;
    };
    if (check(FOR)) {
        advance();
        node->value = first_ident; // nexus name
        if (check(IDENTIFIER)) {
            node->extra = consume_generic_suffix(advance().value); // type name
        }
    } else {
        node->extra = consume_generic_suffix(first_ident); // inherent impl: first_ident IS the type name
    }
    expect(L_BRACE, "Expected '{'");
    while (!check(R_BRACE) && !check(END_OF_FILE)) {
        // Method bodies inside impl are written WITHOUT the `pulse`
        // keyword in every documented example (see the language
        // book's Chapter 8 impl examples — `area() -> float { ... }`,
        // never `pulse area() -> float { ... }`). parse_method_decl
        // already handles exactly this (falling back to
        // parse_pulse_decl only if it happens to see a `pulse`
        // keyword) — calling parse_pulse_decl(false) directly here
        // (the previous version of this code) unconditionally
        // consumed the current token assuming it was `pulse`, which
        // meant it silently ate each method's own name instead: not
        // a single impl method body in the language was ever parsed
        // with its real name intact.
        auto method = parse_method_decl();
        if (method) node->children.push_back(method);
    }
    expect(R_BRACE, "Expected '}'");
    return node;
}

// ── use module::name ─────────────────────────────────────────
ASTNodePtr Parser::parse_use_decl() {
    uint32_t ln = peek().line; advance();
    auto node = std::make_shared<ASTNode>(NodeKind::UseDecl, "", ln);
    std::string path;
    while (check(IDENTIFIER) || check(COLON_COLON) || check(STAR)) {
        path += advance().value;
    }
    node->value = path;
    match(SEMICOLON);
    return node;
}

// ── extern "C" pulse name(params) -> ret ; ───────────────────
// FFI declaration: no body, just a signature that the linker
// resolves against an externally-supplied symbol (a C library,
// a Rust cdylib exporting #[no_mangle] extern "C" functions,
// or any other System V / Win64 ABI compatible object).
ASTNodePtr Parser::parse_extern_decl() {
    uint32_t ln = peek().line;
    advance(); // consume 'extern'

    std::string abi = "C";
    if (check(STRING)) abi = advance().value;

    // extern "C" { pulse a(...) -> t  pulse b(...) -> t ... }
    // is also accepted as a block form, producing multiple
    // ExternDecl siblings wrapped in a Block for convenience.
    if (check(L_BRACE)) {
        advance();
        auto block = std::make_shared<ASTNode>(NodeKind::Block, "", ln);
        while (!check(R_BRACE) && !check(END_OF_FILE)) {
            if (!check(PULSE)) { error("Expected 'pulse' inside extern block"); advance(); continue; }
            auto decl = parse_one_extern_pulse(abi, peek().line);
            block->children.push_back(decl);
        }
        expect(R_BRACE, "Expected '}'");
        return block;
    }

    if (!check(PULSE)) {
        error("Expected 'pulse' after extern \"" + abi + "\"");
        return std::make_shared<ASTNode>(NodeKind::ExternDecl, "", ln);
    }
    return parse_one_extern_pulse(abi, ln);
}

ASTNodePtr Parser::parse_one_extern_pulse(const std::string& abi, uint32_t ln) {
    advance(); // consume 'pulse'
    auto node = std::make_shared<ASTNode>(NodeKind::ExternDecl, "", ln);
    node->extra = abi; // ABI string, e.g. "C"

    if (check(IDENTIFIER)) node->value = advance().value;

    if (check(LPAREN)) {
        advance();
        int pi = 0;
        while (!check(RPAREN) && !check(END_OF_FILE)) {
            size_t pos_before_param = pos_;
            auto param = parse_field_decl();
            if (param) {
                param->attrs["index"] = std::to_string(pi++);
                node->attrs["param_" + std::to_string(pi-1)] = param->value;
                node->attrs["param_type_" + std::to_string(pi-1)] = param->extra;
                node->children.push_back(param);
            }
            match(COMMA);
            if (ensure_progress_or_recover(pos_before_param, "extern parameter list")) break;
        }
        expect(RPAREN, "Expected ')'");
    }

    if (match(ARROW)) {
        node->extra2 = parse_type_name();
    } else {
        node->extra2 = "void";
    }

    match(SEMICOLON);
    // extern declarations never have a body — if one is present
    // by mistake, that's a usage error the user should be told
    // about rather than silently ignored.
    if (check(L_BRACE)) {
        error("extern \"" + abi + "\" function declarations cannot have a body");
        parse_block(); // consume and discard to recover cleanly
    }
    return node;
}

// ── unsafe { body } ────────────────────────────────────────────
// Transparent to codegen: the block's statements are emitted
// as-is. This exists so raw-pointer / FFI code can be visually
// and semantically marked as a deliberate trust boundary, the
// same way Rust's `unsafe` works.
ASTNodePtr Parser::parse_unsafe_block() {
    uint32_t ln = peek().line;
    advance(); // consume 'unsafe'
    auto node = std::make_shared<ASTNode>(NodeKind::UnsafeBlock, "", ln);
    node->children.push_back(parse_block());
    return node;
}

// ── probe expr { arms } ──────────────────────────────────────
ASTNodePtr Parser::parse_probe_stmt() {
    uint32_t ln = peek().line; advance();
    auto node = std::make_shared<ASTNode>(NodeKind::ProbeStmt, "", ln);
    node->children.push_back(parse_expr()); // subject
    expect(L_BRACE, "Expected '{'");
    while (!check(R_BRACE) && !check(END_OF_FILE)) {
        if (!check(DIVERGE)) { advance(); continue; }
        uint32_t aln = peek().line; advance();
        auto arm = std::make_shared<ASTNode>(NodeKind::ProbeArm, "", aln);
        // Pattern: string, number, identifier/wildcard, or an
        // enum-qualified variant pattern (EnumName.Variant, optionally
        // EnumName.Variant(binding1, binding2, ...) to destructure a
        // data-carrying variant's payload into named bindings).
        //
        // The enum-qualified form previously didn't exist at all —
        // only a single bare token was accepted here, so
        // `diverge Status.Error(msg) -> ...` (documented in the
        // language book's enum chapter) could never actually be
        // written; the parser would consume "Status" as the whole
        // pattern and then fail expecting '->' where '.' appeared.
        if (check(IDENTIFIER)) {
            std::string pat = advance().value;
            if (check(DOT)) {
                advance();
                if (check(IDENTIFIER)) pat += "." + advance().value;
                if (check(LPAREN)) {
                    advance();
                    std::vector<std::string> bindings;
                    while (!check(RPAREN) && !check(END_OF_FILE)) {
                        if (check(IDENTIFIER)) bindings.push_back(advance().value);
                        if (!match(COMMA)) break;
                    }
                    expect(RPAREN, "Expected ')' after enum variant pattern bindings");
                    std::string joined;
                    for (size_t i = 0; i < bindings.size(); i++) {
                        if (i) joined += ",";
                        joined += bindings[i];
                    }
                    arm->attrs["bindings"] = joined;
                }
            }
            arm->value = pat;
        } else if (check_any({STRING, NUMBER_INT, NUMBER_FLOAT})) {
            arm->value = advance().value;
        }
        expect(ARROW, "Expected '->'");
        if (check(L_BRACE)) {
            arm->children.push_back(parse_block());
        } else if (check_any({BEAM, BYPASS, SCAN, CHRONOS, RETURN,
                               BREAK, CONTINUE, EMIT, QUANTUM, YIELD})) {
            // Statement keyword — parse it as a block of one
            auto blk = std::make_shared<ASTNode>(NodeKind::Block, "", peek().line);
            blk->children.push_back(parse_stmt());
            arm->children.push_back(blk);
        } else {
            arm->children.push_back(parse_expr());
        }
        node->children.push_back(arm);
        match(SEMICOLON);
    }
    expect(R_BRACE, "Expected '}'");
    return node;
}

// ── emit "event" { data_key: data_val } ──────────────────────
ASTNodePtr Parser::parse_emit_stmt() {
    uint32_t ln = peek().line; advance();
    auto node = std::make_shared<ASTNode>(NodeKind::EmitStmt, "", ln);
    if (check(STRING)) node->value = advance().value;
    if (check(L_BRACE)) {
        advance();
        while (!check(R_BRACE) && !check(END_OF_FILE)) {
            size_t pos_before_field = pos_;
            std::string k, v;
            if (check(IDENTIFIER) || check(STRING)) k = advance().value;
            if (match(COLON) && (check(STRING) || check(IDENTIFIER) || check(NUMBER_INT)))
                v = advance().value;
            node->attrs[k] = v;
            match(COMMA);
            if (ensure_progress_or_recover(pos_before_field, "emit payload")) break;
        }
        expect(R_BRACE, "Expected '}'");
    }
    match(SEMICOLON);
    return node;
}

// ── absorb "event" { body } ──────────────────────────────────
ASTNodePtr Parser::parse_absorb_stmt() {
    uint32_t ln = peek().line; advance();
    auto node = std::make_shared<ASTNode>(NodeKind::AbsorbStmt, "", ln);
    if (check(STRING)) node->value = advance().value;
    if (check(L_BRACE)) node->children.push_back(parse_block());
    match(SEMICOLON);
    return node;
}

// ── yield expr ───────────────────────────────────────────────
ASTNodePtr Parser::parse_yield_stmt() {
    uint32_t ln = peek().line; advance();
    auto node = std::make_shared<ASTNode>(NodeKind::YieldStmt, "", ln);
    if (!check_any({SEMICOLON, R_BRACE, END_OF_FILE}))
        node->children.push_back(parse_expr());
    match(SEMICOLON);
    return node;
}

// ── fusion Name { body } ─────────────────────────────────────
ASTNodePtr Parser::parse_fusion_decl() {
    uint32_t ln = peek().line; advance();
    auto node = std::make_shared<ASTNode>(NodeKind::FusionDecl, "", ln);
    if (check(IDENTIFIER)) node->value = advance().value;
    if (check(L_BRACE))    node->children.push_back(parse_block());
    return node;
}

// ── strand name { animations } ───────────────────────────────
ASTNodePtr Parser::parse_strand_decl() {
    uint32_t ln = peek().line; advance();
    auto node = std::make_shared<ASTNode>(NodeKind::StrandDecl, "", ln);
    if (check(IDENTIFIER)) node->value = advance().value;
    if (check(L_BRACE))    node->children.push_back(parse_block());
    return node;
}

// ══════════════════════════════════════════════════════════════
// PRATT EXPRESSION PARSER
// Precedence climbing: assign → pipeline → or → and → eq →
//   cmp → range → add → mul → unary → call/postfix → primary
// ══════════════════════════════════════════════════════════════

ASTNodePtr Parser::parse_expr(Prec min_prec) {
    auto left = parse_prefix();

    while (true) {
        Prec prec = token_prec(peek().type);
        if (prec == Prec::None) break;
        if (!is_right_assoc(peek().type) && prec <= min_prec) break;
        if ( is_right_assoc(peek().type) && prec <  min_prec) break;
        // For right-assoc operators pass same prec (allow chain); else prec (next level up)
        left = parse_infix(left, is_right_assoc(peek().type) ?
            static_cast<Prec>(static_cast<int>(prec) - 1) : prec);
    }
    return left;
}

ASTNodePtr Parser::parse_prefix() {
    // Unary operators
    if (check(BANG)) {
        advance();
        auto n = span_node(NodeKind::UnaryOp, "!");
        n->children.push_back(parse_expr(Prec::Unary));
        return n;
    }
    if (check(MINUS)) {
        advance();
        auto n = span_node(NodeKind::UnaryOp, "-");
        n->children.push_back(parse_expr(Prec::Unary));
        return n;
    }
    if (check(TILDE)) {        // bitwise NOT
        advance();
        auto n = span_node(NodeKind::UnaryOp, "~");
        n->children.push_back(parse_expr(Prec::Unary));
        return n;
    }

    // Lambda: |params| body  or  |x: int| expr
    if (check(PIPE)) return parse_lambda();

    return parse_primary();
}

ASTNodePtr Parser::parse_infix(ASTNodePtr left, Prec min_prec) {
    TokenType op = peek().type;

    // Assignment operators (right-assoc)
    if (op == EQUAL || op == PLUS_EQ || op == MINUS_EQ ||
        op == STAR_EQ || op == SLASH_EQ) {
        std::string opStr = advance().value;
        auto node = span_node(NodeKind::AssignExpr, opStr);
        node->children.push_back(left);
        node->children.push_back(parse_expr(min_prec));
        return node;
    }

    // Pipeline: expr |> func
    if (op == PIPE_GT) {
        advance();
        auto node = span_node(NodeKind::PipelineExpr, "|>");
        node->children.push_back(left);
        node->children.push_back(parse_expr(Prec::Pipeline));
        return node;
    }

    // Range: start .. end
    if (op == DOT_DOT) {
        advance();
        auto node = span_node(NodeKind::RangeExpr, "..");
        node->children.push_back(left);
        node->children.push_back(parse_expr(Prec::Range));
        return node;
    }

    // Call: expr(args)
    if (op == LPAREN) return parse_call(left);

    // Index: expr[idx]
    if (op == L_BRACKET) return parse_index(left);

    // Member: expr.field
    if (op == DOT) return parse_member(left);

    // Error propagation: expr?
    if (op == QUESTION) {
        advance();
        auto node = span_node(NodeKind::PropagateExpr, "?");
        node->children.push_back(left);
        return node;
    }

    // Binary operators
    std::string opStr = advance().value;
    auto node = span_node(NodeKind::BinaryOp, opStr);
    node->children.push_back(left);
    node->children.push_back(parse_expr(
        static_cast<Prec>(static_cast<int>(token_prec(op)))));
    return node;
}

// ── Primary expressions ───────────────────────────────────────
ASTNodePtr Parser::parse_primary() {
    uint32_t ln = peek().line, cl = peek().col;

    // Literals
    if (check(NUMBER_INT)) {
        auto n = std::make_shared<ASTNode>(NodeKind::IntLit, advance().value, ln, cl);
        return n;
    }
    if (check(NUMBER_FLOAT)) {
        auto n = std::make_shared<ASTNode>(NodeKind::FloatLit, advance().value, ln, cl);
        return n;
    }
    if (check(STRING)) {
        auto n = std::make_shared<ASTNode>(NodeKind::StringLit, advance().value, ln, cl);
        return n;
    }
    if (check(FSTRING)) {
        auto n = std::make_shared<ASTNode>(NodeKind::FStringLit, advance().value, ln, cl);
        return n;
    }
    if (check(TRUE)) {
        advance();
        return std::make_shared<ASTNode>(NodeKind::BoolLit, "true", ln, cl);
    }
    if (check(FALSE)) {
        advance();
        return std::make_shared<ASTNode>(NodeKind::BoolLit, "false", ln, cl);
    }
    // `self` inside an expression (e.g. `self.value`, `return self`)
    // behaves exactly like any other identifier once past the
    // parameter-declaration position (see parse_field_decl's SELF
    // handling for that position) — there was previously no case for
    // it here at all, so any method body actually referencing self
    // (as opposed to just declaring it as a parameter) failed to
    // parse with "Unexpected token: 'self'".
    if (check(SELF)) {
        advance();
        return std::make_shared<ASTNode>(NodeKind::Identifier, "self", ln, cl);
    }
    if (check(NULL_LIT)) {
        advance();
        return std::make_shared<ASTNode>(NodeKind::NullLit, "null", ln, cl);
    }

    // Parenthesized expression, or a tuple literal: (e0, e1, ...).
    // Disambiguated by whether a comma follows the first inner
    // expression — (expr) is still plain grouping (returns the inner
    // expr directly, unwrapped, exactly as before), while (e0, e1)
    // becomes a real TupleExpr. A trailing comma before the closing
    // paren, (e0,), is accepted as a one-element tuple (mirrors how
    // Python/Rust both require the trailing comma specifically to
    // distinguish a one-element tuple from plain grouping) rather
    // than silently falling back to grouping's "unwrap the single
    // inner expr" behavior for that one case.
    if (check(LPAREN)) {
        advance();
        if (check(RPAREN)) {
            // () — empty tuple (the unit-like case; used nowhere in
            // any current book/spec example, but falling through to
            // "expect an inner expression" would be a worse failure
            // mode than just accepting it as a zero-element tuple).
            advance();
            return std::make_shared<ASTNode>(NodeKind::TupleExpr, "", ln, cl);
        }
        auto first = parse_expr();
        if (check(COMMA)) {
            auto node = std::make_shared<ASTNode>(NodeKind::TupleExpr, "", ln, cl);
            node->children.push_back(first);
            while (match(COMMA)) {
                if (check(RPAREN)) break; // trailing comma
                node->children.push_back(parse_expr());
            }
            expect(RPAREN, "Expected ')' after tuple elements");
            return node;
        }
        expect(RPAREN, "Expected ')'");
        return first;
    }

    // spawn TypeName { field: val, ... }
    if (check(SPAWN)) return parse_spawn_expr();

    // proc "cmd"
    if (check(PROC)) return parse_proc_expr();

    // env.VARNAME
    if (check(ENV))  return parse_env_expr();

    // glob "pattern"
    if (check(GLOB)) return parse_glob_expr();

    // cast(expr, Type)
    if (check(CAST)) {
        advance();
        auto node = std::make_shared<ASTNode>(NodeKind::CastExpr, "", ln, cl);
        expect(LPAREN, "Expected '('");
        node->children.push_back(parse_expr());
        expect(COMMA, "Expected ','");
        node->extra = parse_type_name();
        expect(RPAREN, "Expected ')'");
        return node;
    }

    // typeof(expr)
    if (check(TYPEOF)) {
        advance();
        auto node = std::make_shared<ASTNode>(NodeKind::TypeofExpr, "", ln, cl);
        expect(LPAREN, "Expected '('");
        node->children.push_back(parse_expr());
        expect(RPAREN, "Expected ')'");
        return node;
    }

    // sizeof(Type)
    if (check(SIZEOF)) {
        advance();
        auto node = std::make_shared<ASTNode>(NodeKind::SizeofExpr, "", ln, cl);
        expect(LPAREN, "Expected '('");
        node->extra = parse_type_name();
        expect(RPAREN, "Expected ')'");
        return node;
    }

    // await expr
    if (check(AWAIT)) {
        advance();
        auto node = std::make_shared<ASTNode>(NodeKind::AwaitExpr, "", ln, cl);
        node->children.push_back(parse_expr(Prec::Unary));
        return node;
    }

    // weave() chain
    if (check(WEAVE)) return parse_weave_expr();

    // Identifier (variable / function call) or Realm::path
    if (check(IDENTIFIER)) {
        std::string full = advance().value;
        bool is_path = false;
        while (check(COLON_COLON)) {
            advance(); // consume '::'
            is_path = true;
            full += "::";
            if (check(IDENTIFIER)) full += advance().value;
            else { error("Expected identifier after '::'"); break; }
        }
        if (is_path) {
            auto n = std::make_shared<ASTNode>(NodeKind::PathExpr, full, ln, cl);
            return n;
        }
        auto n = std::make_shared<ASTNode>(NodeKind::Identifier, full, ln, cl);
        return n;
    }

    // Fallback — unknown token
    error("Unexpected token: '" + peek().value + "'");
    auto n = std::make_shared<ASTNode>(NodeKind::NullLit, "", ln, cl);
    if (!check(END_OF_FILE)) advance();
    return n;
}

ASTNodePtr Parser::parse_call(ASTNodePtr callee) {
    auto node = span_node(NodeKind::CallExpr);
    node->children.push_back(callee);
    advance(); // '('
    while (!check(RPAREN) && !check(END_OF_FILE)) {
        size_t pos_before_arg = pos_;
        node->children.push_back(parse_expr());
        match(COMMA);
        if (ensure_progress_or_recover(pos_before_arg, "call arguments")) break;
    }
    expect(RPAREN, "Expected ')'");
    return node;
}

ASTNodePtr Parser::parse_index(ASTNodePtr array) {
    auto node = span_node(NodeKind::IndexExpr);
    node->children.push_back(array);
    advance(); // '['
    node->children.push_back(parse_expr());
    expect(R_BRACKET, "Expected ']'");
    return node;
}

ASTNodePtr Parser::parse_member(ASTNodePtr obj) {
    advance(); // '.'
    auto node = span_node(NodeKind::MemberExpr);
    node->children.push_back(obj);
    if (check(IDENTIFIER)) {
        node->value = advance().value;
    } else if (check(NUMBER_INT)) {
        // Tuple positional access: point.0, point.1, ... — lexes as
        // IDENTIFIER DOT NUMBER_INT (confirmed: number-lexing only
        // triggers when the *first* character is already a digit, so
        // the '.' here is tokenized as a standalone DOT by the main
        // dispatch loop, not merged into a float the way "0.5" would
        // be — there's no ambiguity to resolve). node->value holds
        // the index as text ("0", "1", ...); codegen (both backends)
        // reads this the same way it reads a named field, just
        // emitting std::get<N> instead of .field_name for a value
        // whose declared/inferred type is a tuple.
        node->value = advance().value;
    }
    return node;
}

// ── lambda: |x: int, y: int| expr ───────────────────────────
ASTNodePtr Parser::parse_lambda() {
    uint32_t ln = peek().line;
    advance(); // opening '|'
    auto node = std::make_shared<ASTNode>(NodeKind::LambdaExpr, "", ln);
    while (!check(PIPE) && !check(END_OF_FILE)) {
        size_t pos_before_param = pos_;
        auto param = parse_field_decl();
        if (param) node->children.push_back(param);
        match(COMMA);
        if (ensure_progress_or_recover(pos_before_param, "lambda parameter list")) break;
    }
    expect(PIPE, "Expected closing '|' in lambda");
    // Body: block or expression
    if (check(L_BRACE)) node->children.push_back(parse_block());
    else                node->children.push_back(parse_expr(Prec::Assign));
    return node;
}

// ── weave().method().method() ────────────────────────────────
ASTNodePtr Parser::parse_weave_expr() {
    uint32_t ln = peek().line;
    advance(); // 'weave'
    auto node = std::make_shared<ASTNode>(NodeKind::WeaveExpr, "weave", ln);
    if (match(LPAREN)) expect(RPAREN, "Expected ')'");
    // Parse .modifier(args) chain
    while (check(DOT)) {
        advance();
        std::string mod;
        if (check(IDENTIFIER)) mod = advance().value;
        auto m = std::make_shared<ASTNode>(NodeKind::CallExpr, mod, ln);
        if (match(LPAREN)) {
            while (!check(RPAREN) && !check(END_OF_FILE)) {
                size_t pos_before_arg = pos_;
                m->children.push_back(parse_expr());
                match(COMMA);
                if (ensure_progress_or_recover(pos_before_arg, "weave modifier arguments")) break;
            }
            expect(RPAREN, "Expected ')'");
        }
        node->children.push_back(m);
    }
    return node;
}

// ── spawn TypeName { field: val, ... } ────────────────────────
ASTNodePtr Parser::parse_spawn_expr() {
    uint32_t ln = peek().line;
    advance(); // 'spawn'
    auto node = std::make_shared<ASTNode>(NodeKind::SpawnExpr, "", ln);
    if (check(IDENTIFIER)) {
        std::string tname = advance().value;
        while (check(COLON_COLON)) {
            advance();
            tname += "::";
            if (check(IDENTIFIER)) tname += advance().value;
        }
        // Explicit generic type argument(s): `spawn Box<int> { ... }`
        // — previously unparsed (this loop only ever handled `::`
        // qualification), so `spawn Box<int>` failed outright with a
        // cascade of "Expected '{'"/"Expected ':'" errors, since the
        // parser had no idea '<' could legally appear here at all.
        // Mirrors parse_type_name's own generic-argument parsing
        // (`Type<T>` as a flat string with the angle brackets kept
        // literally) so downstream (monomorphize.cpp's SpawnExpr
        // handling in particular) sees the identical "Box<int>" shape
        // it already expects from a type-annotation position.
        if (check(LT)) {
            advance();
            tname += "<" + parse_type_name();
            while (match(COMMA)) tname += ", " + parse_type_name();
            expect(GT, "Expected '>' after generic type argument(s)");
            tname += ">";
        }
        node->value = tname;
    }
    expect(L_BRACE, "Expected '{'");
    while (!check(R_BRACE) && !check(END_OF_FILE)) {
        size_t pos_before_field = pos_;
        std::string k;
        if (check(IDENTIFIER)) k = advance().value;
        expect(COLON, "Expected ':'");
        auto val = parse_expr();
        val->attrs["field"] = k;
        node->children.push_back(val);
        match(COMMA);
        if (ensure_progress_or_recover(pos_before_field, "struct field list")) break;
    }
    expect(R_BRACE, "Expected '}'");
    return node;
}

// ── proc "cmd" ───────────────────────────────────────────────
ASTNodePtr Parser::parse_proc_expr() {
    uint32_t ln = peek().line;
    advance(); // 'proc'
    auto node = std::make_shared<ASTNode>(NodeKind::ProcExpr, "", ln);
    node->children.push_back(parse_expr());
    return node;
}

// ── env.VARNAME ───────────────────────────────────────────────
ASTNodePtr Parser::parse_env_expr() {
    uint32_t ln = peek().line;
    advance(); // 'env'
    auto node = std::make_shared<ASTNode>(NodeKind::EnvExpr, "", ln);
    if (check(DOT)) {
        advance();
        if (check(IDENTIFIER)) node->value = advance().value;
    }
    return node;
}

// ── glob "pattern" ────────────────────────────────────────────
ASTNodePtr Parser::parse_glob_expr() {
    uint32_t ln = peek().line;
    advance(); // 'glob'
    auto node = std::make_shared<ASTNode>(NodeKind::GlobExpr, "", ln);
    node->children.push_back(parse_expr());
    return node;
}

// ── Type name parsing ─────────────────────────────────────────
std::string Parser::parse_type_name() {
    // Tuple type: (T, U, ...) — represented as the literal source
    // text "(T, U)" (parenthesized, comma-separated), the same flat-
    // string convention every other composite type name in this
    // function already uses (e.g. "Vec<int>"). Codegen recognizes the
    // leading '(' the same way it recognizes a trailing "<...>" for a
    // generic instantiation, and maps it to std::tuple<...>.
    if (check(LPAREN)) {
        advance();
        std::string tname = "(";
        if (!check(RPAREN)) {
            tname += parse_type_name();
            while (match(COMMA)) tname += ", " + parse_type_name();
        }
        expect(RPAREN, "Expected ')' after tuple type");
        tname += ")";
        return tname;
    }
    // own T, ref T, mut_ref T
    if (check(OWN)) {
        advance();
        return "own " + parse_type_name();
    }
    if (check(REF)) {
        advance();
        return "ref " + parse_type_name();
    }
    if (check(MUT_REF)) {
        advance();
        return "mut_ref " + parse_type_name();
    }
    // Raw pointer: *mut T / *const T  (FFI / unsafe interop)
    if (check(STAR)) {
        advance();
        std::string qualifier;
        if (check(CONST)) { advance(); qualifier = "const"; }
        else if (check(IDENTIFIER) && peek().value == "mut") { advance(); qualifier = "mut"; }
        else qualifier = "mut"; // default if unspecified
        return "*" + qualifier + " " + parse_type_name();
    }
    // Builtin types
    if (check_any({TYPE_INT, TYPE_FLOAT, TYPE_BOOL, TYPE_STR,
                   TYPE_AUTO, VOID_KW})) {
        return advance().value;
    }
    // Named type (identifier) — supports Realm::Type qualified names
    if (check(IDENTIFIER)) {
        std::string name = advance().value;
        while (check(COLON_COLON)) {
            advance();
            name += "::";
            if (check(IDENTIFIER)) name += advance().value;
        }
        // Generic: Type<T>
        if (check(LT)) {
            advance();
            name += "<" + parse_type_name();
            while (match(COMMA)) name += ", " + parse_type_name();
            expect(GT, "Expected '>'");
            name += ">";
        }
        return name;
    }
    return "auto";
}

// ── field decl: name [: type] [= default] ────────────────────
ASTNodePtr Parser::parse_field_decl() {
    uint32_t ln = peek().line;
    auto node = std::make_shared<ASTNode>(NodeKind::FieldDecl, "", ln);
    // An explicit `self` first parameter (e.g. `impl Numeric for User
    // { double_it(self) -> int { ... } }`) lexes as the dedicated
    // SELF token, never as IDENTIFIER — this branch was previously
    // entirely missing, so `self` was left unconsumed here and then
    // reported as "Unexpected token in parameter list: 'self'" by the
    // caller's recovery logic (which force-advances past it and
    // continues, so a single bad diagnostic was the only visible
    // symptom rather than an infinite loop or a total parse failure —
    // easy to miss unless something is specifically checking parser
    // stderr output, not just checking that an AST came out). Treated
    // as a bare `self` marker parameter (no explicit type — codegen's
    // implicit-self injection, see ir_lower.cpp's ImplDecl case and
    // the AST transpiler's equivalent, already know how to supply the
    // receiver type without one).
    if (check(SELF)) {
        advance();
        node->value = "self";
        return node;
    }
    if (check(IDENTIFIER)) node->value = advance().value;
    if (check(COLON)) { advance(); node->extra = parse_type_name(); }
    if (match(EQUAL)) node->children.push_back(parse_expr());
    return node;
}

// ── method signature in nexus: name(params) -> ret ───────────
ASTNodePtr Parser::parse_method_decl() {
    uint32_t ln = peek().line;
    // If it starts with 'pulse' keyword treat as full pulse
    if (check(PULSE)) return parse_pulse_decl(false);

    auto node = std::make_shared<ASTNode>(NodeKind::MethodDecl, "", ln);
    if (check(IDENTIFIER)) node->value = advance().value;
    if (check(LPAREN)) {
        advance();
        int pi = 0;
        while (!check(RPAREN) && !check(END_OF_FILE)) {
            size_t pos_before_param = pos_;
            auto param = parse_field_decl();
            if (param) {
                node->attrs["param_" + std::to_string(pi++)] = param->value;
                node->attrs["param_type_" + std::to_string(pi-1)] = param->extra;
                node->children.push_back(param);
            }
            match(COMMA);
            if (ensure_progress_or_recover(pos_before_param, "method parameter list")) break;
        }
        expect(RPAREN, "Expected ')'");
    }
    // Return type stored in extra2, matching PulseDecl's convention
    // (see parse_pulse_decl) — every downstream consumer of a method
    // node (emit_forge's in-struct declaration, emit_impl's
    // out-of-line definition, ir_lower.cpp's ImplDecl case) reads
    // extra2 for the return type, since these methods (from impl
    // blocks) are now real, codegen'd declarations, not just
    // parsed-and-discarded nexus interface signatures the way a
    // MethodDecl with no body originally only ever was.
    if (match(ARROW)) node->extra2 = parse_type_name();
    // Abstract method in nexus has no body
    if (check(L_BRACE)) node->children.push_back(parse_block());
    match(SEMICOLON);
    return node;
}

} // namespace xphage::parse
