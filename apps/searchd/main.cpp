#include <drogon/drogon.h>
#include "core/search_service.h"
#include <fstream>
#include <cstdlib>
#include <iostream>
#include <cstdio>
#include <string>
#include <filesystem>
#include <csignal>
#include <atomic>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <cerrno>
#include <cstdarg>

namespace fs = std::filesystem;

// Phase 2.4: Server readiness tracking
static std::atomic<bool> g_server_ready{false};

// Phase 2.3: Signal handling for graceful shutdown
// Phase 2.4: Shutdown idempotency - prevent duplicate signal handling
static std::atomic<bool> g_shutdown_requested{false};
static std::atomic<bool> g_shutdown_in_progress{false};
static void signal_handler(int signal)
{
    if (signal == SIGINT || signal == SIGTERM)
    {
        // Phase 2.4: Idempotent signal handling - ignore if already shutting down
        bool expected = false;
        if (g_shutdown_in_progress.compare_exchange_strong(expected, true))
        {
            g_shutdown_requested = true;
            drogon::app().quit();
        }
    }
}

static void load_docs_from_json(SearchService &s, const std::string &path)
{
    std::ifstream in(path);
    if (!in)
    {
        throw std::runtime_error("Failed to open docs file: " + path);
    }

    Json::Value root;
    in >> root;

    for (const auto &item : root)
    {
        int docId = item["docId"].asInt();
        std::string text = item["text"].asString();
        s.add_document(docId, text);
    }
}

static bool has_flag(int argc, char **argv, const std::string &flag)
{
    for (int i = 1; i < argc; ++i)
        if (std::string(argv[i]) == flag)
            return true;
    return false;
}

static std::string get_arg_value(int argc, char **argv, const std::string &key)
{
    for (int i = 1; i + 1 < argc; ++i)
        if (argv[i] == key)
            return argv[i + 1];
    return "";
}

static std::string getenv_str(const char *name)
{
    const char *v = std::getenv(name);
    return v ? std::string(v) : "";
}

static int getenv_int(const char *name, int def)
{
    auto s = getenv_str(name);
    if (s.empty())
        return def;
    try
    {
        return std::stoi(s);
    }
    catch (...)
    {
        return def;
    }
}

static int get_int_value(int argc, char **argv, const std::string &key, int def)
{
    auto v = get_arg_value(argc, argv, key);
    if (v.empty())
        return def;
    try
    {
        return std::stoi(v);
    }
    catch (...)
    {
        return def;
    }
}

static void print_help()
{
    std::cout
        << "Usage:\n"
        << "  searchd --index --docs <path> --out <index_dir>\n"
        << "  searchd --serve --in <index_dir> --port <port>\n";
}

