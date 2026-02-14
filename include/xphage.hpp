#ifndef XPHAGE_H
#define XPHAGE_H

#include <string>
#include <vector>
#include <unordered_map>
#include <iostream>
#include <memory>

// Token Types (Expanded for Complex UI)
enum TokenType {
    // Core
    PULSE, SHADOW, ATOM, BEAM, SCAN, LINK, MATRIX, MATH,
    BYPASS, QUANTUM, VORTEX, SYNAPSE, CHRONOS, ETHER, VOID, GLOBAL,
    
    // Fusion UI System (Advanced)
    FUSION, SIGNAL, VISION, ORBIT, TRIGGER, INPUT, Z_PLANE,
    
    // Syntax
    IDENTIFIER, STRING, NUMBER, EQUAL, PLUS, MINUS, MULTIPLY, DIVIDE,
    L_BRACE, R_BRACE, L_BRACKET, R_BRACKET, LPAREN, RPAREN, COLON, COMMA, UNKNOWN
};

struct Token {
    TokenType type;
    std::string value;
};

struct MemoryCell {
    std::string data;
    std::string type; 
    bool constant = false; 
    bool is_neural = false; 
};

// Fusion UI Node (Virtual DOM)
struct FusionNode {
    std::string type;               // Vortex, Signal, etc.
    std::string id;                 // Unique ID
    std::unordered_map<std::string, std::string> props; // Style & Data
    std::vector<std::shared_ptr<FusionNode>> children; // Recursive Nesting
};

class XPhageRuntime {
public:
    std::unordered_map<std::string, MemoryCell> cell_map;
    std::unordered_map<std::string, MemoryCell> global_registry;
    
    // UI State
    std::shared_ptr<FusionNode> ui_root;
    std::shared_ptr<FusionNode> current_ui_context;
    bool ui_active = false;

public:
    // Memory Ops
    void write(std::string id, std::string val, std::string type, bool is_const);
    void write_global(std::string id, std::string val);
    MemoryCell read(std::string id);
    
    // Core Ops
    void launch_quantum_process(std::string task_name);
    void hardware_bypass(std::string target, std::string params);
    void activate_vortex();
    void activate_void_protocol();
    void start_torrent_engine(std::string magnet_link);
    void establish_synapse(std::string id, std::string target_api);
    void process_matrix(std::string id, std::string size);
    void activate_chronos(std::string ms_str);
    void activate_ether(std::string target, std::string data_ref);

    // Ultimate Fusion Engine (Jetpack Killer)
    void init_fusion_engine();
    void begin_ui_component(std::string type, std::string params);
    void end_ui_component(); // Close parsing scope
    void render_ui_tree();   // Draw the Virtual DOM
};

class XPhageLexer {
public:
    std::vector<Token> tokenize(const std::string& source);
};

class XPhageLinker {
public:
    void link_library(std::string lib_name, XPhageRuntime& runtime);
};

#endif
