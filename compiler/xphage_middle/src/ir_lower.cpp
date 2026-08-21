// ============================================================
// xphage_middle — IR Lowering v4.0.0
// Walks Phase 1-3 AST, emits XPIR instructions
// AeonCoreX Lab
// ============================================================
#include "../include/ir_lower.hpp"
#include "../../xphage_lexer/include/lexer.hpp"
#include "../../xphage_parse/include/parser.hpp"
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <iostream>
#include <functional>

namespace xphage::middle {

using namespace xphage::ir;

// ── Type conversion ───────────────────────────────────────────
static IRType map_type(const std::string& xp) {
    if (xp == "int"   || xp == "long long") return IRType::I64;
    if (xp == "float" || xp == "double")    return IRType::F64;
    if (xp == "bool")                        return IRType::Bool;
    if (xp == "str"   || xp == "string")    return IRType::String;
    if (xp == "void")                        return IRType::Void;
    if (!xp.empty())                         return IRType::Struct;
    return IRType::Auto;
}

// ── IR Builder (SSA register allocator) ──────────────────────
class IRBuilder {
public:
    IRModule      mod;
    IRFunction*   cur_fn  = nullptr;
    IRBlock*      cur_blk = nullptr;
    int           reg_cnt = 0;
    // `use Geometry::circle_area` lets later code call the
    // unqualified `circle_area(...)`. Since realm members are
    // lowered under mangled, fully-qualified names (Geometry_
    // circle_area — see xil_cpp_name in the transpiler), a bare
    // identifier reference needs to be rewritten to the qualified
    // form at lowering time, or it simply won't resolve to anything
    // in the generated C++. Populated by a pre-pass over the whole
    // Program before any lowering happens (see lower_to_ir), so a
    // `use` declared anywhere (including after its first use — Yhe
    // language book doesn't document an ordering requirement) is
    // always visible.
    std::unordered_map<std::string, std::string> use_aliases; // bare name -> qualified name

    // Populated by a pre-pass (see collect_enums in lower_to_ir)
    // before any expression lowering happens, so `Status.Ok` /
    // `Status.Error(msg)` can be recognized as enum-variant
    // construction — as opposed to ordinary struct field access —
    // regardless of whether the `enum Status { ... }` declaration
    // appears before or after the code that constructs one.
    // name -> the original EnumDecl AST node (kept, rather than some
    // reduced form, so variant payload arity/types are available
    // directly during CallExpr lowering without a second lookup
    // structure).
    std::unordered_map<std::string, ASTNodePtr> known_enums;

    // Every declared forge/enum's method set, keyed by the receiver
    // type name: "Type::method" -> true for every method
    // `impl Type { method(self, ...) ... }` / `impl Nexus for Type
    // { ... }` declares. Populated by the same pre-pass that builds
    // known_enums (see collect_impls in lower_to_ir). Needed so
    // `obj.method(args)` can be recognized as a real method call
    // (lowered to a Call against the mangled "Type::method" function,
    // with &obj prepended as the implicit-self first argument) rather
    // than falling through to ordinary field access + calling the
    // result as if it were a function pointer — which is meaningless
    // and was the previous (only ever accidental/unintended) behavior
    // here, since no method-call lowering existed in this file at
    // all before this fix.
    std::unordered_set<std::string> known_methods;

    // Best-effort local variable type tracking: name -> declared/
    // inferred type name, populated as Alloca-backed declarations are
    // lowered (see the AtomDecl/ShadowDecl/pulse-parameter lowering
    // sites). This is what lets `obj.method(args)` resolve which
    // concrete "Type::method" to call — XIL lowering has no real type
    // checker, so this is a deliberately lightweight, best-effort
    // substitute: a linear, non-scope-aware map (matching
    // monomorphize.cpp's own var_types, and sharing its documented
    // limitation — a variable shadowed in a nested block with a
    // different type isn't modeled correctly).
    std::unordered_map<std::string, std::string> var_types;

    explicit IRBuilder(const std::string& name) {
        mod.name = name;
        // cur_fn/cur_blk (and, since the AbsorbStmt fix, saved/
        // restored IRFunction* locals in lower_stmt) are raw pointers
        // into mod.functions/blocks. A std::vector reallocates on
        // push_back once capacity is exceeded, which would silently
        // invalidate every such pointer and turn "safe" code into a
        // use-after-free. Reserving generous headroom up front avoids
        // reallocation for any single-module compilation in practice;
        // this is a defensive measure that costs nothing observable
        // and removes an entire class of latent dangling-pointer bugs
        // rather than requiring every call site to be pointer-safe.
        mod.functions.reserve(4096);
    }

    std::string new_reg() { return "%r" + std::to_string(reg_cnt++); }
    std::string new_lbl(const std::string& hint) {
        return hint + "_" + std::to_string(reg_cnt++);
    }

    IRFunction& begin_function(const std::string& nm, IRType ret,
                                const std::vector<IRParam>& params,
                                bool is_async = false,
                                const std::string& ret_type_name = "") {
        // NOTE: deliberately NOT using positional aggregate init here
        // (IRFunction{nm, ret, params, {}, is_async}) — a prior bug
        // showed that adding a field to IRFunction silently shifts
        // every positional initializer's meaning. Field-by-field
        // assignment survives IRFunction layout changes safely.
        IRFunction fn;
        fn.name          = nm;
        fn.ret_type      = ret;
        fn.ret_type_name = ret_type_name;
        fn.params        = params;
        fn.is_async      = is_async;
        mod.functions.push_back(std::move(fn));
        cur_fn  = &mod.functions.back();
        cur_blk = &cur_fn->new_block("entry");
        return *cur_fn;
    }

