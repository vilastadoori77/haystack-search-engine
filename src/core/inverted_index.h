#pragma once
#include <string>
#include <unordered_map>
#include <vector>

class InvertedIndex
{
public:
    void add_document(int docId, const std::string &text);

    // Existing: Returns docId;s that contain term
    std::vector<int> search(const std::string &term) const;

    // New : term frequency per doc for this term
    std::vector<std::pair<int, int>> postings(const std::string &term) const;

    // New : document frequency (how many docs contain this term)
    int df(const std::string &term) const;

private:
    // term -> (docId -> tf)
    std::unordered_map<std::string, std::unordered_map<int, int>> index_;
};
