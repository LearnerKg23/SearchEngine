#include "autocomplete/trie.hpp"
#include <cctype>

AutocompleteTrie::AutocompleteTrie(size_t max_cache_per_node)
    : max_cache_(max_cache_per_node), root_(std::make_unique<TrieNode>()) {}

void AutocompleteTrie::clear() {
    root_ = std::make_unique<TrieNode>();
    total_terms_ = 0;
}

void AutocompleteTrie::insert(const std::string& term, uint32_t frequency) {
    if (term.empty()) return;

    TrieNode* curr = root_.get();
    for (char c : term) {
        char lower = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        if (curr->children.find(lower) == curr->children.end()) {
            curr->children[lower] = std::make_unique<TrieNode>();
        }
        curr = curr->children[lower].get();
        curr->update_suggestion(term, frequency, max_cache_);
    }

    if (!curr->is_end_of_word) {
        curr->is_end_of_word = true;
        total_terms_++;
    }
    curr->frequency = frequency;
}

std::vector<std::string> AutocompleteTrie::get_completions(const std::string& prefix, size_t limit) const {
    if (prefix.empty()) return {};

    const TrieNode* curr = root_.get();
    for (char c : prefix) {
        char lower = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        auto it = curr->children.find(lower);
        if (it == curr->children.end()) {
            return {};
        }
        curr = it->second.get();
    }

    std::vector<std::string> results;
    size_t count = std::min(limit, curr->top_suggestions.size());
    results.reserve(count);

    for (size_t i = 0; i < count; ++i) {
        results.push_back(curr->top_suggestions[i].term);
    }

    return results;
}
