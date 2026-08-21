// ============================================================
// xphage_codegen_llvm — LLVM AST Visitor Backend v4.0.0
// Phase 1-3: complete codegen. No C++ intermediate.
// LLVM 16-21 compatible via version-gated macros.
// AeonCoreX Lab
// ============================================================
#include "../include/codegen_llvm.hpp"
#include "xphage/ast.hpp"

// ── non-LLVM stub (always compiled, safe fallback) ───────────
#ifndef ENABLE_LLVM

namespace xphage::codegen_llvm {
bool        is_available()     { return false; }
std::string llvm_version_str() { return "disabled"; }

LLVMResult compile_ast(const Program&, const std::string& o, const LLVMConfig&) {
    LLVMResult r; r.error = "LLVM backend not compiled in (rebuild with -DENABLE_LLVM=ON)"; return r;
}
LLVMResult compile_ir(const xphage::ir::IRModule&, const std::string& o, const LLVMConfig&) {
    LLVMResult r; r.error = "LLVM backend not compiled in"; return r;
}
bool link_binary(const std::vector<std::string>&, const std::string&,
                 const std::string&, bool) { return false; }
} // namespace

void XPhageLLVMCompiler::compile_tokens(const std::vector<Token>&, std::string) {
    fprintf(stderr, "[LLVM] disabled — rebuild with -DENABLE_LLVM=ON\n");
}

#else  // ENABLE_LLVM ─────────────────────────────────────────

// ── Version-gated LLVM includes ──────────────────────────────
#include <llvm/Config/llvm-config.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Verifier.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>

// Host triple / target detection (moved in LLVM 17)
#if LLVM_VERSION_MAJOR >= 17
  #include <llvm/TargetParser/Host.h>
  #include <llvm/TargetParser/Triple.h>
#else
  #include <llvm/Support/Host.h>
  #include <llvm/ADT/Triple.h>
#endif

// CodeGenFileType moved in LLVM 18
#if LLVM_VERSION_MAJOR >= 18
  #include <llvm/Support/CodeGen.h>
  #define XP_OBJ_FILETYPE llvm::CodeGenFileType::ObjectFile
  #define XP_ASM_FILETYPE llvm::CodeGenFileType::AssemblyFile
#else
  #define XP_OBJ_FILETYPE llvm::CGFT_ObjectFile
  #define XP_ASM_FILETYPE llvm::CGFT_AssemblyFile
#endif

// Pass manager
#if LLVM_VERSION_MAJOR >= 16
  #include <llvm/Passes/PassBuilder.h>
  #include <llvm/Analysis/CGSCCPassManager.h>
  #include <llvm/Analysis/LoopAnalysisManager.h>
  #define XP_USE_NEW_PM 1
#endif
#include <llvm/IR/LegacyPassManager.h>

// Reloc model
#if LLVM_VERSION_MAJOR >= 16
  #include <optional>
  #define XP_RM std::optional<llvm::Reloc::Model>
#else
  #define XP_RM llvm::Optional<llvm::Reloc::Model>
#endif

#include <unordered_map>
#include <vector>
#include <string>
#include <memory>
#include <iostream>
#include <fstream>
#include <chrono>
#include <stdexcept>
#include <functional>
#include <cstdlib>

using namespace llvm;

// ─────────────────────────────────────────────────────────────
// ASTCodegen — the visitor that turns AST → LLVM IR
// ─────────────────────────────────────────────────────────────
class ASTCodegen {
public:
    LLVMContext&      ctx;
    IRBuilder<>       B;
    Module&           M;

    // ── Symbol table: name → alloca ptr ───────────────────────
    struct Scope {
        std::unordered_map<std::string, Value*>    vars;      // name → alloca
        std::unordered_map<std::string, Type*>     var_types; // name → LLVM type
        std::unordered_map<std::string, Function*> fns;       // name → Function
    };
    std::vector<Scope>  scopes;

    // Loop stack for break/continue
    struct LoopInfo { BasicBlock* cond; BasicBlock* end; };
    std::vector<LoopInfo> loop_stack;

    // xprt external function cache
    std::unordered_map<std::string, FunctionCallee> xprt_cache;

    // Forge struct types
    std::unordered_map<std::string, StructType*>        forge_types;
    std::unordered_map<std::string, std::vector<std::string>> forge_fields;

    explicit ASTCodegen(LLVMContext& c, Module& m)
        : ctx(c), B(c), M(m) {
        scopes.push_back({});
    }

    // ── Type helpers ──────────────────────────────────────────
    Type* xp_type(const std::string& t) {
        if (t == "int"  || t == "long long" || t.empty()) return B.getInt64Ty();
        if (t == "float"|| t == "double")                  return B.getDoubleTy();
        if (t == "bool")                                   return B.getInt1Ty();
        if (t == "str")                                    return xpstr_ptr_ty();
        if (t == "void")                                   return B.getVoidTy();
        // Raw pointer (FFI): *mut T / *const T → opaque pointer.
        // LLVM's opaque-pointer model means the pointee type doesn't
        // need to be resolved here — any pointer is just `ptr`.
        if (t.size() > 1 && t[0] == '*')                    return opaque_ptr();
        if (forge_types.count(t))                          return forge_types.at(t);
        return B.getInt64Ty(); // default
    }

    // XpStr* is an opaque ptr in LLVM 17+, i8* before
    Type* xpstr_ptr_ty() {
#if LLVM_VERSION_MAJOR >= 17
        return PointerType::getUnqual(ctx);
#else
        return PointerType::getUnqual(Type::getInt8Ty(ctx));
#endif
    }

    Type* opaque_ptr() {
#if LLVM_VERSION_MAJOR >= 17
        return PointerType::getUnqual(ctx);
#else
        return PointerType::getUnqual(Type::getInt8Ty(ctx));
#endif
    }

    // ── Scope helpers ─────────────────────────────────────────
    void push_scope() { scopes.push_back({}); }
    void pop_scope()  { scopes.pop_back(); }

    Value* lookup(const std::string& name) {
        for (int i = (int)scopes.size()-1; i >= 0; i--) {
            auto it = scopes[i].vars.find(name);
            if (it != scopes[i].vars.end()) return it->second;
        }
        return nullptr;
    }

    Type* lookup_type(const std::string& name) {
        for (int i = (int)scopes.size()-1; i >= 0; i--) {
            auto it = scopes[i].var_types.find(name);
            if (it != scopes[i].var_types.end()) return it->second;
        }
        return B.getInt64Ty();
    }

    Function* lookup_fn(const std::string& name) {
        for (int i = (int)scopes.size()-1; i >= 0; i--) {
            auto it = scopes[i].fns.find(name);
            if (it != scopes[i].fns.end()) return it->second;
        }
        if (auto* f = M.getFunction(name)) return f;
        return nullptr;
    }

    void def_var(const std::string& n, Value* v, Type* t) {
        scopes.back().vars[n] = v;
        scopes.back().var_types[n] = t;
    }

    void def_fn(const std::string& n, Function* f) {
        scopes.back().fns[n] = f;
    }

    // ── xprt function declarations (lazy) ─────────────────────
    FunctionCallee xprt(const std::string& name,
                        Type* ret,
                        std::vector<Type*> params,
                        bool variadic = false) {
        auto it = xprt_cache.find(name);
        if (it != xprt_cache.end()) return it->second;
        auto ft = FunctionType::get(ret, params, variadic);
        auto fc = M.getOrInsertFunction(name, ft);
        xprt_cache[name] = fc;
        return fc;
    }