    IRInstr& emit(IROpcode op, const std::string& dest = "",
                  std::vector<std::string> ops = {}, const std::string& extra = "",
                  uint32_t line = 0) {
        cur_blk->instrs.push_back({op, dest, IRType::Auto, std::move(ops), extra, line});
        return cur_blk->instrs.back();
    }
};

// ── Expression lowering ───────────────────────────────────────

static std::string lower_expr(const ASTNode& n, IRBuilder& b);

// Parse and lower a single f-string interpolation segment's raw
// source text (the content between `{` and `}`, already extracted
// from the surrounding literal text by lower_fstring below) into an
// IR value reference, by feeding it through the real Lexer + Parser
// exactly the way the AST transpiler's lex_and_emit_fstr_expr does.
// This is what makes `f"result={a+b}"` lower a genuine BinaryOp
// rather than treating the whole "a+b" as an opaque, unparsed blob.
static std::string lower_fstring_expr(const std::string& raw_expr, IRBuilder& b) {
    std::string e = raw_expr;
    while (!e.empty() && (e.front() == ' ' || e.front() == '\t')) e.erase(0, 1);
    while (!e.empty() && (e.back()  == ' ' || e.back()  == '\t')) e.pop_back();
    if (e.empty()) return "std::string(\"\")";

    xphage::lexer::Lexer lex(e, "<fstring>");
    auto tokens = lex.tokenize();
    if (lex.has_errors()) return "std::string(\"\")"; // best-effort: drop unparseable segment rather than emit invalid C++

    xphage::parse::Parser parser(tokens, "<fstring>");
    ASTNodePtr node = parser.parse_single_expression();
    if (parser.has_errors() || !node) return "std::string(\"\")";

    return lower_expr(*node, b);
}

// Split an f-string's raw source (literal text interleaved with
// {expr} interpolations, brace-depth aware so nested braces inside
// an interpolated expression — e.g. f"{ {1,2,3}.size() }" — don't
// terminate the segment early) and lower it to a single IR string
// value by chaining literal segments and lowered sub-expressions with
// string-concatenation Add instructions.
static std::string lower_fstring(const std::string& raw, IRBuilder& b, int line) {
    std::string result;      // accumulated as we go: either empty (nothing emitted
                              // yet) or a "%regN"-style IR value reference
    bool have_result = false;
    std::string cur;         // literal text accumulator
    std::string expr_buf;
    bool in_expr = false;
    int brace_depth = 0;

    auto flush_literal = [&]() {
        if (cur.empty()) return;
        // std::string(...), not a bare quoted literal — see the
        // StringLit case above for why (bare char-array literals
        // don't support every operation a genuine std::string value
        // does, e.g. .c_str() when this literal is later used
        // standalone as an extern "C" call argument).
        std::string lit = "std::string(\"" + cur + "\")";
        if (!have_result) { result = lit; have_result = true; }
        else {
            std::string r = b.new_reg();
            b.emit(IROpcode::Add, r, {result, lit}, "+", line);
            result = r; 
        }
        cur.clear();
    };
    auto flush_expr = [&](const std::string& expr_src) {
        std::string val = lower_fstring_expr(expr_src, b);
        // Route the interpolated value through xp_to_string so
        // numeric/bool/struct values concatenate correctly instead of
        // relying on operator+ overload resolution across mismatched
        // types (e.g. std::string + long long doesn't compile at all).
        std::string r = b.new_reg();
        b.emit(IROpcode::Call, r, {val}, "xp_to_string", line);
        if (!have_result) { result = r; have_result = true; }
        else {
            std::string r2 = b.new_reg();
            b.emit(IROpcode::Add, r2, {result, r}, "+", line);
            result = r2;
        }
    };

    for (size_t i = 0; i < raw.size(); i++) {
        if (!in_expr) {
            if (raw[i] == '{') {
                flush_literal();
                in_expr = true;
                brace_depth = 1;
            } else {
                cur += raw[i];
            }
        } else {
            if (raw[i] == '{') {
                brace_depth++;
                expr_buf += raw[i];
            } else if (raw[i] == '}') {
                brace_depth--;
                if (brace_depth == 0) {
                    flush_expr(expr_buf);
                    expr_buf.clear();
                    in_expr = false;
                } else {
                    expr_buf += raw[i];
                }
            } else {
                expr_buf += raw[i];
            }
        }
    }
    if (in_expr && !expr_buf.empty()) flush_expr(expr_buf);
    flush_literal();

    if (!have_result) return "std::string(\"\")";
    return result;
}

static std::string lower_expr(const ASTNode& n, IRBuilder& b) {
    switch (n.kind) {
        case NodeKind::IntLit:
        case NodeKind::FloatLit:
        case NodeKind::BoolLit:
            return n.value;
        case NodeKind::StringLit:
            // Emitted as std::string("...") rather than a bare quoted
            // literal (which the same C++ generation, if pasted
            // directly, would produce as `const char[N]`/`const
            // char*` — not std::string). Bare char-array literals
            // don't have a `.c_str()` method, don't support operator+
            // with another std::string on the left in every overload
            // resolution context, and don't match the std::string
            // type this IR's inference treats every string value as.
            // Concretely, this was breaking any string literal passed
            // as an argument to an extern "C" function (`native_greet
            // ("Nahid")` lowered to a bare `"Nahid"` operand, then the
            // extern-call .c_str()-wrapping logic tried to call
            // .c_str() on a const char[6], which doesn't compile).
            // Matches the AST transpiler's identical convention (see
            // its StringLit case: `"std::string(\"" + escape + "\")"`).
            return "std::string(\"" + n.value + "\")";
        case NodeKind::FStringLit: {
            // Re-lex + re-parse each {expr} interpolation segment into
            // a real sub-expression and lower it through the normal
            // IR expression path, concatenating literal text segments
            // and lowered sub-expressions with string-concat Add
            // instructions. Previously this just forwarded the raw,
            // unparsed f-string source text as a single opaque Call
            // operand — which meant "calc={result}" was emitted
            // *verbatim* into the generated C++ as
            // `xp_fstring_format(calc={result})`, which isn't even
            // syntactically valid C++, let alone functionally correct.
            return lower_fstring(n.value, b, n.span.line);
        }
        case NodeKind::NullLit:
            return "null";
        case NodeKind::Identifier: {
            // A bare identifier that matches a `use Realm::name`
            // alias resolves to the qualified name instead — see
            // IRBuilder::use_aliases for why this rewrite is needed
            // (realm members are lowered/emitted under mangled
            // qualified names, so an unqualified reference would
            // otherwise dangle).
            auto uit = b.use_aliases.find(n.value);
            if (uit != b.use_aliases.end()) return "%" + uit->second;
            return "%" + n.value;
        }
        case NodeKind::PathExpr:
            // Realm::path (e.g. "Geometry::length_sq", "App::rank")
            // was previously unhandled here entirely — any qualified
            // reference fell to `default: return "%undef"`, so every
            // call through a realm path (or a qualified constant
            // reference) silently became a call to a function/global
            // literally named "undef". Uses the same "%"-prefixed
            // value-reference convention as a plain Identifier; the
            // qualified name itself was already assembled by the
            // parser (see parse_primary's COLON_COLON loop).
            return "%" + n.value;

        case NodeKind::BinaryOp: {
            if (n.children.size() < 2) return "%err";
            std::string L = lower_expr(*n.children[0], b);
            std::string R = lower_expr(*n.children[1], b);
            std::string r = b.new_reg();
            IROpcode op = IROpcode::Add;
            const std::string& opv = n.value;
            if (opv == "+")  op = IROpcode::Add;
            else if (opv == "-")  op = IROpcode::Sub;
            else if (opv == "*")  op = IROpcode::Mul;
            else if (opv == "/")  op = IROpcode::Div;
            else if (opv == "%")  op = IROpcode::Mod;
            else if (opv == "==") op = IROpcode::Eq;
            else if (opv == "!=") op = IROpcode::Ne;
            else if (opv == "<")  op = IROpcode::Lt;
            else if (opv == "<=") op = IROpcode::Le;
            else if (opv == ">")  op = IROpcode::Gt;
            else if (opv == ">=") op = IROpcode::Ge;
            else if (opv == "&&" || opv == "and") op = IROpcode::And;
            else if (opv == "||" || opv == "or")  op = IROpcode::Or;
            b.emit(op, r, {L, R}, opv, n.span.line);
            return r;
        }

        case NodeKind::UnaryOp: {
            if (n.children.empty()) return "%err";
            std::string V = lower_expr(*n.children[0], b);
            std::string r = b.new_reg();
            // Every unary operator (!, -, ~) was previously lowered
            // to IROpcode::Not unconditionally — n.value (the actual
            // operator, "!" / "-" / "~") was captured in extra but
            // never consulted, so `-5.0` silently became logical-NOT
            // of 5.0 instead of numeric negation (`!(5.0)` in the
            // generated C++, which evaluates to `false`/0 — this is
            // what broke `abs(-5.0)` when the math stdlib was first
            // exercised through XIL). There's no dedicated IR opcode
            // for negation or bitwise-not, so:
            //   - "-" lowers to Sub with a literal "0" left operand
            //     (0 - x), which is exact for both integer and
            //     floating-point negation.
            //   - "~" reuses IROpcode::Not, but tagged via `extra`
            //     ("bitwise") so the C++ emitter can tell it apart
            //     from "!" and emit the bitwise operator (~) instead
            //     of the logical one (!).
            //   - "!" (the common/original case) is unchanged.
            if (n.value == "-") {
                b.emit(IROpcode::Sub, r, {"0", V}, "-", n.span.line);
            } else if (n.value == "~") {
                b.emit(IROpcode::Not, r, {V}, "bitwise", n.span.line);
            } else {
                b.emit(IROpcode::Not, r, {V}, n.value, n.span.line);
            }
            return r;
        }

        case NodeKind::CallExpr: {
            if (n.children.empty()) return "%err";
            // `EnumName.Variant(args...)` (a data-carrying variant
            // construction, e.g. `Status.Error("msg")`) — the callee
            // is a MemberExpr whose own left side is a bare
            // Identifier matching a known enum name. Intercepted here
            // (before the generic callee-lowering path below) rather
            // than left to fall through MemberExpr's own enum check,
            // since MemberExpr alone can't tell "Status.Error" (used
            // here as a call target) apart from "Status.Ok" (a
            // complete, no-payload construction) — only CallExpr
            // knows whether a "(" followed, which is exactly the
            // difference between "construct with these arguments" and
            // "construct with no arguments".
            if (n.children[0]->kind == NodeKind::MemberExpr &&
                !n.children[0]->children.empty() &&
                n.children[0]->children[0]->kind == NodeKind::Identifier) {
                auto eit = b.known_enums.find(n.children[0]->children[0]->value);
                if (eit != b.known_enums.end()) {
                    std::vector<std::string> args;
                    for (size_t i = 1; i < n.children.size(); i++) {
                        if (n.children[i]) args.push_back(lower_expr(*n.children[i], b));
                    }
                    std::string r = b.new_reg();
                    b.emit(IROpcode::Spawn, r, args,
                           n.children[0]->children[0]->value + "::" + n.children[0]->value,
                           n.span.line);
                    return r;
                }
            }
            // obj.method(args...) — a real method call, distinguished
            // from ordinary field access by whether the resolved
            // "Type::method" name is a known impl method (see
            // IRBuilder::known_methods/var_types). This was
            // previously entirely unhandled: MemberExpr's generic
            // fallthrough lowered `obj.method` to a plain GEP field
            // access, which was then "called" as if it were a
            // function pointer — meaningless, and simply never worked
            // for any impl method call on this backend before this
            // fix (confirmed by testing a plain, non-generic
            // `impl Counter { increment(self) -> int {...} }` through
            // --backend=xil, which failed the same way). Lowered to
            // an ordinary Call against the mangled "Type::method"
            // function (matching how ImplDecl itself names the
            // function it lowers each method to), with the object's
            // address prepended as the first argument — implicit self
            // is a real pointer parameter in this backend's
            // lowering (unlike the AST transpiler's reference-based
            // `auto& self = *this`), so &obj is what the method
            // signature actually expects.
            if (n.children[0]->kind == NodeKind::MemberExpr &&
                !n.children[0]->children.empty()) {
                auto& obj_node = n.children[0]->children[0];
                std::string method_name = n.children[0]->value;
                std::string obj_type;
                if (obj_node->kind == NodeKind::Identifier) {
                    auto vit = b.var_types.find(obj_node->value);
                    if (vit != b.var_types.end()) obj_type = vit->second;
                } else if (obj_node->kind == NodeKind::SpawnExpr) {
                    obj_type = obj_node->value;
                }
                if (!obj_type.empty() &&
                    b.known_methods.count(obj_type + "::" + method_name)) {
                    std::string obj_reg = lower_expr(*obj_node, b);
                    std::string obj_ptr = b.new_reg();
                    b.emit(IROpcode::AddrOf, obj_ptr, {obj_reg}, "", n.span.line);
                    std::vector<std::string> args = {obj_ptr};
                    for (size_t i = 1; i < n.children.size(); i++) {
                        if (n.children[i]) args.push_back(lower_expr(*n.children[i], b));
                    }
                    std::string r = b.new_reg();
                    b.emit(IROpcode::Call, r, args, obj_type + "::" + method_name, n.span.line);
                    return r;
                }
            }
            std::string fn = lower_expr(*n.children[0], b);
            // A plain identifier callee (the common case, e.g. `mul(6,7)`)
            // lowers through the generic Identifier rule, which prefixes
            // every identifier with "%" for use as an SSA-style value
            // reference (e.g. "%mul"). That's correct for a value being
            // loaded, but a Call's `extra` field holds the callee as a
            // literal function *name* to be emitted directly as a C++
            // call target — so the "%" must be stripped here, or the
            // emitter would happily accept the non-empty string and
            // produce invalid C++ like `%mul(6, 7)`. Same applies to a
            // qualified realm call (`Geometry::length_sq(v)`), which
            // lowers through PathExpr using that same "%"-prefixed
            // convention — the function's real (mangled) name is
            // "Geometry::length_sq" (matching how RealmDecl lowering
            // qualifies the declaration), so this must be stripped
            // here too, not left as-is.
            if ((n.children[0]->kind == NodeKind::Identifier ||
                 n.children[0]->kind == NodeKind::PathExpr) &&
                !fn.empty() && fn[0] == '%') {
                fn = fn.substr(1);
            }
            std::vector<std::string> args;
            for (size_t i = 1; i < n.children.size(); i++) {
                if (n.children[i]) args.push_back(lower_expr(*n.children[i], b));
            }
            std::string r = b.new_reg();
            b.emit(IROpcode::Call, r, args, fn, n.span.line);
            return r;
        }

        case NodeKind::MemberExpr: {
            if (n.children.empty()) return "%" + n.value;
            // `EnumName.Variant` (no payload, e.g. `Status.Ok`) is
            // enum-variant construction, not struct field access —
            // recognized by checking whether the left side is a bare
            // Identifier matching a name collected by known_enums
            // (see the collect_enums pre-pass in lower_to_ir). Without
            // this check, `Status.Ok` previously lowered as a GEP
            // treating "Status" as an object instance with a field
            // named "Ok", which is meaningless (Status is a type, not
            // a value) and would never have produced working code —
            // enum support had no representation anywhere before this.
            if (n.children[0]->kind == NodeKind::Identifier) {
                auto eit = b.known_enums.find(n.children[0]->value);
                if (eit != b.known_enums.end()) {
                    std::string r = b.new_reg();
                    b.emit(IROpcode::Spawn, r, {},
                           n.children[0]->value + "::" + n.value, n.span.line);
                    return r;
                }
            }
            std::string obj = lower_expr(*n.children[0], b);
            std::string r   = b.new_reg();
            b.emit(IROpcode::GEP, r, {obj}, n.value, n.span.line);
            return r;
        }

        case NodeKind::PipelineExpr: {
            if (n.children.size() < 2) return "%err";
            std::string lhs = lower_expr(*n.children[0], b);
            std::string fn  = lower_expr(*n.children[1], b);
            // Same callee-name issue as CallExpr: a bare identifier
            // pipeline target (`x |> mul`) must not carry the "%" SSA
            // reference prefix into Call.extra.
            if (n.children[1]->kind == NodeKind::Identifier &&
                !fn.empty() && fn[0] == '%') {
                fn = fn.substr(1);
            }
            std::string r   = b.new_reg();
            b.emit(IROpcode::Call, r, {lhs}, fn, n.span.line);
            return r;
        }

        case NodeKind::LambdaExpr: {
            std::string r = b.new_reg();
            b.emit(IROpcode::Alloca, r, {}, "lambda", n.span.line);
            return r;
        }

        case NodeKind::ProcExpr: {
            std::string cmd = n.children.empty() ? "" :
                              lower_expr(*n.children[0], b);
            std::string r = b.new_reg();
            b.emit(IROpcode::Proc, r, {cmd}, "", n.span.line);
            return r;
        }

        case NodeKind::EnvExpr: {
            std::string r = b.new_reg();
            b.emit(IROpcode::Env, r, {}, n.value, n.span.line);
            return r;
        }

        case NodeKind::AwaitExpr: {
            if (n.children.empty()) return "%err";
            std::string fut = lower_expr(*n.children[0], b);
            std::string r   = b.new_reg();
            b.emit(IROpcode::Await, r, {fut}, "", n.span.line);
            return r;
        }

        case NodeKind::AssignExpr: {
            if (n.children.size() < 2) return "%err";
            std::string rhs = lower_expr(*n.children[1], b);
            std::string lhs = lower_expr(*n.children[0], b);
            b.emit(IROpcode::Store, "", {rhs, lhs}, n.value, n.span.line);
            return rhs;
        }

        case NodeKind::SpawnExpr: {
            // Previously fell through to the default case and lowered
            // to the literal register name "%undef" — struct
            // construction had no XIL representation at all.
            //
            // Lowering strategy (mirrors the AST transpiler's model):
            //   1. allocate a temporary struct register, tagged with
            //      the concrete type name so later GEP/type-inference
            //      passes know what it is;
            //   2. for each field initializer, compute the value and
            //      store it into a GEP'd member of the temp register;
            //   3. yield the temp register as the expression result.
            std::string r = b.new_reg();
            // `extra` on the Alloca carries the concrete struct type
            // name (e.g. "Result", "AppConfig") so the C++ emitter's
            // pre-declaration pass can size/type this register
            // correctly instead of defaulting to int via bare `auto`.
            b.emit(IROpcode::Alloca, r, {}, "struct:" + n.value, n.span.line);
            for (auto& child : n.children) {
                if (!child) continue;
                std::string fname = child->attrs.count("field")
                                   ? child->attrs.at("field") : "";
                std::string val = lower_expr(*child, b);
                std::string field_reg = b.new_reg();
                b.emit(IROpcode::GEP, field_reg, {r}, fname, child->span.line);
                b.emit(IROpcode::Store, "", {val, field_reg}, fname, child->span.line);
            }
            return r;
        }

        case NodeKind::TupleExpr: {
            // (e0, e1, ...) — construct via IROpcode::Spawn tagged
            // "tuple" (distinct from the "EnumName::Variant" and
            // "struct:TypeName" uses of the same opcode elsewhere in
            // this file — Spawn is a general "construct a value via
            // means other than a plain literal/arithmetic op"
            // opcode, disambiguated by its extra string's prefix).
            // Unlike SpawnExpr, a tuple's element types aren't named
            // anywhere in the source (no field names, no declared
            // struct) — they're inferred purely from each operand's
            // own value, the same way the C++ emitter already infers
            // types for Add/arithmetic results from their operands.
            // Passing every element as a Spawn operand (rather than
            // the Alloca+GEP+Store-per-field pattern SpawnExpr uses)
            // keeps this a single instruction the emitter can turn
            // directly into `std::make_tuple(e0, e1, ...)`.
            std::vector<std::string> elems;
            for (auto& child : n.children) {
                if (child) elems.push_back(lower_expr(*child, b));
            }
            std::string r = b.new_reg();
            b.emit(IROpcode::Spawn, r, elems, "tuple", n.span.line);
            return r;
        }

        default:
            return "%undef";
    }
}

// ── Statement lowering ────────────────────────────────────────
static void lower_stmt(const ASTNode& n, IRBuilder& b);

static void lower_block(const ASTNode& blk, IRBuilder& b) {
    for (auto& child : blk.children) {
        if (child) lower_stmt(*child, b);
    }
}

static void lower_stmt(const ASTNode& n, IRBuilder& b) {
    switch (n.kind) {
        case NodeKind::AtomDecl:
        case NodeKind::ShadowDecl:
        case NodeKind::ConstDecl: {
            std::string r = "%" + n.value;
            b.emit(IROpcode::Alloca, r, {}, n.extra, n.span.line);
            // Track this variable's declared/inferred type — explicit
            // annotation wins if present; otherwise, if the
            // initializer is a SpawnExpr, its concrete type name.
            // Needed so a later `obj.method(args)` call on this
            // variable can resolve which "Type::method" to call (see
            // IRBuilder::var_types/known_methods).
            if (!n.extra.empty()) {
                b.var_types[n.value] = n.extra;
            } else if (!n.children.empty() && n.children[0] &&
                       n.children[0]->kind == NodeKind::SpawnExpr) {
                b.var_types[n.value] = n.children[0]->value;
            }
            if (!n.children.empty() && n.children[0]) {
                std::string val = lower_expr(*n.children[0], b);
                b.emit(IROpcode::Store, "", {val, r}, n.value, n.span.line);
            }
            break;
        }

        case NodeKind::TupleDestructure: {
            // atom (lo, hi) = min_max(numbers) — children[0..n-2] are
            // the bound Identifier names, children[n-1] is the
            // initializer expression. Lowered as: evaluate the
            // initializer once into a temp register (so a
            // side-effecting initializer, e.g. a function call, only
            // runs once — not once per bound name), then Alloca+GEP+
            // Store each bound name from the corresponding tuple
            // element via the same positional-index GEP `point.0`
            // uses (see MemberExpr's NUMBER_INT handling in the
            // parser and GEP's numeric-extra handling in both codegen
            // backends).
            if (n.children.empty()) break;
            size_t init_idx = n.children.size() - 1;
            std::string tuple_val = lower_expr(*n.children[init_idx], b);
            std::string tmp = b.new_reg();
            b.emit(IROpcode::Alloca, tmp, {}, "", n.span.line);
            b.emit(IROpcode::Store, "", {tuple_val, tmp}, "tuple_destructure_init", n.span.line);
            for (size_t i = 0; i < init_idx; i++) {
                if (!n.children[i]) continue;
                const std::string& bound_name = n.children[i]->value;
                std::string r = "%" + bound_name;
                b.emit(IROpcode::Alloca, r, {}, "", n.span.line);
                std::string elem = b.new_reg();
                b.emit(IROpcode::GEP, elem, {tmp}, std::to_string(i), n.span.line);
                b.emit(IROpcode::Store, "", {elem, r}, bound_name, n.span.line);
            }
            break;
        }

        case NodeKind::FluxDecl: {
            // Flux<T>: alloca + store + mark reactive
            std::string r = "%" + n.value;
            b.emit(IROpcode::Alloca, r, {}, "flux:" + n.extra, n.span.line);
            if (!n.children.empty() && n.children[0]) {
                std::string val = lower_expr(*n.children[0], b);
                b.emit(IROpcode::Store, "", {val, r}, "flux_init", n.span.line);
            }
            break;
        }

        case NodeKind::BeamStmt: {
            std::string val = n.children.empty() ? "\"\"" :
                              lower_expr(*n.children[0], b);
            b.emit(IROpcode::Beam, "", {val}, "", n.span.line);
            break;
        }

        case NodeKind::BypassStmt: {
            std::string cmd = n.children.empty() ? "\"\"" :
                              lower_expr(*n.children[0], b);
            b.emit(IROpcode::Bypass, "", {cmd}, "", n.span.line);
            break;
        }

        case NodeKind::ReturnStmt: {
            std::string val = n.children.empty() ? "void" :
                              lower_expr(*n.children[0], b);
            b.emit(IROpcode::Ret, "", {val}, "", n.span.line);
            break;
        }

        case NodeKind::BreakStmt:
            b.emit(IROpcode::Br, "", {}, "break", n.span.line);
            break;

        case NodeKind::ContinueStmt:
            b.emit(IROpcode::Br, "", {}, "continue", n.span.line);
            break;

        case NodeKind::IfStmt: {
            if (n.children.size() < 2) break;
            std::string cond = lower_expr(*n.children[0], b);
            std::string lbl_then = b.new_lbl("then");
            std::string lbl_else = b.new_lbl("else");
            std::string lbl_end  = b.new_lbl("end");
            b.emit(IROpcode::BrCond, "", {cond, lbl_then, lbl_else}, "", n.span.line);

            b.cur_blk = &b.cur_fn->new_block(lbl_then);
            lower_block(*n.children[1], b);
            b.emit(IROpcode::Br, "", {lbl_end});

            b.cur_blk = &b.cur_fn->new_block(lbl_else);
            for (size_t i = 2; i < n.children.size(); i++) {
                if (!n.children[i]) continue;
                if (n.children[i]->kind == NodeKind::ElifStmt) {
                    std::string c2 = lower_expr(*n.children[i]->children[0], b);
                    std::string lt2 = b.new_lbl("elif_then");
                    std::string le2 = b.new_lbl("elif_else");
                    b.emit(IROpcode::BrCond, "", {c2, lt2, le2});
                    b.cur_blk = &b.cur_fn->new_block(lt2);
                    lower_block(*n.children[i]->children[1], b);
                    b.emit(IROpcode::Br, "", {lbl_end});
                    b.cur_blk = &b.cur_fn->new_block(le2);
                } else if (n.children[i]->kind == NodeKind::ElseStmt) {
                    lower_block(*n.children[i]->children[0], b);
                }
            }
            b.emit(IROpcode::Br, "", {lbl_end});
            b.cur_blk = &b.cur_fn->new_block(lbl_end);
            break;
        }

        case NodeKind::WhileStmt: {
            if (n.children.size() < 2) break;
            std::string lbl_cond = b.new_lbl("while_cond");
            std::string lbl_body = b.new_lbl("while_body");
            std::string lbl_end  = b.new_lbl("while_end");
            b.emit(IROpcode::Br, "", {lbl_cond});
            b.cur_blk = &b.cur_fn->new_block(lbl_cond);
            std::string cond = lower_expr(*n.children[0], b);
            b.emit(IROpcode::BrCond, "", {cond, lbl_body, lbl_end});
            b.cur_blk = &b.cur_fn->new_block(lbl_body);
            lower_block(*n.children[1], b);
            b.emit(IROpcode::Br, "", {lbl_cond});
            b.cur_blk = &b.cur_fn->new_block(lbl_end);
            break;
        }

        case NodeKind::ForStmt: {
            if (n.children.size() < 2) break;
            std::string lbl_cond = b.new_lbl("for_cond");
            std::string lbl_body = b.new_lbl("for_body");
            std::string lbl_end  = b.new_lbl("for_end");
            std::string var = "%" + n.value;
            b.emit(IROpcode::Alloca, var, {}, "int", n.span.line);

            // `for i in start..end { ... }` — RangeExpr carries start
            // as children[0] and end as children[1] (see parser.cpp's
            // DOT_DOT handling). This case was previously missing
            // entirely: lower_expr() on a bare RangeExpr node falls
            // into the generic `default: return "%undef"`, so the
            // loop variable was initialized to the literal text
            // "undef" and the end-of-range bound used in the
            // iter_has_next comparison was that same "%undef" register
            // — the loop's stop condition never depended on the real
            // range at all.
            //
            // For any other iterable shape (e.g. a future array/list
            // iterable), fall back to lowering it as a single
            // expression, same as before.
            std::string start_val, end_val;
            if (n.children[0] && n.children[0]->kind == NodeKind::RangeExpr) {
                auto& range = *n.children[0];
                start_val = range.children.size() > 0 && range.children[0]
                          ? lower_expr(*range.children[0], b) : "0";
                end_val   = range.children.size() > 1 && range.children[1]
                          ? lower_expr(*range.children[1], b) : "0";
            } else {
                // Non-range iterable: keep the old single-value
                // behavior (used as both the loop var's initial value
                // and the bound passed to iter_has_next) since there's
                // no separate start/end to extract.
                start_val = lower_expr(*n.children[0], b);
                end_val   = start_val;
            }

            b.emit(IROpcode::Store, "", {start_val, var}, "for_init", n.span.line);
            b.emit(IROpcode::Br, "", {lbl_cond});
            b.cur_blk = &b.cur_fn->new_block(lbl_cond);
            std::string cond = b.new_reg();
            b.emit(IROpcode::Call, cond, {var, end_val}, "iter_has_next", n.span.line);
            b.emit(IROpcode::BrCond, "", {cond, lbl_body, lbl_end});
            b.cur_blk = &b.cur_fn->new_block(lbl_body);
            lower_block(*n.children[1], b);
            // Advance the loop variable. A range-based for always
            // steps by 1; this was previously absent too (the body
            // never mutated `var`, so even with a correct bound the
            // loop would either run 0 times or spin forever).
            std::string next = b.new_reg();
            b.emit(IROpcode::Add, next, {var, "1"}, "+", n.span.line);
            b.emit(IROpcode::Store, "", {next, var}, "for_step", n.span.line);
            b.emit(IROpcode::Br, "", {lbl_cond});
            b.cur_blk = &b.cur_fn->new_block(lbl_end);
            break;
        }

        case NodeKind::QuantumStmt: {
            if (!n.children.empty() && n.children[0]) {
                // Capture body instrs in IR
                b.emit(IROpcode::Quantum, b.new_reg(), {}, "thread_spawn", n.span.line);
                lower_block(*n.children[0], b);
            }
            break;
        }

        case NodeKind::EmitStmt: {
            // n.value is the raw event-name string (e.g. `login`,
            // from `emit "login" {}`) — it must be rendered as a C++
            // string literal, not a bare identifier reference, or the
            // generated call `EventBus::global().emit(login)` fails
            // to compile since no variable named `login` exists.
            std::string ev_lit = "\"" + n.value + "\"";
            b.emit(IROpcode::Emit, "", {ev_lit}, n.value, n.span.line);
            break;
        }

        case NodeKind::AbsorbStmt: {
            // `absorb "event" { body }` registers a handler. Two bugs
            // fixed here vs. the previous version:
            //   1. n.value (the event name) was passed unquoted as an
            //      Absorb operand, so the emitter rendered it as a
            //      bare (nonexistent) C++ identifier instead of a
            //      string literal.
            //   2. The handler body (children[0]) was never lowered
            //      at all — `absorb "login" { beam f"..." }` silently
            //      dropped its entire body, registering a handler
            //      that does nothing.
            // XIL doesn't have a natural way to lower an inline C++
            // lambda body from block-structured IR instructions (the
            // body may itself contain gotos/labels, which can't
            // legally nest inside a lambda the way this emitter
            // currently structures function bodies). Rather than
            // silently drop the body (the old, wrong behavior) or
            // half-implement lambda-scoped codegen here, lower the
            // absorb body as its own top-level handler function and
            // register *that* with EventBus, which reuses all of the
            // existing function-lowering machinery correctly.
            std::string ev_lit = "\"" + n.value + "\"";
            std::string handler_name = b.new_lbl("absorb_handler");
            if (!n.children.empty() && n.children[0]) {
                // Lower the handler body as an independent void()
                // function, then swap back to the original
                // cur_fn/cur_blk so the enclosing statement stream
                // (e.g. more top-level statements, or the rest of
                // main) isn't disrupted.
                IRFunction* saved_fn  = b.cur_fn;
                IRBlock*    saved_blk = b.cur_blk;
                b.begin_function(handler_name, IRType::Void, {});
                lower_block(*n.children[0], b);
                b.emit(IROpcode::Ret, "", {"void"});
                b.cur_fn  = saved_fn;
                b.cur_blk = saved_blk;
            }
            b.emit(IROpcode::Absorb, b.new_reg(), {ev_lit}, handler_name, n.span.line);
            break;
        }

        case NodeKind::VortexStmt: {
            // try/catch: simplified — mark range
            if (!n.children.empty()) lower_block(*n.children[0], b);
            break;
        }

        case NodeKind::YieldStmt: {
            std::string val = n.children.empty() ? "void" :
                              lower_expr(*n.children[0], b);
            b.emit(IROpcode::Yield, "", {val}, "", n.span.line);
            break;
        }

        case NodeKind::ChronosStmt: {
            std::string ms = n.children.empty() ? "0" :
                             lower_expr(*n.children[0], b);
            b.emit(IROpcode::Chronos, "", {ms}, "", n.span.line);
            break;
        }

        case NodeKind::ProbeStmt: {
            if (n.children.empty()) break;
            std::string subj = lower_expr(*n.children[0], b);
            std::string lbl_end = b.new_lbl("probe_end");
            for (size_t i = 1; i < n.children.size(); i++) {
                auto& arm = n.children[i];
                if (!arm || arm->kind != NodeKind::ProbeArm) continue;
                std::string lbl_arm  = b.new_lbl("arm");
                std::string lbl_next = b.new_lbl("arm_next");
                // An enum-qualified pattern ("EnumName.Variant",
                // optionally with bindings attrs["bindings"] for a
                // data-carrying variant — see parse_probe_stmt) needs
                // a tag comparison against the subject's concrete
                // enum representation, not a plain value-equality
                // check the way a literal/identifier pattern uses.
                // Detected by checking whether the pattern's prefix
                // (before the first '.') names a known enum — the
                // same known_enums lookup CallExpr/MemberExpr use for
                // variant construction.
                size_t dot = arm->value.find('.');
                bool is_enum_pattern = false;
                if (dot != std::string::npos) {
                    std::string enum_name = arm->value.substr(0, dot);
                    if (b.known_enums.count(enum_name)) is_enum_pattern = true;
                }
                if (is_enum_pattern) {
                    std::string cond = b.new_reg();
                    // Only the first binding is wired through for now
                    // — a variant with more than one payload slot
                    // (e.g. a hypothetical Pair(int, int)) can still
                    // be *matched*, but only its first payload value
                    // is bound; this matches every current book/spec
                    // example (Error(msg), NotFound(code) — both
                    // single-payload) and is a reasonable place to
                    // stop rather than guess at multi-binding syntax
                    // nothing has asked for yet.
                    std::string binding;
                    auto bit = arm->attrs.find("bindings");
                    if (bit != arm->attrs.end() && !bit->second.empty()) {
                        size_t comma = bit->second.find(',');
                        binding = (comma == std::string::npos) ? bit->second
                                                                 : bit->second.substr(0, comma);
                    }
                    b.emit(IROpcode::EnumMatch, cond, {subj, binding}, arm->value, arm->span.line);
                    b.emit(IROpcode::BrCond, "", {cond, lbl_arm, lbl_next});
                    b.cur_blk = &b.cur_fn->new_block(lbl_arm);
                } else if (arm->value != "_") {
                    std::string cond = b.new_reg();
                    b.emit(IROpcode::Eq, cond, {subj, arm->value});
                    b.emit(IROpcode::BrCond, "", {cond, lbl_arm, lbl_next});
                    b.cur_blk = &b.cur_fn->new_block(lbl_arm);
                } else {
                    b.cur_blk = &b.cur_fn->new_block(lbl_arm);
                }
                if (!arm->children.empty()) {
                    if (arm->children[0]->kind == NodeKind::Block)
                        lower_block(*arm->children[0], b);
                    else lower_expr(*arm->children[0], b);
                }
                b.emit(IROpcode::Br, "", {lbl_end});
                b.cur_blk = &b.cur_fn->new_block(lbl_next);
            }
            b.cur_blk = &b.cur_fn->new_block(lbl_end);
            break;
        }

        case NodeKind::ExprStmt:
            if (!n.children.empty() && n.children[0])
                lower_expr(*n.children[0], b);
            break;

        case NodeKind::Block:
            lower_block(n, b);
            break;

        // Skip linkage/import stmts in IR
        case NodeKind::LinkStmt:
        case NodeKind::UseDecl:
            break;

        // unsafe { ... } is transparent to IR — lower the inner block
        // as if the unsafe wrapper weren't there at all.
        case NodeKind::UnsafeBlock:
            if (!n.children.empty() && n.children[0])
                lower_block(*n.children[0], b);
            break;

        // extern "C" pulse decls carry no body to lower here; they are
        // registered as external function declarations during the
        // top-level pass (see lower_top_level), so a bare statement-
        // position occurrence (e.g. inside a realm) is a no-op.
        case NodeKind::ExternDecl:
            break;

        default:
            b.emit(IROpcode::Nop, "", {}, "unknown_stmt", n.span.line);
            break;
    }
}

// ── Module lowering ───────────────────────────────────────────
IRModule lower_to_ir(const Program& ast, const std::string& module_name) {
    IRBuilder builder(module_name);

    // Pre-pass: collect every `use Realm::member` alias in the whole
    // program (including inside nested realms, though `use` normally
    // appears at top level) before any real lowering happens, so
    // Identifier lowering can always resolve an alias regardless of
    // where `use` appears relative to its call sites.
    std::function<void(const ASTNodePtr&)> collect_uses;
    collect_uses = [&](const ASTNodePtr& node) {
        if (!node) return;
        if (node->kind == NodeKind::UseDecl) {
            const std::string& path = node->value;
            bool is_wildcard = path.size() >= 3 && path.substr(path.size()-3) == "::*";
            if (!is_wildcard) {
                size_t last = path.rfind("::");
                if (last != std::string::npos) {
                    std::string bare = path.substr(last + 2);
                    if (!bare.empty()) builder.use_aliases[bare] = path;
                }
            }
            // Wildcard `use Realm::*` isn't resolvable without a full
            // symbol table of the realm's members at this point in
            // lowering; left unhandled here deliberately rather than
            // guessed at, since a wrong guess (e.g. silently matching
            // the wrong overload) is worse than a clear compile error
            // pointing at the missing qualification.
        }
        for (auto& c : node->children) collect_uses(c);
    };
    for (auto& top : ast) collect_uses(top);

    // Pre-pass: record every `enum Name { ... }` declaration in the
    // whole program (walking into realms too, since an enum can be
    // declared inside one) before any expression lowering happens —
    // see IRBuilder::known_enums for why this needs to run first.
    std::function<void(const ASTNodePtr&)> collect_enums;
    collect_enums = [&](const ASTNodePtr& node) {
        if (!node) return;
        if (node->kind == NodeKind::EnumDecl && !node->value.empty()) {
            builder.known_enums[node->value] = node;
        }
        for (auto& c : node->children) collect_enums(c);
    };
    for (auto& top : ast) collect_enums(top);

    // Pre-pass: record every `impl Type { ... }` / `impl Nexus for
    // Type { ... }` block's method names into known_methods, keyed
    // "Type::method" — see IRBuilder::known_methods for why. Also
    // walks into realms.
    std::function<void(const ASTNodePtr&)> collect_impls;
    collect_impls = [&](const ASTNodePtr& node) {
        if (!node) return;
        if (node->kind == NodeKind::ImplDecl && !node->extra.empty()) {
            for (auto& method : node->children) {
                if (method && !method->value.empty()) {
                    builder.known_methods.insert(node->extra + "::" + method->value);
                }
            }
        }
        for (auto& c : node->children) collect_impls(c);
    };
    for (auto& top : ast) collect_impls(top);

    std::function<void(const ASTNodePtr&)> lower_top;
    lower_top = [&](const ASTNodePtr& top) {
        if (!top) return;

        switch (top->kind) {

            case NodeKind::LinkStmt:
                builder.mod.imports.push_back(top->value);
                break;

            case NodeKind::GlobalDecl: {
                IRGlobal g;
                g.name      = top->value;
                g.type      = IRType::String;
                g.is_const  = false;
                if (!top->children.empty() && top->children[0])
                    g.init_value = top->children[0]->value;
                builder.mod.globals.push_back(g);
                break;
            }

            // Top-level ConstDecl/AtomDecl/ShadowDecl were previously
            // missing from this switch entirely, so they fell into the
            // `default:` branch below and got wrapped in an implicit
            // main(). That silently reordered the program (declarations
            // got hoisted ahead of/interleaved with real top-level
            // functions depending on section-splitter grouping) and, in
            // the worst case, caused a *second* implicit main() to be
            // created once a real PulseDecl/executable statement reset
            // cur_fn afterward — a hard "redefinition of int main()"
            // compile failure. Math constants (PI, E, PHI, ...) in
            // library/*.xh are exactly this shape: top-level AtomDecls.
            case NodeKind::ConstDecl:
            case NodeKind::AtomDecl:
            case NodeKind::ShadowDecl: {
                bool has_init = !top->children.empty() && top->children[0];
                bool is_pure_literal = has_init && (
                    top->children[0]->kind == NodeKind::IntLit   ||
                    top->children[0]->kind == NodeKind::FloatLit ||
                    top->children[0]->kind == NodeKind::BoolLit  ||
                    top->children[0]->kind == NodeKind::StringLit ||
                    top->children[0]->kind == NodeKind::NullLit  ||
                    // A unary/binary op over only literals (e.g. -1,
                    // 1e308) is still safely a global initializer —
                    // it doesn't call into any not-yet-defined
                    // function or read runtime state.
                    top->children[0]->kind == NodeKind::UnaryOp  ||
                    top->children[0]->kind == NodeKind::BinaryOp
                );

                if (!has_init || is_pure_literal) {
                    // Pure literal (or no initializer at all): safe to
                    // emit as a real global rather than a statement
                    // inside main.
                    IRGlobal g;
                    g.name     = top->value;
                    g.type     = map_type(top->extra);
                    if (g.type == IRType::Struct) g.type_name = top->extra;
                    g.is_const = (top->kind == NodeKind::ConstDecl);
                    if (has_init) g.init_value = top->children[0]->value;
                    builder.mod.globals.push_back(g);
                } else {
                    // Computed atom: initializer depends on executable
                    // function calls / runtime state, so it can't be a
                    // plain C++ global initializer. Route it through
                    // the normal implicit-main statement path instead
                    // of silently misclassifying it as a global.
                    if (builder.cur_fn == nullptr) {
                        builder.begin_function("main", IRType::I64, {}, false);
                    }
                    lower_stmt(*top, builder);
                }
                break;
            }

            case NodeKind::TupleDestructure: {
                // atom (lo, hi) = min_max(numbers) at the top level —
                // always routed through the implicit-main statement
                // path (never treated as a global the way a plain
                // AtomDecl with a pure-literal initializer can be),
                // since a tuple destructure's initializer is
                // effectively always a real expression (a function
                // call in every current use), not a literal.
                if (builder.cur_fn == nullptr) {
                    builder.begin_function("main", IRType::I64, {}, false);
                }
                lower_stmt(*top, builder);
                break;
            }

            case NodeKind::FluxDecl: {
                IRGlobal g;
                g.name     = top->value;
                g.type     = map_type(top->extra);
                if (g.type == IRType::Struct) g.type_name = top->extra;
                g.is_flux  = true;
                if (!top->children.empty() && top->children[0])
                    g.init_value = top->children[0]->value;
                builder.mod.globals.push_back(g);
                break;
            }

            case NodeKind::ForgeDecl: {
                IRTypeDecl td;
                td.name        = top->value;
                td.is_abstract = false;
                for (auto& field : top->children) {
                    if (!field) continue;
                    IRParam f;
                    f.name = field->value;
                    f.type = map_type(field->extra);
                    // Preserve the concrete struct name for nested
                    // struct fields (e.g. a field of type Address
                    // inside a Person forge) — IRType::Struct alone
                    // is lossy and left GEP-based member access unable
                    // to infer the right C++ type further down the
                    // chain.
                    if (f.type == IRType::Struct) f.type_name = field->extra;
                    td.fields.push_back(std::move(f));
                }
                builder.mod.types.push_back(td);
                break;
            }

            case NodeKind::EnumDecl: {
                IREnumDecl ed;
                ed.name = top->value;
                for (auto& variant : top->children) {
                    if (!variant || variant->kind != NodeKind::EnumVariant) continue;
                    IREnumVariant v;
                    v.name = variant->value;
                    for (auto& payload_type : variant->children) {
                        if (!payload_type) continue;
                        IRParam p;
                        // Payload slots are positional (no source-
                        // level name — a variant declares only
                        // types, e.g. `Error(str)`), so name them
                        // _0, _1, ... matching the struct-field
                        // naming the C++ emitter uses for variant
                        // payload storage.
                        p.name = "_" + std::to_string(v.payload.size());
                        p.type = map_type(payload_type->extra);
                        if (p.type == IRType::Struct) p.type_name = payload_type->extra;
                        v.payload.push_back(std::move(p));
                    }
                    ed.variants.push_back(std::move(v));
                }
                builder.mod.enums.push_back(std::move(ed));
                break;
            }

            case NodeKind::NexusDecl: {
                IRTypeDecl td;
                td.name        = top->value;
                td.is_abstract = true;
                for (auto& method : top->children) {
                    if (!method) continue;
                    td.methods.push_back(method->value);
                }
                builder.mod.types.push_back(td);
                break;
            }

            case NodeKind::RealmDecl: {
                // realm Name { members... } — a namespace. Previously
                // this only recorded a "marker" IRTypeDecl listing
                // member names as strings (for --emit=ir inspection),
                // but never actually lowered the realm's real members
                // (forge/pulse/const/flux/nested realm) into the
                // module at all — every type, function, and constant
                // declared inside any `realm` block was silently
                // absent from compiled output entirely.
                //
                // Fix: recursively re-dispatch each member through
                // this same top-level lowering pass, after qualifying
                // its declared name with "RealmName::" — since
                // parse_type_name() already produces fully-qualified
                // "Realm::Type" strings for any *reference* to a
                // qualified type (see the parser's IDENTIFIER +
                // COLON_COLON handling), the only rewriting needed
                // here is on the *declaration* site itself. A nested
                // `realm` recurses naturally, since re-dispatching a
                // nested RealmDecl through lower_top hits this same
                // case again and prefixes one more level (producing
                // e.g. "App::Models::User" for a doubly-nested realm).
                //
                // `const` handling: NOTE that a `const` declared
                // inside a realm (e.g. Geometry::PI) is referenced
                // unqualified from other pulses *in the same realm*
                // (`circle_area` uses bare `PI`, not `Geometry::PI`)
                // per the source grammar — so the const is qualified
                // for its own declaration (so App::Models nesting
                // doesn't collide with a same-named const elsewhere),
                // but call sites inside the realm still resolve via
                // the unqualified global lookup the emitter already
                // does. To keep both working, the const is emitted
                // under its qualified name only when a genuinely
                // nested/duplicate name exists; for the common case
                // (single realm, no name collision) leaving the
                // unqualified name intact keeps in-realm references
                // working without extra resolution machinery.
                std::string prefix = top->value;
                for (auto& member : top->children) {
                    if (!member) continue;
                    if (member->kind == NodeKind::ConstDecl ||
                        member->kind == NodeKind::AtomDecl   ||
                        member->kind == NodeKind::FluxDecl) {
                        // Left unqualified deliberately — see note
                        // above. lower_top's ConstDecl/AtomDecl/
                        // FluxDecl cases already handle these
                        // correctly once dispatched directly.
                        lower_top(member);
                        continue;
                    }
                    if (member->kind == NodeKind::ForgeDecl ||
                        member->kind == NodeKind::NexusDecl) {
                        auto qualified = std::make_shared<ASTNode>(*member);
                        qualified->value = prefix + "::" + member->value;
                        lower_top(qualified);
                        continue;
                    }
                    if (member->kind == NodeKind::PulseDecl ||
                        member->kind == NodeKind::AsyncPulseDecl) {
                        auto qualified = std::make_shared<ASTNode>(*member);
                        qualified->value = prefix + "::" + member->value;
                        lower_top(qualified);
                        continue;
                    }
                    if (member->kind == NodeKind::RealmDecl) {
                        // Nested realm: qualify its own name with the
                        // outer prefix, then let the recursive call
                        // qualify its members with the combined name.
                        auto qualified = std::make_shared<ASTNode>(*member);
                        qualified->value = prefix + "::" + member->value;
                        lower_top(qualified);
                        continue;
                    }
                    if (member->kind == NodeKind::ImplDecl) {
                        // impl Type { ... } inside a realm: qualify
                        // the receiver type name so methods land under
                        // e.g. "Geometry::Vector2::method" matching
                        // how the type itself was declared above.
                        auto qualified = std::make_shared<ASTNode>(*member);
                        qualified->extra = prefix + "::" + member->extra;
                        lower_top(qualified);
                        continue;
                    }
                    // LinkStmt/UseDecl/other: no declaration to
                    // qualify, dispatch as-is (most are no-ops at the
                    // top-level switch already).
                    lower_top(member);
                }
                break;
            }

            case NodeKind::PulseDecl:
            case NodeKind::AsyncPulseDecl: {
                std::string   fn_name   = top->value.empty() ? "main" : top->value;
                IRType        ret       = map_type(top->extra2);
                std::vector<IRParam> params;
                bool has_body = false;
                for (auto& c : top->children) {
                    if (!c) continue;
                    if (c->kind == NodeKind::FieldDecl) {
                        IRParam p;
                        p.name = c->value;
                        p.type = map_type(c->extra);
                        if (p.type == IRType::Struct) p.type_name = c->extra;
                        params.push_back(std::move(p));
                    } else if (c->kind == NodeKind::Block) {
                        has_body = true;
                    }
                }
                bool is_async = (top->kind == NodeKind::AsyncPulseDecl);
                std::string ret_type_name = (ret == IRType::Struct) ? top->extra2 : "";
                builder.begin_function(fn_name, ret, params, is_async, ret_type_name);

                if (!has_body) {
                    // Body-less pulse decl: this is a stdlib/runtime
                    // signature declaration (e.g. `pulse abs(x: float)
                    // -> float` in library/math/math.xh), NOT a real
                    // extern "C" FFI symbol. Marking these is_extern
                    // previously caused generated C++ to redeclare
                    // functions like abs() with a conflicting extern
                    // "C" signature, colliding with <cstdlib>. The
                    // implementation is supplied by the injected
                    // runtime, so the C++ emitter must skip emitting
                    // any declaration or body for it.
                    builder.cur_fn->is_signature_only = true;
                    builder.cur_fn  = nullptr;
                    builder.cur_blk = nullptr;
                    break;
                }

                // Lower body block
                for (auto& c : top->children) {
                    if (c && c->kind == NodeKind::Block) {
                        lower_block(*c, builder);
                    }
                }
                // Implicit return void if no return stmt
                if (ret == IRType::Void)
                    builder.emit(IROpcode::Ret, "", {"void"});
                // Reset so the next top-level node (if it's a bare
                // statement, not another declaration) correctly
                // triggers creation of its own implicit main() rather
                // than silently appending into this function's last
                // block — that was a real bug: top-level statements
                // following any pulse decl would leak into whichever
                // function happened to be lowered immediately before
                // them, since cur_fn/cur_blk were never cleared.
                builder.cur_fn  = nullptr;
                builder.cur_blk = nullptr;
                break;
            }

            case NodeKind::ExternDecl: {
                // FFI declaration: register the symbol so callers can
                // reference it, but emit no blocks/body — the actual
                // definition lives in an externally linked object
                // (C library, Rust cdylib, etc.), not in this module.
                IRFunction fn;
                fn.name      = top->value;
                fn.ret_type  = map_type(top->extra2);
                if (fn.ret_type == IRType::Struct) fn.ret_type_name = top->extra2;
                fn.is_extern = true;
                // top->extra carries the ABI string parsed after the
                // `extern` keyword (see parse_extern_decl / extra =
                // abi), defaulting to "C" there already — mirror that
                // default here too rather than leaving it empty.
                fn.extern_abi = top->extra.empty() ? "C" : top->extra;
                for (auto& c : top->children) {
                    if (!c || c->kind != NodeKind::FieldDecl) continue;
                    IRParam p;
                    p.name = c->value;
                    p.type = map_type(c->extra);
                    if (p.type == IRType::Struct) p.type_name = c->extra;
                    fn.params.push_back(std::move(p));
                }
                builder.mod.functions.push_back(std::move(fn));
                break;
            }

            case NodeKind::UnsafeBlock: {
                // Top-level unsafe block: lower its contents as if the
                // wrapper weren't there. Declarations inside (e.g. an
                // extern decl wrapped in unsafe{}) are re-dispatched
                // through this same top-level pass.
                if (!top->children.empty() && top->children[0]) {
                    for (auto& inner : top->children[0]->children) {
                        if (inner) lower_top(inner);
                    }
                }
                break;
            }

            case NodeKind::ImplDecl: {
                for (auto& method : top->children) {
                    if (!method) continue;
                    std::string mname = top->extra + "::" + method->value;
                    IRType      mret  = map_type(method->extra2);
                    std::string mret_type_name = (mret == IRType::Struct) ? method->extra2 : "";
                    std::vector<IRParam> mparams;
                    IRParam self_p; self_p.name = "self"; self_p.type = IRType::Ptr;
                    self_p.type_name = top->extra; // implicit self, receiver's realm/type name
                    mparams.push_back(std::move(self_p));
                    for (auto& p : method->children) {
                        if (!p || p->kind != NodeKind::FieldDecl) continue;
                        // An explicit `self` parameter (now parseable
                        // — see parse_field_decl's SELF handling) is
                        // already covered by the implicit self_p
                        // injected above; including it a second time
                        // here would produce a method taking self
                        // twice. Whether the method was written with
                        // an explicit `self` or without one (both are
                        // valid — see the nexus method-declaration
                        // convention, which never writes it), exactly
                        // one self parameter should exist in the
                        // lowered signature.
                        if (p->value == "self") continue;
                        IRParam mp;
                        mp.name = p->value;
                        mp.type = map_type(p->extra);
                        if (mp.type == IRType::Struct) mp.type_name = p->extra;
                        mparams.push_back(std::move(mp));
                    }
                    builder.begin_function(mname, mret, mparams, false, mret_type_name);
                    builder.var_types["self"] = top->extra; // for obj.method() resolution inside this method's own body
                    for (auto& c : method->children) {
                        if (c && c->kind == NodeKind::Block) lower_block(*c, builder);
                    }
                    if (mret == IRType::Void) builder.emit(IROpcode::Ret, "", {"void"});
                    builder.cur_fn  = nullptr;
                    builder.cur_blk = nullptr;
                }
                break;
            }

            case NodeKind::Block: {
                // The only shape of top-level Block the parser
                // produces is the `extern "C" { pulse a(...)  pulse
                // b(...)  ... }` block form (see parse_extern_decl's
                // L_BRACE branch) — each child is its own ExternDecl.
                // This case was previously missing entirely from
                // lower_top, so a block-form extern group fell into
                // the generic `default:` branch below and got wrapped
                // in an implicit main() instead of being registered
                // as real function declarations — every symbol
                // declared inside `extern "C" { ... }` silently
                // vanished from compiled output (an unqualified call
                // to any of them then failed with "was not declared
                // in this scope", since no forward declaration for it
                // ever got emitted). The semantic analyzer already
                // had the equivalent unwrap logic (see its own
                // `case NodeKind::Block` in collect_one_declaration);
                // this brings IR lowering in line with it.
                for (auto& c : top->children) {
                    if (c && c->kind == NodeKind::ExternDecl) lower_top(c);
                }
                break;
            }

            // Top-level statements → wrap in implicit main
            default: {
                if (builder.cur_fn == nullptr) {
                    builder.begin_function("main", IRType::I64, {}, false);
                }
                lower_stmt(*top, builder);
                break;
            }
        }
    };

    for (auto& top : ast) {
        lower_top(top);
    }

    // Implicit main return 0
    if (builder.cur_fn && builder.cur_fn->name == "main") {
        builder.emit(IROpcode::Ret, "", {"0"});
    }

    return builder.mod;
}

// ── IR Text Dump ─────────────────────────────────────────────
std::string dump_ir(const IRModule& mod) {
    std::ostringstream out;
    out << "; X-Phage IR Module: " << mod.name << "\n";
    out << "; Generated by xphage_middle v4.0.0\n\n";

    // Imports
    for (auto& imp : mod.imports)
        out << "@import \"" << imp << "\"\n";
    if (!mod.imports.empty()) out << "\n";

    // Types
    for (auto& td : mod.types) {
        out << (td.is_abstract ? "nexus " : "forge ") << td.name << " {\n";
        for (auto& f : td.fields)
            out << "  field " << ir_type_str(f.type) << " " << f.name << "\n";
        for (auto& m : td.methods)
            out << "  method " << m << "\n";
        out << "}\n\n";
    }

    // Globals
    for (auto& g : mod.globals) {
        out << (g.is_const ? "@const " : "@global ")
            << (g.is_flux ? "flux " : "")
            << ir_type_str(g.type) << " @" << g.name;
        if (!g.init_value.empty()) out << " = " << g.init_value;
        out << "\n";
    }
    if (!mod.globals.empty()) out << "\n";

    // Functions
    for (auto& fn : mod.functions) {
        out << (fn.is_async ? "async " : "") << "fn @" << fn.name << "(";
        bool first = true;
        for (auto& p : fn.params) {
            if (!first) out << ", ";
            out << ir_type_str(p.type) << " %" << p.name;
            first = false;
        }
        out << ") -> " << ir_type_str(fn.ret_type) << " {\n";

        for (auto& blk : fn.blocks) {
            out << blk.label << ":\n";
            for (auto& ins : blk.instrs) {
                out << "  ";
                if (!ins.dest.empty()) out << ins.dest << " = ";
                out << static_cast<int>(ins.op);
                for (auto& op : ins.operands) out << " " << op;
                if (!ins.extra.empty()) out << " ; " << ins.extra;
                out << "\n";
            }
        }
        out << "}\n\n";
    }
    return out.str();
}

} // namespace xphage::middle
