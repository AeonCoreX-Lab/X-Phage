#include "../include/monomorphize.hpp"
#include <unordered_map>
#include <unordered_set>
#include <functional>
#include <sstream>

namespace xphage::generics {

namespace {

// Deep-clones an AST subtree. Needed because ASTNode's default copy
// constructor only shallow-copies `children` (a vector of
// shared_ptr<ASTNode> — copying the vector copies the pointers, not
// the pointees), so two specializations of the same generic
// declaration would otherwise share the same child nodes and any
// mutation made to one (the type-substitution walk below) would
// silently corrupt the other.
ASTNodePtr deep_clone(const ASTNodePtr& n) {
    if (!n) return nullptr;
    auto copy = std::make_shared<ASTNode>(*n); // shallow copy of scalar fields + child pointers
    copy->children.clear();
    for (auto& c : n->children) copy->children.push_back(deep_clone(c));
    return copy;
}

// Replaces every occurrence of a type-parameter name with its
// concrete substitution, wherever a type name can appear: a
// TypeAnnot/FieldDecl's `extra` (parameter/field type), a PulseDecl's
// `extra2` (return type), and — for a qualified/generic type
// reference like "Vec<T>" — as a substring match within a larger type
// string, not just an exact match, since parse_type_name() builds
// composite type strings by string concatenation (see its own
// comment). A whole-word substring replace (not literal
// find-and-replace of any substring) avoids mangling an unrelated
// type that merely contains the parameter name as a substring (e.g.
// substituting "T" must not corrupt "Team" or "TreeNode").
std::string substitute_type_string(const std::string& type_str,
                                    const std::unordered_map<std::string, std::string>& subst) {
    if (subst.empty() || type_str.empty()) return type_str;
    std::string result;
    result.reserve(type_str.size());
    size_t i = 0;
    auto is_ident_char = [](char c) {
        return std::isalnum(static_cast<unsigned char>(c)) || c == '_';
    };
    while (i < type_str.size()) {
        if (is_ident_char(type_str[i]) &&
            (i == 0 || !is_ident_char(type_str[i - 1]))) {
            size_t start = i;
            while (i < type_str.size() && is_ident_char(type_str[i])) i++;
            std::string word = type_str.substr(start, i - start);
            auto it = subst.find(word);
            result += (it != subst.end()) ? it->second : word;
        } else {
            result += type_str[i];
            i++;
        }
    }
    return result;
}

// Walks an entire cloned subtree, substituting type-parameter names
// in every field that can hold a type string (FieldDecl/TypeAnnot's
// extra, PulseDecl's extra2, EnumVariant payload types, ImplDecl's
// extra for a generic impl's receiver type — though generic impl
// itself isn't attempted in this pass, see the header's own note on
// scope). Does NOT touch `value` (names/identifiers/operators are
// never type strings in this AST's convention) except for the one
// deliberate case of a SpawnExpr's `value` (the constructed type's
// name, e.g. "Box" or "Box<T>" for a not-yet-specialized generic
// spawn) and a CallExpr whose callee is a generic function's own
// name — both handled separately at the call/construction-site
// rewrite step below, not by this generic walk, since substituting
// those correctly requires knowing which concrete specialization was
// selected, not just which type-parameter names exist.
void substitute_types_in_subtree(const ASTNodePtr& n,
                                  const std::unordered_map<std::string, std::string>& subst) {
    if (!n) return;
    if (!n->extra.empty())  n->extra  = substitute_type_string(n->extra, subst);
    if (!n->extra2.empty()) n->extra2 = substitute_type_string(n->extra2, subst);
    for (auto& c : n->children) substitute_types_in_subtree(c, subst);
}

// One generic declaration's essential info, collected up front.
struct GenericDecl {
    ASTNodePtr           node;         // the original PulseDecl/ForgeDecl/EnumDecl
    std::vector<std::string> param_names; // e.g. ["T"] or ["T", "E"]
};

// A specific instantiation already generated, keyed by (generic name,
// concrete type-argument tuple joined with "|") so the same
// specialization is never generated twice even if called from many
// sites — e.g. identity(1) and identity(2) both want T=int and should
// share one identity__int, not get two separately-generated (but
// identical) copies.
std::string mangle_name(const std::string& base, const std::vector<std::string>& type_args) {
    std::string m = base;
    for (auto& t : type_args) {
        m += "__";
        for (char c : t) m += (std::isalnum(static_cast<unsigned char>(c)) ? c : '_');
    }
    return m;
}

// Infers concrete type arguments for a call/construction site,
// mirroring (deliberately kept in sync with, though not literally
// shared code with — see the header's design-rationale comment on
// why monomorphization is a separate pass rather than folded into
// semantic analysis) SemanticAnalyzer::check_call's own inference:
// walks each parameter position that references a type-parameter
// name, and for a CallExpr pulls the corresponding argument's
// concrete type from a caller-supplied per-node type map (populated
// during a lightweight local re-inference here, since this pass runs
// independently of the semantic analyzer's own internal state).
std::string infer_arg_type(const ASTNodePtr& arg) {
    if (!arg) return "";
    switch (arg->kind) {
        case NodeKind::IntLit:   return "int";
        case NodeKind::FloatLit: return "float";
        case NodeKind::BoolLit:  return "bool";
        case NodeKind::StringLit:
        case NodeKind::FStringLit: return "str";
        case NodeKind::SpawnExpr: return arg->value;
        default: return ""; // identifiers and anything else: not
                             // reliably inferable without a real
                             // symbol table here; a generic call using
                             // a variable of unannotated type simply
                             // won't be monomorphized (see the
                             // fallback behavior in monomorphize()
                             // below, which leaves such a call
                             // untouched rather than guessing).
    }
}

} // namespace

MonomorphizeResult monomorphize(const Program& prog, const Program& library_decls) {
    MonomorphizeResult result;

    // Pass 1: collect every generic declaration (has at least one
    // TypeParamDecl child), from both the program itself and any
    // resolved stdlib declarations passed alongside it. Also collect
    // every (non-generic-declaration-site) PulseDecl's own declared
    // return type by name — needed so `atom found = find_user(42)`
    // followed by `probe found { diverge Option.Some(user) -> ... }`
    // can resolve found's type back to find_user's return type, since
    // the probe's subject is just a plain identifier with no type
    // annotation of its own anywhere nearby.
    std::unordered_map<std::string, GenericDecl> generics;
    std::unordered_map<std::string, std::string> function_return_types; // function name -> declared return type string
    // Generic type's base name -> its impl<T> block(s) (usually one,
    // but nothing prevents `impl<T> Box<T> { ... }` appearing more
    // than once in different files/sections the way an ordinary
    // non-generic impl can). Each ImplDecl node here still has its
    // own TypeParamDecl children (not stripped) and still names its
    // receiver as the generic form ("Box<T>", not yet substituted) —
    // substitution happens per-specialization in
    // instantiate_generic_type, the same place the forge/enum's own
    // fields get substituted.
    std::unordered_map<std::string, std::vector<ASTNodePtr>> generic_impls;
    std::function<void(const ASTNodePtr&)> collect;
    collect = [&](const ASTNodePtr& n) {
        if (!n) return;
        if (n->kind == NodeKind::PulseDecl || n->kind == NodeKind::AsyncPulseDecl ||
            n->kind == NodeKind::ForgeDecl || n->kind == NodeKind::EnumDecl) {
            std::vector<std::string> params;
            for (auto& c : n->children) {
                if (c && c->kind == NodeKind::TypeParamDecl) params.push_back(c->value);
            }
            if (!params.empty() && !n->value.empty()) {
                generics[n->value] = GenericDecl{n, std::move(params)};
            }
            if ((n->kind == NodeKind::PulseDecl || n->kind == NodeKind::AsyncPulseDecl) &&
                !n->value.empty() && !n->extra2.empty()) {
                function_return_types[n->value] = n->extra2;
            }
        }
        if (n->kind == NodeKind::ImplDecl && !n->extra.empty()) {
            bool has_type_params = false;
            for (auto& c : n->children) if (c && c->kind == NodeKind::TypeParamDecl) has_type_params = true;
            if (has_type_params) {
                size_t lt = n->extra.find('<');
                std::string base = (lt == std::string::npos) ? n->extra : n->extra.substr(0, lt);
                generic_impls[base].push_back(n);
            }
        }
        for (auto& c : n->children) collect(c);
    };
    for (auto& n : prog) collect(n);
    for (auto& n : library_decls) collect(n);

    if (generics.empty()) {
        // Nothing generic anywhere — return the program unchanged
        // (still deep-cloned, so the caller owns an independent copy
        // consistent with what this pass does when it DOES rewrite
        // something, rather than a mix of "sometimes clones,
        // sometimes doesn't" depending on program content).
        for (auto& n : prog) result.program.push_back(deep_clone(n));
        return result;
    }

    // Pass 2: walk the whole program (deep-cloned, so rewriting is
    // safe), find every call/construction site referencing a generic
    // declaration, infer its concrete type arguments, and generate
    // (once per distinct name+type-argument combination) a
    // specialization — recording it for insertion after the
    // rewrite walk finishes, and rewriting the call/construction site
    // in place to reference the specialization's mangled name.
    std::unordered_map<std::string, ASTNodePtr> specializations; // mangled name -> cloned+substituted decl
    std::unordered_set<std::string> generic_names_used;          // original generic names that got >=1 real instantiation
    // Variable name -> its declared/inferred concrete type, tracked
    // as the rewrite walk encounters each atom/shadow declaration (in
    // program order — this is a simple linear scan, not real scoping,
    // so a variable shadowed in a nested block with a different type
    // isn't modeled correctly; every current test case is flat enough
    // that this doesn't matter, and getting it wrong just means a
    // probe pattern fails to resolve, which surfaces as a normal
    // downstream error rather than silently generating wrong code).
    // Needed so `atom found = find_user(42)` followed later by `probe
    // found { diverge Option.Some(user) -> ... }` can resolve found's
    // type back through find_user's own (already-instantiated, via
    // function_return_types) return type.
    std::unordered_map<std::string, std::string> var_types;

    // Given a type string that might be a generic instantiation
    // (e.g. "Option<User>", "Result<Config, str>"), and assuming its
    // base name IS a known generic ForgeDecl/EnumDecl, generates (if
    // not already generated) the corresponding specialization and
    // returns its mangled name — or, if type_str isn't a generic
    // instantiation of a known generic type at all, returns type_str
    // unchanged. This is the single shared entry point for every
    // place a concrete generic type name can appear outside of a
    // call/spawn expression: a function's declared return type
    // (PulseDecl.extra2), a parameter's declared type
    // (FieldDecl.extra), or an explicit local annotation
    // (AtomDecl/ShadowDecl.extra) — e.g. `atom found: Option<User> =
    // find_user(42)`. Reusable rather than duplicating the same
    // parse-args/build-subst/clone-and-substitute logic the
    // SpawnExpr case above already does inline, since that logic is
    // identical regardless of which AST field the type string came
    // from.
    std::function<std::string(const std::string&)> instantiate_generic_type;
    instantiate_generic_type = [&](const std::string& type_str) -> std::string {
        size_t lt = type_str.find('<');
        if (lt == std::string::npos || type_str.back() != '>') return type_str;
        std::string base = type_str.substr(0, lt);
        auto git = generics.find(base);
        if (git == generics.end() ||
            (git->second.node->kind != NodeKind::ForgeDecl &&
             git->second.node->kind != NodeKind::EnumDecl)) {
            return type_str; // not a generic type name this pass knows about
        }
        std::string args_str = type_str.substr(lt + 1, type_str.size() - lt - 2);
        std::vector<std::string> type_args;
        std::string cur;
        int depth = 0;
        for (char c : args_str) {
            if (c == '<') depth++;
            if (c == '>') depth--;
            if (c == ',' && depth == 0) { type_args.push_back(cur); cur.clear(); }
            else if (c != ' ') cur += c;
        }
        if (!cur.empty()) type_args.push_back(cur);
        auto& gd = git->second;
        if (type_args.size() != gd.param_names.size()) return type_str; // arity mismatch — leave as-is, will fail downstream with a clearer error than guessing
        std::unordered_map<std::string, std::string> subst;
        for (size_t i = 0; i < gd.param_names.size(); i++) subst[gd.param_names[i]] = type_args[i];
        std::string mangled = mangle_name(base, type_args);
        if (!specializations.count(mangled)) {
            auto spec = deep_clone(gd.node);
            spec->value = mangled;
            std::vector<ASTNodePtr> kept;
            for (auto& c : spec->children) {
                if (!c || c->kind != NodeKind::TypeParamDecl) kept.push_back(c);
            }
            spec->children = std::move(kept);
            substitute_types_in_subtree(spec, subst);
            specializations[mangled] = spec;

            // If this generic type has associated impl<T> block(s)
            // (impl<T> Box<T> { ... }), generate a matching concrete
            // impl for this same specialization: clone each one,
            // substitute T -> the concrete type throughout (method
            // bodies included — a method like `get(self) -> T` needs
            // its own return type substituted the same way the
            // forge/enum's own fields were above), and rename its
            // receiver (`.extra`) to the mangled specialization name
            // so emit_forge's impl_methods_ lookup (keyed by the
            // forge's own, now-mangled, name) finds it. Without this,
            // a generic type's methods would never be instantiated at
            // all — only its fields would be, since ForgeDecl and
            // ImplDecl are entirely separate AST nodes with no
            // structural link between them beyond sharing a type
            // name.
            auto git_impls = generic_impls.find(base);
            if (git_impls != generic_impls.end()) {
                for (auto& impl_node : git_impls->second) {
                    std::string impl_key = mangled + "::impl:" +
                        std::to_string(reinterpret_cast<uintptr_t>(impl_node.get()));
                    if (specializations.count(impl_key)) continue;
                    auto impl_spec = deep_clone(impl_node);
                    impl_spec->extra = mangled;
                    std::vector<ASTNodePtr> ikept;
                    for (auto& c : impl_spec->children) {
                        if (!c || c->kind != NodeKind::TypeParamDecl) ikept.push_back(c);
                    }
                    impl_spec->children = std::move(ikept);
                    substitute_types_in_subtree(impl_spec, subst);
                    specializations[impl_key] = impl_spec;
                }
            }
        }
        generic_names_used.insert(base);
        return mangled;
    };

    std::function<void(ASTNodePtr&, const std::string&)> rewrite;
    rewrite = [&](ASTNodePtr& n, const std::string& enclosing_return_type) {
        if (!n) return;

        // A function body's own enclosing return-type context is set
        // once here (after this node's own extra2 has possibly been
        // instantiated to a concrete mangled type below) and threaded
        // down through every descendant — this is what lets a bare
        // `return Option.Some(user)` inside a `pulse find_user(id:
        // int) -> Option<User>` resolve which Option<X> specialization
        // to construct, since the construction site itself never
        // names a type argument at all (only "Option.Some", not
        // "Option<User>.Some").
        std::string next_context = enclosing_return_type;
        if (n->kind == NodeKind::PulseDecl || n->kind == NodeKind::AsyncPulseDecl) {
            if (!n->extra2.empty()) {
                n->extra2 = instantiate_generic_type(n->extra2);
                next_context = n->extra2;
            } else {
                next_context = "";
            }
        }
        for (auto& c : n->children) rewrite(c, next_context);

        if (n->kind == NodeKind::FieldDecl || n->kind == NodeKind::AtomDecl ||
            n->kind == NodeKind::ShadowDecl) {
            if (!n->extra.empty()) n->extra = instantiate_generic_type(n->extra);
        }

        // Track this variable's concrete type, for a later ProbeStmt
        // (or anything else that needs "what type is this identifier"
        // — currently only ProbeStmt uses var_types) to resolve
        // against. Explicit annotation wins if present (already
        // instantiated above); otherwise, if the initializer is a
        // direct call to a function whose return type is known
        // (already instantiated, via function_return_types — note
        // this map holds the *original* declared return type string,
        // not retroactively updated when that function's own extra2
        // gets rewritten earlier in this same walk, since collection
        // happens once up front in Pass 1 — but since
        // instantiate_generic_type is idempotent on an
        // already-mangled string, and idempotent-no-op on any
        // non-generic type string, looking the original declared
        // return type up here and re-running it through
        // instantiate_generic_type produces the correct mangled name
        // either way).
        if ((n->kind == NodeKind::AtomDecl || n->kind == NodeKind::ShadowDecl) && !n->value.empty()) {
            std::string vtype;
            if (!n->extra.empty()) {
                vtype = n->extra;
            } else if (!n->children.empty() && n->children[0] &&
                       n->children[0]->kind == NodeKind::CallExpr &&
                       !n->children[0]->children.empty() && n->children[0]->children[0] &&
                       n->children[0]->children[0]->kind == NodeKind::Identifier) {
                // Look up by the call's ORIGINAL callee name if it's
                // already been rewritten to a monomorphized function
                // name (e.g. "identity__int") — function_return_types
                // is keyed by the original generic name, so try that
                // first via a reverse mangle-strip; simplest robust
                // approach is to just check both the current (possibly
                // already-rewritten) callee name and, if that misses,
                // fall through with no type (rather than trying to
                // unmangle).
                auto frt = function_return_types.find(n->children[0]->children[0]->value);
                if (frt != function_return_types.end()) {
                    vtype = instantiate_generic_type(frt->second);
                }
            }
            if (!vtype.empty()) var_types[n->value] = vtype;
        }

        // probe/diverge on an enum-typed subject: rewrite each arm's
        // "EnumName.Variant"/"EnumName.Variant(binding)" pattern to
        // "MangledName.Variant" the same way a construction site
        // does, using the probe's subject's tracked type (var_types,
        // populated just above) as the source of the concrete type
        // argument — a probe pattern names no type argument of its
        // own at all (see parse_probe_stmt), so this is the only
        // place that information can come from for a bare identifier
        // subject. A subject that ISN'T a plain identifier (e.g. a
        // direct `probe find_user(42) { ... }`) isn't resolved here;
        // narrower support than the fully general case, consistent
        // with the header's documented scope.
        if (n->kind == NodeKind::ProbeStmt && !n->children.empty() && n->children[0] &&
            n->children[0]->kind == NodeKind::Identifier) {
            auto vit = var_types.find(n->children[0]->value);
            if (vit != var_types.end() && !vit->second.empty()) {
                for (size_t i = 1; i < n->children.size(); i++) {
                    auto& arm = n->children[i];
                    if (!arm || arm->kind != NodeKind::ProbeArm) continue;
                    size_t dot = arm->value.find('.');
                    if (dot == std::string::npos) continue;
                    std::string pattern_enum = arm->value.substr(0, dot);
                    auto git2 = generics.find(pattern_enum);
                    if (git2 == generics.end() || git2->second.node->kind != NodeKind::EnumDecl) continue;
                    std::string prefix = pattern_enum + "__";
                    if (vit->second.compare(0, prefix.size(), prefix) != 0) continue;
                    arm->value = vit->second + arm->value.substr(dot);
                }
            }
        }

        // Generic enum-variant construction with no explicit type
        // argument at the construction site itself — `Option.Some
        // (user)` / `Option.None`, as opposed to `spawn Option<User>
        // {...}` which isn't valid syntax for an enum anyway (enums
        // construct via EnumName.Variant(...), not spawn — see the
        // enum work in an earlier session). The only type-argument
        // signal available is the enclosing function's own
        // (already-instantiated, per next_context above) return type,
        // when it names the same generic enum being constructed —
        // exactly the book's own documented `Option<T>`/`Result<T,E>`
        // usage pattern (§9.3): the concrete type is announced once,
        // in the function signature, and every `return Option.Some
        // (...)`/`return Option.None` inside that function's body
        // shares it. A construction site with no such enclosing
        // context (e.g. a generic enum constructed and immediately
        // assigned to an explicitly-annotated local — `atom x:
        // Option<User> = Option.Some(u)` — rather than returned) is
        // narrower support than the full book example needs, but was
        // not attempted here; see the header's scope note.
        auto try_rewrite_variant_construction = [&](const ASTNodePtr& member_expr,
                                                       ASTNodePtr* call_or_member) {
            if (!member_expr || member_expr->children.empty() || !member_expr->children[0] ||
                member_expr->children[0]->kind != NodeKind::Identifier) {
                return;
            }
            const std::string& enum_name = member_expr->children[0]->value;
            auto git = generics.find(enum_name);
            if (git == generics.end() || git->second.node->kind != NodeKind::EnumDecl) return;
            if (enclosing_return_type.empty()) return;
            // enclosing_return_type is already a mangled name like
            // "Option__User" once instantiated — recover the base
            // name to confirm it actually matches this construction's
            // enum before rewriting, so e.g. a Result<T,E>-returning
            // function accidentally containing a stray Option.Some
            // doesn't get silently mis-rewritten to Option's mangled
            // name just because SOME generic return type was in
            // scope.
            std::string prefix = enum_name + "__";
            if (enclosing_return_type.compare(0, prefix.size(), prefix) != 0) return;
            member_expr->children[0]->value = enclosing_return_type;
        };
        if (n->kind == NodeKind::MemberExpr) {
            try_rewrite_variant_construction(n, nullptr);
        } else if (n->kind == NodeKind::CallExpr && !n->children.empty() &&
                   n->children[0] && n->children[0]->kind == NodeKind::MemberExpr) {
            try_rewrite_variant_construction(n->children[0], &n);
        }

        // Generic function call: `identity(42)` where `identity` is
        // a known generic PulseDecl name, OR an explicit
        // `identity<int>(42)` form (not currently produced by the
        // parser — call-site explicit type arguments weren't added
        // to parse_call — so only implicit inference from arguments
        // is handled here; see the header's scope note).
        if (n->kind == NodeKind::CallExpr && !n->children.empty() &&
            n->children[0] && n->children[0]->kind == NodeKind::Identifier) {
            auto git = generics.find(n->children[0]->value);
            if (git != generics.end() &&
                (git->second.node->kind == NodeKind::PulseDecl ||
                 git->second.node->kind == NodeKind::AsyncPulseDecl)) {
                auto& gd = git->second;
                // Build the substitution map from each type-param
                // position's inferred concrete argument type.
                std::unordered_map<std::string, std::string> subst;
                std::vector<std::string> type_args;
                bool all_resolved = true;
                std::vector<const ASTNode*> gparams; // the generic decl's own FieldDecl params, in order
                for (auto& c : gd.node->children) {
                    if (c && c->kind == NodeKind::FieldDecl) gparams.push_back(c.get());
                }
                for (size_t i = 0; i < gparams.size() && i + 1 < n->children.size() + 1; i++) {
                    if (i + 1 >= n->children.size()) break;
                    const std::string& ptype = gparams[i]->extra;
                    bool is_param = false;
                    for (auto& pn : gd.param_names) if (pn == ptype) is_param = true;
                    if (!is_param) continue;
                    if (subst.count(ptype)) continue; // already resolved from an earlier position
                    std::string concrete = infer_arg_type(n->children[i + 1]);
                    if (concrete.empty()) { all_resolved = false; break; }
                    subst[ptype] = concrete;
                }
                if (all_resolved && subst.size() == gd.param_names.size()) {
                    for (auto& pn : gd.param_names) type_args.push_back(subst[pn]);
                    std::string mangled = mangle_name(gd.node->value, type_args);
                    if (!specializations.count(mangled)) {
                        auto spec = deep_clone(gd.node);
                        spec->value = mangled;
                        // Drop the TypeParamDecl children — the
                        // specialization is fully concrete now, and
                        // downstream (semantic re-registration isn't
                        // re-run, but IR lowering / codegen must not
                        // see a "generic-looking" declaration).
                        std::vector<ASTNodePtr> kept;
                        for (auto& c : spec->children) {
                            if (!c || c->kind != NodeKind::TypeParamDecl) kept.push_back(c);
                        }
                        spec->children = std::move(kept);
                        substitute_types_in_subtree(spec, subst);
                        specializations[mangled] = spec;
                    }
                    generic_names_used.insert(gd.node->value);
                    n->children[0]->value = mangled;
                }
                // else: couldn't resolve every type argument (e.g. an
                // argument whose type isn't statically obvious to
                // this pass's lightweight inference) — leave the call
                // untouched. It will fail at IR lowering/codegen with
                // an "unknown function" error rather than silently
                // producing wrong code; a real diagnostic pointing
                // back at the original call site is a reasonable
                // follow-up (see the header's MonomorphizeError type,
                // currently unused by this fallback path).
            }
        }

        // Generic type construction: `spawn Box<int> { value: 5 }` —
        // explicit type argument in the SpawnExpr's own value string
        // (parse_spawn_expr now captures "Box<int>" verbatim — see
        // its own comment on this fix). This is the only place an
        // *explicit* type argument syntax already exists anywhere in
        // the grammar for a construction expression (call sites only
        // ever get inference, per the note above), so it reuses the
        // same instantiate_generic_type helper the type-annotation
        // fields above use, rather than a separately maintained copy
        // of the same "is this a generic instantiation string, and if
        // so, has it been specialized yet" logic.
        if (n->kind == NodeKind::SpawnExpr && !n->value.empty()) {
            n->value = instantiate_generic_type(n->value);
        }
    };

    std::vector<ASTNodePtr> cloned;
    for (auto& n : prog) cloned.push_back(deep_clone(n));
    for (auto& n : cloned) rewrite(n, "");

    // Pass 3: assemble the final program — original non-generic
    // declarations, plus every generated specialization, minus every
    // original generic declaration (whether or not it ended up with
    // any real instantiation; an uninstantiated generic declaration
    // has nothing concrete to lower to IR/codegen, and leaving it in
    // would just reproduce the exact "T isn't a real type" failure
    // this whole pass exists to avoid).
    for (auto& n : cloned) {
        if (!n) continue;
        bool skip = false;
        auto git = generics.find(n->value);
        if (git != generics.end() &&
            (n->kind == NodeKind::PulseDecl || n->kind == NodeKind::AsyncPulseDecl ||
             n->kind == NodeKind::ForgeDecl || n->kind == NodeKind::EnumDecl)) {
            // Confirm this really is the generic declaration itself
            // (has TypeParamDecl children), not an unrelated
            // non-generic declaration that happens to share a name
            // with something in a different scope.
            bool has_type_params = false;
            for (auto& c : n->children) if (c && c->kind == NodeKind::TypeParamDecl) has_type_params = true;
            if (has_type_params) skip = true;
        }
        // A generic impl<T> block itself (as opposed to one of the
        // concrete impl specializations this pass generated for it,
        // which live in `specializations` and get appended
        // separately below) still references T, not a concrete type
        // — leaving it in the output would fail downstream the same
        // way an un-instantiated generic forge/enum/pulse would.
        if (n->kind == NodeKind::ImplDecl) {
            bool has_type_params = false;
            for (auto& c : n->children) if (c && c->kind == NodeKind::TypeParamDecl) has_type_params = true;
            if (has_type_params) skip = true;
        }
        if (!skip) result.program.push_back(n);
    }
    for (auto& [name, spec] : specializations) {
        result.program.push_back(spec);
    }

    return result;
}

} // namespace xphage::generics
