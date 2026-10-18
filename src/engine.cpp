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
    if (file.is_open()) {
        std::string line;
        std::string candidate;
        while (std::getline(file, line)) {
            // Trim leading whitespace for clean display
            size_t start = line.find_first_not_of(" \t\r\n");
            if (start != std::string::npos) {
                std::string trimmed = line.substr(start);
                // Prefer definition lines, functions, classes, or code logic
                if (trimmed.find("class ") != std::string::npos ||
                    trimmed.find("struct ") != std::string::npos ||
                    trimmed.find("void ") != std::string::npos ||
                    trimmed.find("return ") != std::string::npos ||
                    trimmed.find("bool ") != std::string::npos ||
                    trimmed.find("float ") != std::string::npos) {
                    if (trimmed.size() > 140) trimmed = trimmed.substr(0, 137) + "...";
                    return trimmed;
                }
                if (candidate.empty() && trimmed.size() > 5) {
                    candidate = trimmed;
                }
            }
        }
        if (!candidate.empty()) {
            if (candidate.size() > 140) candidate = candidate.substr(0, 137) + "...";
            return candidate;
        }
    }

    return "// Source: " + filepath + " • Compiled C++ module";
}
