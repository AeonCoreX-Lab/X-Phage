#include "../../include/xphage.hpp"
#include <iostream>
#include <cstdlib>

// Apple Platform Detection
#ifdef __APPLE__
    #include <TargetConditionals.h>
#endif

const std::string RAW_URL = "https://raw.githubusercontent.com/AeonCoreX-Lab/X-Phage/main/";

namespace XPM_Cloud {

bool sync_module(std::string module_name) {
    std::cout << "\033[1;34m[XPM] 🌐 Cloud Sync Initiated for: " << module_name << "...\033[0m\n";
    
    // ---------------------------------------------------------
    // 🚫 iOS SECURITY CHECK
    // iOS does not allow 'system()' calls or shell subprocesses.
    // ---------------------------------------------------------
    #if defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE
        std::cerr << "\033[1;31m[iOS RESTRICTION] ⛔ Cloud Sync & Shell commands are disabled on iOS.\033[0m\n";
        return false;
    #else
        // Desktop/Android Logic
        
        // Create directory path based on target OS
        #ifdef _WIN32
            std::string dir_cmd = "if not exist \"modules\\" + module_name + "\" mkdir \"modules\\" + module_name + "\"";
        #else
            std::string dir_cmd = "mkdir -p modules/" + module_name;
        #endif
        
        // Fix for Android cross-compiler warning: Catching the result and casting to void
        int dir_res = std::system(dir_cmd.c_str());
        (void)dir_res;

        // Fetch header from AeonCoreX-Lab using native curl
        std::string target_file = "modules/" + module_name + "/" + module_name + ".xh";
        std::string cmd = "curl -sL " + RAW_URL + target_file + " -o " + target_file;
        
        int result = std::system(cmd.c_str());
        
        if (result == 0) {
            std::cout << "\033[1;32m[XPM] ✅ Success: '" << module_name << "' synced from AeonCoreX-Lab.\033[0m\n";
            return true;
        } else {
            std::cout << "\033[1;31m[XPM] ❌ Failed to sync: '" << module_name << "'\033[0m\n";
            return false;
        }
    #endif
}

}
