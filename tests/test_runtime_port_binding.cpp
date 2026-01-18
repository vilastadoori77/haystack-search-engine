// Phase 2.3 Runtime Lifecycle Tests - Port Binding Tests
// These tests STRICTLY follow the APPROVED specification: specs/phase2_3_runtime_lifecycle.md
// Do NOT add new behavior, flags, or assumptions beyond what is specified.
// Tests validate port binding failure detection and error reporting.

#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <fstream>
#include <cstdlib>
#include <string>
#include <vector>
#include <random>
#include <unistd.h>
#include <cstdio>
#include <cstdint>
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
    int system_result = std::system(full_cmd.c_str());

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

static std::string create_test_index()
{
    std::string index_dir = create_temp_dir();
    
    // Create index_meta.json
    std::ofstream meta(index_dir + "/index_meta.json");
    meta << R"({"schema_version": 1, "N": 2, "avgdl": 5.0})";
    meta.close();
    
    // Create docs.jsonl
    std::ofstream docs(index_dir + "/docs.jsonl");
    docs << R"({"docId": 1, "text": "hello world"})" << "\n";
    docs << R"({"docId": 2, "text": "test document"})" << "\n";
    docs.close();
    
    // Create minimal valid postings.bin (little-endian uint64_t: 0 terms = 8 bytes of zeros)
    std::ofstream postings(index_dir + "/postings.bin", std::ios::binary);
    std::uint64_t term_count = 0;
    unsigned char bytes[8] = {
        (unsigned char)(term_count & 0xFF),
        (unsigned char)((term_count >> 8) & 0xFF),
        (unsigned char)((term_count >> 16) & 0xFF),
        (unsigned char)((term_count >> 24) & 0xFF),
        (unsigned char)((term_count >> 32) & 0xFF),
        (unsigned char)((term_count >> 40) & 0xFF),
        (unsigned char)((term_count >> 48) & 0xFF),
        (unsigned char)((term_count >> 56) & 0xFF)
    };
    postings.write(reinterpret_cast<const char*>(bytes), 8);
    postings.close();
    
    return index_dir;
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

TEST_CASE("Port binding failure: port already in use, exit code 3")
{
    std::string index_dir = create_test_index();
    std::string searchd_path = find_searchd_path();
    
    // Use random port to avoid conflicts
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(9000, 9999);
    int test_port = dis(gen);
    
    // Bind the port before searchd tries to use it
    int sockfd = bind_port(test_port);
    REQUIRE(sockfd >= 0);
    
    std::string cmd = searchd_path + " --serve --in \"" + index_dir + "\" --port " + std::to_string(test_port);
    std::string stdout_output, stderr_output;
    int exit_code = run_command_capture_output(cmd, stdout_output, stderr_output);
    
    // Clean up
    if (sockfd >= 0)
        close(sockfd);
    cleanup_temp_dir(index_dir);
    
    REQUIRE(exit_code == 3);
    // Per spec: "Error: Failed to bind to port <port>: <error_message>\n"
    REQUIRE(stderr_output.find("Error: Failed to bind to port") != std::string::npos);
    REQUIRE(stderr_output.find(std::to_string(test_port)) != std::string::npos);
    // No startup message should be printed if port binding fails
    REQUIRE(stdout_output.find("Server started on port") == std::string::npos);
}

TEST_CASE("Port binding failure: no startup message on binding failure")
{
    std::string index_dir = create_test_index();
    std::string searchd_path = find_searchd_path();
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(9000, 9999);
    int test_port = dis(gen);
    
    int sockfd = bind_port(test_port);
    REQUIRE(sockfd >= 0);
    
    std::string cmd = searchd_path + " --serve --in \"" + index_dir + "\" --port " + std::to_string(test_port);
    std::string stdout_output, stderr_output;
    int exit_code = run_command_capture_output(cmd, stdout_output, stderr_output);
    
    if (sockfd >= 0)
        close(sockfd);
    cleanup_temp_dir(index_dir);
    
    REQUIRE(exit_code == 3);
    // Per spec: no startup message if port binding fails
    REQUIRE(stdout_output.find("Server started on port") == std::string::npos);
}

TEST_CASE("Port binding failure: error appears on stderr, not stdout")
{
    std::string index_dir = create_test_index();
    std::string searchd_path = find_searchd_path();
    
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
    
    REQUIRE(exit_code == 3);
    // Per spec: runtime errors appear only on stderr
    REQUIRE(stderr_output.find("Error:") != std::string::npos);
    // No error messages on stdout
    REQUIRE(stdout_output.find("Error:") == std::string::npos);
}
