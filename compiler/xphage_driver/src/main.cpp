// ============================================================
// xphage_driver v4.1.0 — Smart Multi-Format Compiler
// Handles: .xp / .xp0 / .xh / .xui / mixed
// AeonCoreX Lab
// ============================================================
#include "../../xphage_interface/include/interface.hpp"
#include "../../xphage_linker/include/linker.hpp"
#include "../../xphage_codegen_llvm/include/codegen_llvm.hpp"
#include <iostream>
#include <vector>
#include <string>
#include <filesystem>
#include <unordered_set>
#include <sstream>

using namespace xphage::interface;

static void print_version() {
    std::cout
        << "\033[1;36mX-Phage Language v4.1.0\033[0m\n"
        << "  Phases 1-4 + Single-file .xp + Smart Tri-Modular\n"
        << "  LLVM: "
        << (xphage::codegen_llvm::is_available()
              ? xphage::codegen_llvm::llvm_version_str()
              : "disabled (build with -DENABLE_LLVM=ON)")
        << "\n"
        << "  AeonCoreX Lab — github.com/AeonCoreX-Lab\n";
}

static void print_usage(const char* prog) {
    std::cout <<
R"(Usage:
  # Single-file (most flexible):
  )" << prog << R"( main.xp                     Compile + run
  )" << prog << R"( build main.xp               Compile to binary

  # Tri-Modular (explicit layers):
  )" << prog << R"( main.xp0                    Compile execution layer
  )" << prog << R"( models.xh main.xp0          Logic + Execution
  )" << prog << R"( models.xh app.xui main.xp0  Full Tri-Modular

  # Split single-file → Tri-Modular:
  )" << prog << R"( split main.xp               Write .xh + .xui + .xp0
  )" << prog << R"( split --dry-run main.xp     Preview split (no write)
  )" << prog << R"( split --out=src/ main.xp    Split to directory

  # Tools:
  )" << prog << R"( repl                         Interactive REPL
  )" << prog << R"( --emit=cpp  <files...>       Emit generated C++
  )" << prog << R"( --emit=ir   <files...>       Emit XPIR text
  )" << prog << R"( --emit=ast  <files...>       Dump AST
  )" << prog << R"( --emit=llvm <files...>       Emit LLVM IR  [Phase 4]
  )" << prog << R"( --backend=llvm <files...>    LLVM native   [Phase 4]
  )" << prog << R"( --backend=xil  <files...>    XIL backend (IR-based C++ codegen)
  )" << prog << R"( --libs                       List stdlib modules
  )" << prog << R"( --version                    Show version
  )" << prog << R"( -O / -O3                     Optimise
  )" << prog << R"( -v                           Verbose

File Extensions:
  .xp   → Single file (all layers — beginner friendly)
  .xh   → Logic Layer   (declarations, forge, nexus)
  .xui  → UI Layer      (fusion components, strands)
  .xp0  → Execution Layer (entry, logic bodies, events)
)";
}

static bool is_source_file(const std::string& s) {
    if (s.size() > 4 && s.substr(s.size()-4) == ".xp0") return true;
    if (s.size() > 4 && s.substr(s.size()-4) == ".xui") return true;
    if (s.size() > 3 && s.substr(s.size()-3) == ".xp")  return true;
    if (s.size() > 3 && s.substr(s.size()-3) == ".xh")  return true;
    return false;
}

static std::string derive_output(const std::vector<std::string>& sources) {
    if (sources.empty()) return "a.out";
    std::string base;
    for (auto& s : sources) {
        auto ext = s.find_last_of('.');
        std::string e = ext != std::string::npos ? s.substr(ext) : "";
        if (e == ".xp0" || e == ".xp") { base = s.substr(0, ext); break; }
    }
    if (base.empty()) {
        auto ext = sources[0].find_last_of('.');
        base = (ext != std::string::npos) ? sources[0].substr(0, ext) : sources[0];
    }
    auto sep = base.find_last_of("/\\");
    return (sep != std::string::npos) ? base.substr(sep+1) : base;
}

