#include "inverted_index.h"
#include "tokenizer.h"
#include <stdexcept>
#include <algorithm>
#include <cstdint>
#include <fstream>
#include <string>
#include <vector>
#include <filesystem>

namespace fs = std::filesystem;

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
    docs.reserve(it->second.size()); // OPTINIZATION: Reserve Capacity
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
    out.reserve(it->second.size()); // Optimize: Reserve Capacity
    for (const auto &kv : it->second)
        out.push_back({kv.first, kv.second});
    std::sort(out.begin(), out.end());
    return out;
}

// NEW: 0(1) access to postings map for performancecritical BM25 scoring
const std::unordered_map<int, int> *InvertedIndex::postings_map(const std::string &term) const
{
    auto it = index_.find(term);
    if (it == index_.end())
        return nullptr;
    return &(it->second);
}

int InvertedIndex::df(const std::string &term) const noexcept
{
    auto it = index_.find(term);
    if (it == index_.end())
        return 0;
    return (int)it->second.size();
}

// Phase 2 : Persistence (postings.bin) ------

// ---- little-endian helpers (portable) ----
static void write_u32(std::ostream &os, std::uint32_t v)
{
    unsigned char b[4] = {
        (unsigned char)(v & 0xFF),
        (unsigned char)((v >> 8) & 0xFF),
        (unsigned char)((v >> 16) & 0xFF),
        (unsigned char)((v >> 24) & 0xFF),
    };
    os.write((const char *)b, 4);
}
static void write_u64(std::ostream &os, std::uint64_t v)
{
    unsigned char b[8] = {
        (unsigned char)(v & 0xFF),
        (unsigned char)((v >> 8) & 0xFF),
        (unsigned char)((v >> 16) & 0xFF),
        (unsigned char)((v >> 24) & 0xFF),
        (unsigned char)((v >> 32) & 0xFF),
        (unsigned char)((v >> 40) & 0xFF),
        (unsigned char)((v >> 48) & 0xFF),
        (unsigned char)((v >> 56) & 0xFF),
    };
    os.write((const char *)b, 8);
}
static void write_i32(std::ostream &os, std::int32_t v) { write_u32(os, (std::uint32_t)v); }

static std::uint32_t read_u32(std::istream &is)
{
    unsigned char b[4];
    is.read((char *)b, 4);
    if (!is)
        throw std::runtime_error("Failed to read u32");
    return (std::uint32_t)b[0] | ((std::uint32_t)b[1] << 8) | ((std::uint32_t)b[2] << 16) | ((std::uint32_t)b[3] << 24);
}
static std::uint64_t read_u64(std::istream &is)
{
    unsigned char b[8];
    is.read((char *)b, 8);
    if (!is)
        throw std::runtime_error("Failed to read u64");
    return (std::uint64_t)b[0] | ((std::uint64_t)b[1] << 8) | ((std::uint64_t)b[2] << 16) | ((std::uint64_t)b[3] << 24) | ((std::uint64_t)b[4] << 32) | ((std::uint64_t)b[5] << 40) | ((std::uint64_t)b[6] << 48) | ((std::uint64_t)b[7] << 56);
}
static std::int32_t read_i32(std::istream &is) { return (std::int32_t)read_u32(is); }

void InvertedIndex::save(const std::string &postings_path) const
{
    fs::path outPath(postings_path);
    auto parent = outPath.parent_path();
    if (!parent.empty())
    {
        std::error_code ec;
        fs::create_directories(parent, ec); // ok if exists
        if (ec)
            throw std::runtime_error("Failed to create directory: " + parent.string());
    }

    fs::path tmpPath = outPath;
    tmpPath += ".tmp";

    std::ofstream out(tmpPath, std::ios::binary);
    if (!out)
        throw std::runtime_error("Failed to write index file: " + postings_path);

    // deterministic term order
    std::vector<std::string> terms;
    terms.reserve(index_.size());
    for (const auto &kv : index_)
        terms.push_back(kv.first);
    std::sort(terms.begin(), terms.end());

    write_u64(out, (std::uint64_t)terms.size());

    for (const auto &term : terms)
    {
        const auto &postMap = index_.at(term);

        write_u32(out, (std::uint32_t)term.size());
        out.write(term.data(), (std::streamsize)term.size());

        // deterministic postings order by docId ascending
        std::vector<std::pair<int, int>> postings;
        postings.reserve(postMap.size());
        for (const auto &p : postMap)
            postings.push_back({p.first, p.second});
        std::sort(postings.begin(), postings.end(),
                  [](auto &a, auto &b)
                  { return a.first < b.first; });

        write_u32(out, (std::uint32_t)postings.size());
        for (const auto &p : postings)
        {
            write_i32(out, (std::int32_t)p.first);  // docId
            write_i32(out, (std::int32_t)p.second); // tf
        }
    }

    out.flush();
    if (!out)
        throw std::runtime_error("Failed to write index file: " + postings_path);
    out.close();

    // atomic replace
    std::error_code ec;
    fs::remove(outPath, ec); // ignore if missing
    ec.clear();
    fs::rename(tmpPath, outPath, ec);
    if (ec)
        throw std::runtime_error("Failed to finalize index file: " + postings_path);
}

void InvertedIndex::load(const std::string &postings_path)
{
    fs::path inPath(postings_path);
    std::ifstream in(inPath, std::ios::binary);
    if (!in)
        throw std::runtime_error("Index file not found: " + postings_path);

    std::unordered_map<std::string, std::unordered_map<int, int>> newIndex;

    std::uint64_t termCount = read_u64(in);
    for (std::uint64_t t = 0; t < termCount; ++t)
    {
        std::uint32_t len = read_u32(in);
        std::string term(len, '\0');
        in.read(term.data(), (std::streamsize)len);
        if (!in)
            throw std::runtime_error("Failed to parse index file: " + postings_path);

        std::uint32_t postingCount = read_u32(in);
        auto &postMap = newIndex[term];

        for (std::uint32_t i = 0; i < postingCount; ++i)
        {
            int docId = (int)read_i32(in);
            int tf = (int)read_i32(in);
            postMap[docId] = tf;
        }
    }

    // replace only after successful parse
    index_ = std::move(newIndex);
}
