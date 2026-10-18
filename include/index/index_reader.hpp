#pragma once
#include "common/types.hpp"
#include <string>
#include <unordered_map>
#include <vector>
#include <fstream>
#include <cstdint>

struct TermRecord {
    uint32_t doc_frequency;
    uint64_t postings_offset;
    uint32_t postings_byte_len;
};

class IndexReader {
public:
    explicit IndexReader(std::string index_dir);
    ~IndexReader();

    // Loads dictionary and document table into memory; opens postings file stream
    bool open();
    void close();
    bool is_open() const { return is_open_; }

    // Fast O(1) in-memory dictionary lookup
    bool has_term(const std::string& term) const;
    uint32_t get_doc_frequency(const std::string& term) const;

    // Reads postings list from disk using dictionary byte offset
    std::vector<Posting> get_postings(const std::string& term);

    // Document table metadata
    uint32_t total_docs() const { return total_docs_; }
    double average_doc_length() const { return avg_doc_length_; }
    std::string get_document_path(doc_id_t doc_id) const;
    uint32_t get_document_length(doc_id_t doc_id) const;
    uint64_t get_document_timestamp(doc_id_t doc_id) const;

    // Returns list of all indexed vocabulary terms
    std::vector<std::string> get_all_terms() const;

    static constexpr uint32_t MAGIC_BYTES = 0x53454152; // "SEAR"
    static constexpr uint16_t FORMAT_VERSION = 1;

private:
    std::string index_dir_;
    bool is_open_ = false;

    uint32_t total_docs_ = 0;
    double avg_doc_length_ = 0.0;

    // Loaded in RAM for O(1) term seeking
    std::unordered_map<std::string, TermRecord> dictionary_;

    struct DocMeta {
        std::string path;
        uint32_t length;
        uint64_t timestamp;
    };
    std::unordered_map<doc_id_t, DocMeta> doc_table_;

    std::ifstream postings_stream_;
};
