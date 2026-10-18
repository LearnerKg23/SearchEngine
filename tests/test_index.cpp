#include "index/index_writer.hpp"
#include "index/index_reader.hpp"
#include "tokenizer/tokenizer.hpp"
#include <iostream>
#include <cassert>
#include <vector>
#include <string>

#define TEST_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            std::cerr << "\033[1;31m[FAIL] " << msg << " (" << __FILE__ << ":" << __LINE__ << ")\033[0m\n"; \
            return false; \
        } \
    } while (0)

bool test_index_write_and_read() {
    std::string test_dir = "bin/.test_search_index";
    Tokenizer tokenizer;

    // Doc 0
    std::string doc0_text = "High performance search engine written in C++";
    auto tokens0 = tokenizer.tokenize(doc0_text);

    // Doc 1
    std::string doc1_text = "Binary disk storage and positional inverted index";
    auto tokens1 = tokenizer.tokenize(doc1_text);

    // Doc 2
    std::string doc2_text = "Search engine query latency under 15 ms";
    auto tokens2 = tokenizer.tokenize(doc2_text);

    // Write index
    {
        IndexWriter writer(test_dir);
        writer.add_document(0, "doc0.txt", 1000, tokens0);
        writer.add_document(1, "doc1.txt", 2000, tokens1);
        writer.add_document(2, "doc2.txt", 3000, tokens2);

        TEST_ASSERT(writer.pending_documents() == 3, "Pending documents should be 3");
        TEST_ASSERT(writer.commit(), "Commit must succeed");
    }

    // Read index
    {
        IndexReader reader(test_dir);
        TEST_ASSERT(reader.open(), "IndexReader::open must succeed");

        TEST_ASSERT(reader.total_docs() == 3, "Total docs must be 3");
        TEST_ASSERT(reader.average_doc_length() > 0.0, "Average doc length must be > 0");
        TEST_ASSERT(reader.get_document_path(0) == "doc0.txt", "Doc 0 path mismatch");
        TEST_ASSERT(reader.get_document_path(1) == "doc1.txt", "Doc 1 path mismatch");
        TEST_ASSERT(reader.get_document_path(2) == "doc2.txt", "Doc 2 path mismatch");

        // Term present in Doc 0 and Doc 2: "engine" / "search"
        TEST_ASSERT(reader.has_term("engine"), "Dictionary must contain 'engine'");
        TEST_ASSERT(reader.get_doc_frequency("engine") == 2, "'engine' doc frequency must be 2");

        auto engine_postings = reader.get_postings("engine");
        TEST_ASSERT(engine_postings.size() == 2, "'engine' postings count must be 2");
        TEST_ASSERT(engine_postings[0].doc_id == 0, "'engine' first doc_id must be 0");
        TEST_ASSERT(engine_postings[1].doc_id == 2, "'engine' second doc_id must be 2");

        // Term present only in Doc 1: "positional"
        TEST_ASSERT(reader.has_term("positional"), "Dictionary must contain 'positional'");
        auto pos_postings = reader.get_postings("positional");
        TEST_ASSERT(pos_postings.size() == 1, "'positional' postings count must be 1");
        TEST_ASSERT(pos_postings[0].doc_id == 1, "'positional' doc_id must be 1");
        TEST_ASSERT(!pos_postings[0].positions.empty(), "'positional' positions must not be empty");

        // Non-existent term
        TEST_ASSERT(!reader.has_term("nonexistent"), "Dictionary should not contain 'nonexistent'");
        TEST_ASSERT(reader.get_postings("nonexistent").empty(), "Postings for nonexistent term must be empty");

        // Check seeking multiple terms consecutively in arbitrary order
        auto p1 = reader.get_postings("search");
        auto p2 = reader.get_postings("storage");
        auto p3 = reader.get_postings("search");
        TEST_ASSERT(p1.size() == p3.size(), "Repeat seek must produce identical results");
        TEST_ASSERT(!p2.empty() && p2[0].doc_id == 1, "Storage postings seek must be accurate");
    }

    std::cout << "\033[1;32m[PASS]\033[0m test_index_write_and_read\n";
    return true;
}

int main() {
    std::cout << "\n=== Running Milestone 2: Positional Index & Binary Storage Tests ===\n";
    bool success = test_index_write_and_read();

    if (success) {
        std::cout << "\n\033[1;32m>>> ALL MILESTONE 2 TESTS PASSED! <<<\033[0m\n\n";
        return 0;
    } else {
        std::cerr << "\n\033[1;31m>>> MILESTONE 2 TESTS FAILED! <<<\033[0m\n\n";
        return 1;
    }
}
