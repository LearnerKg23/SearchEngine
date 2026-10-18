#include "query/phrase_matcher.hpp"

std::vector<Posting> PhraseMatcher::match_pair(const std::vector<Posting>& list_a, 
                                               const std::vector<Posting>& list_b) {
    std::vector<Posting> result;
    size_t i = 0, j = 0;

    while (i < list_a.size() && j < list_b.size()) {
        if (list_a[i].doc_id == list_b[j].doc_id) {
            const auto& a_pos = list_a[i].positions;
            const auto& b_pos = list_b[j].positions;

            // Two-pointer search for consecutive positions (pos_b == pos_a + 1)
            size_t pa = 0, pb = 0;
            std::vector<uint32_t> matched_positions;

            while (pa < a_pos.size() && pb < b_pos.size()) {
                if (a_pos[pa] + 1 == b_pos[pb]) {
                    matched_positions.push_back(b_pos[pb]);
                    ++pa;
                    ++pb;
                } else if (a_pos[pa] + 1 < b_pos[pb]) {
                    ++pa;
                } else {
                    ++pb;
                }
            }

            if (!matched_positions.empty()) {
                Posting match(list_a[i].doc_id);
                match.term_frequency = static_cast<uint32_t>(matched_positions.size());
                match.positions = std::move(matched_positions);
                result.push_back(std::move(match));
            }

            ++i;
            ++j;
        } else if (list_a[i].doc_id < list_b[j].doc_id) {
            ++i;
        } else {
            ++j;
        }
    }

    return result;
}

std::vector<Posting> PhraseMatcher::match_sequence(const std::vector<std::vector<Posting>>& term_postings) {
    if (term_postings.empty()) return {};
    if (term_postings.size() == 1) return term_postings[0];

    std::vector<Posting> current = term_postings[0];
    for (size_t k = 1; k < term_postings.size(); ++k) {
        current = match_pair(current, term_postings[k]);
        if (current.empty()) {
            break;
        }
    }

    return current;
}
