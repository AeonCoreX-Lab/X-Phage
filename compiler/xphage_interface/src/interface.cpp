// ============================================================
// xphage_interface — Compiler Pipeline v4.0.0
// Phase 1-3: Transpiler path   Phase 4: LLVM native path
// AeonCoreX Lab
// ============================================================
#include "../include/interface.hpp"
#include "xphage/runtime.hpp"
#include "xphage/ast.hpp"
#include "../../xphage_lexer/include/lexer.hpp"
#include "../../xphage_parse/include/parser.hpp"
#include "../../xphage_middle/include/ir_lower.hpp"
#include "../../xphage_sema/include/semantic_analyzer.hpp"
#include "../../xphage_generics/include/monomorphize.hpp"
#include "../../xphage_codegen_transpiler/include/codegen_transpiler.hpp"
#include "../../xphage_codegen_llvm/include/codegen_llvm.hpp"

#include <fstream>
#include <sstream>
#include <iostream>
#include <functional>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <unordered_set>

#ifdef __APPLE__
  #include <TargetConditionals.h>
#endif

namespace xphage::interface {

static std::string read_file(const std::string& p) {
    std::ifstream f(p);
    if (!f.is_open()) return "";
    std::ostringstream ss; ss << f.rdbuf(); return ss.str();
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
#else
  #ifdef _WIN32
    return "x86_64-pc-windows-msvc";
  #elif defined(__APPLE__)
    return "x86_64-apple-macosx10.15";
  #else
    return "x86_64-linux-gnu";
  #endif
#endif
}

// Shared LLVM-backend codegen+link path, used by every compile
// entry point (compile(), compile_xp(), compile_multi()) — not
// just compile() as it originally was. `cfg.backend == Backend::
// LLVM` was previously only checked inside compile(), which meant
// `--backend=llvm` was silently ignored for a plain `.xp` file (the
// primary, developer-facing format) and for any Mixed-mode merge;
// only the direct multi-file `.xh`/`.xui`/`.xp0` path could ever
// reach LLVM codegen at all.
//
// Returns true if this call fully handled the compile request
// (successfully or not — check res.success / res.errors either
// way) and the caller should return `res` immediately. Returns
// false if the LLVM backend wasn't selected (cfg.backend !=
// Backend::LLVM), meaning the caller should fall through to its
// normal transpiler path.
static bool try_llvm_backend(const Program& prog, const CompileConfig& cfg,
                              CompileResult& res,
                              std::chrono::steady_clock::time_point t0) {
    if (cfg.backend != Backend::LLVM) return false;

    auto finish = [&](bool success) {
        auto t1 = std::chrono::steady_clock::now();
        res.elapsed_ms = std::chrono::duration<double,std::milli>(t1-t0).count();
        res.success = success;
        return true;
    };

    if (!xphage::codegen_llvm::is_available()) {
        res.errors.push_back("LLVM backend not available. Rebuild with -DENABLE_LLVM=ON");
        return finish(false);
    }

    std::string target = cfg.target_triple.empty() ? detect_target() : cfg.target_triple;
    if (cfg.verbose) std::cout << "[codegen] target: " << target << " (LLVM)\n";

    xphage::codegen_llvm::LLVMConfig lcfg;
    lcfg.target_triple = target;
    lcfg.opt_level     = cfg.optimize ? 3 : 0;
    lcfg.verbose       = cfg.verbose;
    lcfg.emit_llvm_ir  = cfg.emit == EmitKind::LLVMir;
    lcfg.cpu           = "native";

    std::string obj_path = cfg.output_path + ".o";
    auto lr = xphage::codegen_llvm::compile_ast(prog, obj_path, lcfg);

    if (!lr.success) {
        res.errors.push_back("LLVM codegen: " + lr.error);
        return finish(false);
    }

    if (cfg.emit == EmitKind::LLVMir) {
        res.output_path = lr.llvm_ir_path;
        return finish(true);
    }
    if (cfg.emit == EmitKind::Object) {
        res.output_path = obj_path;
        return finish(true);
    }

    // Link to binary — find xprt library relative to compiler
    std::string xprt_lib;
    for (auto& p : {"./build/xprt/libxprt.a", "./xprt/libxprt.a", "./libxprt.a"}) {
        std::ifstream f(p);
        if (f.good()) { xprt_lib = p; break; }
    }

    bool linked = xphage::codegen_llvm::link_binary(
        {obj_path}, xprt_lib, cfg.output_path, cfg.verbose);
    if (!linked) {
        res.errors.push_back("Link failed. Object: " + obj_path);
        return finish(false);
    }

    res.output_path = cfg.output_path;
    return finish(true);
}

// Forward declaration — full definition (and the module-resolution
// rationale) lives further down, near parse_file(). compile() needs
// it before that point in the file.
static Program resolve_stdlib_links(const Program& prog,
                                     const std::string& configured_library_path,
                                     std::vector<std::string>& errors);

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
    xphage::lexer::Lexer lexer(src, cfg.source_path);
    auto tokens = lexer.tokenize();
    for (auto& e : lexer.errors())
        res.errors.push_back(cfg.source_path + ":" +
            std::to_string(e.line) + ":" + std::to_string(e.col) +
            ": error: " + e.message);
    if (lexer.has_errors()) return res;
    if (cfg.verbose)
        std::cout << "[lex] " << tokens.size() << " tokens\n";

    // 3. Parse
    xphage::parse::Parser parser(tokens, cfg.source_path);
    auto ast = parser.parse();
    for (auto& e : parser.errors())
        res.errors.push_back(cfg.source_path + ":" +
            std::to_string(e.line) + ":" + std::to_string(e.col) +
            ": error: " + e.message);
    if (parser.has_errors()) return res;
    if (cfg.verbose) std::cout << "[parse] " << ast.size() << " top-level nodes\n";