    // Common xprt shorthands
    FunctionCallee xprt_str_new() {
        return xprt("xprt_str_new", xpstr_ptr_ty(), {opaque_ptr()});
    }
    FunctionCallee xprt_beam_str() {
        return xprt("xprt_beam_str", B.getVoidTy(), {xpstr_ptr_ty()});
    }
    FunctionCallee xprt_beam_int() {
        return xprt("xprt_beam_int", B.getVoidTy(), {B.getInt64Ty()});
    }
    FunctionCallee xprt_beam_float() {
        return xprt("xprt_beam_float", B.getVoidTy(), {B.getDoubleTy()});
    }
    FunctionCallee xprt_beam_bool() {
        return xprt("xprt_beam_bool", B.getVoidTy(), {B.getInt1Ty()});
    }
    FunctionCallee xprt_str_concat() {
        return xprt("xprt_str_concat", xpstr_ptr_ty(),
                    {xpstr_ptr_ty(), xpstr_ptr_ty()});
    }
    FunctionCallee xprt_str_from_int() {
        return xprt("xprt_str_from_int", xpstr_ptr_ty(), {B.getInt64Ty()});
    }
    FunctionCallee xprt_str_from_float() {
        return xprt("xprt_str_from_float", xpstr_ptr_ty(), {B.getDoubleTy()});
    }
    FunctionCallee xprt_str_from_bool() {
        return xprt("xprt_str_from_bool", xpstr_ptr_ty(), {B.getInt1Ty()});
    }
    FunctionCallee xprt_str_eq_cstr() {
        return xprt("xprt_str_eq_cstr", B.getInt1Ty(),
                    {xpstr_ptr_ty(), opaque_ptr()});
    }

    // ── String literal → global constant + xprt_str_new ──────
    Value* make_str_literal(const std::string& s) {
        // Create a global constant char array
        Constant* cstr = ConstantDataArray::getString(ctx, s, true);
        auto* gv = new GlobalVariable(M, cstr->getType(), true,
                    GlobalValue::PrivateLinkage, cstr, ".xpstr");
        gv->setUnnamedAddr(GlobalValue::UnnamedAddr::Global);
        // Decay to i8* / ptr
        Value* ptr = B.CreateConstGEP2_32(cstr->getType(), gv, 0, 0);
        return B.CreateCall(xprt_str_new(), {ptr});
    }

    // ── f-string builder ─────────────────────────────────────
    // Parses raw f-string content, builds XpStr by concatenation
    Value* build_fstring(const std::string& raw, Function* fn) {
        // Start with empty XpStr*
        Value* result = make_str_literal("");
        std::string cur_lit;
        bool in_expr = false;
        std::string expr_buf;

        auto flush_lit = [&]() {
            if (cur_lit.empty()) return;
            Value* part = make_str_literal(cur_lit);
            result = B.CreateCall(xprt_str_concat(), {result, part});
            cur_lit.clear();
        };

        for (size_t i = 0; i < raw.size(); i++) {
            if (!in_expr) {
                if (raw[i] == '{') { flush_lit(); in_expr = true; }
                else cur_lit += raw[i];
            } else {
                if (raw[i] == '}') {
                    // Evaluate expr_buf as variable or expression
                    std::string trimmed = expr_buf;
                    while (!trimmed.empty() && trimmed.front() == ' ') trimmed.erase(0,1);
                    while (!trimmed.empty() && trimmed.back()  == ' ') trimmed.pop_back();
                    Value* ev = nullptr;

                    // Check if it's a simple identifier we know
                    Value* alloca_ptr = lookup(trimmed);
                    if (alloca_ptr) {
                        Type* t = lookup_type(trimmed);
                        Value* loaded = B.CreateLoad(t, alloca_ptr);
                        if (t == B.getInt64Ty())    ev = B.CreateCall(xprt_str_from_int(),   {loaded});
                        else if (t == B.getDoubleTy()) ev = B.CreateCall(xprt_str_from_float(),{loaded});
                        else if (t == B.getInt1Ty())   ev = B.CreateCall(xprt_str_from_bool(), {B.CreateZExt(loaded, B.getInt64Ty())});
                        else ev = loaded; // already XpStr*
                    } else {
                        // Inline integer constant
                        try {
                            int64_t v = std::stoll(trimmed);
                            ev = B.CreateCall(xprt_str_from_int(), {B.getInt64(v)});
                        } catch(...) {
                            ev = make_str_literal("?" + trimmed + "?");
                        }
                    }
                    if (ev) result = B.CreateCall(xprt_str_concat(), {result, ev});
                    expr_buf.clear();
                    in_expr = false;
                } else {
                    expr_buf += raw[i];
                }
            }
        }
        flush_lit();
        return result;
    }

    // ── Load a value (auto-deref alloca) ──────────────────────
    Value* load_var(const std::string& name) {
        Value* ptr = lookup(name);
        if (!ptr) return B.getInt64(0);
        return B.CreateLoad(lookup_type(name), ptr, name);
    }

