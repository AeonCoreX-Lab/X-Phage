#include "../include/diagnostic.hpp"
#include <sstream>

namespace xphage::diag {

namespace {

// Builds the `^^^^` underline beneath a source line, positioned at
// the diagnostic's column and sized to the printed line-number
// gutter width. Must match the "N | " prefix exactly: gutter_width
// spaces, then " | " (note: matching "N | " means gutter_width
// spaces + " | ", i.e. one space, pipe, one space — three chars,
// not two), then `col` more spaces, then the caret(s).
std::string make_underline(uint32_t col, size_t underline_len, size_t gutter_width) {
    std::ostringstream out;
    out << std::string(gutter_width, ' ') << " | "
        << std::string(col, ' ');
    for (size_t i = 0; i < underline_len; i++) out << '^';
    return out.str();
}

} // namespace

std::string format_diagnostic(const Diagnostic& d,
                               const std::string& source_line,
                               bool use_color) {
    std::ostringstream out;
    const std::string reset = use_color ? "\033[0m" : "";
    const std::string color = use_color ? severity_color(d.severity) : "";
    const std::string bold  = use_color ? "\033[1m" : "";

    // Header: "error[XP2001]: message"
    out << color << severity_str(d.severity);
    if (!d.code.empty()) out << "[" << d.code << "]";
    out << reset << bold << ": " << d.message << reset << "\n";

    // Location: "  --> main.xp:2:14"
    std::string file = d.span.file.empty() ? "<unknown>" : d.span.file;
    out << "  --> " << file << ":" << d.span.line << ":" << d.span.col << "\n";

    // Source snippet, if the caller supplied the actual line text.
    if (!source_line.empty()) {
        std::string line_no = std::to_string(d.span.line);
        size_t gutter_width = line_no.size();
        out << std::string(gutter_width, ' ') << " |\n";
        out << line_no << " | " << source_line << "\n";
        // Column is 1-based in Span; the underline needs the
        // 0-based offset into source_line.
        uint32_t col0 = d.span.col > 0 ? d.span.col - 1 : 0;
        out << color << make_underline(col0, d.underline_len, gutter_width) << reset << "\n";
        out << std::string(gutter_width, ' ') << " |\n";
    }

    // Secondary location, e.g. "previous definition here"
    if (d.has_secondary) {
        std::string sfile = d.secondary_span.file.empty() ? "<unknown>" : d.secondary_span.file;
        out << "  --> " << sfile << ":" << d.secondary_span.line
            << ":" << d.secondary_span.col;
        if (!d.secondary_label.empty()) out << " (" << d.secondary_label << ")";
        out << "\n";
    }

    // Help / suggestion line
    if (d.help.has_value()) {
        out << "  = " << (use_color ? severity_color(Severity::Help) : "")
            << "help" << reset << ": " << *d.help << "\n";
    }

    return out.str();
}

} // namespace xphage::diag
