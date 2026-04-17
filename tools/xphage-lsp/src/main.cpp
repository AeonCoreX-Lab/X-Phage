// ============================================================
// xphage-lsp — Language Server Protocol v3.5.0
// JSON-RPC 2.0 over stdio — VS Code / Neovim / Zed compatible
//
// Implements:
//   initialize, initialized, shutdown, exit
//   textDocument/didOpen, didChange, didClose
//   textDocument/completion
//   textDocument/hover
//   textDocument/publishDiagnostics
// ============================================================
#include "xphage/runtime.hpp"
#include "../../../compiler/xphage_parse/include/parser.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <algorithm>
#include <cstdlib>

// ── Minimal JSON helpers ─────────────────────────────────────
// (no external deps — we hand-roll the tiny subset we need)

static std::string json_str(const std::string& s) {
    std::string r = "\"";
    for (char c : s) {
        if (c == '"')  r += "\\\"";
        else if (c == '\\') r += "\\\\";
        else if (c == '\n') r += "\\n";
        else if (c == '\r') r += "\\r";
        else if (c == '\t') r += "\\t";
        else r += c;
    }
    return r + "\"";
}

static std::string json_field(const std::string& key, const std::string& val) {
    return json_str(key) + ": " + val;
}

static std::string extract_field(const std::string& json,
                                  const std::string& key) {
    std::string search = "\"" + key + "\"";
    size_t pos = json.find(search);
    if (pos == std::string::npos) return "";
    pos = json.find(':', pos);
    if (pos == std::string::npos) return "";
    pos++;
    while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t')) pos++;
    if (pos >= json.size()) return "";
    if (json[pos] == '"') {
        size_t end = json.find('"', pos + 1);
        if (end == std::string::npos) return "";
        return json.substr(pos + 1, end - pos - 1);
    }
    size_t end = json.find_first_of(",}\n", pos);
    return json.substr(pos, end - pos);
}

// ── LSP transport ────────────────────────────────────────────
static void send_message(const std::string& body) {
    std::string msg = "Content-Length: " + std::to_string(body.size())
                    + "\r\n\r\n" + body;
    std::cout << msg << std::flush;
}

static std::string recv_message() {
    std::string header;
    int content_length = 0;

    while (true) {
        std::string line;
        std::getline(std::cin, line);
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty()) break;
        if (line.rfind("Content-Length:", 0) == 0) {
            content_length = std::stoi(line.substr(16));
        }
    }

    std::string body(content_length, '\0');
    std::cin.read(&body[0], content_length);
    return body;
}

// ── Completion items ─────────────────────────────────────────
static const char* KEYWORDS[] = {
    "pulse","shadow","atom","beam","global","quantum","vortex","void",
    "synapse","bypass","chronos","ether","scan","matrix","~link",
    "fusion","Signal","Vision","Orbit","Trigger","Input","Z_Plane",
    nullptr
};

static const char* STDLIB_MODULES[] = {
    "core/types","core/system",
    "io/file","io/console",
    "net/http","net/socket",
    "data/json","data/string",
    "math/basic","math/linalg",
    "media/engine","media/stream",
    "security/crypt","ui/fusion",
    "neural/bci","neural/lsl",
    nullptr
};

static std::string make_completion_list(const std::string& prefix) {
    std::string items = "[";
    bool first = true;

    auto add = [&](const std::string& label, const std::string& kind,
                   const std::string& detail) {
        if (!prefix.empty() &&
            label.rfind(prefix, 0) != 0) return;
        if (!first) items += ",";
        first = false;
        items += "{";
        items += json_field("label", json_str(label)) + ",";
        items += json_field("kind", kind) + ",";
        items += json_field("detail", json_str(detail));
        items += "}";
    };

    for (int i = 0; KEYWORDS[i]; i++)
        add(KEYWORDS[i], "14", "X-Phage keyword");

    for (int i = 0; STDLIB_MODULES[i]; i++)
        add(std::string(STDLIB_MODULES[i]), "9",
            "stdlib module (~link " + std::string(STDLIB_MODULES[i]) + ")");

    items += "]";
    return items;
}

// ── Diagnostics from parser ──────────────────────────────────
static std::string make_diagnostics(const std::string& uri,
                                     const std::string& src) {
    XPhageLexer lexer;
    auto tokens = lexer.tokenize(src);
    xphage::parse::Parser parser(tokens);
    parser.parse();

    std::string diags = "[";
    bool first = true;
    for (auto& e : parser.errors()) {
        if (!first) diags += ",";
        first = false;
        uint32_t ln = e.line > 0 ? e.line - 1 : 0;
        diags += "{";
        diags += json_field("range",
            "{\"start\":{\"line\":" + std::to_string(ln) + ",\"character\":0},"
            " \"end\":{\"line\":" + std::to_string(ln) + ",\"character\":100}}") + ",";
        diags += json_field("severity", "1") + ",";
        diags += json_field("source", json_str("xphage-lsp")) + ",";
        diags += json_field("message", json_str(e.message));
        diags += "}";
    }
    diags += "]";
    return diags;
}