    // ── Expression codegen ────────────────────────────────────
    Value* gen_expr(const ASTNode& n) {
        switch (n.kind) {
            case NodeKind::IntLit:
                return B.getInt64(std::stoll(n.value));
            case NodeKind::FloatLit:
                return ConstantFP::get(B.getDoubleTy(), std::stod(n.value));
            case NodeKind::BoolLit:
                return B.getInt1(n.value == "true" ? 1 : 0);
            case NodeKind::NullLit:
                return Constant::getNullValue(opaque_ptr());
            case NodeKind::StringLit:
                return make_str_literal(n.value);
            case NodeKind::FStringLit:
                return build_fstring(n.value, B.GetInsertBlock()->getParent());

            case NodeKind::Identifier: {
                Value* ptr = lookup(n.value);
                if (!ptr) {
                    // Could be a zero-arg function call shorthand
                    Function* f = lookup_fn(n.value);
                    if (f && f->arg_size() == 0)
                        return B.CreateCall(f, {});
                    return B.getInt64(0);
                }
                return B.CreateLoad(lookup_type(n.value), ptr, n.value);
            }

            case NodeKind::BinaryOp: {
                if (n.children.size() < 2) return B.getInt64(0);
                Value* L = gen_expr(*n.children[0]);
                Value* R = gen_expr(*n.children[1]);
                const std::string& op = n.value;

                // Determine dominant type
                bool is_float = L->getType()->isDoubleTy() ||
                                R->getType()->isDoubleTy();
                bool is_bool  = op=="&&"||op=="||"||
                                op=="=="||op=="!="||op=="<"||op=="<="||
                                op==">"||op==">="||op=="and"||op=="or";

                // Coerce int → float if needed
                if (is_float) {
                    if (L->getType()->isIntegerTy())
                        L = B.CreateSIToFP(L, B.getDoubleTy());
                    if (R->getType()->isIntegerTy())
                        R = B.CreateSIToFP(R, B.getDoubleTy());
                }
                // Coerce i1 → i64 for arithmetic
                if (!is_bool && !is_float) {
                    if (L->getType()->isIntegerTy(1))
                        L = B.CreateZExt(L, B.getInt64Ty());
                    if (R->getType()->isIntegerTy(1))
                        R = B.CreateZExt(R, B.getInt64Ty());
                }

                if (op == "+") return is_float ? B.CreateFAdd(L,R) : B.CreateAdd(L,R);
                if (op == "-") return is_float ? B.CreateFSub(L,R) : B.CreateSub(L,R);
                if (op == "*") return is_float ? B.CreateFMul(L,R) : B.CreateMul(L,R);
                if (op == "/") return is_float ? B.CreateFDiv(L,R) : B.CreateSDiv(L,R);
                if (op == "%") return B.CreateSRem(L,R);

                if (op == "=="||op=="eq") return is_float ? B.CreateFCmpOEQ(L,R):B.CreateICmpEQ(L,R);
                if (op == "!=")           return is_float ? B.CreateFCmpONE(L,R):B.CreateICmpNE(L,R);
                if (op == "<")            return is_float ? B.CreateFCmpOLT(L,R):B.CreateICmpSLT(L,R);
                if (op == "<=")           return is_float ? B.CreateFCmpOLE(L,R):B.CreateICmpSLE(L,R);
                if (op == ">")            return is_float ? B.CreateFCmpOGT(L,R):B.CreateICmpSGT(L,R);
                if (op == ">=")           return is_float ? B.CreateFCmpOGE(L,R):B.CreateICmpSGE(L,R);

                if (op == "&&"||op=="and") {
                    if (!L->getType()->isIntegerTy(1)) L = B.CreateICmpNE(L, B.getInt64(0));
                    if (!R->getType()->isIntegerTy(1)) R = B.CreateICmpNE(R, B.getInt64(0));
                    return B.CreateAnd(L, R);
                }
                if (op == "||"||op=="or") {
                    if (!L->getType()->isIntegerTy(1)) L = B.CreateICmpNE(L, B.getInt64(0));
                    if (!R->getType()->isIntegerTy(1)) R = B.CreateICmpNE(R, B.getInt64(0));
                    return B.CreateOr(L, R);
                }
                // Bitwise operators (integer only)
                if (op == "&")  return B.CreateAnd(L, R);
                if (op == "|")  return B.CreateOr(L, R);
                if (op == "^")  return B.CreateXor(L, R);
                if (op == "<<") return B.CreateShl(L, R);
                if (op == ">>") return B.CreateAShr(L, R); // arithmetic right shift
                return B.getInt64(0);
            }

            case NodeKind::UnaryOp: {
                if (n.children.empty()) return B.getInt64(0);
                Value* V = gen_expr(*n.children[0]);
                if (n.value == "!") {
                    if (!V->getType()->isIntegerTy(1))
                        V = B.CreateICmpNE(V, B.getInt64(0));
                    return B.CreateNot(V);
                }
                if (n.value == "-") {
                    if (V->getType()->isDoubleTy()) return B.CreateFNeg(V);
                    return B.CreateNeg(V);
                }
                if (n.value == "~") {      // bitwise NOT
                    return B.CreateNot(V);
                }
                return V;
            }

            case NodeKind::AssignExpr: {
                if (n.children.size() < 2) return B.getInt64(0);
                std::string op   = n.value; // = += -= *= /=
                std::string name;
                if (n.children[0]->kind == NodeKind::Identifier)
                    name = n.children[0]->value;

                Value* rhs = gen_expr(*n.children[1]);
                Value* ptr = name.empty() ? nullptr : lookup(name);
                if (!ptr) return rhs;

                Type*  t   = lookup_type(name);
                Value* val = rhs;
                if (op != "=") {
                    Value* cur = B.CreateLoad(t, ptr, name);
                    if      (op == "+=") val = t->isDoubleTy() ? B.CreateFAdd(cur,rhs):B.CreateAdd(cur,rhs);
                    else if (op == "-=") val = t->isDoubleTy() ? B.CreateFSub(cur,rhs):B.CreateSub(cur,rhs);
                    else if (op == "*=") val = t->isDoubleTy() ? B.CreateFMul(cur,rhs):B.CreateMul(cur,rhs);
                    else if (op == "/=") val = t->isDoubleTy() ? B.CreateFDiv(cur,rhs):B.CreateSDiv(cur,rhs);
                }
                // Coerce if needed
                if (t != val->getType()) {
                    if (t->isDoubleTy() && val->getType()->isIntegerTy())
                        val = B.CreateSIToFP(val, B.getDoubleTy());
                    else if (t->isIntegerTy(64) && val->getType()->isIntegerTy(1))
                        val = B.CreateZExt(val, B.getInt64Ty());
                    else if (t->isIntegerTy(64) && val->getType()->isDoubleTy())
                        val = B.CreateFPToSI(val, B.getInt64Ty());
                }
                B.CreateStore(val, ptr);
                return val;
            }

            case NodeKind::CallExpr: {
                if (n.children.empty()) return B.getInt64(0);
                std::string fname;
                if (n.children[0]->kind == NodeKind::Identifier)
                    fname = n.children[0]->value;
                Function* fn = lookup_fn(fname);
                if (!fn) fn = M.getFunction(fname);
                if (!fn) return B.getInt64(0);

                std::vector<Value*> args;
                for (size_t i = 1; i < n.children.size(); i++) {
                    if (!n.children[i]) continue;
                    Value* arg = gen_expr(*n.children[i]);
                    // Coerce to expected param type
                    if (i - 1 < fn->arg_size()) {
                        Type* expected = fn->getFunctionType()->getParamType(i-1);
                        if (expected != arg->getType()) {
                            if (expected->isDoubleTy() && arg->getType()->isIntegerTy())
                                arg = B.CreateSIToFP(arg, expected);
                            else if (expected->isIntegerTy(64) && arg->getType()->isIntegerTy(1))
                                arg = B.CreateZExt(arg, expected);
                            else if (expected->isIntegerTy(64) && arg->getType()->isDoubleTy())
                                arg = B.CreateFPToSI(arg, expected);
                        }
                    }
                    args.push_back(arg);
                }
                return B.CreateCall(fn, args);
            }

            case NodeKind::CastExpr: {
                if (n.children.empty()) return B.getInt64(0);
                Value* v  = gen_expr(*n.children[0]);
                Type*  to = xp_type(n.extra);
                if (v->getType() == to) return v;
                if (to->isDoubleTy() && v->getType()->isIntegerTy())
                    return B.CreateSIToFP(v, to);
                if (to->isIntegerTy(64) && v->getType()->isDoubleTy())
                    return B.CreateFPToSI(v, to);
                if (to->isIntegerTy(64) && v->getType()->isIntegerTy(1))
                    return B.CreateZExt(v, to);
                return B.CreateBitCast(v, to);
            }

            case NodeKind::PipelineExpr: {
                // expr |> fn  →  fn(expr)
                if (n.children.size() < 2) return B.getInt64(0);
                Value*  arg  = gen_expr(*n.children[0]);
                std::string fn_name;
                if (n.children[1]->kind == NodeKind::Identifier)
                    fn_name = n.children[1]->value;
                Function* fn = lookup_fn(fn_name);
                if (!fn) return arg;
                return B.CreateCall(fn, {arg});
            }

            case NodeKind::ProcExpr: {
                if (n.children.empty()) return make_str_literal("");
                Value* cmd = gen_expr(*n.children[0]);
                // If cmd is XpStr*, get cstr
                FunctionCallee cstr_fn = xprt("xprt_str_cstr", opaque_ptr(),
                                              {xpstr_ptr_ty()});
                Value* cstr = B.CreateCall(cstr_fn, {cmd});
                FunctionCallee proc_fn = xprt("xprt_proc", xpstr_ptr_ty(),
                                              {opaque_ptr()});
                return B.CreateCall(proc_fn, {cstr});
            }

            case NodeKind::EnvExpr: {
                Value* key = make_cstr(n.value);
                FunctionCallee env_fn = xprt("xprt_env_get", xpstr_ptr_ty(),
                                             {opaque_ptr()});
                return B.CreateCall(env_fn, {key});
            }

            case NodeKind::AwaitExpr: {
                if (n.children.empty()) return B.getInt64(0);
                // std::future<T>::get() equivalent — call as regular function for now
                // Full async requires coroutine support (Phase 6+)
                return gen_expr(*n.children[0]);
            }

            case NodeKind::MemberExpr: {
                if (n.children.empty()) return B.getInt64(0);
                // Handle flux.get()
                std::string obj_name;
                if (n.children[0]->kind == NodeKind::Identifier)
                    obj_name = n.children[0]->value;
                Value* ptr = lookup(obj_name);
                if (!ptr) return B.getInt64(0);
                Type*  t   = lookup_type(obj_name);
                Value* obj = B.CreateLoad(t, ptr, obj_name);
                // For forge structs
                if (forge_fields.count(obj_name)) {
                    auto& fields = forge_fields.at(obj_name);
                    for (size_t i = 0; i < fields.size(); i++) {
                        if (fields[i] == n.value) {
                            Value* gep = B.CreateStructGEP(
                                forge_types.count(obj_name) ? (Type*)forge_types.at(obj_name) : t,
                                ptr, (unsigned)i);
                            // Field type — need to query the struct element type
                            if (forge_types.count(obj_name)) {
                                Type* ft = forge_types.at(obj_name)->getElementType(i);
                                return B.CreateLoad(ft, gep);
                            }
                            return B.CreateLoad(B.getInt64Ty(), gep);
                        }
                    }
                }
                return obj;
            }

            case NodeKind::SpawnExpr: {
                // spawn TypeName { field: val, ... }
                if (!forge_types.count(n.value)) return B.getInt64(0);
                StructType* st = forge_types.at(n.value);
                Value* alloca  = B.CreateAlloca(st, nullptr, n.value + "_spawn");
                auto& fields   = forge_fields.at(n.value);
                for (auto& child : n.children) {
                    if (!child) continue;
                    std::string fname = child->attrs.count("field")
                                        ? child->attrs.at("field") : "";
                    for (size_t i = 0; i < fields.size(); i++) {
                        if (fields[i] == fname) {
                            Value* gep = B.CreateStructGEP(st, alloca, (unsigned)i);
                            Value* val = gen_expr(*child);
                            // Coerce
                            Type* ft = st->getElementType(i);
                            if (ft != val->getType()) {
                                if (ft->isDoubleTy() && val->getType()->isIntegerTy())
                                    val = B.CreateSIToFP(val, ft);
                                else if (ft->isIntegerTy(64) && val->getType()->isDoubleTy())
                                    val = B.CreateFPToSI(val, ft);
                                else if (ft->isIntegerTy(64) && val->getType()->isIntegerTy(1))
                                    val = B.CreateZExt(val, ft);
                            }
                            B.CreateStore(val, gep);
                        }
                    }
                }
                return alloca;
            }

            default:
                return B.getInt64(0);
        }
    }

