#pragma once
#include "common/types.hpp"
#include <vector>

class BooleanRetriever {
public:
    // O(N + M) two-pointer intersection: returns postings present in BOTH sorted lists
    static std::vector<Posting> intersect(const std::vector<Posting>& list_a, 
                                          const std::vector<Posting>& list_b);

    // O(N + M) two-pointer union: returns postings present in EITHER sorted list (deduplicated)
    static std::vector<Posting> merge_union(const std::vector<Posting>& list_a, 
                                            const std::vector<Posting>& list_b);

    // O(N + M) two-pointer difference: returns postings in list_a that are NOT in list_b
    static std::vector<Posting> difference(const std::vector<Posting>& list_a, 
                                           const std::vector<Posting>& list_b);
};