// ── Hover info ───────────────────────────────────────────────
static std::string hover_for(const std::string& word) {
    static std::unordered_map<std::string, std::string> docs = {
        {"pulse",   "**pulse** — declares a function/block"},
        {"shadow",  "**shadow** — mutable variable (heap)"},
        {"atom",    "**atom** — immutable variable"},
        {"beam",    "**beam** — print output to stdout"},
        {"bypass",  "**bypass** — kernel/hardware injection call"},
        {"quantum", "**quantum** — spawn asynchronous thread"},
        {"vortex",  "**vortex** — purge local Shadow RAM"},
        {"void",    "**void** — full memory wipe (VOID Protocol)"},
        {"chronos", "**chronos** — sleep / time dilation (ms)"},
        {"ether",   "**ether** — cloud/network uplink"},
        {"synapse", "**synapse** — neural API handshake"},
        {"matrix",  "**matrix** — GPU matrix allocation"},
        {"~link",   "**~link** — import stdlib or module"},
        {"fusion",  "**fusion** — declare Titan UI composition"},
        {"global",  "**global** — global registry variable"},
        {"scan",    "**scan** — inspect / type-check a value"},
    };
    auto it = docs.find(word);
    if (it != docs.end())
        return "{\"contents\":{\"kind\":\"markdown\","
               + json_field("value", json_str(it->second)) + "}}";
    return "null";
}

// ── Main LSP loop ─────────────────────────────────────────────
int main() {
    std::unordered_map<std::string, std::string> open_docs;
    bool running = true;
    int  req_id  = 0;

    while (running && std::cin.good()) {
        std::string msg = recv_message();
        if (msg.empty()) break;

        std::string method = extract_field(msg, "method");
        std::string id_str = extract_field(msg, "id");

        auto respond = [&](const std::string& result) {
            std::string body = "{";
            body += json_field("jsonrpc", json_str("2.0")) + ",";
            body += json_field("id", id_str.empty() ? "null" : id_str) + ",";
            body += json_field("result", result);
            body += "}";
            send_message(body);
        };

        auto notify = [&](const std::string& meth, const std::string& params) {
            std::string body = "{";
            body += json_field("jsonrpc", json_str("2.0")) + ",";
            body += json_field("method", json_str(meth)) + ",";
            body += json_field("params", params);
            body += "}";
            send_message(body);
        };

        // ── initialize ────────────────────────────────────────
        if (method == "initialize") {
            respond("{"
                "\"capabilities\":{"
                "  \"textDocumentSync\":1,"
                "  \"completionProvider\":{\"triggerCharacters\":[\"/\",\"~\"]},"
                "  \"hoverProvider\":true"
                "},"
                "\"serverInfo\":{"
                "  \"name\":\"xphage-lsp\","
                "  \"version\":\"3.5.0\""
                "}}");
        }
        // ── initialized ───────────────────────────────────────
        else if (method == "initialized") {
            // no-op
        }
        // ── shutdown ──────────────────────────────────────────
        else if (method == "shutdown") {
            respond("null");
        }
        // ── exit ──────────────────────────────────────────────
        else if (method == "exit") {
            running = false;
        }
        // ── textDocument/didOpen ──────────────────────────────
        else if (method == "textDocument/didOpen") {
            std::string uri = extract_field(msg, "uri");
            std::string text = extract_field(msg, "text");
            open_docs[uri] = text;
            std::string diags = make_diagnostics(uri, text);
            notify("textDocument/publishDiagnostics",
                "{" + json_field("uri", json_str(uri)) + ","
                    + json_field("diagnostics", diags) + "}");
        }
        // ── textDocument/didChange ────────────────────────────
        else if (method == "textDocument/didChange") {
            std::string uri  = extract_field(msg, "uri");
            std::string text = extract_field(msg, "text");
            open_docs[uri] = text;
            std::string diags = make_diagnostics(uri, text);
            notify("textDocument/publishDiagnostics",
                "{" + json_field("uri", json_str(uri)) + ","
                    + json_field("diagnostics", diags) + "}");
        }
        // ── textDocument/didClose ─────────────────────────────
        else if (method == "textDocument/didClose") {
            std::string uri = extract_field(msg, "uri");
            open_docs.erase(uri);
        }
        // ── textDocument/completion ───────────────────────────
        else if (method == "textDocument/completion") {
            // TODO: extract word at cursor for prefix filtering
            std::string items = make_completion_list("");
            respond("{\"isIncomplete\":false,\"items\":" + items + "}");
        }
        // ── textDocument/hover ────────────────────────────────
        else if (method == "textDocument/hover") {
            // Simple: extract word from position
            std::string uri = extract_field(msg, "uri");
            // For now return null — position-to-word lookup requires
            // full line tracking which needs the full doc + line/col
            std::string result = "null";
            if (open_docs.count(uri)) {
                // Very simple: return hover for common keywords
                // A real impl would extract word at cursor position
            }
            respond(result);
        }
        // ── Unknown method ────────────────────────────────────
        else if (!id_str.empty()) {
            // Return method-not-found error
            std::string body = "{";
            body += json_field("jsonrpc", json_str("2.0")) + ",";
            body += json_field("id", id_str) + ",";
            body += "\"error\":{\"code\":-32601,"
                    + json_field("message", json_str("Method not found: " + method))
                    + "}";
            body += "}";
            send_message(body);
        }
    }

    return 0;
}