    // Make a C-string pointer for a literal
    Value* make_cstr(const std::string& s) {
        Constant* c = ConstantDataArray::getString(ctx, s, true);
        auto* gv = new GlobalVariable(M, c->getType(), true,
                    GlobalValue::PrivateLinkage, c, ".cstr");
        gv->setUnnamedAddr(GlobalValue::UnnamedAddr::Global);
        return B.CreateConstGEP2_32(c->getType(), gv, 0, 0);
    }

    // ── Statement codegen ─────────────────────────────────────
    void gen_stmt(const ASTNode& n, Function* fn) {
        if (!B.GetInsertBlock()) return;
        if (B.GetInsertBlock()->getTerminator()) {
            // After a return, skip (dead code)
            if (n.kind == NodeKind::ReturnStmt) return;
        }

        switch (n.kind) {
            // ── Variable declarations ──────────────────────────
            case NodeKind::AtomDecl:
            case NodeKind::ShadowDecl:
            case NodeKind::ConstDecl: {
                Type* ty = xp_type(n.extra);
                Value* alloca = B.CreateAlloca(ty, nullptr, n.value);
                def_var(n.value, alloca, ty);
                if (!n.children.empty() && n.children[0]) {
                    Value* init = gen_expr(*n.children[0]);
                    // Coerce
                    if (init->getType() != ty) {
                        if (ty->isDoubleTy() && init->getType()->isIntegerTy())
                            init = B.CreateSIToFP(init, ty);
                        else if (ty->isIntegerTy(64) && init->getType()->isDoubleTy())
                            init = B.CreateFPToSI(init, ty);
                        else if (ty->isIntegerTy(64) && init->getType()->isIntegerTy(1))
                            init = B.CreateZExt(init, ty);
                        else if (ty->isIntegerTy(1) && init->getType()->isIntegerTy(64))
                            init = B.CreateICmpNE(init, B.getInt64(0));
                    }
                    B.CreateStore(init, alloca);
                } else {
                    // Zero-initialise
                    B.CreateStore(Constant::getNullValue(ty), alloca);
                }
                break;
            }

            case NodeKind::FluxDecl: {
                // Store as normal alloca; full reactor hooking via xprt_flux_*
                Type* ty = xp_type(n.extra);
                Value* alloca = B.CreateAlloca(ty, nullptr, n.value + "_flux");
                def_var(n.value, alloca, ty);
                if (!n.children.empty() && n.children[0]) {
                    Value* init = gen_expr(*n.children[0]);
                    B.CreateStore(init, alloca);
                }
                break;
            }

            // ── beam ──────────────────────────────────────────
            case NodeKind::BeamStmt: {
                if (n.children.empty()) {
                    B.CreateCall(xprt("xprt_beam_newline",B.getVoidTy(),{}));
                    break;
                }
                Value* v = gen_expr(*n.children[0]);
                Type*  t = v->getType();
                if (t == B.getInt64Ty())    B.CreateCall(xprt_beam_int(),   {v});
                else if (t == B.getDoubleTy()) B.CreateCall(xprt_beam_float(),{v});
                else if (t == B.getInt1Ty())   B.CreateCall(xprt_beam_bool(), {B.CreateZExt(v,B.getInt64Ty())});
                else B.CreateCall(xprt_beam_str(), {v}); // XpStr*
                break;
            }

            // ── bypass ────────────────────────────────────────
            case NodeKind::BypassStmt: {
                if (n.children.empty()) break;
                Value* v = gen_expr(*n.children[0]);
                FunctionCallee bp = xprt("xprt_bypass", B.getVoidTy(), {opaque_ptr()});
                // If v is XpStr*, get cstr first
                if (v->getType() == xpstr_ptr_ty()) {
                    FunctionCallee cstr_fn = xprt("xprt_str_cstr", opaque_ptr(),{xpstr_ptr_ty()});
                    v = B.CreateCall(cstr_fn, {v});
                }
                B.CreateCall(bp, {v});
                break;
            }

            // ── chronos ───────────────────────────────────────
            case NodeKind::ChronosStmt: {
                FunctionCallee cf = xprt("xprt_chronos", B.getVoidTy(), {B.getInt64Ty()});
                Value* ms = n.children.empty() ? B.getInt64(0) : gen_expr(*n.children[0]);
                if (!ms->getType()->isIntegerTy(64))
                    ms = B.CreateFPToSI(ms, B.getInt64Ty());
                B.CreateCall(cf, {ms});
                break;
            }

            // ── scan ─────────────────────────────────────────
            case NodeKind::ScanStmt: {
                Value* ptr = lookup(n.value);
                if (!ptr) break;
                Type*  ty  = lookup_type(n.value);
                Value* v;
                if (ty == B.getInt64Ty())
                    v = B.CreateCall(xprt("xprt_scan_int",B.getInt64Ty(),{}));
                else if (ty == B.getDoubleTy())
                    v = B.CreateCall(xprt("xprt_scan_float",B.getDoubleTy(),{}));
                else
                    v = B.CreateCall(xprt("xprt_scan_str",xpstr_ptr_ty(),{}));
                B.CreateStore(v, ptr);
                break;
            }

            // ── return ────────────────────────────────────────
            case NodeKind::ReturnStmt: {
                if (n.children.empty()) {
                    B.CreateRetVoid();
                } else {
                    Value* v = gen_expr(*n.children[0]);
                    Type*  rt = fn->getReturnType();
                    if (v->getType() != rt) {
                        if (rt->isDoubleTy() && v->getType()->isIntegerTy())
                            v = B.CreateSIToFP(v, rt);
                        else if (rt->isIntegerTy(64) && v->getType()->isDoubleTy())
                            v = B.CreateFPToSI(v, rt);
                        else if (rt->isIntegerTy(64) && v->getType()->isIntegerTy(1))
                            v = B.CreateZExt(v, rt);
                        else if (rt->isIntegerTy(1) && v->getType()->isIntegerTy(64))
                            v = B.CreateICmpNE(v, B.getInt64(0));
                    }
                    B.CreateRet(v);
                }
                break;
            }

            case NodeKind::BreakStmt:
                if (!loop_stack.empty()) B.CreateBr(loop_stack.back().end);
                break;

            case NodeKind::ContinueStmt:
                if (!loop_stack.empty()) B.CreateBr(loop_stack.back().cond);
                break;

            // ── if / elif / else ──────────────────────────────
            case NodeKind::IfStmt: {
                if (n.children.size() < 2) break;
                gen_if(n, fn);
                break;
            }

            // ── while ─────────────────────────────────────────
            case NodeKind::WhileStmt: {
                if (n.children.size() < 2) break;
                gen_while(n, fn);
                break;
            }

            // ── for i in start..end ───────────────────────────
            case NodeKind::ForStmt: {
                if (n.children.size() < 2) break;
                gen_for(n, fn);
                break;
            }

            // ── probe / diverge ───────────────────────────────
            case NodeKind::ProbeStmt: {
                if (n.children.empty()) break;
                gen_probe(n, fn);
                break;
            }

            // ── emit ──────────────────────────────────────────
            case NodeKind::EmitStmt: {
                FunctionCallee emit_fn = xprt("xprt_emit", B.getVoidTy(),
                    {opaque_ptr(), opaque_ptr()});
                Value* ev  = make_cstr(n.value);
                Value* dat = make_cstr("");
                B.CreateCall(emit_fn, {ev, dat});
                break;
            }

            // ── absorb ────────────────────────────────────────
            case NodeKind::AbsorbStmt: {
                // Create a handler function for this absorb block
                std::string hname = "xp_absorb_" + sanitize_id(n.value) + "_" +
                                    std::to_string(reinterpret_cast<uintptr_t>(&n));
                FunctionType* hft = FunctionType::get(
                    B.getVoidTy(), {opaque_ptr(), opaque_ptr()}, false);
                Function* hfn = Function::Create(hft,
                    Function::InternalLinkage, hname, M);
                BasicBlock* hentry = BasicBlock::Create(ctx, "entry", hfn);
                IRBuilder<> saved = B; // save outer builder
                B.SetInsertPoint(hentry);
                push_scope();
                if (!n.children.empty() && n.children[0])
                    gen_block(*n.children[0], hfn);
                B.CreateRetVoid();
                pop_scope();
                B.restoreIP(saved.saveIP()); // restore builder

                // Register handler at startup
                FunctionCallee absorb_fn = xprt("xprt_absorb", B.getVoidTy(),
                    {opaque_ptr(), opaque_ptr()});
                Value* ev = make_cstr(n.value);
                B.CreateCall(absorb_fn, {ev, hfn});
                break;
            }

            // ── vortex (try/catch) ────────────────────────────
            case NodeKind::VortexStmt: {
                // No exception support in LLVM IR without DWARF/SEH
                // Emit body inline; catch block emitted in else path
                if (!n.children.empty() && n.children[0])
                    gen_block(*n.children[0], fn);
                break;
            }

            // ── quantum (thread) ─────────────────────────────
            case NodeKind::QuantumStmt: {
                // Simplified: emit block inline (thread spawn needs xprt_thread_spawn)
                if (!n.children.empty() && n.children[0])
                    gen_block(*n.children[0], fn);
                break;
            }

            // ── Expression statement ──────────────────────────
            case NodeKind::ExprStmt: {
                if (!n.children.empty() && n.children[0])
                    gen_expr(*n.children[0]);
                break;
            }

            case NodeKind::Block:
                gen_block(n, fn);
                break;

            default:
                // For anything else that's expression-like, evaluate it
                if (!n.children.empty())
                    for (auto& c : n.children)
                        if (c) gen_stmt(*c, fn);
                break;
        }
    }

