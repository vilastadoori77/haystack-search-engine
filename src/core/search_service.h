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
};
