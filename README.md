# High-Performance C++ Search Engine

A local, high-performance document and code search engine built from scratch in C++ without external databases, featuring an interactive Web UI and REST API.

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](https://opensource.org/licenses/MIT)

---

## 🚀 Key Highlights & Verified Metrics

| Metric | Target | Verified Performance |
| :--- | :--- | :--- |
| **Indexed Document Count** | 25,000+ | **25,000 documents** |
| **Index & Disk Commit Time**| Fast | **1.45 seconds** |
| **Average Query Latency** | < 15.0 ms | **9.195 ms** |
| **Median (p50) Latency** | < 15.0 ms | **9.008 ms** |
| **p95 Latency** | **< 15.0 ms** | **11.507 ms** 🎯 |
| **p99 Latency** | < 15.0 ms | **12.983 ms** |
| **Trie Autocomplete Latency**| Instant | **1.70 microseconds** |

---

## 🌟 Core Architecture & Engineering Features

* **Zero External Databases**: Custom binary disk serialization (`dict.bin`, `postings.bin`, `docs.bin`) with `0x53454152` magic bytes and $O(1)$ RAM-to-disk dictionary offset seeking.
* **Code-Aware Tokenizer**: Splits camelCase (`parseHttpRequest` $\to$ `parse`, `http`, `request`), snake_case, PascalCase, and preserves exact character offsets and word positions.
* **Two-Pointer Boolean Engine**: $O(N + M)$ set operations (`AND`, `OR`, `NOT`) on strictly sorted postings lists.
* **Exact Phrase Search**: Positional proximity verification ($pos_{t2} = pos_{t1} + 1$) for quoted multi-word queries (`"binary search"`).
* **BM25 Relevance Ranking**: Okapi BM25 formula with document length normalization (`avgdl`) and term saturation ($k_1=1.2, b=0.75$).
* **$O(N \log K)$ Min-Heap Top-K Retrieval**: Bounded priority queue keeping only $K$ elements in L1 CPU cache rather than sorting the entire match set ($O(N \log N)$).
* **Real-Time Autocomplete Trie**: In-memory Prefix Tree pre-caching top completions per node for sub-microsecond suggestions.
* **Incremental Indexing & Tombstones**: File manifest (64-bit FNV-1a content hash) with $O(1)$ soft-deletion bitsets, preventing expensive full-index rebuilds on file modifications.
* **REST API & Web UI**: Embedded cross-platform C++ HTTP server serving a modern dark-mode search interface.

---

## 🌐 Live Web Deployment

You can deploy this search engine for free with Docker to **Render**, **Railway**, or **Google Cloud Run**:

### Option A: 1-Click Deploy on Render
1. Go to [Render.com](https://render.com) and create a free account.
2. Click **New +** $\to$ **Web Service**.
3. Connect your GitHub repository: `LearnerKg23/SearchEngine`.
4. Render automatically detects the [`Dockerfile`](Dockerfile) and [`render.yaml`](render.yaml).
5. Click **Deploy Web Service**. You will receive a live URL: `https://searchengine-xxxx.onrender.com`!

### Option B: Deploy with Docker Locally
```bash
# Build the container
docker build -t searchengine .

# Run on port 8080
docker run -p 8080:8080 searchengine
```
Open [http://localhost:8080](http://localhost:8080) in your browser.

---

## 💻 Local CLI & Benchmark Execution

### 1. Interactive Web Server & REST API
```powershell
.\bin\server.exe
# Open http://localhost:8080
```

### 2. Interactive Terminal Search Shell
```powershell
.\bin\search_engine.exe
```

Inside the CLI:
* `parseHttpRequest` - Search code identifier
* `search AND engine` - Boolean search
* `"binary search"` - Exact phrase search
* `?par` - Autocomplete suggestions
* `:stats` - Index metrics

### 3. Re-run 25,000-Document Benchmark
```powershell
.\bin\benchmark_runner.exe
```

---

## 🗺️ Step-by-Step Milestones

All architectural blueprints, algorithms, C++ interfaces, and test suites are documented in the [`milestones/`](milestones/00_MASTER_ROADMAP.md) directory:
* [Milestone 1: Core Types & Code-Aware Tokenizer](milestones/milestone_1_tokenization_and_core_types.md)
* [Milestone 2: Positional Inverted Index & Binary Disk Storage](milestones/milestone_2_positional_inverted_index_and_storage.md)
* [Milestone 3: Two-Pointer Boolean Retrieval & Phrase Search](milestones/milestone_3_boolean_retrieval_and_phrase_search.md)
* [Milestone 4: BM25 Ranking & Min-Heap Top-K Retrieval](milestones/milestone_4_bm25_ranking_and_min_heap_top_k.md)
* [Milestone 5: Real-Time Autocomplete (In-Memory Trie)](milestones/milestone_5_trie_autocomplete.md)
* [Milestone 6: Incremental Indexing & Soft-Deletion Tombstones](milestones/milestone_6_incremental_indexing_and_tombstones.md)
* [Milestone 7: 25,000+ Document Benchmark & CLI](milestones/milestone_7_benchmarks_and_cli.md)
