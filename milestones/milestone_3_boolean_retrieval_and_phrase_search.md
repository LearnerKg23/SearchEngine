# Milestone 3: Two-Pointer Boolean Retrieval & Exact Phrase Search

> **Resume Alignment**: *"...using a two-pointer intersection algorithm for Boolean queries."*

---

## 🎯 Objectives
1. Implement the **Two-Pointer Intersection algorithm ($O(N + M)$)** for `AND` operations on sorted postings lists.
2. Implement **Two-Pointer Union ($O(N + M)$)** for `OR` operations and difference for `NOT` operations.
3. Implement **Exact Phrase Search** by comparing positional arrays ($pos_{t2} = pos_{t1} + 1$) on common documents.
4. Build a lightweight **Query Parser** that parses boolean expressions and quoted phrases.

---

## 1. Two-Pointer Intersection Algorithm (`AND`)

Because our inverted index writes postings in strictly ascending `doc_id` order ($doc_0 < doc_1 < \dots < doc_n$), finding documents that contain **both** terms does not require hash sets or nested loops.

We advance two pointers along both postings lists in a single linear pass:

```
List A: [ 2,  5,  9,  14,  20 ]
          ▲
          i
List B: [ 3,  5,  8,  14,  22 ]
          ▲
          j

Step 1: A[0] (2) < B[0] (3) -> Advance i
Step 2: A[1] (5) == B[1] (5) -> Match! Add 5 to result, advance both i and j
Step 3: A[2] (9) > B[2] (8) -> Advance j
Step 4: A[2] (9) < B[3] (14)-> Advance i
Step 5: A[3] (14) == B[3] (14)-> Match! Add 14 to result, advance both i and j
```

### Time & Space Complexity
* **Time Complexity**: $O(N + M)$ where $N$ and $M$ are lengths of the postings lists.
* **Space Complexity**: $O(\min(N, M))$ for output list.

### C++ Implementation
```cpp
std::vector<Posting> intersect_postings(const std::vector<Posting>& list_a, 
                                        const std::vector<Posting>& list_b) {
    std::vector<Posting> result;
    size_t i = 0, j = 0;

    while (i < list_a.size() && j < list_b.size()) {
        if (list_a[i].doc_id == list_b[j].doc_id) {
            // Document matches both terms
            result.push_back(list_a[i]);
            ++i;
            ++j;
        } else if (list_a[i].doc_id < list_b[j].doc_id) {
            ++i;
        } else {
            ++j;
        }
    }
    return result;
}
```

---

## 2. Two-Pointer Union Algorithm (`OR`)

Merges two sorted lists in $O(N + M)$ time, avoiding duplicate document entries:

```cpp
std::vector<Posting> union_postings(const std::vector<Posting>& list_a, 
                                    const std::vector<Posting>& list_b) {
    std::vector<Posting> result;
    size_t i = 0, j = 0;

    while (i < list_a.size() && j < list_b.size()) {
        if (list_a[i].doc_id == list_b[j].doc_id) {
            result.push_back(list_a[i]);
            ++i;
            ++j;
        } else if (list_a[i].doc_id < list_b[j].doc_id) {
            result.push_back(list_a[i]);
            ++i;
        } else {
            result.push_back(list_b[j]);
            ++j;
        }
    }

    while (i < list_a.size()) result.push_back(list_a[i++]);
    while (j < list_b.size()) result.push_back(list_b[j++]);

    return result;
}
```

---

## 3. Exact Phrase Matching (Positional Proximity)

For a phrase query like `"binary search"`, a document must:
1. Contain **both** `"binary"` and `"search"` (found via the two-pointer intersection above).
2. Contain `"search"` at index $pos + 1$ relative to `"binary"` at $pos$.

### Algorithm
```cpp
bool verify_phrase(const std::vector<uint32_t>& pos_a, 
                   const std::vector<uint32_t>& pos_b, 
                   uint32_t distance = 1) {
    size_t i = 0, j = 0;
    while (i < pos_a.size() && j < pos_b.size()) {
        if (pos_a[i] + distance == pos_b[j]) {
            return true; // Consecutive positions found
        } else if (pos_a[i] + distance < pos_b[j]) {
            ++i;
        } else {
            ++j;
        }
    }
    return false;
}
```

For multi-word phrases (e.g. `"quick brown fox"`):
- Intersect positional candidates iteratively across adjacent words.

---

## 4. Query Parser (`include/query/query_parser.hpp`)

Supports 4 query styles:
1. **Single Term**: `vector`
2. **Conjunctive (AND)**: `vector AND memory` or default multi-word `vector memory`
3. **Disjunctive (OR)**: `vector OR array`
4. **Exact Phrase**: `"hash table"`
5. **Negation (NOT)**: `tree NOT binary`

### AST Query Representation
```cpp
enum class QueryType { TERM, AND, OR, NOT, PHRASE };

struct QueryNode {
    QueryType type;
    std::string term;                        // for TERM
    std::vector<std::string> phrase_terms;   // for PHRASE
    std::vector<std::unique_ptr<QueryNode>> children; // for AND, OR, NOT
};
```

---

## 5. Verification & Unit Tests (`tests/test_boolean.cpp`)
* **Test 1: Two-Pointer Intersection**
  - Verify non-overlapping lists return empty list in minimal time.
  - Verify identical lists return the complete list.
  - Verify interspersed lists return exact common elements.
* **Test 2: Two-Pointer Union**
  - Verify deduplication of shared document IDs.
* **Test 3: Exact Phrase Search**
  - Doc 1: `"We build a binary search engine"` (positions 3 and 4 $\rightarrow$ match).
  - Doc 2: `"The search is binary"` (positions 1 and 3 $\rightarrow$ no match).
  - Confirm Doc 1 matches `"binary search"` and Doc 2 is excluded.

---

## ✅ Milestone 3 Checklist
- [ ] Two-pointer intersection implemented with $O(N + M)$ performance.
- [ ] Two-pointer union and difference implemented.
- [ ] Positional phrase matching implemented and verified.
- [ ] Query parser parses terms, operators, and quoted strings into an AST.
