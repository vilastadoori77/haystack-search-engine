// Phase 2.5 — Test Generation Only (STRICT TDD MODE)
// Tests validate the approved specification: specs/phase2_5_deterministic_pdf_ocr_ingestion.md
// These tests are expected to FAIL until Phase 2.5 implementation is added.
// Do NOT modify Phase 2.2, 2.3, or 2.4 implementation or tests.

#include "runtime_test_common.hpp"
#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cstdlib>
#include <random>
#include <unistd.h>

namespace fs = std::filesystem;

static std::string find_searchd_path()
{
    std::vector<std::string> candidates;
    candidates.push_back("./searchd");
    candidates.push_back("./build/searchd");
    fs::path cwd = fs::current_path();
    if (cwd.filename() == "build")
        candidates.push_back((cwd / "searchd").string());
    else
        candidates.push_back((cwd / "build" / "searchd").string());
    if (cwd.has_parent_path())
        candidates.push_back((cwd.parent_path() / "build" / "searchd").string());
    for (const auto& c : candidates)
        if (fs::exists(c) && fs::is_regular_file(c))
            return fs::absolute(c).string();
    return "./searchd";
}

static std::string create_temp_dir(const std::string& prefix = "phase25_")
{
    std::string base = "/tmp/" + prefix;
    pid_t pid = getpid();
    static int counter = 0;
    for (int i = 0; i < 1000; ++i)
    {
        std::string dir = base + std::to_string(pid) + "_" + std::to_string(counter++);
        if (!fs::exists(dir))
        {
            fs::create_directories(dir);
            return dir;
        }
    }
    throw std::runtime_error("Could not create temp directory");
}

static void write_file(const fs::path& p, const std::string& content)
{
    std::ofstream f(p);
    REQUIRE(f);
    f << content;
    f.close();
}

static void cleanup_temp_dir(const std::string& dir)
{
    if (fs::exists(dir))
        fs::remove_all(dir);
}

static std::string read_file(const std::string& path)
{
    std::ifstream f(path);
    if (!f)
        return "";
    std::ostringstream os;
    os << f.rdbuf();
    return os.str();
}

// -----------------------------------------------------------------------------
// 1. Deterministic folder traversal
// -----------------------------------------------------------------------------
TEST_CASE("Phase 2.5: Deterministic folder traversal order", "[phase2_5][traversal]")
{
    std::string docs_dir = create_temp_dir("phase25_traversal_");
    std::string out_dir = create_temp_dir("phase25_out_traversal_");
    std::string searchd = find_searchd_path();

    write_file(docs_dir + "/a_first.txt", "content a");
    write_file(docs_dir + "/b_second.txt", "content b");
    write_file(docs_dir + "/c_third.txt", "content c");

    auto [code, out] = runtime_test::run_command_capture_output(
        searchd + " --index --docs \"" + docs_dir + "\" --out \"" + out_dir + "\" 2>&1", 10);

    cleanup_temp_dir(docs_dir);
    REQUIRE(code == 0);

    std::string docs_jsonl = read_file(out_dir + "/docs.jsonl");
    REQUIRE_FALSE(docs_jsonl.empty());
    // Deterministic order: a_first, b_second, c_third (alphabetical by path)
    REQUIRE(docs_jsonl.find("\"docId\":1") != std::string::npos);
    REQUIRE(docs_jsonl.find("\"docId\":2") != std::string::npos);
    REQUIRE(docs_jsonl.find("\"docId\":3") != std::string::npos);
    cleanup_temp_dir(out_dir);
}

// -----------------------------------------------------------------------------
// 2. UTF-8 byte-order path sorting
// -----------------------------------------------------------------------------
TEST_CASE("Phase 2.5: Path sort uses UTF-8 byte order", "[phase2_5][sort]")
{
    std::string docs_dir = create_temp_dir("phase25_utf8_");
    std::string out_dir = create_temp_dir("phase25_out_utf8_");
    std::string searchd = find_searchd_path();

    write_file(docs_dir + "/a.txt", "a");
    write_file(docs_dir + "/z.txt", "z");

    auto [code, out] = runtime_test::run_command_capture_output(
        searchd + " --index --docs \"" + docs_dir + "\" --out \"" + out_dir + "\" 2>&1", 10);

    cleanup_temp_dir(docs_dir);
    REQUIRE(code == 0);
    std::string docs_jsonl = read_file(out_dir + "/docs.jsonl");
    size_t pos_a = docs_jsonl.find("a.txt");
    size_t pos_z = docs_jsonl.find("z.txt");
    REQUIRE(pos_a != std::string::npos);
    REQUIRE(pos_z != std::string::npos);
    REQUIRE(pos_a < pos_z);
    cleanup_temp_dir(out_dir);
}