    // ── Block codegen ─────────────────────────────────────────
    void gen_block(const ASTNode& blk, Function* fn) {
        push_scope();
        for (auto& child : blk.children) {
            if (!child) continue;
            if (B.GetInsertBlock() && B.GetInsertBlock()->getTerminator()) break;
            gen_stmt(*child, fn);
        }
        pop_scope();
    }

    // ── if / elif / else ─────────────────────────────────────
    void gen_if(const ASTNode& n, Function* fn) {
        BasicBlock* merge = BasicBlock::Create(ctx, "if_end", fn);

        auto emit_branch = [&](Value* cond, const ASTNode& body,
                                BasicBlock* next) {
            if (!cond->getType()->isIntegerTy(1))
                cond = B.CreateICmpNE(cond, B.getInt64(0));
            BasicBlock* then = BasicBlock::Create(ctx, "then", fn);
            B.CreateCondBr(cond, then, next);
            B.SetInsertPoint(then);
            gen_block(body, fn);
            if (!B.GetInsertBlock()->getTerminator()) B.CreateBr(merge);
        };

        BasicBlock* next = merge;
        // Pre-create "next" blocks for elif/else chains
        std::vector<BasicBlock*> elif_blocks;
        BasicBlock*              else_block = nullptr;
        for (size_t i = 2; i < n.children.size(); i++) {
            if (!n.children[i]) continue;
            if (n.children[i]->kind == NodeKind::ElifStmt) {
                elif_blocks.push_back(BasicBlock::Create(ctx,"elif",fn));
            } else if (n.children[i]->kind == NodeKind::ElseStmt) {
                else_block = BasicBlock::Create(ctx,"else",fn);
            }
        }

        // Determine final "else" landing
        BasicBlock* first_next = merge;
        if (else_block)              first_next = else_block;
        else if (!elif_blocks.empty()) first_next = elif_blocks[0];

        // Emit if
        Value* cond = gen_expr(*n.children[0]);
        if (!cond->getType()->isIntegerTy(1))
            cond = B.CreateICmpNE(cond, B.getInt64(0));
        BasicBlock* then = BasicBlock::Create(ctx,"then",fn);
        B.CreateCondBr(cond, then, first_next);
        B.SetInsertPoint(then);
        gen_block(*n.children[1], fn);
        if (!B.GetInsertBlock()->getTerminator()) B.CreateBr(merge);

        // Emit elif chains
        size_t ei = 0;
        BasicBlock* else_or_merge = else_block ? else_block : merge;
        for (size_t i = 2; i < n.children.size(); i++) {
            if (!n.children[i]) continue;
            if (n.children[i]->kind == NodeKind::ElifStmt) {
                BasicBlock* cur_elif = elif_blocks[ei];
                BasicBlock* nxt = (ei+1 < elif_blocks.size())
                                   ? elif_blocks[ei+1] : else_or_merge;
                B.SetInsertPoint(cur_elif);
                Value* ec = gen_expr(*n.children[i]->children[0]);
                if (!ec->getType()->isIntegerTy(1))
                    ec = B.CreateICmpNE(ec, B.getInt64(0));
                BasicBlock* ethen = BasicBlock::Create(ctx,"elif_then",fn);
                B.CreateCondBr(ec, ethen, nxt);
                B.SetInsertPoint(ethen);
                gen_block(*n.children[i]->children[1], fn);
                if (!B.GetInsertBlock()->getTerminator()) B.CreateBr(merge);
                ei++;
            } else if (n.children[i]->kind == NodeKind::ElseStmt && else_block) {
                B.SetInsertPoint(else_block);
                gen_block(*n.children[i]->children[0], fn);
                if (!B.GetInsertBlock()->getTerminator()) B.CreateBr(merge);
            }
        }
        B.SetInsertPoint(merge);
    }

