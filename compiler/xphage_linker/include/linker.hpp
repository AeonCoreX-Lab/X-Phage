#pragma once
// ============================================================
// xphage_linker — Stdlib Module Linker v4.0.0
// ============================================================
#include <string>
#include <vector>

namespace xphage::linker {

std::vector<std::string> resolve_link_flags(const std::vector<std::string>& imports);
std::vector<std::string> resolve_headers(const std::vector<std::string>& imports);
void                     list_stdlib_modules();

} // namespace xphage::linker
