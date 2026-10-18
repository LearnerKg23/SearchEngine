#include "tokenizer/tokenizer.hpp"
#include <cctype>
#include <sstream>
#include <algorithm>

Tokenizer::Tokenizer(bool filter_stop_words)
    : filter_stop_words_(filter_stop_words) {
    if (filter_stop_words_) {
        init_stop_words();
    }
    init_definition_keywords();
}

void Tokenizer::init_stop_words() {
    stop_words_ = {
        "a", "about", "above", "after", "again", "against", "all", "am", "an", "and",
        "any", "are", "aren't", "as", "at", "be", "because", "been", "before", "being",
        "below", "between", "both", "but", "by", "can't", "cannot", "could", "couldn't",
        "did", "didn't", "do", "does", "doesn't", "doing", "don't", "down", "during",
        "each", "few", "for", "from", "further", "had", "hadn't", "has", "hasn't",
        "have", "haven't", "having", "he", "he'd", "he'll", "he's", "her", "here",
        "here's", "hers", "herself", "him", "himself", "his", "how", "how's", "i",
        "i'd", "i'll", "i'm", "i've", "if", "in", "into", "is", "isn't", "it", "it's",
        "its", "itself", "let's", "me", "more", "most", "mustn't", "my", "myself",
        "no", "nor", "not", "of", "off", "on", "once", "only", "or", "other", "ought",
        "our", "ours", "ourselves", "out", "over", "own", "same", "shan't", "she",
        "she'd", "she'll", "she's", "should", "shouldn't", "so", "some", "such",
        "than", "that", "that's", "the", "their", "theirs", "them", "themselves",
        "then", "there", "there's", "these", "they", "they'd", "they'll", "they're",
        "they've", "this", "those", "through", "to", "too", "under", "until", "up",
        "very", "was", "wasn't", "we", "we'd", "we'll", "we're", "we've", "were",
        "weren't", "what", "what's", "when", "when's", "where", "where's", "which",
        "while", "who", "who's", "whom", "why", "why's", "with", "won't", "would",
        "wouldn't", "you", "you'd", "you'll", "you're", "you've", "your", "yours",
        "yourself", "yourselves"
    };
}

void Tokenizer::init_definition_keywords() {
    definition_keywords_ = {
        "class", "struct", "enum", "union", "interface", "trait", "namespace",
        "def", "fn", "function", "func", "void", "int", "float", "double",
        "char", "bool", "auto", "const", "var", "let", "val", "type"
    };
}

bool Tokenizer::is_alphanumeric_or_symbol(char c) {
    return std::isalnum(static_cast<unsigned char>(c)) || c == '_';
}

std::string Tokenizer::normalize(const std::string& raw) {
    std::string result;
    result.reserve(raw.size());
    for (char c : raw) {
        if (c != '_') {
            result.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
        }
    }
    return result;
}

std::vector<std::string> Tokenizer::split_identifier(const std::string& identifier) {
    std::vector<std::string> parts;
    if (identifier.empty()) return parts;

    std::string current;
    for (size_t i = 0; i < identifier.size(); ++i) {
        char c = identifier[i];
        if (c == '_') {
            if (!current.empty()) {
                std::string norm = normalize(current);
                if (!norm.empty()) parts.push_back(norm);
                current.clear();
            }
            continue;
        }

        // camelCase boundary: lowercase followed by uppercase (e.g. 'a' -> 'B')
        if (i > 0 && std::islower(static_cast<unsigned char>(identifier[i - 1])) &&
            std::isupper(static_cast<unsigned char>(c))) {
            if (!current.empty()) {
                std::string norm = normalize(current);
                if (!norm.empty()) parts.push_back(norm);
                current.clear();
            }
        }
        // Acronym boundary: e.g. 'XMLHttp' -> 'XML' and 'Http'
        else if (i + 1 < identifier.size() &&
                 std::isupper(static_cast<unsigned char>(c)) &&
                 std::isupper(static_cast<unsigned char>(identifier[i - 1])) &&
                 std::islower(static_cast<unsigned char>(identifier[i + 1]))) {
            if (!current.empty()) {
                std::string norm = normalize(current);
                if (!norm.empty()) parts.push_back(norm);
                current.clear();
            }
        }

        current += c;
    }

    if (!current.empty()) {
        std::string norm = normalize(current);
        if (!norm.empty()) parts.push_back(norm);
    }
    return parts;
}

