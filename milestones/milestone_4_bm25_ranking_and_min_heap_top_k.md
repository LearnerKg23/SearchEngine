# Milestone 4: BM25 Ranking & $O(N \log K)$ Min-Heap Top-K Retrieval

> **Resume Alignment**: *"Optimized query latency to under 15 ms across 25,000+ documents using a Min-Heap for O(N log K) Top-K retrieval... and implemented the BM25 ranking algorithm to prioritize relevant code definitions and documents."*

---

## 🎯 Objectives
1. Implement the industry-standard **Okapi BM25** ranking algorithm for relevance scoring.
2. Incorporate document length normalization (`avgdl`) and term saturation parameters ($k_1$, $b$).
3. Implement bounded **Top-K retrieval using a Min-Heap** (`std::priority_queue`) to achieve $O(N \log K)$ time complexity instead of expensive $O(N \log N)$ full sorting.

---

## 1. Okapi BM25 Algorithm Breakdown

BM25 addresses the fundamental flaw of simple TF-IDF: in code search, a document repeating a variable 50 times should not be 50 times more relevant than one repeating it 3 times.

### Mathematical Formulation
$$\text{Score}(D, Q) = \sum_{i=1}^{n} \text{IDF}(q_i) \cdot \frac{f(q_i, D) \cdot (k_1 + 1)}{f(q_i, D) + k_1 \cdot \left(1 - b + b \cdot \frac{|D|}{\text{avgdl}}\right)}$$

Where:
* $f(q_i, D)$ is the raw term frequency of $q_i$ in document $D$.
* $|D|$ is the length of document $D$ in words.
* $\text{avgdl}$ is the average document length across all documents in the corpus.
* $k_1$ (typically `1.2` to `1.5`): controls term frequency saturation. As $TF \to \infty$, score asymptotes.
* $b$ (typically `0.75`): controls the penalty for document length. Long files have their TF penalized unless they contain exceptionally high occurrences.

### Inverse Document Frequency (IDF)
$$\text{IDF}(q_i) = \ln\left( \frac{N - n(q_i) + 0.5}{n(q_i) + 0.5} + 1 \right)$$
Where $N$ is total documents in corpus, and $n(q_i)$ is number of documents containing $q_i$.

---

## 2. $O(N \log K)$ Min-Heap Top-K Retrieval

### Why Not `std::sort`?
If a query matches $N = 25,000$ documents and the user only wants the top $K = 10$ results:
* **Full Sort (`std::sort`)**: $O(N \log N) \approx 25,000 \times 14.6 \approx 365,000$ comparisons + full array allocation.
* **Min-Heap**: $O(N \log K) \approx 25,000 \times 3.32 \approx 83,000$ comparisons.
* **Cache Advantage**: The heap only holds 10 elements in L1 CPU cache throughout the loop!

### Min-Heap Mechanics
A **Min-Heap** keeps the *minimum* score of the current top-K at the root:
1. Push the first $K$ scored documents into the heap.
2. For every remaining document $D_{K+1} \dots D_N$:
   - If $\text{score}(D) > \text{heap.top().score}$:
     - `heap.pop()` (removes the smallest of the top-K).
     - `heap.push(D)` (inserts new candidate in $O(\log K)$).
3. At the end, the heap contains the exact top-K documents in ascending order. Reverse it to get descending order.

```
       [ Min-Heap of Size K=3 ]
               ( Score: 2.1 )  <-- Root is SMALLEST in Top-K!
                 /       \
          ( Score: 4.8 )  ( Score: 5.3 )

New Doc arrives with Score: 3.5
  Since 3.5 > 2.1 (root):
  1. Pop 2.1
  2. Push 3.5
  New Root becomes 3.5 (the new minimum among {3.5, 4.8, 5.3})
```

---

## 3. Implementation (`src/bm25_ranker.cpp`)

```cpp
#include "common/types.hpp"
#include <cmath>
#include <queue>
#include <vector>
#include <algorithm>

struct ScoredDoc {
    doc_id_t doc_id;
    float score;

    // Greater-than operator creates a MIN-HEAP
    bool operator>(const ScoredDoc& other) const {
        return score > other.score;
    }
};

class BM25Ranker {
public:
    BM25Ranker(double avgdl, uint32_t total_docs, float k1 = 1.2f, float b = 0.75f)
        : avgdl_(avgdl), total_docs_(total_docs), k1_(k1), b_(b) {}

    float compute_idf(uint32_t doc_freq) const {
        return std::log((total_docs_ - doc_freq + 0.5f) / (doc_freq + 0.5f) + 1.0f);
    }

    float compute_score(uint32_t tf, uint32_t doc_len, float idf) const {
        float num = tf * (k1_ + 1.0f);
        float denom = tf + k1_ * (1.0f - b_ + b_ * (static_cast<float>(doc_len) / avgdl_));
        return idf * (num / denom);
    }

    std::vector<ScoredDoc> top_k(const std::vector<ScoredDoc>& candidates, size_t k) {
        if (candidates.empty() || k == 0) return {};

        // Min-heap bounded to size K
        std::priority_queue<ScoredDoc, std::vector<ScoredDoc>, std::greater<ScoredDoc>> min_heap;

        for (const auto& item : candidates) {
            if (min_heap.size() < k) {
                min_heap.push(item);
            } else if (item.score > min_heap.top().score) {
                min_heap.pop();
                min_heap.push(item);
            }
        }

        // Extract and sort descending
        std::vector<ScoredDoc> results;
        results.reserve(min_heap.size());
        while (!min_heap.empty()) {
            results.push_back(min_heap.top());
            min_heap.pop();
        }
        std::reverse(results.begin(), results.end());
        return results;
    }

private:
    double avgdl_;
    uint32_t total_docs_;
    float k1_;
    float b_;
};
```

---

## 4. Prioritizing Code Definitions (Bonus Resume Signal)
To boost code definitions:
* During tokenization in Milestone 1, identify whether a token appears in a function declaration (e.g., following `class `, `struct `, `void `, `def `).
* Apply a multiplier (e.g. `1.5x` score boost) if the match occurs in a definition line.

---

## 5. Verification & Unit Tests (`tests/test_bm25.cpp`)
* **Test 1: Length Normalization Test**
  - Create Doc A (10 words, contains "vector" twice).
  - Create Doc B (1,000 words, contains "vector" twice).
  - Assert that `Score(Doc A) > Score(Doc B)` due to length normalization penalty $b$.
* **Test 2: Term Saturation Test**
  - Verify that increasing TF from 1 to 2 produces a much larger score jump than increasing TF from 50 to 51.
* **Test 3: Top-K Accuracy vs. Full Sort**
  - Generate 10,000 random scored candidates.
  - Run `top_k()` with $K=10$ against `std::sort()`.
  - Assert the resulting top-10 doc IDs and order are identical.

---

## ✅ Milestone 4 Checklist
- [ ] BM25 IDF and TF formulas implemented with configurable $k_1$ and $b$.
- [ ] Min-Heap Top-K algorithm implemented with `std::greater`.
- [ ] Tests verify $O(N \log K)$ output matches exact full sorting.
