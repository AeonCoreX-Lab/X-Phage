#pragma once
// ============================================================
// X-Phage Diagnostic Engine v1.0.0
//
// Structured compiler diagnostics, modeled on rustc's diagnostic
// system: every error/warning carries a stable code, a primary
// message, a source span, and — where the compiler can work one
// out — a human-actionable suggestion ("did you mean X?",
// "try using ref instead of own", etc.).
//
// Error code ranges (per the language's own design spec):
//   XP1000-1999  Syntax (lexer/parser)
//   XP2000-2999  Semantic (name resolution, type checking)
//   XP3000-3999  Ownership / borrow checking
//   XP4000-4999  Fusion UI
//   XP5000-5999  XPM (package manager)
//   XP6000-6999  FFI / extern
//   XP7000-7999  Performance hints
//   XP8000-8999  AI / optimization recommendations
//   XP9000-9999  Internal compiler errors
// ============================================================
#include "xphage/ast.hpp"
#include <string>
#include <vector>
#include <optional>

namespace xphage::diag {

enum class Severity { Error, Warning, Note, Help };

inline std::string severity_str(Severity s) {
    switch (s) {
        case Severity::Error:   return "error";
        case Severity::Warning: return "warning";
        case Severity::Note:    return "note";
        case Severity::Help:    return "help";
        default:                return "error";
    }
}

// ANSI color per severity, for terminal output. Empty string when
// color is disabled (e.g. piping to a file, or --no-color).
inline std::string severity_color(Severity s) {
    switch (s) {
        case Severity::Error:   return "\033[1;31m"; // red
        case Severity::Warning: return "\033[1;33m"; // yellow
        case Severity::Note:    return "\033[1;36m"; // cyan
        case Severity::Help:    return "\033[1;32m"; // green
        default:                return "\033[1;31m";
    }
}

// A single structured diagnostic. `code` is a stable identifier
// like "XP2001" — stable across compiler versions so it can be
// looked up in documentation, grepped for in CI logs, or used by
// an IDE to offer a quick-fix keyed on the code rather than
// parsing the message text.
struct Diagnostic {
    std::string code;        // "XP2001"
    Severity    severity = Severity::Error;
    std::string message;     // "cannot find value `x` in this scope"
    Span        span;        // file/line/col of the primary point of error
    uint32_t    underline_len = 1; // width of the ^^^ underline; 1 = single
                                     // caret (point location), >1 = spans
                                     // the actual offending token/identifier

    // Optional secondary span + label, for "first declared here" /
    // "previous definition here" style two-location diagnostics.
    bool        has_secondary = false;
    Span        secondary_span;
    std::string secondary_label;

    // Optional actionable suggestion text, e.g. "did you mean
    // `beam`?" or "try using `ref` instead of `own`". Rendered as
    // a `help:` line beneath the main diagnostic.
    std::optional<std::string> help;

    // Optional auto-fix: if non-empty, this is literal replacement
    // text the compiler is confident would fix the issue, suitable
    // for an IDE or `xphage fix` to apply automatically without
    // asking the user to interpret a suggestion in prose.
    std::optional<std::string> suggested_replacement;
};

// Formats a single diagnostic as a human-readable, optionally
// colored block in the style of rustc/clang:
//
//   error[XP2001]: cannot find value `undefined_thing` in this scope
//     --> main.xp:2:14
//      |
//    2 |     atom x = undefined_thing
//      |              ^^^^^^^^^^^^^^^ not found in this scope
//      |
//      = help: did you mean `defined_thing`?
//
// `source_line` is the literal text of the line the span points
// at, if available (used to render the `2 | ...` line and the `^`
// underline at the right column) — the caller is responsible for
// reading it from the original source file, since the Diagnostic
// itself doesn't carry the whole file.
std::string format_diagnostic(const Diagnostic& d,
                               const std::string& source_line = "",
                               bool use_color = true);

} // namespace xphage::diag
