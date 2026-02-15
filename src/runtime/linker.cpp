#include "../../include/xphage.hpp"
#include <fstream>
#include <iostream>
#include <regex>

/**
 * 🔗 X-Phage Intelligent Linker v3.2
 * Architecture: Logic (.xh) & UI (.xui) Separation
 * Features: Global Registry, Hardware Hooks, Cloud Sync
 */

namespace XPM_Cloud {
    bool sync_module(std::string module_name);
}

void XPhageLinker::link_library(std::string lib_name, XPhageRuntime& runtime) {
    
    std::string clean_name = lib_name.substr(0, lib_name.find("."));
    
    // --- 1. FILE RESOLUTION SYSTEM ---
    
    // Priority 1: Check in current directory
    std::string path = lib_name;
    std::ifstream file(path);
    
    // Priority 2: Check in modules folder (XPM Downloaded)
    if (!file.is_open()) {
        path = "modules/" + clean_name + "/" + lib_name;
        file.open(path);
    }

    // Priority 3: Check in stdlib
    if (!file.is_open()) {
        path = "stdlib/" + lib_name;
        file.open(path);
    }

    // Priority 4: XPM Cloud Auto-Fetch
    if (!file.is_open()) {
        std::cout << "\033[1;33m[LINKER] ⚠️ Module '" << lib_name << "' not found locally.\033[0m\n";
        if (XPM_Cloud::sync_module(clean_name)) {
            path = "modules/" + clean_name + "/" + lib_name;
            file.open(path);
        } else {
            std::cerr << "\033[1;31m[SYS PANIC] ⛔ Module resolution failed for: " << lib_name << "\033[0m\n";
            return;
        }
    }

    std::cout << "\033[1;34m[LINKER] 🔗 Resolving Neural Pathways from: " << lib_name << "\033[0m\n";

    std::string line;
    std::smatch match;

    while (std::getline(file, line)) {
        
        // ==========================================
        // 🎨 .xui FILE PARSING (TITAN FUSION UI)
        // ==========================================
        if (lib_name.find(".xui") != std::string::npos) {
            if (std::regex_search(line, match, std::regex(R"(@NeuralComposition\s+(\w+))"))) {
                std::cout << "\033[1;35m[UI LOAD] 💠 Composition Root: " << match[1] << "\033[0m\n";
            }
        } 
        else {
            // ==========================================
            // 🧠 .xh FILE PARSING (LOGIC & HARDWARE)
            // ==========================================

            // 1. Global Variables
            if (std::regex_search(line, match, std::regex(R"(global\s+(\w+)\s*=\s*\"([^\"]+)\")"))) {
                runtime.write_global(match[1], match[2]);
            }

            // 2. Constants (#define)
            else if (std::regex_search(line, match, std::regex(R"(#define\s+(\w+)\s+\"([^\"]+)\")"))) {
                runtime.write(match[1], match[2], "CONST_DEF", true);
            }

            // 3. Standard Vars (atom/shadow)
            else if (std::regex_search(line, match, std::regex(R"((atom|shadow)\s+(\w+)\s*=\s*\"([^\"]+)\")"))) {
                bool is_atom = (match[1] == "atom");
                runtime.write(match[2], match[3], "LIB_VAR", is_atom);
            }

            // 4. Hardware Hooks (~hook)
            else if (std::regex_search(line, match, std::regex(R"(~hook\s+(\w+)\s*->\s*\"?([\w\d_]+)\"?)"))) {
                std::string hook_type = match[1];
                std::string target = match[2];
                
                // --- NEW GPU/NPU HOOK TRIGGERS ---
                if (hook_type == "gpu_accelerate") runtime.init_vulkan_pipeline();
                if (hook_type == "neural_sync") runtime.npu_neural_sync(target);
                
                runtime.write(hook_type + "_SYS", target, "HOOK", true);
                std::cout << "  ↳ \033[1;30mIntercepted Kernel Hook:\033[0m " << hook_type << " -> " << target << "\n";
            }
        }
    }
}
