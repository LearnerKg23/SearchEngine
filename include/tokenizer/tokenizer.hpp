#pragma once
#include "common/types.hpp"
#include <string>
#include <vector>
#include <unordered_set>

class Tokenizer {
public:
    explicit Tokenizer(bool filter_stop_words = false);

    // Tokenizes a document text into tokens with position and character offsets
    std::vector<Token> tokenize(const std::string& content) const;

    // Tokenizes search query terms while preserving boolean keywords
    std::vector<std::string> tokenize_query(const std::string& query) const;

    // Splits camelCase, PascalCase, and snake_case identifiers into sub-tokens
    static std::vector<std::string> split_identifier(const std::string& identifier);

    // Normalizes text to lowercase ASCII
    static std::string normalize(const std::string& raw);

    // Returns true if a string is alphanumeric
    static bool is_alphanumeric_or_symbol(char c);

private:
    bool filter_stop_words_;
    std::unordered_set<std::string> stop_words_;
    std::unordered_set<std::string> definition_keywords_;

    void init_stop_words();
    void init_definition_keywords();
};
