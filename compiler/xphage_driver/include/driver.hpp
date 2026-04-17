#pragma once
// ============================================================
// xphage_driver — CLI Driver Header v3.5.0
//
// Parses command-line arguments and dispatches to:
//   • xphage::interface::compile()
//   • xphage::interface::run_file()
//   • xphage::interface::start_repl()
//   • XPM_Cloud package management
// ============================================================
#include "xphage/runtime.hpp"
#include <string>
#include <vector>

namespace xphage::driver {

// ── Parsed CLI arguments ──────────────────────────────────────
struct CliArgs {
    std::string command;       // "run" | "build" | "init" | "install" | ...
    std::string source_file;   // for run/build
    std::string output;        // -o <path>
    std::string target;        // --target <triple>
    std::string backend;       // "llvm" | "transpiler"
    std::string emit;          // "binary" | "ir" | "ast" | "transpiled"

    // Package management
    std::string pkg_spec;      // for install: "name[@ver]"
    std::string registry_sub;  // list | add | remove | default
    std::string registry_name;
    std::string registry_url;

    // Flags
    bool optimize   = true;
    bool debug_info = false;
    bool verbose    = false;
    bool help       = false;
    bool version    = false;

    std::vector<std::string> extra;
};

// Parse argv into CliArgs. Returns false and prints error on failure.
bool parse_args(int argc, char* argv[], CliArgs& out);

// Dispatch the parsed command. Returns process exit code.
int dispatch(const CliArgs& args);

// Print usage information
void print_usage();
void print_version();

} // namespace xphage::driver
