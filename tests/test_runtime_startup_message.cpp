// Phase 2.3 Runtime Lifecycle Tests - Startup Message Tests
// These tests STRICTLY follow the APPROVED specification: specs/phase2_3_runtime_lifecycle.md
// Do NOT add new behavior, flags, or assumptions beyond what is specified.
// Tests validate startup message format and timing.
//
// Output Capture Method (Option C):
// These tests capture stdout from a background server process using direct file redirection
// without subshells for improved reliability:
//   Pattern: cmd >file1 2>file2 & PID=$!; sleep 2; kill $PID; wait; sync
//   Benefits: No subshell overhead, file handles ready before process starts, better timing
//   Previous approach (cmd >file &) in subshell caused race conditions with file handle setup
//   See test comments for details on why this method is used.

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

TEST_CASE("Successful startup: prints exactly one startup message to stdout")
{
    std::string index_dir = create_test_index();
    std::string searchd_path = find_searchd_path();
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(9000, 9999);
    int test_port = dis(gen);
    
    // Use PID-based unique filenames to avoid conflicts
    pid_t pid = getpid();
    std::string stdout_file_path = "/tmp/haystack_startup_stdout_" + std::to_string(pid);
    std::string stderr_file_path = "/tmp/haystack_startup_stderr_" + std::to_string(pid);
    
    // Option C: Capture output from background process without subshell for better reliability
    // Pattern: cmd >file1 2>file2 & PID=$!; sleep; kill; wait; sync
    // This avoids subshell overhead that can delay file handle setup and cause race conditions
    // Redirection happens in the same shell context, ensuring file handles are ready before process starts
    
    // Create empty output files first to ensure they exist before redirection
    std::ofstream(stdout_file_path).close();
    std::ofstream(stderr_file_path).close();
    
    std::string cmd = searchd_path + " --serve --in \"" + index_dir + "\" --port " + std::to_string(test_port);
    // Start process with direct redirection (no subshell), capture PID, wait for startup, then kill gracefully
    // Note: Using separate commands to ensure proper shell execution order
    std::string full_cmd = cmd + " >" + stdout_file_path + " 2>" + stderr_file_path + " & SERVE_PID=$!; sleep 2; kill $SERVE_PID 2>/dev/null || true; wait $SERVE_PID 2>/dev/null; sleep 0.3; sync";
    std::system(full_cmd.c_str());
    
    // Additional delay and try reading multiple times if file is empty (handle race conditions)
    for (int attempt = 0; attempt < 5; ++attempt)
    {
        usleep(100000); // 100ms between attempts
        std::ifstream test_file(stdout_file_path);
        if (test_file && test_file.peek() != std::ifstream::traits_type::eof())
        {
            break; // File has content, proceed to read
        }
    }
    
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
    
    // Debug: Print what was actually captured (only in case of failure)
    // This helps diagnose why the message isn't being captured
    
    std::remove(stdout_file_path.c_str());
    std::remove(stderr_file_path.c_str());
    cleanup_temp_dir(index_dir);
    
    // Per spec: exactly one startup message after successful index load AND port binding
    // Count occurrences of startup message
    size_t count = 0;
    size_t pos = 0;
    std::string search_str = "Server started on port";
    while ((pos = stdout_output.find(search_str, pos)) != std::string::npos)
    {
        count++;
        pos += search_str.length();
    }
    
    // If message not found, check if there was a port binding error that prevented startup
    if (count == 0)
    {
        // Check stderr for port binding errors - if found, that explains why no startup message
        bool has_port_error = stderr_output.find("Failed to bind to port") != std::string::npos;
        bool has_load_error = stderr_output.find("Error loading index") != std::string::npos;
        
        // If no errors, message should have been printed - this indicates capture issue
        // If errors present, message won't be printed (expected behavior)
        if (!has_port_error && !has_load_error)
        {
            // No errors but no message - likely a capture issue with background process
            // Message is printed in code but not captured by shell redirection
        }
    }
    
    REQUIRE(count == 1);
}

