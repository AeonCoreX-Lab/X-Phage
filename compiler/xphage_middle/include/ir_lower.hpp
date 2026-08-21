#pragma once
// ============================================================
// xphage_middle — IR Lowering v4.0.0
// AST → X-Phage Intermediate Representation (XPIR)
// AeonCoreX Lab
// ============================================================
#include "xphage/ast.hpp"
#include "xphage/ir.hpp"

namespace xphage::middle {

// Lower a full Program AST into an IRModule
xphage::ir::IRModule lower_to_ir(const Program& ast,
                                  const std::string& module_name = "xp_module");

// Dump IR as human-readable text (for --emit=ir)
std::string dump_ir(const xphage::ir::IRModule& mod);

} // namespace xphage::middle
