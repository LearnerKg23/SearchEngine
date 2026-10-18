# Milestone 5: Real-Time Autocomplete (In-Memory Prefix Tree / Trie)

> **Resume Alignment**: *"Engineered real-time autocomplete using an in-memory Trie (Prefix Tree)... to prioritize relevant code definitions and documents."*

---

## 🎯 Objectives
1. Implement a high-performance **In-Memory Trie (Prefix Tree)** for real-time keystroke suggestions.
2. Store term frequencies and **pre-cache Top-K completions at each node** to guarantee $O(L)$ retrieval latency (where $L$ is prefix length), completely eliminating expensive subtree traversals at query time.
3. Ingest unique vocabulary terms and code definitions (functions, classes, structs) into the Trie.

---

## 1. Trie Architecture & Pre-Cached Top-K Mechanics

A naive Trie requires a Depth-First Search (DFS) over the entire subtree at query time to find and sort completions. Under interactive keystrokes, this is too slow.

Instead, we pre-cache the top 5 to 10 most frequent completions directly inside each prefix node during index insertion:

```
                  (root)
                 /      \
               's'      'c'
               /          \
             'e'          'p'
             /              \
           'a'              'p'
         [ "search" (freq: 120), "seccomp" (freq: 15) ]
```

When the user types `"sea"`, the engine traverses 3 pointers (`root -> 's' -> 'e' -> 'a'`) and instantly returns the pre-cached vector in **sub-microsecond** time.

---

## 2. Data Structure Design (`include/autocomplete/trie.hpp`)

```cpp
#pragma once
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <algorithm>

struct Suggestion {
    std::string term;
    uint32_t frequency;

    bool operator>(const Suggestion& other) const {
        return frequency > other.frequency;
    }
};

class TrieNode {
public:
    std::unordered_map<char, std::unique_ptr<TrieNode>> children;
    bool is_end_of_word = false;
    uint32_t frequency = 0;

    // Pre-cached top-K suggestions passing through this node
    std::vector<Suggestion> top_suggestions;

    void update_suggestion(const std::string& term, uint32_t freq, size_t max_cache = 5) {
        // Update existing or add new
        bool found = false;
        for (auto& s : top_suggestions) {
            if (s.term == term) {
                s.frequency = freq;
                found = true;
                break;
            }
        }
        if (!found) {
            top_suggestions.push_back({term, freq});
        }

        // Sort descending by frequency
        std::sort(top_suggestions.begin(), top_suggestions.end(), std::greater<Suggestion>());

        // Bound to max_cache
        if (top_suggestions.size() > max_cache) {
            top_suggestions.resize(max_cache);
        }
    }
};

class AutocompleteTrie {
public:
    AutocompleteTrie(size_t max_cache_per_node = 5)
        : max_cache_(max_cache_per_node), root_(std::make_unique<TrieNode>()) {}

    // Inserts or updates a term with its corpus frequency
    void insert(const std::string& term, uint32_t frequency);

    // Instant O(prefix.length()) prefix search
    std::vector<std::string> get_completions(const std::string& prefix, size_t limit = 5) const;

    // Total unique terms in Trie
    size_t size() const { return total_terms_; }

private:
    size_t max_cache_;
    size_t total_terms_ = 0;
    std::unique_ptr<TrieNode> root_;
};
```

---

## 3. Implementation Details (`src/trie.cpp`)

### A. Insertion with Top-K Path Propagation
When inserting a term, we update every node along the character path:
```cpp
void AutocompleteTrie::insert(const std::string& term, uint32_t frequency) {
    if (term.empty()) return;

    TrieNode* curr = root_.get();
    for (char c : term) {
        if (curr->children.find(c) == curr->children.end()) {
            curr->children[c] = std::make_unique<TrieNode>();
        }
        curr = curr->children[c].get();
        // Propagate top suggestions down the path
        curr->update_suggestion(term, frequency, max_cache_);
    }

    if (!curr->is_end_of_word) {
        curr->is_end_of_word = true;
        total_terms_++;
    }
    curr->frequency = frequency;
}
```

### B. Instant Prefix Lookup
```cpp
std::vector<std::string> AutocompleteTrie::get_completions(const std::string& prefix, size_t limit) const {
    const TrieNode* curr = root_.get();
    for (char c : prefix) {
        auto it = curr->children.find(c);
        if (it == curr->children.end()) {
            return {}; // No completions match this prefix
        }
        curr = it->second.get();
    }

    std::vector<std::string> results;
    for (size_t i = 0; i < std::min(limit, curr->top_suggestions.size()); ++i) {
        results.push_back(curr->top_suggestions[i].term);
    }
    return results;
}
```

---

## 4. Integration with Index & Code Definitions
* At the conclusion of `IndexWriter::commit()`, iterate over the dictionary.
* Insert every vocabulary term and its document frequency into `AutocompleteTrie`.
* Terms tagged as code definitions (e.g., function names, class definitions) receive an artificially boosted frequency so they bubble to the top of user suggestions.

---

## 5. Verification & Unit Tests (`tests/test_trie.cpp`)
* **Test 1: Prefix Suggestions**
  - Insert: `("apple", 10)`, `("application", 50)`, `("app", 5)`.
  - Query prefix `"app"`: Expected order is `["application", "apple", "app"]` (highest frequency first).
* **Test 2: Non-Existent Prefix**
  - Query `"xyz"`: Returns empty vector without error.
* **Test 3: Prefix Substring Bounds**
  - Query single character `"a"`: Returns bounded top-5 completions.
* **Test 4: Microsecond Latency Benchmark**
  - Insert 50,000 unique terms into the Trie.
  - Benchmark 1,000 prefix lookups with `<chrono>`. Assert average lookup latency is **< 10 microseconds**.

---

## ✅ Milestone 5 Checklist
- [ ] `TrieNode` with cached `top_suggestions` vector implemented.
- [ ] `AutocompleteTrie::insert()` updates path cache.
- [ ] Prefix queries execute in $O(L)$ time with zero recursion.
- [ ] All unit and performance tests pass.
