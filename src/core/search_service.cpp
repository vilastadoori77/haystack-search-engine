#include "search_service.h"
#include "query_parser.h"
#include <algorithm>
#include <unordered_set>

// Adds a document to the tunderlying inverted index
void SearchService::add_document(int doc_id, const std::string &text)
{
    idx_.add_document(doc_id, text);
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

// Main search function
std::vector<int> SearchService::search(const std::string &query) const
{
    // Step 1: Parse  the query string
    // Extracts: terms, not_terms, and is_or flag
    auto pq = parse_query(query);
    std::vector<int> result;
    bool first = true;
    // Step 2: Collect matching documents for each term
    for (const auto &term : pq.terms)
    {
        auto docs = idx_.search(term);

        // First term initializes the result set
        if (first)
        {
            result = docs;
            first = false;
        }
        // combine results depending on OR/AND logic
        else
        {
            result = pq.is_or ? union_sorted(result, docs) : intersect_sorted(result, docs);
        }
    }

    // Step 3: Apply Not terms (exclusion)
    if (!pq.not_terms.empty())
    {
        // Collect all documents to exclude
        std::unordered_set<int> excluded;
        for (const auto &t : pq.not_terms)
        {
            for (int id : idx_.search(t))
            {
                excluded.insert(id);
            }
        }

        // Remove excluded documents from result
        std::vector<int> filtered;
        for (int id : result)
        {
            if (!excluded.count(id))
            {
                filtered.push_back(id);
            }
        }
        result = filtered;
    }

    // Step 4: normalize the output
    // - sort
    // - remove duplicates
    std::sort(result.begin(), result.end());
    result.erase(std::unique(result.begin(), result.end()), result.end());
    return result;
}
