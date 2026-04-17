#include "../../include/xphage/runtime.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cstdlib>

#ifdef __APPLE__
    #include <TargetConditionals.h>
#endif

namespace XPM_Cloud {
    bool sync_module(std::string module_name);
    bool update_stdlib();
    bool install_package(const std::string& spec);
    bool publish_package();
    bool generate_lock();
    bool registry_list();
    bool registry_add(const std::string& name, const std::string& url);
    bool registry_remove(const std::string& name);
    bool registry_default(const std::string& name);
}

// ---------------------------------------------------------
// 🧠 SMART COMPILER STRATEGY
// ---------------------------------------------------------
void compile_project(std::string source_file) {
    std::ifstream file(source_file);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open " << source_file << "\n";
        return;
    }
    std::stringstream buffer;
    buffer << file.rdbuf();

    XPhageLexer lexer;
    std::vector<Token> tokens = lexer.tokenize(buffer.str());

    #ifdef ENABLE_LLVM
        std::cout << "\033[1;35m[BUILDER] 🧬 Selected Backend: LLVM (Titan Core)\033[0m\n";
        XPhageLLVMCompiler llvm_compiler;
        llvm_compiler.compile_tokens(tokens, "output.o");
        std::cout << "\033[1;33m[LINKER] 🔗 Linking native object...\033[0m\n";
        int res = std::system("clang output.o -o output_app -lm");
        if (res == 0) std::cout << "\033[1;32m[SUCCESS] ✅ Native Binary Built: ./output_app\033[0m\n";
    #else
        std::cout << "\033[1;35m[BUILDER] 🚀 Selected Backend: Titan Transpiler (Universal C++)\033[0m\n";
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
            if (res == 0)
                std::cout << "\033[1;32m[SUCCESS] ✅ Binary Built: ./output_app\033[0m\n";
            else
                std::cerr << "\033[1;31m[ERROR] ❌ Compilation failed.\033[0m\n";
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
                int j = i + 2;
                std::string param_build;
                while (j < (int)tokens.size() && tokens[j].type != RPAREN) {
                    param_build += tokens[j].value;
                    j++;
                }
                params = param_build;
                i = j + 1;
            }

            auto node = std::make_shared<FusionNode>(type);
            node->props["raw_params"] = params;
            if (parent) parent->children.push_back(node);
            else        runtime.ui_root = node;

            if (i + 1 < tokens.size() && tokens[i + 1].type == L_BRACE)
                i = parse_ui_block(tokens, i + 1, runtime, node);
            else
                i++;
        } else {
            i++;
        }
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
    std::cout << "\033[1;35mXPM - X-Phage Package Manager\033[0m\n";
    std::cout << "\033[1;33m⚡ AeonCoreX Lab | LLVM + Transpiler | Registry: github.com/AeonCoreX-Lab/xpm-registry\033[0m\n";
    std::cout << "\033[1;36m💡 Type 'exit' to quit. 'help' for commands.\033[0m\n";
}

void repl_help() {
    std::cout << "\n\033[1;33mREPL commands:\033[0m\n";
    std::cout << "  exit             - Exit the shell\n";
    std::cout << "  help             - Show this help\n";
    std::cout << "  fusion { ... }   - Declare a UI block\n";
}

void print_usage() {
    std::cout << "\033[1;33mUsage:\033[0m\n";
    std::cout << "  xphage                              Start REPL\n";
    std::cout << "  xphage run <file.xp0>               Execute a file\n";
    std::cout << "  xphage build <file.xp0>             Compile to native binary\n";
    std::cout << "  xphage init                         Create new project\n";
    std::cout << "\n\033[1;33mPackage Management:\033[0m\n";
    std::cout << "  xphage install <pkg>[@ver]          Install a package\n";
    std::cout << "  xphage publish                      Publish package to registry\n";
    std::cout << "  xphage lock                         Generate xphage.lock\n";
    std::cout << "  xphage sync <module>                Download a stdlib module\n";
    std::cout << "  xphage update-stdlib                Download full stdlib\n";
    std::cout << "\n\033[1;33mRegistry:\033[0m\n";
    std::cout << "  xphage registry list                List registries\n";
    std::cout << "  xphage registry add <n> <url>       Add private registry\n";
    std::cout << "  xphage registry remove <n>          Remove registry\n";
    std::cout << "  xphage registry default <n>         Set default registry\n";
    std::cout << "                                      (use 'github' to reset)\n";
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

        std::vector<Token> tokens = lexer.tokenize(line);
        if (!tokens.empty() && tokens[0].type == FUSION) {
            parse_ui_block(tokens, 1, runtime, runtime.ui_root);
            runtime.render_ui_tree();
        }
    }
}

