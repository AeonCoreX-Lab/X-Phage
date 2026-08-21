#include "../include/semantic_analyzer.hpp"
#include <sstream>
#include <unordered_set>

namespace xphage::sema {

using diag::Diagnostic;
using diag::Severity;

SemanticAnalyzer::SemanticAnalyzer(const std::string& source_text)
    : source_text_(source_text) {
    seed_builtins();
}

// ── Builtin seeding ──────────────────────────────────────────────
// Mirrors the function name lists the transpiler's runtime blocks
// provide (see transpiler.cpp's math_runtime_names() /
// string_runtime_names() / io_runtime_names() /
// collections_runtime_names()) plus core_ops.cpp's always-available
// helpers. Kept as a static literal list here rather than sharing a
// single source of truth across two compiler modules, since
// xphage_sema doesn't currently depend on xphage_codegen_transpiler
// (and shouldn't start to, to keep the pass ordering clean — sema
// runs before any backend is even chosen). If the transpiler's
// runtime function set changes, this list needs a matching update;
// it intentionally only includes the names, not their full
// signatures, since arity checking against a stdlib function uses
// the same general call-checking logic as a user function once the
// symbol is registered.
void SemanticAnalyzer::seed_builtins() {
    static const char* math_names[] = {
        "sqrt", "cbrt", "pow", "exp", "exp2", "log", "log2", "log10",
        "abs", "sign", "floor", "ceil", "round", "trunc", "fract",
        "sin", "cos", "tan", "asin", "acos", "atan", "atan2",
        "sinh", "cosh", "tanh", "to_rad", "to_deg",
        "min", "max", "clamp", "minf", "maxf", "clampf", "gcd", "lcm",
        "lerp", "lerp_clamp", "smoothstep", "smootherstep", "map_range",
        "bezier3", "bezier4",
        "rand", "rand_int", "rand_float", "rand_bool", "rand_gaussian", "rand_seed",
        "bit_count", "bit_leading_zeros", "bit_trailing_zeros",
        "is_power_of_two", "next_power_of_two",
        // not yet implemented but signature-declared by math.xh —
        // still registered so calling them is a link error, not an
        // unknown-identifier sema error (the distinction matters:
        // "this exists but isn't implemented yet" vs "this was
        // never a real function" are different problems)
        "vec2_add", "vec2_sub", "vec2_mul", "vec2_dot", "vec2_length",
        "vec3_add", "vec3_sub", "vec3_cross", "mean", "median", "stddev", "variance",
    };
    static const char* string_names[] = {
        "str_contains", "str_find", "str_find_last", "str_count",
        "str_starts_with", "str_ends_with", "str_is_empty", "str_is_blank",
        "str_upper", "str_lower", "str_title", "str_trim", "str_trim_left",
        "str_trim_right", "str_trim_chars", "str_reverse", "str_repeat",
        "str_slice", "str_left", "str_right", "str_char_at", "str_len",
        "str_split", "str_split_lines", "str_split_words", "str_join",
        "str_replace", "str_replace_first", "str_remove",
        "str_pad_left", "str_pad_right", "str_center",
        "str_to_int", "str_to_float", "str_to_bool", "int_to_str",
        "float_to_str", "bool_to_str", "str_to_hex", "hex_to_str",
        "str_matches", "str_regex_find", "str_regex_find_all",
        "str_regex_replace", "str_regex_split",
        "str_format_int", "str_format_float", "str_format_hex", "str_format_bin",
        "str_char_count", "str_is_ascii", "str_is_utf8",
    };
    static const char* io_names[] = {
        "io_read", "io_write", "io_append", "io_exists", "io_delete",
        "io_mkdir", "io_mkdir_all", "io_copy", "io_move", "io_size",
        "io_list_dir", "io_is_dir", "io_is_file",
        "print", "println", "eprint", "eprintln", "input",
        "path_join", "path_join3", "path_basename", "path_dirname",
        "path_extension", "path_stem", "path_absolute", "path_normalize",
        "env_get", "env_set", "env_has",
        "io_read_lines", "io_read_bytes", "io_write_bytes",
        "io_temp_file", "io_temp_dir",
    };
    static const char* collections_names[] = {
        "vec_new", "vec_push", "vec_get", "vec_set", "vec_size", "vec_len",
        "vec_is_empty", "vec_first", "vec_last", "vec_remove", "vec_remove_item",
        "vec_insert", "vec_contains", "vec_index_of", "vec_reverse", "vec_sort",
        "vec_unique", "vec_slice", "vec_concat", "vec_join", "vec_zip",
        "vec_enumerate", "vec_sum", "vec_min", "vec_max", "vec_avg", "vec_from_str",
        "map_new", "map_set", "map_get", "map_get_or", "map_has", "map_remove",
        "map_keys", "map_values", "map_size", "map_is_empty", "map_merge", "map_from_pairs",
        "set_new", "set_add", "set_remove", "set_contains", "set_size",
        "set_union", "set_intersect", "set_diff", "set_to_vec", "set_from_vec",
        "queue_new", "queue_push", "queue_pop", "queue_peek", "queue_size", "queue_is_empty",
        "stack_new", "stack_push", "stack_pop", "stack_peek", "stack_size", "stack_is_empty",
        "range", "range_step", "range_float",
        "zip", "enumerate", "take", "drop", "chunk",
        "vec_map", "vec_filter", "vec_reduce", "vec_each", "vec_sort_by",
        "option_map", "result_map", "flatten", "partition", "take_while", "drop_while",
    };
    static const char* core_names[] = {
        // Always available — core_ops.cpp / runtime.hpp, no ~link required
        "str_trim", "str_upper", "str_lower", "str_len", "str_replace",
        "str_contains", "str_starts_with", "str_ends_with",
        "str_to_int", "str_to_float", "int_to_str", "float_to_str",
    };

    auto seed_all = [&](const char* const* names, size_t count) {
        for (size_t i = 0; i < count; i++) {
            Symbol sym;
            sym.name = names[i];
            sym.kind = SymbolKind::Function;
            sym.type = "auto";
            sym.is_used = true; // builtins are never flagged unused
            symtab_.declare(sym);
        }
    };
    seed_all(math_names, sizeof(math_names)/sizeof(math_names[0]));
    seed_all(string_names, sizeof(string_names)/sizeof(string_names[0]));
    seed_all(io_names, sizeof(io_names)/sizeof(io_names[0]));
    seed_all(collections_names, sizeof(collections_names)/sizeof(collections_names[0]));
    seed_all(core_names, sizeof(core_names)/sizeof(core_names[0]));
}

// ── Diagnostic helpers ────────────────────────────────────────────
void SemanticAnalyzer::error(const std::string& code, const Span& span,
                              const std::string& msg, uint32_t underline_len,
                              std::optional<std::string> help) {
    Diagnostic d;
    d.code = code;
    d.severity = Severity::Error;
    d.message = msg;
    d.span = span;
    d.underline_len = underline_len;
    d.help = help;
    diags_.push_back(std::move(d));
}

void SemanticAnalyzer::warning(const std::string& code, const Span& span,
                                const std::string& msg, uint32_t underline_len,
                                std::optional<std::string> help) {
    Diagnostic d;
    d.code = code;
    d.severity = Severity::Warning;
    d.message = msg;
    d.span = span;
    d.underline_len = underline_len;
    d.help = help;
    diags_.push_back(std::move(d));
}

// ── Top-level entry point ─────────────────────────────────────────
AnalysisResult SemanticAnalyzer::analyze(const Program& prog) {
    diags_.clear();

    for (auto& n : prog) collect_impls(n);
    collect_declarations(prog);
    check_program(prog);

    AnalysisResult result;
    result.diagnostics = diags_;
    for (auto& d : diags_) {
        if (d.severity == Severity::Error) { result.ok = false; break; }
    }
    return result;
}

// Pre-pass (before declaration collection, so bound-checking has
// this data available from the very first call site it needs to
// validate): record every `impl Nexus for Type { ... }` into
// impl_registry_. Walks into realms too, since an impl can be
// declared inside one. Deliberately does NOT try to disambiguate
// `impl Type { ... }` (no nexus — an inherent impl, which the parser
// represents identically except with an empty `.extra`, see
// parse_impl_decl's own comment on this ambiguity) — an empty
// extra means nothing to register, not an error.
void SemanticAnalyzer::collect_impls(const ASTNodePtr& n) {
    if (!n) return;
    if (n->kind == NodeKind::ImplDecl && !n->value.empty() && !n->extra.empty()) {
        impl_registry_[n->value].insert(n->extra);
    }
    for (auto& c : n->children) collect_impls(c);
}

bool SemanticAnalyzer::satisfies_bound(const std::string& type_name,
                                        const std::string& bound_nexus) const {
    if (bound_nexus.empty()) return true;   // no bound to check
    if (type_name.empty() || type_name == "auto") return true; // couldn't infer — don't false-positive
    // XPhage's built-in scalar types are treated as satisfying any
    // bound named exactly "Numeric" — there's no `impl Numeric for
    // int` anywhere to find via impl_registry_ (int/float aren't
    // forge types), but a generic numeric function is the single
    // most common reason to write a bound at all, and a strict
    // registry-only check would make `pulse add<T: Numeric>(a: T, b:
    // T) -> T` unusable with XPhage's own primitive numeric types —
    // clearly not the intended meaning of the bound.
    if (bound_nexus == "Numeric" && (type_name == "int" || type_name == "float")) {
        return true;
    }
    auto it = impl_registry_.find(bound_nexus);
    if (it == impl_registry_.end()) return false; // bound names a nexus nothing implements
    return it->second.count(type_name) > 0;
}

// ── Pass 1: declaration collection ────────────────────────────────
void SemanticAnalyzer::collect_declarations(const Program& prog) {
    for (auto& n : prog) collect_one_declaration(n);
}

void SemanticAnalyzer::collect_one_declaration(const ASTNodePtr& n) {
    if (!n) return;

    switch (n->kind) {
        case NodeKind::PulseDecl:
        case NodeKind::AsyncPulseDecl: {
            if (n->value.empty()) break; // anonymous pulse — nothing to register
            if (auto* existing = symtab_.lookup_local(n->value)) {
                // A builtin placeholder (seeded by seed_builtins(),
                // identifiable by its line-0 declared_at — no real
                // source location ever has line 0) is not a genuine
                // prior declaration; a real `pulse sqrt(...)` in
                // math.xh providing the actual implementation should
                // silently take over the symbol table entry, not be
                // flagged as redefining something.
                bool existing_is_builtin = (existing->declared_at.line == 0 &&
                                             existing->declared_at.file.empty());
                if (existing->kind == SymbolKind::Function && !existing_is_builtin) {
                    Diagnostic d;
                    d.code = "XP2010";
                    d.severity = Severity::Error;
                    d.message = "the function `" + n->value + "` is defined multiple times";
                    d.span = n->span;
                    d.underline_len = (uint32_t)n->value.size();
                    d.has_secondary = true;
                    d.secondary_span = existing->declared_at;
                    d.secondary_label = "previous definition of `" + n->value + "` here";
                    diags_.push_back(std::move(d));
                    break;
                }
            }
            Symbol sym;
            sym.name = n->value;
            sym.kind = SymbolKind::Function;
            sym.type = n->extra2.empty() ? "void" : n->extra2;
            sym.declared_at = n->span;
            FunctionSig sig;
            sig.return_type = sym.type;
            sig.is_async = (n->kind == NodeKind::AsyncPulseDecl);
            for (auto& c : n->children) {
                if (c && c->kind == NodeKind::TypeParamDecl) {
                    sig.type_params.push_back({c->value, c->extra});
                } else if (c && c->kind == NodeKind::FieldDecl) {
                    sig.param_types.push_back(c->extra.empty() ? "auto" : c->extra);
                }
            }
            sym.fn_sig = std::move(sig);
            symtab_.declare(sym);
            break;
        }

        case NodeKind::ForgeDecl:
        case NodeKind::NexusDecl: {
            if (n->value.empty()) break;
            if (auto* existing = symtab_.lookup_local(n->value)) {
                Diagnostic d;
                d.code = "XP2011";
                d.severity = Severity::Error;
                d.message = "the type `" + n->value + "` is defined multiple times";
                d.span = n->span;
                d.underline_len = (uint32_t)n->value.size();
                d.has_secondary = true;
                d.secondary_span = existing->declared_at;
                d.secondary_label = "previous definition of `" + n->value + "` here";
                diags_.push_back(std::move(d));
                break;
            }
            Symbol sym;
            sym.name = n->value;
            sym.kind = SymbolKind::Type;
            sym.declared_at = n->span;
            for (auto& c : n->children) {
                if (c && c->kind == NodeKind::TypeParamDecl) {
                    sym.type_params.push_back({c->value, c->extra});
                }
            }
            symtab_.declare(sym);
            break;
        }

        case NodeKind::EnumDecl: {
            if (n->value.empty()) break;
            if (auto* existing = symtab_.lookup_local(n->value)) {
                Diagnostic d;
                d.code = "XP2011";
                d.severity = Severity::Error;
                d.message = "the type `" + n->value + "` is defined multiple times";
                d.span = n->span;
                d.underline_len = (uint32_t)n->value.size();
                d.has_secondary = true;
                d.secondary_span = existing->declared_at;
                d.secondary_label = "previous definition of `" + n->value + "` here";
                diags_.push_back(std::move(d));
                break;
            }
            Symbol sym;
            sym.name = n->value;
            sym.kind = SymbolKind::Enum;
            sym.declared_at = n->span;
            for (auto& c : n->children) {
                if (c && c->kind == NodeKind::TypeParamDecl) {
                    sym.type_params.push_back({c->value, c->extra});
                }
            }
            symtab_.declare(sym);
            // Each variant's arity/payload-type validation happens at
            // the actual construction site (check_call /
            // check_member, see below) rather than here — collection
            // only needs to make the enum's own name resolvable so
            // `Status.Ok` / `Status.Error(...)` don't fail with
            // "cannot find value `Status`" before ever reaching a
            // variant-specific check.
            break;
        }

        case NodeKind::GlobalDecl:
        case NodeKind::ConstDecl:
        case NodeKind::FluxDecl: {
            if (n->value.empty()) break;
            Symbol sym;
            sym.name = n->value;
            sym.kind = SymbolKind::Variable;
            sym.type = n->extra.empty() ? "auto" : n->extra;
            sym.declared_at = n->span;
            sym.is_mutable = (n->kind != NodeKind::ConstDecl); // flux is reactive
                                                                  // but still assignable,
                                                                  // same as a global
            symtab_.declare(sym);
            break;
        }

        case NodeKind::AtomDecl:
        case NodeKind::ShadowDecl: {
            // Top-level atom/shadow (e.g. the stdlib's `atom PI: float
            // = ...`, or a top-level program's `shadow t1 = ...`).
            // Function-body-local atom/shadow are handled later, in
            // check_block, with proper scoping — this collection pass
            // only walks top-level nodes.
            if (n->value.empty()) break;
            Symbol sym;
            sym.name = n->value;
            sym.kind = SymbolKind::Variable;
            // An explicit annotation (`atom u: User = ...`) always
            // wins; absent one, fall back to inferring from the
            // initializer expression itself (`atom u = spawn User
            // {...}` — SpawnExpr's own value already carries the
            // concrete type name) rather than defaulting straight to
            // "auto". Without this, every un-annotated spawn'd atom
            // read back as type "auto" everywhere downstream that
            // needs a real type — including generic bound-checking at
            // a call site, which silently skips "auto" arguments
            // rather than false-positive on something it has no type
            // information for.
            if (!n->extra.empty()) {
                sym.type = n->extra;
            } else if (!n->children.empty() && n->children[0] &&
                       n->children[0]->kind == NodeKind::SpawnExpr) {
                sym.type = n->children[0]->value;
            } else {
                sym.type = "auto";
            }
            sym.declared_at = n->span;
            sym.is_mutable = (n->kind == NodeKind::ShadowDecl);
            symtab_.declare(sym);
            break;
        }

        case NodeKind::TupleDestructure: {
            // Top-level `atom (lo, hi) = min_max(numbers)` — same
            // registration as check_stmt's TupleDestructure case
            // (see that comment), just at Pass 1 / top-level scope
            // rather than inside a function body. The initializer
            // expression itself isn't checked here (Pass 1 only
            // registers symbols; check_program's statement-level pass
            // checks expressions), only the bound names are declared.
            if (n->children.empty()) break;
            size_t init_idx = n->children.size() - 1;
            for (size_t i = 0; i < init_idx; i++) {
                if (!n->children[i] || n->children[i]->value.empty()) continue;
                Symbol sym;
                sym.name = n->children[i]->value;
                sym.kind = SymbolKind::Variable;
                sym.type = "auto";
                sym.declared_at = n->span;
                sym.is_mutable = (n->value == "shadow");
                symtab_.declare(sym);
            }
            break;
        }

        case NodeKind::ExternDecl: {
            if (n->value.empty()) break;
            Symbol sym;
            sym.name = n->value;
            sym.kind = SymbolKind::Function;
            sym.type = n->extra2.empty() ? "void" : n->extra2;
            sym.declared_at = n->span;
            sym.is_used = true; // FFI declarations are rarely "unused" in
                                 // a meaningful sense — they exist to be
                                 // available, not necessarily called from
                                 // every translation unit
            FunctionSig sig;
            sig.return_type = sym.type;
            for (auto& c : n->children) {
                if (c && c->kind == NodeKind::FieldDecl) {
                    sig.param_types.push_back(c->extra.empty() ? "auto" : c->extra);
                }
            }
            sym.fn_sig = std::move(sig);
            symtab_.declare(sym);
            break;
        }

        case NodeKind::Block: {
            // extern "C" { pulse a... pulse b... } block form —
            // unwrap and register each ExternDecl child the same way.
            for (auto& c : n->children) {
                if (c && c->kind == NodeKind::ExternDecl) collect_one_declaration(c);
            }
            break;
        }

        case NodeKind::RealmDecl: {
            // Symbols declared inside a realm are scoped under that
            // realm's namespace at the C++ level (Realm::member), but
            // for this pass's purposes — catching undefined names at
            // the call site — registering them as flat top-level
            // names is a reasonable approximation: XPhage's `use`
            // declarations bring a realm member into unqualified
            // scope, and fully qualified name resolution (Geometry::
            // length_sq) is left to the C++ compiler's own namespace
            // lookup as a deliberate scope limitation of this pass.
            for (auto& c : n->children) collect_one_declaration(c);
            break;
        }

        default:
            break;
    }
}

// ── Pass 2: body checking ─────────────────────────────────────────
void SemanticAnalyzer::check_program(const Program& prog) {
    for (auto& n : prog) {
        if (!n) continue;
        if (n->kind == NodeKind::PulseDecl || n->kind == NodeKind::AsyncPulseDecl) {
            check_pulse_body(n);
        } else if (n->kind == NodeKind::RealmDecl) {
            check_program(n->children);
        } else if (n->kind == NodeKind::ImplDecl) {
            // Previously fell to the final `else` branch below and
            // was passed to check_stmt as if it were a bare top-level
            // statement — meaningless for a declaration node, and
            // meant every impl method body was completely unchecked
            // (no parameter registration, self included — see
            // check_pulse_body's handling below for what a proper
            // method body check now does — no identifier/call
            // validation, nothing). Each method is checked the same
            // way a standalone pulse's body is, since methods and
            // pulses share the same body-checking needs (parameter
            // scoping, self included since parse_field_decl now
            // parses it as a real FieldDecl parameter).
            for (auto& method : n->children) {
                if (method) check_pulse_body(method);
            }
        } else if (n->kind == NodeKind::EnumDecl) {
            // Nothing to check inside an EnumDecl's own children
            // (EnumVariant/TypeParamDecl aren't executable) — but
            // this must still be excluded from the bare-statement
            // fallback below, the same reasoning as ImplDecl above.
            continue;
        } else if (n->kind == NodeKind::Block) {
            // extern block — no body to check (declarations only)
            continue;
        } else if (n->kind == NodeKind::ForgeDecl || n->kind == NodeKind::NexusDecl ||
                   n->kind == NodeKind::GlobalDecl || n->kind == NodeKind::ConstDecl ||
                   n->kind == NodeKind::FluxDecl ||
                   n->kind == NodeKind::ExternDecl || n->kind == NodeKind::UseDecl ||
                   n->kind == NodeKind::LinkStmt) {
            continue; // declarations already handled in pass 1; nothing
                      // further to check at top level for these kinds
        } else {
            // A bare top-level statement (atom/shadow with an
            // initializer expression, beam, an assignment, etc.) —
            // these run inside the implicit main(), so check them in
            // the top-level scope directly, same as a statement
            // inside any function body.
            check_stmt(n);
        }
    }
}

void SemanticAnalyzer::check_pulse_body(const ASTNodePtr& pulse_decl) {
    symtab_.push_scope();

    for (auto& c : pulse_decl->children) {
        if (c && c->kind == NodeKind::FieldDecl && !c->value.empty()) {
            Symbol sym;
            sym.name = c->value;
            sym.kind = SymbolKind::Variable;
            sym.type = c->extra.empty() ? "auto" : c->extra;
            sym.declared_at = c->span;
            sym.is_used = true; // parameters aren't flagged unused — a
                                 // function signature often needs a
                                 // parameter present even if a
                                 // particular implementation doesn't
                                 // use it (interface consistency)
            symtab_.declare(sym);
        }
    }

    for (auto& c : pulse_decl->children) {
        if (c && c->kind == NodeKind::Block) check_block(c);
    }

    symtab_.pop_scope();
}

void SemanticAnalyzer::check_block(const ASTNodePtr& block) {
    symtab_.push_scope();
    for (auto& stmt : block->children) check_stmt(stmt);
    symtab_.pop_scope();
}

void SemanticAnalyzer::check_stmt(const ASTNodePtr& stmt) {
    if (!stmt) return;

    switch (stmt->kind) {
        case NodeKind::AtomDecl:
        case NodeKind::ShadowDecl: {
            if (!stmt->children.empty() && stmt->children[0]) {
                check_expr(stmt->children[0]);
            }
            if (!stmt->value.empty()) {
                if (symtab_.is_at_root_scope()) {
                    // Top-level atom/shadow — Pass 1 (collect_declar-
                    // ations) already registered this exact symbol
                    // before body-checking ever started. Re-declaring
                    // it here would find that Pass-1 registration via
                    // lookup_local and report the statement as
                    // shadowing itself. There is genuinely nothing
                    // left to do here for a top-level declaration
                    // beyond the initializer check already done above
                    // — mark it used-if-referenced is handled the
                    // normal way when something later reads it.
                } else {
                    if (auto* existing = symtab_.lookup_local(stmt->value)) {
                        warning("XP2020", stmt->span,
                            "variable `" + stmt->value + "` shadows a previous declaration in the same scope",
                            (uint32_t)stmt->value.size());
                    }
                    Symbol sym;
                    sym.name = stmt->value;
                    sym.kind = SymbolKind::Variable;
                    // Same inference-from-initializer fallback as the
                    // top-level AtomDecl/ShadowDecl case above — see
                    // its comment for why.
                    if (!stmt->extra.empty()) {
                        sym.type = stmt->extra;
                    } else if (!stmt->children.empty() && stmt->children[0] &&
                               stmt->children[0]->kind == NodeKind::SpawnExpr) {
                        sym.type = stmt->children[0]->value;
                    } else {
                        sym.type = "auto";
                    }
                    sym.declared_at = stmt->span;
                    sym.is_mutable = (stmt->kind == NodeKind::ShadowDecl);
                    symtab_.declare(sym);
                }
            }
            break;
        }

        case NodeKind::TupleDestructure: {
            // atom (lo, hi) = min_max(numbers) — children[0..n-2] are
            // the bound Identifier names, children[n-1] is the
            // initializer. Registers each bound name as a real
            // symbol (type "auto" — this pass doesn't attempt to
            // infer each tuple element's individual concrete type,
            // matching this pass's general "auto when not confidently
            // knowable" convention elsewhere) so a later reference to
            // e.g. `lo` resolves instead of failing with "cannot find
            // value `lo`" the way it would with no declaration at
            // all — TupleDestructure isn't handled by the generic
            // default: case (which only recurses into children, never
            // declares anything), so without this case here, every
            // name a tuple destructure binds would be entirely
            // unresolvable afterward.
            if (stmt->children.empty()) break;
            size_t init_idx = stmt->children.size() - 1;
            check_expr(stmt->children[init_idx]);
            for (size_t i = 0; i < init_idx; i++) {
                if (!stmt->children[i] || stmt->children[i]->value.empty()) continue;
                Symbol sym;
                sym.name = stmt->children[i]->value;
                sym.kind = SymbolKind::Variable;
                sym.type = "auto";
                sym.declared_at = stmt->span;
                sym.is_mutable = (stmt->value == "shadow");
                symtab_.declare(sym);
            }
            break;
        }

        case NodeKind::Block:
            check_block(stmt);
            break;

        case NodeKind::IfStmt:
        case NodeKind::ElifStmt: {
            if (!stmt->children.empty()) check_expr(stmt->children[0]);
            for (size_t i = 1; i < stmt->children.size(); i++) {
                if (stmt->children[i] && stmt->children[i]->kind == NodeKind::Block)
                    check_block(stmt->children[i]);
                else
                    check_stmt(stmt->children[i]);
            }
            break;
        }

        case NodeKind::WhileStmt: {
            if (!stmt->children.empty()) check_expr(stmt->children[0]);
            for (size_t i = 1; i < stmt->children.size(); i++) check_stmt(stmt->children[i]);
            break;
        }

        case NodeKind::ForStmt: {
            // for i in range { ... } — the loop variable is scoped to
            // the loop body only.
            symtab_.push_scope();
            if (!stmt->value.empty()) {
                Symbol sym;
                sym.name = stmt->value;
                sym.kind = SymbolKind::Variable;
                sym.type = "auto";
                sym.declared_at = stmt->span;
                sym.is_used = true;
                symtab_.declare(sym);
            }
            for (auto& c : stmt->children) {
                if (c && c->kind == NodeKind::Block) check_block(c);
                else if (c) check_expr(c);
            }
            symtab_.pop_scope();
            break;
        }

        case NodeKind::ReturnStmt:
        case NodeKind::YieldStmt:
        case NodeKind::BeamStmt:
        case NodeKind::BypassStmt: {
            for (auto& c : stmt->children) check_expr(c);
            break;
        }

        case NodeKind::AbsorbStmt:
        case NodeKind::EmitStmt: {
            // absorb "event" { ... } / emit "event" { ... } — the
            // payload is a Block (a statement list), which needs
            // check_block (its own scope, statement-by-statement
            // checking), not check_expr (which has no notion of a
            // Block as a statement container and would just recurse
            // through its children as if they were subexpressions).
            for (auto& c : stmt->children) {
                if (c && c->kind == NodeKind::Block) check_block(c);
                else if (c) check_expr(c);
            }
            break;
        }

        case NodeKind::ExprStmt: {
            for (auto& c : stmt->children) check_expr(c);
            // A bare ExprStmt's "value" itself might directly be the
            // expression (depending on how the parser shaped it) —
            // check defensively.
            break;
        }

        case NodeKind::UnsafeBlock: {
            if (!stmt->children.empty() && stmt->children[0]) check_block(stmt->children[0]);
            break;
        }

        case NodeKind::PulseDecl:
        case NodeKind::AsyncPulseDecl: {
            // A nested/local function definition — register it in
            // the current scope (so sibling statements can call it)
            // then check its body in its own nested scope.
            collect_one_declaration(stmt);
            check_pulse_body(stmt);
            break;
        }

        default: {
            // Statement kinds not specifically handled above (probe/
            // diverge, quantum, chronos, ether, vortex, matrix,
            // synapse, absorb, link, use, realm-nested decls, etc.)
            // still have their expression-typed children walked, so
            // a call/identifier deep inside one of these is still
            // checked — just without a kind-specific scoping rule.
            // This is a deliberate "check what we can identify,
            // don't block on what we don't have a specific rule for
            // yet" trade-off, consistent with the rest of the
            // compiler's incremental approach to language coverage.
            for (auto& c : stmt->children) {
                if (c) check_expr(c);
            }
            break;
        }
    }
}

std::string SemanticAnalyzer::check_expr(const ASTNodePtr& expr) {
    if (!expr) return "auto";

    switch (expr->kind) {
        case NodeKind::Identifier:
            check_identifier(expr);
            if (auto* sym = symtab_.lookup(expr->value)) return sym->type;
            return "auto";

        case NodeKind::CallExpr:
            check_call(expr);
            return "auto"; // return-type inference for the call result
                            // is intentionally out of scope for this
                            // pass — see check_call for what IS checked

        case NodeKind::IntLit:    return "int";
        case NodeKind::FloatLit:  return "float";
        case NodeKind::BoolLit:   return "bool";
        case NodeKind::StringLit:
        case NodeKind::FStringLit: return "str";
        case NodeKind::NullLit:   return "auto";

        case NodeKind::BinaryOp:
            check_binary_op(expr);
            return "auto";

        case NodeKind::AssignExpr:
            check_assign(expr);
            return "auto";

        case NodeKind::SpawnExpr: {
            // SpawnExpr.value is the concrete constructed type's name
            // (e.g. "User", or "Geometry::Vector2" for a
            // realm-qualified spawn — see parse_spawn_expr) — a real,
            // useful type for anything downstream that wants it
            // (generic bound-checking at a call site is the concrete
            // motivating case; previously this fell into the generic
            // "auto" fallback group below, which is fine for most
            // purposes but meant a spawn used directly as a call
            // argument, e.g. `add(spawn User{...}, ...)`, was
            // silently exempted from bound-checking the same way an
            // un-annotated `atom` used to be — see AtomDecl/
            // ShadowDecl's own SpawnExpr-inference fix for the
            // matching case this addresses for the indirect-via-atom
            // form).
            for (auto& c : expr->children) check_expr(c);
            return expr->value.empty() ? "auto" : expr->value;
        }

        case NodeKind::UnaryOp:
        case NodeKind::IndexExpr:
        case NodeKind::MemberExpr:
        case NodeKind::CastExpr:
        case NodeKind::RangeExpr:
        case NodeKind::PipelineExpr:
        case NodeKind::AwaitExpr:
        case NodeKind::PropagateExpr: {
            for (auto& c : expr->children) check_expr(c);
            return "auto";
        }

        default: {
            for (auto& c : expr->children) check_expr(c);
            return "auto";
        }
    }
}

void SemanticAnalyzer::check_identifier(const ASTNodePtr& n) {
    auto* sym = symtab_.lookup(n->value);
    if (sym) {
        sym->is_used = true;
        return;
    }

    auto suggestion = symtab_.suggest_similar(n->value, SymbolKind::Variable);
    if (!suggestion.has_value()) {
        suggestion = symtab_.suggest_similar(n->value, SymbolKind::Function);
    }

    std::optional<std::string> help;
    if (suggestion.has_value()) {
        help = "did you mean `" + *suggestion + "`?";
    }

    error("XP2001", n->span,
          "cannot find value `" + n->value + "` in this scope",
          (uint32_t)n->value.size(), help);
}

void SemanticAnalyzer::check_call(const ASTNodePtr& n) {
    if (n->children.empty()) return;
    auto& callee = n->children[0];

    // Only a plain identifier callee gets full resolution + arity
    // checking here. A computed callee (e.g. calling through a
    // member access or an array index) is walked for its
    // subexpressions but not arity-checked, since the symbol table
    // doesn't yet track member-function signatures separately.
    if (!callee || callee->kind != NodeKind::Identifier) {
        for (auto& c : n->children) check_expr(c);
        return;
    }

    auto* sym = symtab_.lookup(callee->value);
    if (!sym) {
        auto suggestion = symtab_.suggest_similar(callee->value, SymbolKind::Function);
        std::optional<std::string> help;
        if (suggestion.has_value()) {
            help = "did you mean `" + *suggestion + "`?";
        } else if (callee->value.size() > 2) {
            // No close function-name match — for a call specifically
            // (as opposed to a bare identifier reference), a missing
            // ~link is a very common real cause, so it's worth a
            // generic nudge even without a specific module to name.
            help = "if this is a standard library function, check that the "
                   "module providing it is linked with ~link \"<module>\"";
        }
        error("XP2002", callee->span,
              "cannot find function `" + callee->value + "` in this scope",
              (uint32_t)callee->value.size(), help);
        // Still check the arguments themselves for their own errors,
        // even though the callee itself didn't resolve.
        for (size_t i = 1; i < n->children.size(); i++) check_expr(n->children[i]);
        return;
    }

    sym->is_used = true;

    if (sym->kind != SymbolKind::Function) {
        error("XP2003", callee->span,
              "`" + callee->value + "` is not a function and cannot be called",
              (uint32_t)callee->value.size(),
              "`" + callee->value + "` is declared as a variable here");
        for (size_t i = 1; i < n->children.size(); i++) check_expr(n->children[i]);
        return;
    }

    // Arity check — only when the signature doesn't contain any
    // "auto" parameters (a function genuinely generic over argument
    // count/shape, like the collections module's higher-order
    // functions, intentionally isn't arity-checked here).
    if (sym->fn_sig.has_value()) {
        size_t expected = sym->fn_sig->param_types.size();
        size_t actual = n->children.size() - 1; // children[0] is the callee
        bool has_auto_param = false;
        for (auto& t : sym->fn_sig->param_types) if (t == "auto") has_auto_param = true;

        if (!has_auto_param && expected != actual) {
            std::ostringstream msg;
            msg << "this function takes " << expected
                << (expected == 1 ? " argument" : " arguments")
                << " but " << actual << (actual == 1 ? " was" : " were") << " supplied";
            error("XP2004", n->span, msg.str(), (uint32_t)callee->value.size());
        }

        // Generic call: infer each type parameter's concrete type
        // argument from the actual argument expressions passed at
        // the positions that reference it, then (a) check every
        // position referencing the same type parameter agreed on the
        // same concrete type (e.g. `add<T>(a: T, b: T)` called as
        // `add(1, "x")` — both positions reference T, but infer
        // different concrete types) and (b) validate the inferred
        // type against the parameter's bound, if it has one.
        if (!sym->fn_sig->type_params.empty() && expected == actual) {
            std::unordered_map<std::string, std::string> inferred; // type param name -> concrete type
            std::unordered_map<std::string, Span> first_use_span;  // for a clear conflict message
            for (size_t i = 0; i < expected; i++) {
                const std::string& ptype = sym->fn_sig->param_types[i];
                bool is_type_param = false;
                std::string bound;
                for (auto& tp : sym->fn_sig->type_params) {
                    if (tp.name == ptype) { is_type_param = true; bound = tp.bound; break; }
                }
                if (!is_type_param) continue;
                std::string arg_type = check_expr(n->children[i + 1]);
                if (arg_type.empty() || arg_type == "auto") continue; // nothing concrete to check
                auto it = inferred.find(ptype);
                if (it == inferred.end()) {
                    inferred[ptype] = arg_type;
                    first_use_span[ptype] = n->children[i + 1]->span;
                    if (!bound.empty() && !satisfies_bound(arg_type, bound)) {
                        error("XP2012", n->children[i + 1]->span,
                              "type `" + arg_type + "` does not satisfy bound `" + bound +
                              "` required by type parameter `" + ptype + "`",
                              1,
                              "`" + bound + "` requires an `impl " + bound + " for " +
                              arg_type + "` (or, for Numeric, int/float)");
                    }
                } else if (it->second != arg_type) {
                    error("XP2013", n->children[i + 1]->span,
                          "type parameter `" + ptype + "` was inferred as `" + it->second +
                          "` here but as `" + arg_type + "` at a previous argument",
                          1,
                          "every argument using the same type parameter must agree on its concrete type");
                }
            }
            // Arguments not referencing a type parameter still need
            // their own subexpressions checked (identifiers marked
            // used, nested calls validated, etc.) — the loop above
            // only called check_expr for type-parameter positions.
            for (size_t i = 0; i < expected; i++) {
                const std::string& ptype = sym->fn_sig->param_types[i];
                bool is_type_param = false;
                for (auto& tp : sym->fn_sig->type_params) if (tp.name == ptype) { is_type_param = true; break; }
                if (!is_type_param) check_expr(n->children[i + 1]);
            }
            return;
        }
    }

    for (size_t i = 1; i < n->children.size(); i++) check_expr(n->children[i]);
}

void SemanticAnalyzer::check_assign(const ASTNodePtr& n) {
    // AssignExpr shape: children[0] = target, children[1] = value
    if (n->children.size() < 2) {
        for (auto& c : n->children) check_expr(c);
        return;
    }
    auto& target = n->children[0];
    check_expr(n->children[1]); // check RHS first — its own identifiers
                                 // must resolve regardless of LHS shape

    if (target && target->kind == NodeKind::Identifier) {
        auto* sym = symtab_.lookup(target->value);
        if (!sym) {
            // Reuse the same undefined-identifier diagnostic as a
            // plain read — assigning to an undeclared name is the
            // same underlying problem.
            check_identifier(target);
            return;
        }
        sym->is_used = true;
        if (!sym->is_mutable) {
            error("XP2030", target->span,
                  "cannot assign to `" + target->value + "`, as it is declared with `atom`",
                  (uint32_t)target->value.size(),
                  "consider using `shadow` instead of `atom` if this value needs to change");
        }
    } else if (target) {
        check_expr(target); // member/index assignment target — walk
                             // its subexpressions for their own checks
    }
}

void SemanticAnalyzer::check_binary_op(const ASTNodePtr& n) {
    for (auto& c : n->children) check_expr(c);
}

} // namespace xphage::sema
