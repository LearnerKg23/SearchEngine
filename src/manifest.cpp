#include "index/manifest.hpp"
#include <fstream>
#include <iostream>

Manifest::Manifest(std::string manifest_path)
    : manifest_path_(std::move(manifest_path)) {}

bool Manifest::save() const {
    std::ofstream out(manifest_path_, std::ios::binary);
    if (!out.is_open()) return false;

    uint32_t magic = MANIFEST_MAGIC;
    uint16_t version = MANIFEST_VERSION;
    uint32_t count = static_cast<uint32_t>(entries_.size());

    out.write(reinterpret_cast<const char*>(&magic), sizeof(magic));
    out.write(reinterpret_cast<const char*>(&version), sizeof(version));
    out.write(reinterpret_cast<const char*>(&count), sizeof(count));

    for (const auto& pair : entries_) {
        const std::string& path = pair.first;
        const FileMetadata& meta = pair.second;

        uint16_t path_len = static_cast<uint16_t>(path.size());
        out.write(reinterpret_cast<const char*>(&path_len), sizeof(path_len));
        out.write(path.data(), path_len);

        out.write(reinterpret_cast<const char*>(&meta.doc_id), sizeof(meta.doc_id));
        out.write(reinterpret_cast<const char*>(&meta.file_size), sizeof(meta.file_size));
        out.write(reinterpret_cast<const char*>(&meta.last_modified_time), sizeof(meta.last_modified_time));
        out.write(reinterpret_cast<const char*>(&meta.content_hash), sizeof(meta.content_hash));
    }

    return true;
}

bool Manifest::load() {
    entries_.clear();
    std::ifstream in(manifest_path_, std::ios::binary);
    if (!in.is_open()) return false;

    uint32_t magic = 0;
    uint16_t version = 0;
    in.read(reinterpret_cast<char*>(&magic), sizeof(magic));
    in.read(reinterpret_cast<char*>(&version), sizeof(version));

    if (magic != MANIFEST_MAGIC || version != MANIFEST_VERSION) {
        return false;
    }

    uint32_t count = 0;
    in.read(reinterpret_cast<char*>(&count), sizeof(count));

    for (uint32_t i = 0; i < count; ++i) {
        uint16_t path_len = 0;
        in.read(reinterpret_cast<char*>(&path_len), sizeof(path_len));

        std::string path(path_len, '\0');
        in.read(&path[0], path_len);

        FileMetadata meta{};
        in.read(reinterpret_cast<char*>(&meta.doc_id), sizeof(meta.doc_id));
        in.read(reinterpret_cast<char*>(&meta.file_size), sizeof(meta.file_size));
        in.read(reinterpret_cast<char*>(&meta.last_modified_time), sizeof(meta.last_modified_time));
        in.read(reinterpret_cast<char*>(&meta.content_hash), sizeof(meta.content_hash));

        entries_[path] = meta;
    }

    return true;
}

Manifest::FileStatus Manifest::check_file(const std::string& path, uint64_t size, 
                                          uint64_t mtime, uint64_t hash) const {
    auto it = entries_.find(path);
    if (it == entries_.end()) {
        return FileStatus::NEW;
    }
    if (it->second.content_hash != hash || it->second.file_size != size) {
        return FileStatus::MODIFIED;
    }
    return FileStatus::UNCHANGED;
}

void Manifest::record_file(const std::string& path, doc_id_t id, 
                           uint64_t size, uint64_t mtime, uint64_t hash) {
    entries_[path] = {id, size, mtime, hash};
}

void Manifest::remove_file(const std::string& path) {
    entries_.erase(path);
}

bool Manifest::has_file(const std::string& path) const {
    return entries_.find(path) != entries_.end();
}

doc_id_t Manifest::get_doc_id(const std::string& path) const {
    auto it = entries_.find(path);
    if (it == entries_.end()) return INVALID_DOC_ID;
    return it->second.doc_id;
}

// ---------------- TombstoneManager Implementation ----------------

TombstoneManager::TombstoneManager(std::string storage_path)
    : storage_path_(std::move(storage_path)) {}

void TombstoneManager::mark_deleted(doc_id_t id) {
    if (id >= tombstones_.size()) {
        tombstones_.resize(id + 1024, false);
    }
    if (!tombstones_[id]) {
        tombstones_[id] = true;
        deleted_count_++;
    }
}

bool TombstoneManager::is_deleted(doc_id_t id) const {
    if (id >= tombstones_.size()) return false;
    return tombstones_[id];
}

void TombstoneManager::clear() {
    tombstones_.clear();
    deleted_count_ = 0;
}

bool TombstoneManager::save() const {
    if (storage_path_.empty()) return false;
    std::ofstream out(storage_path_, std::ios::binary);
    if (!out.is_open()) return false;

    uint32_t size = static_cast<uint32_t>(tombstones_.size());
    out.write(reinterpret_cast<const char*>(&size), sizeof(size));

    for (size_t i = 0; i < tombstones_.size(); ++i) {
        uint8_t bit = tombstones_[i] ? 1 : 0;
        out.write(reinterpret_cast<const char*>(&bit), sizeof(bit));
    }

    return true;
}

bool TombstoneManager::load() {
    if (storage_path_.empty()) return false;
    std::ifstream in(storage_path_, std::ios::binary);
    if (!in.is_open()) return false;

    uint32_t size = 0;
    in.read(reinterpret_cast<char*>(&size), sizeof(size));

    tombstones_.assign(size, false);
    deleted_count_ = 0;

    for (uint32_t i = 0; i < size; ++i) {
        uint8_t bit = 0;
        in.read(reinterpret_cast<char*>(&bit), sizeof(bit));
        if (bit) {
            tombstones_[i] = true;
            deleted_count_++;
        }
    }

    return true;
}
