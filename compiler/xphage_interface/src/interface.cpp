// ============================================================
// xphage_interface — Compiler Pipeline Driver v3.5.0
// Source → Lex → Parse → IR → Codegen → Binary
// ============================================================
#include "../include/interface.hpp"
#include "xphage/runtime.hpp"
#include "xphage/ast.hpp"
#include "../../xphage_parse/include/parser.hpp"
#include "../../xphage_middle/include/ir_lower.hpp"

#include <fstream>
#include <sstream>
#include <iostream>
#include <chrono>
#include <cstdlib>

#ifdef __APPLE__
    #include <TargetConditionals.h>
#endif

namespace xphage::interface {

// ── Forward decls (implemented in their own TUs) ─────────────
// These are called via the legacy C++ class interface for now.
extern "C++" {
    // declared in runtime.hpp
}

// ── Helpers ──────────────────────────────────────────────────
static std::string read_file(const std::string& path) {
    std::ifstream f(path);
    if (!f.is_open()) return "";
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

static std::string detect_target() {
#if defined(__aarch64__) || defined(_M_ARM64)
    #ifdef _WIN32
        return "aarch64-pc-windows-msvc";
    #elif defined(__APPLE__)
        return "arm64-apple-macosx11.0";
    #else
        return "aarch64-linux-gnu";
    #endif
#elif defined(__x86_64__) || defined(_M_X64)
    #ifdef _WIN32
        return "x86_64-pc-windows-msvc";
    #elif defined(__APPLE__)
        return "x86_64-apple-macosx10.15";
    #else
        return "x86_64-linux-gnu";
    #endif
#else
    return "unknown";
#endif
}

// ── compile() ────────────────────────────────────────────────
CompileResult compile(const CompileConfig& cfg) {
    auto t0 = std::chrono::steady_clock::now();
    CompileResult res;

    // 1. Read source
    std::string src = read_file(cfg.source_path);
    if (src.empty()) {
        res.errors.push_back("Cannot open: " + cfg.source_path);
        return res;
    }

    // 2. Lex
    XPhageLexer lexer;
    auto tokens = lexer.tokenize(src, cfg.source_path);

    // 3. Parse → AST
    xphage::parse::Parser parser(tokens, cfg.source_path);
    auto ast = parser.parse();
    for (auto& e : parser.errors())
        res.errors.push_back(cfg.source_path + ":" +
            std::to_string(e.line) + ":" + std::to_string(e.col) +
            ": error: " + e.message);
    if (parser.has_errors()) return res;

    // 4. Emit=AST dump
    if (cfg.emit == EmitKind::AST) {
        std::ofstream out(cfg.output_path + ".ast");
        out << "; X-Phage AST dump for: " << cfg.source_path << "\n";
        std::function<void(const ASTNodePtr&, int)> dump_ast =
            [&](const ASTNodePtr& n, int d) {
                if (!n) return;
                out << std::string(d * 2, ' ') << "[" << (int)n->kind
                    << "] " << n->value;
                if (!n->extra.empty()) out << " | " << n->extra;
                out << "\n";
                for (auto& c : n->children) dump_ast(c, d + 1);
            };
        for (auto& s : ast) dump_ast(s, 0);
        res.success = true;
        res.output_path = cfg.output_path + ".ast";
        return res;
    }

    // 5. IR lowering
    std::string mod_name = cfg.source_path.substr(
        cfg.source_path.find_last_of("/\\") + 1);
    auto ir_mod = xphage::middle::lower_to_ir(ast, mod_name);

    // 6. Emit=IR dump
    if (cfg.emit == EmitKind::IR) {
        std::string ir_text = xphage::middle::dump_ir(ir_mod);
        std::string out_path = cfg.output_path + ".xpir";
        std::ofstream out(out_path);
        out << ir_text;
        res.success = true;
        res.output_path = out_path;
        return res;
    }

    // 7. Codegen
    std::string target = cfg.target_triple.empty() ? detect_target()
                                                    : cfg.target_triple;
    if (cfg.verbose)
        std::cout << "[interface] Target: " << target << "\n";

    if (cfg.backend == Backend::LLVM) {
#ifdef ENABLE_LLVM
        XPhageLLVMCompiler llvm;
        std::string obj = cfg.output_path + ".o";
        llvm.compile_tokens(tokens, obj);

        // Link
        std::string link_cmd = "clang " + obj + " -o " + cfg.output_path + " -lm";
        if (cfg.optimize) link_cmd += " -O3";
        int r = std::system(link_cmd.c_str());
        if (r != 0) {
            res.errors.push_back("Linker failed (exit " + std::to_string(r) + ")");
            return res;
        }
        res.output_path = cfg.output_path;
        res.success     = true;
#else
        res.warnings.push_back("LLVM not compiled in — falling back to Transpiler");
        goto transpiler_path;
#endif
    } else {
#ifndef ENABLE_LLVM
        transpiler_path:
#endif
        XPhageTranspiler transpiler;
        std::string cpp_out = cfg.output_path + "_gen.cpp";
        transpiler.transpile_to_cpp(tokens, cpp_out);

        if (cfg.emit == EmitKind::Transpiled) {
            res.success = true;
            res.output_path = cpp_out;
            return res;
        }

#if defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE
        res.success = true;
        res.output_path = cpp_out;
        return res;
#else
        std::string cc_cmd;
#ifdef _WIN32
        cc_cmd = "g++ " + cpp_out + " -o " + cfg.output_path +
                 ".exe -std=c++17";
#else
        cc_cmd = "c++ " + cpp_out + " -o " + cfg.output_path +
                 " -std=c++17 -pthread";
#endif
        if (cfg.optimize) cc_cmd += " -O3";
        int r = std::system(cc_cmd.c_str());
        if (r != 0) {
            res.errors.push_back("C++ compilation failed");
            return res;
        }
        res.success = true;
        res.output_path = cfg.output_path;
#endif
    }

    auto t1 = std::chrono::steady_clock::now();
    res.elapsed_ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
    if (cfg.verbose)
        std::cout << "[interface] Compiled in " << res.elapsed_ms << " ms\n";

    return res;
}

// ── run_file() ────────────────────────────────────────────────
// Interpret: lex + execute tokens directly via runtime
int run_file(const std::string& path) {
    std::string src = read_file(path);
    if (src.empty()) {
        std::cerr << "Error: cannot open " << path << "\n";
        return 1;
    }

    XPhageLexer lexer;
    auto tokens = lexer.tokenize(src, path);

    XPhageRuntime runtime;
    XPhageLinker  linker;
    runtime.ui_root = std::make_shared<FusionNode>("ROOT_CANVAS");

    // Process ~link directives first
    for (size_t i = 0; i < tokens.size(); i++) {
        if (tokens[i].type == LINK && i + 1 < tokens.size()) {
            linker.link_library(tokens[i + 1].value, runtime);
            i++;
        }
    }

    // Execute statements
    for (size_t i = 0; i < tokens.size(); i++) {
        auto& t = tokens[i];

        if (t.type == GLOBAL && i + 3 < tokens.size()) {
            runtime.write_global(tokens[i+1].value, tokens[i+3].value);
            i += 3;
        }
        else if (t.type == ATOM && i + 3 < tokens.size()) {
            runtime.write(tokens[i+1].value, tokens[i+3].value, "atom", true);
            i += 3;
        }
        else if (t.type == SHADOW && i + 3 < tokens.size()) {
            runtime.write(tokens[i+1].value, tokens[i+3].value, "shadow", false);
            i += 3;
        }
        else if (t.type == BEAM && i + 1 < tokens.size()) {
            auto cell = runtime.read(tokens[i+1].value);
            std::cout << cell.data << "\n";
            i++;
        }
        else if (t.type == BYPASS && i + 1 < tokens.size()) {
            runtime.hardware_bypass(tokens[i+1].value, "");
        }
        else if (t.type == QUANTUM && i + 1 < tokens.size()) {
            runtime.launch_quantum_process(tokens[i+1].value);
            i++;
        }
        else if (t.type == VORTEX) {
            runtime.activate_vortex();
        }
        else if (t.type == VOID) {
            runtime.activate_void_protocol();
        }
        else if (t.type == CHRONOS && i + 1 < tokens.size()) {
            runtime.activate_chronos(tokens[i+1].value);
            i++;
        }
        else if (t.type == ETHER && i + 2 < tokens.size()) {
            runtime.activate_ether(tokens[i+1].value, tokens[i+2].value);
            i += 2;
        }
        else if (t.type == SYNAPSE && i + 2 < tokens.size()) {
            runtime.establish_synapse(tokens[i+1].value, tokens[i+2].value);
            i += 2;
        }
        else if (t.type == MATRIX && i + 2 < tokens.size()) {
            runtime.process_matrix(tokens[i+1].value, tokens[i+3].value);
        }
        else if (t.type == FUSION) {
            runtime.init_fusion_engine();
        }
    }

    if (runtime.ui_active) runtime.render_ui_tree();
    return 0;
}

// ── start_repl() ─────────────────────────────────────────────
void start_repl() {
    std::cout << "\033[1;36m";
    std::cout << "  _  _  ____  __  __ \n";
    std::cout << " ( \\/ )(  _ \\(  \\/  )\n";
    std::cout << "  )  (  )___/ )    ( \n";
    std::cout << " (_/\\_)(__)  (_/\\/_) \033[1;32mv3.5.0\033[0m\n\n";
    std::cout << "\033[1;35mX-Phage REPL — AeonCoreX Lab\033[0m\n";
    std::cout << "\033[1;36m💡 Type 'exit' to quit. 'help' for commands.\033[0m\n";

    XPhageRuntime runtime;
    XPhageLexer   lexer;
    XPhageLinker  linker;
    runtime.ui_root = std::make_shared<FusionNode>("ROOT_CANVAS");

    std::string line;
    while (true) {
        std::cout << "\033[1;32mxp> \033[0m";
        if (!std::getline(std::cin, line)) break;
        if (line == "exit" || line == "quit") break;
        if (line.empty()) continue;
        if (line == "help") {
            std::cout << "  exit    — quit\n"
                      << "  clear   — clear screen\n"
                      << "  vars    — show all variables\n"
                      << "  globals — show global registry\n"
                      << "  Any X-Phage statement\n";
            continue;
        }
        if (line == "clear") { std::system("clear"); continue; }
        if (line == "vars") {
            for (auto& [k, v] : runtime.cell_map)
                std::cout << "  " << k << " = " << v.data << " [" << v.type << "]\n";
            continue;
        }
        if (line == "globals") {
            for (auto& [k, v] : runtime.global_registry)
                std::cout << "  " << k << " = " << v.data << "\n";
            continue;
        }

        // Execute single line
        std::string tmp_path = "/tmp/xp_repl.tmp.xp0";
        std::ofstream tf(tmp_path);
        tf << line << "\n";
        tf.close();
        run_file(tmp_path);
    }
}

} // namespace xphage::interface
