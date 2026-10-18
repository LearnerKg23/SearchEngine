#pragma once
#include "common/types.hpp"
#include "index/index_reader.hpp"
#include "tokenizer/tokenizer.hpp"
#include <string>
#include <vector>

class QueryParser {
public:
    explicit QueryParser(IndexReader& reader);

    // Parses and executes a query, returning filtered matching postings
    std::vector<Posting> execute(const std::string& query_str);

private:
    IndexReader& reader_;
    Tokenizer tokenizer_;

    std::vector<Posting> eval_term(const std::string& term);
    std::vector<Posting> eval_phrase(const std::string& phrase);
};
