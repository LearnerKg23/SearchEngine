#pragma once
#include "common/types.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>

class IndexWriter {
public:
    explicit IndexWriter(std::string index_dir);

    // Adds a document and its extracted tokens to the accumulation buffer
    void add_document(doc_id_t doc_id, const std::string& path, 
                      uint64_t last_modified, const std::vector<Token>& tokens);

    // Flushes in-memory index to binary files: dict.bin, postings.bin, and docs.bin
    bool commit();

    // Discards in-memory accumulation buffer
    void clear();

    size_t pending_documents() const { return doc_table_.size(); }
    size_t unique_terms() const { return memory_index_.size(); }

    static constexpr uint32_t MAGIC_BYTES = 0x53454152; // "SEAR"
    static constexpr uint16_t FORMAT_VERSION = 1;

private:
    std::string index_dir_;

    struct DocEntry {
        doc_id_t id;
        std::string path;
        uint32_t length;
        uint64_t timestamp;
    };

    std::vector<DocEntry> doc_table_;
    // In-memory inverted index: term -> list of postings
    std::unordered_map<std::string, std::vector<Posting>> memory_index_;
};
