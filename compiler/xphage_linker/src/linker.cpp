// ============================================================
// xphage_linker v4.0.0
// stdlib module linking for X-Phage
// Note: XPM is a separate repo — zero dependency here
// AeonCoreX Lab
// ============================================================
#include "../include/linker.hpp"
#include "xphage/runtime.hpp"
#include <iostream>
#include <unordered_map>

namespace xphage::linker {

// ── Stdlib module descriptor ──────────────────────────────────
struct StdlibModule {
    std::string link_flag;   // -lssl, -lcurl, etc.
    std::string header_hint; // which header to include in generated code
    std::string note;
};

static const std::unordered_map<std::string, StdlibModule> STDLIB_MODULES = {
    { "io",          { "", "<filesystem>", "file I/O, proc, env, glob" } },
    { "math",        { "-lm", "<cmath>", "sin/cos/sqrt/pow/log/Vec/Mat" } },
    { "string",      { "", "<string>", "str_split/join/trim/regex" } },
    { "collections", { "", "<vector>", "Option/Result/Vec/Map/Set" } },
    { "net",         { "-lcurl", "<curl/curl.h>", "http_get/post/WebSocket" } },
    { "os",          { "-lpthread", "<thread>", "thread_spawn/mutex/signals" } },
    { "crypt",       { "-lssl -lcrypto", "<openssl/sha.h>", "sha256/aes/argon2/uuid" } },
    { "ai",          { "", "<vector>", "Tensor/nn_linear/llm_load" } },
    { "fusion-ui",   { "", "xphage/fusion.hpp", "Fusion GPU UI framework" } },
};

std::vector<std::string> resolve_link_flags(const std::vector<std::string>& imports) {
    std::vector<std::string> flags;
    for (auto& imp : imports) {
        auto it = STDLIB_MODULES.find(imp);
        if (it == STDLIB_MODULES.end()) continue;
        if (!it->second.link_flag.empty())
            flags.push_back(it->second.link_flag);
    }
    return flags;
}

std::vector<std::string> resolve_headers(const std::vector<std::string>& imports) {
    std::vector<std::string> headers;
    for (auto& imp : imports) {
        auto it = STDLIB_MODULES.find(imp);
        if (it == STDLIB_MODULES.end()) continue;
        if (!it->second.header_hint.empty())
            headers.push_back(it->second.header_hint);
    }
    return headers;
}

void list_stdlib_modules() {
    std::cout << "\n  X-Phage Standard Library Modules\n";
    std::cout << "  ─────────────────────────────────\n";
    for (auto& [name, mod] : STDLIB_MODULES) {
        std::cout << "  ~link \"" << name << "\"\n"
                  << "    " << mod.note << "\n"
                  << "    link: " << (mod.link_flag.empty() ? "(none)" : mod.link_flag) << "\n\n";
    }
}

} // namespace xphage::linker

// ── Legacy XPhageLinker bridge ─────────────────────────────────
void XPhageLinker::link_library(const std::string& lib_name,
                                 XPhageRuntime& runtime) {
    runtime.write_global("__linked_" + lib_name, "1");
    std::cout << "[linker] ~link \"" << lib_name << "\" resolved\n";
}
