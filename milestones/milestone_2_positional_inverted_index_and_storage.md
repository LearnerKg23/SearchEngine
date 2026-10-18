# Milestone 2: Positional Inverted Index & Persistent Binary Disk Storage

> **Resume Alignment**: *"Architected a local document and code search engine from scratch in C++, implementing... positional inverted indexing, persistent disk storage, and information retrieval without external databases."*

---

## 🎯 Objectives
1. Implement the **Positional Inverted Index** data structure in memory.
2. Design a **custom binary disk storage format** with zero external database dependencies (no SQLite, RocksDB, or LevelDB).
3. Implement `IndexWriter` (binary serialization) and `IndexReader` (fast dictionary lookup + on-demand disk seeking).

---

## 1. Storage Architecture: 3-File Binary Model

Instead of an external database, the engine persists index data across three clean, specialized binary files:

```
[ .search_index/ ]
 ├── dict.bin        # Vocabulary / Dictionary: maps term -> (doc_freq, postings_offset, byte_len)
 ├── postings.bin    # Contiguous binary blocks of postings lists (doc_id, tf, positions)
 └── docs.bin        # Document table: doc_id -> (filepath, doc_length, timestamp)
```

```
                                [ DICTIONARY (dict.bin) ]
  ┌───────────────┬──────────────┬──────────────────┬─────────────────┬──────────────────┐
  │ Term Len (u16)│  Term Bytes  │  Doc Freq (u32)  │ Disk Offset(u64)│ Byte Length(u32) │
  ├───────────────┼──────────────┼──────────────────┼─────────────────┼──────────────────┤
  │       6       │  "search"    │        45        │     0x00001020  │       512        │
  └───────────────┴──────────────┴──────────────────┴─────────────────┴──────────────────┘
                                                            │
                                              Points into   ▼
                                [ POSTINGS FILE (postings.bin) ]
  ┌─────────────────┬─────────────────┬─────────────────┬────────────────────────────────┐
  │ Doc ID (u32)    │ Term Freq (u32) │ Pos Count (u32) │ Positions array [u32, u32, ..] │
  ├─────────────────┼─────────────────┼─────────────────┼────────────────────────────────┤
  │      101        │        2        │        2        │           [4, 18]              │
  └─────────────────┴─────────────────┴─────────────────┴────────────────────────────────┘
```

---

## 2. Binary Layout Specification

### A. File Headers
Every binary file begins with an 8-byte magic sequence and metadata:
* **Magic Bytes**: `0x53 0x45 0x41 0x52` (`"SEAR"`)
* **Format Version**: `uint16_t` (e.g. `1`)
* **Total Entries**: `uint32_t` (number of terms or documents)

### B. `dict.bin` Record Layout
For each unique term:
1. `uint16_t term_length`
2. `char term_bytes[term_length]`
3. `uint32_t doc_frequency` (number of documents containing the term)
4. `uint64_t postings_offset` (byte offset in `postings.bin`)
5. `uint32_t postings_bytes` (byte size of this term's postings payload)

### C. `postings.bin` Payload Layout
For each document matching the term:
1. `uint32_t doc_id`
2. `uint32_t term_frequency`
3. `uint32_t position_count`
4. `uint32_t positions[position_count]`

### D. `docs.bin` Record Layout
For each indexed document:
1. `uint32_t doc_id`
2. `uint32_t doc_length` (total word count, crucial for BM25 length normalization)
3. `uint64_t last_modified` (timestamp for incremental change detection)
4. `uint16_t path_length`
5. `char path_bytes[path_length]`

---

## 3. Class Interfaces

### `IndexWriter` (`include/index/index_writer.hpp`)
```cpp
#pragma once
#include "common/types.hpp"
#include <string>
#include <unordered_map>
#include <vector>

class IndexWriter {
public:
    explicit IndexWriter(const std::string& index_dir);

    // Adds a tokenized document to the in-memory accumulation buffer
    void add_document(doc_id_t doc_id, const std::string& path, 
                      uint64_t last_modified, const std::vector<Token>& tokens);

    // Flushes in-memory postings and dictionary to persistent binary disk files
    void commit();

private:
    std::string index_dir_;
    
    // In-memory accumulation: term -> list of postings
    std::unordered_map<std::string, std::vector<Posting>> memory_index_;
    
    struct DocRecord {
        doc_id_t id;
        std::string path;
        uint32_t length;
        uint64_t timestamp;
    };
    std::vector<DocRecord> doc_table_;
};
```

### `IndexReader` (`include/index/index_reader.hpp`)
```cpp
#pragma once
#include "common/types.hpp"
#include <string>
#include <unordered_map>
#include <fstream>
#include <memory>

struct TermMeta {
    uint32_t doc_frequency;
    uint64_t disk_offset;
    uint32_t byte_length;
};

class IndexReader {
public:
    explicit IndexReader(const std::string& index_dir);
    bool open();

    // Fast O(1) in-memory dictionary lookup
    bool has_term(const std::string& term) const;
    uint32_t get_doc_frequency(const std::string& term) const;

    // Seeks postings.bin on disk and deserializes postings list
    std::vector<Posting> get_postings(const std::string& term);

    // Document table metadata
    uint32_t total_docs() const { return total_docs_; }
    double average_doc_length() const { return avg_doc_length_; }
    std::string get_document_path(doc_id_t doc_id) const;
    uint32_t get_document_length(doc_id_t doc_id) const;

private:
    std::string index_dir_;
    uint32_t total_docs_ = 0;
    double avg_doc_length_ = 0.0;

    // Loaded entirely in memory for instant O(1) term seeking
    std::unordered_map<std::string, TermMeta> dictionary_;
    std::unordered_map<doc_id_t, std::string> doc_paths_;
    std::unordered_map<doc_id_t, uint32_t> doc_lengths_;

    std::ifstream postings_stream_;
};
```

---

## 4. Key Performance Details
1. **Zero External DB Overhead**: Bypasses SQL parsing, write-ahead logs, and page cache overhead. Disk writes are purely sequential streaming appends.
2. **Instant Warmup**: Only `dict.bin` and `docs.bin` headers are loaded into memory on startup. Postings lists are read on-demand via `std::ifstream::seekg()`.
3. **Sorted Invariant**: Postings in `postings.bin` are strictly written in ascending `doc_id` order, which is the foundational prerequisite for the Two-Pointer retrieval algorithm in Milestone 3.

---

## 5. Verification & Unit Tests (`tests/test_index.cpp`)
* **Test 1: Write and Read Multi-Doc Corpus**
  - Index 5 synthetic documents with known terms (`"search"`, `"engine"`, `"cpp"`).
  - Call `commit()`, close writer.
  - Open `IndexReader`, verify `has_term("engine") == true`.
  - Verify `get_postings("engine")` returns exact `doc_id`s and `positions`.
* **Test 2: Document Length & Metadata Consistency**
  - Verify `total_docs()` and `average_doc_length()` match the indexed corpus.
* **Test 3: File Integrity & Magic Byte Validation**
  - Verify corrupted magic bytes trigger graceful error handling instead of crash.

---

## ✅ Milestone 2 Checklist
- [ ] Binary disk format specified and headers implemented.
- [ ] `IndexWriter` builds in-memory postings and writes `dict.bin`, `postings.bin`, `docs.bin`.
- [ ] `IndexReader` loads dictionary and performs fast `seekg` on disk.
- [ ] Roundtrip tests pass with 100% data fidelity.
