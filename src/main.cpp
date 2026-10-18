#include "engine.hpp"
#include <iostream>
#include <chrono>
#include <string>
#include <iomanip>

void print_banner() {
    std::cout << "\033[1;36m";
    std::cout << "=========================================================\n";
    std::cout << "         HIGH-PERFORMANCE C++ SEARCH ENGINE CLI          \n";
    std::cout << "=========================================================\n";
    std::cout << "\033[0m";
    std::cout << "Commands:\n";
    std::cout << "  <query>          - Execute BM25 search (e.g. 'search AND engine', '\"binary search\"')\n";
    std::cout << "  ?<prefix>        - Real-time Trie autocomplete (e.g. '?sea', '?par')\n";
    std::cout << "  :stats           - Display index statistics\n";
    std::cout << "  :exit or :quit   - Exit the search shell\n";
    std::cout << "=========================================================\n\n";
}

int main(int argc, char* argv[]) {
    std::string index_dir = ".search_index";
    if (argc >= 2) {
        index_dir = argv[1];
    }

    print_banner();

    SearchEngine engine(index_dir);

    // If index doesn't exist, create a starter index
    if (!engine.load()) {
        std::cout << "[*] No existing index found at '" << index_dir << "'. Seeding sample corpus...\n";

        engine.add_document(0, "src/network/http_server.cpp", 1700000000,
            "class HttpServer { void parseHttpRequest(const std::string& buffer); }; // High performance socket server");
        engine.add_document(1, "include/network/http_parser.hpp", 1700000001,
            "bool parseHttpRequest(const char* data, size_t length); // Fast HTTP request parsing routine");
        engine.add_document(2, "src/index/inverted_index.cpp", 1700000002,
            "class PositionalInvertedIndex { void insert(uint32_t doc_id, uint32_t position); }; // Binary disk index");
        engine.add_document(3, "src/query/bm25_ranker.cpp", 1700000003,
            "float compute_bm25_score(uint32_t tf, uint32_t doc_length, float idf); // Min-Heap Top-K ranking algorithm");
        engine.add_document(4, "src/autocomplete/trie.cpp", 1700000004,
            "class AutocompleteTrie { std::vector<std::string> get_completions(const std::string& prefix); };");

        engine.commit();
        std::cout << "[+] Sample corpus seeded (" << engine.total_documents() << " documents).\n\n";
    } else {
        std::cout << "[+] Loaded index from '" << index_dir << "' (" 
                  << engine.total_documents() << " documents, " 
                  << engine.vocabulary_size() << " vocabulary terms).\n\n";
    }

    std::string line;
    while (true) {
        std::cout << "\033[1;32mSearchEngine>\033[0m ";
        if (!std::getline(std::cin, line)) {
            break;
        }

        if (line.empty()) continue;
        if (line == ":exit" || line == ":quit") {
            std::cout << "Exiting SearchEngine. Goodbye!\n";
            break;
        }
        if (line == ":stats") {
            std::cout << "Index Directory  : " << index_dir << "\n";
            std::cout << "Total Documents  : " << engine.total_documents() << "\n";
            std::cout << "Vocabulary Size  : " << engine.vocabulary_size() << "\n\n";
            continue;
        }

        // Autocomplete command: '?prefix'
        if (line.front() == '?') {
            std::string prefix = line.substr(1);
            auto suggestions = engine.autocomplete(prefix, 5);
            std::cout << "  \033[1;33mAutocomplete suggestions for '" << prefix << "':\033[0m\n";
            if (suggestions.empty()) {
                std::cout << "    [No completions found]\n\n";
            } else {
                for (size_t i = 0; i < suggestions.size(); ++i) {
                    std::cout << "    " << (i + 1) << ". " << suggestions[i] << "\n";
                }
                std::cout << "\n";
            }
            continue;
        }

        // Execute Search Query
        auto t0 = std::chrono::high_resolution_clock::now();
        auto results = engine.search(line, 5);
        auto t1 = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> elapsed = t1 - t0;

        std::cout << "\nResults for \033[1;37m'" << line << "'\033[0m (" 
                  << results.size() << " hits in " 
                  << std::fixed << std::setprecision(2) << elapsed.count() << " ms):\n";
        std::cout << "----------------------------------------------------------------------\n";

        if (results.empty()) {
            std::cout << "  No matching documents found.\n";
        } else {
            for (size_t i = 0; i < results.size(); ++i) {
                const auto& r = results[i];
                std::cout << "  [" << (i + 1) << "] \033[1;34mScore: " 
                          << std::fixed << std::setprecision(2) << r.score 
                          << "\033[0m | DocID: " << r.doc_id 
                          << " | File: \033[1;32m" << r.path << "\033[0m\n";
                if (!r.snippet.empty()) {
                    std::cout << "      \033[0;90m" << r.snippet << "\033[0m\n";
                }
            }
        }
        std::cout << "----------------------------------------------------------------------\n\n";
    }

    return 0;
}
