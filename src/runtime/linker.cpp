#include "../../include/xphage.hpp"
#include <fstream>
#include <iostream>
#include <regex>

/**
 * 🔗 X-Phage Intelligent Linker v3.1
 * Architecture: Logic (.xh) & UI (.xui) Separation
 * Features: Global Registry, Hardware Hooks, Cloud Sync
 */

// Forward declaration for XPM Cloud (Assuming it's linked via build.sh)
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
        }
    }

    // FATAL ERROR
    if (!file.is_open()) {
        std::cerr << "\033[1;31m[LINKER FATAL] ❌ Target '" << lib_name << "' is missing or corrupted.\033[0m\n";
        return;
    }

    // --- 2. MODE DETECTION (.xh vs .xui) ---
    bool is_ui_mode = (lib_name.find(".xui") != std::string::npos);
    
    if (is_ui_mode) {
        std::cout << "\033[1;35m[LINKER] 🎨 Loading Fusion UI Layout: " << lib_name << "...\033[0m\n";
    } else {
        std::cout << "\033[1;36m[LINKER] 🧠 Injecting Logic Core: " << lib_name << "...\033[0m\n";
    }

    // --- 3. PARSING LOGIC ---
    std::string line;
    while (std::getline(file, line)) {
        std::smatch match;

        if (is_ui_mode) {
            // ==========================================
            // 🎨 .xui FILE PARSING (UI COMPONENTS)
            // ==========================================
            
            // Handle: Component(param: "value") or Component "Value"
            // Regex captures: 1=Component Name, 2=Params inside ()
            if (std::regex_search(line, match, std::regex(R"((Signal|Vision|Orbit|Trigger|Vortex)\s*\(?([^\)]*)\)?)"))) {
                std::string element = match[1];
                std::string params = match[2];
                
                // Clean params if empty
                if (params.empty() || params == " ") params = "Default_Layout";
                
                runtime.fusion_render(element, params);
            }
            
            // Handle: @NeuralComposition(Name)
            else if (std::regex_search(line, match, std::regex(R"(@NeuralComposition\(([^)]+)\))"))) {
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
                runtime.write("HOOK_" + match[1].str(), match[2], "SYS_HOOK", true);
                // Optional: Log hook injection
                // std::cout << "  └─ [HOOK] " << match[1] << " -> " << match[2] << "\n";
            }
        }
    }
    std::cout << "\033[1;32m[LINKER] ✔ Module Synced Successfully.\033[0m\n";
}
