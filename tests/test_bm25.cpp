#include "query/bm25_ranker.hpp"
#include "index/index_writer.hpp"
#include "index/index_reader.hpp"
#include "tokenizer/tokenizer.hpp"
#include <iostream>
#include <cassert>
#include <vector>
#include <algorithm>
#include <random>

#define TEST_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            std::cerr << "\033[1;31m[FAIL] " << msg << " (" << __FILE__ << ":" << __LINE__ << ")\033[0m\n"; \
            return false; \
        } \
    } while (0)

bool test_bm25_length_penalty() {
    std::string test_dir = "bin/.test_bm25_len";
    Tokenizer tokenizer;

    // Doc 0: Short document (5 words, contains "vector" twice)
    auto t0 = tokenizer.tokenize("vector vector dynamic array memory");
    // Doc 1: Long document (contains "vector" twice, but 50 other words)
    std::string long_doc = "vector vector ";
    for (int i = 0; i < 50; ++i) long_doc += "word ";
    auto t1 = tokenizer.tokenize(long_doc);

    {
        IndexWriter writer(test_dir);
        writer.add_document(0, "short.cpp", 1000, t0);
        writer.add_document(1, "long.cpp", 2000, t1);
        writer.commit();
    }

    IndexReader reader(test_dir);
    TEST_ASSERT(reader.open(), "Reader failed to open");
    BM25Ranker ranker(reader);

    float idf = ranker.compute_idf("vector");
    float score_short = ranker.score_term(2, reader.get_document_length(0), idf);
    float score_long = ranker.score_term(2, reader.get_document_length(1), idf);

    TEST_ASSERT(score_short > score_long, "Shorter document with same TF must score higher than long document");

    std::cout << "\033[1;32m[PASS]\033[0m test_bm25_length_penalty (Short: " 
              << score_short << " > Long: " << score_long << ")\n";
    return true;
}

bool test_bm25_term_saturation() {
    std::string test_dir = "bin/.test_bm25_sat";
    Tokenizer tokenizer;
    auto t0 = tokenizer.tokenize("sample document");

    {
        IndexWriter writer(test_dir);
        writer.add_document(0, "dummy.txt", 1000, t0);
        writer.commit();
    }

    IndexReader reader(test_dir);
    reader.open();
    BM25Ranker ranker(reader);

    float idf = 1.0f;
    uint32_t doc_len = static_cast<uint32_t>(reader.average_doc_length());

    float s1 = ranker.score_term(1, doc_len, idf);
    float s2 = ranker.score_term(2, doc_len, idf);
    float s50 = ranker.score_term(50, doc_len, idf);
    float s51 = ranker.score_term(51, doc_len, idf);

    float delta_low = s2 - s1;
    float delta_high = s51 - s50;

    TEST_ASSERT(delta_low > delta_high * 5.0f, "BM25 score gain must saturate at high TF");

    std::cout << "\033[1;32m[PASS]\033[0m test_bm25_term_saturation (Delta low: " 
              << delta_low << " >> Delta high: " << delta_high << ")\n";
    return true;
}

bool test_min_heap_top_k_accuracy() {
    // Generate 10,000 scored documents with random scores
    std::mt19937 rng(42);
    std::uniform_real_distribution<float> dist(0.1f, 100.0f);

    std::vector<ScoredDoc> candidates;
    candidates.reserve(10000);
    for (uint32_t i = 0; i < 10000; ++i) {
        candidates.emplace_back(i, dist(rng));
    }

    size_t K = 10;
    auto top_k = BM25Ranker::select_top_k(candidates, K);
    TEST_ASSERT(top_k.size() == K, "Top-K size must be exactly K");

    // Compare against full std::sort
    std::vector<ScoredDoc> sorted_all = candidates;
    std::sort(sorted_all.begin(), sorted_all.end(), [](const ScoredDoc& a, const ScoredDoc& b) {
        return a.score > b.score;
    });

    for (size_t i = 0; i < K; ++i) {
        TEST_ASSERT(top_k[i].doc_id == sorted_all[i].doc_id, "Top-K order must match exact full sort");
        TEST_ASSERT(std::abs(top_k[i].score - sorted_all[i].score) < 1e-4f, "Top-K scores must match exact full sort");
    }

    std::cout << "\033[1;32m[PASS]\033[0m test_min_heap_top_k_accuracy across 10,000 documents\n";
    return true;
}

int main() {
    std::cout << "\n=== Running Milestone 4: BM25 Ranking & Min-Heap Top-K Tests ===\n";
    bool success = true;
    success &= test_bm25_length_penalty();
    success &= test_bm25_term_saturation();
    success &= test_min_heap_top_k_accuracy();

    if (success) {
        std::cout << "\n\033[1;32m>>> ALL MILESTONE 4 TESTS PASSED! <<<\033[0m\n\n";
        return 0;
    } else {
        std::cerr << "\n\033[1;31m>>> MILESTONE 4 TESTS FAILED! <<<\033[0m\n\n";
        return 1;
    }
}
