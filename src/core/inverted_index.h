#pragma once
#include <string>
#include <unordered_map>
#include <vector>

class InvertedIndex
{
public:
    void save(const std::string &postings_path) const;
    void load(const std::string &postings_path);
    void add_document(int docId, const std::string &text);

    // Returns docIds that contain term (sorted)
    std::vector<int> search(const std::string &term) const;

    // Returns term frequency per doc for this term (sorted by docId)
    std::vector<std::pair<int, int>> postings(const std::string &term) const;

    // New :Direct access to postings map for o(1) TF lookup(performance critical)
    // Returns nullptr if term not found
    const std::unordered_map<int, int> *postings_map(const std::string &term) const;

    // New : document frequency (how many docs contain this term)
    int df(const std::string &term) const noexcept;

private:
    // term -> (docId -> tf)
    std::unordered_map<std::string, std::unordered_map<int, int>> index_;
};
