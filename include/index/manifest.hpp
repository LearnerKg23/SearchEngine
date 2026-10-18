#pragma once
#include "common/types.hpp"
#include <string>
#include <unordered_map>
#include <vector>
#include <cstdint>

// 64-bit FNV-1a content hash
inline uint64_t compute_fnv1a(const char* data, size_t length) {
    uint64_t hash = 14695981039346656037ULL;
    for (size_t i = 0; i < length; ++i) {
        hash ^= static_cast<uint8_t>(data[i]);
        hash *= 1099511628211ULL;
    }
    return hash;
}

struct FileMetadata {
    doc_id_t doc_id;
    uint64_t file_size;
    uint64_t last_modified_time;
    uint64_t content_hash;
};

class Manifest {
public:
    enum class FileStatus { UNCHANGED, MODIFIED, NEW, DELETED };

    explicit Manifest(std::string manifest_path);

    bool load();
    bool save() const;

    FileStatus check_file(const std::string& path, uint64_t size, 
                          uint64_t mtime, uint64_t hash) const;

    void record_file(const std::string& path, doc_id_t id, 
                     uint64_t size, uint64_t mtime, uint64_t hash);

    void remove_file(const std::string& path);

    bool has_file(const std::string& path) const;
    doc_id_t get_doc_id(const std::string& path) const;

    size_t size() const { return entries_.size(); }
    const std::unordered_map<std::string, FileMetadata>& entries() const { return entries_; }

private:
    std::string manifest_path_;
    std::unordered_map<std::string, FileMetadata> entries_;

    static constexpr uint32_t MANIFEST_MAGIC = 0x4D414E49; // "MANI"
    static constexpr uint16_t MANIFEST_VERSION = 1;
};

class TombstoneManager {
public:
    explicit TombstoneManager(std::string storage_path = "");

    void mark_deleted(doc_id_t id);
    bool is_deleted(doc_id_t id) const;

    size_t deleted_count() const { return deleted_count_; }
    void clear();

    bool save() const;
    bool load();

private:
    std::string storage_path_;
    std::vector<bool> tombstones_;
    size_t deleted_count_ = 0;
};
