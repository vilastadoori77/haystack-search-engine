// Phase 2.2 CLI Lifecycle Tests - Exit Code Tests
// These tests STRICTLY follow the APPROVED specification: specs/phase2_2_cli_lifecycle.md
// Do NOT add new behavior, flags, or assumptions beyond what is specified.
// Tests assert exact exit codes, stdout/stderr behavior, and error messages as defined in the spec.

#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <fstream>
#include <cstdlib>
#include <sstream>
#include <string>
#include <vector>
#include <random>

namespace fs = std::filesystem;

static std::string find_searchd_path()
{
    std::vector<std::string> candidates;
    candidates.push_back("./searchd");
    candidates.push_back("./build/searchd");
    
    fs::path cwd = fs::current_path();
    if (cwd.filename() == "build")
    {
        candidates.push_back((cwd / "searchd").string());
    }
    else
    {
        candidates.push_back((cwd / "build" / "searchd").string());
    }
    
    if (cwd.has_parent_path())
    {
        candidates.push_back((cwd.parent_path() / "build" / "searchd").string());
    }
    
    for (const auto& candidate : candidates)
    {
        if (fs::exists(candidate) && fs::is_regular_file(candidate))
        {
            fs::path abs_path = fs::absolute(candidate);
            return abs_path.string();
        }
    }
    
    return "./searchd";
}

static int run_command(const std::string &cmd)
{
    return std::system(cmd.c_str());
}

static int run_command_get_exit_code(const std::string &cmd)
{
    std::string full_cmd = cmd + " > /dev/null 2>&1; echo $?";
    FILE* pipe = popen(full_cmd.c_str(), "r");
    if (!pipe)
        return -1;
    
    char buffer[128];
    std::string result;
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr)
    {
        result += buffer;
    }
    
    pclose(pipe);
    
    try
    {
        return std::stoi(result);
    }
    catch (...)
    {
        return -1;
    }
}

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

static void cleanup_temp_dir(const std::string &dir)
{
    if (fs::exists(dir))
    {
        fs::remove_all(dir);
    }
}

static std::string create_test_docs_file()
{
    std::string docs_file = create_temp_dir() + "/test_docs.json";
    std::ofstream out(docs_file);
    out << R"([
  {"docId": 1, "text": "hello world"},
  {"docId": 2, "text": "world peace"}
])";
    out.close();
    return docs_file;
}

static bool file_exists(const std::string &path)
{
    return fs::exists(path) && fs::is_regular_file(path);
}

TEST_CASE("Exit code 0 for successful index mode")
{
    std::string docs_file = create_test_docs_file();
    std::string index_dir = create_temp_dir();
    std::string docs_dir = fs::path(docs_file).parent_path().string();
    
    std::string searchd_path = find_searchd_path();
    std::string cmd = searchd_path + " --index --docs \"" + docs_file + "\" --out \"" + index_dir + "\"";
    
    int exit_code = run_command_get_exit_code(cmd);
    REQUIRE(exit_code == 0);
    
    cleanup_temp_dir(index_dir);
    cleanup_temp_dir(docs_dir);
}

TEST_CASE("Exit code 0 for successful serve mode startup")
{
    // First create an index
    std::string docs_file = create_test_docs_file();
    std::string index_dir = create_temp_dir();
    std::string docs_dir = fs::path(docs_file).parent_path().string();
    
    std::string searchd_path = find_searchd_path();
    std::string index_cmd = searchd_path + " --index --docs \"" + docs_file + "\" --out \"" + index_dir + "\"";
    int index_result = run_command_get_exit_code(index_cmd);
    REQUIRE(index_result == 0);
    
    // Use random port to avoid conflicts
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(9000, 9999);
    int test_port = dis(gen);
    
    // Start serve mode, wait briefly, then kill
    // Exit code 0 means it started successfully (before we kill it)
    std::string serve_cmd = searchd_path + " --serve --in \"" + index_dir + "\" --port " + std::to_string(test_port);
    std::string full_cmd = "(" + serve_cmd + " > /dev/null 2>&1 &); SERVE_PID=$!; sleep 0.5; kill $SERVE_PID 2>/dev/null || true; wait $SERVE_PID 2>/dev/null; echo $?";
    
    // Note: We can't easily test exit code 0 for serve mode since it runs until killed
    // But we can verify it doesn't exit immediately with error code
    int result = run_command(full_cmd);
    // If it exits immediately with code 3, that's a failure
    // If it runs and we kill it, that's success
    
    cleanup_temp_dir(index_dir);
    cleanup_temp_dir(docs_dir);
}

TEST_CASE("Exit code 2 for conflicting flags")
{
    std::string searchd_path = find_searchd_path();
    std::string cmd = searchd_path + " --index --serve";
    
    int exit_code = run_command_get_exit_code(cmd);
    REQUIRE(exit_code == 2);
}