    // Resolve `~link "math"` etc. — same as compile_xp(), see there
    // for the full rationale.
    auto stdlib_decls = resolve_stdlib_links(ast, cfg.library_path, res.errors);
    if (!stdlib_decls.empty()) {
        ast.insert(ast.begin(), stdlib_decls.begin(), stdlib_decls.end());
    }
    if (!res.errors.empty()) return res;

    // 4. AST dump
    if (cfg.emit == EmitKind::AST) {
        std::string out_path = cfg.output_path + ".ast";
        std::ofstream out(out_path);
        out << "; X-Phage AST — " << cfg.source_path << "\n";
        std::function<void(const ASTNodePtr&,int)> dump =
            [&](const ASTNodePtr& n, int d) {
                if (!n) return;
                out << std::string(d*2,' ')
                    << "[" << (int)n->kind << "] " << n->value;
                if (!n->extra.empty())  out << " : " << n->extra;
                if (!n->extra2.empty()) out << " -> " << n->extra2;
                out << "\n";
                for (auto& c : n->children) dump(c, d+1);
            };
        for (auto& s : ast) dump(s, 0);
        res.success     = true;
        res.output_path = out_path;
        auto t1 = std::chrono::steady_clock::now();
        res.elapsed_ms  = std::chrono::duration<double,std::milli>(t1-t0).count();
        return res;
    }

    // ── Semantic Analysis ────────────────────────────────────────
    // See compile_xp() for the full rationale — same standalone
    // pass, same diagnostic formatting.
    {
        xphage::sema::SemanticAnalyzer analyzer(src);
        auto analysis = analyzer.analyze(ast);
        for (auto& d : analysis.diagnostics) {
            std::string source_line;
            if (!d.span.file.empty()) {
                std::ifstream f(d.span.file);
                std::string line;
                uint32_t ln = 1;
                while (std::getline(f, line)) {
                    if (ln == d.span.line) { source_line = line; break; }
                    ln++;
                }
            }
            std::string formatted = xphage::diag::format_diagnostic(d, source_line, true);
            if (d.severity == xphage::diag::Severity::Error) {
                res.errors.push_back(formatted);
            } else if (cfg.emit_warnings) {
                res.warnings.push_back(formatted);
            }
        }
        if (!analysis.ok) return res;
    }

    // ── Monomorphization ─────────────────────────────────────────
    // Runs after semantic analysis (which validated every generic
    // declaration's body once, generically, and bound-checked every
    // call site against its type parameters) and before IR lowering
    // — see monomorphize.hpp's own comment for the full rationale.
    // Replaces `ast` with a version containing zero generic
    // declarations: every generic pulse/forge/enum has become one
    // concrete specialization per distinct set of type arguments
    // actually used, and every call/construction site has been
    // rewritten to reference the right one. IR lowering and both
    // codegen backends need no generics-specific handling as a
    // result — they see exactly the same shape of program they
    // always have.
    {
        auto mono = xphage::generics::monomorphize(ast);
        ast = std::move(mono.program);
        for (auto& e : mono.errors) res.errors.push_back(e.message);
        if (!mono.errors.empty()) return res;
    }

    // 5. IR lowering
    std::string mod_name = cfg.source_path.substr(cfg.source_path.find_last_of("/\\")+1);
    auto ir_mod = xphage::middle::lower_to_ir(ast, mod_name);

    if (cfg.emit == EmitKind::IR) {
        std::string ir_text  = xphage::middle::dump_ir(ir_mod);
        std::string out_path = cfg.output_path + ".xpir";
        std::ofstream out(out_path); out << ir_text;
        res.success     = true;
        res.output_path = out_path;
        auto t1 = std::chrono::steady_clock::now();
        res.elapsed_ms  = std::chrono::duration<double,std::milli>(t1-t0).count();
        return res;
    }

    // 6. Code generation
    if (try_llvm_backend(ast, cfg, res, t0)) return res;

    std::string target = cfg.target_triple.empty() ? detect_target() : cfg.target_triple;
    if (cfg.verbose) std::cout << "[codegen] target: " << target << "\n";

    // ── XIL backend path ──────────────────────────────────────
    // Consumes the ir_mod already computed above (step 5) instead of
    // going through the AST-based transpiler. This is the real XIL
    // production path referenced during v3.5.0 stabilization — prior
    // to this wiring, lower_to_ir() was only ever reachable via
    // --emit=ir (a text dump for inspection), and transpile_ir() had
    // no caller that fed its output through actual C++ compilation.
    if (cfg.backend == Backend::XIL) {
        std::string cpp_out = cfg.output_path + "_xil_gen.cpp";
        xphage::codegen_transpiler::TranspilerConfig tcfg;
        tcfg.verbose = cfg.verbose;
        auto tr = xphage::codegen_transpiler::transpile_ir(ir_mod, cpp_out, tcfg);
        if (!tr.success) {
            res.errors.push_back("XIL transpiler: " + tr.error);
            return res;
        }

        if (cfg.emit == EmitKind::Transpiled) {
            res.success     = true;
            res.output_path = cpp_out;
            auto t1 = std::chrono::steady_clock::now();
            res.elapsed_ms  = std::chrono::duration<double,std::milli>(t1-t0).count();
            return res;
        }

#if defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE
        res.success     = true;
        res.output_path = cpp_out;
        if (cfg.emit_warnings)
            res.warnings.push_back("iOS sandbox: .cpp generated, cannot invoke compiler");
#else
        std::string cc;
#ifdef _WIN32
        cc = "g++ \"" + cpp_out + "\" -o \"" + cfg.output_path + ".exe\" -std=c++17 -pthread";
#else
        cc = "c++ \"" + cpp_out + "\" -o \"" + cfg.output_path + "\" -std=c++17 -pthread -lstdc++fs";
#endif
        if (cfg.optimize) cc += " -O3";
        for (auto& lib : cfg.link_libs) cc += " " + lib;
        if (cfg.verbose)  std::cout << "[cc] " << cc << "\n";

        if (std::system(cc.c_str()) != 0) {
            res.errors.push_back("C++ compilation failed. Generated source: " + cpp_out);
            return res;
        }
        res.success     = true;
        res.output_path = cfg.output_path;
#endif
        auto t1 = std::chrono::steady_clock::now();
        res.elapsed_ms  = std::chrono::duration<double,std::milli>(t1-t0).count();
        if (cfg.verbose)
            std::cout << "[interface] done (XIL) " << res.elapsed_ms << " ms\n";
        return res;
    }

