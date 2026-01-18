// Phase 2.2 CLI Lifecycle Tests - Unit Tests
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

    for (const auto &candidate : candidates)
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

static int run_command_capture_stderr(const std::string &cmd, std::string &stderr_output)
{
    // Redirect stderr to stdout so we can capture it with popen
    std::string full_cmd = cmd + " 2>&1";

    FILE *pipe = popen(full_cmd.c_str(), "r");
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

TEST_CASE("Flag exclusivity: --index and --serve together fails with exit code 2")
{
    std::string searchd_path = find_searchd_path();
    std::string cmd = searchd_path + " --index --serve";

    std::string stderr_output;
    int exit_code = run_command_capture_stderr(cmd, stderr_output);

    REQUIRE(exit_code == 2);
    // Exact error message from spec: "Error: --index and --serve cannot be used together\n"
    REQUIRE(stderr_output.find("Error: --index and --serve cannot be used together") != std::string::npos);
}

TEST_CASE("Index mode: missing --out flag exits with code 2")
{
    std::string docs_file = create_test_docs_file();
    std::string docs_dir = fs::path(docs_file).parent_path().string();

    std::string searchd_path = find_searchd_path();
    std::string cmd = searchd_path + " --index --docs \"" + docs_file + "\"";

    std::string stderr_output;
    int exit_code = run_command_capture_stderr(cmd, stderr_output);

    REQUIRE(exit_code == 2);
    // Exact error message from spec: "Error: --out <index_dir> is required when using --index mode\n"
    REQUIRE(stderr_output.find("Error: --out <index_dir> is required when using --index mode") != std::string::npos);

    cleanup_temp_dir(docs_dir);
}

TEST_CASE("Index mode: missing --docs flag exits with code 2")
{
    std::string index_dir = create_temp_dir();

    std::string searchd_path = find_searchd_path();
    std::string cmd = searchd_path + " --index --out \"" + index_dir + "\" ";

    std::string stderr_output;
    int exit_code = run_command_capture_stderr(cmd, stderr_output);

    REQUIRE(exit_code == 2);
    // Exact error message from spec: "Error: --docs <path> is required when using --index mode\n"
    REQUIRE(stderr_output.find("Error: --docs <path> is required when using --index mode") != std::string::npos);

    cleanup_temp_dir(index_dir);
}

TEST_CASE("Serve mode: missing --in flag exits with code 2")
{
    std::string searchd_path = find_searchd_path();
    std::string cmd = searchd_path + " --serve --port 8900 ";

    std::string stderr_output;
    int exit_code = run_command_capture_stderr(cmd, stderr_output);

    REQUIRE(exit_code == 2);
    // Exact error message from spec: "Error: --in <index_dir> is required when using --serve mode\n"
    REQUIRE(stderr_output.find("Error: --in <index_dir> is required when using --serve mode") != std::string::npos);
}

TEST_CASE("Serve mode: missing --port flag exits with code 2")
{
    std::string index_dir = create_temp_dir();

    std::string searchd_path = find_searchd_path();
    std::string cmd = searchd_path + " --serve --in \"" + index_dir + "\" ";

    std::string stderr_output;
    int exit_code = run_command_capture_stderr(cmd, stderr_output);

    REQUIRE(exit_code == 2);
    // Exact error message from spec: "Error: --port <port> is required when using --serve mode\n"
    REQUIRE(stderr_output.find("Error: --port <port> is required when using --serve mode") != std::string::npos);

    cleanup_temp_dir(index_dir);
}

TEST_CASE("Invalid flag combination: --index with --in exits with code 2")
{
    std::string docs_file = create_test_docs_file();
    std::string index_dir = create_temp_dir();
    std::string docs_dir = fs::path(docs_file).parent_path().string();

    std::string searchd_path = find_searchd_path();
    std::string cmd = searchd_path + " --index --docs \"" + docs_file + "\" --out \"" + index_dir + "\" --in \"" + index_dir + "\" ";

    std::string stderr_output;
    int exit_code = run_command_capture_stderr(cmd, stderr_output);

    REQUIRE(exit_code == 2);
    // Exact error message from spec: "Error: --in cannot be used with --index mode\n"
    REQUIRE(stderr_output.find("Error: --in cannot be used with --index mode") != std::string::npos);

    cleanup_temp_dir(index_dir);
    cleanup_temp_dir(docs_dir);
}

TEST_CASE("Invalid flag combination: --index with --port exits with code 2")
{
    std::string docs_file = create_test_docs_file();
    std::string index_dir = create_temp_dir();
    std::string docs_dir = fs::path(docs_file).parent_path().string();

    std::string searchd_path = find_searchd_path();
    std::string cmd = searchd_path + " --index --docs \"" + docs_file + "\" --out \"" + index_dir + "\" --port 8900 ";

    std::string stderr_output;
    int exit_code = run_command_capture_stderr(cmd, stderr_output);

    REQUIRE(exit_code == 2);
    REQUIRE(stderr_output.find("Error:") != std::string::npos);

    cleanup_temp_dir(index_dir);
    cleanup_temp_dir(docs_dir);
}

TEST_CASE("Invalid flag combination: --serve with --docs exits with code 2")
{
    std::string docs_file = create_test_docs_file();
    std::string index_dir = create_temp_dir();
    std::string docs_dir = fs::path(docs_file).parent_path().string();

    std::string searchd_path = find_searchd_path();
    std::string cmd = searchd_path + " --serve --in \"" + index_dir + "\" --port 8900 --docs \"" + docs_file + "\" ";

    std::string stderr_output;
    int exit_code = run_command_capture_stderr(cmd, stderr_output);

    REQUIRE(exit_code == 2);
    // Exact error message from spec: "Error: --docs cannot be used with --serve mode\n"
    REQUIRE(stderr_output.find("Error: --docs cannot be used with --serve mode") != std::string::npos);

    cleanup_temp_dir(index_dir);
    cleanup_temp_dir(docs_dir);
}

TEST_CASE("Invalid flag combination: --serve with --out exits with code 2")
{
    std::string index_dir = create_temp_dir();

    std::string searchd_path = find_searchd_path();
    std::string cmd = searchd_path + " --serve --in \"" + index_dir + "\" --port 8900 --out \"" + index_dir + "\" ";

    std::string stderr_output;
    int exit_code = run_command_capture_stderr(cmd, stderr_output);

    REQUIRE(exit_code == 2);
    REQUIRE(stderr_output.find("Error:") != std::string::npos);

    cleanup_temp_dir(index_dir);
}

TEST_CASE("Invalid port: non-numeric port exits with code 2")
{
    std::string index_dir = create_temp_dir();

    std::string searchd_path = find_searchd_path();
    std::string cmd = searchd_path + " --serve --in \"" + index_dir + "\" --port invalid ";

    std::string stderr_output;
    int exit_code = run_command_capture_stderr(cmd, stderr_output);

    REQUIRE(exit_code == 2);
    // Exact error message from spec: "Error: Invalid port number: <port>\n"
    REQUIRE(stderr_output.find("Error: Invalid port number:") != std::string::npos);

    cleanup_temp_dir(index_dir);
}

TEST_CASE("Invalid port: port 0 exits with code 2")
{
    std::string index_dir = create_temp_dir();

    std::string searchd_path = find_searchd_path();
    std::string cmd = searchd_path + " --serve --in \"" + index_dir + "\" --port 0 ";

    std::string stderr_output;
    int exit_code = run_command_capture_stderr(cmd, stderr_output);

    REQUIRE(exit_code == 2);
    // Exact error message from spec: "Error: Invalid port number: <port>\n"
    REQUIRE(stderr_output.find("Error: Invalid port number: 0") != std::string::npos);

    cleanup_temp_dir(index_dir);
}

TEST_CASE("Invalid port: port out of range (70000) exits with code 2")
{
    std::string index_dir = create_temp_dir();

    std::string searchd_path = find_searchd_path();
    std::string cmd = searchd_path + " --serve --in \"" + index_dir + "\" --port 70000 ";

    std::string stderr_output;
    int exit_code = run_command_capture_stderr(cmd, stderr_output);

    REQUIRE(exit_code == 2);
    // Exact error message from spec: "Error: Invalid port number: <port>\n"
    REQUIRE(stderr_output.find("Error: Invalid port number: 70000") != std::string::npos);

    cleanup_temp_dir(index_dir);
}

TEST_CASE("Help flag prints usage and exits with code 0")
{
    std::string searchd_path = find_searchd_path();
    std::string cmd = searchd_path + " --help ";

    std::string output;
    int exit_code = run_command_capture_stderr(cmd, output);

    REQUIRE(exit_code == 0);
    // Help should print something (usage, examples, etc.)
    REQUIRE(!output.empty());
}

TEST_CASE("[validation] No arguments behaves like --help")
{
    std::string searchd_path = find_searchd_path();
    std::string cmd = searchd_path + " ";

    std::string output;
    int exit_code = run_command_capture_stderr(cmd, output);

    REQUIRE(exit_code == 0);
    // Should print help/usage information
    REQUIRE(!output.empty());
}
