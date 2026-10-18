#include "query/boolean_retriever.hpp"

std::vector<Posting> BooleanRetriever::intersect(const std::vector<Posting>& list_a, 
                                                 const std::vector<Posting>& list_b) {
    std::vector<Posting> result;
    size_t i = 0, j = 0;

    while (i < list_a.size() && j < list_b.size()) {
        if (list_a[i].doc_id == list_b[j].doc_id) {
            Posting combined(list_a[i].doc_id);
            combined.term_frequency = list_a[i].term_frequency + list_b[j].term_frequency;
            // Merge positions for possible downstream proximity checks
            combined.positions = list_a[i].positions;
            combined.positions.insert(combined.positions.end(), 
                                      list_b[j].positions.begin(), 
                                      list_b[j].positions.end());
            result.push_back(std::move(combined));
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

std::vector<Posting> BooleanRetriever::merge_union(const std::vector<Posting>& list_a, 
                                                   const std::vector<Posting>& list_b) {
    std::vector<Posting> result;
    size_t i = 0, j = 0;

    while (i < list_a.size() && j < list_b.size()) {
        if (list_a[i].doc_id == list_b[j].doc_id) {
            Posting combined(list_a[i].doc_id);
            combined.term_frequency = list_a[i].term_frequency + list_b[j].term_frequency;
            combined.positions = list_a[i].positions;
            combined.positions.insert(combined.positions.end(), 
                                      list_b[j].positions.begin(), 
                                      list_b[j].positions.end());
            result.push_back(std::move(combined));
            ++i;
            ++j;
        } else if (list_a[i].doc_id < list_b[j].doc_id) {
            result.push_back(list_a[i]);
            ++i;
        } else {
            result.push_back(list_b[j]);
            ++j;
        }
    }

    while (i < list_a.size()) result.push_back(list_a[i++]);
    while (j < list_b.size()) result.push_back(list_b[j++]);

    return result;
}

std::vector<Posting> BooleanRetriever::difference(const std::vector<Posting>& list_a, 
                                                  const std::vector<Posting>& list_b) {
    std::vector<Posting> result;
    size_t i = 0, j = 0;

    while (i < list_a.size() && j < list_b.size()) {
        if (list_a[i].doc_id == list_b[j].doc_id) {
            // Excluded by difference
            ++i;
            ++j;
        } else if (list_a[i].doc_id < list_b[j].doc_id) {
            result.push_back(list_a[i]);
            ++i;
        } else {
            ++j;
        }
    }

    while (i < list_a.size()) {
        result.push_back(list_a[i++]);
    }

    return result;
}
