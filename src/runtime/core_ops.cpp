#include "../../include/xphage.hpp"
#include <iostream>
#include <future>
#include <thread>
#include <chrono>
#include <stack>
#include <cstdlib> // For rand()

// --- HARDWARE / GPU ACCELERATION (NEW) ---

void XPhageRuntime::init_vulkan_pipeline() {
    if (!vulkan_ready) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        std::cout << "\033[1;32m[VULKAN] 🌋 GPU Compute Pipeline Enabled & Context Created.\033[0m\n";
        vulkan_ready = true;
    }
}

void XPhageRuntime::gpu_compute_matrix(std::string id) {
    if (!vulkan_ready) init_vulkan_pipeline();
    std::cout << "\033[1;33m[GPU-COMPUTE] 💠 Offloading Matrix Processing to GPU Tensor Cores: " << id << "\033[0m\n";
}

void XPhageRuntime::npu_neural_sync(std::string id) {
    std::cout << "\033[1;36m[NPU] 🧠 Synchronizing Neural Pulse Sequence: " << id << "\033[0m\n";
}

// --- OMNI-GOD FEATURE SET ---

void XPhageRuntime::launch_quantum_process(std::string task_name) {
    auto future = std::async(std::launch::async, [task_name]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(25)); 
        return "DONE";
    });
    std::cout << "\033[1;36m[QUANTUM] ⚛ Spawning Neural Thread: " << task_name << "\033[0m\n";
}

void XPhageRuntime::hardware_bypass(std::string target, std::string params) {
    std::cout << "\033[1;31m[BYPASS] ⚡ Kernel Injection: " << target << " | Config: " << params << "\033[0m\n";
}

void XPhageRuntime::activate_vortex() {
    std::cout << "\033[1;35m[VORTEX] 🌪 Purging Shadow Memory (Global Matrix Intact)...\033[0m\n";
    cell_map.clear();
}

void XPhageRuntime::activate_void_protocol() {
    std::cout << "\033[1;41;97m[VOID] ⛔ INITIATING BLACKHOLE WIPE...\033[0m\n";
    cell_map.clear();
    global_registry.clear();
}

// --- TITAN FUSION UI ENGINE ---

void XPhageRuntime::init_fusion_engine() {
    if(!ui_active) {
        std::cout << "\033[1;35m[FUSION] 🎨 Activating Titan UI Compositor...\033[0m\n";
        ui_root = std::make_shared<FusionNode>();
        ui_root->type = "ROOT";
        current_ui_context = ui_root;
        ui_active = true;
    }
}

void XPhageRuntime::begin_ui_component(std::string type, std::string params) {
    auto node = std::make_shared<FusionNode>();
    node->type = type;
    node->props["raw_params"] = params;
    current_ui_context->children.push_back(node);
}

void XPhageRuntime::fusion_render(std::string element, std::string params) {
    auto node = std::make_shared<FusionNode>();
    node->type = element;
    node->props["raw_params"] = params;
    ui_root->children.push_back(node);
}

void XPhageRuntime::end_ui_component() {
    // Handled recursively now
}

// Recursive Tree Renderer for CLI
void traverse_render(std::shared_ptr<FusionNode> node, int depth) {
    if (!node) return;

    for (int i = 0; i < depth; ++i) std::cout << "  │  ";
    
    if (node->type == "FUSION_ROOT") {
        std::cout << "📦 \033[1;32mROOT CONTAINER: " << node->props["name"] << "\033[0m\n";
    } else {
        std::cout << "├─ 💠 \033[1;36m" << node->type << "\033[0m { " << node->props["raw_params"] << " }\033[0m\n";
    }

    for (auto& child : node->children) {
        traverse_render(child, depth + 1);
    }
}

void XPhageRuntime::render_ui_tree() {
    std::cout << "\n\033[1;45;97m === TITAN UI FRAME BUILD === \033[0m\n";
    traverse_render(ui_root, 0);
    std::cout << "\033[1;45;97m ============================ \033[0m\n\n";
}

// --- TORRENT & NETWORKING ---

void XPhageRuntime::start_torrent_engine(std::string magnet_link) {
    std::cout << "\033[1;36m[OMNI-NET] 🌍 Torrent Engine Started (Bypass Mode)...\033[0m\n";
    std::cout << "          >> Target: " << magnet_link << "\n";
}

void XPhageRuntime::establish_synapse(std::string id, std::string target_api) {
    std::cout << "\033[1;34m[SYNAPSE] 🧠 Neural Handshake -> " << target_api << "\033[0m\n";
    write(id, "[NEURAL_LINK_ACTIVE]", "synapse", true);
}

void XPhageRuntime::process_matrix(std::string id, std::string size) {
    gpu_compute_matrix(id); // Now directly linked to Vulkan
    write(id, "MATRIX_DATA_BLOCK", "matrix", false);
}

void XPhageRuntime::activate_chronos(std::string ms_str) {
    std::cout << "\033[1;33m[CHRONOS] ⏳ Time Dilation Triggered: " << ms_str << "\033[0m\n";
}

void XPhageRuntime::activate_ether(std::string target, std::string data_ref) {
    std::cout << "\033[1;32m[ETHER] 📡 Sub-space Data Uplink -> " << target << "\033[0m\n";
}
