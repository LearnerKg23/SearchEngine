#include "tokenizer/tokenizer.hpp"
#include <iostream>
#include <cassert>
#include <vector>
#include <string>

// Lightweight test assertion utility
#define TEST_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            std::cerr << "\033[1;31m[FAIL] " << msg << " (" << __FILE__ << ":" << __LINE__ << ")\033[0m\n"; \
            return false; \
        } \
    } while (0)

bool test_natural_language() {
    Tokenizer tokenizer(false);
    std::string text = "The quick brown fox jumps over 123 lazy dogs.";
    auto tokens = tokenizer.tokenize(text);

    TEST_ASSERT(!tokens.empty(), "Tokens should not be empty");
    TEST_ASSERT(tokens[0].text == "the", "First token should be 'the'");
    TEST_ASSERT(tokens[1].text == "quick", "Second token should be 'quick'");
    TEST_ASSERT(tokens[6].text == "123", "Seventh token should be '123'");
    TEST_ASSERT(tokens[7].text == "lazy", "Eighth token should be 'lazy'");
    TEST_ASSERT(tokens[8].text == "dogs", "Ninth token should be 'dogs'");

    // Check positions are sequential
    for (size_t i = 1; i < tokens.size(); ++i) {
        TEST_ASSERT(tokens[i].position == tokens[i - 1].position + 1, "Positions should be sequential");
    }

    std::cout << "\033[1;32m[PASS]\033[0m test_natural_language\n";
    return true;
}

bool test_camel_case_splitting() {
    auto parts1 = Tokenizer::split_identifier("parseHttpRequest");
    TEST_ASSERT(parts1.size() == 3, "parseHttpRequest should split into 3 parts");
    TEST_ASSERT(parts1[0] == "parse", "parts[0] should be parse");
    TEST_ASSERT(parts1[1] == "http", "parts[1] should be http");
    TEST_ASSERT(parts1[2] == "request", "parts[2] should be request");

    auto parts2 = Tokenizer::split_identifier("XMLHttpRequest");
    TEST_ASSERT(parts2.size() == 3, "XMLHttpRequest should split into 3 parts");
    TEST_ASSERT(parts2[0] == "xml", "parts[0] should be xml");
    TEST_ASSERT(parts2[1] == "http", "parts[1] should be http");
    TEST_ASSERT(parts2[2] == "request", "parts[2] should be request");

    Tokenizer tokenizer;
    auto tokens = tokenizer.tokenize("parseHttpRequest");
    // Should contain the full identifier and all sub-parts
    bool has_full = false, has_parse = false, has_http = false, has_request = false;
    for (const auto& t : tokens) {
        if (t.text == "parsehttprequest") has_full = true;
        if (t.text == "parse") has_parse = true;
        if (t.text == "http") has_http = true;
        if (t.text == "request") has_request = true;
    }
    TEST_ASSERT(has_full && has_parse && has_http && has_request, "Tokenizer should emit full token and sub-parts");

    std::cout << "\033[1;32m[PASS]\033[0m test_camel_case_splitting\n";
    return true;
}

bool test_snake_case_splitting() {
    auto parts = Tokenizer::split_identifier("calculate_bm25_score");
    TEST_ASSERT(parts.size() == 3, "calculate_bm25_score should split into 3 parts");
    TEST_ASSERT(parts[0] == "calculate", "parts[0] should be calculate");
    TEST_ASSERT(parts[1] == "bm25", "parts[1] should be bm25");
    TEST_ASSERT(parts[2] == "score", "parts[2] should be score");

    auto scream = Tokenizer::split_identifier("MAX_BUFFER_SIZE");
    TEST_ASSERT(scream.size() == 3, "MAX_BUFFER_SIZE should split into 3 parts");
    TEST_ASSERT(scream[0] == "max", "parts[0] should be max");
    TEST_ASSERT(scream[1] == "buffer", "parts[1] should be buffer");
    TEST_ASSERT(scream[2] == "size", "parts[2] should be size");

    std::cout << "\033[1;32m[PASS]\033[0m test_snake_case_splitting\n";
    return true;
}

bool test_code_definitions() {
    Tokenizer tokenizer;
    std::string code = "class SearchEngine { void executeQuery(); };";
    auto tokens = tokenizer.tokenize(code);

    bool search_engine_is_def = false;
    bool execute_query_is_def = false;

    for (const auto& t : tokens) {
        if (t.text == "searchengine" && t.is_code_definition) search_engine_is_def = true;
        if (t.text == "executequery" && t.is_code_definition) execute_query_is_def = true;
    }

    TEST_ASSERT(search_engine_is_def, "SearchEngine should be tagged as a code definition");
    TEST_ASSERT(execute_query_is_def, "executeQuery should be tagged as a code definition");

    std::cout << "\033[1;32m[PASS]\033[0m test_code_definitions\n";
    return true;
}

bool test_character_offsets() {
    Tokenizer tokenizer;
    std::string text = "int count = 42; // counter";
    auto tokens = tokenizer.tokenize(text);

    for (const auto& t : tokens) {
        TEST_ASSERT(t.char_offset < text.size(), "Offset must be within bounds");
        // Verify offset points to non-whitespace
        TEST_ASSERT(!std::isspace(static_cast<unsigned char>(text[t.char_offset])), "Offset must start at word");
    }

    std::cout << "\033[1;32m[PASS]\033[0m test_character_offsets\n";
    return true;
}

bool test_query_tokenization() {
    Tokenizer tokenizer;
    std::string query = "searchEngine AND \"binary search\" OR NOT vector";
    auto terms = tokenizer.tokenize_query(query);

    TEST_ASSERT(terms.size() == 6, "Query should tokenize into 6 elements");
    TEST_ASSERT(terms[0] == "searchengine", "terms[0] should be 'searchengine'");
    TEST_ASSERT(terms[1] == "AND", "terms[1] should be 'AND'");
    TEST_ASSERT(terms[2] == "\"binary search\"", "terms[2] should be '\"binary search\"'");
    TEST_ASSERT(terms[3] == "OR", "terms[3] should be 'OR'");
    TEST_ASSERT(terms[4] == "NOT", "terms[4] should be 'NOT'");
    TEST_ASSERT(terms[5] == "vector", "terms[5] should be 'vector'");

    std::cout << "\033[1;32m[PASS]\033[0m test_query_tokenization\n";
    return true;
}

int main() {
    std::cout << "\n=== Running Milestone 1: Tokenizer Tests ===\n";
    bool success = true;
    success &= test_natural_language();
    success &= test_camel_case_splitting();
    success &= test_snake_case_splitting();
    success &= test_code_definitions();
    success &= test_character_offsets();
    success &= test_query_tokenization();

    if (success) {
        std::cout << "\n\033[1;32m>>> ALL MILESTONE 1 TESTS PASSED! <<<\033[0m\n\n";
        return 0;
    } else {
        std::cerr << "\n\033[1;31m>>> SOME MILESTONE 1 TESTS FAILED! <<<\033[0m\n\n";
        return 1;
    }
}
