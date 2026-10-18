#pragma once
#include "common/types.hpp"
#include "tokenizer/tokenizer.hpp"
#include "index/index_writer.hpp"
#include "index/index_reader.hpp"
#include "index/manifest.hpp"
#include "query/query_parser.hpp"
#include "query/bm25_ranker.hpp"
#include "autocomplete/trie.hpp"
#include <string>
#include <vector>
#include <memory>

struct SearchResult {
    doc_id_t doc_id;
    std::string path;
    float score;
    std::string snippet;
};

class SearchEngine {
public:
    explicit SearchEngine(std::string index_dir = ".search_index");
    ~SearchEngine();

    // Ingests and indexes a single document
    void add_document(doc_id_t doc_id, const std::string& path, 
                      uint64_t timestamp, const std::string& content);

    // Flushes all in-memory postings to persistent disk files
    bool commit();

    // Loads persistent binary index into memory
    bool load();

    // Executes search query, returning top-K BM25 ranked results
    std::vector<SearchResult> search(const std::string& query_str, size_t k = 10);

    // Real-time keystroke autocomplete suggestions
    std::vector<std::string> autocomplete(const std::string& prefix, size_t limit = 5) const;

    // Soft delete a document
    void delete_document(doc_id_t doc_id);

    uint32_t total_documents() const;
    size_t vocabulary_size() const;

private:
    std::string index_dir_;
    Tokenizer tokenizer_;
    std::unique_ptr<IndexWriter> writer_;
    std::unique_ptr<IndexReader> reader_;
    std::unique_ptr<QueryParser> parser_;
    std::unique_ptr<BM25Ranker> ranker_;
    std::unique_ptr<AutocompleteTrie> trie_;
    std::unique_ptr<TombstoneManager> tombstones_;
    std::unique_ptr<Manifest> manifest_;

    bool is_loaded_ = false;

    std::string extract_snippet(const std::string& filepath, 
                                const std::vector<uint32_t>& positions) const;
};
