#pragma once
#include "common/types.hpp"
#include <vector>

class PhraseMatcher {
public:
    // Finds documents where list_b occurs exactly one position after list_a (pos_b == pos_a + 1)
    static std::vector<Posting> match_pair(const std::vector<Posting>& list_a, 
                                           const std::vector<Posting>& list_b);

    // Matches an arbitrary N-word phrase sequence across sequential postings lists
    static std::vector<Posting> match_sequence(const std::vector<std::vector<Posting>>& term_postings);
};
