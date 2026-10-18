# Milestone 6: Incremental Indexing & Soft-Deletion Tombstones

> **Resume Alignment**: *"Designed an incremental indexing architecture using a file-metadata manifest and soft-deletion tombstones, enabling real-time file modifications without expensive full-index rebuilds."*

---

## 🎯 Objectives
1. Eliminate expensive full-index rebuilds when files are modified, added, or deleted in a 25,000+ file workspace.
2. Implement a **File Metadata Manifest (`manifest.bin`)** tracking file paths, timestamps, sizes, and content hashes (FNV-1a / CRC32).
3. Implement **Soft-Deletion Tombstones** using an $O(1)$ bitset to immediately invalidate modified/deleted documents.
4. Architect an **In-Memory Delta Index** that buffers real-time updates and merges them with the base disk index during queries.

---

## 1. The Architecture of Incremental Updates

```
                           [ File Modification Detected ]
                                         │
                   ┌─────────────────────┴─────────────────────┐
                   │                                           │
          [ Old File Version ]                        [ New File Version ]
                   │                                           │
         Mark old `doc_id` in                         Assign new `doc_id`
         Tombstone Bitset ($O(1)$)                    Index into In-Memory
                   │                                     Delta Buffer
                   ▼                                           │
        ┌─────────────────────┐                                │
        │ Base Disk Index     │                                │
        │ (Ignores Tombstones)│                                │
        └──────────┬──────────┘                                │
                   │                                           │
                   └─────────────────┬─────────────────────────┘
                                     ▼
                           [ Unified Query View ]
```

---

## 2. File Metadata Manifest (`include/index/manifest.hpp`)

The manifest persists state about every file currently tracked:

```cpp
#pragma once
#include "common/types.hpp"
#include <string>
#include <unordered_map>
#include <vector>
#include <filesystem>

struct FileRecord {
    doc_id_t doc_id;
    uint64_t file_size;
    uint64_t last_modified_time;
    uint64_t content_hash; // FNV-1a or CRC64 for rapid change verification
};

class Manifest {
public:
    explicit Manifest(const std::string& manifest_path);

    bool load();
    bool save() const;

    // Fast check: has file been modified or is it new?
    enum class FileStatus { UNCHANGED, MODIFIED, NEW, DELETED };
    FileStatus check_file(const std::string& path) const;

    void record_file(const std::string& path, doc_id_t doc_id, 
                     uint64_t size, uint64_t mtime, uint64_t hash);
    void remove_file(const std::string& path);

    const std::unordered_map<std::string, FileRecord>& all_records() const { return records_; }

private:
    std::string path_;
    std::unordered_map<std::string, FileRecord> records_;
};
```

### High-Speed FNV-1a Content Hashing
```cpp
inline uint64_t fnv1a_hash(const char* data, size_t length) {
    uint64_t hash = 14695981039346656037ULL;
    for (size_t i = 0; i < length; ++i) {
        hash ^= static_cast<uint8_t>(data[i]);
        hash *= 1099511628211ULL;
    }
    return hash;
}
```

---

## 3. Soft-Deletion Tombstone Bitset

Instead of mutating existing binary postings on disk (which would require shifting gigabytes of data):
1. Maintain a dynamic bitset: `std::vector<bool> tombstones_`.
2. When a file is modified:
   - Look up its old `doc_id` in `Manifest`.
   - Set `tombstones_[old_doc_id] = true` ($O(1)$ operation).
   - Re-index the new version with an incremented `new_doc_id` into the **Delta Index**.
3. During query processing, filter out any candidate where `tombstones_[doc_id] == true`.

```cpp
class TombstoneManager {
public:
    void mark_deleted(doc_id_t id) {
        if (id >= tombstones_.size()) {
            tombstones_.resize(id + 1024, false);
        }
        tombstones_[id] = true;
        deleted_count_++;
    }

    bool is_deleted(doc_id_t id) const {
        if (id >= tombstones_.size()) return false;
        return tombstones_[id];
    }

    size_t deleted_count() const { return deleted_count_; }

    // Save and load tombstones to disk
    void persist(const std::string& path) const;
    void load(const std::string& path);

private:
    std::vector<bool> tombstones_;
    size_t deleted_count_ = 0;
};
```

---

## 4. In-Memory Delta Index & Query Merging

When querying:
1. Fetch postings list from **Base Disk Index** $\to$ filter out documents matching `tombstones_.is_deleted(doc_id)`.
2. Fetch postings list from **In-Memory Delta Index**.
3. Merge both lists using the Two-Pointer union algorithm.
4. Pass the unified postings to BM25 and Min-Heap Top-K.

### Index Compaction / Vacuuming
When `deleted_count() / total_docs() > 0.15` (over 15% dead entries):
* Trigger background **Compaction**:
  - Read `docs.bin` and `postings.bin`.
  - Rewrite disk files skipping tombstoned doc IDs.
  - Reset tombstones bitset.
  - Flush the delta index into the new base index.

---

## 5. Verification & Unit Tests (`tests/test_incremental.cpp`)
* **Test 1: Modify File Without Full Rebuild**
  - Index `fileA.txt` containing `"alpha beta"`. Commit to disk.
  - Verify query `"alpha"` returns `fileA.txt`.
  - Modify `fileA.txt` to contain `"alpha gamma"` (removed `"beta"`).
  - Run incremental update.
  - Assert query `"beta"` returns **0 results** (tombstoned).
  - Assert query `"gamma"` returns `fileA.txt` (from Delta Index).
  - Measure time taken: assert update completes in **< 5 ms** (versus full rebuild).
* **Test 2: Delete File**
  - Delete `fileA.txt` from disk.
  - Run incremental check. Assert old `doc_id` is tombstoned and search excludes it.
* **Test 3: Compaction & Vacuum**
  - Trigger compaction and verify disk size shrinks and data integrity remains 100%.

---

## ✅ Milestone 6 Checklist
- [ ] `Manifest` tracking paths, sizes, timestamps, and FNV-1a hashes.
- [ ] `TombstoneManager` bitset with $O(1)$ deletion checks.
- [ ] Incremental update workflow updating Delta Index and tombstones.
- [ ] Compactor / vacuum tool for purging dead records.
