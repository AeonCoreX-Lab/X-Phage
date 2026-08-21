#pragma once
// ============================================================
// xphage_codegen_transpiler — C++ Transpiler Backend v4.0.0
// AST → portable C++17 source
// AeonCoreX Lab
// ============================================================
#include "xphage/ast.hpp"
#include "xphage/ir.hpp"
#include <string>
#include <vector>

namespace xphage::codegen_transpiler {

struct TranspilerConfig {
    std::string cpp_standard  = "c++17";
    bool        emit_comments = true;
    bool        emit_debug    = false;
    bool        minimize      = false;
    bool        verbose       = false;
};

struct TranspilerResult {
    bool        success       = false;
    std::string output_path;
    std::string error;
    size_t      lines_emitted = 0;
};

// Primary: transpile AST → C++17 source
TranspilerResult transpile_ast(const Program& ast,
                                const std::string& output_cpp,
                                const TranspilerConfig& cfg = {});

// Legacy: token-stream → minimal C++ shell
TranspilerResult transpile_tokens(const std::vector<Token>& tokens,
                                   const std::string& output_cpp,
                                   const TranspilerConfig& cfg = {});

// Transpile IR module → C++ source
TranspilerResult transpile_ir(const xphage::ir::IRModule& mod,
                               const std::string& output_cpp,
                               const TranspilerConfig& cfg = {});

std::string boilerplate_header();

} // namespace xphage::codegen_transpiler