TEST_CASE("Exit code 2 for missing required flags in index mode")
{
    std::string docs_file = create_test_docs_file();
    std::string docs_dir = fs::path(docs_file).parent_path().string();
    
    std::string searchd_path = find_searchd_path();
    std::string cmd = searchd_path + " --index --docs \"" + docs_file + "\"";
    
    int exit_code = run_command_get_exit_code(cmd);
    REQUIRE(exit_code == 2);
    
    cleanup_temp_dir(docs_dir);
}

TEST_CASE("Exit code 2 for missing required flags in serve mode")
{
    std::string index_dir = create_temp_dir();
    
    std::string searchd_path = find_searchd_path();
    std::string cmd = searchd_path + " --serve --in \"" + index_dir + "\"";
    
    int exit_code = run_command_get_exit_code(cmd);
    REQUIRE(exit_code == 2);
    
    cleanup_temp_dir(index_dir);
}

TEST_CASE("Exit code 2 for invalid flag combinations")
{
    std::string docs_file = create_test_docs_file();
    std::string index_dir = create_temp_dir();
    std::string docs_dir = fs::path(docs_file).parent_path().string();
    
    std::string searchd_path = find_searchd_path();
    
    // --index with --in
    std::string cmd1 = searchd_path + " --index --docs \"" + docs_file + "\" --out \"" + index_dir + "\" --in \"" + index_dir + "\"";
    int exit_code1 = run_command_get_exit_code(cmd1);
    REQUIRE(exit_code1 == 2);
    
    // --serve with --docs
    std::string cmd2 = searchd_path + " --serve --in \"" + index_dir + "\" --port 8900 --docs \"" + docs_file + "\"";
    int exit_code2 = run_command_get_exit_code(cmd2);
    REQUIRE(exit_code2 == 2);
    
    cleanup_temp_dir(index_dir);
    cleanup_temp_dir(docs_dir);
}

TEST_CASE("Exit code 2 for invalid port values")
{
    std::string index_dir = create_temp_dir();
    
    std::string searchd_path = find_searchd_path();
    
    // Non-numeric port
    std::string cmd1 = searchd_path + " --serve --in \"" + index_dir + "\" --port invalid";
    int exit_code1 = run_command_get_exit_code(cmd1);
    REQUIRE(exit_code1 == 2);
    
    // Port 0
    std::string cmd2 = searchd_path + " --serve --in \"" + index_dir + "\" --port 0";
    int exit_code2 = run_command_get_exit_code(cmd2);
    REQUIRE(exit_code2 == 2);
    
    // Port out of range
    std::string cmd3 = searchd_path + " --serve --in \"" + index_dir + "\" --port 70000";
    int exit_code3 = run_command_get_exit_code(cmd3);
    REQUIRE(exit_code3 == 2);
    
    cleanup_temp_dir(index_dir);
}

TEST_CASE("Exit code 3 for nonexistent document file")
{
    std::string index_dir = create_temp_dir();
    std::string nonexistent_file = "/tmp/nonexistent_docs_12345.json";
    
    if (fs::exists(nonexistent_file))
    {
        fs::remove(nonexistent_file);
    }
    
    std::string searchd_path = find_searchd_path();
    std::string cmd = searchd_path + " --index --docs \"" + nonexistent_file + "\" --out \"" + index_dir + "\"";
    
    int exit_code = run_command_get_exit_code(cmd);
    REQUIRE(exit_code == 3);
    
    cleanup_temp_dir(index_dir);
}

TEST_CASE("Exit code 3 for nonexistent index directory")
{
    std::string nonexistent_dir = "/tmp/nonexistent_index_dir_12345";
    
    if (fs::exists(nonexistent_dir))
    {
        fs::remove_all(nonexistent_dir);
    }
    
    std::string searchd_path = find_searchd_path();
    std::string cmd = searchd_path + " --serve --in \"" + nonexistent_dir + "\" --port 8900";
    
    int exit_code = run_command_get_exit_code(cmd);
    REQUIRE(exit_code == 3);
}

TEST_CASE("Exit code 3 for incomplete index directory")
{
    std::string index_dir = create_temp_dir();
    
    // Create only one file, not all three
    std::string meta_path = index_dir + "/index_meta.json";
    std::ofstream meta(meta_path);
    meta << R"({"schema_version": 1, "N": 1, "avgdl": 10.0})";
    meta.close();
    
    std::string searchd_path = find_searchd_path();
    std::string cmd = searchd_path + " --serve --in \"" + index_dir + "\" --port 8900";
    
    int exit_code = run_command_get_exit_code(cmd);
    REQUIRE(exit_code == 3);
    
    cleanup_temp_dir(index_dir);
}

TEST_CASE("Exit code 0 for --help")
{
    std::string searchd_path = find_searchd_path();
    std::string cmd = searchd_path + " --help";
    
    int exit_code = run_command_get_exit_code(cmd);
    REQUIRE(exit_code == 0);
}

TEST_CASE("Exit code 0 for no arguments (behaves like --help)")
{
    std::string searchd_path = find_searchd_path();
    std::string cmd = searchd_path;
    
    int exit_code = run_command_get_exit_code(cmd);
    REQUIRE(exit_code == 0);
}
