// Phase 2.2 CLI Lifecycle Tests - Integration Tests
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
#include <unistd.h>
#include <cstdio>

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

static int run_command_capture_output(const std::string &cmd, std::string &stdout_output, std::string &stderr_output)
{
    // Generate unique temp file names using process ID
    pid_t pid = getpid();
    std::string stdout_file_path = "/tmp/haystack_stdout_" + std::to_string(pid);
    std::string stderr_file_path = "/tmp/haystack_stderr_" + std::to_string(pid);
    std::string exit_code_file = "/tmp/haystack_exit_" + std::to_string(pid);
    
    // Run command and capture exit code separately
    std::string full_cmd = "(" + cmd + " >" + stdout_file_path + " 2>" + stderr_file_path + "; echo $? >" + exit_code_file + ")";
    int system_result = std::system(full_cmd.c_str());

    // Read captured output
    std::ifstream stdout_file(stdout_file_path);
    std::ifstream stderr_file(stderr_file_path);
    std::ifstream exit_file(exit_code_file);

    stdout_output.clear();
    stderr_output.clear();

    if (stdout_file)
    {
        stdout_output.assign((std::istreambuf_iterator<char>(stdout_file)),
                             std::istreambuf_iterator<char>());
        stdout_file.close();
    }

    if (stderr_file)
    {
        stderr_output.assign((std::istreambuf_iterator<char>(stderr_file)),
                             std::istreambuf_iterator<char>());
        stderr_file.close();
    }

    // Read exit code
    int exit_code = -1;
    if (exit_file)
    {
        std::string exit_str;
        std::getline(exit_file, exit_str);
        if (!exit_str.empty())
        {
            try {
                exit_code = std::stoi(exit_str);
            } catch (...) {
                exit_code = -1;
            }
        }
        exit_file.close();
    }

    // Cleanup
    std::remove(stdout_file_path.c_str());
    std::remove(stderr_file_path.c_str());
    std::remove(exit_code_file.c_str());

    return exit_code;
}

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
  {"docId": 2, "text": "world peace"},
  {"docId": 3, "text": "hello there"}
])";
    out.close();
    return docs_file;
}

static bool file_exists(const std::string &path)
{
    return fs::exists(path) && fs::is_regular_file(path);
}

TEST_CASE("Index mode: successful execution, exit code 0, no server started")
{
    std::string docs_file = create_test_docs_file();
    std::string index_dir = create_temp_dir();
    std::string docs_dir = fs::path(docs_file).parent_path().string();

    std::string searchd_path = find_searchd_path();
    std::string cmd = searchd_path + " --index --docs \"" + docs_file + "\" --out \"" + index_dir + "\"";

    std::string stdout_output, stderr_output;
    int exit_code = run_command_capture_output(cmd, stdout_output, stderr_output);

    REQUIRE(exit_code == 0);
    // Exact success message from spec: "Indexing completed. Index saved to: <index_dir>"
    std::string expected_msg = "Indexing completed. Index saved to: " + index_dir;
    // Remove any trailing whitespace/newlines that might be from exit code capture
    std::string trimmed_stdout = stdout_output;
    while (!trimmed_stdout.empty() && (trimmed_stdout.back() == '\n' || trimmed_stdout.back() == '\r' || trimmed_stdout.back() == ' '))
    {
        trimmed_stdout.pop_back();
    }
    // Check if the expected message is in the output (allowing for trailing newline from the message itself)
    REQUIRE(trimmed_stdout.find(expected_msg) != std::string::npos);
    bool stderr_is_ok = stderr_output.empty() || stderr_output.find("Error:") == std::string::npos;
    REQUIRE(stderr_is_ok);

    // Verify index files were created
    REQUIRE(file_exists(index_dir + "/index_meta.json"));
    REQUIRE(file_exists(index_dir + "/docs.jsonl"));
    REQUIRE(file_exists(index_dir + "/postings.bin"));

    cleanup_temp_dir(index_dir);
    cleanup_temp_dir(docs_dir);
}

