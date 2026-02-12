#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>
#include <map>

/* * X-Phage Engine v1.3
 * Feature: Global Library Loader & Splash Screen
 */

enum TokenType {
    PULSE, SHADOW, ATOM, BEAM, SCAN, BYPASS, LINK, IDENTIFIER, STRING, EQUAL, L_BRACE, R_BRACE, UNKNOWN
};

struct Token {
    TokenType type;
    std::string value;
};

class XPhageLexer {
public:
    std::vector<Token> tokenize(const std::string& source) {
        std::vector<Token> tokens;
        size_t i = 0;
        while (i < source.length()) {
            if (isspace(source[i])) { i++; continue; }
            if (source[i] == '{') { tokens.push_back({L_BRACE, "{"}); i++; continue; }
            if (source[i] == '}') { tokens.push_back({R_BRACE, "}"}); i++; continue; }
            if (source[i] == '=') { tokens.push_back({EQUAL, "="}); i++; continue; }
            if (source[i] == '"') {
                std::string str; i++;
                while (i < source.length() && source[i] != '"') { str += source[i]; i++; }
                tokens.push_back({STRING, str}); i++; continue;
            }
            std::string word;
            while (i < source.length() && !isspace(source[i]) && 
                   source[i] != '{' && source[i] != '}' && source[i] != '=' && source[i] != '"') {
                word += source[i]; i++;
            }
            if (word == "pulse") tokens.push_back({PULSE, word});
            else if (word == "shadow") tokens.push_back({SHADOW, word});
            else if (word == "atom") tokens.push_back({ATOM, word});
            else if (word == "beam") tokens.push_back({BEAM, word});
            else if (word == "scan") tokens.push_back({SCAN, word});
            else if (word == "~link") tokens.push_back({LINK, word});
            else if (!word.empty()) tokens.push_back({IDENTIFIER, word});
        }
        return tokens;
    }
};

int main(int argc, char* argv[]) {
    // 🎨 Global Identity Splash Screen
    std::cout << "\033[1;32m"; 
    std::cout << "  __  __      _____  _                             \n";
    std::cout << "  \\ \\/ /     |  __ \\| |                            \n";
    std::cout << "   \\  /______| |__) | |__   __ _  __ _  ___        \n";
    std::cout << "   /  \\______|  ___/| '_ \\ / _` |/ _` |/ _ \\       \n";
    std::cout << "  / /\\ \\     | |    | | | | (_| | (_| |  __/       \n";
    std::cout << " /_/  \\_\\    |_|    |_| |_|\\__,_|\\__, |\\___|       \n";
    std::cout << "                                  __/ |            \n";
    std::cout << "                                 |___/  v1.3       \n";
    std::cout << "\033[0m" << ">> Global Identity System Active...\n\n";

    if (argc < 2) {
        std::cerr << "Usage: xphage <file.xp0>" << std::endl;
        return 1;
    }

    std::ifstream file(argv[1]);
    if (!file.is_open()) return 1;
    std::stringstream buffer; buffer << file.rdbuf();
    
    XPhageLexer lexer;
    std::vector<Token> tokens = lexer.tokenize(buffer.str());
    std::map<std::string, std::string> ghost_memory;
    bool skip_block = false;

    for (size_t i = 0; i < tokens.size(); ++i) {
        if (skip_block && tokens[i].type != R_BRACE) continue;
        if (tokens[i].type == R_BRACE) { skip_block = false; continue; }

        // 🔗 Library Linking Logic
        if (tokens[i].type == LINK && i + 1 < tokens.size()) {
            std::string libName = tokens[i+1].value;
            std::cout << "\033[1;34m[System] Linking Library: " << libName << "...\033[0m\n";
            // Logic to simulate library connection
            continue;
        }

        if ((tokens[i].type == SHADOW || tokens[i].type == ATOM) && i+3 < tokens.size()) {
            ghost_memory[tokens[i+1].value] = tokens[i+3].value;
        }

        if (tokens[i].type == SCAN && i+1 < tokens.size()) {
            if (ghost_memory[tokens[i+1].value] == "" || ghost_memory[tokens[i+1].value] == "0") skip_block = true;
        }

        if (tokens[i].type == BEAM && i+1 < tokens.size()) {
            std::string out = (tokens[i+1].type == IDENTIFIER) ? ghost_memory[tokens[i+1].value] : tokens[i+1].value;
            std::cout << ">> " << out << std::endl;
        }
    }
    return 0;
}