int main(int argc, char **argv)
{
    // Phase 2.3: Ensure stdout/stderr are synchronized and unbuffered for immediate output
    std::ios::sync_with_stdio(true);
    std::setvbuf(stdout, NULL, _IONBF, 0); // Disable stdout buffering
    std::setvbuf(stderr, NULL, _IONBF, 0); // Disable stderr buffering

    // No args ==> help

    if (argc == 1 || has_flag(argc, argv, "--help"))
    {
        print_help();
        return 0;
    }

    // Flag Exclusivity Must be checked BEFORE anything else
    // Per spec: Validation order: 1) Flag conflicts, 2) Mode requirement, 3) Required flags

    const bool index_mode = has_flag(argc, argv, "--index");
    const bool serve_mode = has_flag(argc, argv, "--serve");

    // 1. Check for conflicting mode flags
    if (index_mode && serve_mode)
    {
        std::cerr << "Error: --index and --serve cannot be used together\n";
        return 2;
    }

    // 2. Check for invalid flag combinations
    if (index_mode && has_flag(argc, argv, "--in"))
    {
        std::cerr << "Error: --in cannot be used with --index mode\n";
        return 2;
    }

    if (index_mode && has_flag(argc, argv, "--port"))
    {
        std::cerr << "Error: --port cannot be used with --index mode\n";
        return 2;
    }

    if (serve_mode && has_flag(argc, argv, "--docs"))
    {
        std::cerr << "Error: --docs cannot be used with --serve mode\n";
        return 2;
    }

    if (serve_mode && has_flag(argc, argv, "--out"))
    {
        std::cerr << "Error: --out cannot be used with --serve mode\n";
        return 2;
    }

    // 3. Check that at least one mode is specified
    if (!index_mode && !serve_mode)
    {
        std::cerr << "Error: Missing required mode flag (--index or --serve)\n";
        return 2;
    }

    // Docs path: CLI > ENV > default
    std::string docs = get_arg_value(argc, argv, "--docs");
    if (docs.empty())
        docs = getenv_str("DOCS_PATH");
    if (docs.empty())
        docs = "data/docs.json";

    // Index output directory: CLI only
    std::string index_dir = get_arg_value(argc, argv, "--out");
    std::string in_dir = get_arg_value(argc, argv, "--in");

    // Port: CLI > ENV > fallback
#ifndef SEARCHD_PORT
#define SEARCHD_PORT 8900
#endif
    int port = get_int_value(argc, argv, "--port", getenv_int("SEARCHD_PORT", SEARCHD_PORT));

    // Create Search Engine Instance
    SearchService s;

    // Sample documents (temporary -later loaded from disk/db)
    // s.add_document(1, "Teamcenter migration guide: map attributes, validate schema, run dry-run.");
    // s.add_document(2, "PLM data migration: cleansing, mapping, validation.");
    // s.add_document(3, "Schema validation checklist for integration projects.");
    // s.add_document(1, "The quick brown fox jumps over the lazy dog.");
    // load_docs_from_json(s, "data/docs.json");
    // Fix for the bug.  libc++abi: terminating due to uncaught exception of type std::runtime_error: Failed to open docs file: data/docs.json zsh: abort      ./build/searchd --serve --in idx --port 8900 Need to comment lines from 112 to 116

    /*if (docs.empty())
        docs = getenv_str("DOCS_PATH");
    if (docs.empty())
        docs = "data/docs.json";
    load_docs_from_json(s, docs);*/

    // Index-only mode: build index, save it, and exit
    if (index_mode)
    {
        if (index_dir.empty())
        {
            std::cerr << "Error: --out <index_dir> is required when using --index mode\n";
            return 2;
        }

        // Fix for Index mode: missing --docs flag exits with code 2 - Explicit -- docs validation
        //-------------------------------------------------------------------------------
        // /Users/vilastadoori/_Haystack_proj/tests/test_cli_validation.cpp:140
        //...............................................................................

        /// Users/vilastadoori/_Haystack_proj/tests/test_cli_validation.cpp:150: FAILED:
        // REQUIRE( exit_code == 2 )
        // with expansion:
        // 3 == 2

        if (!has_flag(argc, argv, "--docs"))
        {
            std::cerr << "Error: --docs <path> is required when using --index mode\n";
            return 2;
        }

        try
        {
            load_docs_from_json(s, docs); // <-- ONLY here in index mode
            s.save(index_dir);
            std::cout << "Indexing completed. Index saved to: " << index_dir << "\n";
            return 0;
        }
        catch (const std::exception &e)
        {
            std::cerr << "Error indexing/saving: " << e.what() << "\n";
            return 3;
        }
    }

    // Serve mode:: load persisted index, then run HTTP server

    if (serve_mode)
    {
        if (in_dir.empty())
        {
            std::cerr << "Error: --in <index_dir> is required when using --serve mode\n";
            return 2;
        }
        // Fix Explicit --port validation for serve mode.
        if (!has_flag(argc, argv, "--port"))
        {
            std::cerr << "Error: --port <port> is required when using --serve mode\n";
            return 2;
        }
        std::string port_str = get_arg_value(argc, argv, "--port");
        // Validate numeric + range (per spec: "Error: Invalid port number: <port>\n")
        try
        {
            int port_num = std::stoi(port_str);
            if (port_num < 1 || port_num > 65535)
            {
                std::cerr << "Error: Invalid port number: " << port_str << "\n";
                return 2;
            }
            port = port_num;
        }
        catch (...)
        {
            // Per spec: "Error: Invalid port number: <port>\n"
            std::cerr << "Error: Invalid port number: " << port_str << "\n";
            return 2;
        }

        // Per spec: Validate index directory exists BEFORE loading
        // "Index directory missing: Print Error: Index directory not found: <index_dir>\n to stderr, exit 3"
        if (!fs::exists(in_dir) || !fs::is_directory(in_dir))
        {
            std::cerr << "Error: Index directory not found: " << in_dir << "\n";
            return 3;
        }

        // Per spec: Validate required index files exist BEFORE loading
        // "Missing index files: Print Error: Index file not found: <file_path>\n to stderr, exit 3"
        std::string meta_file = in_dir + "/index_meta.json";
        std::string docs_file = in_dir + "/docs.jsonl";
        std::string postings_file = in_dir + "/postings.bin";

        if (!fs::exists(meta_file))
        {
            std::cerr << "Error: Index file not found: " << meta_file << "\n";
            return 3;
        }
        if (!fs::exists(docs_file))
        {
            std::cerr << "Error: Index file not found: " << docs_file << "\n";
            return 3;
        }
        if (!fs::exists(postings_file))
        {
            std::cerr << "Error: Index file not found: " << postings_file << "\n";
            return 3;
        }

        try
        {
            s.load(in_dir); // <-- ONLY here in serve mode
        }
        catch (const std::exception &e)
        {
            // Per spec: "Error loading index: <specific_error>\n"
            // The load() function already formats errors correctly (e.g., "Unsupported schema version: <version>")
            std::cerr << "Error loading index: " << e.what() << "\n";
            return 3;
        }
    }

    // Phase 2.3: If not in serve mode, nothing more to do
    if (!serve_mode)
    {
        return 0;
    }

    // Phase 2.3: Register signal handlers for graceful shutdown (serve mode only)
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    // Phase 2.4: Health endpoint with readiness and shutdown state checks
    drogon::app().registerHandler(
        "/health",
        [](const drogon::HttpRequestPtr &req,
           std::function<void(const drogon::HttpResponsePtr &)> &&callback)
        {
            auto resp = drogon::HttpResponse::newHttpResponse();
            
            // Phase 2.4: Return HTTP 200 ONLY when ready and not shutting down
            if (g_server_ready.load() && !g_shutdown_in_progress.load())
            {
                resp->setStatusCode(drogon::k200OK);
                resp->setContentTypeCode(drogon::CT_TEXT_PLAIN);
                // Phase 2.4: Deterministic, constant response body
                resp->setBody("OK");
            }
            else
            {
                // Phase 2.4: Return non-200 when not ready or shutting down
                resp->setStatusCode(drogon::k503ServiceUnavailable);
                resp->setContentTypeCode(drogon::CT_TEXT_PLAIN);
                resp->setBody("");
            }
            callback(resp);
        });
    // Search endpoint
    drogon::app().registerHandler(
        "/search",
        [&s](const drogon::HttpRequestPtr &req,
             std::function<void(const drogon::HttpResponsePtr &)> &&callback)
        {
            auto q = req->getParameter("q");
            int k = 10;
            auto kstr = req->getParameter("k");
            if (!kstr.empty())
            {
                try
                {
                    k = std::max(1, std::stoi(kstr));
                }
                catch (...)
                {
                    // ignore invalid k param
                }
            }

            auto hits = s.search_with_snippets(q);
            if (hits.size() > static_cast<size_t>(k))
            {
                hits.resize(k);
            }

            Json::Value json(Json::objectValue);
            json["query"] = q;

            Json::Value arr(Json::arrayValue);
            for (const auto &hit : hits)
            {
                Json::Value h(Json::objectValue);
                h["docId"] = hit.docId;
                h["score"] = hit.score;
                h["snippet"] = hit.snippet;
                arr.append(h);
            }
            json["results"] = arr;
            callback(drogon::HttpResponse::newHttpJsonResponse(json));
        });

    // Phase 2.3: Port binding and startup message
    // Per spec: Port binding must succeed BEFORE printing startup message
    // Per spec: Print exactly ONE startup message: "Server started on port <port> using index: <index_dir>\n"

    // Phase 2.3: Test port binding before starting server (per spec: must detect failure before startup)
    // Create a test socket to verify port is available (without SO_REUSEADDR to properly detect conflicts)
    int test_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (test_sock >= 0)
    {
        struct sockaddr_in addr;
        std::memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(port);

        // Try to bind WITHOUT SO_REUSEADDR - if this fails, port is already in use or permission denied
        if (bind(test_sock, (struct sockaddr *)&addr, sizeof(addr)) < 0)
        {
            close(test_sock);
            // Per spec: "Error: Failed to bind to port <port>: <error_message>\n"
            // Capture errno immediately before calling strerror
            int saved_errno = errno;
            std::string error_msg = std::strerror(saved_errno);
            std::cerr << "Error: Failed to bind to port " << port << ": " << error_msg << "\n";
            std::cerr.flush(); // Ensure error is written before exit
            return 3;
        }

        // Close test socket - actual binding will happen in drogon (drogon will use SO_REUSEADDR if needed)
        close(test_sock);
    }

    // Phase 2.3: Print startup message after port check succeeds (per spec: after index load AND port binding verification)
    // Print directly here since we've verified the port is available, and actual binding will succeed in run()
    // Use write() directly to file descriptor 1 (stdout) to bypass any buffering, even when redirected
    // Per spec: Print exactly ONE startup message
    std::string msg = "Server started on port " + std::to_string(port) + " using index: " + in_dir + "\n";
    write(1, msg.c_str(), msg.length()); // write() to fd 1 (stdout) - immediate, no buffering
    fsync(1); // Force sync if stdout is a file

    // Phase 2.3: Register startup message callback as backup (should not be called if message already printed)
    // This is kept for compatibility but the message is printed above
    drogon::app().registerBeginningAdvice([port, in_dir]()
                                          { 
                                            // Message already printed above, this callback should not execute
                                          });

    // Phase 2.4: Register callback to mark server as ready after it starts accepting connections
    drogon::app().registerBeginningAdvice([]()
                                          {
                                            // Phase 2.4: Mark server as ready after HTTP server starts accepting connections
                                            g_server_ready.store(true);
                                          });

    // Register the listener - actual binding happens during run()
    drogon::app().addListener("0.0.0.0", port);

    // Start the server - binding happens here but we've already verified it will succeed
    try
    {
        drogon::app().run();

        // Phase 2.3: Clean shutdown returns exit code 0
        return 0;
    }
    catch (const std::exception &e)
    {
        // Catch any exceptions during run() - should not happen if port binding succeeded
        // Per spec: "Error: Failed to bind to port <port>: <error_message>\n"
        std::cerr << "Error: Failed to bind to port " << port << ": " << e.what() << "\n";
        return 3;
    }
    catch (...)
    {
        // Catch any other exceptions (should not happen)
        std::cerr << "Error: Failed to bind to port " << port << ": Unknown error\n";
        return 3;
    }
}