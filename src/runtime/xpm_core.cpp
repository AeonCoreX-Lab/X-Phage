#include "../../include/xphage.hpp"
#include <iostream>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <vector>
#include <algorithm>

// Apple Platform Detection
#ifdef __APPLE__
    #include <TargetConditionals.h>
#endif

const std::string RAW_URL = "https://raw.githubusercontent.com/AeonCoreX-Lab/X-Phage/main/";

namespace XPM_Cloud {

// Helper: create directory (cross‑platform)
static bool create_directory(const std::string& path) {
    #ifdef _WIN32
        std::string cmd = "if not exist \"" + path + "\" mkdir \"" + path + "\"";
    #else
        std::string cmd = "mkdir -p \"" + path + "\"";
    #endif
    return std::system(cmd.c_str()) == 0;
}

// Helper: download a single file
static bool download_file(const std::string& url, const std::string& dest) {
    std::string cmd = "curl -sL \"" + url + "\" -o \"" + dest + "\"";
    return std::system(cmd.c_str()) == 0;
}

// ------------------------------------------------------------------
// 1. Sync a single module (supports subdirectories)
// ------------------------------------------------------------------
bool sync_module(std::string module_name) {
    std::cout << "\033[1;34m[XPM] 🌐 Cloud Sync Initiated for: " << module_name << "...\033[0m\n";

    #if defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE
        std::cerr << "\033[1;31m[iOS RESTRICTION] ⛔ Cloud Sync & Shell commands are disabled on iOS.\033[0m\n";
        return false;
    #else
        // Normalise path: replace backslashes with forward slashes
        std::replace(module_name.begin(), module_name.end(), '\\', '/');
        
        // Determine target directory and file name
        std::string target_dir = "modules";
        size_t last_slash = module_name.find_last_of('/');
        if (last_slash != std::string::npos) {
            std::string subdir = module_name.substr(0, last_slash);
            target_dir += "/" + subdir;
            module_name = module_name.substr(last_slash + 1);
        }
        
        // Ensure the directory exists
        if (!create_directory(target_dir)) {
            std::cerr << "[XPM] Failed to create directory: " << target_dir << "\n";
            return false;
        }
        
        // Target file path
        std::string target_file = target_dir + "/" + module_name + ".xh";
        
        // Download
        std::string url = RAW_URL + "modules/" + module_name + "/" + module_name + ".xh";
        if (!download_file(url, target_file)) {
            std::cerr << "[XPM] Failed to download: " << url << "\n";
            return false;
        }
        
        std::cout << "\033[1;32m[XPM] ✅ Success: '" << module_name << "' synced to " << target_file << "\033[0m\n";
        return true;
    #endif
}

// ------------------------------------------------------------------
// 2. Update the entire standard library (stdlib)
// ------------------------------------------------------------------
bool update_stdlib() {
    std::cout << "\033[1;35m[XPM] 🧬 Initiating Titan Stdlib Pulse Update...\033[0m\n";

    #if defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE
        std::cerr << "\033[1;31m[iOS RESTRICTION] ⛔ Standard library update is disabled on iOS.\033[0m\n";
        return false;
    #else
        // List of all standard library files (relative to stdlib/)
        std::vector<std::string> libs = {
            "core/types.xh",
            "core/system.xh",
            "math/basic.xh",
            "math/linalg.xh",
            "io/file.xh",
            "io/console.xh",
            "net/http.xh",
            "net/socket.xh",
            "data/json.xh",
            "data/string.xh",
            "media/engine.xh",
            "media/stream.xh",
            "security/crypt.xh",
            "ui/fusion.xh"
        };
        
        bool all_ok = true;
        for (const auto& lib : libs) {
            std::string dest = "stdlib/" + lib;
            
            // Extract directory path (e.g., stdlib/net)
            size_t last_slash = dest.find_last_of('/');
            if (last_slash != std::string::npos) {
                std::string dir = dest.substr(0, last_slash);
                create_directory(dir);  // ensure subfolder exists
            }
            
            std::string url = RAW_URL + "stdlib/" + lib;
            if (!download_file(url, dest)) {
                std::cerr << "\033[1;31m  ✖ Failed to download: " << lib << "\033[0m\n";
                all_ok = false;
            } else {
                std::cout << "  \033[1;32m✔ Updated:\033[0m " << lib << "\n";
            }
        }
        
        if (all_ok) {
            std::cout << "\033[1;32m[XPM] ✅ All systems operational. Stdlib is now v4.0 Alpha.\033[0m\n";
        }
        return all_ok;
    #endif
}

} // namespace XPM_Cloud