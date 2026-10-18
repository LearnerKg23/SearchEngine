#pragma once
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <algorithm>
#include <cstdint>

struct TrieSuggestion {
    std::string term;
    uint32_t frequency;

    TrieSuggestion() : frequency(0) {}
    TrieSuggestion(std::string t, uint32_t f) : term(std::move(t)), frequency(f) {}

    bool operator>(const TrieSuggestion& other) const {
        return frequency > other.frequency;
    }
};

class TrieNode {
public:
    std::unordered_map<char, std::unique_ptr<TrieNode>> children;
    bool is_end_of_word = false;
    uint32_t frequency = 0;

    // Pre-cached top-K suggestions passing through this prefix node
    std::vector<TrieSuggestion> top_suggestions;

    void update_suggestion(const std::string& term, uint32_t freq, size_t max_cache = 5) {
        bool found = false;
        for (auto& s : top_suggestions) {
            if (s.term == term) {
                s.frequency = freq;
                found = true;
                break;
            }
        }
        if (!found) {
            top_suggestions.emplace_back(term, freq);
        }

        std::sort(top_suggestions.begin(), top_suggestions.end(), std::greater<TrieSuggestion>());

        if (top_suggestions.size() > max_cache) {
            top_suggestions.resize(max_cache);
        }
    }
};

class AutocompleteTrie {
public:
    explicit AutocompleteTrie(size_t max_cache_per_node = 5);

    // Inserts a term and its frequency, propagating the top suggestions down the character path
    void insert(const std::string& term, uint32_t frequency);

    // Instant O(prefix.length()) lookup using pre-cached node suggestions
    std::vector<std::string> get_completions(const std::string& prefix, size_t limit = 5) const;

    size_t size() const { return total_terms_; }
    void clear();

private:
    size_t max_cache_;
    size_t total_terms_ = 0;
    std::unique_ptr<TrieNode> root_;
};
