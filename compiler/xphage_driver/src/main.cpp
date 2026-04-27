#include "../../../include/xphage/runtime.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cstdlib>

#ifdef __APPLE__
    #include <TargetConditionals.h>
#endif

// ---------------------------------------------------------
// 🧠 SMART COMPILER STRATEGY
// ---------------------------------------------------------
void compile_project(std::string source_file) {
    std::ifstream file(source_file);
    if (!file.is_open()) { std::cerr << "Error: Could not open " << source_file << "\n"; return; }
    std::stringstream buffer;
    buffer << file.rdbuf();

    XPhageLexer lexer;
    std::vector<Token> tokens = lexer.tokenize(buffer.str(), source_file);

    #ifdef ENABLE_LLVM
        std::cout << "\033[1;35m[BUILDER] 🧬 Selected Backend: LLVM (Titan Core)\033[0m\n";
        XPhageLLVMCompiler llvm_compiler;
        llvm_compiler.compile_tokens(tokens, "output.o");
        std::cout << "\033[1;33m[LINKER] 🔗 Linking native object...\033[0m\n";
        int res = std::system("clang output.o -o output_app -lm");
        if (res == 0) std::cout << "\033[1;32m[SUCCESS] ✅ Native Binary Built: ./output_app\033[0m\n";
    #else
        std::cout << "\033[1;35m[BUILDER] 🚀 Selected Backend: Titan Transpiler\033[0m\n";
        XPhageTranspiler transpiler;
        std::string cpp_out = "output_gen.cpp";
        transpiler.transpile_to_cpp(tokens, cpp_out);
        #if defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE
            std::cout << "\033[1;32m[iOS] ✅ Source generated: " << cpp_out << "\033[0m\n";
        #else
            std::string compiler_cmd;
            #ifdef _WIN32
                compiler_cmd = "g++ " + cpp_out + " -o output_app.exe -std=c++17 -O3";
            #else
                compiler_cmd = "c++ " + cpp_out + " -o output_app -std=c++17 -O3 -pthread";
            #endif
            std::cout << "\033[1;33m[GCC/CLANG] 🔨 Compiling...\033[0m\n";
            int res = std::system(compiler_cmd.c_str());
            if (res == 0) std::cout << "\033[1;32m[SUCCESS] ✅ Binary Built: ./output_app\033[0m\n";
            else          std::cerr << "\033[1;31m[ERROR] ❌ Compilation failed.\033[0m\n";
        #endif
    #endif
}

// ---------------------------------------------------------
// 🎨 UI PARSER (Recursive)
// ---------------------------------------------------------
size_t parse_ui_block(const std::vector<Token>& tokens, size_t index,
                      XPhageRuntime& runtime, std::shared_ptr<FusionNode> parent) {
    size_t i = index;
    if (tokens[i].type == L_BRACE) i++;
    while (i < tokens.size()) {
        if (tokens[i].type == R_BRACE) return i + 1;
        if (tokens[i].type == VORTEX || tokens[i].type == ORBIT ||
            tokens[i].type == Z_PLANE || tokens[i].type == SIGNAL ||
            tokens[i].type == VISION  || tokens[i].type == TRIGGER ||
            tokens[i].type == INPUT) {
            std::string type   = tokens[i].value;
            std::string params = "Default";
            if (i + 1 < tokens.size() && tokens[i + 1].type == LPAREN) {
                int j = i + 2; std::string pb;
                while (j < (int)tokens.size() && tokens[j].type != RPAREN) { pb += tokens[j].value; j++; }
                params = pb; i = j + 1;
            }
            auto node = std::make_shared<FusionNode>(type);
            node->props["raw_params"] = params;
            if (parent) parent->children.push_back(node);
            else        runtime.ui_root = node;
            if (i + 1 < tokens.size() && tokens[i + 1].type == L_BRACE)
                i = parse_ui_block(tokens, i + 1, runtime, node);
            else i++;
        } else { i++; }
    }
    return i;
}

// ---------------------------------------------------------
// 📢 BANNER & HELP
// ---------------------------------------------------------
void print_banner() {
    std::cout << "\033[1;36m";
    std::cout << "  _  _  ____  __  __ \n";
    std::cout << " ( \\/ )(  _ \\(  \\/  )\n";
    std::cout << "  )  (  )___/ )    ( \n";
    std::cout << " (_/\\_)(__)  (_/\\/_) \033[1;32mv3.5.0\033[0m\n\n";
    std::cout << "\033[1;35mX-Phage Language\033[0m\n";
    std::cout << "\033[1;33m⚡ AeonCoreX Lab | LLVM + Transpiler\033[0m\n";
    std::cout << "\033[1;36m💡 Type 'exit' to quit. 'help' for commands.\033[0m\n";
}

void repl_help() {
    std::cout << "\n\033[1;33mREPL commands:\033[0m\n"
              << "  exit             - Exit the shell\n"
              << "  help             - Show this help\n"
              << "  fusion { ... }   - Declare a UI block\n";
}

