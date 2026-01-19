// Phase 2.3 Runtime Lifecycle Tests - Failure Ordering Tests
// These tests STRICTLY follow the APPROVED specification: specs/phase2_3_runtime_lifecycle.md
// Do NOT add new behavior, flags, or assumptions beyond what is specified.
// Tests validate single root cause reporting (first failure wins).

#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <fstream>
#include <cstdlib>
#include <string>
#include <vector>
#include <random>
#include <unistd.h>
#include <cstdio>
#include <sys/socket.h>
#include <netinet/in.h>

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

static int bind_port(int port)
{
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        return -1;
    
    int opt = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);
    
    if (bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        close(sockfd);
        return -1;
    }
    
    if (listen(sockfd, 1) < 0)
    {
        close(sockfd);
        return -1;
    }
    
    return sockfd;
}

TEST_CASE("Failure ordering: index load fails first, no port binding attempt")
{
    std::string index_dir = create_temp_dir();
    std::string searchd_path = find_searchd_path();
    
    // Create index with unsupported schema version (index load will fail first)
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
    
    // Try multiple ports in case one is already in use
    int test_port = dis(gen);
    int sockfd = bind_port(test_port);
    int attempts = 0;
    while (sockfd < 0 && attempts < 10)
    {
        test_port = dis(gen);
        sockfd = bind_port(test_port);
        attempts++;
    }
    REQUIRE(sockfd >= 0);
    
    std::string cmd = searchd_path + " --serve --in \"" + index_dir + "\" --port " + std::to_string(test_port);
    std::string stdout_output, stderr_output;
    int exit_code = run_command_capture_output(cmd, stdout_output, stderr_output);
    
    if (sockfd >= 0)
        close(sockfd);
    cleanup_temp_dir(index_dir);
    
    // Per spec: if index load fails, no port binding attempt is made
    // Should only see index load error, not port binding error
    REQUIRE(exit_code == 3);
    REQUIRE(stderr_output.find("Error loading index:") != std::string::npos);
    // Should not have port binding error (first failure wins)
    REQUIRE(stderr_output.find("Failed to bind to port") == std::string::npos);
}

TEST_CASE("Failure ordering: only first error message appears in stderr")
{
    std::string index_dir = create_temp_dir();
    std::string searchd_path = find_searchd_path();
    
    // Create corrupted index (will fail on load)
    std::ofstream meta(index_dir + "/index_meta.json");
    meta << R"({"schema_version": 1, "N": 2, "avgdl": 5.0})";
    meta.close();
    
    std::ofstream docs(index_dir + "/docs.jsonl");
    docs << "INVALID JSON {broken";
    docs.close();
    
    std::ofstream postings(index_dir + "/postings.bin");
    postings << "INVALID BINARY";
    postings.close();
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(9000, 9999);
    int test_port = dis(gen);
    
    std::string cmd = searchd_path + " --serve --in \"" + index_dir + "\" --port " + std::to_string(test_port);
    std::string stdout_output, stderr_output;
    int exit_code = run_command_capture_output(cmd, stdout_output, stderr_output);
    
    cleanup_temp_dir(index_dir);
    
    // Per spec: only one error message appears in stderr for any startup failure
    REQUIRE(exit_code == 3);
    // Count "Error:" occurrences - should be only one
    size_t error_count = 0;
    size_t pos = 0;
    while ((pos = stderr_output.find("Error:", pos)) != std::string::npos)
    {
        error_count++;
        pos += 6; // length of "Error:"
    }
    REQUIRE(error_count <= 1); // Should be at most one error message
}

TEST_CASE("Failure ordering: exit code 3 for any runtime failure")
{
    std::string index_dir = create_temp_dir();
    std::string searchd_path = find_searchd_path();
    
    // Create invalid index
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
    
    // Per spec: exit code is 3 for any runtime failure
    REQUIRE(exit_code == 3);
}
