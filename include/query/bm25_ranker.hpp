#pragma once
#include "common/types.hpp"
#include "index/index_reader.hpp"
#include <vector>
#include <string>
#include <queue>
#include <cmath>

class BM25Ranker {
public:
    explicit BM25Ranker(IndexReader& reader, float k1 = 1.2f, float b = 0.75f);

    // Computes Inverse Document Frequency (IDF) for a term
    float compute_idf(const std::string& term) const;

    // Computes single-term BM25 score for a document
    float score_term(uint32_t tf, uint32_t doc_length, float idf) const;

    // Scores a set of postings across query terms and extracts top-K using O(N log K) Min-Heap
    std::vector<ScoredDoc> rank_top_k(const std::vector<std::string>& query_terms, 
                                      const std::vector<Posting>& candidate_postings, 
                                      size_t k = 10) const;

    // Pure Min-Heap Top-K algorithm on pre-scored candidates
    static std::vector<ScoredDoc> select_top_k(const std::vector<ScoredDoc>& candidates, size_t k);

    float k1() const { return k1_; }
    float b() const { return b_; }

private:
    IndexReader& reader_;
    float k1_;
    float b_;
    double avgdl_;
    uint32_t total_docs_;
};