void print_usage() {
    std::cout << "\033[1;33mUsage:\033[0m\n"
              << "  xphage                    Start REPL\n"
              << "  xphage run <file.xp0>     Execute a file\n"
              << "  xphage build <file.xp0>   Compile to native binary\n"
              << "  xphage init               Create new project\n"
              << "  xphage -v | --version     Show version\n"
              << "\n\033[1;33mPackage Management:\033[0m\n"
              << "  Use \033[1;36mxpm\033[0m for all package operations.\n"
              << "  Install XPM: https://github.com/AeonCoreX-Lab/XPM\n"
              << "\n  xpm add <pkg>             Install a package\n"
              << "  xpm publish               Publish to registry\n"
              << "  xpm lock                  Generate lock file\n"
              << "  xpm sync <module>         Sync a stdlib module\n"
              << "  xpm registry list         List registries\n";
}

// ---------------------------------------------------------
// 🔄 REPL & EXECUTION
// ---------------------------------------------------------
void start_repl() {
    print_banner();
    std::string line;
    XPhageRuntime runtime;
    XPhageLexer lexer;
    runtime.ui_root = std::make_shared<FusionNode>("ROOT_CANVAS");
    while (true) {
        std::cout << "\033[1;32mxp> \033[0m";
        std::getline(std::cin, line);
        if (line == "exit") break;
        if (line.empty()) continue;
        if (line == "help") { repl_help(); continue; }
        std::vector<Token> tokens = lexer.tokenize(line, "<repl>");
        if (!tokens.empty() && tokens[0].type == FUSION) {
            parse_ui_block(tokens, 1, runtime, runtime.ui_root);
            runtime.render_ui_tree();
        }
    }
}

void execute_file(std::string filename) {
    std::ifstream file(filename);
    if (!file.is_open()) { std::cerr << "Error: Could not open " << filename << "\n"; return; }
    std::stringstream buffer;
    buffer << file.rdbuf();
    XPhageLexer lexer;
    std::vector<Token> tokens = lexer.tokenize(buffer.str(), filename);
    XPhageRuntime runtime;
    runtime.ui_root = std::make_shared<FusionNode>("ROOT_CANVAS");
    for (size_t i = 0; i < tokens.size(); i++) {
        if (tokens[i].type == FUSION) {
            i = parse_ui_block(tokens, i + 1, runtime, runtime.ui_root);
            runtime.render_ui_tree();
        }
    }
}

// ---------------------------------------------------------
// Package-management redirect to xpm binary
// ---------------------------------------------------------
static int xpm_redirect(const std::string& args) {
#if defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE
    std::cout << "\033[1;33m[iOS] ⚠️  Package management not available in sandbox.\033[0m\n";
    return 1;
#else
    const char* probe_null =
        #ifdef _WIN32
        "NUL";
        #else
        "/dev/null";
        #endif
    std::string probe_cmd = "xpm --version >\"";
    probe_cmd += probe_null;
    probe_cmd += "\" 2>&1";
    if (std::system(probe_cmd.c_str()) != 0) {
        std::cerr << "\033[1;31m[xphage] Package management requires XPM.\033[0m\n";
        std::cerr << "  Install: https://github.com/AeonCoreX-Lab/XPM\n";
        return 1;
    }
    return std::system(("xpm " + args).c_str());
#endif
}

// ---------------------------------------------------------
// ⚙️ MAIN ENTRY POINT
// ---------------------------------------------------------
int main(int argc, char* argv[]) {
    if (argc == 1) { start_repl(); return 0; }

    std::string cmd = argv[1];

    if (cmd == "-v" || cmd == "--version") {
        std::cout << "X-Phage v3.5.0\nDeveloped by AeonCoreX Lab\n"
                  << "https://github.com/AeonCoreX-Lab/X-Phage\n";
        return 0;
    }
    else if (cmd == "run"   && argc > 2) { execute_file(argv[2]);    return 0; }
    else if (cmd == "build" && argc > 2) { compile_project(argv[2]); return 0; }
    else if (cmd == "init") {
        #if defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE
            std::cout << "\033[1;33m[iOS] ⚠️ Init skipped (Sandbox Mode).\033[0m\n";
        #else
            std::system("mkdir -p src stdlib modules bin");
            std::system("touch src/main.xp0");
            std::ofstream pkg("xphage.pkg");
            pkg << "[package]\nname    = \"my-xphage-app\"\nversion = \"0.1.0\"\n"
                << "author  = \"\"\ndesc    = \"\"\n\n[dependencies]\n";
            pkg.close();
            std::cout << "\033[1;32m[SUCCESS] Project initialized.\033[0m\n"
                      << "  Edit xphage.pkg to configure your project.\n"
                      << "  Run \033[1;36mxpm add <package>\033[0m to add dependencies.\n";
        #endif
        return 0;
    }
    // Package commands — forward to xpm (backward-compat shims)
    else if ((cmd == "install" || cmd == "add") && argc > 2)
        return xpm_redirect("add " + std::string(argv[2]));
    else if (cmd == "publish")       return xpm_redirect("publish");
    else if (cmd == "lock")          return xpm_redirect("lock");
    else if (cmd == "sync" && argc > 2)
        return xpm_redirect("sync " + std::string(argv[2]));
    else if (cmd == "update-stdlib") return xpm_redirect("update-stdlib");
    else if (cmd == "registry") {
        std::string sub = "registry";
        for (int i = 2; i < argc; i++) sub += " " + std::string(argv[i]);
        return xpm_redirect(sub);
    }
    else { print_usage(); }

    return 0;
}
