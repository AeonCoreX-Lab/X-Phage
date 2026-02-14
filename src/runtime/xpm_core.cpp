#include "../../include/xphage.hpp"
#include <iostream>
#include <cstdlib>

/**
 * X-Phage Package Manager (XPM) Cloud Sync
 * Automatically fetches missing modules from AeonCoreX-Lab
 */

const std::string RAW_URL = "https://raw.githubusercontent.com/AeonCoreX-Lab/X-Phage/main/";

bool XPM_Cloud::sync_module(std::string module_name) {
    std::cout << "\033[1;34m[XPM] 🌐 Cloud Sync Initiated for: " << module_name << "...\033[0m\n";
    
    // Create directory path based on target OS
    #ifdef _WIN32
        std::string dir_cmd = "if not exist \"modules\\" + module_name + "\" mkdir \"modules\\" + module_name + "\"";
    #else
        std::string dir_cmd = "mkdir -p modules/" + module_name;
    #endif
    
    std::system(dir_cmd.c_str());

    // Fetch header from AeonCoreX-Lab using native curl
    std::string target_file = "modules/" + module_name + "/" + module_name + ".xh";
    std::string cmd = "curl -sL " + RAW_URL + target_file + " -o " + target_file;
    
    int result = std::system(cmd.c_str());
    
    if (result == 0) {
        std::cout << "\033[1;32m[XPM] ✅ Success: '" << module_name << "' synced from AeonCoreX-Lab.\033[0m\n";
        return true;
    } else {
        std::cerr << "\033[1;31m[XPM] ❌ Error: Failed to fetch from Global Registry. Check connection.\033[0m\n";
        return false;
    }
}
