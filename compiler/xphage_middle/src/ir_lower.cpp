// ============================================================
// xphage_middle — IR Lowering v3.5.0
// AST → xphage::ir::IRModule
// ============================================================
#include "xphage/ast.hpp"
#include "xphage/ir.hpp"
#include <string>
#include <vector>
#include <memory>
#include <stdexcept>

namespace xphage::middle {

using namespace xphage::ir;

// ── Register allocator (simple counter) ─────────────────────
struct RegAlloc {
    int next = 0;
    IRValue fresh(std::string type = "str") {
        return IRValue::reg("r" + std::to_string(next++) );
    }
};

// ── IR builder ───────────────────────────────────────────────
class IRLowering {
public:
    IRModule lower(const Program& prog, const std::string& mod_name = "module") {
        IRModule mod;
        mod.name = mod_name;

        for (auto& node : prog) {
            if (!node) continue;
            if (node->kind == NodeKind::PulseDecl) {
                mod.functions.push_back(lower_pulse(*node));
            } else if (node->kind == NodeKind::GlobalDecl) {
                mod.globals.emplace_back(node->value, node->extra);
            }
        }
        return mod;
    }

private:
    RegAlloc regs_;

    IRFunction lower_pulse(const ASTNode& pulse) {
        IRFunction fn;
        fn.name = pulse.value.empty() ? "main" : pulse.value;

        IRBasicBlock entry;
        entry.label = "entry";

        if (!pulse.children.empty() && pulse.children[0]) {
            lower_block(*pulse.children[0], entry);
        }

        // Return
        IRInstr ret;
        ret.op      = Opcode::Return;
        ret.dest    = IRValue::imm("0", "i64");
        entry.instrs.push_back(ret);

        fn.blocks.push_back(std::move(entry));
        return fn;
    }

    void lower_block(const ASTNode& block, IRBasicBlock& bb) {
        for (auto& child : block.children) {
            if (!child) continue;
            lower_node(*child, bb);
        }
    }

    void lower_node(const ASTNode& node, IRBasicBlock& bb) {
        switch (node.kind) {

        case NodeKind::BeamStmt: {
            IRInstr ins;
            ins.op   = Opcode::Print;
            ins.dest = IRValue::imm("", "void");
            if (!node.children.empty() && node.children[0])
                ins.operands.push_back(IRValue::imm(node.children[0]->value));
            else
                ins.operands.push_back(IRValue::imm(node.value));
            bb.instrs.push_back(ins);
            break;
        }

        case NodeKind::AtomDecl:
        case NodeKind::ShadowDecl: {
            IRInstr alloc;
            alloc.op   = Opcode::Alloc;
            alloc.dest = IRValue::reg(node.value);
            if (!node.children.empty() && node.children[0])
                alloc.operands.push_back(IRValue::imm(node.children[0]->value));
            bb.instrs.push_back(alloc);
            break;
        }

        case NodeKind::GlobalDecl: {
            IRInstr store;
            store.op   = Opcode::Store;
            store.dest = IRValue::reg(node.value);
            store.operands.push_back(IRValue::imm(node.extra));
            bb.instrs.push_back(store);
            break;
        }

        case NodeKind::BypassStmt: {
            IRInstr bypass;
            bypass.op   = Opcode::Bypass;
            bypass.dest = IRValue::imm("", "void");
            bypass.operands.push_back(IRValue::imm(node.value));
            if (!node.children.empty() && node.children[0]) {
                for (auto& pair : node.children[0]->children) {
                    if (pair)
                        bypass.operands.push_back(
                            IRValue::imm(pair->value + "=" + pair->extra));
                }
            }
            bb.instrs.push_back(bypass);
            break;
        }

        case NodeKind::QuantumStmt: {
            IRInstr q;
            q.op   = Opcode::Quantum;
            q.dest = IRValue::imm("", "void");
            q.operands.push_back(IRValue::imm(node.value));
            bb.instrs.push_back(q);
            break;
        }

        case NodeKind::VortexStmt: {
            IRInstr v;
            v.op   = Opcode::Vortex;
            v.dest = IRValue::imm("", "void");
            bb.instrs.push_back(v);
            break;
        }

        case NodeKind::VoidStmt: {
            IRInstr v;
            v.op   = Opcode::VoidProtocol;
            v.dest = IRValue::imm("", "void");
            bb.instrs.push_back(v);
            break;
        }

        case NodeKind::ChronosStmt: {
            IRInstr c;
            c.op   = Opcode::Chronos;
            c.dest = IRValue::imm("", "void");
            c.operands.push_back(IRValue::imm(node.value, "i64"));
            bb.instrs.push_back(c);
            break;
        }

        case NodeKind::EtherStmt: {
            IRInstr e;
            e.op   = Opcode::Ether;
            e.dest = IRValue::imm("", "void");
            e.operands.push_back(IRValue::imm(node.value));
            e.operands.push_back(IRValue::imm(node.extra));
            bb.instrs.push_back(e);
            break;
        }

        case NodeKind::LinkStmt: {
            IRInstr lnk;
            lnk.op   = Opcode::LinkLib;
            lnk.dest = IRValue::imm("", "void");
            lnk.operands.push_back(IRValue::imm(node.value));
            bb.instrs.push_back(lnk);
            break;
        }

        case NodeKind::SynapseStmt: {
            IRInstr s;
            s.op   = Opcode::SynapseLink;
            s.dest = IRValue::imm("", "void");
            s.operands.push_back(IRValue::imm(node.value));
            s.operands.push_back(IRValue::imm(node.extra));
            bb.instrs.push_back(s);
            break;
        }

        case NodeKind::MatrixStmt: {
            IRInstr m;
            m.op   = Opcode::GpuCompute;
            m.dest = IRValue::reg(node.value);
            m.operands.push_back(IRValue::imm(node.extra, "i64"));
            bb.instrs.push_back(m);
            break;
        }

        case NodeKind::Block: {
            lower_block(node, bb);
            break;
        }

        default:
            break;
        }
    }
};

// ── Public API ───────────────────────────────────────────────
IRModule lower_to_ir(const Program& prog, const std::string& mod_name) {
    IRLowering lowering;
    return lowering.lower(prog, mod_name);
}

// Textual IR dump (for --emit=ir)
std::string dump_ir(const IRModule& mod) {
    std::string out;
    out += "; X-Phage IR — module: " + mod.name + "\n\n";

    for (auto& [name, val] : mod.globals)
        out += "@" + name + " = \"" + val + "\"\n";
    if (!mod.globals.empty()) out += "\n";

    for (auto& fn : mod.functions) {
        out += "fn " + fn.name + "() {\n";
        for (auto& bb : fn.blocks) {
            out += bb.label + ":\n";
            for (auto& ins : bb.instrs) {
                out += "  ";
                if (!ins.dest.repr.empty())
                    out += ins.dest.repr + " = ";
                out += std::to_string((int)ins.op);
                for (auto& op : ins.operands)
                    out += " " + op.repr;
                out += "\n";
            }
        }
        out += "}\n\n";
    }
    return out;
}

} // namespace xphage::middle
