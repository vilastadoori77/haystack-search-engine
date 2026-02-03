#pragma once

#include <vector>
#include <string>
#include <unordered_map>
#include "inverted_index.h"
#include <shared_mutex>

struct SearchHit
{
    int docId;
    double score;
    std::string snippet;
};

class SearchService
{
public:
    void add_document(int docId, const std::string &text);
    void save(const std::string &index_dir) const;
    void load(const std::string &index_dir);
    std::vector<int> search(const std::string &query) const;
    std::vector<SearchHit> search_with_snippets(const std::string &query) const;
    std::vector<std::pair<int, double>> search_scored(const std::string &query) const;

private:
    mutable std::shared_mutex mu_;
    InvertedIndex idx_;

    // docId -> document length (token count)
    std::unordered_map<int, size_t> doc_len_;
    std::unordered_map<int, std::string> doc_text_;

    // number of documents indexed
    int N_ = 0;

    // average doument length
    double avgdl_ = 0.0;
};
