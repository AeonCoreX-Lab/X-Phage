#pragma once
// ============================================================
// xphage_linker — Symbol Linker + Module Resolver v3.5.0
//
// Responsibilities:
//   • Resolve ~link directives to .xh files on disk
//   • Parse .xh files: globals, #defines, ~hook, pulse stubs
//   • Trigger GPU/NPU hooks when hardware decorators are found
//   • Auto-fetch missing modules via XPM_Cloud
// ============================================================
#include "xphage/runtime.hpp"
#include <string>
#include <vector>

namespace xphage::linker {

// ── Link resolution search order ─────────────────────────────
// 1. Current working directory
// 2. modules/<name>/<name>.xh  (XPM downloaded)
// 3. library/core/xh/<name>.xh
// 4. library/std/xh/<category>/<name>.xh
// 5. library/alloc/xh/<name>.xh
// 6. XPHAGE_STDLIB env var / install prefix
// 7. XPM cloud auto-fetch (if network available)

struct LinkConfig {
    std::vector<std::string> search_dirs;  // additional search paths
    bool   auto_fetch = true;              // XPM cloud fetch on miss
    bool   verbose    = false;
};

struct LinkResult {
    bool        found    = false;
    std::string resolved_path;
    std::string error;
};

// Resolve a module name/path to an absolute file path.
LinkResult resolve_module(const std::string& spec,
                          const LinkConfig& cfg = {});

// Load and process a resolved .xh file into the runtime.
bool load_module(const std::string& path,
                 XPhageRuntime& runtime,
                 bool verbose = false);

} // namespace xphage::linker

// ── Legacy class interface (used by existing .cpp files) ──────
class XPhageLinker {
public:
    void link_library(std::string lib_name, XPhageRuntime& runtime);
};
