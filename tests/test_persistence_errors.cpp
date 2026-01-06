#include <catch2/catch_test_macros.hpp>
#include "core/search_service.h"
#include "core/inverted_index.h"
#include <filesystem>
#include <fstream>
#include <stdexcept>

namespace fs = std::filesystem;

static std::string create_temp_dir()
{
    std::string base = "/tmp/haystack_test_";
    for (int i = 0; i < 1000; ++i)
    {
        std::string dir = base + std::to_string(i);
        if (!fs::exists(dir))
        {
            fs::create_directories(dir);
            return dir;
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

TEST_CASE("Loading unsupported schema_version throws with clear error")
{
    std::string index_dir = create_temp_dir();

    // Create index_meta.json with unsupported version
    std::string meta_path = index_dir + "/index_meta.json";
    std::ofstream meta(meta_path);
    meta << R"({"schema_version": 2, "N": 1, "avgdl": 10.0})";
    meta.close();

    // Create minimal other files
    std::string docs_path = index_dir + "/docs.jsonl";
    std::ofstream docs(docs_path);
    docs << R"({"docId": 1, "text": "test"})" << std::endl;
    docs.close();

    std::string postings_path = index_dir + "/postings.bin";
    std::ofstream postings(postings_path, std::ios::binary);
    postings.write("dummy", 5);
    postings.close();

    SearchService ss;
    REQUIRE_THROWS_AS(ss.load(index_dir), std::runtime_error);
    
    try
    {
        ss.load(index_dir);
    }
    catch (const std::runtime_error& e)
    {
        std::string msg = e.what();
        REQUIRE(msg.find("Unsupported schema version") != std::string::npos);
        REQUIRE(msg.find("Expected: 1") != std::string::npos);
    }

    cleanup_temp_dir(index_dir);
}

TEST_CASE("Loading missing index_meta.json throws with clear error")
{
    std::string index_dir = create_temp_dir();

    // Create only docs.jsonl and postings.bin (missing index_meta.json)
    std::string docs_path = index_dir + "/docs.jsonl";
    std::ofstream docs(docs_path);
    docs << R"({"docId": 1, "text": "test"})" << std::endl;
    docs.close();

    std::string postings_path = index_dir + "/postings.bin";
    std::ofstream postings(postings_path, std::ios::binary);
    postings.write("dummy", 5);
    postings.close();

    SearchService ss;
    REQUIRE_THROWS_AS(ss.load(index_dir), std::runtime_error);
    
    try
    {
        ss.load(index_dir);
    }
    catch (const std::runtime_error& e)
    {
        std::string msg = e.what();
        REQUIRE(msg.find("Index file not found") != std::string::npos);
        REQUIRE(msg.find("index_meta.json") != std::string::npos);
    }

    cleanup_temp_dir(index_dir);
}

TEST_CASE("Loading missing docs.jsonl throws with clear error")
{
    std::string index_dir = create_temp_dir();

    // Create only index_meta.json and postings.bin (missing docs.jsonl)
    std::string meta_path = index_dir + "/index_meta.json";
    std::ofstream meta(meta_path);
    meta << R"({"schema_version": 1, "N": 1, "avgdl": 10.0})";
    meta.close();

    std::string postings_path = index_dir + "/postings.bin";
    std::ofstream postings(postings_path, std::ios::binary);
    postings.write("dummy", 5);
    postings.close();

    SearchService ss;
    REQUIRE_THROWS_AS(ss.load(index_dir), std::runtime_error);
    
    try
    {
        ss.load(index_dir);
    }
    catch (const std::runtime_error& e)
    {
        std::string msg = e.what();
        REQUIRE(msg.find("Index file not found") != std::string::npos);
        REQUIRE(msg.find("docs.jsonl") != std::string::npos);
    }

    cleanup_temp_dir(index_dir);
}

TEST_CASE("Loading missing postings.bin throws with clear error")
{
    std::string index_dir = create_temp_dir();

    // Create only index_meta.json and docs.jsonl (missing postings.bin)
    std::string meta_path = index_dir + "/index_meta.json";
    std::ofstream meta(meta_path);
    meta << R"({"schema_version": 1, "N": 1, "avgdl": 10.0})";
    meta.close();

    std::string docs_path = index_dir + "/docs.jsonl";
    std::ofstream docs(docs_path);
    docs << R"({"docId": 1, "text": "test"})" << std::endl;
    docs.close();

    SearchService ss;
    REQUIRE_THROWS_AS(ss.load(index_dir), std::runtime_error);
    
    try
    {
        ss.load(index_dir);
    }
    catch (const std::runtime_error& e)
    {
        std::string msg = e.what();
        REQUIRE(msg.find("Index file not found") != std::string::npos);
        REQUIRE(msg.find("postings.bin") != std::string::npos);
    }

    cleanup_temp_dir(index_dir);
}

TEST_CASE("Loading corrupted index_meta.json throws with clear error")
{
    std::string index_dir = create_temp_dir();

    // Create corrupted JSON (invalid syntax)
    std::string meta_path = index_dir + "/index_meta.json";
    std::ofstream meta(meta_path);
    meta << R"({invalid json syntax)";
    meta.close();

    std::string docs_path = index_dir + "/docs.jsonl";
    std::ofstream docs(docs_path);
    docs << R"({"docId": 1, "text": "test"})" << std::endl;
    docs.close();

    std::string postings_path = index_dir + "/postings.bin";
    std::ofstream postings(postings_path, std::ios::binary);
    postings.write("dummy", 5);
    postings.close();

    SearchService ss;
    REQUIRE_THROWS_AS(ss.load(index_dir), std::runtime_error);
    
    try
    {
        ss.load(index_dir);
    }
    catch (const std::runtime_error& e)
    {
        std::string msg = e.what();
        REQUIRE(msg.find("Failed to parse index file") != std::string::npos);
        REQUIRE(msg.find("index_meta.json") != std::string::npos);
    }

    cleanup_temp_dir(index_dir);
}

TEST_CASE("Loading corrupted postings.bin throws with clear error")
{
    std::string index_dir = create_temp_dir();

    // Create valid metadata and docs
    std::string meta_path = index_dir + "/index_meta.json";
    std::ofstream meta(meta_path);
    meta << R"({"schema_version": 1, "N": 1, "avgdl": 10.0})";
    meta.close();

    std::string docs_path = index_dir + "/docs.jsonl";
    std::ofstream docs(docs_path);
    docs << R"({"docId": 1, "text": "test"})" << std::endl;
    docs.close();

    // Create corrupted binary (too short, invalid format)
    std::string postings_path = index_dir + "/postings.bin";
    std::ofstream postings(postings_path, std::ios::binary);
    postings.write("x", 1);  // Invalid format
    postings.close();

    SearchService ss;
    REQUIRE_THROWS_AS(ss.load(index_dir), std::runtime_error);
    
    try
    {
        ss.load(index_dir);
    }
    catch (const std::runtime_error& e)
    {
        std::string msg = e.what();
        REQUIRE(msg.find("Failed to parse index file") != std::string::npos);
        REQUIRE(msg.find("postings.bin") != std::string::npos);
    }

    cleanup_temp_dir(index_dir);
}
