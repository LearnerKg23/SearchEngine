#include "query/boolean_retriever.hpp"
#include "query/phrase_matcher.hpp"
#include "query/query_parser.hpp"
#include "index/index_writer.hpp"
#include "index/index_reader.hpp"
#include "tokenizer/tokenizer.hpp"
#include <iostream>
#include <cassert>
#include <vector>

#define TEST_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            std::cerr << "\033[1;31m[FAIL] " << msg << " (" << __FILE__ << ":" << __LINE__ << ")\033[0m\n"; \
            return false; \
        } \
    } while (0)

bool test_boolean_retriever_primitives() {
    std::vector<Posting> list_a = { Posting(2), Posting(5), Posting(9), Posting(14), Posting(20) };
    std::vector<Posting> list_b = { Posting(3), Posting(5), Posting(8), Posting(14), Posting(22) };

    // 1. Intersection: should be [5, 14]
    auto inter = BooleanRetriever::intersect(list_a, list_b);
    TEST_ASSERT(inter.size() == 2, "Intersection size should be 2");
    TEST_ASSERT(inter[0].doc_id == 5, "First intersection match should be doc 5");
    TEST_ASSERT(inter[1].doc_id == 14, "Second intersection match should be doc 14");

    // 2. Union: should be [2, 3, 5, 8, 9, 14, 20, 22]
    auto uni = BooleanRetriever::merge_union(list_a, list_b);
    TEST_ASSERT(uni.size() == 8, "Union size should be 8");
    TEST_ASSERT(uni[0].doc_id == 2 && uni[1].doc_id == 3 && uni[2].doc_id == 5, "Union order invalid");

    // 3. Difference: list_a NOT list_b -> [2, 9, 20]
    auto diff = BooleanRetriever::difference(list_a, list_b);
    TEST_ASSERT(diff.size() == 3, "Difference size should be 3");
    TEST_ASSERT(diff[0].doc_id == 2, "diff[0] should be 2");
    TEST_ASSERT(diff[1].doc_id == 9, "diff[1] should be 9");
    TEST_ASSERT(diff[2].doc_id == 20, "diff[2] should be 20");

    std::cout << "\033[1;32m[PASS]\033[0m test_boolean_retriever_primitives\n";
    return true;
}

bool test_phrase_matching_logic() {
    // Doc 1: "binary search" at positions [3, 4] -> consecutive match
    Posting p_bin1(1, 1, {3});
    Posting p_sear1(1, 1, {4});

    // Doc 2: "search is binary" -> "binary" at pos 5, "search" at pos 2 -> non-consecutive
    Posting p_bin2(2, 1, {5});
    Posting p_sear2(2, 1, {2});

    std::vector<Posting> bin_postings = { p_bin1, p_bin2 };
    std::vector<Posting> sear_postings = { p_sear1, p_sear2 };

    auto phrase_matches = PhraseMatcher::match_pair(bin_postings, sear_postings);
    TEST_ASSERT(phrase_matches.size() == 1, "Only Doc 1 should match consecutive phrase");
    TEST_ASSERT(phrase_matches[0].doc_id == 1, "Matched doc_id should be 1");

    std::cout << "\033[1;32m[PASS]\033[0m test_phrase_matching_logic\n";
    return true;
}

bool test_full_query_execution() {
    std::string test_dir = "bin/.test_bool_index";
    Tokenizer tokenizer;

    // Doc 0: "high performance search engine"
    auto t0 = tokenizer.tokenize("high performance search engine");
    // Doc 1: "distributed database and search query"
    auto t1 = tokenizer.tokenize("distributed database and search query");
    // Doc 2: "high performance database engine"
    auto t2 = tokenizer.tokenize("high performance database engine");

    {
        IndexWriter writer(test_dir);
        writer.add_document(0, "doc0.txt", 1000, t0);
        writer.add_document(1, "doc1.txt", 2000, t1);
        writer.add_document(2, "doc2.txt", 3000, t2);
        writer.commit();
    }

    IndexReader reader(test_dir);
    TEST_ASSERT(reader.open(), "Reader open failed");
    QueryParser parser(reader);

    // 1. AND Query: "search AND database" -> only Doc 1
    auto res_and = parser.execute("search AND database");
    TEST_ASSERT(res_and.size() == 1, "search AND database must match 1 doc");
    TEST_ASSERT(res_and[0].doc_id == 1, "Match must be doc 1");

    // 2. OR Query: "engine OR query" -> Doc 0, Doc 1, Doc 2
    auto res_or = parser.execute("engine OR query");
    TEST_ASSERT(res_or.size() == 3, "engine OR query must match all 3 docs");

    // 3. NOT Query: "engine NOT database" -> Doc 0 has engine and no database
    auto res_not = parser.execute("engine NOT database");
    TEST_ASSERT(res_not.size() == 1, "engine NOT database must match 1 doc");
    TEST_ASSERT(res_not[0].doc_id == 0, "Match must be doc 0");

    // 4. Exact Phrase Query: "\"high performance\"" -> Doc 0 and Doc 2
    auto res_phrase = parser.execute("\"high performance\"");
    TEST_ASSERT(res_phrase.size() == 2, "\"high performance\" must match doc 0 and doc 2");
    TEST_ASSERT(res_phrase[0].doc_id == 0 && res_phrase[1].doc_id == 2, "Phrase matches mismatch");

    std::cout << "\033[1;32m[PASS]\033[0m test_full_query_execution\n";
    return true;
}

int main() {
    std::cout << "\n=== Running Milestone 3: Boolean Retrieval & Phrase Search Tests ===\n";
    bool success = true;
    success &= test_boolean_retriever_primitives();
    success &= test_phrase_matching_logic();
    success &= test_full_query_execution();

    if (success) {
        std::cout << "\n\033[1;32m>>> ALL MILESTONE 3 TESTS PASSED! <<<\033[0m\n\n";
        return 0;
    } else {
        std::cerr << "\n\033[1;31m>>> MILESTONE 3 TESTS FAILED! <<<\033[0m\n\n";
        return 1;
    }
}
