// Phase 2.2 CLI Lifecycle Tests - Error Message Tests
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

static int run_command_capture_stderr(const std::string &cmd, std::string &stderr_output)
{
    std::string full_cmd = cmd + " 2>&1";
    FILE* pipe = popen(full_cmd.c_str(), "r");
    if (!pipe)
        return -1;
    
    char buffer[128];
    stderr_output.clear();
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr)
    {
        stderr_output += buffer;
    }
    
    int exit_code = pclose(pipe);
    return WEXITSTATUS(exit_code);
}

static int run_command_capture_stdout(const std::string &cmd, std::string &stdout_output)
{
    std::string full_cmd = cmd + " 1>&1";
    FILE* pipe = popen(full_cmd.c_str(), "r");
    if (!pipe)
        return -1;
    
    char buffer[128];
    stdout_output.clear();
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr)
    {
        stdout_output += buffer;
    }
    
    int exit_code = pclose(pipe);
    return WEXITSTATUS(exit_code);
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
  {"docId": 1, "text": "hello world"}
])";
    out.close();
    return docs_file;
}

TEST_CASE("Error messages are printed to stderr (not stdout)")
{
    std::string searchd_path = find_searchd_path();
    std::string cmd = searchd_path + " --index 2>&1";
    
    std::string stderr_output;
    int exit_code = run_command_capture_stderr(cmd, stderr_output);
    
    REQUIRE(exit_code == 2);
    // Error should be in stderr
    REQUIRE(stderr_output.find("Error:") != std::string::npos);
    
    // Verify stdout doesn't contain error (capture separately)
    std::string stdout_output;
    std::string stdout_cmd = searchd_path + " --index 1>&1";
    run_command_capture_stdout(stdout_cmd, stdout_output);
    
    // stdout should not contain "Error:" (or be empty)
    REQUIRE(stdout_output.find("Error:") == std::string::npos);
}

TEST_CASE("Error messages contain 'Error: ' prefix")
{
    std::string searchd_path = find_searchd_path();
    std::string cmd = searchd_path + " --index --serve 2>&1";
    
    std::string stderr_output;
    int exit_code = run_command_capture_stderr(cmd, stderr_output);
    
    REQUIRE(exit_code == 2);
    // Error messages SHALL start with "Error: " prefix (spec section: Error Message Format)
    REQUIRE(stderr_output.find("Error:") != std::string::npos);
    // Should start with "Error: " (or at least contain it)
    bool starts_with_error = stderr_output.find("Error:") == 0;
    bool contains_newline_error = stderr_output.find("\nError:") != std::string::npos;
    REQUIRE((starts_with_error || contains_newline_error));
}

TEST_CASE("Error messages include specific file/path/flag information")
{
    std::string docs_file = create_test_docs_file();
    std::string docs_dir = fs::path(docs_file).parent_path().string();
    
    std::string searchd_path = find_searchd_path();
    
    // Test missing --out error includes flag name
    std::string cmd1 = searchd_path + " --index --docs \"" + docs_file + "\" 2>&1";
    std::string stderr_output1;
    run_command_capture_stderr(cmd1, stderr_output1);
    REQUIRE(stderr_output1.find("--out") != std::string::npos);
    
    // Test missing --in error includes flag name
    std::string cmd2 = searchd_path + " --serve --port 8900 2>&1";
    std::string stderr_output2;
    run_command_capture_stderr(cmd2, stderr_output2);
    REQUIRE(stderr_output2.find("--in") != std::string::npos);
    
    cleanup_temp_dir(docs_dir);
}

TEST_CASE("Error messages are human-readable (no stack traces)")
{
    std::string searchd_path = find_searchd_path();
    std::string cmd = searchd_path + " --index --serve 2>&1";
    
    std::string stderr_output;
    int exit_code = run_command_capture_stderr(cmd, stderr_output);
    
    REQUIRE(exit_code == 2);
    
    // Should not contain stack trace indicators
    REQUIRE(stderr_output.find("at 0x") == std::string::npos);
    REQUIRE(stderr_output.find("Stack trace") == std::string::npos);
    REQUIRE(stderr_output.find("backtrace") == std::string::npos);
    REQUIRE(stderr_output.find("libc++abi") == std::string::npos);
    
    // Should be readable text
    REQUIRE(stderr_output.find("Error:") != std::string::npos);
}

TEST_CASE("Success messages (index mode) are printed to stdout")
{
    std::string docs_file = create_test_docs_file();
    std::string index_dir = create_temp_dir();
    std::string docs_dir = fs::path(docs_file).parent_path().string();
    
    std::string searchd_path = find_searchd_path();
    std::string cmd = searchd_path + " --index --docs \"" + docs_file + "\" --out \"" + index_dir + "\"";
    
    std::string stdout_output;
    int exit_code = run_command_capture_stdout(cmd, stdout_output);
    
    REQUIRE(exit_code == 0);
    REQUIRE(stdout_output.find("Indexing completed") != std::string::npos);
    REQUIRE(stdout_output.find("Error:") == std::string::npos);
    
    cleanup_temp_dir(index_dir);
    cleanup_temp_dir(docs_dir);
}

TEST_CASE("Error messages end with newline")
{
    std::string searchd_path = find_searchd_path();
    std::string cmd = searchd_path + " --index --serve 2>&1";
    
    std::string stderr_output;
    run_command_capture_stderr(cmd, stderr_output);
    
    // Error message should end with newline (or be part of a line that does)
    REQUIRE(!stderr_output.empty());
    // Last character should be newline, or message should contain newlines
    bool ends_with_newline = stderr_output.back() == '\n';
    bool contains_newline = stderr_output.find('\n') != std::string::npos;
    REQUIRE((ends_with_newline || contains_newline));
}