// ── Project Discovery (design doc Stage 1) + Module Loader (Stage 3) ──
// AST-based, not filename-based: a .xp and a same-named Tri-Modular
// set (.xh/.xui/.xp0) can legitimately both exist for unrelated
// reasons — two independent examples of the same program, a
// migration in progress, etc. — so "the filename matches" is not
// sufficient evidence that they're meant to be merged together.
//
// The actual decision:
//   1. Parse the .xp and ask what it already has (analyze_xp_layers).
//   2. If it's already self-sufficient (has a real execution entry
//      point), compile it standalone — don't even look at siblings.
//      A working program is never silently combined with something
//      else just because a same-named file happens to sit nearby.
//   3. If it's missing the UI layer specifically (no fusion/strand),
//      and a sibling .xui exists, check that sibling for symbol
//      names which collide with the .xp's own declared symbols.
//        - No collision → merge (it's genuinely complementary).
//        - Collision → don't guess; warn and fall back to compiling
//          the .xp standalone (only safe if it's self-sufficient on
//          its own — see step 2) or surface a clear ambiguity error
//          (if it's NOT self-sufficient and the only candidate
//          sibling is unusable due to the collision).
//   4. The same logic applies independently for a missing Logic
//      layer (.xh) or missing Execution layer (.xp0).
struct DiscoveryResult {
    std::vector<std::string> files;       // .xp plus whichever siblings were
                                            // judged safe to merge
    std::vector<std::string> warnings;    // ambiguity notices to surface
                                            // to the user even when we did
                                            // pick a safe default
};

static DiscoveryResult discover_sibling_modules(const std::string& xp_path, bool verbose) {
    namespace fs = std::filesystem;
    DiscoveryResult result;
    result.files.push_back(xp_path);

    auto layers = xphage::interface::analyze_xp_layers(xp_path);

    if (layers.is_self_sufficient) {
        if (verbose)
            std::cout << "\033[1;36m[discover]\033[0m " << xp_path
                      << " is self-sufficient (has its own execution entry point) — "
                      << "compiling standalone, not checking for sibling modules\n";
        return result; // exactly {xp_path}, untouched
    }

    fs::path p(xp_path);
    std::string stem = p.stem().string();
    fs::path dir = p.has_parent_path() ? p.parent_path() : fs::path(".");

    // Only look for siblings that would fill a layer this .xp
    // genuinely doesn't have. A .xp with no UI of its own but a
    // real execution entry point, for instance, has no need for a
    // sibling .xp0 even if one exists — that case is already
    // handled by the is_self_sufficient check above; what's left
    // here is specifically the "missing execution, and possibly
    // missing UI/logic too" case.
    struct SiblingCandidate { const char* ext; bool needed; };
    SiblingCandidate candidates[] = {
        {".xh",  !layers.has_logic},
        {".xui", !layers.has_ui},
        {".xp0", !layers.has_execution},
    };

    std::unordered_set<std::string> own_symbols(
        layers.declared_symbol_names.begin(), layers.declared_symbol_names.end());

    for (auto& cand : candidates) {
        if (!cand.needed) continue;
        fs::path candidate_path = dir / (stem + cand.ext);
        std::error_code ec;
        if (!fs::exists(candidate_path, ec) || !fs::is_regular_file(candidate_path, ec)) continue;

        auto sibling_symbols = xphage::interface::declared_symbol_names_in_file(candidate_path.string());
        std::vector<std::string> collisions;
        for (auto& sym : sibling_symbols) {
            if (own_symbols.count(sym)) collisions.push_back(sym);
        }

        if (!collisions.empty()) {
            std::ostringstream w;
            w << "Found " << xp_path << " and " << candidate_path.string()
              << ", but they declare overlapping symbol(s) (";
            for (size_t i = 0; i < collisions.size(); i++) {
                if (i) w << ", ";
                w << collisions[i];
            }
            w << ") — these are likely two independent versions of the same example, "
              << "not complementary modules. Not merging automatically.";
            if (layers.has_execution) {
                w << " Compiling " << xp_path << " standalone.";
            } else {
                w << " " << xp_path << " has no execution entry point on its own and "
                  << candidate_path.string() << " was the only candidate to provide one — "
                  << "compile will likely fail; pass both files explicitly "
                  << "(xphage build " << xp_path << " " << candidate_path.string()
                  << ") if you do want them merged despite the name overlap.";
            }
            result.warnings.push_back(w.str());
            continue; // do not add this sibling to result.files
        }

        result.files.push_back(candidate_path.string());
        if (verbose)
            std::cout << "\033[1;36m[discover]\033[0m " << candidate_path.string()
                      << " fills the missing " << cand.ext << " layer — merging\n";
    }

    return result;
}

