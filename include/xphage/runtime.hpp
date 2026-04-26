#pragma once
// ============================================================
// X-Phage Runtime v3.5.0
// ============================================================
#include "ast.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

// ============================================================
// Memory cell (Shadow RAM + Global Registry)
// ============================================================
struct MemoryCell {
    std::string data;
    std::string type;
    bool        constant  = false;
    bool        is_neural = false;
};

// ============================================================
// Runtime engine
// ============================================================
class XPhageRuntime {
public:
    std::unordered_map<std::string, MemoryCell> cell_map;
    std::unordered_map<std::string, MemoryCell> global_registry;
    std::shared_ptr<FusionNode> ui_root;
    std::shared_ptr<FusionNode> current_ui_context;
    bool ui_active    = false;
    bool vulkan_ready = false;

    // Memory
    void write(std::string id, std::string val,
               std::string type = "string", bool is_const = false);
    void write_global(std::string id, std::string val);
    MemoryCell read(std::string id) const;

    // Core ops
    void launch_quantum_process(std::string task_name);
    void hardware_bypass(std::string target, std::string params);
    void activate_vortex();
    void activate_void_protocol();
    void start_torrent_engine(std::string magnet_link);
    void establish_synapse(std::string id, std::string target_api);
    void process_matrix(std::string id, std::string size);
    void activate_chronos(std::string ms_str);
    void activate_ether(std::string target, std::string data_ref);

    // GPU / NPU
    void init_vulkan_pipeline();
    void gpu_compute_matrix(std::string matrix_id);
    void npu_neural_sync(std::string pulse_id);

    // Fusion UI
    void init_fusion_engine();
    void begin_ui_component(std::string type, std::string params);
    void fusion_render(std::string element, std::string params);
    void end_ui_component();
    void render_ui_tree();
};

// ============================================================
// Forward declarations for compiler classes
// ============================================================
class XPhageLexer {
public:
    std::vector<Token> tokenize(const std::string& source,
                                const std::string& filename = "<input>");
};

class XPhageLinker {
public:
    void link_library(std::string lib_name, XPhageRuntime& runtime);
};

class XPhageLLVMCompiler {
public:
    void compile_tokens(const std::vector<Token>& tokens,
                        std::string output_obj);
};

class XPhageTranspiler {
public:
    void transpile_to_cpp(const std::vector<Token>& tokens,
                          std::string output_cpp);
};
