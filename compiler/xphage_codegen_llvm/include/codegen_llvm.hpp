#pragma once
// ============================================================
// xphage_codegen_llvm — LLVM Native Backend Header v3.5.0
//
// Converts X-Phage token stream (or IR) to LLVM IR,
// then emits a native object file via the LLVM TargetMachine.
//
// Platform support:
//   x86-64 (linux, windows, macos)
//   aarch64 (linux arm64, macos apple silicon, windows arm64)
//
// LLVM version compatibility:
//   Tested: LLVM 16, 17, 18, 19, 20, 21
//   Version-gated includes/API for breaking changes per major.
// ============================================================
#include "xphage/runtime.hpp"
#include "xphage/ir.hpp"
#include <string>
#include <vector>

namespace xphage::codegen_llvm {

struct LLVMCodegenConfig {
    std::string  target_triple;   // "" = host default
    std::string  cpu             = "generic";
    std::string  features;        // "+avx2,+fma" etc.
    bool         optimize        = true;
    bool         debug_info      = false;
    bool         pic             = false;     // position-independent code
    bool         verbose         = false;
};

struct LLVMCodegenResult {
    bool        success      = false;
    std::string output_path;
    std::string error;
};

// Compile a token stream to a native object file.
LLVMCodegenResult compile_tokens(const std::vector<Token>& tokens,
                                  const std::string& output_obj,
                                  const LLVMCodegenConfig& cfg = {});

// Compile an IR module to a native object file.
LLVMCodegenResult compile_ir(const xphage::ir::IRModule& mod,
                               const std::string& output_obj,
                               const LLVMCodegenConfig& cfg = {});

// Check whether LLVM was compiled in at build time.
bool is_available();

// Return the LLVM version string, or "disabled" if not compiled.
std::string llvm_version();

} // namespace xphage::codegen_llvm

// ── Legacy class interface ────────────────────────────────────
class XPhageLLVMCompiler {
public:
    void compile_tokens(const std::vector<Token>& tokens,
                        std::string output_obj);
};
