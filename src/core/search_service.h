#pragma one

#include <vector>
#include <string>
#include "inverted_index.h"

class SearchService
{
public:
    void add_document(int docId, const std::string &text);
    std::vector<int> search(const std::string &query) const;

private:
    InvertedIndex idx_;

    // docId -> document length (token count)
    std::unordered_map<int, int> doc_len_;

    // number of documents indexed
    int N_ = 0;

    // average doument length
    double avgdl_ = 0.0;
};
