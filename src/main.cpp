#include "../include/xphage.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

/**
 * 🧬 X-Phage Omni-God Engine v3.3 [TITAN CLI]
 * Architecture: Neural-Native | Env: Global System PATH
 */

namespace XPM_Cloud {
    bool sync_module(std::string module_name);
}

// LLVM Compiler Entry Point (From llvm_compiler.cpp)
extern void compile_to_native(std::string source_file);

// Helper to handle nested UI blocks recursively
size_t parse_ui_block(const std::vector<Token>& tokens, size_t index, XPhageRuntime& runtime, std::shared_ptr<FusionNode> parent) {
    size_t i = index;
    if (tokens[i].type == L_BRACE) i++;

    while (i < tokens.size()) {
        if (tokens[i].type == R_BRACE) return i + 1;

        if (tokens[i].type == VORTEX || tokens[i].type == ORBIT || tokens[i].type == Z_PLANE || 
            tokens[i].type == SIGNAL || tokens[i].type == VISION || tokens[i].type == TRIGGER || tokens[i].type == INPUT) {
            
            std::string type = tokens[i].value;
            std::string params = "Default";

            if (i+1 < tokens.size() && tokens[i+1].type == LPAREN) {
                int j = i + 2;
                std::string param_build = "";
                while(j < tokens.size() && tokens[j].type != RPAREN) {
                    param_build += tokens[j].value + " "; j++;
                }
                params = param_build;
                i = j; 
            }

            auto node = std::make_shared<FusionNode>(type);
            node->props["raw_params"] = params;
            parent->children.push_back(node);
            i++;
        } else {
            i++;
        }
    }
    return i;
}

// ---------------------------------------------------------
// 🚀 CORE EXECUTION LOGIC (Run a .xp0 file)
// ---------------------------------------------------------
void execute_file(std::string filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "\033[1;31m[ERROR] Could not open file: \033[0m" << filepath << "\n";
        return;
    }
    std::stringstream buffer;
    buffer << file.rdbuf();

    XPhageLexer lexer;
    XPhageRuntime runtime;
    XPhageLinker linker;

    std::vector<Token> tokens = lexer.tokenize(buffer.str());

    for (size_t i = 0; i < tokens.size(); i++) {
        if (tokens[i].type == LINK && i + 1 < tokens.size()) {
            linker.link_library(tokens[i+1].value, runtime);
            i++;
        }
        else if (tokens[i].type == FUSION && i + 2 < tokens.size()) {
             std::string root_name = tokens[i+1].value;
             auto root = std::make_shared<FusionNode>("ROOT_CANVAS");
             root->props["name"] = root_name;
             runtime.ui_root = root; 
             
             i += 2; 
             if (tokens[i].type == L_BRACE) {
                 i = parse_ui_block(tokens, i, runtime, root);
                 runtime.render_ui_tree(); 
             }
        }
        else if (tokens[i].type == BYPASS) {
             std::string config = "AUTO";
             if (i+3 < tokens.size() && tokens[i+2].type == L_BRACE) { config = "KERNEL_INJECTED"; } 
             if (tokens[i+1].value == "torrent_engine") runtime.start_torrent_engine("AUTO_FETCHED");
             else runtime.hardware_bypass(tokens[i+1].value, config);
             i++;
        }
        else if (tokens[i].type == QUANTUM) { runtime.launch_quantum_process(tokens[i+1].value); i++; }
        else if (tokens[i].type == VOID) { runtime.activate_void_protocol(); }
        else if (tokens[i].type == BEAM) {
            MemoryCell cell = runtime.read(tokens[i+1].value);
            std::cout << "\033[1;32m[BEAM OUT] ⚡ \033[0m" << cell.data << "\n";
            i++;
        }
    }
}

// ---------------------------------------------------------
// 💻 REPL (Interactive Shell)
// ---------------------------------------------------------
void print_banner() {
    std::cout << "\033[1;36m"
              << "   _  __       ___  __                   \n"
              << "  | |/ /      / _ \\/ /  ___ ____ ____   \n"
              << "  |   /  __  / ___/ _ \\/ _ `/ _ `/ -_)  \n"
              << " /   |  / _/ /_/  /_//_/\\_,_/\\_, /\\__/  \n"
              << "/_/|_|                  /___/        \n"
              << "\033[0m"
              << "\033[1;35m 🧬 Omni-God Engine v3.3 [TITAN CLI]\033[0m\n"
              << " Architecture: LLVM Native | Env: Global\n\n";
}

void start_repl() {
    print_banner();
    std::cout << " Type \033[1;31m'exit'\033[0m to leave the matrix.\n\n";
    std::string line;
    while (true) {
        std::cout << "\033[1;32mX-Phage λ\033[0m ";
        if (!std::getline(std::cin, line) || line == "exit") break;
        if (line.empty()) continue;
        std::cout << "\033[1;30m>> [Processing locally...]\033[0m\n";
        // REPL Execution logic can be expanded here
    }
}

// ---------------------------------------------------------
// ⚙️ CLI ARGUMENT PARSER (Like Python/Node)
// ---------------------------------------------------------
int main(int argc, char* argv[]) {
    if (argc == 1) {
        start_repl(); // No args = Start interactive mode
    } else {
        std::string cmd = argv[1];
        
        if (cmd == "-v" || cmd == "--version") {
            std::cout << "X-Phage v3.3.0 (Titan Build)\n";
        } 
        else if (cmd == "run" && argc > 2) {
            execute_file(argv[2]); // Run interpreter
        } 
        else if (cmd == "build" && argc > 2) {
            compile_to_native(argv[2]); // Compile to native using LLVM
        } 
        else if (cmd == "init") {
            // Scaffold a new project (Like npm init)
            std::system("mkdir src stdlib modules bin && touch src/main.xp0");
            std::cout << "\033[1;32m[SUCCESS] X-Phage project initialized globally.\033[0m\n";
        }
        else {
            std::cout << "Usage:\n";
            std::cout << "  xphage                  (Starts REPL)\n";
            std::cout << "  xphage run <file.xp0>   (Executes a file)\n";
            std::cout << "  xphage build <file.xp0> (Compiles to Native OS Binary via LLVM)\n";
            std::cout << "  xphage init             (Creates project structure)\n";
            std::cout << "  xphage --version        (Shows version)\n";
        }
    }
    return 0;
}