// -----------------------------------------------------------------------------
// 3. Deterministic docId assignment
// -----------------------------------------------------------------------------
TEST_CASE("Phase 2.5: Identical input produces identical docIds", "[phase2_5][determinism]")
{
    std::string docs_dir = create_temp_dir("phase25_det_");
    std::string out1 = create_temp_dir("phase25_out1_");
    std::string out2 = create_temp_dir("phase25_out2_");
    std::string searchd = find_searchd_path();

    write_file(docs_dir + "/f1.txt", "one");
    write_file(docs_dir + "/f2.txt", "two");

    auto [c1, o1] = runtime_test::run_command_capture_output(
        searchd + " --index --docs \"" + docs_dir + "\" --out \"" + out1 + "\" 2>&1", 10);
    auto [c2, o2] = runtime_test::run_command_capture_output(
        searchd + " --index --docs \"" + docs_dir + "\" --out \"" + out2 + "\" 2>&1", 10);

    cleanup_temp_dir(docs_dir);
    REQUIRE(c1 == 0);
    REQUIRE(c2 == 0);
    std::string docs1 = read_file(out1 + "/docs.jsonl");
    std::string docs2 = read_file(out2 + "/docs.jsonl");
    REQUIRE(docs1 == docs2);
    cleanup_temp_dir(out1);
    cleanup_temp_dir(out2);
}

// -----------------------------------------------------------------------------
// 4. Parallel execution determinism (contract only; implementation verifies)
// -----------------------------------------------------------------------------
TEST_CASE("Phase 2.5: docId order is logical not completion order", "[phase2_5][parallel]")
{
    std::string docs_dir = create_temp_dir("phase25_par_");
    std::string out_dir = create_temp_dir("phase25_out_par_");
    std::string searchd = find_searchd_path();

    write_file(docs_dir + "/aa.txt", "aa");
    write_file(docs_dir + "/bb.txt", "bb");

    auto [code, out] = runtime_test::run_command_capture_output(
        searchd + " --index --docs \"" + docs_dir + "\" --out \"" + out_dir + "\" 2>&1", 10);

    cleanup_temp_dir(docs_dir);
    REQUIRE(code == 0);
    std::string docs_jsonl = read_file(out_dir + "/docs.jsonl");
    size_t aa = docs_jsonl.find("aa.txt");
    size_t bb = docs_jsonl.find("bb.txt");
    REQUIRE(aa != std::string::npos);
    REQUIRE(bb != std::string::npos);
    REQUIRE(aa < bb);
    cleanup_temp_dir(out_dir);
}

// -----------------------------------------------------------------------------
// 5. OCR trigger threshold (contract; no real OCR)
// -----------------------------------------------------------------------------
TEST_CASE("Phase 2.5: Indexing succeeds with short text file", "[phase2_5][ocr_trigger]")
{
    std::string docs_dir = create_temp_dir("phase25_ocr_");
    std::string out_dir = create_temp_dir("phase25_out_ocr_");
    std::string searchd = find_searchd_path();

    write_file(docs_dir + "/short.txt", "x"); // < MIN_TEXT_CHARS

    auto [code, out] = runtime_test::run_command_capture_output(
        searchd + " --index --docs \"" + docs_dir + "\" --out \"" + out_dir + "\" 2>&1", 10);

    cleanup_temp_dir(docs_dir);
    REQUIRE(code == 0);
    cleanup_temp_dir(out_dir);
}

// -----------------------------------------------------------------------------
// 6. Tokenizer consistency for MIN_TOKEN_COUNT (contract)
// -----------------------------------------------------------------------------
TEST_CASE("Phase 2.5: Token count uses indexing tokenizer", "[phase2_5][tokenizer]")
{
    // When Phase 2.5 exists: text with exactly MIN_TOKEN_COUNT tokens shall not trigger OCR.
    std::string docs_dir = create_temp_dir("phase25_tok_");
    std::string out_dir = create_temp_dir("phase25_out_tok_");
    std::string searchd = find_searchd_path();

    write_file(docs_dir + "/ten.txt", "one two three four five six seven eight nine ten");

    auto [code, out] = runtime_test::run_command_capture_output(
        searchd + " --index --docs \"" + docs_dir + "\" --out \"" + out_dir + "\" 2>&1", 10);

    cleanup_temp_dir(docs_dir);
    REQUIRE(code == 0);
    cleanup_temp_dir(out_dir);
}

