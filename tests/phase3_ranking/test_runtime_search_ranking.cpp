#include "runtime_test_common.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <unistd.h>
#include <vector>

namespace fs = std::filesystem;

namespace
{

static std::string find_searchd_path()
{
    std::vector<std::string> candidates = {"./searchd", "./build/searchd"};
    for (const auto &c : candidates)
    {
        if (fs::exists(c))
        {
            return c;
        }
    }
    return "./searchd";
}

static std::string create_temp_dir(const std::string &prefix = "phase3_rt_")
{
    std::string base = "/tmp/" + prefix + std::to_string(getpid()) + "_";
    for (int i = 0; i < 1000; ++i)
    {
        std::string path = base + std::to_string(i);
        if (!fs::exists(path))
        {
            fs::create_directories(path);
            return path;
        }
    }
    throw std::runtime_error("Failed to create temp dir");
}

static void cleanup_temp_dir(const std::string &path)
{
    std::error_code ec;
    fs::remove_all(path, ec);
}

static void write_file(const std::string &path, const std::string &content)
{
    std::ofstream out(path);
    REQUIRE(out.good());
    out << content;
}

static std::vector<double> extract_scores(const std::string &json_body)
{
    std::vector<double> scores;
    const std::string key = "\"score\":";
    size_t pos = 0;
    while ((pos = json_body.find(key, pos)) != std::string::npos)
    {
        pos += key.size();
        char *end = nullptr;
        const double value = std::strtod(json_body.c_str() + static_cast<std::ptrdiff_t>(pos), &end);
        scores.push_back(value);
        if (end == nullptr)
        {
            break;
        }
        pos = static_cast<size_t>(end - json_body.c_str());
    }
    return scores;
}

static std::vector<int> extract_doc_ids(const std::string &json_body)
{
    std::vector<int> ids;
    const std::string key = "\"docId\":";
    size_t pos = 0;
    while ((pos = json_body.find(key, pos)) != std::string::npos)
    {
        pos += key.size();
        while (pos < json_body.size() && (json_body[pos] == ' ' || json_body[pos] == '\t'))
        {
            ++pos;
        }
        ids.push_back(std::stoi(json_body.substr(pos)));
        ++pos;
    }
    return ids;
}

} // namespace

TEST_CASE("Phase 3: /search returns results ordered by descending score", "[phase3][runtime][search]")
{
    const std::string searchd = find_searchd_path();
    const std::string docs_dir = create_temp_dir("phase3_docs_");
    const std::string out_dir = create_temp_dir("phase3_out_");
    const int port = 19020 + static_cast<int>(getpid() % 1000);

    write_file(docs_dir + "/a.txt", "alpha bravo alpha");
    write_file(docs_dir + "/b.txt", "alpha charlie");
    write_file(docs_dir + "/c.txt", "bravo charlie");

    auto [index_code, index_out] = runtime_test::run_command_capture_output(
        searchd + " --index --docs \"" + docs_dir + "\" --out \"" + out_dir + "\" 2>&1", 15);
    REQUIRE(index_code == 0);

    const std::string serve_cmd =
        "(" + searchd + " --serve --in \"" + out_dir + "\" --port " + std::to_string(port) +
        " 2>/dev/null &); pid=$!; sleep 3; curl -s 'http://127.0.0.1:" + std::to_string(port) +
        "/search?q=alpha'; kill $pid 2>/dev/null; wait $pid 2>/dev/null";

    auto [serve_code, body] = runtime_test::run_command_capture_output(serve_cmd, 20);
    REQUIRE(serve_code == 0);

    const auto scores = extract_scores(body);
    REQUIRE(scores.size() >= 2);
    for (size_t i = 1; i < scores.size(); ++i)
    {
        REQUIRE(scores[i - 1] >= scores[i]);
    }
    REQUIRE(body.find("\"score\"") != std::string::npos);

    cleanup_temp_dir(docs_dir);
    cleanup_temp_dir(out_dir);
}

TEST_CASE("Phase 3: /search truncates to top-k after ranking", "[phase3][runtime][search][k]")
{
    const std::string searchd = find_searchd_path();
    const std::string docs_dir = create_temp_dir("phase3_k_docs_");
    const std::string out_dir = create_temp_dir("phase3_k_out_");
    const int port = 19120 + static_cast<int>(getpid() % 1000);

    for (int i = 1; i <= 10; ++i)
    {
        write_file(docs_dir + "/doc" + std::to_string(i) + ".txt",
                   "rankterm token" + std::to_string(i) + " filler");
    }

    auto [index_code, index_out] = runtime_test::run_command_capture_output(
        searchd + " --index --docs \"" + docs_dir + "\" --out \"" + out_dir + "\" 2>&1", 20);
    REQUIRE(index_code == 0);

    const std::string full_cmd =
        "(" + searchd + " --serve --in \"" + out_dir + "\" --port " + std::to_string(port) +
        " 2>/dev/null &); pid=$!; sleep 3; curl -s 'http://127.0.0.1:" + std::to_string(port) +
        "/search?q=rankterm&k=50'; kill $pid 2>/dev/null; wait $pid 2>/dev/null";

    const std::string top_cmd =
        "(" + searchd + " --serve --in \"" + out_dir + "\" --port " + std::to_string(port) +
        " 2>/dev/null &); pid=$!; sleep 3; curl -s 'http://127.0.0.1:" + std::to_string(port) +
        "/search?q=rankterm&k=3'; kill $pid 2>/dev/null; wait $pid 2>/dev/null";

    auto [full_code, full_body] = runtime_test::run_command_capture_output(full_cmd, 25);
    auto [top_code, top_body] = runtime_test::run_command_capture_output(top_cmd, 25);
    REQUIRE(full_code == 0);
    REQUIRE(top_code == 0);

    const auto full_ids = extract_doc_ids(full_body);
    const auto top_ids = extract_doc_ids(top_body);
    REQUIRE(top_ids.size() == 3);
    REQUIRE(full_ids.size() >= 3);
    REQUIRE(top_ids == std::vector<int>(full_ids.begin(), full_ids.begin() + 3));

    cleanup_temp_dir(docs_dir);
    cleanup_temp_dir(out_dir);
}
