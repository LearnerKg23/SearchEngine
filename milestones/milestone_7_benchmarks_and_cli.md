# Milestone 7: 25,000+ Document Benchmark & Interactive CLI

> **Resume Alignment**: *"Optimized query latency to under 15 ms across 25,000+ documents using a Min-Heap for O(N log K) Top-K retrieval... and implemented the BM25 ranking algorithm..."*

---

## 🎯 Objectives
1. Build an automated **Corpus Generator** capable of ingesting or generating 25,000+ realistic code and text documents.
2. Build an automated **Benchmark Suite** that records cold and warm latencies, calculating **p50, p95, and p99 percentiles** to prove the **< 15 ms latency** claim.
3. Build a terminal-based **Interactive Search Shell** with real-time autocomplete suggestions, BM25 score displays, and snippet highlighting.

---

## 1. 25,000+ Document Corpus Generator (`benchmarks/corpus_generator.cpp`)

To reproduce the exact resume metric, we generate a synthetic corpus of 25,000 realistic C++ and Markdown files:

```cpp
// Generates 25,000 documents with realistic distributions of:
// - C++ class and function definitions (camelCase, snake_case)
// - Common keywords, rare terms, and comments
// - Document lengths ranging between 100 and 5,000 words
```

Alternatively, you can point the engine at an existing large open-source codebase (such as the Linux kernel headers or Boost).

---

## 2. Latency Benchmark Suite (`benchmarks/benchmark_runner.cpp`)

The benchmark measures query latency across 1,000 randomized queries using `std::chrono::high_resolution_clock`.

```cpp
#include "engine.hpp"
#include <chrono>
#include <vector>
#include <iostream>
#include <algorithm>
#include <numeric>

void run_latency_benchmark(SearchEngine& engine, const std::vector<std::string>& query_workload) {
    std::vector<double> latencies_ms;
    latencies_ms.reserve(query_workload.size());

    // Warmup phase (100 queries)
    for (size_t i = 0; i < 100 && i < query_workload.size(); ++i) {
        engine.search(query_workload[i], 10);
    }

    // Timed benchmark phase
    for (const auto& q : query_workload) {
        auto start = std::chrono::high_resolution_clock::now();
        
        auto results = engine.search(q, 10);
        
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> duration = end - start;
        latencies_ms.push_back(duration.count());
    }

    // Calculate percentiles
    std::sort(latencies_ms.begin(), latencies_ms.end());
    double p50 = latencies_ms[latencies_ms.size() * 0.50];
    double p95 = latencies_ms[latencies_ms.size() * 0.95];
    double p99 = latencies_ms[latencies_ms.size() * 0.99];
    double max_lat = latencies_ms.back();
    double avg = std::accumulate(latencies_ms.begin(), latencies_ms.end(), 0.0) / latencies_ms.size();

    std::cout << "\n========================================\n";
    std::cout << "      LATENCY BENCHMARK RESULTS         \n";
    std::cout << "========================================\n";
    std::cout << "Total Documents Indexed : " << engine.total_documents() << "\n";
    std::cout << "Total Queries Executed  : " << query_workload.size() << "\n";
    std::cout << "Average Latency         : " << avg << " ms\n";
    std::cout << "p50 (Median) Latency    : " << p50 << " ms\n";
    std::cout << "p95 Latency             : " << p95 << " ms\n";
    std::cout << "p99 Latency             : " << p99 << " ms\n";
    std::cout << "Max Latency             : " << max_lat << " ms\n";
    std::cout << "========================================\n";

    if (p95 < 15.0) {
        std::cout << "[PASSED] Query latency is under 15 ms target!\n";
    } else {
        std::cout << "[WARNING] p95 exceeded 15 ms. Check I/O seeks or heap bounds.\n";
    }
}
```

---

## 3. Interactive CLI Search Shell (`src/main.cpp`)

The CLI offers an engaging experience:
1. **Interactive Autocomplete**: Suggests completions as you type.
2. **Colorized Snippet Highlighting**: Extracts the matching line from the source file and highlights matching terms using ANSI escape sequences.

```
$ ./bin/search_engine --index ./.search_index

High-Performance C++ Search Engine (25,000 docs loaded)
Type a query or ':exit' to quit. Press TAB for autocomplete.

Query: parseHttp
  Suggestions: [ parseHttpRequest, parseHttpHeaders, parseHttpBody ]

Results for 'parseHttpRequest' (Top 5 in 3.42 ms):
----------------------------------------------------------------------
[1] Score: 14.82 | File: src/network/http_parser.cpp:42
    void parseHttpRequest(const std::string& buffer) {
         ^^^^^^^^^^^^^^^^
[2] Score: 11.20 | File: include/network/http_parser.hpp:18
    bool parseHttpRequestHeader(const char* data);
         ^^^^^^^^^^^^^^^^
[3] Score: 8.45  | File: tests/test_http.cpp:89
    TEST(HttpTest, parseHttpRequestSuccess) {
                   ^^^^^^^^^^^^^^^^
----------------------------------------------------------------------
```

---

## 4. Interview Talking Points Cheat Sheet

When discussing this project in interviews:

1. **Why is your retrieval so fast (< 15 ms)?**
   > *"Three reasons: First, dictionary lookups are $O(1)$ in RAM, pointing to pre-computed disk offsets in `postings.bin`. Second, Boolean queries use a cache-friendly Two-Pointer sweep with no dynamic allocations. Third, ranking uses a bounded Min-Heap of size $K$, keeping only 10 elements in L1 cache rather than sorting all 25,000 matches."*

2. **Why didn't you use SQLite or RocksDB?**
   > *"General-purpose databases introduce write-ahead logging, B-Tree fragmentation, and SQL parsing overhead. By implementing a custom binary format with 8-byte headers, contiguous postings lists, and dictionary offsets, we achieved maximum spatial cache locality with zero third-party dependencies."*

3. **How does BM25 improve code search over standard TF-IDF?**
   > *"In source code, identifier repetition is frequent (e.g. iterating over `buffer`). BM25 introduces term frequency saturation via the $k_1$ parameter, so repeating a term 50 times does not inflate relevance over a file having it 4 times. Furthermore, the $b$ parameter penalizes excessively long monolithic files."*

4. **How do tombstones handle real-time file updates?**
   > *"Instead of rewriting gigabytes of binary postings whenever a file changes, we mark its old `doc_id` in a bitset in $O(1)$ time and append the new version to an in-memory Delta Index. Searches query both and filter out tombstoned IDs. When tombstones exceed 15% of the corpus, an automated vacuum compacts the index."*

---

## ✅ Milestone 7 Checklist
- [ ] Synthetic or real 25,000-document corpus indexed.
- [ ] Benchmark runner executes 1,000 queries and calculates p50, p95, p99.
- [ ] Confirmed p95 latency is **< 15 ms**.
- [ ] Interactive CLI demonstrates search, autocomplete, and snippet highlighting.
