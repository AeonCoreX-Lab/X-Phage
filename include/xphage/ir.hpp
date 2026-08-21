#pragma once
// ============================================================
// X-Phage IR — Intermediate Representation v4.0.0
// Sits between AST and final codegen (transpiler / LLVM)
// AeonCoreX Lab
// ============================================================
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

namespace xphage::ir {

// ── IR Value types ────────────────────────────────────────────
enum class IRType {
    Void, I64, F64, Bool, String,
    Ptr, Array, Struct, Func, Auto
};

inline std::string ir_type_str(IRType t) {
    switch (t) {
        case IRType::Void:   return "void";
        case IRType::I64:    return "i64";
        case IRType::F64:    return "f64";
        case IRType::Bool:   return "i1";
        case IRType::String: return "str";
        case IRType::Ptr:    return "ptr";
        case IRType::Array:  return "arr";
        case IRType::Struct: return "struct";
        case IRType::Func:   return "fn";
        default:             return "auto";
    }
}

// ── IR Instruction kinds ──────────────────────────────────────
enum class IROpcode {
    // Arithmetic
    Add, Sub, Mul, Div, Mod,
    // Comparison
    Eq, Ne, Lt, Le, Gt, Ge,
    // Logical
    And, Or, Not,
    // Control flow
    Br, BrCond, Ret, Call, TailCall,
    // Memory
    Alloca, Load, Store, GEP,
    // Takes the address of a value register (operands[0]) — used by
    // a method-call lowering (obj.method(args)) to obtain the pointer
    // a real method's implicit `self` parameter expects, since XIL's
    // implicit-self is a genuine pointer parameter (unlike the AST
    // transpiler's reference-based `auto& self = *this`). Distinct
    // from GEP (field access) and Alloca (fresh storage) — this
    // operates on an already-existing value register.
    AddrOf,
    // Conversions
    Trunc, ZExt, SExt, FPToI, IToFP, Bitcast,
    // X-Phage-specific
    Beam, Bypass, Spawn, Emit, Absorb,
    Quantum, Chronos, Await, Yield, Proc, Env,
    // enum pattern matching (probe/diverge on an enum subject) —
    // compares subj's tag against a specific variant and, if a
    // binding name was given for a data-carrying variant, extracts
    // the payload into it. See IRInstr's own fields for how this
    // opcode's operands/extra are laid out; kept as a dedicated
    // opcode (distinct from a generic Eq comparison, which is what a
    // literal/identifier probe pattern still lowers to) because tag
    // comparison and payload extraction both need the *concrete* C++
    // enum representation (Tag enum class + std::variant payload) at
    // emission time — a plain register-to-register Eq has no way to
    // express that.
    EnumMatch,
    // Phi (SSA join)
    Phi,
    // Intrinsics
    Unreachable, Nop
};

struct IRValue {
    std::string name;
    IRType      type = IRType::Auto;
    std::string literal; // for constants
    bool        is_const = false;
};

struct IRInstr {
    IROpcode    op;
    std::string dest;       // result register (%0, %1, ...)
    IRType      dest_type  = IRType::Auto;
    std::vector<std::string> operands;
    std::string extra;      // string payload, label, etc.
    uint32_t    line = 0;
};

struct IRBlock {
    std::string label;
    std::vector<IRInstr> instrs;
};

struct IRParam {
    std::string name;
    IRType      type = IRType::Auto;
    // Concrete X-Phage type name, populated when type == IRType::Struct
    // (or whenever the generic enum tag alone would be lossy for C++
    // codegen, e.g. "AppConfig", "Result", "Server"). Empty when the
    // param's type is fully described by `type` already (I64, F64, ...).
    std::string type_name;
};

struct IRFunction {
    std::string            name;
    IRType                 ret_type  = IRType::Void;
    // Concrete X-Phage return type name, mirrors IRParam::type_name.
    // Needed because IRType::Struct alone can't tell a C++ emitter
    // whether a function returns AppConfig, Result, Server, etc.
    std::string            ret_type_name;
    std::vector<IRParam>   params;
    std::vector<IRBlock>   blocks;
    bool                   is_async  = false;
    // Genuine FFI / external declaration (real extern "C" symbol
    // resolved by the linker from an external object).
    bool                   is_extern = false;
    // ABI string from `extern "C" pulse ...` (e.g. "C"). Only
    // meaningful when is_extern is true. Defaults to "C" since that's
    // overwhelmingly the common case (matches the parser's own
    // default — see parse_extern_decl's `abi = "C"`), but is tracked
    // explicitly rather than hardcoded downstream so a source-level
    // `extern "C-unwind"` or similar isn't silently coerced to plain
    // "C" by the emitter.
    std::string            extern_abi = "C";
    // Declaration exists only to describe a runtime-provided function
    // (e.g. stdlib signatures like abs(), fstring_format, iter_has_next).
    // Signature-only functions must NOT be emitted as extern "C" decls
    // in the generated C++ — their implementation is injected by the
    // runtime, and marking them is_extern collides with real library
    // symbols (e.g. <cstdlib>'s abs()).
    bool                   is_signature_only = false;

    IRBlock& entry_block() { return blocks.front(); }
    IRBlock& new_block(const std::string& lbl) {
        blocks.push_back(IRBlock{lbl, {}});
        return blocks.back();
    }
};

struct IRGlobal {
    std::string name;
    IRType      type = IRType::Auto;
    // Concrete X-Phage type name when type == IRType::Struct.
    std::string type_name;
    std::string init_value;
    bool        is_const = false;
    bool        is_flux  = false;  // Phase 2 reactive state
};

struct IRTypeDecl {
    std::string              name;
    bool                     is_abstract = false; // nexus
    std::vector<IRParam>     fields;
    std::vector<std::string> methods;
};

struct IREnumVariant {
    std::string           name;
    // Payload slot types, in declared order — empty for a
    // no-payload variant (e.g. Status.Ok). Each entry mirrors
    // IRParam's type/type_name pairing: `type` carries the coarse
    // IRType, `type_name` the concrete struct name when type ==
    // IRType::Struct (same convention used everywhere else in this
    // header for exactly the same reason — IRType::Struct alone is
    // lossy).
    std::vector<IRParam>  payload;
};

struct IREnumDecl {
    std::string                  name;
    std::vector<IREnumVariant>   variants;
};

struct IRModule {
    std::string                  name;
    std::vector<IRGlobal>        globals;
    std::vector<IRTypeDecl>      types;
    std::vector<IREnumDecl>      enums;
    std::vector<IRFunction>      functions;
    std::vector<std::string>     imports;
    std::unordered_map<std::string, std::string> meta;
};

} // namespace xphage::ir