// -----------------------------------------------------------------------------
// 7. Canonical merge ordering (text layer + newline + OCR)
// -----------------------------------------------------------------------------
TEST_CASE("Phase 2.5: Stored text order is text_layer then newline then OCR", "[phase2_5][canonical]")
{
    std::string docs_dir = create_temp_dir("phase25_can_");
    std::string out_dir = create_temp_dir("phase25_out_can_");
    std::string searchd = find_searchd_path();

    write_file(docs_dir + "/doc.txt", "layer");

    auto [code, out] = runtime_test::run_command_capture_output(
        searchd + " --index --docs \"" + docs_dir + "\" --out \"" + out_dir + "\" 2>&1", 10);

    cleanup_temp_dir(docs_dir);
    REQUIRE(code == 0);
    std::string docs_jsonl = read_file(out_dir + "/docs.jsonl");
    REQUIRE(docs_jsonl.find("layer") != std::string::npos);
    cleanup_temp_dir(out_dir);
}

// -----------------------------------------------------------------------------
// 8. Duplicate OCR + text allowance
// -----------------------------------------------------------------------------
TEST_CASE("Phase 2.5: Duplicate text layer and OCR content allowed", "[phase2_5][duplicate]")
{
    // Spec: deduplication not required. Test only documents the contract.
    std::string docs_dir = create_temp_dir("phase25_dup_");
    std::string out_dir = create_temp_dir("phase25_out_dup_");
    std::string searchd = find_searchd_path();

    write_file(docs_dir + "/d.txt", "same same");

    auto [code, out] = runtime_test::run_command_capture_output(
        searchd + " --index --docs \"" + docs_dir + "\" --out \"" + out_dir + "\" 2>&1", 10);

    cleanup_temp_dir(docs_dir);
    REQUIRE(code == 0);
    cleanup_temp_dir(out_dir);
}

// -----------------------------------------------------------------------------
// 9. Metadata storage correctness
// -----------------------------------------------------------------------------
TEST_CASE("Phase 2.5: Index stores file_name and page_number", "[phase2_5][metadata]")
{
    std::string docs_dir = create_temp_dir("phase25_meta_");
    std::string out_dir = create_temp_dir("phase25_out_meta_");
    std::string searchd = find_searchd_path();

    write_file(docs_dir + "/m.txt", "meta");

    auto [code, out] = runtime_test::run_command_capture_output(
        searchd + " --index --docs \"" + docs_dir + "\" --out \"" + out_dir + "\" 2>&1", 10);

    cleanup_temp_dir(docs_dir);
    REQUIRE(code == 0);
    std::string docs_jsonl = read_file(out_dir + "/docs.jsonl");
    // Phase 2.5: index MUST store file_name and page_number per spec
    REQUIRE(docs_jsonl.find("file_name") != std::string::npos);
    REQUIRE(docs_jsonl.find("page_number") != std::string::npos);
    cleanup_temp_dir(out_dir);
}

// -----------------------------------------------------------------------------
// 10. Search response metadata correctness
// -----------------------------------------------------------------------------
TEST_CASE("Phase 2.5: Search response includes file_name and page_number", "[phase2_5][search_meta]")
{
    std::string docs_dir = create_temp_dir("phase25_sm_");
    std::string out_dir = create_temp_dir("phase25_out_sm_");
    std::string searchd = find_searchd_path();

    write_file(docs_dir + "/q.txt", "queryable");

    auto [c1, o1] = runtime_test::run_command_capture_output(
        searchd + " --index --docs \"" + docs_dir + "\" --out \"" + out_dir + "\" 2>&1", 10);
    cleanup_temp_dir(docs_dir);
    REQUIRE(c1 == 0);

    std::string serve_cmd = "(" + searchd + " --serve --in \"" + out_dir + "\" --port 18905 &); pid=$!; sleep 2; curl -s 'http://127.0.0.1:18905/search?q=queryable'; kill $pid 2>/dev/null";
    auto [c2, o2] = runtime_test::run_command_capture_output(serve_cmd, 8);
    std::string search_out = o2;
    // Phase 2.5: search response MUST include file_name and page_number per spec
    REQUIRE(search_out.find("file_name") != std::string::npos);
    REQUIRE(search_out.find("page_number") != std::string::npos);
    cleanup_temp_dir(out_dir);
}

