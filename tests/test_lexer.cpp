// ============================================================
// X-Phage Lexer Tests v4.0.0
// ============================================================
#include "xphage/ast.hpp"
#include "lexer.hpp"
#include <cassert>
#include <iostream>

void test_keywords() {
    xphage::lexer::Lexer lex(
        "pulse atom shadow global beam bypass quantum if elif else while for in return break continue", "test");
    auto toks = lex.tokenize();
    assert(toks[0].type  == PULSE    && "pulse");
    assert(toks[1].type  == ATOM     && "atom");
    assert(toks[2].type  == SHADOW   && "shadow");
    assert(toks[3].type  == GLOBAL   && "global");
    assert(toks[4].type  == BEAM     && "beam");
    assert(toks[5].type  == BYPASS   && "bypass");
    assert(toks[6].type  == QUANTUM  && "quantum");
    assert(toks[7].type  == IF       && "if");
    assert(toks[8].type  == ELIF     && "elif");
    assert(toks[9].type  == ELSE     && "else");
    assert(toks[10].type == WHILE    && "while");
    assert(toks[11].type == FOR      && "for");
    assert(toks[12].type == IN       && "in");
    assert(toks[13].type == RETURN   && "return");
    assert(toks[14].type == BREAK    && "break");
    assert(toks[15].type == CONTINUE && "continue");
    std::cout << "[PASS] test_keywords\n";
}

void test_phase2_keywords() {
    xphage::lexer::Lexer lex("forge nexus flux probe diverge emit absorb weave impl self", "test");
    auto toks = lex.tokenize();
    assert(toks[0].type == FORGE   && "forge");
    assert(toks[1].type == NEXUS   && "nexus");
    assert(toks[2].type == FLUX    && "flux");
    assert(toks[3].type == PROBE   && "probe");
    assert(toks[4].type == DIVERGE && "diverge");
    assert(toks[5].type == EMIT    && "emit");
    assert(toks[6].type == ABSORB  && "absorb");
    assert(toks[7].type == WEAVE   && "weave");
    assert(toks[8].type == IMPL    && "impl");
    assert(toks[9].type == SELF    && "self");
    std::cout << "[PASS] test_phase2_keywords\n";
}

void test_phase3_keywords() {
    xphage::lexer::Lexer lex("own ref mut_ref async await yield proc env glob lambda", "test");
    auto toks = lex.tokenize();
    assert(toks[0].type == OWN     && "own");
    assert(toks[1].type == REF     && "ref");
    assert(toks[2].type == MUT_REF && "mut_ref");
    assert(toks[3].type == ASYNC   && "async");
    assert(toks[4].type == AWAIT   && "await");
    assert(toks[5].type == YIELD   && "yield");
    assert(toks[6].type == PROC    && "proc");
    assert(toks[7].type == ENV     && "env");
    assert(toks[8].type == GLOB    && "glob");
    assert(toks[9].type == LAMBDA  && "lambda");
    std::cout << "[PASS] test_phase3_keywords\n";
}

void test_operators() {
    xphage::lexer::Lexer lex("== != <= >= |> .. -> && || += -= *= /= ?", "test");
    auto toks = lex.tokenize();
    assert(toks[0].type == EQ_EQ    && "==");
    assert(toks[1].type == BANG_EQ  && "!=");
    assert(toks[2].type == LT_EQ    && "<=");
    assert(toks[3].type == GT_EQ    && ">=");
    assert(toks[4].type == PIPE_GT  && "|>");
    assert(toks[5].type == DOT_DOT  && "..");
    assert(toks[6].type == ARROW    && "->");
    assert(toks[7].type == AND_AND  && "&&");
    assert(toks[8].type == PIPE_PIPE && "||");
    assert(toks[9].type == PLUS_EQ  && "+=");
    assert(toks[10].type == MINUS_EQ && "-=");
    assert(toks[11].type == STAR_EQ  && "*=");
    assert(toks[12].type == SLASH_EQ && "/=");
    assert(toks[13].type == QUESTION && "?");
    std::cout << "[PASS] test_operators\n";
}

void test_fstring() {
    xphage::lexer::Lexer lex(R"(f"Hello {name}, score: {score}")", "test");
    auto toks = lex.tokenize();
    assert(toks[0].type == FSTRING);
    assert(toks[0].value == "Hello {name}, score: {score}");
    std::cout << "[PASS] test_fstring\n";
}

void test_numbers() {
    xphage::lexer::Lexer lex("42 3.14 0xFF 100f", "test");
    auto toks = lex.tokenize();
    assert(toks[0].type == NUMBER_INT   && toks[0].value == "42");
    assert(toks[1].type == NUMBER_FLOAT && toks[1].value == "3.14");
    assert(toks[2].type == NUMBER_INT   && toks[2].value == "0xFF");
    assert(toks[3].type == NUMBER_FLOAT);
    std::cout << "[PASS] test_numbers\n";
}

void test_link() {
    xphage::lexer::Lexer lex("~link \"io\"", "test");
    auto toks = lex.tokenize();
    assert(toks[0].type == LINK);
    assert(toks[1].type == STRING && toks[1].value == "io");
    std::cout << "[PASS] test_link\n";
}

void test_line_tracking() {
    xphage::lexer::Lexer lex("atom x\natom y\natom z", "test");
    auto toks = lex.tokenize();
    assert(toks[0].line == 1 && "line 1");
    assert(toks[2].line == 2 && "line 2");
    assert(toks[4].line == 3 && "line 3");
    std::cout << "[PASS] test_line_tracking\n";
}

void test_comments_stripped() {
    xphage::lexer::Lexer lex("atom x = 1 // this is a comment\natom y = 2", "test");
    auto toks = lex.tokenize();
    // Should not have any comment token
    for (auto& t : toks)
        assert(t.type != TOK_ERROR);
    // Should have: atom x = 1 atom y = 2 EOF
    assert(toks[0].type == ATOM);
    assert(toks[1].type == IDENTIFIER && toks[1].value == "x");
    std::cout << "[PASS] test_comments_stripped\n";
}

int main() {
    std::cout << "=== X-Phage Lexer Tests ===\n";
    test_keywords();
    test_phase2_keywords();
    test_phase3_keywords();
    test_operators();
    test_fstring();
    test_numbers();
    test_link();
    test_line_tracking();
    test_comments_stripped();
    std::cout << "=== All lexer tests passed ===\n";
    return 0;
}
