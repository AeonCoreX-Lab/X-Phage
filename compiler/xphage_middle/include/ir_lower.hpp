#pragma once
#include "xphage/ast.hpp"
#include "xphage/ir.hpp"
#include <string>

namespace xphage::middle {
xphage::ir::IRModule lower_to_ir(const Program& prog,
                                  const std::string& mod_name = "module");
std::string           dump_ir(const xphage::ir::IRModule& mod);
} // namespace xphage::middle