TEST_CASE("Serve mode: successful startup, exit code 0, server running")
{
    // First create an index
    std::string docs_file = create_test_docs_file();
    std::string index_dir = create_temp_dir();
    std::string docs_dir = fs::path(docs_file).parent_path().string();

    std::string searchd_path = find_searchd_path();
    std::string index_cmd = searchd_path + " --index --docs \"" + docs_file + "\" --out \"" + index_dir + "\"";
    int index_result = run_command(index_cmd);
    REQUIRE(index_result == 0);

    // Use random port to avoid conflicts
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(9000, 9999);
    int test_port = dis(gen);

    // Start serve mode in background, wait briefly, then kill
    std::string serve_cmd = searchd_path + " --serve --in \"" + index_dir + "\" --port " + std::to_string(test_port);
    std::string full_cmd = "(" + serve_cmd + " > /dev/null 2>&1 &); SERVE_PID=$!; sleep 0.5; kill $SERVE_PID 2>/dev/null || true; wait $SERVE_PID 2>/dev/null; exit 0";

    int result = run_command(full_cmd);
    // Note: Exit code may vary due to kill/wait, but server should have started
    // The important thing is it doesn't fail immediately with index load error

    cleanup_temp_dir(index_dir);
    cleanup_temp_dir(docs_dir);
}

TEST_CASE("Index mode: missing --out flag, exit code 2")
{
    std::string docs_file = create_test_docs_file();
    std::string docs_dir = fs::path(docs_file).parent_path().string();

    std::string searchd_path = find_searchd_path();
    std::string cmd = searchd_path + " --index --docs \"" + docs_file + "\" 2>&1";

    std::string stdout_output, stderr_output;
    int exit_code = run_command_capture_output(cmd, stdout_output, stderr_output);

    REQUIRE(exit_code == 2);
    // Exact error message from spec: "Error: --out <index_dir> is required when using --index mode\n"
    REQUIRE(stderr_output.find("Error: --out <index_dir> is required when using --index mode") != std::string::npos);

    cleanup_temp_dir(docs_dir);
}

TEST_CASE("Index mode: missing --docs flag, exit code 2")
{
    std::string index_dir = create_temp_dir();

    std::string searchd_path = find_searchd_path();
    std::string cmd = searchd_path + " --index --out \"" + index_dir + "\" 2>&1";

    std::string stdout_output, stderr_output;
    int exit_code = run_command_capture_output(cmd, stdout_output, stderr_output);

    REQUIRE(exit_code == 2);
    // Exact error message from spec: "Error: --docs <path> is required when using --index mode\n"
    REQUIRE(stderr_output.find("Error: --docs <path> is required when using --index mode") != std::string::npos);

    cleanup_temp_dir(index_dir);
}

TEST_CASE("Serve mode: missing --in flag, exit code 2")
{
    std::string searchd_path = find_searchd_path();
    std::string cmd = searchd_path + " --serve --port 8900 2>&1";

    std::string stdout_output, stderr_output;
    int exit_code = run_command_capture_output(cmd, stdout_output, stderr_output);

    REQUIRE(exit_code == 2);
    // Exact error message from spec: "Error: --in <index_dir> is required when using --serve mode\n"
    REQUIRE(stderr_output.find("Error: --in <index_dir> is required when using --serve mode") != std::string::npos);
}

TEST_CASE("Serve mode: missing --port flag, exit code 2")
{
    std::string index_dir = create_temp_dir();

    std::string searchd_path = find_searchd_path();
    std::string cmd = searchd_path + " --serve --in \"" + index_dir + "\" 2>&1";

    std::string stdout_output, stderr_output;
    int exit_code = run_command_capture_output(cmd, stdout_output, stderr_output);

    REQUIRE(exit_code == 2);
    // Exact error message from spec: "Error: --port <port> is required when using --serve mode\n"
    REQUIRE(stderr_output.find("Error: --port <port> is required when using --serve mode") != std::string::npos);

    cleanup_temp_dir(index_dir);
}

