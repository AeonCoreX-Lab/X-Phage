#pragma once
// ============================================================
// X-Phage Symbol Table v1.0.0
//
// Tracks every declared name (variable, function, type) across
// nested lexical scopes, for the semantic analyzer to resolve
// identifiers and function calls against. This is what makes
// "cannot find value `x` in this scope" and "did you mean `y`?"
// possible — without it, undefined-name errors only show up as
// whatever the downstream C++ compiler happens to say, pointing
// at the generated file.
// ============================================================
#include "xphage/ast.hpp"
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include <optional>

namespace xphage::sema {

enum class SymbolKind { Variable, Function, Type, Module, Enum };

// One <T> / <T: Bound> generic type parameter on a Function/Type/Enum
// symbol. `bound`, when non-empty, names a nexus (interface) the
// concrete type argument must implement — validated at each
// call/construction site (see SemanticAnalyzer::check_type_argument)
// rather than left as syntax-only, since accepting `T: Numeric` and
// then silently allowing `T = User` would make the bound annotation
// actively misleading rather than just incomplete.
struct TypeParamInfo {
    std::string name;   // e.g. "T"
    std::string bound;  // e.g. "Numeric" — empty if unbounded
};

struct FunctionSig {
    std::vector<std::string> param_types; // "int", "float", "str", "auto", ...
    std::string               return_type;
    bool                      is_async = false;
    // Non-empty exactly when this function is generic (`pulse
    // identity<T>(x: T) -> T`). A parameter/return type in
    // param_types/return_type that matches one of these names by
    // string (e.g. "T") is a type-parameter reference, not a real
    // concrete type — see is_type_param_reference.
    std::vector<TypeParamInfo> type_params;
};

struct Symbol {
    std::string  name;
    // Every construction site in semantic_analyzer.cpp currently sets
    // this explicitly, but SymbolKind has no inherently "safe" zero
    // value the way a default-constructed enum silently becomes 0 —
    // and 0 here is Variable, which is a plausible-looking value to
    // read even when uninitialized, making a missed assignment easy
    // to overlook in review. Defaulting to Variable removes the
    // undefined-behavior risk from any future call site that forgets
    // to set it (reading an uninitialized enum is undefined behavior
    // in C++, not just "some garbage int" — this is worth closing off
    // even though every current site happens to set it correctly).
    SymbolKind   kind = SymbolKind::Variable;
    std::string  type;        // declared type, e.g. "int", "Player", "auto"
    Span         declared_at;
    bool         is_mutable = true;   // atom (immutable-by-convention) vs shadow
    bool         is_used    = false;  // for unused-variable warnings
    std::optional<FunctionSig> fn_sig; // populated when kind == Function
    // Populated when kind == Type or kind == Enum and the forge/enum
    // is generic (`forge Box<T> { ... }`, `enum Option<T> { ... }`).
    // Mirrors FunctionSig::type_params — kept as a separate field
    // rather than folding Type/Enum into fn_sig, since a forge/enum
    // symbol was never a function and giving it a FunctionSig just to
    // reach one field would be a confusing/misleading combination.
    std::vector<TypeParamInfo> type_params;
};

// A single lexical scope — a flat name→Symbol map, plus a link to
// its enclosing scope for outward lookup. Function bodies, blocks
// ({ }), and the top-level program each get their own Scope.
class Scope {
public:
    explicit Scope(Scope* parent = nullptr) : parent_(parent) {}

    void declare(Symbol sym) { symbols_[sym.name] = std::move(sym); }

    // Looks up a name in this scope, then walks outward through
    // parent scopes until found or the chain is exhausted.
    Symbol* lookup(const std::string& name) {
        auto it = symbols_.find(name);
        if (it != symbols_.end()) return &it->second;
        return parent_ ? parent_->lookup(name) : nullptr;
    }

    // Looks up a name in THIS scope only — used to detect a
    // shadowing re-declaration within the same block, which is
    // worth a warning even though XPhage doesn't forbid it.
    Symbol* lookup_local(const std::string& name) {
        auto it = symbols_.find(name);
        return it != symbols_.end() ? &it->second : nullptr;
    }

    Scope* parent() const { return parent_; }
    const std::unordered_map<std::string, Symbol>& symbols() const { return symbols_; }

private:
    Scope* parent_;
    std::unordered_map<std::string, Symbol> symbols_;
};

// Owns the full chain of scopes for one compilation. push_scope/
// pop_scope follow the AST's block structure during a single
// recursive-descent walk (the semantic analyzer drives this, not
// the symbol table itself — this class is just storage + lookup).
class SymbolTable {
public:
    SymbolTable() { scopes_.push_back(std::make_unique<Scope>(nullptr)); }

    Scope* current() { return scopes_.back().get(); }

    void push_scope() {
        scopes_.push_back(std::make_unique<Scope>(scopes_.back().get()));
    }
    void pop_scope() {
        if (scopes_.size() > 1) scopes_.pop_back();
    }

    void declare(Symbol sym) { current()->declare(std::move(sym)); }
    Symbol* lookup(const std::string& name) { return current()->lookup(name); }
    Symbol* lookup_local(const std::string& name) { return current()->lookup_local(name); }

    // True when no scope has been pushed beyond the initial one —
    // i.e. we're looking at top-level/module scope, not inside any
    // function body or block. Used to distinguish a top-level atom/
    // shadow declaration (already registered by the semantic
    // analyzer's declaration-collection pass) from a function-body-
    // local one (which only the body-checking pass ever sees).
    bool is_at_root_scope() const { return scopes_.size() == 1; }

    // Computes the Levenshtein edit distance between two strings —
    // used to power "did you mean `x`?" suggestions: when a name
    // isn't found, the closest declared name (by edit distance,
    // within a small threshold) is offered as a guess.
    static size_t edit_distance(const std::string& a, const std::string& b);

    // Searches every symbol visible from the current scope (walking
    // outward through parents) for the closest name match to
    // `target`, restricted to symbols of `kind`. Returns nullopt if
    // nothing is within a reasonable edit-distance threshold (to
    // avoid offering wildly unrelated suggestions).
    std::optional<std::string> suggest_similar(const std::string& target, SymbolKind kind);

private:
    std::vector<std::unique_ptr<Scope>> scopes_;
};

} // namespace xphage::sema
