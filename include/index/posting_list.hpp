#pragma once
#include "common/types.hpp"
#include <vector>
#include <algorithm>

namespace index_util {

// Sorts postings by doc_id to maintain the sorted invariant required for two-pointer algorithms
inline void sort_postings(std::vector<Posting>& postings) {
    std::sort(postings.begin(), postings.end(), [](const Posting& a, const Posting& b) {
        return a.doc_id < b.doc_id;
    });
}

} // namespace index_util