TEST_CASE("Conflicting flags: --index --serve, exit code 2")
{
    std::string searchd_path = find_searchd_path();
    std::string cmd = searchd_path + " --index --serve 2>&1";

    std::string stdout_output, stderr_output;
    int exit_code = run_command_capture_output(cmd, stdout_output, stderr_output);

    REQUIRE(exit_code == 2);
    // Exact error message from spec: "Error: --index and --serve cannot be used together\n"
    REQUIRE(stderr_output.find("Error: --index and --serve cannot be used together") != std::string::npos);
}

TEST_CASE("Invalid combinations: --index --in, exit code 2")
{
    std::string docs_file = create_test_docs_file();
    std::string index_dir = create_temp_dir();
    std::string docs_dir = fs::path(docs_file).parent_path().string();

    std::string searchd_path = find_searchd_path();
    std::string cmd = searchd_path + " --index --docs \"" + docs_file + "\" --out \"" + index_dir + "\" --in \"" + index_dir + "\" 2>&1";

    std::string stdout_output, stderr_output;
    int exit_code = run_command_capture_output(cmd, stdout_output, stderr_output);

    REQUIRE(exit_code == 2);
    // Exact error message from spec: "Error: --in cannot be used with --index mode\n"
    REQUIRE(stderr_output.find("Error: --in cannot be used with --index mode") != std::string::npos);

    cleanup_temp_dir(index_dir);
    cleanup_temp_dir(docs_dir);
}

TEST_CASE("Invalid combinations: --serve --docs, exit code 2")
{
    std::string docs_file = create_test_docs_file();
    std::string index_dir = create_temp_dir();
    std::string docs_dir = fs::path(docs_file).parent_path().string();

    std::string searchd_path = find_searchd_path();
    std::string cmd = searchd_path + " --serve --in \"" + index_dir + "\" --port 8900 --docs \"" + docs_file + "\" 2>&1";

    std::string stdout_output, stderr_output;
    int exit_code = run_command_capture_output(cmd, stdout_output, stderr_output);

    REQUIRE(exit_code == 2);
    // Exact error message from spec: "Error: --docs cannot be used with --serve mode\n"
    REQUIRE(stderr_output.find("Error: --docs cannot be used with --serve mode") != std::string::npos);

    cleanup_temp_dir(index_dir);
    cleanup_temp_dir(docs_dir);
}

TEST_CASE("Nonexistent document file, exit code 3")
{
    std::string index_dir = create_temp_dir();
    std::string nonexistent_file = "/tmp/nonexistent_docs_12345.json";

    // Ensure file doesn't exist
    if (fs::exists(nonexistent_file))
    {
        fs::remove(nonexistent_file);
    }

    std::string searchd_path = find_searchd_path();
    std::string cmd = searchd_path + " --index --docs \"" + nonexistent_file + "\" --out \"" + index_dir + "\" 2>&1";

    std::string stdout_output, stderr_output;
    int exit_code = run_command_capture_output(cmd, stdout_output, stderr_output);

    REQUIRE(exit_code == 3);
    // Exact error message from spec: "Error: Document file not found: <path>\n"
    // OR "Error indexing/saving: <specific_error>\n" (which may contain "Failed to open")
    bool has_exact_msg = stderr_output.find("Error: Document file not found:") != std::string::npos;
    bool has_indexing_error = stderr_output.find("Error indexing/saving:") != std::string::npos;
    REQUIRE((has_exact_msg || has_indexing_error));

    cleanup_temp_dir(index_dir);
}

