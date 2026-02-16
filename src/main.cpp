#include "../include/xphage.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cstdlib>

// --- FIX: Apple Security Header ---
#ifdef __APPLE__
    #include <TargetConditionals.h>
#endif

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
                    param_build += tokens[j].value;
                    j++;
                }
                params = param_build;
                i = j + 1; 
            }

            // --- Node Creation (Now works with Constructor) ---
            auto node = std::make_shared<FusionNode>(type);
            node->props["raw_params"] = params;
            
            if (parent) {
                parent->children.push_back(node);
            } else {
                runtime.ui_root = node;
            }

            // Check for children (Nested Block)
            if (i+1 < tokens.size() && tokens[i+1].type == L_BRACE) {
                i = parse_ui_block(tokens, i+1, runtime, node);
            } else {
                i++;
            }
        } else {
            i++;
        }
    }
    return i;
}

// REPL Mode
void start_repl() {
    std::cout << "\033[1;35mX-Phage Titan Shell [v3.3.1] | Type 'exit' to quit\033[0m\n";
    std::string line;
    XPhageRuntime runtime;
    XPhageLexer lexer;
    
    // Initialize Root UI Context
    runtime.ui_root = std::make_shared<FusionNode>("ROOT_CANVAS");

    while (true) {
        std::cout << "\033[1;32mxp> \033[0m";
        std::getline(std::cin, line);
        if (line == "exit") break;
        if (line.empty()) continue;

        // Simple REPL parsing (can be expanded)
        std::vector<Token> tokens = lexer.tokenize(line);
        if (!tokens.empty() && tokens[0].type == FUSION) {
             parse_ui_block(tokens, 1, runtime, runtime.ui_root);
             runtime.render_ui_tree();
        }
    }
}

// File Execution
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
    // --- Fix: Constructor Usage ---
    runtime.ui_root = std::make_shared<FusionNode>("ROOT_CANVAS");

    // Scan for UI blocks
    for (size_t i = 0; i < tokens.size(); i++) {
        if (tokens[i].type == FUSION) {
            i = parse_ui_block(tokens, i+1, runtime, runtime.ui_root);
            runtime.render_ui_tree();
        }
    }
}

// ---------------------------------------------------------
// ⚙️ CLI ARGUMENT PARSER
// ---------------------------------------------------------
int main(int argc, char* argv[]) {
    if (argc == 1) {
        start_repl(); 
    } else {
        std::string cmd = argv[1];
        
        if (cmd == "-v" || cmd == "--version") {
            std::cout << "X-Phage v3.3.1 (Titan Build)\n";
        } 
        else if (cmd == "run" && argc > 2) {
            execute_file(argv[2]); 
        } 
        else if (cmd == "build" && argc > 2) {
            compile_to_native(argv[2]); 
        } 
        else if (cmd == "init") {
            // Scaffold a new project
            // --- FIX: iOS Safety Check ---
            #if defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE
                std::cout << "\033[1;33m[iOS] ⚠️ Init skipped (Sandbox Mode).\033[0m\n";
            #else
                std::system("mkdir src stdlib modules bin && touch src/main.xp0");
                std::cout << "\033[1;32m[SUCCESS] X-Phage project initialized globally.\033[0m\n";
            #endif
        }
        else {
            std::cout << "Usage:\n";
            std::cout << "  xphage                  (Starts REPL)\n";
            std::cout << "  xphage run <file.xp0>   (Executes a file)\n";
            std::cout << "  xphage build <file.xp0> (Compiles to Native OS)\n";
            std::cout << "  xphage init             (Creates new project)\n";
        }
    }
    return 0;
}
