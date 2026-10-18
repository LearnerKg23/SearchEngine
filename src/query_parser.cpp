#include "query/query_parser.hpp"
#include "query/boolean_retriever.hpp"
#include "query/phrase_matcher.hpp"

QueryParser::QueryParser(IndexReader& reader)
    : reader_(reader), tokenizer_(false) {}

std::vector<Posting> QueryParser::eval_term(const std::string& term) {
    if (!reader_.has_term(term)) {
        return {};
    }
    return reader_.get_postings(term);
}

std::vector<Posting> QueryParser::eval_phrase(const std::string& phrase_str) {
    // Strip surrounding quotes
    std::string clean = phrase_str;
    if (clean.size() >= 2 && clean.front() == '"' && clean.back() == '"') {
        clean = clean.substr(1, clean.size() - 2);
    }

    auto tokens = tokenizer_.tokenize(clean);
    if (tokens.empty()) return {};

    std::vector<std::vector<Posting>> term_postings;
    for (const auto& tok : tokens) {
        if (!reader_.has_term(tok.text)) {
            return {}; // If any phrase word is completely missing from corpus, phrase match is empty
        }
        term_postings.push_back(reader_.get_postings(tok.text));
    }

    return PhraseMatcher::match_sequence(term_postings);
}

std::vector<Posting> QueryParser::execute(const std::string& query_str) {
    auto tokens = tokenizer_.tokenize_query(query_str);
    if (tokens.empty()) return {};

    enum class Op { AND, OR, NOT };
    Op next_op = Op::AND;

    std::vector<Posting> current_result;
    bool has_initial_result = false;

    for (size_t i = 0; i < tokens.size(); ++i) {
        const std::string& tok = tokens[i];

        if (tok == "AND") {
            next_op = Op::AND;
            continue;
        } else if (tok == "OR") {
            next_op = Op::OR;
            continue;
        } else if (tok == "NOT") {
            next_op = Op::NOT;
            continue;
        }

        // Evaluate token (either phrase or term)
        std::vector<Posting> term_result;
        if (!tok.empty() && tok.front() == '"' && tok.back() == '"') {
            term_result = eval_phrase(tok);
        } else {
            term_result = eval_term(tok);
        }

        if (!has_initial_result) {
            current_result = std::move(term_result);
            has_initial_result = true;
        } else {
            switch (next_op) {
                case Op::AND:
                    current_result = BooleanRetriever::intersect(current_result, term_result);
                    break;
                case Op::OR:
                    current_result = BooleanRetriever::merge_union(current_result, term_result);
                    break;
                case Op::NOT:
                    current_result = BooleanRetriever::difference(current_result, term_result);
                    break;
            }
            next_op = Op::AND; // Reset to default conjunction
        }
    }

    return current_result;
}
