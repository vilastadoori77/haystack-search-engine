// Phase 2.3 Runtime Lifecycle Tests - Index Validation Tests
// These tests STRICTLY follow the APPROVED specification: specs/phase2_3_runtime_lifecycle.md
// Do NOT add new behavior, flags, or assumptions beyond what is specified.
// Tests validate index schema version and corruption handling during load.

#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <fstream>
#include <cstdlib>
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

static int run_command_capture_output(const std::string &cmd, std::string &stdout_output, std::string &stderr_output)
{
    pid_t pid = getpid();
    std::string stdout_file_path = "/tmp/haystack_stdout_" + std::to_string(pid);
    std::string stderr_file_path = "/tmp/haystack_stderr_" + std::to_string(pid);
    std::string exit_code_file = "/tmp/haystack_exit_" + std::to_string(pid);
    
    std::string full_cmd = "(" + cmd + " >" + stdout_file_path + " 2>" + stderr_file_path + "; echo $? >" + exit_code_file + ")";
    std::system(full_cmd.c_str());

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

    std::remove(stdout_file_path.c_str());
    std::remove(stderr_file_path.c_str());
    std::remove(exit_code_file.c_str());

    return exit_code;
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

TEST_CASE("Index load failure: unsupported schema version, exit code 3")
{
    std::string index_dir = create_temp_dir();
    std::string searchd_path = find_searchd_path();
    
    // Create index_meta.json with unsupported schema version
    std::ofstream meta(index_dir + "/index_meta.json");
    meta << R"({"schema_version": 999, "N": 2, "avgdl": 5.0})";
    meta.close();
    
    std::ofstream docs(index_dir + "/docs.jsonl");
    docs << R"({"docId": 1, "text": "hello world"})" << "\n";
    docs.close();
    
    std::ofstream postings(index_dir + "/postings.bin");
    postings.close();
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(9000, 9999);
    int test_port = dis(gen);
    
    std::string cmd = searchd_path + " --serve --in \"" + index_dir + "\" --port " + std::to_string(test_port);
    std::string stdout_output, stderr_output;
    int exit_code = run_command_capture_output(cmd, stdout_output, stderr_output);
    
    cleanup_temp_dir(index_dir);
    
    // Per spec: unsupported schema version exits with code 3
    REQUIRE(exit_code == 3);
    // Per spec: "Error loading index: Unsupported schema version: <version>\n"
    bool has_unsupported_version = stderr_output.find("Error loading index: Unsupported schema version:") != std::string::npos;
    bool has_error_loading = stderr_output.find("Error loading index:") != std::string::npos;
    REQUIRE((has_unsupported_version || has_error_loading));
    // No startup message if index load fails
    REQUIRE(stdout_output.find("Server started on port") == std::string::npos);
}

TEST_CASE("Index load failure: corrupted postings.bin, exit code 3")
{
    std::string index_dir = create_temp_dir();
    std::string searchd_path = find_searchd_path();
    
    std::ofstream meta(index_dir + "/index_meta.json");
    meta << R"({"schema_version": 1, "N": 2, "avgdl": 5.0})";
    meta.close();
    
    std::ofstream docs(index_dir + "/docs.jsonl");
    docs << R"({"docId": 1, "text": "hello world"})" << "\n";
    docs.close();
    
    // Create corrupted postings.bin with invalid binary data
    std::ofstream postings(index_dir + "/postings.bin");
    postings << "INVALID_BINARY_DATA_NOT_A_VALID_POSTINGS_FILE";
    postings.close();
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(9000, 9999);
    int test_port = dis(gen);
    
    std::string cmd = searchd_path + " --serve --in \"" + index_dir + "\" --port " + std::to_string(test_port);
    std::string stdout_output, stderr_output;
    int exit_code = run_command_capture_output(cmd, stdout_output, stderr_output);
    
    cleanup_temp_dir(index_dir);
    
    // Per spec: index file corruption exits with code 3
    REQUIRE(exit_code == 3);
    // Per spec: "Error loading index: <specific_error>\n"
    REQUIRE(stderr_output.find("Error loading index:") != std::string::npos);
    // No startup message if index load fails
    REQUIRE(stdout_output.find("Server started on port") == std::string::npos);
}

TEST_CASE("Index load failure: malformed docs.jsonl, exit code 3")
{
    std::string index_dir = create_temp_dir();
    std::string searchd_path = find_searchd_path();
    
    std::ofstream meta(index_dir + "/index_meta.json");
    meta << R"({"schema_version": 1, "N": 2, "avgdl": 5.0})";
    meta.close();
    
    // Create malformed docs.jsonl (not valid JSON)
    std::ofstream docs(index_dir + "/docs.jsonl");
    docs << "NOT VALID JSON {invalid syntax";
    docs.close();
    
    std::ofstream postings(index_dir + "/postings.bin");
    postings.close();
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(9000, 9999);
    int test_port = dis(gen);
    
    std::string cmd = searchd_path + " --serve --in \"" + index_dir + "\" --port " + std::to_string(test_port);
    std::string stdout_output, stderr_output;
    int exit_code = run_command_capture_output(cmd, stdout_output, stderr_output);
    
    cleanup_temp_dir(index_dir);
    
    // Per spec: malformed index files exit with code 3
    REQUIRE(exit_code == 3);
    // Per spec: "Error loading index: <specific_error>\n"
    REQUIRE(stderr_output.find("Error loading index:") != std::string::npos);
    // No startup message if index load fails
    REQUIRE(stdout_output.find("Server started on port") == std::string::npos);
}

TEST_CASE("Index load failure: no server startup occurs")
{
    std::string index_dir = create_temp_dir();
    std::string searchd_path = find_searchd_path();
    
    std::ofstream meta(index_dir + "/index_meta.json");
    meta << R"({"schema_version": 999, "N": 2, "avgdl": 5.0})"; // Unsupported version
    meta.close();
    
    std::ofstream docs(index_dir + "/docs.jsonl");
    docs << R"({"docId": 1, "text": "hello world"})" << "\n";
    docs.close();
    
    std::ofstream postings(index_dir + "/postings.bin");
    postings.close();
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(9000, 9999);
    int test_port = dis(gen);
    
    std::string cmd = searchd_path + " --serve --in \"" + index_dir + "\" --port " + std::to_string(test_port);
    std::string stdout_output, stderr_output;
    int exit_code = run_command_capture_output(cmd, stdout_output, stderr_output);
    
    cleanup_temp_dir(index_dir);
    
    // Per spec: no server startup occurs if index load validation fails
    REQUIRE(exit_code == 3);
    REQUIRE(stdout_output.find("Server started on port") == std::string::npos);
}
