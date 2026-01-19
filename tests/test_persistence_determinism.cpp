#include <catch2/catch_test_macros.hpp>
#include "core/search_service.h"
#include <filesystem>
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

TEST_CASE("Query results order is identical before and after save/load")
{
    SearchService ss;
    ss.add_document(1, "apple banana cherry");
    ss.add_document(2, "banana cherry date");
    ss.add_document(3, "cherry date elderberry");
    ss.add_document(4, "date elderberry fig");

    std::string query = "banana cherry";

    // Get results before save
    auto results_before = ss.search(query);

    std::string index_dir = create_temp_dir();
    ss.save(index_dir);

    SearchService ss2;
    ss2.load(index_dir);

    // Get results after load
    auto results_after = ss2.search(query);

    // Verify identical order
    REQUIRE(results_before.size() == results_after.size());
    for (size_t i = 0; i < results_before.size(); ++i)
    {
        REQUIRE(results_before[i] == results_after[i]);
    }

    cleanup_temp_dir(index_dir);
}

TEST_CASE("BM25 scores match within tolerance (1e-9) after save/load")
{
    SearchService ss;
    ss.add_document(1, "the quick brown fox jumps over the lazy dog");
    ss.add_document(2, "the lazy dog sleeps all day");
    ss.add_document(3, "a quick fox is better than a lazy dog");

    std::string query = "quick fox";

    // Get scored results before save
    auto scored_before = ss.search_scored(query);

    std::string index_dir = create_temp_dir();
    ss.save(index_dir);

    SearchService ss2;
    ss2.load(index_dir);

    // Get scored results after load
    auto scored_after = ss2.search_scored(query);

    // Verify same number of results
    REQUIRE(scored_before.size() == scored_after.size());

    // Verify scores match within tolerance
    const double tolerance = 1e-9;
    for (size_t i = 0; i < scored_before.size(); ++i)
    {
        REQUIRE(scored_before[i].first == scored_after[i].first);
        double diff = std::abs(scored_before[i].second - scored_after[i].second);
        REQUIRE(diff < tolerance);
    }

    cleanup_temp_dir(index_dir);
}

TEST_CASE("Snippets are identical after save/load")
{
    SearchService ss;
    std::string doc1_text = "Teamcenter migration guide: map attributes, validate schema, run dry-run.";
    std::string doc2_text = "PLM data migration: cleansing, mapping, validation.";
    ss.add_document(1, doc1_text);
    ss.add_document(2, doc2_text);

    std::string query = "migration schema";

    // Get snippets before save
    auto snippets_before = ss.search_with_snippets(query);

    std::string index_dir = create_temp_dir();
    ss.save(index_dir);

    SearchService ss2;
    ss2.load(index_dir);

    // Get snippets after load
    auto snippets_after = ss2.search_with_snippets(query);

    // Verify identical snippets
    REQUIRE(snippets_before.size() == snippets_after.size());
    for (size_t i = 0; i < snippets_before.size(); ++i)
    {
        REQUIRE(snippets_before[i].docId == snippets_after[i].docId);
        REQUIRE(snippets_before[i].snippet == snippets_after[i].snippet);
        REQUIRE(std::abs(snippets_before[i].score - snippets_after[i].score) < 1e-9);
    }

    cleanup_temp_dir(index_dir);
}

TEST_CASE("Multiple save/load cycles maintain correctness")
{
    SearchService ss;
    ss.add_document(1, "alpha beta gamma");
    ss.add_document(2, "beta gamma delta");
    ss.add_document(3, "gamma delta epsilon");

    std::string query = "beta gamma";

    // Initial search
    auto initial_results = ss.search_scored(query);

    // First save/load cycle
    std::string index_dir = create_temp_dir();
    ss.save(index_dir);

    SearchService ss2;
    ss2.load(index_dir);
    auto after_first = ss2.search_scored(query);

    // Second save/load cycle
    std::string index_dir2 = create_temp_dir();
    ss2.save(index_dir2);

    SearchService ss3;
    ss3.load(index_dir2);
    auto after_second = ss3.search_scored(query);

    // Verify all three are identical
    REQUIRE(initial_results.size() == after_first.size());
    REQUIRE(after_first.size() == after_second.size());

    for (size_t i = 0; i < initial_results.size(); ++i)
    {
        REQUIRE(initial_results[i].first == after_first[i].first);
        REQUIRE(after_first[i].first == after_second[i].first);
        REQUIRE(std::abs(initial_results[i].second - after_first[i].second) < 1e-9);
        REQUIRE(std::abs(after_first[i].second - after_second[i].second) < 1e-9);
    }

    cleanup_temp_dir(index_dir);
    cleanup_temp_dir(index_dir2);
}