int main(int argc, char* argv[]) {
    if (argc < 2) { start_repl(); return 0; }

    std::string cmd = argv[1];
    if (cmd == "--version" || cmd == "-V") { print_version(); return 0; }
    if (cmd == "--help"    || cmd == "-h") { print_usage(argv[0]); return 0; }
    if (cmd == "repl")                     { start_repl(); return 0; }
    if (cmd == "--libs")   { xphage::linker::list_stdlib_modules(); return 0; }

    if (cmd == "split") {
        SplitConfig scfg;
        for (int i = 2; i < argc; i++) {
            std::string a = argv[i];
            if (a == "--dry-run")        scfg.dry_run = true;
            else if (a == "-v")          scfg.verbose = true;
            else if (a.substr(0,6) == "--out=") scfg.output_dir = a.substr(6);
            else if (is_source_file(a))  scfg.source_path = a;
        }
        if (scfg.source_path.empty()) {
            std::cerr << "\033[1;31m[error]\033[0m split: no source file given\n";
            return 1;
        }
        auto res = split_xp(scfg);
        if (!res.success) {
            std::cerr << "\033[1;31m[error]\033[0m " << res.error << "\n";
            return 1;
        }
        return 0;
    }

    CompileConfig cfg;
    cfg.verbose  = false;
    cfg.optimize = false;
    cfg.emit     = EmitKind::Binary;
    cfg.backend  = Backend::Transpiler;

    std::vector<std::string> sources;
    bool build_only = false;

    for (int i = 1; i < argc; i++) {
        std::string a = argv[i];
        if (a == "build")             { build_only = true;   continue; }
        if (a == "run")               { build_only = false;  continue; }
        if (a == "-v" || a == "--verbose") { cfg.verbose  = true; continue; }
        if (a == "-O" || a == "-O3")       { cfg.optimize = true; continue; }
        if (a == "--emit=cpp")  { cfg.emit = EmitKind::Transpiled; continue; }
        if (a == "--emit=ir")   { cfg.emit = EmitKind::IR;         continue; }
        if (a == "--emit=ast")  { cfg.emit = EmitKind::AST;        continue; }
        if (a == "--emit=llvm") { cfg.emit = EmitKind::LLVMir;  cfg.backend = Backend::LLVM; continue; }
        if (a == "--emit=obj")  { cfg.emit = EmitKind::Object;  cfg.backend = Backend::LLVM; continue; }
        if (a == "--backend=llvm")       { cfg.backend = Backend::LLVM;       continue; }
        if (a == "--backend=transpiler") { cfg.backend = Backend::Transpiler; continue; }
        if (a == "--backend=xil")        { cfg.backend = Backend::XIL;        continue; }
        if (a == "-o" && i+1 < argc)  { cfg.output_path = argv[++i]; continue; }
        if (a.size()>3 && a.substr(0,3)=="-o") { cfg.output_path = a.substr(3); continue; }
        if (a.size()>9 && a.substr(0,9)=="--target=") { cfg.target_triple = a.substr(9); continue; }
        if (a.size()>15 && a.substr(0,15)=="--library-path=") { cfg.library_path = a.substr(15); continue; }
        // FFI native-library linking: forwarded verbatim to the
        // final C++ link step (e.g. -lnative_demo, -L/path/to/libs,
        // or a direct .a/.so/.lib path).
        if (a.size()>2 && a.substr(0,2)=="-l") { cfg.link_libs.push_back(a); continue; }
        if (a.size()>2 && a.substr(0,2)=="-L") { cfg.link_libs.push_back(a); continue; }
        if (a.size()>4 && (a.substr(a.size()-2)==".a" || a.substr(a.size()-3)==".so")) {
            cfg.link_libs.push_back(a); continue;
        }
        if (is_source_file(a)) sources.push_back(a);
    }

    if (sources.empty()) {
        std::cerr << "\033[1;31m[error]\033[0m no source file specified\n";
        print_usage(argv[0]);
        return 1;
    }

    if (cfg.output_path.empty()) cfg.output_path = derive_output(sources);

    CompileResult res;

    if (sources.size() == 1) {
        cfg.source_path = sources[0];
        std::string ext;
        auto dot = sources[0].find_last_of('.');
        if (dot != std::string::npos) ext = sources[0].substr(dot);

        if (ext == ".xp") {
            auto discovery = discover_sibling_modules(sources[0], cfg.verbose);
            for (auto& w : discovery.warnings) {
                std::cerr << "\033[1;33m[warning]\033[0m " << w << "\n";
            }
            if (discovery.files.size() > 1) {
                // A sibling module was found that fills a layer this
                // .xp genuinely doesn't have on its own, with no
                // symbol-name collision — safe to merge (Mixed mode,
                // design doc Stage 3).
                if (cfg.verbose)
                    std::cout << "\033[1;36m[mixed]\033[0m " << sources[0]
                              << " + " << (discovery.files.size()-1) << " discovered sibling module(s)\n";
                res = compile_multi(discovery.files, cfg);
            } else {
                if (cfg.verbose) std::cout << "\033[1;36m[xp]\033[0m Single-file mode\n";
                res = compile_xp(cfg);
            }
        } else {
            res = compile(cfg);
        }
    } else {
        if (cfg.verbose) {
            std::cout << "\033[1;36m[multi]\033[0m " << sources.size() << " files:\n";
            for (auto& s : sources) std::cout << "  " << s << "\n";
        }
        res = compile_multi(sources, cfg);
    }

    if (!res.success) {
        for (auto& e : res.errors) {
            // A formatted Diagnostic (from the Semantic Analyzer)
            // already carries its own complete ANSI styling,
            // including internal resets between the colored header
            // and the plain-text body/snippet — wrapping the whole
            // string in another color code would re-color text that
            // was deliberately left uncolored, and is unnecessary
            // besides. Plain error strings (lexer/parser errors,
            // "Cannot open: ...", etc.) have no ANSI codes of their
            // own and still get the simple red-wrap treatment.
            if (e.rfind("\033[", 0) == 0) {
                std::cerr << e << "\n";
            } else {
                std::cerr << "\033[1;31m" << e << "\033[0m\n";
            }
        }
        return 1;
    }
    for (auto& w : res.warnings) {
        if (w.rfind("\033[", 0) == 0) {
            std::cerr << w << "\n";
        } else {
            std::cerr << "\033[1;33m[warn] " << w << "\033[0m\n";
        }
    }

    if (build_only || cfg.emit != EmitKind::Binary) {
        if (cfg.verbose || cfg.emit != EmitKind::Binary)
            std::cout << "\033[1;32m[ok]\033[0m " << res.output_path
                      << "  (" << res.elapsed_ms << " ms)\n";
        return 0;
    }

    if (res.output_path.empty()) {
        std::cerr << "\033[1;31m[error]\033[0m no binary to run\n";
        return 1;
    }
    std::string run_cmd = res.output_path;
    if (run_cmd.find('/') == std::string::npos) run_cmd = "./" + run_cmd;
    return std::system(run_cmd.c_str());
}
