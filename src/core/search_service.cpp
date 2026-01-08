#include "search_service.h"
#include "query_parser.h"
#include "tokenizer.h"
#include <algorithm>
#include <unordered_set>
#include "core/snippet.h"
#include <cmath>
#include <shared_mutex>
#include <mutex>
#include <stdexcept>
#include <filesystem>
#include <fstream>
#include <sstream>

// jsoncpp is already used in the project via Dragon,
// so Json::Value is available for use here if needed.
#include <json/json.h>

namespace fs = std::filesystem;

static void require_file(const fs::path &p)
{
    if (!fs::exists(p) || !fs::is_regular_file(p))
    {
        throw std::runtime_error("File does not exist: " + p.string());
    }
}

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
    // while (i < a.size() || j < b.size())
    while (i < a.size() && j < b.size())
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

static void write_atomic(const fs::path &final_path,
                         const std::function<void(std::ofstream &)> &writer,
                         std::ios::openmode mode = std::ios::out)
{
    fs::create_directories(final_path.parent_path());

    const fs::path tmp_path = final_path.string() + ".tmp";

    std::ofstream out(tmp_path, mode);
    if (!out)
        throw std::runtime_error("Failed to write index file: " + tmp_path.string());

    writer(out);
    out.flush();
    out.close();

    // Atomic replace (best effort). Rename is atomic on same filesystem.
    std::error_code ec;
    fs::rename(tmp_path, final_path, ec);
    if (ec)
    {
        // If target exists, try remove then rename (portable-ish approach).
        fs::remove(final_path, ec);
        ec.clear();
        fs::rename(tmp_path, final_path, ec);
    }
    if (ec)
        throw std::runtime_error("Failed to commit index file: " + final_path.string() + " (" + ec.message() + ")");
}

void SearchService::save(const std::string &index_dir) const
{
    // throw std::runtime_error("SearchService::save not implemented");
    std::unique_lock lock(mu_); // save must be exclusive

    const fs::path dir(index_dir);
    fs::create_directories(dir);

    const fs::path meta_path = dir / "index_meta.json";
    const fs::path docs_path = dir / "docs.jsonl";
    const fs::path postings_path = dir / "postings.bin";

    // 1) index_meta.json
    write_atomic(meta_path, [&](std::ofstream &out)
                 {
            Json::Value meta(Json::objectValue);
            meta["schema_version"] = 1;
            meta["N"] = N_;
            meta["avgdl"] = avgdl_;

            Json::StreamWriterBuilder w;
            w["indentation"] = ""; // deterministic
            out << Json::writeString(w, meta); }, std::ios::out | std::ios::trunc);

    // 2) docs.jsonl (ordered by docId asc)
    write_atomic(docs_path, [&](std::ofstream &out)
                 {
            std::vector<int> ids;
            ids.reserve(doc_text_.size());
            for (const auto& kv : doc_text_) ids.push_back(kv.first);
            std::sort(ids.begin(), ids.end());

            Json::StreamWriterBuilder w;
            w["indentation"] = ""; // one-line JSON

            for (int docId : ids)
            {
                Json::Value row(Json::objectValue);
                row["docId"] = docId;
                row["text"]  = doc_text_.at(docId);
                out << Json::writeString(w, row) << "\n";
            } }, std::ios::out | std::ios::trunc);

    // 3) postings.bin (let InvertedIndex handle its own atomic file or write_atomic here)
    // Best: write postings.bin.tmp then rename:
    write_atomic(postings_path,
                 [&](std::ofstream &out)
                 {
                     // easiest approach: have InvertedIndex::save write to a path.
                     // But since we already opened an ofstream, we’ll delegate by temp path instead.
                     // So: close this approach and do direct file-based save below.
                 });
    // The above placeholder isn't useful; do file-based atomic save instead:
    // write postings to postings.bin.tmp and rename
    const fs::path postings_tmp = postings_path.string() + ".tmp";
    idx_.save(postings_tmp.string());

    std::error_code ec;
    fs::rename(postings_tmp, postings_path, ec);
    if (ec)
    {
        fs::remove(postings_path, ec);
        ec.clear();
        fs::rename(postings_tmp, postings_path, ec);
    }
    if (ec)
        throw std::runtime_error("Failed to commit index file: " + postings_path.string() + " (" + ec.message() + ")");
}

void SearchService::load(const std::string &index_dir)
{
    std::unique_lock lock(mu_); // load must be exclusive
    fs::path dir(index_dir);

    // Required files
    fs::path meta_path = dir / "index_meta.json";
    fs::path docs_path = dir / "docs.jsonl";
    fs::path postings_path = dir / "postings.bin";

    require_file(meta_path);
    require_file(docs_path);
    require_file(postings_path);

    // Parse index_meta.json

    Json::Value meta;
    {
        std::ifstream in(meta_path);
        if (!in)
        {
            throw std::runtime_error("Failed to open index_meta.json" + meta_path.string());
        }

        in >> meta;
    }

    int schema_version = meta.get("schema_version", 0).asInt();
    if (schema_version != 1)
    {
        throw std::runtime_error("Unsupported schema_version: " + std::to_string(schema_version));
    }

    // Restore corpus stats
    N_ = meta.get("N", 0).asInt();
    avgdl_ = meta.get("avgdl", 0.0).asDouble();

    // Load documents from docs.jsonl
    doc_text_.clear();
    doc_len_.clear();
    {
        std::ifstream in(docs_path);
        if (!in)
        {
            throw std::runtime_error("Failed to open docs.jsonl: " + docs_path.string());
        }

        std::string line;
        while (std::getline(in, line))
        {
            if (line.empty())
                continue;

            Json::Value row;
            std::istringstream lineStream(line);
            lineStream >> row;

            int docId = row.get("docId", -1).asInt();
            std::string text = row.get("text", "").asString();

            if (docId < 0)
            {
                throw std::runtime_error("Invalid docId in docs.jsonl");
            }

            doc_text_[docId] = text;
            // Calculate document length (token count)
            doc_len_[docId] = (int)tokenize(text).size();
        }
    }

    // Load postings from postings.bin
    idx_.load(postings_path.string());
}