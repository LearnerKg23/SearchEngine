#pragma once
#include <cstdint>
#include <string>
#include <vector>

// Unique document identifier (0 to 4,294,967,294)
using doc_id_t = uint32_t;

// Sentinel value for uninitialized or invalid document ID
constexpr doc_id_t INVALID_DOC_ID = 0xFFFFFFFF;

// Represents an extracted token with positional and source metadata
struct Token {
    std::string text;           // Lowercase normalized token string
    uint32_t position;          // 0-based word index in document (for phrase/proximity search)
    uint32_t char_offset;       // Character offset in original source text (for snippet highlighting)
    bool is_code_definition;    // True if token appeared as a function, class, or variable definition

    Token() : position(0), char_offset(0), is_code_definition(false) {}
    Token(std::string t, uint32_t pos, uint32_t offset, bool is_def = false)
        : text(std::move(t)), position(pos), char_offset(offset), is_code_definition(is_def) {}
};

// Represents a single posting in an inverted postings list
struct Posting {
    doc_id_t doc_id;
    uint32_t term_frequency;
    std::vector<uint32_t> positions; // 0-based positions where term occurs in doc_id

    Posting() : doc_id(INVALID_DOC_ID), term_frequency(0) {}
    explicit Posting(doc_id_t id) : doc_id(id), term_frequency(0) {}
    Posting(doc_id_t id, uint32_t tf, std::vector<uint32_t> pos)
        : doc_id(id), term_frequency(tf), positions(std::move(pos)) {}
};

// Represents a document scored and ranked for retrieval
struct ScoredDoc {
    doc_id_t doc_id;
    float score;

    ScoredDoc() : doc_id(INVALID_DOC_ID), score(0.0f) {}
    ScoredDoc(doc_id_t id, float s) : doc_id(id), score(s) {}

    // Greater-than comparator for Min-Heap top-K ordering
    bool operator>(const ScoredDoc& other) const {
        return score > other.score;
    }

    // Less-than comparator for standard descending sort
    bool operator<(const ScoredDoc& other) const {
        return score < other.score;
    }
};
