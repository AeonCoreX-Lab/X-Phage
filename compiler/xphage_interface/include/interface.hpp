#pragma once
// ============================================================
// xphage_interface — Public Compiler API v3.5.0
// Drives the full pipeline: source → binary
// ============================================================
#include <string>
#include <vector>

namespace xphage::interface {

enum class Backend  { LLVM, Transpiler };
enum class EmitKind { Binary, Object, IR, AST, Transpiled };

struct CompileConfig {
    std::string  source_path;
    std::string  output_path    = "output";
    std::string  target_triple;
    Backend      backend        = Backend::LLVM;
    EmitKind     emit           = EmitKind::Binary;
    bool         optimize       = true;
    bool         debug_info     = false;
    bool         verbose        = false;
    std::vector<std::string> include_paths;
    std::vector<std::string> lib_paths;
};

struct CompileResult {
    bool        success  = false;
    std::string output_path;
    std::vector<std::string> errors;
    std::vector<std::string> warnings;
    double      elapsed_ms = 0.0;
};

CompileResult compile(const CompileConfig& cfg);
int           run_file(const std::string& path);
void          start_repl();

} // namespace xphage::interface
