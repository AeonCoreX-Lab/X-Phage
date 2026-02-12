#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>
#include <unordered_map>
#include <future>
#include <chrono>
#include <thread>

/* * X-Phage Engine v3.0.1 [OMNI-GOD EDITION]
 * Features: Quantum Threading, Vortex Wipe, Synapse, Chronos Time-Warp, Ether Uplink.
 */

enum TokenType {
    PULSE, SHADOW, ATOM, BEAM, SCAN, LINK, MATRIX, MATH, 
    BYPASS, QUANTUM, VORTEX, SYNAPSE, CHRONOS, ETHER,
    IDENTIFIER, STRING, NUMBER, EQUAL, PLUS, MINUS, MULTIPLY, DIVIDE,
    L_BRACE, R_BRACE, L_BRACKET, R_BRACKET, COMMA, UNKNOWN
};

struct Token {
    TokenType type;
    std::string value;
};

struct MemoryCell {
    std::string data;
    std::string type; 
    bool constant = false; 
};

class XPhageRuntime {
private:
    std::unordered_map<std::string, MemoryCell> cell_map;
public:
    void write(std::string id, std::string val, std::string type, bool is_const) {
        if (cell_map.count(id) && cell_map[id].constant) {
            std::cerr << "\033[1;31m[OMNI PANIC] ⛔ Atomic Violation on " << id << "\033[0m\n";
            exit(1);
        }
        cell_map[id] = {val, type, is_const};
    }

    MemoryCell read(std::string id) {
        if (cell_map.find(id) != cell_map.end()) return cell_map[id];
        return {"NULL", "null", false};
    }

    // FEATURE: QUANTUM (Parallel)
    void launch_quantum_process(std::string task_name) {
        auto future = std::async(std::launch::async, [task_name]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(10)); 
            return "DONE";
        });
        std::cout << "\033[1;36m[QUANTUM] ⚛ Spawning Parallel Thread: " << task_name << "\033[0m\n";
    }

    // FEATURE: VORTEX (Memory Wipe)
    void activate_vortex() {
        std::cout << "\033[1;35m[VORTEX] 🌪 Purging Shadow Memory...\033[0m\n";
        for (auto it = cell_map.begin(); it != cell_map.end(); ) {
            if (!it->second.constant) it = cell_map.erase(it);
            else ++it;
        }
        std::cout << "\033[1;32m[VORTEX] ✔ RAM Cleaned. Trace Removed.\033[0m\n";
    }

    // FEATURE: CHRONOS (Time Control)
    void activate_chronos(std::string ms_str) {
        int ms = std::stoi(ms_str);
        std::cout << "\033[1;33m[CHRONOS] ⏳ Time-Warping for " << ms << "ms...\033[0m\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(ms));
    }

    // FEATURE: ETHER (Network Uplink)
    void activate_ether(std::string target, std::string data_ref) {
        std::string data = read(data_ref).data;
        if(data == "NULL") data = data_ref; // If raw string
        std::cout << "\033[1;94m[ETHER] ☁ Uplinking to " << target << " >>> Packet: [" << data << "]\033[0m\n";
    }

    void hardware_bypass(std::string target) {
        std::cout << "\033[1;31m[BYPASS] ⚡ Kernel Injection: " << target << "\033[0m\n";
    }

    void perform_math(std::string target, std::string op, std::string val1, std::string val2) {
        double a = std::stod(read(val1).data != "NULL" ? read(val1).data : val1);
        double b = std::stod(read(val2).data != "NULL" ? read(val2).data : val2);
        double res = 0;
        if (op == "+") res = a + b;
        else if (op == "-") res = a - b;
        else if (op == "*") res = a * b;
        else if (op == "/") res = a / b;
        write(target, std::to_string(res), "int", false);
    }
};

