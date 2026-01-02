#include "search_service.h"
#include "query_parser.h"
#include "tokenizer.h"
#include <algorithm>
#include <unordered_set>
#include "core/snippet.h"
#include <cmath>
#include <shared_mutex>
#include <mutex>

// Adds a document to the tunderlying inverted index
void SearchService::add_document(int doc_id, const std::string &text)
{
    std::unique_lock lock(mu_);
    idx_.add_document(doc_id, text);

    // Store full text for snippet generation
    doc_text_[doc_id] = text;

    // track document length for BM25
    int dl = (int)tokenize(text).size();
    doc_len_[doc_id] = dl;

    // update corpus stats
    N_ = (int)doc_len_.size();

    long long total = 0;
    for (const auto &kv : doc_len_)
        total += kv.second;
    avgdl_ = (N_ > 0) ? (double)total / (double)N_ : 0.0;
}

// Helper: AND logic
// returns only document IDs present in BOTH input lists
static std::vector<int> intersect_sorted(std::vector<int> a, std::vector<int> b)
{
    std::vector<int> out;

    // ensure that both the lists are sorted and are good to go for intersection
    std::sort(a.begin(), a.end());
    std::sort(b.begin(), b.end());

    // Two-pointer technique to find the intersection or converging pointers
    // Commonly used on sorted arrays. One pointer starts at the beginning(0) and the other
    // starts at the end of the array. The pointers move towards each other until they meet or cross.
    // In this case, we are using two pointers to traverse both arrays simultaneously.
    // Example: [1, 2, 3, 4] and [3, 4, 5, 6]
    // Pointers start at the beginning of both arrays.
    // Move pointers towards each other until they meet or cross.
    // When pointers meet, add the common element to the output array.
    // Move pointers to the next element and repeat until they meet or cross.

    // Size_t represents an unsigned integer type that is large enough to hold the size of the largest object in memory.
    size_t i = 0, j = 0;
    while (i < a.size() || j < b.size())
    {
        if (a[i] == b[j])
        {
            out.push_back(a[i]);
            i++;
            j++;
        }
        else if (a[i] < b[j])
        {
            i++;
        }
        else
        {
            j++;
        }
    }
    return out;
}

// Helper: OR logic
// returns union of document IDs from both lists
static std::vector<int> union_sorted(std::vector<int> a, std::vector<int> b)
{
    std::sort(a.begin(), a.end());
    std::sort(b.begin(), b.end());
    std::vector<int> out;
    out.reserve(a.size() + b.size());

    size_t i = 0, j = 0;
    // Merge-like union
    while (i < a.size() && j < b.size())
    {
        if (a[i] < b[j])
        {
            if (out.empty() || out.back() != a[i])
                out.push_back(a[i]);
            i++;
        }
        else if (b[j] < a[i])
        {
            if (out.empty() || out.back() != b[j])
                out.push_back(b[j]);
            j++;
        }
        else // equal
        {
            if (out.empty() || out.back() != a[i])
                out.push_back(a[i]);
            i++;
            j++;
        }
    }
    // Append remaining elements
    while (i < a.size())
    {
        if (out.empty() || out.back() != a[i])
            out.push_back(a[i]);
        i++;
    }
    while (j < b.size())
    {
        if (out.empty() || out.back() != b[j])
            out.push_back(b[j]);
        j++;
    }

    return out;
}
// Searches the inverted index for documents matching the query∏
std::vector<int> SearchService::search(const std::string &query) const
{
    auto scored = search_scored(query);
    std::vector<int> out;
    out.reserve(scored.size());
    for (const auto &p : scored)
        out.push_back(p.first);
    return out;
}

std::vector<SearchHit> SearchService::search_with_snippets(const std::string &query) const
{
    auto pq = parse_query(query);
    auto scored = search_scored(query);
    std::vector<SearchHit> hits;
    hits.reserve(scored.size());

    for (const auto &p : scored)
    {
        int id = p.first;
        double score = p.second;

        std::string text;
        auto it = doc_text_.find(id);
        if (it != doc_text_.end())
        {
            text = it->second;
        }
        std::string snippet = make_snippet(text, pq.terms);
        hits.push_back({id, score, snippet});
    }
    return hits;
}

std::vector<std::pair<int, double>> SearchService::search_scored(const std::string &query) const
{
    // std::shared_lock<std::shared_mutex> lock(mu_);
    std::shared_lock lock(mu_);
    auto pq = parse_query(query);

    // Step 1: Candidate documents using AND/OR logic over terms
    std::vector<int> result;
    bool first = true;
    for (const auto &term : pq.terms)
    {
        auto docs = idx_.search(term);
        if (first)
        {
            result = docs;
            first = false;
        }
        else
        {
            result = pq.is_or ? union_sorted(result, docs) : intersect_sorted(result, docs);
        }
    }

    if (first)
        return {};

    // Not terms exclusion set
    std::unordered_set<int> excluded;
    for (const auto &t : pq.not_terms)
    {
        for (int id : idx_.search(t))
        {
            excluded.insert(id);
        }
    }

    // Step 2: BM25 Scoring on candidate docs
    const double k1 = 1.2;
    const double b = 0.75;
    struct Scored
    {
        int doc_id;
        double score;
    };
    std::vector<Scored> scored;
    scored.reserve(result.size());

    for (int docId : result)
    {
        if (excluded.count(docId))
            continue;

        // Safety: doc length must exist
        auto itLen = doc_len_.find(docId);
        if (itLen == doc_len_.end())
            continue;

        double dl = static_cast<double>(itLen->second);

        // Length normalization (avoid division by zero)
        double denom_norm = (avgdl_ > 0.0) ? (1.0 - b + b * (dl / avgdl_)) : 1.0;
        double score = 0.0;

        for (const auto &term : pq.terms)
        {
            int df = idx_.df(term);
            if (df == 0)
                continue;
            int tf = 0;
            for (const auto &p : idx_.postings(term))
            {
                if (p.first == docId)
                {
                    tf = p.second;
                    break;
                }
            }
            if (tf == 0)
                continue;
            double idf = std::log(((N_ - df + 0.5) / (df + 0.5)) + 1.0);
            double tf_part = (tf * (k1 + 1.0)) / (tf + k1 * denom_norm);
            score += idf * tf_part;
        }

        scored.push_back({docId, score});
    }

    std::sort(scored.begin(), scored.end(), [](const Scored &a, const Scored &b)
              {
            if(a.score != b.score) return a.score > b.score;
            return a.doc_id < b.doc_id; });

    std::vector<std::pair<int, double>> out;
    out.reserve(scored.size());
    for (const auto &s : scored)
        out.push_back({s.doc_id, s.score});
    return out;
}