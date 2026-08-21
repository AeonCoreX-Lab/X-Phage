#pragma once
// ============================================================
// xphage_interface — Compiler Pipeline v4.0.0
// Source → Lex → Parse → IR → Codegen → Binary
// AeonCoreX Lab
// ============================================================
#include "xphage/ast.hpp"
#include <string>
#include <vector>

namespace xphage::interface {

enum class Backend  { Transpiler, LLVM, XIL };
enum class EmitKind { Binary, Transpiled, AST, IR, LLVMir, Object };

struct CompileConfig {
    std::string source_path;
    std::string output_path   = "a.out";
    std::string target_triple;          // empty = auto-detect
    Backend     backend       = Backend::Transpiler;
    EmitKind    emit          = EmitKind::Binary;
    bool        optimize      = false;
    bool        verbose       = false;
    bool        emit_warnings = true;
    // Extra linker arguments for FFI / native dependencies, e.g.
    // {"-L/path/to/libs", "-lnative_demo"} or a direct .a/.so path
    // such as "/path/to/libnative_demo.a". Passed through verbatim
    // to the final C++ link command (transpiler backend) or to the
    // LLVM backend's link_binary step.
    std::vector<std::string> link_libs;
    // Root directory containing the stdlib modules (io/, math/,
    // string/, etc.), each as <library_path>/<name>/<name>.xh.
    // Resolution order if empty: $XPHAGE_HOME/library, then a
    // path relative to the running executable, then ./library.
    std::string library_path;
};

struct CompileResult {
    bool                     success    = false;
    std::string              output_path;
    std::vector<std::string> errors;
    std::vector<std::string> warnings;
    double                   elapsed_ms = 0.0;
};

// ── AST-based project layer analysis ──────────────────────────
// Used by the driver's project-discovery logic to decide, *before*
// attempting any merge, whether a .xp file is self-sufficient or
// genuinely needs a sibling module. This replaces a purely
// filename-based "if main.xui exists, merge it" rule (which breaks
// when a .xp and a same-named Tri-Modular set both happen to exist
// for unrelated reasons — e.g. two independent examples of the same
// program) with a semantic one: does the .xp's own AST already
// have a real Logic declaration, a real UI declaration, and a real
// Execution entry point?
struct ProjectLayerAnalysis {
    bool has_logic     = false;  // any ForgeDecl/NexusDecl/non-body PulseDecl/etc.
    bool has_ui        = false;  // any FusionDecl/StrandDecl
    bool has_execution = false;  // a real entry point (pulse main, or any
                                  // top-level statement outside a function)
    bool is_self_sufficient = false; // has_logic-or-n/a AND has_execution
                                      // (UI is optional — not every program
                                      // has a UI layer at all)
    std::vector<std::string> declared_symbol_names; // forge/nexus/pulse names,
                                                      // for duplicate-symbol
                                                      // checks against a
                                                      // candidate sibling
                                                      // before merging
};
ProjectLayerAnalysis analyze_xp_layers(const std::string& xp_path);

// Extracts just the declared forge/nexus/pulse names from any
// source file — used by the driver's discovery logic to check a
// candidate sibling module for symbol-name overlap with a .xp's
// own declared symbols before deciding it's safe to merge them.
std::vector<std::string> declared_symbol_names_in_file(const std::string& path);

CompileResult compile(const CompileConfig& cfg);
int           run_file(const std::string& path);
void          start_repl();

// ── Single-file .xp support ───────────────────────────────────
CompileResult compile_xp(const CompileConfig& cfg);

// Split .xp file into .xh + .xui + .xp0 on disk
struct SplitConfig {
    std::string source_path;
    std::string output_dir  = ".";
    bool        dry_run     = false;
    bool        verbose     = false;
};
struct SplitResult {
    bool        success = false;
    std::string xh_path, xui_path, xp0_path;
    std::string error;
    int         logic_nodes = 0, ui_nodes = 0, exec_nodes = 0;
};
SplitResult split_xp(const SplitConfig& cfg);

// Smart multi-file compile (.xh + .xui + .xp0 / .xp in any combination)
CompileResult compile_multi(const std::vector<std::string>& paths,
                             const CompileConfig& base_cfg);

} // namespace xphage::interface
