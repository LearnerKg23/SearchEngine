#include "engine.hpp"
#include <fstream>
#include <iostream>

SearchEngine::SearchEngine(std::string index_dir)
    : index_dir_(std::move(index_dir)), tokenizer_(false) {
    writer_ = std::make_unique<IndexWriter>(index_dir_);
    reader_ = std::make_unique<IndexReader>(index_dir_);
    trie_ = std::make_unique<AutocompleteTrie>(5);
    tombstones_ = std::make_unique<TombstoneManager>(index_dir_ + "/tombstones.bin");
    manifest_ = std::make_unique<Manifest>(index_dir_ + "/manifest.bin");
}

SearchEngine::~SearchEngine() = default;

void SearchEngine::add_document(doc_id_t doc_id, const std::string& path, 
                                uint64_t timestamp, const std::string& content) {
    auto tokens = tokenizer_.tokenize(content);
    writer_->add_document(doc_id, path, timestamp, tokens);

    uint64_t hash = compute_fnv1a(content.data(), content.size());
    manifest_->record_file(path, doc_id, content.size(), timestamp, hash);
}

bool SearchEngine::commit() {
    if (!writer_->commit()) {
        return false;
    }
    manifest_->save();
    return load();
}

bool SearchEngine::load() {
    if (!reader_->open()) {
        return false;
    }

    parser_ = std::make_unique<QueryParser>(*reader_);
    ranker_ = std::make_unique<BM25Ranker>(*reader_);

    // Rebuild in-memory Trie from persistent dictionary
    trie_->clear();
    auto all_terms = reader_->get_all_terms();
    for (const auto& term : all_terms) {
        trie_->insert(term, reader_->get_doc_frequency(term));
    }

    tombstones_->load();
    manifest_->load();
    is_loaded_ = true;
    return true;
}

std::vector<SearchResult> SearchEngine::search(const std::string& query_str, size_t k) {
    if (!is_loaded_ && !load()) {
        return {};
    }

    // 1. Boolean and phrase retrieval
    auto raw_postings = parser_->execute(query_str);

    // 2. Filter out tombstoned (soft-deleted) documents
    std::vector<Posting> live_postings;
    live_postings.reserve(raw_postings.size());
    for (const auto& p : raw_postings) {
        if (!tombstones_->is_deleted(p.doc_id)) {
            live_postings.push_back(p);
        }
    }

    if (live_postings.empty()) {
        return {};
    }

    // 3. Extract query terms for BM25 IDF computation
    auto query_terms = tokenizer_.tokenize_query(query_str);
    std::vector<std::string> clean_terms;
    for (const auto& t : query_terms) {
        if (t != "AND" && t != "OR" && t != "NOT" && !t.empty() && t.front() != '"') {
            clean_terms.push_back(t);
        }
    }

    // 4. BM25 scoring & O(N log K) Min-Heap Top-K selection
    auto top_k = ranker_->rank_top_k(clean_terms, live_postings, k);

    // 5. Assemble search results with file metadata and snippets
    std::vector<SearchResult> results;
    results.reserve(top_k.size());

    for (const auto& scored : top_k) {
        SearchResult res;
        res.doc_id = scored.doc_id;
        res.score = scored.score;
        res.path = reader_->get_document_path(scored.doc_id);
        res.snippet = extract_snippet(res.path, {});
        results.push_back(std::move(res));
    }

    return results;
}

std::vector<std::string> SearchEngine::autocomplete(const std::string& prefix, size_t limit) const {
    return trie_->get_completions(prefix, limit);
}

void SearchEngine::delete_document(doc_id_t doc_id) {
    tombstones_->mark_deleted(doc_id);
    tombstones_->save();
}

uint32_t SearchEngine::total_documents() const {
    return reader_->total_docs();
}

size_t SearchEngine::vocabulary_size() const {
    return trie_->size();
}

std::string SearchEngine::extract_snippet(const std::string& filepath, 
                                          const std::vector<uint32_t>& /*positions*/) const {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        return "[Preview unavailable]";
    }

    std::string line;
    if (std::getline(file, line)) {
        if (line.size() > 120) {
            line = line.substr(0, 117) + "...";
        }
        return line;
    }
    return "";
}
