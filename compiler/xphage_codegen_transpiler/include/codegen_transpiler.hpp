#pragma once
// ============================================================
// xphage_codegen_transpiler — C++ Transpiler Backend v3.5.0
//
// Converts the X-Phage token stream to portable C++17 source.
// The generated source is then compiled by the host's clang++/g++.
//
// Use cases:
//   • Mobile platforms (iOS, Android) where LLVM is unavailable
//   • Embedded targets without LLVM backend support
//   • Quick prototyping (no LLVM install needed)
//   • Cross-compilation via host C++ toolchain
// ============================================================
#include "xphage/runtime.hpp"
#include "xphage/ir.hpp"
#include <string>
#include <vector>

namespace xphage::codegen_transpiler {

struct TranspilerConfig {
    std::string cpp_standard  = "c++17";
    bool        emit_comments = true;   // include origin comments in output
    bool        emit_debug    = false;  // emit #line directives
    bool        minimize      = false;  // strip whitespace from output
    bool        verbose       = false;
};

struct TranspilerResult {
    bool        success    = false;
    std::string output_path;
    std::string error;
    size_t      lines_emitted = 0;
};

// Transpile token stream → C++ source file.
TranspilerResult transpile_tokens(const std::vector<Token>& tokens,
                                   const std::string& output_cpp,
                                   const TranspilerConfig& cfg = {});

// Transpile IR module → C++ source file.
TranspilerResult transpile_ir(const xphage::ir::IRModule& mod,
                               const std::string& output_cpp,
                               const TranspilerConfig& cfg = {});

// Return the C++ boilerplate header inserted in every transpiled file.
std::string boilerplate_header();

} // namespace xphage::codegen_transpiler

// ── Legacy class interface ────────────────────────────────────
class XPhageTranspiler {
public:
    void transpile_to_cpp(const std::vector<Token>& tokens,
                          std::string output_cpp);
};
