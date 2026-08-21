#pragma once
// ============================================================
// X-Phage Section Detector v4.1.0
// Single-file .xp → Virtual Tri-Modular split
//   Logic Layer    → .xh  (forge, nexus, const, signature-only pulse)
//   UI Layer       → .xui (fusion, strand)
//   Execution Layer→ .xp0 (bodies, side-effects, entry point)
// AeonCoreX Lab
// ============================================================
#include "xphage/ast.hpp"
#include <string>
#include <vector>
#include <memory>

namespace xphage::interface {

enum class Section { Logic, UI, Execution };

struct SectionedNode {
    ASTNodePtr node;
    Section    section;
};

class SectionDetector {
public:
    static Section classify(const ASTNode& node);
    static std::vector<SectionedNode> classify_all(const Program& prog);

    struct SplitResult {
        Program logic, ui, execution;
    };
    static SplitResult split(const Program& prog);

    static std::string emit_xh(const Program& logic_nodes,  const std::string& orig_path = "");
    static std::string emit_xui(const Program& ui_nodes,    const std::string& orig_path = "");
    static std::string emit_xp0(const Program& exec_nodes,
                                 const std::vector<std::string>& logic_links,
                                 const std::string& orig_path = "");

    // Detects duplicate top-level forge/nexus declarations (genuine
    // redefinition errors in the generated C++). Pulses are excluded
    // unless BOTH occurrences have a body (signature + body pairing
    // across .xh/.xp0 is the normal, expected workflow). Realm is
    // excluded since C++ namespaces support reopening across files.
    // Returns one human-readable error string per duplicate found
    // (empty vector = no duplicates).
    static std::vector<std::string> check_duplicates(const Program& merged);

private:
    static bool is_pure_literal(const ASTNodePtr& expr);
};

} // namespace xphage::interface
