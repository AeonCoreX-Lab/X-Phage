#include "../include/symbol_table.hpp"
#include <algorithm>
#include <vector>

namespace xphage::sema {

size_t SymbolTable::edit_distance(const std::string& a, const std::string& b) {
    const size_t m = a.size(), n = b.size();
    std::vector<std::vector<size_t>> dp(m + 1, std::vector<size_t>(n + 1));
    for (size_t i = 0; i <= m; i++) dp[i][0] = i;
    for (size_t j = 0; j <= n; j++) dp[0][j] = j;
    for (size_t i = 1; i <= m; i++) {
        for (size_t j = 1; j <= n; j++) {
            if (a[i-1] == b[j-1]) {
                dp[i][j] = dp[i-1][j-1];
            } else {
                dp[i][j] = 1 + std::min({dp[i-1][j], dp[i][j-1], dp[i-1][j-1]});
            }
        }
    }
    return dp[m][n];
}

std::optional<std::string> SymbolTable::suggest_similar(const std::string& target, SymbolKind kind) {
    // Threshold: a suggestion is only offered if it's "close
    // enough" to plausibly be a typo rather than an unrelated name.
    // A flat distance of <= 2 (or <= 30% of the target's length,
    // whichever is larger) catches the common cases — one
    // transposed/missing/extra/wrong letter, or a doubled letter —
    // without flooding the user with implausible guesses for short
    // names.
    std::string best;
    size_t best_dist = SIZE_MAX;
    size_t threshold = std::max<size_t>(2, target.size() / 3);

    for (Scope* s = current(); s != nullptr; s = s->parent()) {
        for (auto& [name, sym] : s->symbols()) {
            if (sym.kind != kind) continue;
            if (name == target) continue; // exact match isn't "similar", it's found
            size_t d = edit_distance(target, name);
            if (d <= threshold && d < best_dist) {
                best_dist = d;
                best = name;
            }
        }
    }

    if (best.empty()) return std::nullopt;
    return best;
}

} // namespace xphage::sema
