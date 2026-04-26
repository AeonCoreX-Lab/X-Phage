#pragma once
// ============================================================
// X-Phage IR (Intermediate Representation) v3.5.0
// Compiler stage: AST → IR → Codegen
// ============================================================
#include <string>
#include <vector>
#include <memory>

namespace xphage::ir {

enum class Opcode {
    // Memory
    Alloc, Load, Store, Free,
    // Arithmetic
    Add, Sub, Mul, Div, Mod,
    // Control flow
    Jump, Branch, Call, Return,
    // I/O
    Print, Read,
    // X-Phage runtime ops
    Bypass, Quantum, Ether, VoidProtocol, Vortex,
    NeuralSync, GpuCompute, Chronos,
    LinkLib, TorrentStart, SynapseLink,
};

struct IRValue {
    enum class Kind { Immediate, Reg, Label, GlobalRef };
    Kind        kind = Kind::Immediate;
    std::string repr;
    std::string type; // "i64", "f64", "str", "void"

    static IRValue imm(std::string v, std::string t = "str")
        { return {Kind::Immediate, std::move(v), std::move(t)}; }
    static IRValue reg(std::string r)
        { return {Kind::Reg, std::move(r), ""}; }
    static IRValue lbl(std::string l)
        { return {Kind::Label, std::move(l), ""}; }
};

struct IRInstr {
    Opcode              op;
    IRValue             dest;
    std::vector<IRValue> operands;
    std::string         comment;
};

struct IRBasicBlock {
    std::string          label;
    std::vector<IRInstr> instrs;
};

struct IRFunction {
    std::string                 name;
    std::vector<std::string>    params;
    std::vector<IRBasicBlock>   blocks;
};

struct IRModule {
    std::string                                      name;
    std::vector<IRFunction>                          functions;
    std::vector<std::pair<std::string,std::string>>  globals;
};

} // namespace xphage::ir
