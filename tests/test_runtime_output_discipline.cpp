// Phase 2.3 Runtime Lifecycle Tests - Output Discipline Tests
// These tests STRICTLY follow the APPROVED specification: specs/phase2_3_runtime_lifecycle.md
// Do NOT add new behavior, flags, or assumptions beyond what is specified.
// Tests validate stdout/stderr separation.

#include "runtime_test_common.hpp"
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
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <cerrno>

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

// Wrapper for the safe subprocess helper with 5-second timeout
static int run_command_capture_output(const std::string &cmd, std::string &stdout_output, std::string &stderr_output)
{
    return runtime_test::run_command_split_output(cmd, stdout_output, stderr_output, 5);
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

static std::string create_test_index()
{
    std::string index_dir = create_temp_dir();
    
    std::ofstream meta(index_dir + "/index_meta.json");
    meta << R"({"schema_version": 1, "N": 2, "avgdl": 5.0})";
    meta.close();
    
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

TEST_CASE("Output discipline: runtime errors appear only on stderr")
{
    std::string index_dir = create_temp_dir();
    std::string searchd_path = find_searchd_path();
    
    // Create corrupted index to trigger runtime error
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
    
    // Per spec: runtime errors appear only on stderr
    bool has_error_on_stderr = (stderr_output.find("Error:") != std::string::npos) || (stderr_output.find("Error loading index:") != std::string::npos);
    REQUIRE(has_error_on_stderr);
    REQUIRE(stdout_output.find("Error:") == std::string::npos);
}

TEST_CASE("Output discipline: startup success messages appear only on stdout")
{
    // Save original signal handler and ignore SIGTERM to prevent signal propagation
    void (*old_handler)(int) = signal(SIGTERM, SIG_IGN);
    
    std::string index_dir = create_test_index();
    std::string searchd_path = find_searchd_path();
    
    // Try multiple random ports to avoid conflicts
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(9000, 9999);
    int test_port = -1;
    bool success = false;
    
    // Try up to 10 different ports to find one that works
    for (int port_attempt = 0; port_attempt < 10; ++port_attempt)
    {
        test_port = dis(gen);
        
        // Use PID-based unique filenames to avoid conflicts
        pid_t test_pid = getpid();
        std::string stdout_file_path = "/tmp/haystack_output_stdout_" + std::to_string(test_pid) + "_" + std::to_string(port_attempt);
        std::string stderr_file_path = "/tmp/haystack_output_stderr_" + std::to_string(test_pid) + "_" + std::to_string(port_attempt);
        
        std::string cmd = searchd_path + " --serve --in \"" + index_dir + "\" --port " + std::to_string(test_port) + 
                          " >" + stdout_file_path + " 2>" + stderr_file_path;
        
        // Use fork() to run command (shell handles redirection)
        pid_t pid = fork();
        if (pid == 0)
        {
            // Child process: run command (shell will handle redirection)
            std::system(cmd.c_str());
            _exit(0);
        }
        else if (pid > 0)
        {
            // Parent process: wait for server to start and print message
            // Poll /health endpoint to confirm server is ready before checking for message
            bool server_ready = false;
            for (int attempt = 0; attempt < 40; ++attempt)
            {
                usleep(150000); // 150ms between attempts (up to 6 seconds total)
                std::string check_cmd = "curl -s -o /dev/null -w \"%{http_code}\" --max-time 1 --connect-timeout 1 \"http://localhost:" + std::to_string(test_port) + "/health\" 2>/dev/null || echo \"-1\"";
                FILE *pipe = popen(check_cmd.c_str(), "r");
                if (pipe)
                {
                    char buffer[128];
                    std::string result;
                    while (fgets(buffer, sizeof(buffer), pipe) != nullptr)
                    {
                        result += buffer;
                    }
                    pclose(pipe);
                    if (result.find("200") != std::string::npos)
                    {
                        server_ready = true;
                        break;
                    }
                }
            }
            
            // Give server additional time to ensure message is flushed to stdout
            if (server_ready)
            {
                usleep(500000); // 500ms after readiness to ensure message is written and flushed
                
                // Check if process is still running before sending signal
                int status;
                pid_t check_result = waitpid(pid, &status, WNOHANG);
                if (check_result == 0)
                {
                    // Process still running - send SIGTERM
                    if (kill(pid, SIGTERM) == -1 && errno != ESRCH)
                    {
                        // Error sending signal (but ignore if process doesn't exist)
                    }
                }
                
                // Wait for process to exit
                bool process_exited = false;
                for (int i = 0; i < 30; ++i)
                {
                    pid_t result = waitpid(pid, &status, WNOHANG);
                    if (result == pid)
                    {
                        process_exited = true;
                        break;
                    }
                    if (result == -1 && errno == ECHILD)
                    {
                        process_exited = true;
                        break;
                    }
                    usleep(100000); // 100ms
                }
                
                // If still running, force kill (only if we confirmed it was running)
                if (!process_exited && check_result == 0)
                {
                    kill(pid, SIGKILL);
                    waitpid(pid, &status, 0);
                }
                
                // Ensure files are flushed - wait for all I/O to complete
                sync();
                usleep(500000); // 500ms delay to ensure file writes are complete and flushed to disk
                
                // Read captured output
                std::ifstream stdout_file(stdout_file_path);
                std::ifstream stderr_file(stderr_file_path);
                
                std::string stdout_output, stderr_output;
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
                
                // Check if we got the message - if yes, break out of port retry loop
                if (stdout_output.find("Server started on port") != std::string::npos)
                {
                    // Successfully captured message - clean up and verify
                    std::remove(stdout_file_path.c_str());
                    std::remove(stderr_file_path.c_str());
                    cleanup_temp_dir(index_dir);
                    
                    // Per spec: startup success messages appear only on stdout
                    REQUIRE(stdout_output.find("Server started on port") != std::string::npos);
                    REQUIRE(stderr_output.find("Server started on port") == std::string::npos);
                    
                    // Restore original signal handler
                    signal(SIGTERM, old_handler);
                    return; // Success - exit test
                }
                
                // Clean up files for this failed attempt
                std::remove(stdout_file_path.c_str());
                std::remove(stderr_file_path.c_str());
            }
            else
            {
                // Server didn't start - kill process and try next port
                int status;
                kill(pid, SIGKILL);
                waitpid(pid, &status, 0);
                std::remove(stdout_file_path.c_str());
                std::remove(stderr_file_path.c_str());
            }
        }
    }
    
    // If we get here, all port attempts failed
    cleanup_temp_dir(index_dir);
    
    // Restore original signal handler
    signal(SIGTERM, old_handler);
    
    // This should not happen - at least one port should work
    REQUIRE(success);
}

TEST_CASE("Output discipline: port binding error on stderr, not stdout")
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
    
    // Per spec: runtime errors appear only on stderr
    REQUIRE(stderr_output.find("Error: Failed to bind to port") != std::string::npos);
    REQUIRE(stdout_output.find("Error:") == std::string::npos);
}

TEST_CASE("Output discipline: no mixed output streams")
{
    std::string index_dir = create_temp_dir();
    std::string searchd_path = find_searchd_path();
    
    // Create invalid index (will produce error)
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
    
    // Per spec: no mixed output streams
    // Errors should not be on stdout
    REQUIRE(stdout_output.find("Error:") == std::string::npos);
    REQUIRE(stdout_output.find("Error loading index:") == std::string::npos);
    // Success messages should not be on stderr
    REQUIRE(stderr_output.find("Server started on port") == std::string::npos);
}
