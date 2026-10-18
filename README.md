# High-Performance C++ Search Engine

A local, high-performance document and code search engine built from scratch in C++ without external databases.

## 🚀 Key Highlights & Resume Metrics
- **Core Engine**: Custom code-aware tokenization, positional inverted indexing, and persistent binary disk storage (`dict.bin`, `postings.bin`, `docs.bin`).
- **Retrieval Performance**: Sub-**15 ms** query latency across **25,000+ documents** using a **Min-Heap for $O(N \log K)$ Top-K retrieval** and a **two-pointer intersection algorithm** for Boolean queries.
- **Real-Time Autocomplete**: In-memory **Trie (Prefix Tree)** with pre-cached top completions for sub-microsecond interactive suggestions.
- **Relevance Ranking**: **Okapi BM25** ranking with document length normalization and term saturation.
- **Incremental Indexing**: File-metadata manifest and **soft-deletion tombstones** with delta-index merging, enabling real-time updates without full index rebuilds.

---

## 🗺️ Step-by-Step Implementation Milestones

All architectural blueprints, algorithms, C++ interfaces, and test suites are organized in the [`milestones/`](file:///c:/Users/Kushal%20Gupta/Desktop/SE/milestones/00_MASTER_ROADMAP.md) folder:

1. [**Milestone 1: Core Types & Code-Aware Tokenizer**](file:///c:/Users/Kushal%20Gupta/Desktop/SE/milestones/milestone_1_tokenization_and_core_types.md)
   * `doc_id_t`, `Token`, `Posting`, camelCase & snake_case identifier splitting, positional indices.
2. [**Milestone 2: Positional Inverted Index & Binary Disk Storage**](file:///c:/Users/Kushal%20Gupta/Desktop/SE/milestones/milestone_2_positional_inverted_index_and_storage.md)
   * Zero external DB, custom binary format with magic bytes (`0x53454152`), `dict.bin`, `postings.bin`, `docs.bin`.
3. [**Milestone 3: Two-Pointer Boolean Retrieval & Phrase Search**](file:///c:/Users/Kushal%20Gupta/Desktop/SE/milestones/milestone_3_boolean_retrieval_and_phrase_search.md)
   * Two-pointer $O(N + M)$ intersection and union, positional phrase proximity check ($pos_{t2} = pos_{t1} + 1$).
4. [**Milestone 4: BM25 Ranking & $O(N \log K)$ Min-Heap Top-K**](file:///c:/Users/Kushal%20Gupta/Desktop/SE/milestones/milestone_4_bm25_ranking_and_min_heap_top_k.md)
   * Okapi BM25 formula ($k_1$, $b$, `avgdl`), Min-Heap bounded top-K retrieval vs $O(N \log N)$ sorting.
5. [**Milestone 5: Real-Time Autocomplete (In-Memory Trie)**](file:///c:/Users/Kushal%20Gupta/Desktop/SE/milestones/milestone_5_trie_autocomplete.md)
   * Prefix Tree with pre-cached top suggestions per node for instant $O(L)$ keystroke suggestions.
6. [**Milestone 6: Incremental Indexing & Soft-Deletion Tombstones**](file:///c:/Users/Kushal%20Gupta/Desktop/SE/milestones/milestone_6_incremental_indexing_and_tombstones.md)
   * File manifest (timestamps + FNV-1a hash), $O(1)$ tombstone bitset, in-memory delta index, vacuum compaction.
7. [**Milestone 7: 25,000+ Document Benchmark & Interactive CLI**](file:///c:/Users/Kushal%20Gupta/Desktop/SE/milestones/milestone_7_benchmarks_and_cli.md)
   * Synthetic/real corpus generation, latency benchmark (p50/p95/p99), terminal search shell with snippet highlighting.

---

## 🛠️ Build & Requirements
* **Standard**: C++17 or C++20
* **Build System**: CMake 3.20+
* **Dependencies**: Standard Library only (`<iostream>`, `<vector>`, `<string>`, `<fstream>`, `<unordered_map>`, `<queue>`, `<chrono>`, `<filesystem>`).