    // ── Phase 1-3 Transpiler path ─────────────────────────────
    std::string cpp_out = cfg.output_path + "_gen.cpp";
    xphage::codegen_transpiler::TranspilerConfig tcfg;
    tcfg.verbose = cfg.verbose;
    auto tr = xphage::codegen_transpiler::transpile_ast(ast, cpp_out, tcfg);
    if (!tr.success) {
        res.errors.push_back("Transpiler: " + tr.error);
        return res;
    }

    if (cfg.emit == EmitKind::Transpiled) {
        res.success     = true;
        res.output_path = cpp_out;
        auto t1 = std::chrono::steady_clock::now();
        res.elapsed_ms  = std::chrono::duration<double,std::milli>(t1-t0).count();
        return res;
    }

#if defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE
    res.success     = true;
    res.output_path = cpp_out;
    if (cfg.emit_warnings)
        res.warnings.push_back("iOS sandbox: .cpp generated, cannot invoke compiler");
#else
    std::string cc;
#ifdef _WIN32
    cc = "g++ \"" + cpp_out + "\" -o \"" + cfg.output_path + ".exe\" -std=c++17 -pthread";
#else
    cc = "c++ \"" + cpp_out + "\" -o \"" + cfg.output_path + "\" -std=c++17 -pthread -lstdc++fs";
#endif
    if (cfg.optimize) cc += " -O3";
    for (auto& lib : cfg.link_libs) cc += " " + lib;
    if (cfg.verbose)  std::cout << "[cc] " << cc << "\n";

    if (std::system(cc.c_str()) != 0) {
        res.errors.push_back("C++ compilation failed. Generated source: " + cpp_out);
        return res;
    }
    res.success     = true;
    res.output_path = cfg.output_path;
#endif

    auto t1 = std::chrono::steady_clock::now();
    res.elapsed_ms  = std::chrono::duration<double,std::milli>(t1-t0).count();
    if (cfg.verbose)
        std::cout << "[interface] done " << res.elapsed_ms << " ms\n";
    return res;
}

int run_file(const std::string& path) {
    CompileConfig cfg;
    cfg.source_path = path;
    cfg.output_path = "/tmp/xp_run_" +
        std::to_string(std::hash<std::string>{}(path) & 0xFFFF);
    cfg.verbose     = false;
    cfg.optimize    = false;
    auto res = compile(cfg);
    if (!res.success) {
        for (auto& e : res.errors) std::cerr << e << "\n";
        return 1;
    }
#if defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE
    std::cout << "[iOS] Generated: " << res.output_path << "\n";
    return 0;
#else
    return std::system(res.output_path.c_str());
#endif
}

void start_repl() {
    std::cout << "\033[1;36m";
    std::cout << "  __  __    ____  _\n";
    std::cout << "  \\ \\/ /   |  _ \\| |__   __ _  __ _  ___\n";
    std::cout << "   \\  / _  | |_) | '_ \\ / _` |/ _` |/ _ \\\n";
    std::cout << "   /  \\ _| |  __/| | | | (_| | (_| |  __/\n";
    std::cout << "  /_/\\_\\   |_|   |_| |_|\\__,_|\\__, |\\___|\n";
    std::cout << "                               |___/\033[0m\n";
    std::cout << "\033[1;32mv4.0.0\033[0m  "
              << "\033[1;35mAeonCoreX Lab\033[0m";
    if (xphage::codegen_llvm::is_available())
        std::cout << "  \033[1;33m[LLVM " << xphage::codegen_llvm::llvm_version_str() << "]\033[0m";
    std::cout << "\n\033[1;36mType 'exit' to quit.\033[0m\n\n";

    std::string line;
    std::string session_src;

    while (true) {
        std::cout << "\033[1;32mxp> \033[0m";
        if (!std::getline(std::cin, line)) break;
        if (line == "exit" || line == "quit") break;
        if (line.empty()) continue;
        if (line == "help") {
            std::cout
                << "  :clear  — clear session\n"
                << "  :dump   — show accumulated source\n"
                << "  :run    — run accumulated source\n"
                << "  Any X-Phage expression/statement\n";
            continue;
        }
        if (line == ":clear")  { session_src.clear(); std::cout << "[repl] cleared\n"; continue; }
        if (line == ":dump")   { std::cout << session_src; continue; }
        if (line == ":run") {
            std::string tmp = "/tmp/xp_repl_sess.xp0";
            std::ofstream f(tmp); f << session_src; f.close();
            run_file(tmp); continue;
        }
        session_src += line + "\n";
        std::string tmp = "/tmp/xp_repl_line.xp0";
        { std::ofstream f(tmp); f << session_src; }
        xphage::lexer::Lexer lex(session_src, "<repl>");
        auto toks = lex.tokenize();
        if (lex.has_errors()) {
            for (auto& e : lex.errors())
                std::cerr << "\033[1;31m[err]\033[0m "
                          << e.line << ": " << e.message << "\n";
            continue;
        }
        xphage::parse::Parser p(toks, "<repl>");
        auto ast  = p.parse();
        if (p.has_errors()) {
            for (auto& e : p.errors())
                std::cerr << "\033[1;31m[err]\033[0m "
                          << e.line << ": " << e.message << "\n";
        } else {
            run_file(tmp);
        }
    }
    std::cout << "\033[1;33mGoodbye!\033[0m\n";
}

} // namespace xphage::interface

// ============================================================
// X-Phage .xp Single-File + Smart Multi-File Compilation v4.1.0
// Handles: .xp / .xp0 / .xh / .xui / mixed combinations
// AeonCoreX Lab
// ============================================================
#include "../include/section_detector.hpp"

