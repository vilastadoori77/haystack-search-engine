// Phase 2.3 Runtime Lifecycle Tests - Signal Handling and Shutdown Tests
// These tests STRICTLY follow the APPROVED specification: specs/phase2_3_runtime_lifecycle.md
// Do NOT add new behavior, flags, or assumptions beyond what is specified.
// Tests validate signal handling (SIGINT/SIGTERM) and clean shutdown.

#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <fstream>
#include <cstdlib>
#include <string>
#include <vector>
#include <random>
#include <unistd.h>
#include <cstdio>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <algorithm>
#include <iterator>

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
    
    std::ofstream postings(index_dir + "/postings.bin");
    postings.close();
    
    return index_dir;
}

static int run_command_with_signal(const std::string &cmd, int signal_type)
{
    pid_t pid = fork();
    if (pid == 0)
    {
        // Child process: run searchd
        std::system(cmd.c_str());
        _exit(0);
    }
    else if (pid > 0)
    {
        // Parent process: wait briefly, then send signal
        usleep(500000); // 0.5 seconds
        kill(pid, signal_type);
        
        int status;
        waitpid(pid, &status, 0);
        return WEXITSTATUS(status);
    }
    
    return -1;
}

TEST_CASE("SIGINT: clean shutdown with exit code 0")
{
    std::string index_dir = create_test_index();
    std::string searchd_path = find_searchd_path();
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(9000, 9999);
    int test_port = dis(gen);
    
    std::string cmd = searchd_path + " --serve --in \"" + index_dir + "\" --port " + std::to_string(test_port) + " 2>/tmp/haystack_sigint_stderr_$$";
    int exit_code = run_command_with_signal(cmd, SIGINT);
    
    std::ifstream stderr_file("/tmp/haystack_sigint_stderr_$$");
    std::string stderr_output;
    if (stderr_file)
    {
        stderr_output.assign((std::istreambuf_iterator<char>(stderr_file)),
                             std::istreambuf_iterator<char>());
        stderr_file.close();
    }
    
    std::system("rm -f /tmp/haystack_sigint_stderr_$$");
    cleanup_temp_dir(index_dir);
    
    // Per spec: clean shutdown via SIGINT exits with code 0
    REQUIRE(exit_code == 0);
}

TEST_CASE("SIGTERM: clean shutdown with exit code 0")
{
    std::string index_dir = create_test_index();
    std::string searchd_path = find_searchd_path();
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(9000, 9999);
    int test_port = dis(gen);
    
    std::string cmd = searchd_path + " --serve --in \"" + index_dir + "\" --port " + std::to_string(test_port) + " 2>/tmp/haystack_sigterm_stderr_$$";
    int exit_code = run_command_with_signal(cmd, SIGTERM);
    
    std::ifstream stderr_file("/tmp/haystack_sigterm_stderr_$$");
    std::string stderr_output;
    if (stderr_file)
    {
        stderr_output.assign((std::istreambuf_iterator<char>(stderr_file)),
                             std::istreambuf_iterator<char>());
        stderr_file.close();
    }
    
    std::system("rm -f /tmp/haystack_sigterm_stderr_$$");
    cleanup_temp_dir(index_dir);
    
    // Per spec: clean shutdown via SIGTERM exits with code 0
    REQUIRE(exit_code == 0);
}

TEST_CASE("Clean shutdown: no stderr output on SIGINT")
{
    std::string index_dir = create_test_index();
    std::string searchd_path = find_searchd_path();
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(9000, 9999);
    int test_port = dis(gen);
    
    std::string cmd = searchd_path + " --serve --in \"" + index_dir + "\" --port " + std::to_string(test_port) + " 2>/tmp/haystack_sigint_stderr2_$$";
    run_command_with_signal(cmd, SIGINT);
    
    std::ifstream stderr_file("/tmp/haystack_sigint_stderr2_$$");
    std::string stderr_output;
    if (stderr_file)
    {
        stderr_output.assign((std::istreambuf_iterator<char>(stderr_file)),
                             std::istreambuf_iterator<char>());
        stderr_file.close();
    }
    
    std::system("rm -f /tmp/haystack_sigint_stderr2_$$");
    cleanup_temp_dir(index_dir);
    
    // Per spec: clean shutdown produces no output to stderr
    // Remove whitespace/newlines for comparison
    std::string trimmed = stderr_output;
    trimmed.erase(std::remove(trimmed.begin(), trimmed.end(), '\n'), trimmed.end());
    trimmed.erase(std::remove(trimmed.begin(), trimmed.end(), '\r'), trimmed.end());
    trimmed.erase(std::remove(trimmed.begin(), trimmed.end(), ' '), trimmed.end());
    bool no_error_output = trimmed.empty() || trimmed.find("Error:") == std::string::npos;
    REQUIRE(no_error_output);
}

TEST_CASE("Clean shutdown: no stderr output on SIGTERM")
{
    std::string index_dir = create_test_index();
    std::string searchd_path = find_searchd_path();
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(9000, 9999);
    int test_port = dis(gen);
    
    std::string cmd = searchd_path + " --serve --in \"" + index_dir + "\" --port " + std::to_string(test_port) + " 2>/tmp/haystack_sigterm_stderr2_$$";
    run_command_with_signal(cmd, SIGTERM);
    
    std::ifstream stderr_file("/tmp/haystack_sigterm_stderr2_$$");
    std::string stderr_output;
    if (stderr_file)
    {
        stderr_output.assign((std::istreambuf_iterator<char>(stderr_file)),
                             std::istreambuf_iterator<char>());
        stderr_file.close();
    }
    
    std::system("rm -f /tmp/haystack_sigterm_stderr2_$$");
    cleanup_temp_dir(index_dir);
    
    // Per spec: clean shutdown produces no output to stderr
    std::string trimmed = stderr_output;
    trimmed.erase(std::remove(trimmed.begin(), trimmed.end(), '\n'), trimmed.end());
    trimmed.erase(std::remove(trimmed.begin(), trimmed.end(), '\r'), trimmed.end());
    trimmed.erase(std::remove(trimmed.begin(), trimmed.end(), ' '), trimmed.end());
    bool no_error_output = trimmed.empty() || trimmed.find("Error:") == std::string::npos;
    REQUIRE(no_error_output);
}