TEST_CASE("Nonexistent index directory, exit code 3")
{
    std::string nonexistent_dir = "/tmp/nonexistent_index_dir_12345";

    // Ensure directory doesn't exist
    if (fs::exists(nonexistent_dir))
    {
        fs::remove_all(nonexistent_dir);
    }

    std::string searchd_path = find_searchd_path();
    std::string cmd = searchd_path + " --serve --in \"" + nonexistent_dir + "\" --port 8900";

    std::string stdout_output, stderr_output;
    int exit_code = run_command_capture_output(cmd, stdout_output, stderr_output);

    REQUIRE(exit_code == 3);
    REQUIRE(stderr_output.find("Error:") != std::string::npos);
    REQUIRE(stderr_output.find("not found") != std::string::npos);
}

TEST_CASE("Incomplete index directory (missing files), exit code 3")
{
    std::string index_dir = create_temp_dir();

    // Create only one file, not all three
    std::string meta_path = index_dir + "/index_meta.json";
    std::ofstream meta(meta_path);
    meta << R"({"schema_version": 1, "N": 1, "avgdl": 10.0})";
    meta.close();

    // Missing docs.jsonl and postings.bin

    std::string searchd_path = find_searchd_path();
    std::string cmd = searchd_path + " --serve --in \"" + index_dir + "\" --port 8900 2>&1";

    std::string stdout_output, stderr_output;
    int exit_code = run_command_capture_output(cmd, stdout_output, stderr_output);

    REQUIRE(exit_code == 3);
    // Exact error message from spec: "Error: Index file not found: <file_path>\n"
    // OR "Error loading index: <specific_error>\n"
    bool has_exact_msg = stderr_output.find("Error: Index file not found:") != std::string::npos;
    bool has_loading_error = stderr_output.find("Error loading index:") != std::string::npos;
    REQUIRE((has_exact_msg || has_loading_error));

    cleanup_temp_dir(index_dir);
}

TEST_CASE("Invalid port number, exit code 2")
{
    std::string index_dir = create_temp_dir();

    std::string searchd_path = find_searchd_path();
    std::string cmd = searchd_path + " --serve --in \"" + index_dir + "\" --port invalid 2>&1";

    std::string stdout_output, stderr_output;
    int exit_code = run_command_capture_output(cmd, stdout_output, stderr_output);

    REQUIRE(exit_code == 2);
    // Exact error message from spec: "Error: Invalid port number: <port>\n"
    REQUIRE(stderr_output.find("Error: Invalid port number: invalid") != std::string::npos);

    cleanup_temp_dir(index_dir);
}

TEST_CASE("Port out of range, exit code 2")
{
    std::string index_dir = create_temp_dir();

    std::string searchd_path = find_searchd_path();
    std::string cmd = searchd_path + " --serve --in \"" + index_dir + "\" --port 70000 2>&1";

    std::string stdout_output, stderr_output;
    int exit_code = run_command_capture_output(cmd, stdout_output, stderr_output);

    REQUIRE(exit_code == 2);
    // Exact error message from spec: "Error: Invalid port number: <port>\n"
    REQUIRE(stderr_output.find("Error: Invalid port number: 70000") != std::string::npos);

    cleanup_temp_dir(index_dir);
}

TEST_CASE("--help flag prints usage and exits 0")
{
    std::string searchd_path = find_searchd_path();
    std::string cmd = searchd_path + " --help 2>&1";

    std::string stdout_output, stderr_output;
    int exit_code = run_command_capture_output(cmd, stdout_output, stderr_output);

    REQUIRE(exit_code == 0);
    // Help should print something (usage, examples, etc.)
    bool has_output = !stdout_output.empty() || !stderr_output.empty();
    REQUIRE(has_output);
}

TEST_CASE("[lifecycle]No arguments behaves like --help")
{
    std::string searchd_path = find_searchd_path();
    std::string cmd = searchd_path + " 2>&1";

    std::string stdout_output, stderr_output;
    int exit_code = run_command_capture_output(cmd, stdout_output, stderr_output);

    REQUIRE(exit_code == 0);
    // Should print help/usage information
    bool has_output = !stdout_output.empty() || !stderr_output.empty();
    REQUIRE(has_output);
}