void execute_file(std::string filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open " << filename << "\n";
        return;
    }
    std::stringstream buffer;
    buffer << file.rdbuf();

    XPhageLexer lexer;
    std::vector<Token> tokens = lexer.tokenize(buffer.str());
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
// ⚙️ MAIN ENTRY POINT
// ---------------------------------------------------------
int main(int argc, char* argv[]) {
    if (argc == 1) {
        start_repl();
        return 0;
    }

    std::string cmd = argv[1];

    // Version
    if (cmd == "-v" || cmd == "--version") {
        std::cout << "XPM v3.5.0 (X-Phage Package Manager)\n";
        std::cout << "Developed by AeonCoreX Lab\n";
        std::cout << "Registry: github.com/" << "AeonCoreX-Lab/xpm-registry\n";
    }

    // Run / Build
    else if (cmd == "run" && argc > 2)   execute_file(argv[2]);
    else if (cmd == "build" && argc > 2) compile_project(argv[2]);

    // Init
    else if (cmd == "init") {
        #if defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE
            std::cout << "\033[1;33m[iOS] ⚠️ Init skipped (Sandbox Mode).\033[0m\n";
        #else
            std::system("mkdir -p src stdlib modules bin");
            std::system("touch src/main.xp0");
            std::ofstream pkg("xphage.pkg");
            pkg << "[package]\n"
                << "name    = \"my-xphage-app\"\n"
                << "version = \"0.1.0\"\n"
                << "author  = \"\"\n"
                << "desc    = \"\"\n\n"
                << "[dependencies]\n";
            pkg.close();
            std::cout << "\033[1;32m[SUCCESS] Project initialized.\033[0m\n";
            std::cout << "  Edit xphage.pkg to set your project name and dependencies.\n";
        #endif
    }

    // Package management
    else if (cmd == "install") {
        if (argc < 3) {
            std::cerr << "\033[1;31m[XPM] Usage: xphage install <package>[@version]\033[0m\n";
            return 1;
        }
        XPM_Cloud::install_package(argv[2]);
    }
    else if (cmd == "publish")      XPM_Cloud::publish_package();
    else if (cmd == "lock")         XPM_Cloud::generate_lock();
    else if (cmd == "sync" && argc > 2) XPM_Cloud::sync_module(argv[2]);
    else if (cmd == "update-stdlib")    XPM_Cloud::update_stdlib();

    // Registry
    else if (cmd == "registry") {
        std::string sub = (argc >= 3) ? argv[2] : "list";

        if (sub == "list")
            XPM_Cloud::registry_list();
        else if (sub == "add" && argc >= 5)
            XPM_Cloud::registry_add(argv[3], argv[4]);
        else if (sub == "remove" && argc >= 4)
            XPM_Cloud::registry_remove(argv[3]);
        else if (sub == "default" && argc >= 4)
            XPM_Cloud::registry_default(argv[3]);
        else {
            std::cerr << "\033[1;31m[XPM] Unknown registry subcommand.\033[0m\n";
            std::cerr << "  xphage registry list\n"
                      << "  xphage registry add <n> <url>\n"
                      << "  xphage registry remove <n>\n"
                      << "  xphage registry default <n|github>\n";
            return 1;
        }
    }

    // Unknown
    else {
        print_usage();
    }

    return 0;
}