TEST_CASE("Successful startup: message contains port number and index directory")
{
    std::string index_dir = create_test_index();
    std::string searchd_path = find_searchd_path();
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(9000, 9999);
    int test_port = dis(gen);
    
    // Use PID-based unique filenames to avoid conflicts
    pid_t pid = getpid();
    std::string stdout_file_path = "/tmp/haystack_startup_stdout2_" + std::to_string(pid);
    std::string stderr_file_path = "/tmp/haystack_startup_stderr2_" + std::to_string(pid);
    
    // Option C: Capture output from background process without subshell for better reliability
    // Pattern: cmd >file1 2>file2 & PID=$!; sleep; kill; wait; sync
    std::string cmd = searchd_path + " --serve --in \"" + index_dir + "\" --port " + std::to_string(test_port);
    std::string full_cmd = cmd + " >" + stdout_file_path + " 2>" + stderr_file_path + " & SERVE_PID=$!; sleep 2; kill $SERVE_PID 2>/dev/null || true; wait $SERVE_PID 2>/dev/null; sleep 0.2; sync";
    std::system(full_cmd.c_str());
    
    // Additional delay and try reading multiple times if file is empty (handle race conditions)
    for (int attempt = 0; attempt < 5; ++attempt)
    {
        usleep(100000); // 100ms between attempts
        std::ifstream test_file(stdout_file_path);
        if (test_file && test_file.peek() != std::ifstream::traits_type::eof())
        {
            break; // File has content, proceed to read
        }
    }
    
    std::ifstream stdout_file(stdout_file_path);
    std::string stdout_output;
    if (stdout_file)
    {
        stdout_output.assign((std::istreambuf_iterator<char>(stdout_file)),
                             std::istreambuf_iterator<char>());
        stdout_file.close();
    }
    
    std::remove(stdout_file_path.c_str());
    std::remove(stderr_file_path.c_str());
    cleanup_temp_dir(index_dir);
    
    // Per spec: "Server started on port <port> using index: <index_dir>\n"
    REQUIRE(stdout_output.find("Server started on port") != std::string::npos);
    REQUIRE(stdout_output.find(std::to_string(test_port)) != std::string::npos);
    REQUIRE(stdout_output.find(index_dir) != std::string::npos);
    REQUIRE(stdout_output.find("using index:") != std::string::npos);
}

TEST_CASE("Startup message: appears on stdout, not stderr")
{
    std::string index_dir = create_test_index();
    std::string searchd_path = find_searchd_path();
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(9000, 9999);
    int test_port = dis(gen);
    
    // Use PID-based unique filenames to avoid conflicts
    pid_t pid = getpid();
    std::string stdout_file_path = "/tmp/haystack_startup_stdout3_" + std::to_string(pid);
    std::string stderr_file_path = "/tmp/haystack_startup_stderr3_" + std::to_string(pid);
    
    // Option C: Capture output from background process without subshell for better reliability
    // Pattern: cmd >file1 2>file2 & PID=$!; sleep; kill; wait; sync
    std::string cmd = searchd_path + " --serve --in \"" + index_dir + "\" --port " + std::to_string(test_port);
    std::string full_cmd = cmd + " >" + stdout_file_path + " 2>" + stderr_file_path + " & SERVE_PID=$!; sleep 2; kill $SERVE_PID 2>/dev/null || true; wait $SERVE_PID 2>/dev/null; sleep 0.2; sync";
    std::system(full_cmd.c_str());
    
    // Additional delay and try reading multiple times if file is empty (handle race conditions)
    for (int attempt = 0; attempt < 5; ++attempt)
    {
        usleep(100000); // 100ms between attempts
        std::ifstream test_file(stdout_file_path);
        if (test_file && test_file.peek() != std::ifstream::traits_type::eof())
        {
            break; // File has content, proceed to read
        }
    }
    
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
    
    std::remove(stdout_file_path.c_str());
    std::remove(stderr_file_path.c_str());
    cleanup_temp_dir(index_dir);
    
    // Per spec: startup success messages appear only on stdout
    REQUIRE(stdout_output.find("Server started on port") != std::string::npos);
    REQUIRE(stderr_output.find("Server started on port") == std::string::npos);
}
