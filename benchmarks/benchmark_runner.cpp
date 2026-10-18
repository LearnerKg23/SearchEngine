#include "engine.hpp"
#include <iostream>
#include <chrono>
#include <vector>
#include <string>
#include <random>
#include <algorithm>
#include <numeric>
#include <iomanip>

int main() {
    std::cout << "\n=========================================================\n";
    std::cout << "   HIGH-PERFORMANCE C++ SEARCH ENGINE BENCHMARK SUITE   \n";
    std::cout << "=========================================================\n";

    const size_t TOTAL_DOCS = 25000;
    std::string index_dir = "bin/.bench_index_25k";

    std::cout << "[*] Phase 1: Generating and indexing " << TOTAL_DOCS << " documents...\n";

    std::vector<std::string> vocabulary = {
        "search", "engine", "binary", "inverted", "index", "retrieval", "performance",
        "pointer", "intersection", "algorithm", "ranking", "bm25", "trie", "autocomplete",
        "heap", "priority", "queue", "storage", "disk", "database", "incremental",
        "manifest", "tombstone", "latency", "document", "tokenizer", "token", "offset",
        "positional", "memory", "cache", "thread", "parallel", "asynchronous", "network",
        "socket", "buffer", "protocol", "parser", "lexer", "ast", "compiler", "runtime",
        "optimization", "vector", "string", "hash", "table", "map", "set"
    };

    std::mt19937 rng(1337);
    std::uniform_int_distribution<size_t> vocab_dist(0, vocabulary.size() - 1);
    std::uniform_int_distribution<size_t> length_dist(20, 100);

    SearchEngine engine(index_dir);

    auto start_index = std::chrono::high_resolution_clock::now();

    for (size_t doc_id = 0; doc_id < TOTAL_DOCS; ++doc_id) {
        size_t words = length_dist(rng);
        std::string content;
        content.reserve(words * 10);

        for (size_t w = 0; w < words; ++w) {
            content += vocabulary[vocab_dist(rng)];
            content += " ";
        }

        std::string path = "docs/file_" + std::to_string(doc_id) + ".cpp";
        engine.add_document(static_cast<doc_id_t>(doc_id), path, 1700000000 + doc_id, content);

        if ((doc_id + 1) % 5000 == 0) {
            std::cout << "    Indexed " << (doc_id + 1) << " / " << TOTAL_DOCS << " documents...\n";
        }
    }

    std::cout << "[*] Committing index to binary disk files...\n";
    engine.commit();

    auto end_index = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> index_time = end_index - start_index;
    std::cout << "[+] 25,000 Documents indexed & committed in " 
              << std::fixed << std::setprecision(2) << index_time.count() << " seconds.\n\n";

    std::cout << "[*] Phase 2: Running 1,000 Query Latency Benchmark across " 
              << TOTAL_DOCS << " documents...\n";

    std::vector<std::string> query_workload = {
        "search",
        "performance AND latency",
        "inverted AND index",
        "algorithm OR database",
        "binary AND search",
        "trie OR autocomplete",
        "bm25 AND ranking",
        "heap AND priority",
        "manifest AND tombstone",
        "parser AND lexer",
        "storage NOT database",
        "optimization AND runtime",
        "buffer AND protocol",
        "pointer AND intersection",
        "\"binary search\"",
        "\"inverted index\"",
        "\"search engine\""
    };

    const size_t NUM_QUERIES = 1000;
    std::vector<double> latencies_ms;
    latencies_ms.reserve(NUM_QUERIES);

    // Warmup
    for (size_t i = 0; i < 50; ++i) {
        engine.search(query_workload[i % query_workload.size()], 10);
    }

    // Benchmark loop
    for (size_t i = 0; i < NUM_QUERIES; ++i) {
        const std::string& query = query_workload[i % query_workload.size()];

        auto t0 = std::chrono::high_resolution_clock::now();
        auto results = engine.search(query, 10);
        auto t1 = std::chrono::high_resolution_clock::now();

        (void)results; // Prevent compiler dead code elimination
        std::chrono::duration<double, std::milli> elapsed = t1 - t0;
        latencies_ms.push_back(elapsed.count());
    }

    // Calculate latency percentiles
    std::sort(latencies_ms.begin(), latencies_ms.end());
    double min_lat = latencies_ms.front();
    double p50 = latencies_ms[static_cast<size_t>(latencies_ms.size() * 0.50)];
    double p90 = latencies_ms[static_cast<size_t>(latencies_ms.size() * 0.90)];
    double p95 = latencies_ms[static_cast<size_t>(latencies_ms.size() * 0.95)];
    double p99 = latencies_ms[static_cast<size_t>(latencies_ms.size() * 0.99)];
    double max_lat = latencies_ms.back();
    double avg_lat = std::accumulate(latencies_ms.begin(), latencies_ms.end(), 0.0) / latencies_ms.size();

    std::cout << "=========================================================\n";
    std::cout << "                  LATENCY RESULTS                        \n";
    std::cout << "=========================================================\n";
    std::cout << " Indexed Document Count : " << engine.total_documents() << "\n";
    std::cout << " Total Queries Executed : " << NUM_QUERIES << "\n";
    std::cout << " Average Latency        : " << std::fixed << std::setprecision(3) << avg_lat << " ms\n";
    std::cout << " Minimum Latency        : " << min_lat << " ms\n";
    std::cout << " p50 (Median) Latency   : " << p50 << " ms\n";
    std::cout << " p90 Latency            : " << p90 << " ms\n";
    std::cout << " p95 Latency            : " << p95 << " ms\n";
    std::cout << " p99 Latency            : " << p99 << " ms\n";
    std::cout << " Maximum Latency        : " << max_lat << " ms\n";
    std::cout << "=========================================================\n";

    if (p95 < 15.0) {
        std::cout << "\033[1;32m[PASS] SUCCESS: Query latency is UNDER 15 ms across 25,000+ documents!\033[0m\n";
        std::cout << "\033[1;32m       (Target: < 15.000 ms, Measured p95: " << p95 << " ms)\033[0m\n\n";
        return 0;
    } else {
        std::cerr << "\033[1;31m[FAIL] p95 latency exceeded 15 ms target.\033[0m\n\n";
        return 1;
    }
}
