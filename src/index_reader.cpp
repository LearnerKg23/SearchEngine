#include "index/index_reader.hpp"
#include <iostream>

IndexReader::IndexReader(std::string index_dir)
    : index_dir_(std::move(index_dir)), is_open_(false) {}

IndexReader::~IndexReader() {
    close();
}

void IndexReader::close() {
    if (postings_stream_.is_open()) {
        postings_stream_.close();
    }
    dictionary_.clear();
    doc_table_.clear();
    total_docs_ = 0;
    avg_doc_length_ = 0.0;
    is_open_ = false;
}

bool IndexReader::open() {
    close();

    std::string docs_path = index_dir_ + "/docs.bin";
    std::string dict_path = index_dir_ + "/dict.bin";
    std::string postings_path = index_dir_ + "/postings.bin";

    // 1. Read docs.bin
    {
        std::ifstream docs_in(docs_path, std::ios::binary);
        if (!docs_in.is_open()) return false;

        uint32_t magic = 0;
        uint16_t version = 0;
        docs_in.read(reinterpret_cast<char*>(&magic), sizeof(magic));
        docs_in.read(reinterpret_cast<char*>(&version), sizeof(version));

        if (magic != MAGIC_BYTES || version != FORMAT_VERSION) {
            std::cerr << "Invalid docs.bin magic/version\n";
            return false;
        }

        docs_in.read(reinterpret_cast<char*>(&total_docs_), sizeof(total_docs_));
        docs_in.read(reinterpret_cast<char*>(&avg_doc_length_), sizeof(avg_doc_length_));

        for (uint32_t i = 0; i < total_docs_; ++i) {
            doc_id_t id = 0;
            uint32_t len = 0;
            uint64_t ts = 0;
            uint16_t path_len = 0;

            docs_in.read(reinterpret_cast<char*>(&id), sizeof(id));
            docs_in.read(reinterpret_cast<char*>(&len), sizeof(len));
            docs_in.read(reinterpret_cast<char*>(&ts), sizeof(ts));
            docs_in.read(reinterpret_cast<char*>(&path_len), sizeof(path_len));

            std::string path(path_len, '\0');
            docs_in.read(&path[0], path_len);

            doc_table_[id] = {std::move(path), len, ts};
        }
    }

    // 2. Read dict.bin
    {
        std::ifstream dict_in(dict_path, std::ios::binary);
        if (!dict_in.is_open()) return false;

        uint32_t magic = 0;
        uint16_t version = 0;
        dict_in.read(reinterpret_cast<char*>(&magic), sizeof(magic));
        dict_in.read(reinterpret_cast<char*>(&version), sizeof(version));

        if (magic != MAGIC_BYTES || version != FORMAT_VERSION) {
            std::cerr << "Invalid dict.bin magic/version\n";
            return false;
        }

        uint32_t term_count = 0;
        dict_in.read(reinterpret_cast<char*>(&term_count), sizeof(term_count));

        for (uint32_t i = 0; i < term_count; ++i) {
            uint16_t term_len = 0;
            dict_in.read(reinterpret_cast<char*>(&term_len), sizeof(term_len));

            std::string term(term_len, '\0');
            dict_in.read(&term[0], term_len);

            TermRecord rec{};
            dict_in.read(reinterpret_cast<char*>(&rec.doc_frequency), sizeof(rec.doc_frequency));
            dict_in.read(reinterpret_cast<char*>(&rec.postings_offset), sizeof(rec.postings_offset));
            dict_in.read(reinterpret_cast<char*>(&rec.postings_byte_len), sizeof(rec.postings_byte_len));

            dictionary_[term] = rec;
        }
    }

    // 3. Open postings.bin stream for fast random access
    postings_stream_.open(postings_path, std::ios::binary);
    if (!postings_stream_.is_open()) {
        return false;
    }

    is_open_ = true;
    return true;
}

bool IndexReader::has_term(const std::string& term) const {
    return dictionary_.find(term) != dictionary_.end();
}

uint32_t IndexReader::get_doc_frequency(const std::string& term) const {
    auto it = dictionary_.find(term);
    if (it == dictionary_.end()) return 0;
    return it->second.doc_frequency;
}

std::vector<Posting> IndexReader::get_postings(const std::string& term) {
    auto it = dictionary_.find(term);
    if (it == dictionary_.end() || !postings_stream_.is_open()) {
        return {};
    }

    const TermRecord& rec = it->second;
    postings_stream_.clear(); // Clear any eof/error flags before seek
    postings_stream_.seekg(static_cast<std::streamoff>(rec.postings_offset), std::ios::beg);

    std::vector<Posting> postings;
    postings.reserve(rec.doc_frequency);

    for (uint32_t i = 0; i < rec.doc_frequency; ++i) {
        Posting p;
        postings_stream_.read(reinterpret_cast<char*>(&p.doc_id), sizeof(p.doc_id));
        postings_stream_.read(reinterpret_cast<char*>(&p.term_frequency), sizeof(p.term_frequency));

        uint32_t pos_count = 0;
        postings_stream_.read(reinterpret_cast<char*>(&pos_count), sizeof(pos_count));

        if (pos_count > 0) {
            p.positions.resize(pos_count);
            postings_stream_.read(reinterpret_cast<char*>(p.positions.data()), 
                                 pos_count * sizeof(uint32_t));
        }

        postings.push_back(std::move(p));
    }

    return postings;
}

std::string IndexReader::get_document_path(doc_id_t doc_id) const {
    auto it = doc_table_.find(doc_id);
    if (it == doc_table_.end()) return "";
    return it->second.path;
}

uint32_t IndexReader::get_document_length(doc_id_t doc_id) const {
    auto it = doc_table_.find(doc_id);
    if (it == doc_table_.end()) return 0;
    return it->second.length;
}

uint64_t IndexReader::get_document_timestamp(doc_id_t doc_id) const {
    auto it = doc_table_.find(doc_id);
    if (it == doc_table_.end()) return 0;
    return it->second.timestamp;
}

std::vector<std::string> IndexReader::get_all_terms() const {
    std::vector<std::string> terms;
    terms.reserve(dictionary_.size());
    for (const auto& pair : dictionary_) {
        terms.push_back(pair.first);
    }
    return terms;
}
