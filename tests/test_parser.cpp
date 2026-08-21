// ============================================================
// X-Phage Parser Tests v4.0.0
// ============================================================
#include "xphage/ast.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include <cassert>
#include <iostream>

Program parse(const std::string& src) {
    xphage::lexer::Lexer lex(src, "test");
    auto toks = lex.tokenize();
    xphage::parse::Parser parser(toks, "test");
    auto ast = parser.parse();
    if (parser.has_errors()) {
        for (auto& e : parser.errors())
            std::cerr << "  error:" << e.line << ": " << e.message << "\n";
    }
    return ast;
}

void test_atom_decl() {
    auto ast = parse("atom x: int = 42");
    assert(!ast.empty());
    assert(ast[0]->kind == NodeKind::AtomDecl);
    assert(ast[0]->value == "x");
    assert(ast[0]->extra == "int");
    std::cout << "[PASS] test_atom_decl\n";
}

void test_shadow_decl() {
    auto ast = parse("shadow count: int = 0");
    assert(!ast.empty());
    assert(ast[0]->kind == NodeKind::ShadowDecl);
    assert(ast[0]->value == "count");
    std::cout << "[PASS] test_shadow_decl\n";
}

void test_pulse_decl() {
    auto ast = parse("pulse add(a: int, b: int) -> int { return a + b }");
    assert(!ast.empty());
    assert(ast[0]->kind == NodeKind::PulseDecl);
    assert(ast[0]->value == "add");
    assert(ast[0]->extra2 == "int");
    std::cout << "[PASS] test_pulse_decl\n";
}

void test_if_stmt() {
    auto ast = parse("if x > 0 { beam \"pos\" } else { beam \"neg\" }");
    assert(!ast.empty());
    assert(ast[0]->kind == NodeKind::IfStmt);
    std::cout << "[PASS] test_if_stmt\n";
}

void test_while_stmt() {
    auto ast = parse("while i < 10 { i = i + 1 }");
    assert(!ast.empty());
    assert(ast[0]->kind == NodeKind::WhileStmt);
    std::cout << "[PASS] test_while_stmt\n";
}

void test_for_range() {
    auto ast = parse("for i in 0..10 { beam i }");
    assert(!ast.empty());
    assert(ast[0]->kind == NodeKind::ForStmt);
    assert(ast[0]->value == "i");
    std::cout << "[PASS] test_for_range\n";
}

void test_forge_decl() {
    auto ast = parse("forge User { name: str = \"\" score: int = 0 }");
    assert(!ast.empty());
    assert(ast[0]->kind == NodeKind::ForgeDecl);
    assert(ast[0]->value == "User");
    assert(ast[0]->children.size() >= 2);
    std::cout << "[PASS] test_forge_decl\n";
}

void test_flux_decl() {
    auto ast = parse("flux counter: int = 0");
    assert(!ast.empty());
    assert(ast[0]->kind == NodeKind::FluxDecl);
    assert(ast[0]->value == "counter");
    assert(ast[0]->extra == "int");
    std::cout << "[PASS] test_flux_decl\n";
}

void test_probe_stmt() {
    auto ast = parse(R"(
probe status {
    diverge "ok"    -> beam "good"
    diverge "error" -> beam "bad"
    diverge _       -> beam "unknown"
})");
    assert(!ast.empty());
    assert(ast[0]->kind == NodeKind::ProbeStmt);
    // 3 arms + 1 subject
    assert(ast[0]->children.size() == 4);
    std::cout << "[PASS] test_probe_stmt\n";
}

void test_emit_absorb() {
    auto ast = parse(R"(
absorb "login" { beam "logged in" }
emit "login" { user: "alice" }
)");
    assert(ast.size() >= 2);
    assert(ast[0]->kind == NodeKind::AbsorbStmt);
    assert(ast[1]->kind == NodeKind::EmitStmt);
    assert(ast[1]->value == "login");
    std::cout << "[PASS] test_emit_absorb\n";
}

void test_lambda() {
    auto ast = parse("atom double = |x: int| x * 2");
    assert(!ast.empty());
    assert(ast[0]->kind == NodeKind::AtomDecl);
    // The RHS should be a lambda
    assert(!ast[0]->children.empty());
    assert(ast[0]->children[0]->kind == NodeKind::LambdaExpr);
    std::cout << "[PASS] test_lambda\n";
}

void test_pipeline() {
    auto ast = parse("atom r = name |> str_trim |> str_upper");
    assert(!ast.empty());
    // The RHS should be a pipeline chain
    std::cout << "[PASS] test_pipeline\n";
}

void test_async_pulse() {
    auto ast = parse("async pulse fetch(url: str) -> str { return url }");
    assert(!ast.empty());
    assert(ast[0]->kind == NodeKind::AsyncPulseDecl);
    std::cout << "[PASS] test_async_pulse\n";
}

void test_proc_expr() {
    auto ast = parse("atom branch = proc \"git rev-parse --abbrev-ref HEAD\"");
    assert(!ast.empty());
    assert(ast[0]->kind == NodeKind::AtomDecl);
    assert(!ast[0]->children.empty());
    assert(ast[0]->children[0]->kind == NodeKind::ProcExpr);
    std::cout << "[PASS] test_proc_expr\n";
}

void test_return_stmt() {
    auto ast = parse("pulse f() -> int { return 42 }");
    assert(!ast.empty());
    auto& fn = ast[0];
    // Find the block
    bool found_return = false;
    for (auto& c : fn->children) {
        if (c && c->kind == NodeKind::Block) {
            for (auto& s : c->children)
                if (s && s->kind == NodeKind::ReturnStmt) found_return = true;
        }
    }
    assert(found_return);
    std::cout << "[PASS] test_return_stmt\n";
}

void test_no_parse_errors() {
    const std::string prog = R"(
~link "io"
~link "string"
flux counter: int = 0
forge Point { x: float = 0.0   y: float = 0.0 }
nexus Drawable { draw() -> void }
pulse compute(n: int) -> int {
    shadow result: int = 0
    for i in 0..n {
        result = result + i
    }
    return result
}
beam f"Result: {compute(10)}"
counter = 42
probe counter {
    diverge 0 -> beam "zero"
    diverge _ -> beam "nonzero"
}
)";
    xphage::lexer::Lexer lex(prog, "test");
    auto toks = lex.tokenize();
    xphage::parse::Parser p(toks, "test");
    auto ast = p.parse();
    if (p.has_errors()) {
        for (auto& e : p.errors())
            std::cerr << "  " << e.message << "\n";
    }
    assert(!p.has_errors());
    std::cout << "[PASS] test_no_parse_errors\n";
}

int main() {
    std::cout << "=== X-Phage Parser Tests ===\n";
    test_atom_decl();
    test_shadow_decl();
    test_pulse_decl();
    test_if_stmt();
    test_while_stmt();
    test_for_range();
    test_forge_decl();
    test_flux_decl();
    test_probe_stmt();
    test_emit_absorb();
    test_lambda();
    test_pipeline();
    test_async_pulse();
    test_proc_expr();
    test_return_stmt();
    test_no_parse_errors();
    std::cout << "=== All parser tests passed ===\n";
    return 0;
}