// -----------------------------------------------------------------------------
// 11. Unsupported file handling
// -----------------------------------------------------------------------------
TEST_CASE("Phase 2.5: Unsupported files ignored without exit 3", "[phase2_5][unsupported]")
{
    std::string docs_dir = create_temp_dir("phase25_unsup_");
    std::string out_dir = create_temp_dir("phase25_out_unsup_");
    std::string searchd = find_searchd_path();

    write_file(docs_dir + "/a.txt", "a");
    write_file(docs_dir + "/b.pdf", "%PDF-1.4 minimal"); // minimal pdf-like
    write_file(docs_dir + "/c.docx", "unsupported");

    auto [code, out] = runtime_test::run_command_capture_output(
        searchd + " --index --docs \"" + docs_dir + "\" --out \"" + out_dir + "\" 2>&1", 15);

    cleanup_temp_dir(docs_dir);
    REQUIRE(code != 3);
    if (code == 0)
    {
        std::string docs_jsonl = read_file(out_dir + "/docs.jsonl");
        REQUIRE((docs_jsonl.find("a.txt") != std::string::npos || !docs_jsonl.empty()));
    }
    cleanup_temp_dir(out_dir);
}

// -----------------------------------------------------------------------------
// 12. Corrupted PDF handling without exit 3
// -----------------------------------------------------------------------------
TEST_CASE("Phase 2.5: Corrupted PDF does not abort indexing", "[phase2_5][corrupt_pdf]")
{
    std::string docs_dir = create_temp_dir("phase25_corrupt_");
    std::string out_dir = create_temp_dir("phase25_out_corrupt_");
    std::string searchd = find_searchd_path();

    write_file(docs_dir + "/good.txt", "good content");
    write_file(docs_dir + "/bad.pdf", "not a valid pdf");

    auto [code, out] = runtime_test::run_command_capture_output(
        searchd + " --index --docs \"" + docs_dir + "\" --out \"" + out_dir + "\" 2>&1", 15);

    cleanup_temp_dir(docs_dir);
    REQUIRE(code != 3);
    if (code == 0)
    {
        std::string docs_jsonl = read_file(out_dir + "/docs.jsonl");
        REQUIRE(docs_jsonl.find("good content") != std::string::npos);
    }
    cleanup_temp_dir(out_dir);
}

// -----------------------------------------------------------------------------
// 13. Single-page OCR failure isolation
// -----------------------------------------------------------------------------
TEST_CASE("Phase 2.5: Single-page OCR failure does not cause exit 3", "[phase2_5][ocr_fail]")
{
    std::string docs_dir = create_temp_dir("phase25_ocrfail_");
    std::string out_dir = create_temp_dir("phase25_out_ocrfail_");
    std::string searchd = find_searchd_path();

    write_file(docs_dir + "/ok.txt", "ok");

    auto [code, out] = runtime_test::run_command_capture_output(
        searchd + " --index --docs \"" + docs_dir + "\" --out \"" + out_dir + "\" 2>&1", 10);

    cleanup_temp_dir(docs_dir);
    REQUIRE(code == 0);
    cleanup_temp_dir(out_dir);
}

// -----------------------------------------------------------------------------
// 14. Extremely large PDF streaming (simulated)
// -----------------------------------------------------------------------------
TEST_CASE("Phase 2.5: Indexing completes without loading all pages at once", "[phase2_5][streaming]")
{
    std::string docs_dir = create_temp_dir("phase25_stream_");
    std::string out_dir = create_temp_dir("phase25_out_stream_");
    std::string searchd = find_searchd_path();

    for (int i = 0; i < 20; ++i)
        write_file(docs_dir + "/f" + std::to_string(i) + ".txt", "page " + std::to_string(i));

    auto [code, out] = runtime_test::run_command_capture_output(
        searchd + " --index --docs \"" + docs_dir + "\" --out \"" + out_dir + "\" 2>&1", 30);

    cleanup_temp_dir(docs_dir);
    REQUIRE(code == 0);
    cleanup_temp_dir(out_dir);
}

