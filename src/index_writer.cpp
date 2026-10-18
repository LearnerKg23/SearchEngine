#include "index/index_writer.hpp"
#include <fstream>
#include <iostream>
#include <algorithm>
#include <numeric>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/stat.h>
#include <sys/types.h>
#endif

static bool make_directory(const std::string& path) {
#ifdef _WIN32
    return CreateDirectoryA(path.c_str(), NULL) || GetLastError() == ERROR_ALREADY_EXISTS;
#else
    return mkdir(path.c_str(), 0755) == 0 || errno == EEXIST;
#endif
}

IndexWriter::IndexWriter(std::string index_dir)
    : index_dir_(std::move(index_dir)) {}

void IndexWriter::clear() {
    doc_table_.clear();
    memory_index_.clear();
}

void IndexWriter::add_document(doc_id_t doc_id, const std::string& path, 
                               uint64_t last_modified, const std::vector<Token>& tokens) {
    uint32_t doc_len = static_cast<uint32_t>(tokens.size());
    doc_table_.push_back({doc_id, path, doc_len, last_modified});

    for (const auto& token : tokens) {
        if (token.text.empty()) continue;

        auto& postings = memory_index_[token.text];
        if (postings.empty() || postings.back().doc_id != doc_id) {
            postings.emplace_back(doc_id);
        }
        Posting& current = postings.back();
        current.term_frequency++;
        current.positions.push_back(token.position);
    }
}

bool IndexWriter::commit() {
    if (!make_directory(index_dir_)) {
        std::cerr << "Failed to create index directory: " << index_dir_ << "\n";
        return false;
    }

    std::string docs_path = index_dir_ + "/docs.bin";
    std::string dict_path = index_dir_ + "/dict.bin";
    std::string postings_path = index_dir_ + "/postings.bin";

    // 1. Serialize docs.bin
    {
        std::ofstream docs_out(docs_path, std::ios::binary);
        if (!docs_out.is_open()) return false;

        uint32_t magic = MAGIC_BYTES;
        uint16_t version = FORMAT_VERSION;
        uint32_t count = static_cast<uint32_t>(doc_table_.size());

        uint64_t total_words = 0;
        for (const auto& d : doc_table_) {
            total_words += d.length;
        }
        double avg_dl = (count > 0) ? (static_cast<double>(total_words) / count) : 0.0;

        docs_out.write(reinterpret_cast<const char*>(&magic), sizeof(magic));
        docs_out.write(reinterpret_cast<const char*>(&version), sizeof(version));
        docs_out.write(reinterpret_cast<const char*>(&count), sizeof(count));
        docs_out.write(reinterpret_cast<const char*>(&avg_dl), sizeof(avg_dl));

        for (const auto& doc : doc_table_) {
            docs_out.write(reinterpret_cast<const char*>(&doc.id), sizeof(doc.id));
            docs_out.write(reinterpret_cast<const char*>(&doc.length), sizeof(doc.length));
            docs_out.write(reinterpret_cast<const char*>(&doc.timestamp), sizeof(doc.timestamp));

            uint16_t path_len = static_cast<uint16_t>(doc.path.size());
            docs_out.write(reinterpret_cast<const char*>(&path_len), sizeof(path_len));
            docs_out.write(doc.path.data(), path_len);
        }
    }

    // 2. Serialize postings.bin and dict.bin simultaneously
    {
        std::ofstream postings_out(postings_path, std::ios::binary);
        std::ofstream dict_out(dict_path, std::ios::binary);
        if (!postings_out.is_open() || !dict_out.is_open()) return false;

        uint32_t magic = MAGIC_BYTES;
        uint16_t version = FORMAT_VERSION;
        uint32_t term_count = static_cast<uint32_t>(memory_index_.size());

        dict_out.write(reinterpret_cast<const char*>(&magic), sizeof(magic));
        dict_out.write(reinterpret_cast<const char*>(&version), sizeof(version));
        dict_out.write(reinterpret_cast<const char*>(&term_count), sizeof(term_count));

        for (auto& pair : memory_index_) {
            const std::string& term = pair.first;
            std::vector<Posting>& postings = pair.second;

            // Maintain strictly ascending doc_id invariant
            std::sort(postings.begin(), postings.end(), [](const Posting& a, const Posting& b) {
                return a.doc_id < b.doc_id;
            });

            uint64_t offset = static_cast<uint64_t>(postings_out.tellp());

            // Write postings payload to postings.bin
            for (const auto& p : postings) {
                postings_out.write(reinterpret_cast<const char*>(&p.doc_id), sizeof(p.doc_id));
                postings_out.write(reinterpret_cast<const char*>(&p.term_frequency), sizeof(p.term_frequency));

                uint32_t pos_count = static_cast<uint32_t>(p.positions.size());
                postings_out.write(reinterpret_cast<const char*>(&pos_count), sizeof(pos_count));
                if (pos_count > 0) {
                    postings_out.write(reinterpret_cast<const char*>(p.positions.data()), 
                                       pos_count * sizeof(uint32_t));
                }
            }

            uint64_t end_offset = static_cast<uint64_t>(postings_out.tellp());
            uint32_t byte_len = static_cast<uint32_t>(end_offset - offset);

            // Write term metadata to dict.bin
            uint16_t term_len = static_cast<uint16_t>(term.size());
            uint32_t doc_freq = static_cast<uint32_t>(postings.size());

            dict_out.write(reinterpret_cast<const char*>(&term_len), sizeof(term_len));
            dict_out.write(term.data(), term_len);
            dict_out.write(reinterpret_cast<const char*>(&doc_freq), sizeof(doc_freq));
            dict_out.write(reinterpret_cast<const char*>(&offset), sizeof(offset));
            dict_out.write(reinterpret_cast<const char*>(&byte_len), sizeof(byte_len));
        }
    }

    return true;
}
