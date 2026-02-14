#include "../include/xphage.hpp"
#include <iostream>
#include <fstream>
#include <sstream>

/**
 * 🧬 X-Phage Omni-God Engine v3.1
 * Architecture: Neural-Native | UI: Titan Fusion (Recursive)
 */

// Helper to handle nested UI blocks recursively
size_t parse_ui_block(const std::vector<Token>& tokens, size_t index, XPhageRuntime& runtime, std::shared_ptr<FusionNode> parent) {
    size_t i = index;
    
    // If starting a block with {
    if (tokens[i].type == L_BRACE) i++;

    while (i < tokens.size()) {
        if (tokens[i].type == R_BRACE) {
            return i + 1; // End of block
        }

        // Handle UI Components
        if (tokens[i].type == VORTEX || tokens[i].type == ORBIT || tokens[i].type == Z_PLANE || 
            tokens[i].type == SIGNAL || tokens[i].type == VISION || tokens[i].type == TRIGGER || tokens[i].type == INPUT) {
            
            std::string type = tokens[i].value;
            std::string params = "Default";

            // Parse Parameters ( ... )
            if (i+1 < tokens.size() && tokens[i+1].type == LPAREN) {
                int j = i + 2;
                std::string param_build = "";
                while(j < tokens.size() && tokens[j].type != RPAREN) {
                    if(tokens[j].type == STRING) param_build += "\"" + tokens[j].value + "\"";
                    else if (tokens[j].type == COLON) param_build += ":";
                    else if (tokens[j].type == COMMA) param_build += ", ";
                    else param_build += tokens[j].value;
                    j++;
                }
                params = param_build;
                i = j + 1; // Skip past )
            }

            // Create Node
            auto node = std::make_shared<FusionNode>();
            node->type = type;
            node->props["raw_params"] = params;
            
            if (parent) parent->children.push_back(node);
            else runtime.current_ui_context->children.push_back(node); // Fallback

            // If component has children { ... }
            if (i < tokens.size() && tokens[i].type == L_BRACE) {
                 // Recursive call for children
                 i = parse_ui_block(tokens, i, runtime, node);
            } else {
                 i++;
            }
        } 
        else {
            i++; // Skip non-UI tokens inside UI block for safety
        }
    }
    return i;
}

int main(int argc, char* argv[]) {
    std::cout << "\033[1;35mX-PHAGE [OMNI-GOD v3.2 TITAN]\033[0m\n";
    std::cout << "\033[1;90mSystem: Recursive Fusion UI | Core: Neural-Bypass\033[0m\n\n";

    if (argc < 2) { std::cout << "Usage: xphage <file>\n"; return 1; }

    std::ifstream file(argv[1]);
    std::stringstream buffer; buffer << file.rdbuf();
    
    XPhageLexer lexer;
    XPhageRuntime runtime;
    XPhageLinker linker;

    std::vector<Token> tokens = lexer.tokenize(buffer.str());

    for (size_t i = 0; i < tokens.size(); ++i) {
        
        // --- 1. GLOBAL & LOGIC ---
        if (tokens[i].type == GLOBAL && i + 3 < tokens.size()) {
            runtime.write_global(tokens[i+1].value, tokens[i+3].value); i += 3;
        }
        else if (tokens[i].type == LINK) { linker.link_library(tokens[i+1].value, runtime); i++; }
        
        // --- 2. TITAN FUSION UI ENTRY ---
        // Handles: fusion "Name" { ... }
        else if (tokens[i].type == FUSION) {
             runtime.init_fusion_engine();
             
             // Create Root Container
             auto root = std::make_shared<FusionNode>();
             root->type = "FUSION_ROOT";
             root->props["name"] = (i+1 < tokens.size()) ? tokens[i+1].value : "App";
             runtime.ui_root = root; // Reset root
             
             i += 2; // Skip 'fusion' and 'Name'
             
             // Enter Recursive Parse
             if (tokens[i].type == L_BRACE) {
                 i = parse_ui_block(tokens, i, runtime, root);
                 runtime.render_ui_tree(); // Draw after building
             }
        }

        // --- 3. HARDWARE OPS ---
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
            std::cout << "\033[1;97m>> " << ((cell.data != "raw_ref") ? cell.data : tokens[i+1].value) << "\033[0m\n";
            i++;
        }
    }

    return 0;
}
