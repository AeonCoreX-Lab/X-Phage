#include "../../include/xphage/runtime.hpp"
#include <iostream>

/**
 * X-Phage Memory Management (Dual-Layer)
 * Handles Local Shadow RAM and Global Registry
 */

void XPhageRuntime::write(std::string id, std::string val, std::string type, bool is_const) {
    if (cell_map.count(id) && cell_map[id].constant) {
        std::cerr << "\033[1;31m[OMNI PANIC] ⛔ Atomic Violation: '" << id << "' is immutable.\033[0m\n";
        return; 
    }
    cell_map[id] = {val, type, is_const, false};
    // Debug log omitted for clean output
}

void XPhageRuntime::write_global(std::string id, std::string val) {
    global_registry[id] = {val, "GLOBAL", false, false};
    std::cout << "\033[1;35m[GLOBAL] 🌍 Universal Registry Updated: " << id << " -> " << val << "\033[0m\n";
}

MemoryCell XPhageRuntime::read(std::string id) const {
    // Priority 1: Check Local Memory
    if (cell_map.count(id)) {
        return cell_map[id];
    }
    // Priority 2: Check Global Registry
    if (global_registry.count(id)) {
        return global_registry[id];
    }
    
    // Priority 3: Check if it's a raw string literal passing through
    return {id, "raw_ref", false, false};
}
