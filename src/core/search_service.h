#pragma one

#include <vector>
#include <string>
#include <unordered_map>
#include "inverted_index.h"

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
    std::vector<int> search(const std::string &query) const;
    std::vector<SearchHit> search_with_snippets(const std::string &query) const;

private:
    InvertedIndex idx_;

    // docId -> document length (token count)
    std::unordered_map<int, int> doc_len_;
    std::unordered_map<int, std::string> doc_text_;

    // number of documents indexed
    int N_ = 0;

    // average doument length
    double avgdl_ = 0.0;
};
