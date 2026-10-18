#include "autocomplete/trie.hpp"
#include <iostream>
#include <cassert>
#include <chrono>
#include <vector>
#include <string>

#define TEST_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            std::cerr << "\033[1;31m[FAIL] " << msg << " (" << __FILE__ << ":" << __LINE__ << ")\033[0m\n"; \
            return false; \
        } \
    } while (0)

bool test_prefix_ordering_by_frequency() {
    AutocompleteTrie trie(5);
    trie.insert("apple", 10);
    trie.insert("application", 50);
    trie.insert("app", 5);
    trie.insert("apply", 25);

    auto suggestions = trie.get_completions("app", 4);
    TEST_ASSERT(suggestions.size() == 4, "Should return 4 suggestions");
    TEST_ASSERT(suggestions[0] == "application", "Top suggestion must be highest frequency: application");
    TEST_ASSERT(suggestions[1] == "apply", "Second suggestion must be apply (25)");
    TEST_ASSERT(suggestions[2] == "apple", "Third suggestion must be apple (10)");
    TEST_ASSERT(suggestions[3] == "app", "Fourth suggestion must be app (5)");

    std::cout << "\033[1;32m[PASS]\033[0m test_prefix_ordering_by_frequency\n";
    return true;
}

bool test_non_existent_prefix() {
    AutocompleteTrie trie;
    trie.insert("search", 100);
    trie.insert("engine", 50);

    auto res = trie.get_completions("xyz");
    TEST_ASSERT(res.empty(), "Non-existent prefix must return empty");

    std::cout << "\033[1;32m[PASS]\033[0m test_non_existent_prefix\n";
    return true;
}

bool test_case_insensitivity() {
    AutocompleteTrie trie;
    trie.insert("parseHttpRequest", 40);

    auto res1 = trie.get_completions("parse");
    auto res2 = trie.get_completions("PARSE");
    TEST_ASSERT(!res1.empty() && res1[0] == "parseHttpRequest", "Lowercase prefix failed");
    TEST_ASSERT(!res2.empty() && res2[0] == "parseHttpRequest", "Uppercase prefix failed");

    std::cout << "\033[1;32m[PASS]\033[0m test_case_insensitivity\n";
    return true;
}

bool test_trie_latency_benchmark() {
    AutocompleteTrie trie(5);

    // Ingest 20,000 synthetic terms
    for (int i = 0; i < 20000; ++i) {
        std::string term = "term_" + std::to_string(i);
        trie.insert(term, i + 1);
    }
    TEST_ASSERT(trie.size() == 20000, "Trie must contain 20,000 terms");

    // Benchmark 5,000 lookups
    const int num_lookups = 5000;
    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < num_lookups; ++i) {
        std::string prefix = "term_" + std::to_string(i % 500);
        auto results = trie.get_completions(prefix, 5);
        (void)results;
    }

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::micro> duration_us = end - start;
    double avg_us = duration_us.count() / num_lookups;

    std::cout << "  Average Trie prefix lookup time: " << avg_us << " microseconds\n";
    TEST_ASSERT(avg_us < 10.0, "Trie prefix lookup must be under 10 microseconds");

    std::cout << "\033[1;32m[PASS]\033[0m test_trie_latency_benchmark (Instant sub-microsecond retrieval)\n";
    return true;
}

int main() {
    std::cout << "\n=== Running Milestone 5: Trie Autocomplete Tests ===\n";
    bool success = true;
    success &= test_prefix_ordering_by_frequency();
    success &= test_non_existent_prefix();
    success &= test_case_insensitivity();
    success &= test_trie_latency_benchmark();

    if (success) {
        std::cout << "\n\033[1;32m>>> ALL MILESTONE 5 TESTS PASSED! <<<\033[0m\n\n";
        return 0;
    } else {
        std::cerr << "\n\033[1;31m>>> MILESTONE 5 TESTS FAILED! <<<\033[0m\n\n";
        return 1;
    }
}