class XPhageLexer {
public:
    std::vector<Token> tokenize(const std::string& source) {
        std::vector<Token> tokens;
        size_t i = 0;
        while (i < source.length()) {
            char c = source[i];
            if (isspace(c)) { i++; continue; }
            if (c == '/' && source[i+1] == '/') { while(i < source.length() && source[i] != '\n') i++; continue; }
            
            // Syntax
            if (c == '{') { tokens.push_back({L_BRACE, "{"}); i++; continue; }
            if (c == '}') { tokens.push_back({R_BRACE, "}"}); i++; continue; }
            if (c == '[') { tokens.push_back({L_BRACKET, "["}); i++; continue; }
            if (c == ']') { tokens.push_back({R_BRACKET, "]"}); i++; continue; }
            if (c == '=') { tokens.push_back({EQUAL, "="}); i++; continue; }
            if (c == '+') { tokens.push_back({PLUS, "+"}); i++; continue; }
            if (c == '-') { tokens.push_back({MINUS, "-"}); i++; continue; }
            if (c == '*') { tokens.push_back({MULTIPLY, "*"}); i++; continue; }
            if (c == '/') { tokens.push_back({DIVIDE, "/"}); i++; continue; }
            
            if (c == '"') {
                std::string str; i++;
                while (i < source.length() && source[i] != '"') { str += source[i]; i++; }
                tokens.push_back({STRING, str}); i++; continue;
            }

            if (isalnum(c) || c == '_' || c == '.' || c == '~') {
                std::string word;
                while (i < source.length() && (isalnum(source[i]) || source[i] == '_' || source[i] == '.' || source[i] == '~')) {
                    word += source[i]; i++;
                }
                if (word == "pulse") tokens.push_back({PULSE, word});
                else if (word == "shadow") tokens.push_back({SHADOW, word});
                else if (word == "atom") tokens.push_back({ATOM, word});
                else if (word == "beam") tokens.push_back({BEAM, word});
                else if (word == "quantum") tokens.push_back({QUANTUM, word});
                else if (word == "vortex") tokens.push_back({VORTEX, word});
                else if (word == "synapse") tokens.push_back({SYNAPSE, word});
                else if (word == "bypass") tokens.push_back({BYPASS, word});
                else if (word == "chronos") tokens.push_back({CHRONOS, word});
                else if (word == "ether") tokens.push_back({ETHER, word});
                else if (word == "scan") tokens.push_back({SCAN, word});
                else if (word == "matrix") tokens.push_back({MATRIX, word});
                else if (word == "~link") tokens.push_back({LINK, word});
                else if (isdigit(word[0])) tokens.push_back({NUMBER, word});
                else tokens.push_back({IDENTIFIER, word});
                continue;
            }
            i++;
        }
        return tokens;
    }
};

int main(int argc, char* argv[]) {
    std::cout << "\033[1;35mX-PHAGE [OMNI-GOD ENGINE v5.0]\033[0m\n";
    if (argc < 2) return 1;
    std::ifstream file(argv[1]);
    std::stringstream buffer; buffer << file.rdbuf();
    
    XPhageLexer lexer;
    std::vector<Token> tokens = lexer.tokenize(buffer.str());
    XPhageRuntime runtime;

    for (size_t i = 0; i < tokens.size(); ++i) {
        if (tokens[i].type == BYPASS) { runtime.hardware_bypass(tokens[i+1].value); i++; }
        if (tokens[i].type == QUANTUM) { runtime.launch_quantum_process(tokens[i+1].value); i++; }
        if (tokens[i].type == VORTEX) { runtime.activate_vortex(); }
        if (tokens[i].type == CHRONOS) { runtime.activate_chronos(tokens[i+1].value); i++; }
        if (tokens[i].type == SYNAPSE) { std::cout << "\033[1;34m[SYNAPSE] 🧠 Neural Handshake: " << tokens[i+1].value << " connected.\033[0m\n"; }
        
        if (tokens[i].type == ETHER && i+2 < tokens.size()) { 
            // ether "Target" "Data"
            runtime.activate_ether(tokens[i+1].value, tokens[i+2].value); 
            i += 2; 
        }

        if ((tokens[i].type == SHADOW || tokens[i].type == ATOM) && i+3 < tokens.size()) {
            runtime.write(tokens[i+1].value, tokens[i+3].value, "str", tokens[i].type == ATOM);
        }
        
        if (tokens[i].type == IDENTIFIER && i+4 < tokens.size() && tokens[i+1].type == EQUAL) {
            runtime.perform_math(tokens[i].value, tokens[i+3].value, tokens[i+2].value, tokens[i+4].value);
            i += 4;
        }
        if (tokens[i].type == BEAM) {
            auto cell = runtime.read(tokens[i+1].value);
            std::cout << "\033[1;32mOUT>\033[0m " << (cell.data == "NULL" ? tokens[i+1].value : cell.data) << "\n";
        }
    }
    return 0;
}
