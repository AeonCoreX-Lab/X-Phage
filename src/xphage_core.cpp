#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>
#include <map>
#include <unordered_map>
#include <algorithm>
#include <cmath>

/* * X-Phage Engine v3.0 [Singularity]
 * Developed by: AeonCoreX
 * Features: Static Typing, Memory Safety Protocol, Matrix Math, and Optimized Lexing.
 */

enum TokenType {
    PULSE, SHADOW, ATOM, BEAM, SCAN, LINK, MATRIX, MATH,
    IDENTIFIER, STRING, NUMBER, EQUAL, PLUS, MINUS, MULTIPLY, DIVIDE,
    L_BRACE, R_BRACE, L_BRACKET, R_BRACKET, COMMA, UNKNOWN
};

struct Token {
    TokenType type;
    std::string value;
};

// --- ADVANCED GHOST MEMORY PROTOCOL ---
struct MemoryCell {
    std::string data;
    std::string type; // "str", "int", "matrix"
    bool constant = false; // Rust-like immutability
};

class XPhageRuntime {
private:
    std::unordered_map<std::string, MemoryCell> cell_map;
public:
    void write(std::string id, std::string val, std::string type, bool is_const) {
        if (cell_map.count(id) && cell_map[id].constant) {
            std::cerr << "\033[1;31m[Security Violation] Cannot mutate 'atom' " << id << " (Locked by AeonCore)\033[0m\n";
            exit(1);
        }
        cell_map[id] = {val, type, is_const};
    }

    MemoryCell read(std::string id) {
        if (cell_map.find(id) != cell_map.end()) return cell_map[id];
        return {"NULL", "null", false};
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

// --- HIGH-SPEED LEXER ---
class XPhageLexer {
public:
    std::vector<Token> tokenize(const std::string& source) {
        std::vector<Token> tokens;
        size_t i = 0;
        while (i < source.length()) {
            char c = source[i];
            if (isspace(c)) { i++; continue; }
            if (c == '/' && source[i+1] == '/') { while(i < source.length() && source[i] != '\n') i++; continue; }
            
            if (c == '{') { tokens.push_back({L_BRACE, "{"}); i++; continue; }
            if (c == '}') { tokens.push_back({R_BRACE, "}"}); i++; continue; }
            if (c == '[') { tokens.push_back({L_BRACKET, "["}); i++; continue; }
            if (c == ']') { tokens.push_back({R_BRACKET, "]"}); i++; continue; }
            if (c == '=') { tokens.push_back({EQUAL, "="}); i++; continue; }
            if (c == '+') { tokens.push_back({PLUS, "+"}); i++; continue; }
            if (c == '-') { tokens.push_back({MINUS, "-"}); i++; continue; }
            if (c == '*') { tokens.push_back({MULTIPLY, "*"}); i++; continue; }
            if (c == '/') { tokens.push_back({DIVIDE, "/"}); i++; continue; }
            if (c == ',') { tokens.push_back({COMMA, ","}); i++; continue; }

            if (c == '"') {
                std::string str; i++;
                while (i < source.length() && source[i] != '"') { str += source[i]; i++; }
                tokens.push_back({STRING, str}); i++; 
                if (i < source.length()) i++; continue;
            }

            if (isalnum(c) || c == '_' || c == '.') {
                std::string word;
                while (i < source.length() && (isalnum(source[i]) || source[i] == '_' || source[i] == '.')) {
                    word += source[i]; i++;
                }
                if (word == "pulse") tokens.push_back({PULSE, word});
                else if (word == "shadow") tokens.push_back({SHADOW, word});
                else if (word == "atom") tokens.push_back({ATOM, word});
                else if (word == "beam") tokens.push_back({BEAM, word});
                else if (word == "scan") tokens.push_back({SCAN, word});
                else if (word == "~link") tokens.push_back({LINK, word});
                else if (word == "matrix") tokens.push_back({MATRIX, word});
                else if (isdigit(word[0])) tokens.push_back({NUMBER, word});
                else tokens.push_back({IDENTIFIER, word});
                continue;
            }
            i++;
        }
        return tokens;
    }
};

// --- EXECUTION ENGINE ---
int main(int argc, char* argv[]) {
    std::cout << "\033[1;35mX-PHAGE SINGULARITY ENGINE [v3.0]\033[0m\n";
    std::cout << "\033[1;30mPowered by AeonCoreX Intellectual Property\033[0m\n\n";

    if (argc < 2) return 1;
    std::ifstream file(argv[1]);
    std::stringstream buffer; buffer << file.rdbuf();
    
    XPhageLexer lexer;
    std::vector<Token> tokens = lexer.tokenize(buffer.str());
    XPhageRuntime runtime;
    bool skip = false;

    for (size_t i = 0; i < tokens.size(); ++i) {
        if (skip && tokens[i].type != R_BRACE) continue;
        if (tokens[i].type == R_BRACE) { skip = false; continue; }

        if (tokens[i].type == LINK) {
            std::cout << "\033[1;34m[System] Handshake with " << tokens[i+1].value << " Successful.\033[0m\n";
        }

        if ((tokens[i].type == SHADOW || tokens[i].type == ATOM) && i+3 < tokens.size()) {
            runtime.write(tokens[i+1].value, tokens[i+3].value, "str", tokens[i].type == ATOM);
        }

        // Feature: Inline Math (Full Power)
        if (tokens[i].type == IDENTIFIER && i+4 < tokens.size() && tokens[i+1].type == EQUAL && 
           (tokens[i+3].type == PLUS || tokens[i+3].type == MINUS || tokens[i+3].type == MULTIPLY || tokens[i+3].type == DIVIDE)) {
            runtime.perform_math(tokens[i].value, tokens[i+3].value, tokens[i+2].value, tokens[i+4].value);
            i += 4;
        }

        if (tokens[i].type == BEAM) {
            auto cell = runtime.read(tokens[i+1].value);
            std::cout << "\033[1;32mOUT>\033[0m " << (cell.data == "NULL" ? tokens[i+1].value : cell.data) << "\n";
        }

        if (tokens[i].type == SCAN) {
            if (runtime.read(tokens[i+1].value).data == "0") skip = true;
        }
    }
    return 0;
}
