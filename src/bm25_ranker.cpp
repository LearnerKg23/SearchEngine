#include "query/bm25_ranker.hpp"
#include <algorithm>

BM25Ranker::BM25Ranker(IndexReader& reader, float k1, float b)
    : reader_(reader), k1_(k1), b_(b) {
    avgdl_ = reader_.average_doc_length();
    total_docs_ = reader_.total_docs();
}

float BM25Ranker::compute_idf(const std::string& term) const {
    uint32_t df = reader_.get_doc_frequency(term);
    if (df == 0 || total_docs_ == 0) return 0.0f;

    float n = static_cast<float>(df);
    float N = static_cast<float>(total_docs_);
    return std::log((N - n + 0.5f) / (n + 0.5f) + 1.0f);
}

float BM25Ranker::score_term(uint32_t tf, uint32_t doc_length, float idf) const {
    if (tf == 0 || avgdl_ <= 0.0) return 0.0f;

    float len_norm = 1.0f - b_ + b_ * (static_cast<float>(doc_length) / static_cast<float>(avgdl_));
    float num = tf * (k1_ + 1.0f);
    float denom = tf + k1_ * len_norm;

    return idf * (num / denom);
}

std::vector<ScoredDoc> BM25Ranker::select_top_k(const std::vector<ScoredDoc>& candidates, size_t k) {
    if (candidates.empty() || k == 0) return {};

    // Min-Heap bounded to size K: root is always the lowest score among the current top-K
    std::priority_queue<ScoredDoc, std::vector<ScoredDoc>, std::greater<ScoredDoc>> min_heap;

    for (const auto& doc : candidates) {
        if (min_heap.size() < k) {
            min_heap.push(doc);
        } else if (doc.score > min_heap.top().score) {
            min_heap.pop();
            min_heap.push(doc);
        }
    }

    // Extract elements from min-heap (comes out in ascending order)
    std::vector<ScoredDoc> top_results;
    top_results.reserve(min_heap.size());
    while (!min_heap.empty()) {
        top_results.push_back(min_heap.top());
        min_heap.pop();
    }

    // Reverse to return descending order (highest score first)
    std::reverse(top_results.begin(), top_results.end());
    return top_results;
}

std::vector<ScoredDoc> BM25Ranker::rank_top_k(const std::vector<std::string>& query_terms, 
                                              const std::vector<Posting>& candidate_postings, 
                                              size_t k) const {
    if (candidate_postings.empty() || k == 0) return {};

    // Precalculate IDFs for query terms
    float total_idf = 0.0f;
    for (const auto& term : query_terms) {
        total_idf += compute_idf(term);
    }
    if (total_idf <= 0.0f) total_idf = 1.0f;

    std::vector<ScoredDoc> scored_docs;
    scored_docs.reserve(candidate_postings.size());

    for (const auto& p : candidate_postings) {
        uint32_t doc_len = reader_.get_document_length(p.doc_id);
        float score = score_term(p.term_frequency, doc_len, total_idf);
        scored_docs.emplace_back(p.doc_id, score);
    }

    return select_top_k(scored_docs, k);
}
