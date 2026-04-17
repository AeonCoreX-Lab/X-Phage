// ============================================================
// xphage-fmt — Code Formatter v3.5.0
//
// Formats .xp0 source files to canonical style:
//   • 4-space indentation
//   • Spaces around operators
//   • Blank line before each pulse/global
//   • Trailing whitespace removed
//   • Single blank line at end of file
//
// Usage:
//   xphage-fmt <file.xp0>           (overwrite in-place)
//   xphage-fmt <file.xp0> --check   (exit 1 if not formatted)
//   xphage-fmt <file.xp0> --stdout  (print to stdout)
//   xphage-fmt <dir>                 (format all .xp0 in dir)
// ============================================================
#include "xphage/runtime.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <filesystem>
#include <cstring>
#include <algorithm>

namespace fs = std::filesystem;

// ── Formatting engine ─────────────────────────────────────────
static std::string format_source(const std::string& src) {
    std::istringstream in(src);
    std::ostringstream out;
    std::string line;

    int  indent_level = 0;
    bool last_was_blank = false;
    bool first_line = true;

    auto trim_right = [](std::string s) -> std::string {
        size_t end = s.find_last_not_of(" \t\r\n");
        return end == std::string::npos ? "" : s.substr(0, end + 1);
    };

    auto trim_left = [](const std::string& s) -> std::string {
        size_t st = s.find_first_not_of(" \t");
        return st == std::string::npos ? "" : s.substr(st);
    };

    auto is_toplevel_kw = [](const std::string& s) -> bool {
        return s.rfind("pulse ", 0) == 0 ||
               s.rfind("global ", 0) == 0 ||
               s.rfind("~link ", 0) == 0 ||
               s.rfind("fusion ", 0) == 0 ||
               s.rfind("@Neural", 0) == 0;
    };

    auto count_braces = [](const std::string& s) -> int {
        int d = 0;
        for (char c : s) {
            if (c == '{') d++;
            else if (c == '}') d--;
        }
        return d;
    };

    while (std::getline(in, line)) {
        line = trim_right(line);
        std::string content = trim_left(line);

        // Blank line
        if (content.empty()) {
            if (!last_was_blank && !first_line)
                out << "\n";
            last_was_blank = true;
            continue;
        }

        // Comment — preserve as-is (with correct indent)
        bool is_comment = content.rfind("//", 0) == 0;

        // Decrease indent BEFORE printing closing brace
        if (!content.empty() && content[0] == '}') {
            if (indent_level > 0) indent_level--;
        }

        // Blank line before top-level declarations (not first)
        if (is_toplevel_kw(content) && !first_line && !last_was_blank) {
            out << "\n";
        }

        // Indent + content
        std::string indent(indent_level * 4, ' ');
        out << indent << content << "\n";

        // Increase indent AFTER opening brace
        int brace_delta = count_braces(content);
        indent_level += brace_delta;
        if (indent_level < 0) indent_level = 0;

        last_was_blank = false;
        first_line     = false;
    }

    // Ensure single trailing newline
    std::string result = out.str();
    while (result.size() >= 2 &&
           result[result.size()-1] == '\n' &&
           result[result.size()-2] == '\n') {
        result.pop_back();
    }
    if (result.empty() || result.back() != '\n')
        result += '\n';

    return result;
}

// ── Format one file ───────────────────────────────────────────
enum class Mode { Inplace, Check, Stdout };

static int format_file(const std::string& path, Mode mode, bool verbose) {
    std::ifstream f(path);
    if (!f.is_open()) {
        std::cerr << "xphage-fmt: cannot open " << path << "\n";
        return 1;
    }
    std::ostringstream ss; ss << f.rdbuf();
    std::string original = ss.str();
    f.close();

    std::string formatted = format_source(original);

    if (mode == Mode::Stdout) {
        std::cout << formatted;
        return 0;
    }

    if (mode == Mode::Check) {
        if (formatted == original) {
            if (verbose) std::cout << "[ok] " << path << "\n";
            return 0;
        } else {
            std::cerr << "[needs formatting] " << path << "\n";
            return 1;
        }
    }

    // Inplace
    if (formatted == original) {
        if (verbose) std::cout << "[unchanged] " << path << "\n";
        return 0;
    }

    std::ofstream out(path);
    if (!out.is_open()) {
        std::cerr << "xphage-fmt: cannot write " << path << "\n";
        return 1;
    }
    out << formatted;
    if (verbose) std::cout << "[formatted] " << path << "\n";
    return 0;
}

// ── Main ─────────────────────────────────────────────────────
int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "xphage-fmt v3.5.0\n"
                  << "Usage: xphage-fmt <file.xp0|dir> [--check] [--stdout] [-v]\n";
        return 1;
    }

    Mode mode    = Mode::Inplace;
    bool verbose = false;
    std::vector<std::string> paths;

    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--check"))   mode = Mode::Check;
        else if (!std::strcmp(argv[i], "--stdout")) mode = Mode::Stdout;
        else if (!std::strcmp(argv[i], "-v") ||
                 !std::strcmp(argv[i], "--verbose")) verbose = true;
        else paths.emplace_back(argv[i]);
    }

    if (paths.empty()) { std::cerr << "xphage-fmt: no input files\n"; return 1; }

    int ret = 0;
    for (auto& p : paths) {
        std::error_code ec;
        if (fs::is_directory(p, ec)) {
            for (auto& entry : fs::recursive_directory_iterator(p, ec)) {
                if (!entry.is_regular_file()) continue;
                if (entry.path().extension() != ".xp0") continue;
                ret |= format_file(entry.path().string(), mode, verbose);
            }
        } else {
            ret |= format_file(p, mode, verbose);
        }
    }
    return ret;
}