    // ── while ────────────────────────────────────────────────
    void gen_while(const ASTNode& n, Function* fn) {
        BasicBlock* cond_bb = BasicBlock::Create(ctx,"while_cond",fn);
        BasicBlock* body_bb = BasicBlock::Create(ctx,"while_body",fn);
        BasicBlock* end_bb  = BasicBlock::Create(ctx,"while_end",fn);

        loop_stack.push_back({cond_bb, end_bb});
        B.CreateBr(cond_bb);
        B.SetInsertPoint(cond_bb);
        Value* c = gen_expr(*n.children[0]);
        if (!c->getType()->isIntegerTy(1))
            c = B.CreateICmpNE(c, B.getInt64(0));
        B.CreateCondBr(c, body_bb, end_bb);
        B.SetInsertPoint(body_bb);
        gen_block(*n.children[1], fn);
        if (!B.GetInsertBlock()->getTerminator()) B.CreateBr(cond_bb);
        loop_stack.pop_back();
        B.SetInsertPoint(end_bb);
    }

    // ── for i in start..end ──────────────────────────────────
    void gen_for(const ASTNode& n, Function* fn) {
        auto& iter_node = n.children[0];
        bool is_range = iter_node && iter_node->kind == NodeKind::RangeExpr;

        BasicBlock* cond_bb = BasicBlock::Create(ctx,"for_cond",fn);
        BasicBlock* body_bb = BasicBlock::Create(ctx,"for_body",fn);
        BasicBlock* step_bb = BasicBlock::Create(ctx,"for_step",fn);
        BasicBlock* end_bb  = BasicBlock::Create(ctx,"for_end",fn);

        // Loop variable
        Value* loop_var = B.CreateAlloca(B.getInt64Ty(), nullptr, n.value);
        def_var(n.value, loop_var, B.getInt64Ty());

        if (is_range) {
            Value* start = gen_expr(*iter_node->children[0]);
            Value* limit = gen_expr(*iter_node->children[1]);
            // Allocate limit
            Value* lim_var = B.CreateAlloca(B.getInt64Ty(), nullptr, "for_lim");
            if (!start->getType()->isIntegerTy(64)) start = B.CreateFPToSI(start,B.getInt64Ty());
            if (!limit->getType()->isIntegerTy(64)) limit = B.CreateFPToSI(limit,B.getInt64Ty());
            B.CreateStore(start, loop_var);
            B.CreateStore(limit, lim_var);

            loop_stack.push_back({step_bb, end_bb});
            B.CreateBr(cond_bb);
            B.SetInsertPoint(cond_bb);
            Value* cur = B.CreateLoad(B.getInt64Ty(), loop_var, "cur");
            Value* lim = B.CreateLoad(B.getInt64Ty(), lim_var, "lim");
            Value* c   = B.CreateICmpSLT(cur, lim);
            B.CreateCondBr(c, body_bb, end_bb);
            B.SetInsertPoint(body_bb);
            push_scope();
            gen_block(*n.children[1], fn);
            pop_scope();
            if (!B.GetInsertBlock()->getTerminator()) B.CreateBr(step_bb);
            B.SetInsertPoint(step_bb);
            Value* next = B.CreateAdd(B.CreateLoad(B.getInt64Ty(),loop_var), B.getInt64(1));
            B.CreateStore(next, loop_var);
            B.CreateBr(cond_bb);
            loop_stack.pop_back();
        } else {
            // Generic iterable: skip for now, emit empty loop
            B.CreateBr(end_bb);
        }
        B.SetInsertPoint(end_bb);
    }

    // ── probe / diverge ──────────────────────────────────────
    void gen_probe(const ASTNode& n, Function* fn) {
        Value* subj = gen_expr(*n.children[0]);
        BasicBlock* end_bb = BasicBlock::Create(ctx,"probe_end",fn);

        for (size_t i = 1; i < n.children.size(); i++) {
            auto& arm = n.children[i];
            if (!arm || arm->kind != NodeKind::ProbeArm) continue;

            BasicBlock* arm_bb  = BasicBlock::Create(ctx,"arm",fn);
            BasicBlock* next_bb = BasicBlock::Create(ctx,"next",fn);

            if (arm->value == "_") {
                // Wildcard — always taken
                B.CreateBr(arm_bb);
                B.SetInsertPoint(arm_bb);
                if (!arm->children.empty() && arm->children[0])
                    gen_block(*arm->children[0], fn);
                if (!B.GetInsertBlock()->getTerminator()) B.CreateBr(end_bb);
                B.SetInsertPoint(next_bb);
                B.CreateBr(end_bb);
                break;
            } else {
                // Compare subject against pattern
                Value* cond;
                Type*  st = subj->getType();
                if (st == B.getInt64Ty()) {
                    Value* pat = B.getInt64(std::stoll(arm->value));
                    cond = B.CreateICmpEQ(subj, pat);
                } else if (st == xpstr_ptr_ty()) {
                    Value* pat = make_cstr(arm->value);
                    cond = B.CreateCall(xprt_str_eq_cstr(), {subj, pat});
                } else {
                    cond = B.CreateICmpEQ(subj,
                        ConstantInt::get(st, std::stoll(arm->value)));
                }
                B.CreateCondBr(cond, arm_bb, next_bb);
                B.SetInsertPoint(arm_bb);
                if (!arm->children.empty() && arm->children[0])
                    gen_block(*arm->children[0], fn);
                if (!B.GetInsertBlock()->getTerminator()) B.CreateBr(end_bb);
                B.SetInsertPoint(next_bb);
            }
        }
        if (!B.GetInsertBlock()->getTerminator()) B.CreateBr(end_bb);
        B.SetInsertPoint(end_bb);
    }

    // ── Forge type declaration ────────────────────────────────
    void gen_forge(const ASTNode& n) {
        std::vector<Type*> field_types;
        std::vector<std::string> field_names;
        for (auto& f : n.children) {
            if (!f || f->kind != NodeKind::FieldDecl) continue;
            field_types.push_back(xp_type(f->extra));
            field_names.push_back(f->value);
        }
        StructType* st = StructType::create(ctx, field_types, "forge." + n.value);
        forge_types[n.value]  = st;
        forge_fields[n.value] = field_names;
    }

