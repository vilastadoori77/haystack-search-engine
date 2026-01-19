#include <catch2/catch_test_macros.hpp>
#include "core/search_service.h"
#include "core/inverted_index.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cmath>
#include <random>
#include <unistd.h>

namespace fs = std::filesystem;

static std::string create_temp_dir()
{
    std::string base = "/tmp/haystack_test_";
    // Use PID + random number to avoid collisions
    pid_t pid = getpid();
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 999999);
    
    for (int i = 0; i < 1000; ++i)
    {
        std::string dir = base + std::to_string(pid) + "_" + std::to_string(dis(gen));
        if (!fs::exists(dir))
        {
            try {
                fs::create_directories(dir);
                return dir;
            } catch (...) {
                // Directory might have been created by another process, try again
                continue;
            }
        }
    }
    throw std::runtime_error("Could not create temp directory");
}

static void cleanup_temp_dir(const std::string& dir)
{
    if (fs::exists(dir))
    {
        fs::remove_all(dir);
    }
}

TEST_CASE("InvertedIndex::save() and InvertedIndex::load() round-trip")
{
    InvertedIndex idx;
    idx.add_document(1, "hello world");
    idx.add_document(2, "world peace");
    idx.add_document(3, "hello there");

    std::string temp_file = create_temp_dir() + "/postings.bin";
    cleanup_temp_dir(fs::path(temp_file).parent_path().string());

    // Save
    idx.save(temp_file);
    REQUIRE(fs::exists(temp_file));

    // Load into new index
    InvertedIndex idx2;
    idx2.load(temp_file);

    // Verify search results match
    REQUIRE(idx.search("hello") == idx2.search("hello"));
    REQUIRE(idx.search("world") == idx2.search("world"));
    REQUIRE(idx.search("peace") == idx2.search("peace"));

    cleanup_temp_dir(fs::path(temp_file).parent_path().string());
}

TEST_CASE("Binary postings format preserves term order and docId/tf pairs")
{
    InvertedIndex idx;
    idx.add_document(1, "zebra apple");
    idx.add_document(2, "apple banana");
    idx.add_document(3, "banana cherry");

    std::string temp_file = create_temp_dir() + "/postings.bin";
    cleanup_temp_dir(fs::path(temp_file).parent_path().string());

    idx.save(temp_file);

    InvertedIndex idx2;
    idx2.load(temp_file);

    // Verify postings are preserved
    auto postings1 = idx.postings("apple");
    auto postings2 = idx2.postings("apple");
    REQUIRE(postings1 == postings2);

    auto postings3 = idx.postings("banana");
    auto postings4 = idx2.postings("banana");
    REQUIRE(postings3 == postings4);

    cleanup_temp_dir(fs::path(temp_file).parent_path().string());
}

TEST_CASE("SearchService::save() creates all three files")
{
    SearchService ss;
    ss.add_document(1, "hello world");
    ss.add_document(2, "world peace");

    std::string index_dir = create_temp_dir();

    ss.save(index_dir);

    std::string meta_path = index_dir + "/index_meta.json";
    std::string docs_path = index_dir + "/docs.jsonl";
    std::string postings_path = index_dir + "/postings.bin";

    REQUIRE(fs::exists(meta_path));
    REQUIRE(fs::exists(docs_path));
    REQUIRE(fs::exists(postings_path));

    cleanup_temp_dir(index_dir);
}

TEST_CASE("SearchService::load() restores index state")
{
    SearchService ss;
    ss.add_document(1, "hello world");
    ss.add_document(2, "world peace");
    ss.add_document(3, "hello there");

    std::string index_dir = create_temp_dir();

    ss.save(index_dir);

    // Create new service and load
    SearchService ss2;
    ss2.load(index_dir);

    // Verify search works
    auto results1 = ss.search("hello");
    auto results2 = ss2.search("hello");
    REQUIRE(results1 == results2);

    cleanup_temp_dir(index_dir);
}

TEST_CASE("save/load preserves docIds exactly")
{
    SearchService ss;
    ss.add_document(42, "test document");
    ss.add_document(100, "another test");
    ss.add_document(7, "third document");

    std::string index_dir = create_temp_dir();
    ss.save(index_dir);

    SearchService ss2;
    ss2.load(index_dir);

    // Verify all original docIds are present
    auto results = ss2.search("test");
    REQUIRE(std::find(results.begin(), results.end(), 42) != results.end());
    REQUIRE(std::find(results.begin(), results.end(), 100) != results.end());

    auto results2 = ss2.search("document");
    REQUIRE(std::find(results2.begin(), results2.end(), 42) != results2.end());
    REQUIRE(std::find(results2.begin(), results2.end(), 7) != results2.end());

    cleanup_temp_dir(index_dir);
}

TEST_CASE("save/load preserves BM25 corpus stats (N, avgdl)")
{
    SearchService ss;
    ss.add_document(1, "short");
    ss.add_document(2, "medium length text");
    ss.add_document(3, "this is a longer document with more words");

    std::string index_dir = create_temp_dir();
    ss.save(index_dir);

    SearchService ss2;
    ss2.load(index_dir);

    // Verify search results are identical (which implies stats are correct)
    auto results1 = ss.search_scored("text");
    auto results2 = ss2.search_scored("text");
    REQUIRE(results1.size() == results2.size());
    for (size_t i = 0; i < results1.size(); ++i)
    {
        REQUIRE(results1[i].first == results2[i].first);
        REQUIRE(std::abs(results1[i].second - results2[i].second) < 1e-9);
    }

    cleanup_temp_dir(index_dir);
}

TEST_CASE("save/load preserves document texts")
{
    SearchService ss;
    std::string text1 = "The quick brown fox jumps over the lazy dog.";
    std::string text2 = "PLM data migration: cleansing, mapping, validation.";
    ss.add_document(1, text1);
    ss.add_document(2, text2);

    std::string index_dir = create_temp_dir();
    ss.save(index_dir);

    SearchService ss2;
    ss2.load(index_dir);

    // Verify snippets contain original text content
    auto hits = ss2.search_with_snippets("fox");
    REQUIRE(hits.size() == 1);
    REQUIRE(hits[0].docId == 1);
    REQUIRE(hits[0].snippet.find("fox") != std::string::npos);

    auto hits2 = ss2.search_with_snippets("migration");
    REQUIRE(hits2.size() == 1);
    REQUIRE(hits2[0].docId == 2);
    REQUIRE(hits2[0].snippet.find("migration") != std::string::npos);

    cleanup_temp_dir(index_dir);
}