std::vector<Token> Tokenizer::tokenize(const std::string& content) const {
    std::vector<Token> tokens;
    uint32_t current_position = 0;
    bool next_is_definition = false;

    size_t i = 0;
    const size_t n = content.size();

    while (i < n) {
        // Skip whitespace and non-alphanumeric separators (except quotes/delimiters)
        if (!is_alphanumeric_or_symbol(content[i])) {
            i++;
            continue;
        }

        // Start of a word/token
        size_t start_offset = i;
        while (i < n && is_alphanumeric_or_symbol(content[i])) {
            i++;
        }

        std::string raw_word = content.substr(start_offset, i - start_offset);
        std::string lower_word = normalize(raw_word);

        if (lower_word.empty()) {
            continue;
        }

        // Check if this token matches a definition keyword
        if (definition_keywords_.find(lower_word) != definition_keywords_.end()) {
            next_is_definition = true;
            // Keywords are still indexed as regular tokens
            if (!filter_stop_words_ || stop_words_.find(lower_word) == stop_words_.end()) {
                tokens.emplace_back(lower_word, current_position++, static_cast<uint32_t>(start_offset), false);
            }
            continue;
        }

        bool is_def = next_is_definition;
        next_is_definition = false; // Reset trigger

        // Stop words check
        if (filter_stop_words_ && stop_words_.find(lower_word) != stop_words_.end()) {
            current_position++;
            continue;
        }

        // Code identifier decomposition (camelCase, snake_case)
        std::vector<std::string> sub_parts = split_identifier(raw_word);

        // Always index the full unified normalized token
        tokens.emplace_back(lower_word, current_position, static_cast<uint32_t>(start_offset), is_def);

        // If identifier was composite (e.g. parseHttpRequest), also index sub-parts
        if (sub_parts.size() > 1) {
            for (const auto& sub : sub_parts) {
                if (sub != lower_word && (!filter_stop_words_ || stop_words_.find(sub) == stop_words_.end())) {
                    tokens.emplace_back(sub, current_position, static_cast<uint32_t>(start_offset), is_def);
                }
            }
        }

        current_position++;
    }

    return tokens;
}

std::vector<std::string> Tokenizer::tokenize_query(const std::string& query) const {
    std::vector<std::string> terms;
    size_t i = 0;
    const size_t n = query.size();

    while (i < n) {
        while (i < n && std::isspace(static_cast<unsigned char>(query[i]))) {
            i++;
        }
        if (i >= n) break;

        // Quoted phrase
        if (query[i] == '"') {
            size_t start = ++i;
            while (i < n && query[i] != '"') {
                i++;
            }
            std::string phrase = query.substr(start, i - start);
            if (!phrase.empty()) {
                terms.push_back("\"" + phrase + "\"");
            }
            if (i < n && query[i] == '"') i++;
            continue;
        }

        // Normal term or operator (AND, OR, NOT)
        size_t start = i;
        while (i < n && !std::isspace(static_cast<unsigned char>(query[i])) && query[i] != '"') {
            i++;
        }
        std::string raw = query.substr(start, i - start);
        if (raw == "AND" || raw == "OR" || raw == "NOT") {
            terms.push_back(raw);
        } else {
            std::string norm = normalize(raw);
            if (!norm.empty()) {
                terms.push_back(norm);
            }
        }
    }

    return terms;
}