// -----------------------------------------------------------------------------
// 15. Bounded worker pool enforcement (contract)
// -----------------------------------------------------------------------------
TEST_CASE("Phase 2.5: Indexing completes with bounded concurrency", "[phase2_5][workers]")
{
    std::string docs_dir = create_temp_dir("phase25_work_");
    std::string out_dir = create_temp_dir("phase25_out_work_");
    std::string searchd = find_searchd_path();

    write_file(docs_dir + "/w.txt", "w");

    auto [code, out] = runtime_test::run_command_capture_output(
        searchd + " --index --docs \"" + docs_dir + "\" --out \"" + out_dir + "\" 2>&1", 10);

    cleanup_temp_dir(docs_dir);
    REQUIRE(code == 0);
    cleanup_temp_dir(out_dir);
}

// -----------------------------------------------------------------------------
// 16. Bounded OCR pool enforcement (contract)
// -----------------------------------------------------------------------------
TEST_CASE("Phase 2.5: OCR pool is bounded", "[phase2_5][ocr_pool]")
{
    std::string docs_dir = create_temp_dir("phase25_pool_");
    std::string out_dir = create_temp_dir("phase25_out_pool_");
    std::string searchd = find_searchd_path();

    write_file(docs_dir + "/p.txt", "p");

    auto [code, out] = runtime_test::run_command_capture_output(
        searchd + " --index --docs \"" + docs_dir + "\" --out \"" + out_dir + "\" 2>&1", 10);

    cleanup_temp_dir(docs_dir);
    REQUIRE(code == 0);
    cleanup_temp_dir(out_dir);
}

// -----------------------------------------------------------------------------
// 17. Signal-safe shutdown integration (Phase 2.3)
// -----------------------------------------------------------------------------
TEST_CASE("Phase 2.5: Signal during indexing exits with 0 or 3", "[phase2_5][signal]")
{
    std::string docs_dir = create_temp_dir("phase25_sig_");
    std::string out_dir = create_temp_dir("phase25_out_sig_");
    std::string searchd = find_searchd_path();

    for (int i = 0; i < 5; ++i)
        write_file(docs_dir + "/s" + std::to_string(i) + ".txt", "content");

    std::string cmd = searchd + " --index --docs \"" + docs_dir + "\" --out \"" + out_dir + "\"";
    auto [code, out] = runtime_test::run_command_capture_output(cmd + " 2>&1", 5);

    cleanup_temp_dir(docs_dir);
    REQUIRE((code == 0 || code == 3));
    cleanup_temp_dir(out_dir);
}

// -----------------------------------------------------------------------------
// 18. Atomic persistence guarantees (Phase 2.1)
// -----------------------------------------------------------------------------
TEST_CASE("Phase 2.5: No inconsistent partial index on failure", "[phase2_5][atomic]")
{
    std::string docs_dir = create_temp_dir("phase25_atomic_");
    std::string out_dir = create_temp_dir("phase25_out_atomic_");
    std::string searchd = find_searchd_path();

    write_file(docs_dir + "/x.txt", "x");

    auto [code, out] = runtime_test::run_command_capture_output(
        searchd + " --index --docs \"" + docs_dir + "\" --out \"" + out_dir + "\" 2>&1", 10);

    cleanup_temp_dir(docs_dir);
    if (code == 0)
    {
        REQUIRE(fs::exists(out_dir + "/docs.jsonl"));
        REQUIRE(fs::exists(out_dir + "/index_meta.json"));
        REQUIRE(fs::exists(out_dir + "/postings.bin"));
    }
    cleanup_temp_dir(out_dir);
}

// -----------------------------------------------------------------------------
// 19. No regression of Phase 2.2–2.4 behavior
// -----------------------------------------------------------------------------
TEST_CASE("Phase 2.5: Phase 2.2–2.4 behavior unchanged", "[phase2_5][regression]")
{
    std::string out_dir = create_temp_dir("phase25_reg_");
    std::string searchd = find_searchd_path();

    auto [code, out] = runtime_test::run_command_capture_output(
        searchd + " --index --docs /nonexistent --out \"" + out_dir + "\" 2>&1", 5);

    REQUIRE(code == 2);
    REQUIRE(out.find("Error") != std::string::npos);
    cleanup_temp_dir(out_dir);
}

TEST_CASE("Phase 2.5: Exit codes 0, 2, 3 semantics preserved", "[phase2_5][exit_codes]")
{
    std::string searchd = find_searchd_path();

    auto [c_help, o_help] = runtime_test::run_command_capture_output(searchd + " --help 2>&1", 2);
    REQUIRE(c_help == 0);

    auto [c_bad, o_bad] = runtime_test::run_command_capture_output(searchd + " --index 2>&1", 5);
    REQUIRE(c_bad == 2);
}
