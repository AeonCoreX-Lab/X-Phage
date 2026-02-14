#include "../include/xphage.hpp"
#include <cctype>

/**
 * X-Phage Lexer Module v3.1
 * Supports Recursive UI Structures & Property Maps
 */

std::vector<Token> XPhageLexer::tokenize(const std::string& source) {
    std::vector<Token> tokens;
    size_t i = 0;
    while (i < source.length()) {
        char c = source[i];

        if (isspace(c)) { i++; continue; }
        if (c == '/' && source[i+1] == '/') { 
            while(i < source.length() && source[i] != '\n') i++; 
            continue; 
        }
        
        // Structural Tokens
        if (c == '{') { tokens.push_back({L_BRACE, "{"}); i++; continue; }
        if (c == '}') { tokens.push_back({R_BRACE, "}"}); i++; continue; }
        if (c == '[') { tokens.push_back({L_BRACKET, "["}); i++; continue; }
        if (c == ']') { tokens.push_back({R_BRACKET, "]"}); i++; continue; }
        if (c == '(') { tokens.push_back({LPAREN, "("}); i++; continue; }
        if (c == ')') { tokens.push_back({RPAREN, ")"}); i++; continue; }
        if (c == ':') { tokens.push_back({COLON, ":"}); i++; continue; }
        if (c == ',') { tokens.push_back({COMMA, ","}); i++; continue; }
        if (c == '=') { tokens.push_back({EQUAL, "="}); i++; continue; }

        if (c == '"') {
            std::string str; i++;
            while (i < source.length() && source[i] != '"') { 
                str += source[i]; i++; 
            }
            tokens.push_back({STRING, str}); i++; continue;
        }

        if (isalnum(c) || c == '_' || c == '.' || c == '~' || c == '@' || c == '#') {
            std::string word;
            while (i < source.length() && (isalnum(source[i]) || source[i] == '_' || source[i] == '.' || source[i] == '~' || source[i] == '@' || source[i] == '#')) {
                word += source[i]; i++;
            }

            if (word == "pulse") tokens.push_back({PULSE, word});
            else if (word == "global") tokens.push_back({GLOBAL, word});
            else if (word == "shadow") tokens.push_back({SHADOW, word});
            else if (word == "atom") tokens.push_back({ATOM, word});
            else if (word == "beam") tokens.push_back({BEAM, word});
            else if (word == "quantum") tokens.push_back({QUANTUM, word});
            else if (word == "vortex") tokens.push_back({VORTEX, word}); // Context dependent
            else if (word == "void") tokens.push_back({VOID, word});
            else if (word == "synapse") tokens.push_back({SYNAPSE, word});
            else if (word == "bypass") tokens.push_back({BYPASS, word});
            else if (word == "chronos") tokens.push_back({CHRONOS, word});
            else if (word == "ether") tokens.push_back({ETHER, word});
            else if (word == "scan") tokens.push_back({SCAN, word});
            else if (word == "matrix") tokens.push_back({MATRIX, word});
            else if (word == "~link") tokens.push_back({LINK, word});
            
            // Advanced UI Tokens
            else if (word == "fusion" || word == "@NeuralComposition") tokens.push_back({FUSION, word});
            else if (word == "Signal") tokens.push_back({SIGNAL, word});
            else if (word == "Vision") tokens.push_back({VISION, word});
            else if (word == "Orbit") tokens.push_back({ORBIT, word});
            else if (word == "Trigger") tokens.push_back({TRIGGER, word});
            else if (word == "Z_Plane") tokens.push_back({Z_PLANE, word});
            else if (word == "Input") tokens.push_back({INPUT, word});
            
            else if (isdigit(word[0])) tokens.push_back({NUMBER, word}); 
            else tokens.push_back({IDENTIFIER, word});
            continue;
        }
        i++;
    }
    return tokens;
}