namespace xphage::interface {

enum class FileLayer { Single, Execution, Logic, UI, Unknown };

static FileLayer detect_layer(const std::string& path) {
    std::string ext;
    auto dot = path.find_last_of('.');
    if (dot != std::string::npos) ext = path.substr(dot);
    if (ext == ".xp")  return FileLayer::Single;
    if (ext == ".xp0") return FileLayer::Execution;
    if (ext == ".xh")  return FileLayer::Logic;
    if (ext == ".xui") return FileLayer::UI;
    return FileLayer::Unknown;
}

static Program parse_file(const std::string& path, std::vector<std::string>& errors) {
    std::string src = read_file(path);
    if (src.empty()) { errors.push_back("Cannot open: " + path); return {}; }
    xphage::lexer::Lexer lexer(src, path);
    auto tokens = lexer.tokenize();
    for (auto& e : lexer.errors())
        errors.push_back(path + ":" + std::to_string(e.line) + ":" +
                         std::to_string(e.col) + ": error: " + e.message);
    if (lexer.has_errors()) return {};
    xphage::parse::Parser parser(tokens, path);
    auto ast = parser.parse();
    for (auto& e : parser.errors())
        errors.push_back(path + ":" + std::to_string(e.line) + ":" +
                         std::to_string(e.col) + ": " + e.message);
    return ast;
}

// ── Stdlib module resolution ───────────────────────────────────
// `~link "math"` must locate library/math/math.xh on disk, parse
// it the same way any source file is parsed, and merge its
// declarations (function signatures, forge types, atom constants)
// into the program being compiled — so that calling sqrt(x) after
// `~link "math"` resolves to a real signature instead of being an
// undeclared-identifier error from the C++ compiler.
//
// Resolution order for the library root, first match wins:
//   1. cfg.library_path, if the caller set one explicitly
//   2. $XPHAGE_HOME/library
//   3. <directory containing the running executable>/../library
//      (covers an installed layout: bin/xphage + library/ as
//      siblings, e.g. /usr/local/{bin,library})
//   4. ./library (relative to the current working directory —
//      convenient for running from a source checkout)
namespace fs = std::filesystem;

static std::string find_library_root(const std::string& configured) {
    if (!configured.empty() && fs::exists(configured)) return configured;

    if (const char* home = std::getenv("XPHAGE_HOME")) {
        fs::path p = fs::path(home) / "library";
        if (fs::exists(p)) return p.string();
    }

    // Executable-relative: /proc/self/exe on Linux. Best-effort —
    // if unavailable (non-Linux, sandboxed), this check simply
    // doesn't match and we fall through to the next option.
    std::error_code ec;
    fs::path exe = fs::read_symlink("/proc/self/exe", ec);
    if (!ec) {
        fs::path candidate = exe.parent_path().parent_path() / "library";
        if (fs::exists(candidate)) return candidate.string();
    }

    if (fs::exists("./library")) return "./library";

    return "";
}

// A module is loaded at most once per compile, even if `~link`ed
// from multiple files or accidentally repeated — re-parsing and
// re-merging the same .xh twice would otherwise produce duplicate
// forge/pulse declarations and a confusing "redefinition" error.
static Program resolve_module(const std::string& name,
                               const std::string& library_root,
                               std::unordered_set<std::string>& loaded,
                               std::vector<std::string>& errors) {
    if (loaded.count(name)) return {};
    loaded.insert(name);

    if (library_root.empty()) {
        errors.push_back("~link \"" + name + "\": stdlib library/ directory "
            "not found (set XPHAGE_HOME or run from a directory containing library/)");
        return {};
    }

    fs::path xh_path = fs::path(library_root) / name / (name + ".xh");
    if (!fs::exists(xh_path)) {
        errors.push_back("~link \"" + name + "\": no such stdlib module "
            "(expected " + xh_path.string() + ")");
        return {};
    }

    return parse_file(xh_path.string(), errors);
}

// Walks the program for every LinkStmt, resolves modules it
// recognises as stdlib names, and returns their merged
// declarations. Unrecognised ~link targets (e.g. "fusion-ui", a
// Spore package, or a bare path like "core/types" from a project's
// own multi-file layout) are left alone — only known stdlib module
// names are resolved here.
static const std::unordered_set<std::string>& stdlib_module_names() {
    static const std::unordered_set<std::string> names = {
        "io", "math", "string", "collections", "net", "os", "crypt", "ai", "solver"
    };
    return names;
}

static Program resolve_stdlib_links(const Program& prog,
                                     const std::string& configured_library_path,
                                     std::vector<std::string>& errors) {
    Program extra;
    std::unordered_set<std::string> loaded;
    std::string root; // computed lazily, only if a stdlib ~link is actually present
    bool root_computed = false;

    for (auto& n : prog) {
        if (!n || n->kind != NodeKind::LinkStmt) continue;
        if (!stdlib_module_names().count(n->value)) continue;

        if (!root_computed) {
            root = find_library_root(configured_library_path);
            root_computed = true;
        }
        auto module_decls = resolve_module(n->value, root, loaded, errors);
        extra.insert(extra.end(), module_decls.begin(), module_decls.end());
    }
    return extra;
}

ProjectLayerAnalysis analyze_xp_layers(const std::string& xp_path) {
    ProjectLayerAnalysis result;

    std::string src = read_file(xp_path);
    if (src.empty()) return result; // unreadable — treat as having nothing;
                                     // the real compile attempt will report
                                     // the actual "cannot open" error properly

    xphage::lexer::Lexer lexer(src, xp_path);
    auto tokens = lexer.tokenize();
    if (lexer.has_errors()) return result; // let the real compile surface this

    xphage::parse::Parser parser(tokens, xp_path);
    auto ast = parser.parse();
    if (parser.has_errors()) return result; // same — don't duplicate diagnostics here

    auto sectioned = SectionDetector::classify_all(ast);

    bool has_main_pulse = false;
    bool has_top_level_stmt = false;

    for (auto& sn : sectioned) {
        if (!sn.node) continue;

        if (sn.section == Section::Logic) result.has_logic = true;
        if (sn.section == Section::UI)    result.has_ui    = true;

        if (sn.node->kind == NodeKind::PulseDecl || sn.node->kind == NodeKind::AsyncPulseDecl) {
            if (!sn.node->value.empty()) result.declared_symbol_names.push_back(sn.node->value);
            if (sn.node->value == "main") {
                bool has_body = false;
                for (auto& c : sn.node->children)
                    if (c && c->kind == NodeKind::Block) { has_body = true; break; }
                if (has_body) has_main_pulse = true;
            }
        }
        if (sn.node->kind == NodeKind::ForgeDecl || sn.node->kind == NodeKind::NexusDecl) {
            if (!sn.node->value.empty()) result.declared_symbol_names.push_back(sn.node->value);
        }
        if (sn.section == Section::Execution &&
            sn.node->kind != NodeKind::PulseDecl && sn.node->kind != NodeKind::AsyncPulseDecl &&
            sn.node->kind != NodeKind::ImplDecl) {
            // A bare top-level statement (beam, atom assignment, etc.)
            // outside any function — this is the "implicit main()"
            // pattern, and counts as a real execution entry point the
            // same way an explicit `pulse main` does.
            has_top_level_stmt = true;
        }
    }

    result.has_execution = has_main_pulse || has_top_level_stmt;
    // Self-sufficient = has a real way to run (has_execution). Logic
    // and UI are optional layers — a program with no forge/nexus and
    // no UI at all (e.g. "beam \"hello\"") is still completely valid
    // and self-sufficient on its own.
    result.is_self_sufficient = result.has_execution;

    return result;
}

// Extracts just the declared forge/nexus/pulse names from any
// source file (used for a sibling module being considered for
// merge, not the main .xp — hence a separate, lighter-weight
// helper rather than reusing ProjectLayerAnalysis's full shape).
std::vector<std::string> declared_symbol_names_in_file(const std::string& path) {
    std::vector<std::string> errors; // discarded — a sibling with a parse
                                      // error simply contributes no names
                                      // here; the real compile attempt
                                      // (if we do end up merging it) will
                                      // surface the actual error properly
    auto ast = parse_file(path, errors);
    std::vector<std::string> names;
    for (auto& n : ast) {
        if (!n) continue;
        if ((n->kind == NodeKind::PulseDecl || n->kind == NodeKind::AsyncPulseDecl ||
             n->kind == NodeKind::ForgeDecl || n->kind == NodeKind::NexusDecl) &&
            !n->value.empty() && n->value != "main") {
            names.push_back(n->value);
        }
    }
    return names;
}

static void validate_layer(const Program& ast, FileLayer layer,
                            const std::string& path, std::vector<std::string>& warnings) {
    if (layer == FileLayer::Logic) {
        for (auto& n : ast) {
            if (!n) continue;
            switch (n->kind) {
                case NodeKind::BeamStmt: case NodeKind::BypassStmt:
                case NodeKind::QuantumStmt: case NodeKind::IfStmt:
                case NodeKind::WhileStmt: case NodeKind::ForStmt:
                case NodeKind::EmitStmt: case NodeKind::AbsorbStmt:
                    warnings.push_back("[warn] " + path + ":" + std::to_string(n->span.line) +
                        " — execution statement in .xh (Logic Layer)");
                    break;
                case NodeKind::FusionDecl: case NodeKind::StrandDecl:
                    warnings.push_back("[warn] " + path + ":" + std::to_string(n->span.line) +
                        " — UI declaration in .xh (Logic Layer). Use .xui");
                    break;
                default: break;
            }
        }
    }
    if (layer == FileLayer::UI) {
        for (auto& n : ast) {
            if (!n) continue;
            switch (n->kind) {
                case NodeKind::BeamStmt: case NodeKind::IfStmt:
                case NodeKind::WhileStmt: case NodeKind::ForStmt:
                    warnings.push_back("[warn] " + path + ":" + std::to_string(n->span.line) +
                        " — execution code in .xui (UI Layer). Use .xp0");
                    break;
                default: break;
            }
        }
    }
}

CompileResult compile_xp(const CompileConfig& cfg) {
    auto t0 = std::chrono::steady_clock::now();
    CompileResult res;

    std::string src = read_file(cfg.source_path);
    if (src.empty()) { res.errors.push_back("Cannot open: " + cfg.source_path); return res; }

    xphage::lexer::Lexer lexer(src, cfg.source_path);
    auto tokens = lexer.tokenize();
    for (auto& e : lexer.errors())
        res.errors.push_back(cfg.source_path + ":" + std::to_string(e.line) +
            ":" + std::to_string(e.col) + ": error: " + e.message);
    if (lexer.has_errors()) return res;
    xphage::parse::Parser parser(tokens, cfg.source_path);
    auto ast = parser.parse();
    for (auto& e : parser.errors())
        res.errors.push_back(cfg.source_path + ":" + std::to_string(e.line) +
            ":" + std::to_string(e.col) + ": error: " + e.message);
    if (parser.has_errors()) return res;

    // Resolve `~link "math"` etc. against the stdlib library/
    // directory and merge in the declarations they bring in scope,
    // before anything downstream (section split, transpile, IR
    // lowering) sees the program. A resolution failure (module not
    // found, library/ missing) is reported as a normal compile
    // error rather than silently ignored — calling an unresolved
    // stdlib function should fail clearly, not fall through to a
    // raw C++ "not declared in this scope".
    auto stdlib_decls = resolve_stdlib_links(ast, cfg.library_path, res.errors);
    if (!stdlib_decls.empty()) {
        ast.insert(ast.begin(), stdlib_decls.begin(), stdlib_decls.end());
    }
    if (!res.errors.empty()) return res;

    // ── Semantic Analysis ────────────────────────────────────────
    // A standalone pass (Parser → AST → Semantic Analyzer → Verified
    // AST → ...), run after parsing and stdlib resolution, before
    // any code generation. This is what catches undefined variables/
    // functions, arity mismatches, and duplicate declarations as
    // proper XPhage-level diagnostics — with the real source location
    // and a "did you mean" suggestion where possible — instead of
    // letting them through to the generated C++ and surfacing as a
    // g++ error pointing at internal, synthesized code.
    {
        xphage::sema::SemanticAnalyzer analyzer(src);
        auto analysis = analyzer.analyze(ast);
        for (auto& d : analysis.diagnostics) {
            std::string source_line;
            // Best-effort: read the actual offending line from the
            // file the diagnostic points at, for a rustc-style
            // snippet. This may be a different file than
            // cfg.source_path (e.g. a merged stdlib module or
            // sibling), which is exactly why the diagnostic's own
            // span.file — not cfg.source_path — is used here.
            if (!d.span.file.empty()) {
                std::ifstream f(d.span.file);
                std::string line;
                uint32_t ln = 1;
                while (std::getline(f, line)) {
                    if (ln == d.span.line) { source_line = line; break; }
                    ln++;
                }
            }
            std::string formatted = xphage::diag::format_diagnostic(d, source_line, true);
            if (d.severity == xphage::diag::Severity::Error) {
                res.errors.push_back(formatted);
            } else if (cfg.emit_warnings) {
                res.warnings.push_back(formatted);
            }
        }
        if (!analysis.ok) return res;
    }

    // ── Monomorphization ─────────────────────────────────────────
    // See compile()'s identical block for the full rationale.
    {
        auto mono = xphage::generics::monomorphize(ast);
        ast = std::move(mono.program);
        for (auto& e : mono.errors) res.errors.push_back(e.message);
        if (!mono.errors.empty()) return res;
    }

    if (cfg.verbose) {
        auto split_info = SectionDetector::classify_all(ast);
        int logic=0, ui=0, exec=0;
        for (auto& s : split_info) {
            if (s.section == Section::Logic)     logic++;
            else if (s.section == Section::UI)   ui++;
            else                                  exec++;
        }
        std::cout << "[xp split] Logic:" << logic << " UI:" << ui << " Execution:" << exec << "\n";
    }

    auto split = SectionDetector::split(ast);
    Program merged;
    merged.insert(merged.end(), split.logic.begin(),     split.logic.end());
    merged.insert(merged.end(), split.ui.begin(),        split.ui.end());
    merged.insert(merged.end(), split.execution.begin(), split.execution.end());

    if (cfg.emit == EmitKind::AST) {
        std::string out_path = cfg.output_path + ".ast";
        std::ofstream out(out_path);
        out << "; X-Phage AST — " << cfg.source_path << "\n";
        std::function<void(const ASTNodePtr&,int)> dump =
            [&](const ASTNodePtr& n, int d) {
                if (!n) return;
                out << std::string(d*2,' ')
                    << "[" << (int)n->kind << "] " << n->value;
                if (!n->extra.empty())  out << " : " << n->extra;
                if (!n->extra2.empty()) out << " -> " << n->extra2;
                out << "\n";
                for (auto& c : n->children) dump(c, d+1);
            };
        for (auto& s : merged) dump(s, 0);
        res.success     = true;
        res.output_path = out_path;
        auto t1 = std::chrono::steady_clock::now();
        res.elapsed_ms  = std::chrono::duration<double,std::milli>(t1-t0).count();
        return res;
    }

    std::string mod_name = cfg.source_path.substr(cfg.source_path.find_last_of("/\\") + 1);
    auto ir_mod = xphage::middle::lower_to_ir(merged, mod_name);

    if (cfg.emit == EmitKind::IR) {
        std::string txt = xphage::middle::dump_ir(ir_mod);
        std::string out = cfg.output_path + ".xpir";
        std::ofstream f(out); f << txt;
        res.success = true; res.output_path = out;
        auto t1 = std::chrono::steady_clock::now();
        res.elapsed_ms = std::chrono::duration<double,std::milli>(t1-t0).count();
        return res;
    }

    // Catch duplicate declarations BEFORE invoking the C++ compiler,
    // so the error is a clear X-Phage diagnostic, not a cryptic
    // "redefinition of struct X" from the generated C++.
    auto dup_errors = SectionDetector::check_duplicates(merged);
    if (!dup_errors.empty()) {
        for (auto& e : dup_errors) res.errors.push_back("error: " + e);
        return res;
    }

    if (try_llvm_backend(merged, cfg, res, t0)) return res;

    // ── XIL backend path (multi-file / Tri-Modular merge) ──────
    // Mirrors the Backend::XIL branch in compile_xp() above, reusing
    // the ir_mod already computed from the merged Program at this
    // call site (.xh + .xui + .xp0 combos, or split single-.xp files).
    if (cfg.backend == Backend::XIL) {
        std::string cpp_out = cfg.output_path + "_xil_gen.cpp";
        xphage::codegen_transpiler::TranspilerConfig tcfg2;
        tcfg2.verbose = cfg.verbose;
        auto trx = xphage::codegen_transpiler::transpile_ir(ir_mod, cpp_out, tcfg2);
        if (!trx.success) { res.errors.push_back("XIL transpiler: " + trx.error); return res; }

        if (cfg.emit == EmitKind::Transpiled) {
            res.success = true; res.output_path = cpp_out;
            auto t1 = std::chrono::steady_clock::now();
            res.elapsed_ms = std::chrono::duration<double,std::milli>(t1-t0).count();
            return res;
        }

#if defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE
        res.success = true; res.output_path = cpp_out;
        res.warnings.push_back("iOS: generated C++ at " + cpp_out);
#else
        std::string cc_x = "c++ \"" + cpp_out + "\" -o \"" + cfg.output_path
                       + "\" -std=c++17 -pthread -lstdc++fs";
        if (cfg.optimize) cc_x += " -O3";
        for (auto& lib : cfg.link_libs) cc_x += " " + lib;
        if (cfg.verbose) std::cout << "[cc] " << cc_x << "\n";
        if (std::system(cc_x.c_str()) != 0) {
            res.errors.push_back("C++ compilation failed. Check: " + cpp_out);
            return res;
        }
        res.success = true; res.output_path = cfg.output_path;
#endif
        auto t1 = std::chrono::steady_clock::now();
        res.elapsed_ms = std::chrono::duration<double,std::milli>(t1-t0).count();
        return res;
    }

    std::string cpp_out = cfg.output_path + "_gen.cpp";
    xphage::codegen_transpiler::TranspilerConfig tcfg;
    tcfg.verbose = cfg.verbose;
    auto tr = xphage::codegen_transpiler::transpile_ast(merged, cpp_out, tcfg);
    if (!tr.success) { res.errors.push_back("Transpiler: " + tr.error); return res; }

    if (cfg.emit == EmitKind::Transpiled) {
        res.success = true; res.output_path = cpp_out;
        auto t1 = std::chrono::steady_clock::now();
        res.elapsed_ms = std::chrono::duration<double,std::milli>(t1-t0).count();
        return res;
    }

#if defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE
    res.success = true; res.output_path = cpp_out;
    res.warnings.push_back("iOS: generated C++ at " + cpp_out);
#else
    std::string cc = "c++ \"" + cpp_out + "\" -o \"" + cfg.output_path
                   + "\" -std=c++17 -pthread -lstdc++fs";
    if (cfg.optimize) cc += " -O3";
    for (auto& lib : cfg.link_libs) cc += " " + lib;
    if (cfg.verbose) std::cout << "[cc] " << cc << "\n";
    if (std::system(cc.c_str()) != 0) {
        res.errors.push_back("C++ compilation failed. Check: " + cpp_out);
        return res;
    }
    res.success = true; res.output_path = cfg.output_path;
#endif

    auto t1 = std::chrono::steady_clock::now();
    res.elapsed_ms = std::chrono::duration<double,std::milli>(t1-t0).count();
    return res;
}

CompileResult compile_multi(const std::vector<std::string>& paths,
                             const CompileConfig& base_cfg) {
    auto t0 = std::chrono::steady_clock::now();
    CompileResult res;

    Program logic_nodes, ui_nodes, exec_nodes;

    for (auto& path : paths) {
        FileLayer layer = detect_layer(path);
        if (layer == FileLayer::Unknown) {
            res.warnings.push_back("[warn] Unknown file type: " + path + " — skipping");
            continue;
        }
        auto ast = parse_file(path, res.errors);
        if (!res.errors.empty()) return res;
        validate_layer(ast, layer, path, res.warnings);

        if (layer == FileLayer::Single) {
            auto split = SectionDetector::split(ast);
            logic_nodes.insert(logic_nodes.end(), split.logic.begin(), split.logic.end());
            ui_nodes.insert(ui_nodes.end(),        split.ui.begin(),    split.ui.end());
            exec_nodes.insert(exec_nodes.end(),    split.execution.begin(), split.execution.end());
        } else if (layer == FileLayer::Logic) {
            logic_nodes.insert(logic_nodes.end(), ast.begin(), ast.end());
        } else if (layer == FileLayer::UI) {
            ui_nodes.insert(ui_nodes.end(), ast.begin(), ast.end());
        } else {
            exec_nodes.insert(exec_nodes.end(), ast.begin(), ast.end());
        }
    }

    Program merged;
    merged.insert(merged.end(), logic_nodes.begin(), logic_nodes.end());
    merged.insert(merged.end(), ui_nodes.begin(),    ui_nodes.end());
    merged.insert(merged.end(), exec_nodes.begin(),  exec_nodes.end());

    if (base_cfg.verbose)
        std::cout << "[multi] L:" << logic_nodes.size()
                  << " UI:" << ui_nodes.size() << " E:" << exec_nodes.size() << " merged\n";

    xphage::codegen_transpiler::TranspilerConfig tcfg;
    tcfg.verbose = base_cfg.verbose;
    std::string cpp_out = base_cfg.output_path + "_gen.cpp";

    auto dup_errors = SectionDetector::check_duplicates(merged);
    if (!dup_errors.empty()) {
        for (auto& e : dup_errors) res.errors.push_back("error: " + e);
        return res;
    }

    // ── Semantic Analysis ────────────────────────────────────────
    // Same standalone pass as compile()/compile_xp() — run here on
    // the fully merged (Logic+UI+Execution) program, after the
    // structural duplicate check above but before any code
    // generation, so a Mixed-mode build gets the same quality of
    // diagnostics as a single-file one.
    {
        xphage::sema::SemanticAnalyzer analyzer; // no single source_text — this
                                                   // is a merge of possibly several
                                                   // files; per-diagnostic source
                                                   // lines are read from each
                                                   // diagnostic's own span.file below
        auto analysis = analyzer.analyze(merged);
        for (auto& d : analysis.diagnostics) {
            std::string source_line;
            if (!d.span.file.empty()) {
                std::ifstream f(d.span.file);
                std::string line;
                uint32_t ln = 1;
                while (std::getline(f, line)) {
                    if (ln == d.span.line) { source_line = line; break; }
                    ln++;
                }
            }
            std::string formatted = xphage::diag::format_diagnostic(d, source_line, true);
            if (d.severity == xphage::diag::Severity::Error) {
                res.errors.push_back(formatted);
            } else if (base_cfg.emit_warnings) {
                res.warnings.push_back(formatted);
            }
        }
        if (!analysis.ok) return res;
    }

    // ── Monomorphization ─────────────────────────────────────────
    // See compile()'s identical block for the full rationale.
    {
        auto mono = xphage::generics::monomorphize(merged);
        merged = std::move(mono.program);
        for (auto& e : mono.errors) res.errors.push_back(e.message);
        if (!mono.errors.empty()) return res;
    }

    if (try_llvm_backend(merged, base_cfg, res, t0)) return res;

    // ── XIL backend path ────────────────────────────────────────
    // This is the multi-file (.xh + .xui + .xp0) entry point used by
    // the golden test suite. ir_mod is computed lazily here (rather
    // than always, like compile_xp() does) since most callers use
    // the default AST-based transpiler and don't need it.
    if (base_cfg.backend == Backend::XIL) {
        std::string mod_name = base_cfg.output_path.substr(
            base_cfg.output_path.find_last_of("/\\") + 1);
        auto ir_mod = xphage::middle::lower_to_ir(merged, mod_name);

        if (base_cfg.emit == EmitKind::IR) {
            std::string txt = xphage::middle::dump_ir(ir_mod);
            std::string out = base_cfg.output_path + ".xpir";
            std::ofstream f(out); f << txt;
            res.success = true; res.output_path = out;
            auto t1 = std::chrono::steady_clock::now();
            res.elapsed_ms = std::chrono::duration<double,std::milli>(t1-t0).count();
            return res;
        }

        auto trx = xphage::codegen_transpiler::transpile_ir(ir_mod, cpp_out, tcfg);
        if (!trx.success) { res.errors.push_back("XIL transpiler: " + trx.error); return res; }

        if (base_cfg.emit == EmitKind::Transpiled) {
            res.success = true; res.output_path = cpp_out;
            auto t1 = std::chrono::steady_clock::now();
            res.elapsed_ms = std::chrono::duration<double,std::milli>(t1-t0).count();
            return res;
        }

#if !(defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE)
        std::string cc_x = "c++ \"" + cpp_out + "\" -o \"" + base_cfg.output_path
                       + "\" -std=c++17 -pthread -lstdc++fs";
        if (base_cfg.optimize) cc_x += " -O3";
        for (auto& lib : base_cfg.link_libs) cc_x += " " + lib;
        if (base_cfg.verbose) std::cout << "[cc] " << cc_x << "\n";
        if (std::system(cc_x.c_str()) != 0) {
            res.errors.push_back("C++ compilation failed. Check: " + cpp_out);
            return res;
        }
#endif
        res.success = true; res.output_path = base_cfg.output_path;
        auto t1 = std::chrono::steady_clock::now();
        res.elapsed_ms = std::chrono::duration<double,std::milli>(t1-t0).count();
        return res;
    }

    auto tr = xphage::codegen_transpiler::transpile_ast(merged, cpp_out, tcfg);
    if (!tr.success) { res.errors.push_back("Transpiler: " + tr.error); return res; }

    if (base_cfg.emit == EmitKind::Transpiled) {
        res.success = true; res.output_path = cpp_out;
        auto t1 = std::chrono::steady_clock::now();
        res.elapsed_ms = std::chrono::duration<double,std::milli>(t1-t0).count();
        return res;
    }

#if !(defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE)
    std::string cc = "c++ \"" + cpp_out + "\" -o \"" + base_cfg.output_path
                   + "\" -std=c++17 -pthread -lstdc++fs";
    if (base_cfg.optimize) cc += " -O3";
    for (auto& lib : base_cfg.link_libs) cc += " " + lib;
    if (base_cfg.verbose) std::cout << "[cc] " << cc << "\n";
    if (std::system(cc.c_str()) != 0) {
        res.errors.push_back("C++ compilation failed. Check: " + cpp_out);
        return res;
    }
#endif
    res.success = true; res.output_path = base_cfg.output_path;
    auto t1 = std::chrono::steady_clock::now();
    res.elapsed_ms = std::chrono::duration<double,std::milli>(t1-t0).count();
    return res;
}

SplitResult split_xp(const SplitConfig& cfg) {
    SplitResult res;
    std::vector<std::string> errs;
    auto ast = parse_file(cfg.source_path, errs);
    if (!errs.empty()) { res.error = errs[0]; return res; }

    auto split = SectionDetector::split(ast);
    res.logic_nodes = (int)split.logic.size();
    res.ui_nodes    = (int)split.ui.size();
    res.exec_nodes  = (int)split.execution.size();

    std::string base = cfg.source_path;
    auto dot = base.find_last_of('.');
    if (dot != std::string::npos) base = base.substr(0, dot);
    auto sep = base.find_last_of("/\\");
    std::string stem = (sep != std::string::npos) ? base.substr(sep+1) : base;

    res.xh_path  = cfg.output_dir + "/" + stem + ".xh";
    res.xui_path = cfg.output_dir + "/" + stem + ".xui";
    res.xp0_path = cfg.output_dir + "/" + stem + ".xp0";

    std::string xh_src  = SectionDetector::emit_xh(split.logic, cfg.source_path);
    std::string xui_src = SectionDetector::emit_xui(split.ui,    cfg.source_path);
    std::string xp0_src = SectionDetector::emit_xp0(split.execution, {stem}, cfg.source_path);

    if (cfg.dry_run || cfg.verbose) {
        std::cout << "\n\033[1;36m── .xh (Logic Layer) ──────────────────\033[0m\n" << xh_src
                  << "\n\033[1;36m── .xui (UI Layer) ───────────────────\033[0m\n" << xui_src
                  << "\n\033[1;36m── .xp0 (Execution Layer) ────────────\033[0m\n" << xp0_src;
    }

    if (!cfg.dry_run) {
        auto write_if = [](const std::string& path, const std::string& src) {
            std::ofstream f(path); f << src;
        };
        if (!split.logic.empty())     write_if(res.xh_path,  xh_src);
        if (!split.ui.empty())        write_if(res.xui_path, xui_src);
        if (!split.execution.empty()) write_if(res.xp0_path, xp0_src);

        std::cout << "\033[1;32m[split]\033[0m " << cfg.source_path << " → ";
        if (!split.logic.empty())     std::cout << stem << ".xh ";
        if (!split.ui.empty())        std::cout << stem << ".xui ";
        if (!split.execution.empty()) std::cout << stem << ".xp0";
        std::cout << "\n";
        std::cout << "  Logic nodes    : " << res.logic_nodes << "\n";
        std::cout << "  UI nodes       : " << res.ui_nodes    << "\n";
        std::cout << "  Execution nodes: " << res.exec_nodes  << "\n";
    }
    res.success = true;
    return res;
}

} // namespace xphage::interface