    // ── extern "ABI" pulse name(params) -> ret  → FFI declaration ──
    // Declares an external symbol for the linker to resolve — no
    // body is emitted. This is how XPhage calls into a C library,
    // a Rust `cdylib`/`staticlib` (functions exported as
    // `#[no_mangle] extern "C" fn ...`), or any other ABI-compatible
    // object. The actual `.a`/`.so`/`.lib` must be supplied to the
    // linker separately (via xphage.pkg native-deps or a manual
    // -l flag) — this only creates the LLVM-side declaration.
    Function* gen_extern(const ASTNode& n) {
        if (Function* existing = M.getFunction(n.value)) return existing;

        Type* ret = xp_type(n.extra2);
        std::vector<Type*> param_types;
        for (auto& c : n.children) {
            if (c && c->kind == NodeKind::FieldDecl) {
                param_types.push_back(xp_type(c->extra));
            }
        }
        FunctionType* ft = FunctionType::get(ret, param_types, false);
        Function* fn = Function::Create(ft, Function::ExternalLinkage, n.value, M);
        def_fn(n.value, fn);
        return fn;
    }

    // ── Pulse (function) codegen ──────────────────────────────
    Function* gen_pulse(const ASTNode& n) {
        std::string  name   = n.value.empty() ? "main" : n.value;
        std::string  ret_tn = n.extra2;
        bool is_main = (name == "main");

        // Collect params (FieldDecl children before body)
        std::vector<std::pair<std::string,std::string>> params;
        for (auto& c : n.children) {
            if (!c) continue;
            if (c->kind == NodeKind::FieldDecl) {
                params.push_back({c->value, c->extra});
            }
        }

        // Build LLVM function type
        Type* ret = is_main ? B.getInt32Ty() : xp_type(ret_tn);
        std::vector<Type*> param_types;
        for (auto& [pn,pt] : params) param_types.push_back(xp_type(pt));

        FunctionType* ft = FunctionType::get(ret, param_types, false);
        Function* fn = Function::Create(ft, Function::ExternalLinkage, name, M);

        // Name params
        size_t i = 0;
        for (auto& arg : fn->args()) {
            if (i < params.size()) {
                arg.setName(params[i].first);
                i++;
            }
        }
        def_fn(name, fn);

        // Find body block
        const ASTNode* body = nullptr;
        for (auto& c : n.children) {
            if (c && c->kind == NodeKind::Block) { body = c.get(); break; }
        }
        if (!body) {
            // Declaration-only (forward decl) — no body needed
            fn->setLinkage(Function::ExternalLinkage);
            return fn;
        }

        BasicBlock* entry = BasicBlock::Create(ctx, "entry", fn);
        B.SetInsertPoint(entry);
        push_scope();

        // Allocate and store each arg
        i = 0;
        for (auto& arg : fn->args()) {
            Type* aty = xp_type(params[i].second);
            Value* alloca = B.CreateAlloca(aty, nullptr, params[i].first);
            B.CreateStore(&arg, alloca);
            def_var(params[i].first, alloca, aty);
            i++;
        }

        gen_block(*body, fn);
        pop_scope();

        // Add implicit return if block didn't terminate
        if (!B.GetInsertBlock()->getTerminator()) {
            if (ret == B.getVoidTy())   B.CreateRetVoid();
            else if (ret == B.getInt32Ty()) B.CreateRet(B.getInt32(0));
            else if (ret == B.getInt64Ty()) B.CreateRet(B.getInt64(0));
            else if (ret == B.getDoubleTy())B.CreateRet(ConstantFP::get(ret,0.0));
            else if (ret == B.getInt1Ty())  B.CreateRet(B.getInt1(0));
            else B.CreateRetVoid();
        }

        // Verify
        std::string err;
        raw_string_ostream es(err);
        if (verifyFunction(*fn, &es)) {
            errs() << "[xphage llvm] verify error in " << name << ": " << err << "\n";
        }
        return fn;
    }

    static std::string sanitize_id(const std::string& s) {
        std::string r; for(char c:s) r += (isalnum(c)?c:'_'); return r;
    }

    // ── Top-level module codegen ──────────────────────────────
    void gen_module(const Program& prog) {
        // Pass 1: forge types (needed before function bodies reference them)
        for (auto& top : prog) {
            if (!top) continue;
            if (top->kind == NodeKind::ForgeDecl) gen_forge(*top);
        }

        // Pass 2: forward-declare all pulses so mutual recursion works
        for (auto& top : prog) {
            if (!top) continue;
            if (top->kind == NodeKind::PulseDecl ||
                top->kind == NodeKind::AsyncPulseDecl) {
                std::string nm = top->value.empty() ? "main" : top->value;
                // Create forward decl
                std::vector<Type*> param_types;
                for (auto& c : top->children) {
                    if (c && c->kind == NodeKind::FieldDecl)
                        param_types.push_back(xp_type(c->extra));
                }
                bool is_main = (nm == "main");
                Type* ret = is_main ? B.getInt32Ty() : xp_type(top->extra2);
                FunctionType* ft = FunctionType::get(ret, param_types, false);
                Function* fn = Function::Create(ft, Function::ExternalLinkage, nm, M);
                def_fn(nm, fn);
            }
        }

        // Pass 3: emit all pulse bodies + impl methods + top-level stmts
        Function* implicit_main = nullptr;
        BasicBlock* implicit_entry = nullptr;
        bool has_explicit_main = false;

        // Check for explicit main pulse
        for (auto& top : prog) {
            if (!top) continue;
            if ((top->kind == NodeKind::PulseDecl || top->kind == NodeKind::AsyncPulseDecl)
                && (top->value == "main" || top->value.empty())) {
                has_explicit_main = true;
                break;
            }
        }

        // If no explicit main, create implicit one for top-level stmts
        if (!has_explicit_main) {
            FunctionType* mft = FunctionType::get(B.getInt32Ty(), false);
            implicit_main = Function::Create(mft, Function::ExternalLinkage, "main", M);
            implicit_entry = BasicBlock::Create(ctx, "entry", implicit_main);
            B.SetInsertPoint(implicit_entry);
            def_fn("main", implicit_main);
            push_scope();
        }

        for (auto& top : prog) {
            if (!top) continue;
            switch (top->kind) {
                case NodeKind::ForgeDecl:
                case NodeKind::NexusDecl:
                case NodeKind::LinkStmt:
                    break; // already handled / metadata only

                case NodeKind::RealmDecl:
                case NodeKind::UseDecl:
                    // Namespace/realm lowering for LLVM IR not yet implemented
                    // (full scoped symbol mangling required). Use the
                    // transpiler backend (--backend=transpiler, default)
                    // for programs that use `realm`.
                    break;

                case NodeKind::ExternDecl:
                    gen_extern(*top);
                    break;

                case NodeKind::UnsafeBlock: {
                    // Transparent: lower the inner declarations/statements
                    // through this same top-level dispatch.
                    if (!top->children.empty() && top->children[0]) {
                        for (auto& inner : top->children[0]->children) {
                            if (!inner) continue;
                            if (inner->kind == NodeKind::ExternDecl) {
                                gen_extern(*inner);
                            } else if (implicit_main) {
                                gen_stmt(*inner, implicit_main);
                            }
                        }
                    }
                    break;
                }

                case NodeKind::GlobalDecl: {
                    if (!implicit_main) break;
                    // Global as local in implicit main
                    Type* ty = B.getInt64Ty();
                    Value* a  = B.CreateAlloca(ty, nullptr, top->value);
                    def_var(top->value, a, ty);
                    if (!top->children.empty() && top->children[0]) {
                        Value* v = gen_expr(*top->children[0]);
                        B.CreateStore(v, a);
                    }
                    break;
                }

                case NodeKind::FluxDecl: {
                    if (!implicit_main) break;
                    Type* ty = xp_type(top->extra);
                    Value* a  = B.CreateAlloca(ty, nullptr, top->value);
                    def_var(top->value, a, ty);
                    if (!top->children.empty() && top->children[0]) {
                        Value* v = gen_expr(*top->children[0]);
                        B.CreateStore(v, a);
                    }
                    break;
                }

                case NodeKind::PulseDecl:
                case NodeKind::AsyncPulseDecl: {
                    // Restore implicit_main insertion point after each pulse
                    BasicBlock* saved_bb = nullptr;
                    if (implicit_main && B.GetInsertBlock()) {
                        saved_bb = B.GetInsertBlock();
                    }
                    gen_pulse(*top);
                    if (saved_bb) B.SetInsertPoint(saved_bb);
                    break;
                }

                case NodeKind::ImplDecl: {
                    for (auto& method : top->children) {
                        if (!method) continue;
                        // Emit as free function: ForgeType__method_name
                        ASTNode synthetic = *method;
                        synthetic.value = top->extra + "__" + method->value;
                        BasicBlock* saved = implicit_main ? B.GetInsertBlock() : nullptr;
                        gen_pulse(synthetic);
                        if (saved) B.SetInsertPoint(saved);
                    }
                    break;
                }

                case NodeKind::AbsorbStmt: {
                    if (implicit_main) gen_stmt(*top, implicit_main);
                    break;
                }

                default: {
                    // All other top-level stmts → implicit main
                    if (implicit_main) gen_stmt(*top, implicit_main);
                    break;
                }
            }
        }

        // Close implicit main
        if (implicit_main) {
            pop_scope();
            if (!B.GetInsertBlock()->getTerminator())
                B.CreateRet(B.getInt32(0));
        }
    }
};

