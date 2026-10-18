# High-Performance C++ Search Engine: Master Roadmap

This roadmap breaks down the entire project into 7 modular, step-by-step milestones. Each milestone includes exact specifications, algorithms, data structures, and verification tests.

---

## 📑 Milestone Index

| Milestone | Focus Area | Key Concepts & Data Structures |
| :--- | :--- | :--- |
| [**Milestone 1**](./milestone_1_tokenization_and_core_types.md) | **Core Types & Code-Aware Tokenizer** | `Token`, `Posting`, `doc_id_t`, camelCase/snake_case splitting, char offsets |
| [**Milestone 2**](./milestone_2_positional_inverted_index_and_storage.md) | **Positional Inverted Index & Binary Storage** | Custom binary serialization (`dict.bin`, `postings.bin`, `docs.bin`), zero external DB |
| [**Milestone 3**](./milestone_3_boolean_retrieval_and_phrase_search.md) | **Two-Pointer Boolean & Phrase Retrieval** | Two-pointer intersection/union ($O(N+M)$), positional proximity matching |
| [**Milestone 4**](./milestone_4_bm25_ranking_and_min_heap_top_k.md) | **BM25 Ranking & Min-Heap Top-K** | BM25 formula, length normalization, $O(N \log K)$ Min-Heap ranking |
| [**Milestone 5**](./milestone_5_trie_autocomplete.md) | **Real-Time Autocomplete (Trie)** | In-memory Prefix Tree, cached top completions, sub-millisecond prefix search |
| [**Milestone 6**](./milestone_6_incremental_indexing_and_tombstones.md) | **Incremental Indexing & Tombstones** | File manifest, content hashing, soft-deletion bitsets, delta index merging |
| [**Milestone 7**](./milestone_7_benchmarks_and_cli.md) | **25,000+ Doc Benchmark & Interactive CLI** | Sub-15 ms latency validation (p50/p95/p99), terminal search shell |

---

## 🏗️ Target Project File Structure

```
SE/
├── CMakeLists.txt                  # Top-level CMake build configuration
├── milestones/                     # Step-by-step milestone guides (this folder)
│   ├── 00_MASTER_ROADMAP.md
│   ├── milestone_1_tokenization_and_core_types.md
│   ├── milestone_2_positional_inverted_index_and_storage.md
│   ├── milestone_3_boolean_retrieval_and_phrase_search.md
│   ├── milestone_4_bm25_ranking_and_min_heap_top_k.md
│   ├── milestone_5_trie_autocomplete.md
│   ├── milestone_6_incremental_indexing_and_tombstones.md
│   └── milestone_7_benchmarks_and_cli.md
├── include/                        # Header files
│   ├── common/
│   │   ├── types.hpp               # DocId, Token, Posting, ScoredDoc
│   │   └── config.hpp              # Engine constants (k1, b, max_k)
│   ├── tokenizer/
│   │   └── tokenizer.hpp           # Lexer & code-aware tokenizer
│   ├── index/
│   │   ├── posting_list.hpp        # In-memory postings list
│   │   ├── index_writer.hpp        # Binary serializer to disk
│   │   ├── index_reader.hpp        # Binary deserializer & disk seeker
│   │   └── manifest.hpp            # File metadata manifest & tombstones
│   ├── query/
│   │   ├── query_parser.hpp        # AST parser for boolean & phrase queries
│   │   ├── boolean_retriever.hpp   # Two-pointer intersection & union
│   │   ├── phrase_matcher.hpp      # Positional distance verification
│   │   └── bm25_ranker.hpp         # BM25 scoring & Min-Heap Top-K
│   ├── autocomplete/
│   │   └── trie.hpp                # Prefix tree for suggestions
│   └── engine.hpp                  # High-level engine interface
├── src/                            # Source implementations
│   ├── tokenizer.cpp
│   ├── index_writer.cpp
│   ├── index_reader.cpp
│   ├── manifest.cpp
│   ├── query_parser.cpp
│   ├── boolean_retriever.cpp
│   ├── phrase_matcher.cpp
│   ├── bm25_ranker.cpp
│   ├── trie.cpp
│   ├── engine.cpp
│   └── main.cpp                    # Interactive CLI entrypoint
├── tests/                          # Unit tests
│   ├── test_tokenizer.cpp
│   ├── test_index.cpp
│   ├── test_boolean.cpp
│   ├── test_bm25.cpp
│   ├── test_trie.cpp
│   └── test_incremental.cpp
└── benchmarks/                     # Performance verification suite
    ├── corpus_generator.cpp        # Generates or imports 25,000+ files
    └── benchmark_runner.cpp        # Latency & throughput benchmarker
```

---

## ⚙️ Prerequisites & Tooling

* **Language Standard**: C++17 or C++20
* **Build System**: CMake 3.20+
* **Compiler**:
  * Windows: Visual Studio 2022 (MSVC `cl.exe`) or Clang / GCC (MinGW-w64)
  * Linux/macOS: GCC 10+ or Clang 12+
* **Dependencies**:
  * Pure standard library (`<iostream>`, `<vector>`, `<string>`, `<fstream>`, `<unordered_map>`, `<queue>`, `<chrono>`, `<filesystem>`).
  * **No external database or search engine libraries** (no SQLite, RocksDB, Lucene, or Elasticsearch).
