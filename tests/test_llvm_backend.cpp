// ============================================================
// X-Phage LLVM Backend Integration Tests v4.0.0
// Only meaningful when compiled with -DENABLE_LLVM=ON
// AeonCoreX Lab
// ============================================================
#include "xphage/ast.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include "codegen_llvm.hpp"
#include <cassert>
#include <iostream>
#include <fstream>
#include <cstdlib>

static Program parse(const std::string& src) {
    xphage::lexer::Lexer lex(src, "test");
    auto toks = lex.tokenize();
    xphage::parse::Parser parser(toks, "test");
    auto ast = parser.parse();
    if (parser.has_errors())
        for (auto& e : parser.errors())
            std::cerr << "  err:" << e.line << ": " << e.message << "\n";
    return ast;
}

void test_llvm_available() {
#ifdef ENABLE_LLVM
    assert(xphage::codegen_llvm::is_available());
    std::string ver = xphage::codegen_llvm::llvm_version_str();
    assert(!ver.empty());
    std::cout << "[PASS] test_llvm_available — " << ver << "\n";
#else
    assert(!xphage::codegen_llvm::is_available());
    std::cout << "[SKIP] test_llvm_available — LLVM not compiled in\n";
#endif
}

void test_compile_hello_llvm() {
#ifndef ENABLE_LLVM
    std::cout << "[SKIP] test_compile_hello_llvm — LLVM not compiled in\n";
    return;
#else
    auto ast = parse(R"(
beam "Hello from LLVM!"
atom x: int = 21
atom y: int = 21
beam f"answer={x + y}"
)");
    assert(!ast.empty());

    xphage::codegen_llvm::LLVMConfig cfg;
    cfg.opt_level   = 0;
    cfg.emit_llvm_ir = true;
    cfg.verbose     = false;

    std::string obj = "/tmp/test_hello.o";
    auto res = xphage::codegen_llvm::compile_ast(ast, obj, cfg);

    if (!res.success) {
        std::cerr << "  LLVM error: " << res.error << "\n";
    }
    assert(res.success);

    // Check .o file was created
    std::ifstream f(obj);
    assert(f.good() && "object file not created");

    // Check .ll file was created
    if (!res.llvm_ir_path.empty()) {
        std::ifstream fll(res.llvm_ir_path);
        assert(fll.good() && ".ll file not created");
        std::cout << "  IR dump: " << res.llvm_ir_path << "\n";
    }

    std::cout << "[PASS] test_compile_hello_llvm (" << res.elapsed_ms << " ms)\n";

    // Link and run
    std::string bin = "/tmp/test_hello_xphage";
    bool linked = xphage::codegen_llvm::link_binary({obj}, "", bin, false);
    assert(linked && "Link failed");

    int rc = std::system(bin.c_str());
    assert(rc == 0);
    std::cout << "[PASS] test_run_hello_llvm\n";
#endif
}

void test_control_flow_llvm() {
#ifndef ENABLE_LLVM
    std::cout << "[SKIP] test_control_flow_llvm — LLVM not compiled in\n";
    return;
#else
    auto ast = parse(R"(
shadow sum: int = 0
for i in 0..10 {
    sum = sum + i
}
beam f"sum={sum}"
)");
    xphage::codegen_llvm::LLVMConfig cfg;
    cfg.opt_level = 2;
    std::string obj = "/tmp/test_loop.o";
    auto res = xphage::codegen_llvm::compile_ast(ast, obj, cfg);
    assert(res.success);

    std::string bin = "/tmp/test_loop_xp";
    assert(xphage::codegen_llvm::link_binary({obj}, "", bin));

    // Capture output
    FILE* fp = popen(bin.c_str(), "r");
    char buf[256] = {};
    fgets(buf, sizeof(buf), fp);
    pclose(fp);
    std::string out(buf);
    // sum of 0..9 = 45
    assert(out.find("sum=45") != std::string::npos);
    std::cout << "[PASS] test_control_flow_llvm\n";
#endif
}

void test_function_call_llvm() {
#ifndef ENABLE_LLVM
    std::cout << "[SKIP] test_function_call_llvm\n";
    return;
#else
    auto ast = parse(R"(
pulse add(a: int, b: int) -> int {
    return a + b
}
pulse square(n: int) -> int {
    return n * n
}
beam f"add(3,4)={add(3,4)}"
beam f"sq(7)={square(7)}"
)");
    xphage::codegen_llvm::LLVMConfig cfg;
    cfg.opt_level = 2;
    std::string obj = "/tmp/test_fn.o";
    auto res = xphage::codegen_llvm::compile_ast(ast, obj, cfg);
    assert(res.success);

    std::string bin = "/tmp/test_fn_xp";
    assert(xphage::codegen_llvm::link_binary({obj}, "", bin));

    FILE* fp = popen(bin.c_str(), "r");
    char buf[512] = {}; char line[256];
    while (fgets(line, sizeof(line), fp)) strncat(buf, line, sizeof(buf)-strlen(buf)-1);
    pclose(fp);
    std::string out(buf);
    assert(out.find("add(3,4)=7")  != std::string::npos);
    assert(out.find("sq(7)=49")    != std::string::npos);
    std::cout << "[PASS] test_function_call_llvm\n";
#endif
}

void test_optimisation_levels() {
#ifndef ENABLE_LLVM
    std::cout << "[SKIP] test_optimisation_levels\n";
    return;
#else
    auto ast = parse("beam \"opt_test\"");
    for (int opt : {0, 1, 2, 3}) {
        xphage::codegen_llvm::LLVMConfig cfg;
        cfg.opt_level = opt;
        std::string obj = "/tmp/test_opt" + std::to_string(opt) + ".o";
        auto res = xphage::codegen_llvm::compile_ast(ast, obj, cfg);
        assert(res.success && ("Opt level " + std::to_string(opt) + " failed").c_str());
    }
    std::cout << "[PASS] test_optimisation_levels (O0-O3)\n";
#endif
}

int main() {
    std::cout << "=== X-Phage LLVM Backend Tests ===\n";
    test_llvm_available();
    test_compile_hello_llvm();
    test_control_flow_llvm();
    test_function_call_llvm();
    test_optimisation_levels();
    std::cout << "=== Done ===\n";
    return 0;
}
