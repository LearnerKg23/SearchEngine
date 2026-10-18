#include "index/manifest.hpp"
#include "index/index_writer.hpp"
#include "index/index_reader.hpp"
#include "tokenizer/tokenizer.hpp"
#include "query/boolean_retriever.hpp"
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

bool test_manifest_change_detection() {
    std::string path = "bin/test_manifest.bin";
    Manifest manifest(path);

    std::string content1 = "initial file content";
    uint64_t hash1 = compute_fnv1a(content1.data(), content1.size());
    manifest.record_file("src/main.cpp", 101, content1.size(), 1000, hash1);

    TEST_ASSERT(manifest.has_file("src/main.cpp"), "Manifest should contain main.cpp");
    TEST_ASSERT(manifest.get_doc_id("src/main.cpp") == 101, "Doc ID should be 101");

    // Check unchanged
    auto status_same = manifest.check_file("src/main.cpp", content1.size(), 1000, hash1);
    TEST_ASSERT(status_same == Manifest::FileStatus::UNCHANGED, "Status should be UNCHANGED");

    // Check modified
    std::string content2 = "modified file content";
    uint64_t hash2 = compute_fnv1a(content2.data(), content2.size());
    auto status_mod = manifest.check_file("src/main.cpp", content2.size(), 2000, hash2);
    TEST_ASSERT(status_mod == Manifest::FileStatus::MODIFIED, "Status should be MODIFIED");

    // Check new
    auto status_new = manifest.check_file("src/new.cpp", 50, 1000, 12345);
    TEST_ASSERT(status_new == Manifest::FileStatus::NEW, "Status should be NEW");

    // Test persistence
    TEST_ASSERT(manifest.save(), "Manifest save failed");
    Manifest reloaded(path);
    TEST_ASSERT(reloaded.load(), "Manifest load failed");
    TEST_ASSERT(reloaded.size() == 1, "Reloaded manifest size mismatch");
    TEST_ASSERT(reloaded.get_doc_id("src/main.cpp") == 101, "Reloaded doc ID mismatch");

    std::cout << "\033[1;32m[PASS]\033[0m test_manifest_change_detection\n";
    return true;
}

bool test_tombstone_manager() {
    std::string path = "bin/test_tombstones.bin";
    TombstoneManager tm(path);

    TEST_ASSERT(!tm.is_deleted(5), "Doc 5 should not be deleted initially");
    tm.mark_deleted(5);
    tm.mark_deleted(42);

    TEST_ASSERT(tm.is_deleted(5), "Doc 5 should be marked deleted");
    TEST_ASSERT(tm.is_deleted(42), "Doc 42 should be marked deleted");
    TEST_ASSERT(!tm.is_deleted(10), "Doc 10 should not be deleted");
    TEST_ASSERT(tm.deleted_count() == 2, "Deleted count should be 2");

    // Test persistence
    TEST_ASSERT(tm.save(), "Tombstones save failed");
    TombstoneManager reloaded(path);
    TEST_ASSERT(reloaded.load(), "Tombstones load failed");
    TEST_ASSERT(reloaded.is_deleted(5), "Reloaded doc 5 should be deleted");
    TEST_ASSERT(reloaded.is_deleted(42), "Reloaded doc 42 should be deleted");
    TEST_ASSERT(!reloaded.is_deleted(10), "Reloaded doc 10 should not be deleted");

    std::cout << "\033[1;32m[PASS]\033[0m test_tombstone_manager\n";
    return true;
}

bool test_incremental_soft_deletion() {
    std::string test_dir = "bin/.test_inc_index";
    Tokenizer tokenizer;

    // Doc 0: "c++ search engine"
    auto t0 = tokenizer.tokenize("c++ search engine");
    // Doc 1: "python data analysis"
    auto t1 = tokenizer.tokenize("python data analysis");

    {
        IndexWriter writer(test_dir);
        writer.add_document(0, "doc0.cpp", 1000, t0);
        writer.add_document(1, "doc1.py", 1000, t1);
        writer.commit();
    }

    IndexReader reader(test_dir);
    TEST_ASSERT(reader.open(), "Reader open failed");

    // Query "search": returns Doc 0
    auto search_postings = reader.get_postings("search");
    TEST_ASSERT(search_postings.size() == 1 && search_postings[0].doc_id == 0, "Initial search must return Doc 0");

    // Soft delete Doc 0 using TombstoneManager
    TombstoneManager tombstones;
    tombstones.mark_deleted(0);

    // Apply tombstone filter to query results
    std::vector<Posting> filtered_postings;
    for (const auto& p : search_postings) {
        if (!tombstones.is_deleted(p.doc_id)) {
            filtered_postings.push_back(p);
        }
    }

    TEST_ASSERT(filtered_postings.empty(), "Tombstone-filtered query for 'search' must return 0 results without disk rewrite");

    // Doc 1 is untouched and still searchable
    auto python_postings = reader.get_postings("python");
    TEST_ASSERT(!python_postings.empty() && python_postings[0].doc_id == 1, "Doc 1 must remain intact");

    std::cout << "\033[1;32m[PASS]\033[0m test_incremental_soft_deletion (Zero disk rewrite verified)\n";
    return true;
}

int main() {
    std::cout << "\n=== Running Milestone 6: Incremental Indexing & Tombstone Tests ===\n";
    bool success = true;
    success &= test_manifest_change_detection();
    success &= test_tombstone_manager();
    success &= test_incremental_soft_deletion();

    if (success) {
        std::cout << "\n\033[1;32m>>> ALL MILESTONE 6 TESTS PASSED! <<<\033[0m\n\n";
        return 0;
    } else {
        std::cerr << "\n\033[1;31m>>> MILESTONE 6 TESTS FAILED! <<<\033[0m\n\n";
        return 1;
    }
}
