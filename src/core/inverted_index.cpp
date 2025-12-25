#include "core/inverted_index.h"
#include "core/tokenizer.h"

void InvertedIndex::add_document(int docId, const std::string &text)
{
    for (const auto &token : tokenize(text))
    {
        index_[token][docId]++;
    }
}

std::vector<int> InvertedIndex::search(const std::string &term) const
{
    auto it = index_.find(term);
    if (it == index_.end())
        return {};
    std::vector<int> docs;
    docs.reserve(it->second.size());
    for (const auto &kv : it->second)
        docs.push_back(kv.first);
    std::sort(docs.begin(), docs.end());
    return docs;
}

std::vector<std::pair<int, int>> InvertedIndex::postings(const std::string &term) const
{
    auto it = index_.find(term);
    if (it == index_.end())
        return {};
    std::vector<std::pair<int, int>> out;
    out.reserve(it->second.size());
    for (const auto &kv : it->second)
        out.push_back({kv.first, kv.second});
    std::sort(out.begin(), out.end());
    return out;
}

int InvertedIndex::df(const std::string &term) const
{
    auto it = index_.find(term);
    if (it == index_.end())
        return 0;
    return (int)it->second.size();
}