// ─────────────────────────────────────────────────────────────
// Optimisation helper
// ─────────────────────────────────────────────────────────────
static void run_optimisations(Module& M, TargetMachine* TM, int opt_level) {
    if (opt_level == 0) return;

#ifdef XP_USE_NEW_PM
    LoopAnalysisManager      LAM;
    FunctionAnalysisManager  FAM;
    CGSCCAnalysisManager     CGAM;
    ModuleAnalysisManager    MAM;

    PassBuilder PB(TM);
    PB.registerModuleAnalyses(MAM);
    PB.registerCGSCCAnalyses(CGAM);
    PB.registerFunctionAnalyses(FAM);
    PB.registerLoopAnalyses(LAM);
    PB.crossRegisterProxies(LAM, FAM, CGAM, MAM);

    OptimizationLevel ol;
    switch (opt_level) {
        case 1:  ol = OptimizationLevel::O1; break;
        case 3:  ol = OptimizationLevel::O3; break;
        default: ol = OptimizationLevel::O2; break;
    }
    auto MPM = PB.buildPerModuleDefaultPipeline(ol);
    MPM.run(M, MAM);
#else
    legacy::PassManager PM;
    TM->addPassesToEmitFile(PM, *raw_null_ostream::Create(), nullptr,
                             XP_ASM_FILETYPE);
#endif
}

// ─────────────────────────────────────────────────────────────
// Public API
// ─────────────────────────────────────────────────────────────
namespace xphage::codegen_llvm {

bool is_available() { return true; }

std::string llvm_version_str() {
    return std::string(LLVM_VERSION_STRING) +
           " (LLVM " + std::to_string(LLVM_VERSION_MAJOR) + ")";
}

LLVMResult compile_ast(const Program& ast,
                        const std::string& output_obj,
                        const LLVMConfig& cfg) {
    LLVMResult res;
    auto t0 = std::chrono::steady_clock::now();

    // Initialise all LLVM targets
    InitializeAllTargetInfos();
    InitializeAllTargets();
    InitializeAllTargetMCs();
    InitializeAllAsmParsers();
    InitializeAllAsmPrinters();

    LLVMContext ctx;
    auto mod = std::make_unique<Module>("xphage", ctx);

    // Codegen
    try {
        ASTCodegen cg(ctx, *mod);
        cg.gen_module(ast);
    } catch (const std::exception& e) {
        res.error = std::string("Codegen exception: ") + e.what();
        return res;
    }

    // Optional: dump LLVM IR
    if (cfg.emit_llvm_ir) {
        std::string ir_path = output_obj + ".ll";
        std::error_code ec;
        raw_fd_ostream ll_out(ir_path, ec);
        if (!ec) {
            mod->print(ll_out, nullptr);
            res.llvm_ir_path = ir_path;
        }
    }

    // Select target
    std::string triple = cfg.target_triple.empty()
        ? sys::getDefaultTargetTriple() : cfg.target_triple;
    mod->setTargetTriple(triple);

    std::string err_str;
    const Target* tgt = TargetRegistry::lookupTarget(triple, err_str);
    if (!tgt) { res.error = "Target lookup failed: " + err_str; return res; }

    TargetOptions opts;
    XP_RM rm;
    if (cfg.pic) rm = Reloc::PIC_;

    std::string cpu  = cfg.cpu.empty()      ? "generic" : cfg.cpu;
    std::string feat = cfg.features;

    auto TM = std::unique_ptr<TargetMachine>(
        tgt->createTargetMachine(triple, cpu, feat, opts, rm));
    mod->setDataLayout(TM->createDataLayout());

    // Optimise
    run_optimisations(*mod, TM.get(), cfg.opt_level);

    // Verify module
    std::string verr;
    raw_string_ostream vs(verr);
    if (verifyModule(*mod, &vs)) {
        if (cfg.verbose) errs() << "[xphage llvm] module verify warning: " << verr << "\n";
    }

    // Emit object
    std::error_code ec;
    raw_fd_ostream dest(output_obj, ec, sys::fs::OF_None);
    if (ec) { res.error = "Cannot open output: " + ec.message(); return res; }

    legacy::PassManager PM;
    if (TM->addPassesToEmitFile(PM, dest, nullptr, XP_OBJ_FILETYPE)) {
        res.error = "Cannot emit object file for this target";
        return res;
    }
    PM.run(*mod);
    dest.flush();

    auto t1 = std::chrono::steady_clock::now();
    res.elapsed_ms = std::chrono::duration<double,std::milli>(t1-t0).count();
    res.output_obj  = output_obj;
    res.success     = true;

    if (cfg.verbose)
        outs() << "[llvm] " << triple << " → " << output_obj
               << "  (" << res.elapsed_ms << " ms)\n";
    return res;
}

LLVMResult compile_ir(const xphage::ir::IRModule& mod,
                       const std::string& output_obj,
                       const LLVMConfig& cfg) {
    LLVMResult r;
    r.error = "compile_ir: use compile_ast for Phase 4. LLVM IR module path in Phase 5.";
    return r;
}

bool link_binary(const std::vector<std::string>& objs,
                 const std::string& xprt_lib,
                 const std::string& output_bin,
                 bool verbose) {
    // Build link command: cc objs... xprt.a -lm -lpthread -o bin
    std::string cmd = "cc";
    for (auto& o : objs) cmd += " \"" + o + "\"";
    if (!xprt_lib.empty()) cmd += " \"" + xprt_lib + "\"";
    cmd += " -lm -lpthread";
#if !defined(_WIN32)
    cmd += " -lstdc++fs";
#endif
    cmd += " -o \"" + output_bin + "\"";
    if (verbose) errs() << "[xphage link] " << cmd << "\n";
    return std::system(cmd.c_str()) == 0;
}

} // namespace xphage::codegen_llvm

// Legacy bridge
void XPhageLLVMCompiler::compile_tokens(const std::vector<Token>& toks,
                                         std::string output_obj) {
    fprintf(stderr, "[llvm] token-based compile not supported in v4 — use compile_ast\n");
}

#endif // ENABLE_LLVM
