// Unit tests for Phase 2.5 file_scanner (scan_inputs).
#include "ingestion/file_scanner.hpp"
#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <fstream>
#include <string>
#include <unistd.h>

namespace fs = std::filesystem;

static std::string create_temp_dir(const std::string& prefix)
{
    std::string base = fs::temp_directory_path().string() + "/" + prefix;
    pid_t pid = getpid();
    static int counter = 0;
    for (int i = 0; i < 1000; ++i)
    {
        std::string dir = base + std::to_string(pid) + "_" + std::to_string(counter++);
        if (!fs::exists(dir))
        {
            fs::create_directories(dir);
            return dir;
        }
    }
    throw std::runtime_error("Could not create temp directory");
}

static void write_file(const std::string& path, const std::string& content)
{
    std::ofstream f(path);
    f << content;
}

TEST_CASE("scan_inputs: non-existent root returns empty", "[phase2_5][file_scanner]")
{
    auto paths = haystack::phase2_5::scan_inputs(fs::path("/nonexistent/dir/12345"));
    REQUIRE(paths.empty());
}

TEST_CASE("scan_inputs: file as root returns empty", "[phase2_5][file_scanner]")
{
    std::string dir = create_temp_dir("scan_file_");
    std::string fpath = dir + "/f.txt";
    write_file(fpath, "x");
    auto paths = haystack::phase2_5::scan_inputs(fs::path(fpath));
    REQUIRE(paths.empty());
    fs::remove_all(dir);
}

TEST_CASE("scan_inputs: empty directory returns empty", "[phase2_5][file_scanner]")
{
    std::string dir = create_temp_dir("scan_empty_");
    auto paths = haystack::phase2_5::scan_inputs(fs::path(dir));
    REQUIRE(paths.empty());
    fs::remove_all(dir);
}

TEST_CASE("scan_inputs: only .txt and .pdf; .docx ignored", "[phase2_5][file_scanner]")
{
    std::string dir = create_temp_dir("scan_ext_");
    write_file(dir + "/a.txt", "a");
    write_file(dir + "/b.pdf", "pdf");
    write_file(dir + "/c.docx", "doc");
    auto paths = haystack::phase2_5::scan_inputs(fs::path(dir));
    REQUIRE(paths.size() == 2u);
    REQUIRE(paths[0].filename().string() == "a.txt");
    REQUIRE(paths[1].filename().string() == "b.pdf");
    fs::remove_all(dir);
}

TEST_CASE("scan_inputs: case-sensitive extensions; .TXT not included", "[phase2_5][file_scanner]")
{
    std::string dir = create_temp_dir("scan_case_");
    write_file(dir + "/lower.txt", "x");
    write_file(dir + "/upper.TXT", "x");
    auto paths = haystack::phase2_5::scan_inputs(fs::path(dir));
    REQUIRE(paths.size() == 1u);
    REQUIRE(paths[0].filename().string() == "lower.txt");
    fs::remove_all(dir);
}

TEST_CASE("scan_inputs: recursive; files in subdirs included", "[phase2_5][file_scanner]")
{
    std::string dir = create_temp_dir("scan_rec_");
    fs::create_directories(dir + "/sub");
    write_file(dir + "/root.txt", "r");
    write_file(dir + "/sub/nested.txt", "n");
    auto paths = haystack::phase2_5::scan_inputs(fs::path(dir));
    REQUIRE(paths.size() == 2u);
    // Sorted by full path: .../root.txt < .../sub/nested.txt
    REQUIRE(paths[0].filename().string() == "root.txt");
    REQUIRE(paths[1].filename().string() == "nested.txt");
    fs::remove_all(dir);
}

TEST_CASE("scan_inputs: sorted by normalized absolute path (UTF-8 byte order)", "[phase2_5][file_scanner]")
{
    std::string dir = create_temp_dir("scan_sort_");
    write_file(dir + "/a_first.txt", "a");
    write_file(dir + "/b_second.txt", "b");
    write_file(dir + "/c_third.txt", "c");
    auto paths = haystack::phase2_5::scan_inputs(fs::path(dir));
    REQUIRE(paths.size() == 3u);
    REQUIRE(paths[0].filename().string() == "a_first.txt");
    REQUIRE(paths[1].filename().string() == "b_second.txt");
    REQUIRE(paths[2].filename().string() == "c_third.txt");
    for (const auto& p : paths)
        REQUIRE(p.is_absolute());
    fs::remove_all(dir);
}

TEST_CASE("scan_inputs: mixed .txt and .pdf in deterministic order", "[phase2_5][file_scanner]")
{
    std::string dir = create_temp_dir("scan_mixed_");
    write_file(dir + "/1.txt", "t");
    write_file(dir + "/2.pdf", "%PDF");
    write_file(dir + "/3.txt", "t");
    auto paths = haystack::phase2_5::scan_inputs(fs::path(dir));
    REQUIRE(paths.size() == 3u);
    REQUIRE(paths[0].filename().string() == "1.txt");
    REQUIRE(paths[1].filename().string() == "2.pdf");
    REQUIRE(paths[2].filename().string() == "3.txt");
    fs::remove_all(dir);
}
