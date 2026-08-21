#pragma once
// ============================================================
// xphage_generics — Monomorphization
// Generic (pulse<T>/forge<T>/enum<T>) -> concrete specializations
// AeonCoreX Lab
// ============================================================
//
// Runs AFTER semantic analysis (not before it — see the design
// rationale below), producing a new Program in which every generic
// declaration has been replaced by zero or more concrete, fully
// type-substituted specializations, and every call/construction site
// that used a generic declaration has been rewritten to reference the
// specific specialization its inferred type arguments select.
//
// Design rationale (why this runs after semantic analysis, not as an
// AST-to-AST substitution pass before it): a generic declaration's
// own body needs to be checked once, generically, with its type
// parameter(s) treated as valid placeholder types — not re-checked
// from scratch for every instantiation, and not left completely
// unchecked until some concrete call site happens to exist. Monomorphizing
// first (before semantic analysis ever sees the generic form) would
// mean the analyzer only ever sees already-concrete, specialized
// copies, which breaks exactly this: a generic pulse with zero call
// sites in the program would have its body never checked at all, and
// a generic pulse called from many places would have its body
// re-checked once per call site instead of once. Monomorphizing after
// semantic analysis keeps "check the generic form once" and
// "instantiate for each concrete use" as two separate, correctly-
// ordered concerns.
//
// After this pass runs, the resulting Program contains NO generic
// declarations at all (every TypeParamDecl-bearing PulseDecl/
// ForgeDecl/EnumDecl has been removed, replaced by its concrete
// specializations) and no call/construction site references a
// generic name anymore — IR lowering and both C++ codegen backends
// consume the output exactly as if the program had been hand-written
// without generics in the first place. Neither backend needs any
// generics-specific logic as a result.

#include "xphage/ast.hpp"
#include <vector>
#include <string>

namespace xphage::generics {

// One error found during monomorphization (e.g. a generic call whose
// type arguments couldn't be determined at all — should be rare in
// practice, since check_call in semantic analysis already validates
// bounds and argument-count/consistency before this pass runs; this
// exists as a defensive backstop, not the primary place errors are
// expected to surface).
struct MonomorphizeError {
    std::string message;
    uint32_t    line = 0;
};

struct MonomorphizeResult {
    Program                          program;
    std::vector<MonomorphizeError>   errors;
    bool                              ok = true;
};

// Runs monomorphization over the whole program. `library_decls`, if
// non-empty, is a second set of declarations (e.g. resolved stdlib
// `~link` declarations) that generic call sites in `prog` may also
// reference — passed separately, matching how resolve_stdlib_links'
// output is threaded through the rest of the pipeline (see
// interface.cpp), rather than requiring the caller to merge the two
// programs first.
MonomorphizeResult monomorphize(const Program& prog,
                                  const Program& library_decls = {});

} // namespace xphage::generics
