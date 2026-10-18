# Milestone 1: Core Types & Code-Aware Tokenizer

> **Resume Alignment**: *"Architected a local document and code search engine from scratch in C++, implementing tokenization..."*

---

## 🎯 Objectives
1. Define the core data types used throughout the search engine (`DocId`, `Token`, `Posting`).
2. Implement a high-performance **Code-Aware Tokenizer** that splits natural language words and programming identifiers (`camelCase`, `snake_case`, `SCREAMING_SNAKE_CASE`).
3. Record positional indices and character offsets for every token to support exact phrase queries and snippet highlighting.

---

## 1. Core Data Structures (`include/common/types.hpp`)

```cpp
#pragma once
#include <cstdint>
#include <string>
#include <vector>

// Unique identifier for a document
using doc_id_t = uint32_t;

// Special constant representing an invalid doc ID
constexpr doc_id_t INVALID_DOC_ID = 0xFFFFFFFF;

// Represents an extracted token with positional metadata
struct Token {
    std::string text;       // Normalized lowercase token text
    uint32_t position;      // 0-based word index in document (for phrase search)
    uint32_t char_offset;   // Byte/character offset in original file (for snippet UI)
};

// Represents a single posting in an inverted list
struct Posting {
    doc_id_t doc_id;
    uint32_t term_frequency;
    std::vector<uint32_t> positions; // 0-based positions where term occurs in doc_id
};
```

---

## 2. Tokenizer Architecture (`include/tokenizer/tokenizer.hpp`)

Code files contain identifiers that standard NLP tokenizers destroy or treat as random text. Our tokenizer must handle:
1. **camelCase**: `getUserProfile` $\rightarrow$ `["get", "user", "profile", "getuserprofile"]`
2. **snake_case**: `parse_binary_data` $\rightarrow$ `["parse", "binary", "data", "parse_binary_data"]`
3. **Natural language**: Lowercasing, stripping punctuation, preserving numbers where appropriate (`utf8`, `c++20` $\rightarrow$ `cpp20`).

### Class Interface
```cpp
#pragma once
#include "common/types.hpp"
#include <string>
#include <string_view>
#include <vector>
#include <unordered_set>

class Tokenizer {
public:
    Tokenizer();

    // Tokenizes a full file content into a sequence of tokens with positions
    std::vector<Token> tokenize(std::string_view content) const;

    // Tokenizes a search query string (preserves operators like AND, OR, NOT, quotes)
    std::vector<std::string> tokenize_query(std::string_view query) const;

    // Utility: splits identifier into sub-words (e.g. camelCase -> ["camel", "case"])
    static std::vector<std::string> split_identifier(std::string_view identifier);

    // Normalizes a token (lowercases ASCII)
    static std::string normalize(std::string_view raw);

private:
    std::unordered_set<std::string> stop_words_;
    void init_stop_words();
};
```

---

## 3. Implementation Details (`src/tokenizer.cpp`)

### A. Identifier Sub-word Splitting Algorithm
```cpp
std::vector<std::string> Tokenizer::split_identifier(std::string_view identifier) {
    std::vector<std::string> parts;
    if (identifier.empty()) return parts;

    std::string current;
    for (size_t i = 0; i < identifier.size(); ++i) {
        char c = identifier[i];
        if (c == '_') {
            if (!current.empty()) {
                parts.push_back(normalize(current));
                current.clear();
            }
            continue;
        }

        // Check for camelCase boundary (e.g. 'a' followed by 'B')
        if (i > 0 && std::islower(identifier[i - 1]) && std::isupper(c)) {
            if (!current.empty()) {
                parts.push_back(normalize(current));
                current.clear();
            }
        }
        // Check for acronym boundary (e.g. 'HTTP' followed by 'Request')
        else if (i + 1 < identifier.size() && std::isupper(c) && 
                 std::isupper(identifier[i - 1]) && std::islower(identifier[i + 1])) {
            if (!current.empty()) {
                parts.push_back(normalize(current));
                current.clear();
            }
        }

        current += c;
    }

    if (!current.empty()) {
        parts.push_back(normalize(current));
    }
    return parts;
}
```

### B. Tracking Position & Character Offsets
* Keep a running `uint32_t current_position = 0`.
* For every word extracted:
  - If it is a composite identifier (e.g. `parseJson`), emit the sub-tokens (`parse`, `json`) at the same or sequential position, plus the full string (`parsejson`).
  - Increment `current_position++`.
  - Record the `char_offset` relative to the document start.

---

## 4. Verification & Unit Tests (`tests/test_tokenizer.cpp`)

Create automated tests covering:
* **Test 1: Standard Natural Language**
  - Input: `"The quick brown fox jumps over 123 lazy dogs."`
  - Output: `["quick", "brown", "fox", "jumps", "123", "lazy", "dogs"]` with sequential positions.
* **Test 2: camelCase & PascalCase**
  - Input: `"XMLHttpRequest parseHttpRequest"`
  - Output: sub-tokens `xml`, `http`, `request`, `parse` and unified tokens.
* **Test 3: snake_case & SCREAMING_SNAKE_CASE**
  - Input: `"MAX_BUFFER_SIZE read_exact_bytes"`
  - Output: `max`, `buffer`, `size`, `read`, `exact`, `bytes`.
* **Test 4: Offset Accuracy**
  - Verify that `content.substr(token.char_offset, token.text.size())` accurately matches the original token in the text.

---

## ✅ Milestone 1 Checklist
- [ ] `include/common/types.hpp` created with `doc_id_t`, `Token`, and `Posting`.
- [ ] `include/tokenizer/tokenizer.hpp` and `src/tokenizer.cpp` implemented.
- [ ] Code-aware identifier splitting tested on camelCase and snake_case.
- [ ] All unit tests pass with zero memory leaks.
