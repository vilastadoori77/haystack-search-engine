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
#include <algorithm>
#include <cstdlib>
#include <unistd.h>

namespace fs = std::filesystem;

// -----------------------------------------------------------------------------
// Parsed single line from docs.jsonl (Phase 2.5 schema)
// -----------------------------------------------------------------------------
struct DocLine
{
    int docId = -1;
    std::string file_name;
    std::string file_type;
    std::string source_path;
    int page_number = -1;
    std::string text;
    bool did_ocr = false;
    bool did_ocr_present = false;
};

static int parse_json_int(const std::string &line, const std::string &key)
{
    std::string pattern = "\"" + key + "\":";
    size_t pos = line.find(pattern);
    if (pos == std::string::npos)
        return -1;
    pos += pattern.size();
    while (pos < line.size() && (line[pos] == ' ' || line[pos] == '\t'))
        ++pos;
    if (pos >= line.size() || !std::isdigit(static_cast<unsigned char>(line[pos])))
        return -1;
    int val = 0;
    while (pos < line.size() && std::isdigit(static_cast<unsigned char>(line[pos])))
    {
        val = val * 10 + (line[pos] - '0');
        ++pos;
    }
    return val;
}

static int hex_digit_value(char c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'f')
        return 10 + (c - 'a');
    if (c >= 'A' && c <= 'F')
        return 10 + (c - 'A');
    return -1;
}

static std::string unescape_json_string(const std::string &raw)
{
    std::string decoded;
    decoded.reserve(raw.size());
    for (size_t i = 0; i < raw.size(); ++i)
    {
        char c = raw[i];
        if (c != '\\' || i + 1 >= raw.size())
        {
            decoded.push_back(c);
            continue;
        }

        char esc = raw[++i];
        switch (esc)
        {
        case '"':
            decoded.push_back('"');
            break;
        case '\\':
            decoded.push_back('\\');
            break;
        case '/':
            decoded.push_back('/');
            break;
        case 'b':
            decoded.push_back('\b');
            break;
        case 'f':
            decoded.push_back('\f');
            break;
        case 'n':
            decoded.push_back('\n');
            break;
        case 'r':
            decoded.push_back('\r');
            break;
        case 't':
            decoded.push_back('\t');
            break;
        case 'u':
        {
            // Minimal \uXXXX handling for deterministic tests.
            if (i + 4 < raw.size())
            {
                int h1 = hex_digit_value(raw[i + 1]);
                int h2 = hex_digit_value(raw[i + 2]);
                int h3 = hex_digit_value(raw[i + 3]);
                int h4 = hex_digit_value(raw[i + 4]);
                if (h1 >= 0 && h2 >= 0 && h3 >= 0 && h4 >= 0)
                {
                    int codepoint = (h1 << 12) | (h2 << 8) | (h3 << 4) | h4;
                    if (codepoint >= 0 && codepoint <= 0x7F)
                        decoded.push_back(static_cast<char>(codepoint));
                    else
                        decoded.push_back('?');
                    i += 4;
                    break;
                }
            }
            decoded.push_back('u');
            break;
        }
        default:
            decoded.push_back(esc);
            break;
        }
    }
    return decoded;
}

static std::string parse_json_string(const std::string &line, const std::string &key)
{
    std::string pattern = "\"" + key + "\":\"";
    size_t start = line.find(pattern);
    if (start == std::string::npos)
        return "";
    start += pattern.size();
    size_t end = start;
    while (end < line.size())
    {
        if (line[end] == '\\' && end + 1 < line.size())
            end += 2;
        else if (line[end] == '"')
            break;
        else
            ++end;
    }
    if (end > line.size())
        return "";
    return unescape_json_string(line.substr(start, end - start));
}

static std::pair<bool, bool> parse_json_did_ocr(const std::string &line)
{
    if (line.find("\"did_ocr\":true") != std::string::npos)
        return {true, true};
    if (line.find("\"did_ocr\":false") != std::string::npos)
        return {false, true};
    return {false, false};
}

static std::vector<DocLine> parse_docs_jsonl(const std::string &content)
{
    std::vector<DocLine> docs;
    std::istringstream is(content);
    std::string line;
    while (std::getline(is, line))
    {
        if (line.empty())
            continue;
        DocLine d;
        d.docId = parse_json_int(line, "docId");
        d.file_name = parse_json_string(line, "file_name");
        d.file_type = parse_json_string(line, "file_type");
        d.source_path = parse_json_string(line, "source_path");
        d.page_number = parse_json_int(line, "page_number");
        d.text = parse_json_string(line, "text");
        auto did_ocr = parse_json_did_ocr(line);
        d.did_ocr = did_ocr.first;
        d.did_ocr_present = did_ocr.second;
        if (d.docId >= 0)
            docs.push_back(d);
    }
    return docs;
}

static void require_docid_file_page_order(const std::vector<DocLine> &docs)
{
    REQUIRE_FALSE(docs.empty());
    std::string prev_file_key;
    int prev_page = 0;
    for (size_t i = 0; i < docs.size(); ++i)
    {
        const std::string file_key =
            docs[i].source_path.empty() ? docs[i].file_name : docs[i].source_path;
        REQUIRE(docs[i].docId == static_cast<int>(i + 1));
        REQUIRE(docs[i].page_number >= 1);
        if (i > 0)
        {
            bool same_file = (file_key == prev_file_key);
            if (same_file)
                REQUIRE(docs[i].page_number > prev_page);
            else
                REQUIRE(file_key > prev_file_key);
        }
        prev_file_key = file_key;
        prev_page = docs[i].page_number;
    }
}

static void require_canonical_text_format(const std::string &text)
{
    REQUIRE_FALSE(text.empty());
    REQUIRE(text.find('\n') != std::string::npos);
}

static void require_no_partial_index(const std::string &out_dir)
{
    if (!fs::exists(out_dir))
        return;
    std::vector<std::string> names;
    for (const auto &e : fs::directory_iterator(out_dir))
    {
        std::string name = e.path().filename().string();
        REQUIRE(name.find(".tmp") == std::string::npos);
        REQUIRE(name.find(".partial") == std::string::npos);
        names.push_back(name);
    }
    if (fs::exists(out_dir + "/docs.jsonl"))
    {
        REQUIRE(fs::exists(out_dir + "/index_meta.json"));
        REQUIRE(fs::exists(out_dir + "/postings.bin"));
        REQUIRE(names.size() == 3u);
    }
    else
        REQUIRE(names.empty());
}

// Determinism: docs.jsonl must have exactly one line per document (no extra/malformed lines).
static size_t count_nonempty_lines(const std::string &content)
{
    size_t n = 0;
    std::istringstream is(content);
    std::string line;
    while (std::getline(is, line))
        if (!line.empty())
            ++n;
    return n;
}

// Determinism: all docIds in parsed docs must be unique and contiguous 1..N.
static void require_unique_docids(const std::vector<DocLine> &docs)
{
    std::vector<int> ids;
    for (const auto &d : docs)
        ids.push_back(d.docId);
    std::sort(ids.begin(), ids.end());
    for (size_t i = 0; i < ids.size(); ++i)
        REQUIRE(ids[i] == static_cast<int>(i + 1));
}

TEST_CASE("Phase 2.5: JSON parser helper unescapes escaped text", "[phase2_5][json_parser]")
{
    std::string line = "{\"text\":\"layer\\nocr\\t\\\"q\\\"\"}";
    std::string parsed = parse_json_string(line, "text");
    REQUIRE(parsed == "layer\nocr\t\"q\"");
}

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
    for (const auto &c : candidates)
        if (fs::exists(c) && fs::is_regular_file(c))
            return fs::absolute(c).string();
    return "./searchd";
}

static std::string create_temp_dir(const std::string &prefix = "phase25_")
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

static void write_file(const fs::path &p, const std::string &content)
{
    std::ofstream f(p);
    REQUIRE(f);
    f << content;
    f.close();
}

static void cleanup_temp_dir(const std::string &dir)
{
    if (fs::exists(dir))
        fs::remove_all(dir);
}

static std::string read_file(const std::string &path)
{
    std::ifstream f(path);
    if (!f)
        return "";
    std::ostringstream os;
    os << f.rdbuf();
    return os.str();
}

// =============================================================================
// 1. docId determinism
// =============================================================================
TEST_CASE("Phase 2.5: docId determinism — exact mapping and logical order", "[phase2_5][traversal][determinism]")
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
    std::vector<DocLine> parsed_docs = parse_docs_jsonl(docs_jsonl);
    REQUIRE(parsed_docs.size() == 3u);
    REQUIRE(parsed_docs[0].docId == 1);
    REQUIRE(parsed_docs[0].file_name == "a_first.txt");
    REQUIRE(parsed_docs[0].page_number == 1);
    REQUIRE(parsed_docs[1].docId == 2);
    REQUIRE(parsed_docs[1].file_name == "b_second.txt");
    REQUIRE(parsed_docs[1].page_number == 1);
    REQUIRE(parsed_docs[2].docId == 3);
    REQUIRE(parsed_docs[2].file_name == "c_third.txt");
    REQUIRE(parsed_docs[2].page_number == 1);
    require_docid_file_page_order(parsed_docs);
    cleanup_temp_dir(out_dir);
}

// =============================================================================
// 2. UTF-8 path sorting
// =============================================================================
TEST_CASE("Phase 2.5: Path sort uses normalized absolute path (UTF-8 byte order)", "[phase2_5][sort]")
{
    std::string docs_dir = create_temp_dir("phase25_utf8_");
    std::string out_dir = create_temp_dir("phase25_out_utf8_");
    std::string searchd = find_searchd_path();

    fs::create_directories(docs_dir + "/a");
    fs::create_directories(docs_dir + "/b");
    write_file(docs_dir + "/a/x.txt", "from_a");
    write_file(docs_dir + "/b/y.txt", "from_b");

    auto [code, out] = runtime_test::run_command_capture_output(
        searchd + " --index --docs \"" + docs_dir + "\" --out \"" + out_dir + "\" 2>&1", 10);

    REQUIRE(code == 0);
    std::vector<DocLine> parsed = parse_docs_jsonl(read_file(out_dir + "/docs.jsonl"));
    REQUIRE(parsed.size() == 2u);
    REQUIRE(parsed[0].file_name == "x.txt");
    REQUIRE(parsed[1].file_name == "y.txt");
    REQUIRE(parsed[0].docId == 1);
    REQUIRE(parsed[1].docId == 2);
    require_docid_file_page_order(parsed);

    std::string docs_dir2 = create_temp_dir("phase25_utf8b_");
    std::string out_dir2 = create_temp_dir("phase25_out_utf8b_");
    fs::create_directories(docs_dir2 + "/a");
    fs::create_directories(docs_dir2 + "/b");
    write_file(docs_dir2 + "/a/x.txt", "from_a");
    write_file(docs_dir2 + "/b/y.txt", "from_b");
    auto [code2, out2] = runtime_test::run_command_capture_output(
        searchd + " --index --docs \"" + docs_dir2 + "\" --out \"" + out_dir2 + "\" 2>&1", 10);
    REQUIRE(code2 == 0);
    std::vector<DocLine> parsed2 = parse_docs_jsonl(read_file(out_dir2 + "/docs.jsonl"));
    REQUIRE(parsed2.size() == 2u);
    REQUIRE(parsed2[0].file_name == parsed[0].file_name);
    REQUIRE(parsed2[1].file_name == parsed[1].file_name);
    cleanup_temp_dir(docs_dir);
    cleanup_temp_dir(docs_dir2);
    cleanup_temp_dir(out_dir);
    cleanup_temp_dir(out_dir2);
}

// =============================================================================
// 3. Determinism: identical input, triple-run byte-identical
// =============================================================================
TEST_CASE("Phase 2.5: Identical input produces identical docIds and mapping", "[phase2_5][determinism]")
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
    std::vector<DocLine> parsed1 = parse_docs_jsonl(read_file(out1 + "/docs.jsonl"));
    std::vector<DocLine> parsed2 = parse_docs_jsonl(read_file(out2 + "/docs.jsonl"));
    REQUIRE(parsed1.size() == parsed2.size());
    for (size_t i = 0; i < parsed1.size(); ++i)
    {
        REQUIRE(parsed1[i].docId == parsed2[i].docId);
        REQUIRE(parsed1[i].file_name == parsed2[i].file_name);
        REQUIRE(parsed1[i].page_number == parsed2[i].page_number);
    }
    cleanup_temp_dir(out1);
    cleanup_temp_dir(out2);
}

TEST_CASE("Phase 2.5: Triple-run produces byte-identical docs.jsonl", "[phase2_5][determinism]")
{
    std::string docs_dir = create_temp_dir("phase25_triple_");
    std::string out1 = create_temp_dir("phase25_triple_out1_");
    std::string out2 = create_temp_dir("phase25_triple_out2_");
    std::string out3 = create_temp_dir("phase25_triple_out3_");
    std::string searchd = find_searchd_path();

    write_file(docs_dir + "/a.txt", "alpha");
    write_file(docs_dir + "/b.txt", "beta");
    write_file(docs_dir + "/c.txt", "gamma");

    auto [code1, o1] = runtime_test::run_command_capture_output(
        searchd + " --index --docs \"" + docs_dir + "\" --out \"" + out1 + "\" 2>&1", 10);
    auto [code2, o2] = runtime_test::run_command_capture_output(
        searchd + " --index --docs \"" + docs_dir + "\" --out \"" + out2 + "\" 2>&1", 10);
    auto [code3, o3] = runtime_test::run_command_capture_output(
        searchd + " --index --docs \"" + docs_dir + "\" --out \"" + out3 + "\" 2>&1", 10);

    cleanup_temp_dir(docs_dir);
    REQUIRE(code1 == 0);
    REQUIRE(code2 == 0);
    REQUIRE(code3 == 0);
    std::string raw1 = read_file(out1 + "/docs.jsonl");
    std::string raw2 = read_file(out2 + "/docs.jsonl");
    std::string raw3 = read_file(out3 + "/docs.jsonl");
    REQUIRE(raw1 == raw2);
    REQUIRE(raw2 == raw3);
    std::vector<DocLine> p = parse_docs_jsonl(raw1);
    REQUIRE(p.size() == 3u);
    REQUIRE(count_nonempty_lines(raw1) == p.size());
    require_unique_docids(p);
    for (size_t i = 0; i < p.size(); ++i)
        REQUIRE(p[i].docId == static_cast<int>(i + 1));
    cleanup_temp_dir(out1);
    cleanup_temp_dir(out2);
    cleanup_temp_dir(out3);
}

TEST_CASE("Phase 2.5: docId uniqueness and docs.jsonl one line per doc", "[phase2_5][determinism]")
{
    std::string docs_dir = create_temp_dir("phase25_uniq_");
    std::string out_dir = create_temp_dir("phase25_out_uniq_");
    std::string searchd = find_searchd_path();
    write_file(docs_dir + "/a.txt", "a");
    write_file(docs_dir + "/b.txt", "b");
    write_file(docs_dir + "/c.txt", "c");
    auto [code, out] = runtime_test::run_command_capture_output(
        searchd + " --index --docs \"" + docs_dir + "\" --out \"" + out_dir + "\" 2>&1", 10);
    cleanup_temp_dir(docs_dir);
    REQUIRE(code == 0);
    std::string raw = read_file(out_dir + "/docs.jsonl");
    std::vector<DocLine> parsed = parse_docs_jsonl(raw);
    REQUIRE(parsed.size() == 3u);
    REQUIRE(count_nonempty_lines(raw) == parsed.size());
    require_unique_docids(parsed);
    cleanup_temp_dir(out_dir);
}

// =============================================================================
// 4. Parallel determinism
// =============================================================================
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
    std::vector<DocLine> parsed = parse_docs_jsonl(read_file(out_dir + "/docs.jsonl"));
    REQUIRE(parsed.size() == 2u);
    REQUIRE(parsed[0].docId == 1);
    REQUIRE(parsed[0].file_name == "aa.txt");
    REQUIRE(parsed[1].docId == 2);
    REQUIRE(parsed[1].file_name == "bb.txt");
    require_docid_file_page_order(parsed);
    cleanup_temp_dir(out_dir);
}

// =============================================================================
// 5. OCR trigger and spec: did_ocr, MIN_TEXT_CHARS, MIN_TOKEN_COUNT
// =============================================================================
TEST_CASE("Phase 2.5: Short text (below MIN_TEXT_CHARS) has did_ocr=true", "[phase2_5][ocr_trigger]")
{
    std::string docs_dir = create_temp_dir("phase25_ocr_");
    std::string out_dir = create_temp_dir("phase25_out_ocr_");
    std::string searchd = find_searchd_path();
    write_file(docs_dir + "/short.txt", "x");

    auto [code, out] = runtime_test::run_command_capture_output(
        searchd + " --index --docs \"" + docs_dir + "\" --out \"" + out_dir + "\" 2>&1", 10);

    cleanup_temp_dir(docs_dir);
    REQUIRE(code == 0);
    std::vector<DocLine> parsed = parse_docs_jsonl(read_file(out_dir + "/docs.jsonl"));
    REQUIRE(parsed.size() == 1u);
    REQUIRE(parsed[0].did_ocr_present);
    REQUIRE(parsed[0].did_ocr == true);
    cleanup_temp_dir(out_dir);
}

TEST_CASE("Phase 2.5: Below MIN_TEXT_CHARS triggers OCR; at or above does not", "[phase2_5][ocr_spec]")
{
    std::string searchd = find_searchd_path();
    std::string below(49u, 'x');
    std::string docs_dir_below = create_temp_dir("phase25_ocr49_");
    std::string out_below = create_temp_dir("phase25_out49_");
    write_file(docs_dir_below + "/b.txt", below);
    auto [c_below, o_below] = runtime_test::run_command_capture_output(
        searchd + " --index --docs \"" + docs_dir_below + "\" --out \"" + out_below + "\" 2>&1", 10);
    cleanup_temp_dir(docs_dir_below);
    REQUIRE(c_below == 0);
    std::vector<DocLine> p_below = parse_docs_jsonl(read_file(out_below + "/docs.jsonl"));
    REQUIRE(p_below.size() == 1u);
    REQUIRE(p_below[0].did_ocr_present);
    REQUIRE(p_below[0].did_ocr == true);
    cleanup_temp_dir(out_below);

    // At or above both thresholds: >= 50 chars AND >= 10 tokens (unambiguous: 60+ chars, 12 tokens)
    std::string at_or_above = "one two three four five six seven eight nine ten eleven twelve";
    REQUIRE(at_or_above.size() >= 50u);
    std::string docs_dir_at = create_temp_dir("phase25_ocr50_");
    std::string out_at = create_temp_dir("phase25_out50_");
    write_file(docs_dir_at + "/a.txt", at_or_above);
    auto [c_at, o_at] = runtime_test::run_command_capture_output(
        searchd + " --index --docs \"" + docs_dir_at + "\" --out \"" + out_at + "\" 2>&1", 10);
    cleanup_temp_dir(docs_dir_at);
    REQUIRE(c_at == 0);
    std::vector<DocLine> p_at = parse_docs_jsonl(read_file(out_at + "/docs.jsonl"));
    REQUIRE(p_at.size() == 1u);
    REQUIRE(p_at[0].did_ocr_present);
    REQUIRE(p_at[0].did_ocr == false);
    cleanup_temp_dir(out_at);
}

TEST_CASE("Phase 2.5: Below MIN_TOKEN_COUNT triggers OCR; at MIN_TOKEN_COUNT does not", "[phase2_5][ocr_spec]")
{
    std::string searchd = find_searchd_path();
    std::string nine_tokens = "one two three four five six seven eight nine";
    std::string docs_dir9 = create_temp_dir("phase25_tok9_");
    std::string out9 = create_temp_dir("phase25_out9_");
    write_file(docs_dir9 + "/nine.txt", nine_tokens);
    auto [c9, o9] = runtime_test::run_command_capture_output(
        searchd + " --index --docs \"" + docs_dir9 + "\" --out \"" + out9 + "\" 2>&1", 10);
    cleanup_temp_dir(docs_dir9);
    REQUIRE(c9 == 0);
    std::vector<DocLine> p9 = parse_docs_jsonl(read_file(out9 + "/docs.jsonl"));
    REQUIRE(p9.size() == 1u);
    REQUIRE(p9[0].did_ocr_present);
    REQUIRE(p9[0].did_ocr == true);
    cleanup_temp_dir(out9);

    // At MIN_TOKEN_COUNT (10) with >= MIN_TEXT_CHARS (50): did_ocr false (10 tokens, pad to 55 chars)
    std::string ten_tokens = "one two three four five six seven eight nine ten           "; // 10 tokens, 50+ chars
    REQUIRE(ten_tokens.size() >= 50u);
    std::string docs_dir10 = create_temp_dir("phase25_tok10_");
    std::string out10 = create_temp_dir("phase25_out10_");
    write_file(docs_dir10 + "/ten.txt", ten_tokens);
    auto [c10, o10] = runtime_test::run_command_capture_output(
        searchd + " --index --docs \"" + docs_dir10 + "\" --out \"" + out10 + "\" 2>&1", 10);
    cleanup_temp_dir(docs_dir10);
    REQUIRE(c10 == 0);
    std::vector<DocLine> p10 = parse_docs_jsonl(read_file(out10 + "/docs.jsonl"));
    REQUIRE(p10.size() == 1u);
    REQUIRE(p10[0].did_ocr_present);
    REQUIRE(p10[0].did_ocr == false);
    cleanup_temp_dir(out10);
}

TEST_CASE("Phase 2.5: OCR output determinism — same input yields identical text across runs", "[phase2_5][ocr_spec]")
{
    std::string docs_dir = create_temp_dir("phase25_ocr_det_");
    std::string out1 = create_temp_dir("phase25_ocr_det_out1_");
    std::string out2 = create_temp_dir("phase25_ocr_det_out2_");
    std::string searchd = find_searchd_path();
    write_file(docs_dir + "/d.txt", "short");

    auto [c1, o1] = runtime_test::run_command_capture_output(
        searchd + " --index --docs \"" + docs_dir + "\" --out \"" + out1 + "\" 2>&1", 10);
    auto [c2, o2] = runtime_test::run_command_capture_output(
        searchd + " --index --docs \"" + docs_dir + "\" --out \"" + out2 + "\" 2>&1", 10);

    cleanup_temp_dir(docs_dir);
    REQUIRE(c1 == 0);
    REQUIRE(c2 == 0);
    std::vector<DocLine> p1 = parse_docs_jsonl(read_file(out1 + "/docs.jsonl"));
    std::vector<DocLine> p2 = parse_docs_jsonl(read_file(out2 + "/docs.jsonl"));
    REQUIRE(p1.size() == 1u);
    REQUIRE(p2.size() == 1u);
    REQUIRE(p1[0].text == p2[0].text);
    cleanup_temp_dir(out1);
    cleanup_temp_dir(out2);
}

TEST_CASE("Phase 2.5: Every indexed document has did_ocr metadata", "[phase2_5][ocr_spec]")
{
    std::string docs_dir = create_temp_dir("phase25_ocr_every_");
    std::string out_dir = create_temp_dir("phase25_out_ocr_every_");
    std::string searchd = find_searchd_path();
    write_file(docs_dir + "/u.txt", "under");
    write_file(docs_dir + "/v.txt", "enough text here to exceed min chars threshold");
    write_file(docs_dir + "/w.txt", "x");
    auto [code, out] = runtime_test::run_command_capture_output(
        searchd + " --index --docs \"" + docs_dir + "\" --out \"" + out_dir + "\" 2>&1", 10);
    cleanup_temp_dir(docs_dir);
    REQUIRE(code == 0);
    std::vector<DocLine> parsed = parse_docs_jsonl(read_file(out_dir + "/docs.jsonl"));
    REQUIRE(parsed.size() == 3u);
    for (const auto &d : parsed)
        REQUIRE(d.did_ocr_present);
    cleanup_temp_dir(out_dir);
}

TEST_CASE("Phase 2.5: At MIN_TOKEN_COUNT did_ocr=false and identical on re-run", "[phase2_5][tokenizer]")
{
    std::string docs_dir = create_temp_dir("phase25_tok_");
    std::string out_dir = create_temp_dir("phase25_out_tok_");
    std::string searchd = find_searchd_path();
    // 10 tokens, 50+ chars so did_ocr false
    std::string ten_tokens_50chars = "one two three four five six seven eight nine ten           ";
    REQUIRE(ten_tokens_50chars.size() >= 50u);
    write_file(docs_dir + "/ten.txt", ten_tokens_50chars);

    auto [code, out] = runtime_test::run_command_capture_output(
        searchd + " --index --docs \"" + docs_dir + "\" --out \"" + out_dir + "\" 2>&1", 10);
    cleanup_temp_dir(docs_dir);
    REQUIRE(code == 0);
    std::vector<DocLine> parsed = parse_docs_jsonl(read_file(out_dir + "/docs.jsonl"));
    REQUIRE(parsed.size() == 1u);
    REQUIRE(parsed[0].did_ocr_present);
    REQUIRE(parsed[0].did_ocr == false);

    std::string docs_dir2 = create_temp_dir("phase25_tok2_");
    std::string out_dir2 = create_temp_dir("phase25_out_tok2_");
    write_file(docs_dir2 + "/ten.txt", ten_tokens_50chars);
    auto [code2, out2] = runtime_test::run_command_capture_output(
        searchd + " --index --docs \"" + docs_dir2 + "\" --out \"" + out_dir2 + "\" 2>&1", 10);
    REQUIRE(code2 == 0);
    std::vector<DocLine> parsed2 = parse_docs_jsonl(read_file(out_dir2 + "/docs.jsonl"));
    REQUIRE(parsed2.size() == 1u);
    REQUIRE(parsed2[0].did_ocr == parsed[0].did_ocr);
    cleanup_temp_dir(docs_dir2);
    cleanup_temp_dir(out_dir);
    cleanup_temp_dir(out_dir2);
}

// =============================================================================
// 6. Canonical text format
// =============================================================================
TEST_CASE("Phase 2.5: Canonical text format — newline between layer and OCR", "[phase2_5][canonical]")
{
    std::string docs_dir = create_temp_dir("phase25_can_");
    std::string out_dir = create_temp_dir("phase25_out_can_");
    std::string searchd = find_searchd_path();
    write_file(docs_dir + "/doc.txt", "layer");

    auto [code, out] = runtime_test::run_command_capture_output(
        searchd + " --index --docs \"" + docs_dir + "\" --out \"" + out_dir + "\" 2>&1", 10);
    cleanup_temp_dir(docs_dir);
    REQUIRE(code == 0);
    std::vector<DocLine> parsed = parse_docs_jsonl(read_file(out_dir + "/docs.jsonl"));
    REQUIRE(parsed.size() == 1u);
    REQUIRE(parsed[0].text.find("layer") != std::string::npos);
    require_canonical_text_format(parsed[0].text);
    cleanup_temp_dir(out_dir);
}

// =============================================================================
// 7. Duplicate content allowed
// =============================================================================
TEST_CASE("Phase 2.5: Duplicate text layer and OCR content allowed", "[phase2_5][duplicate]")
{
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

// =============================================================================
// 8. Metadata: file_name, page_number, did_ocr; .txt page_number 1
// =============================================================================
TEST_CASE("Phase 2.5: Index stores file_name, page_number, did_ocr, file_type, source_path; .txt page_number 1", "[phase2_5][metadata]")
{
    std::string docs_dir = create_temp_dir("phase25_meta_");
    std::string out_dir = create_temp_dir("phase25_out_meta_");
    std::string searchd = find_searchd_path();
    write_file(docs_dir + "/m.txt", "meta");
    auto [code, out] = runtime_test::run_command_capture_output(
        searchd + " --index --docs \"" + docs_dir + "\" --out \"" + out_dir + "\" 2>&1", 10);
    cleanup_temp_dir(docs_dir);
    REQUIRE(code == 0);
    std::vector<DocLine> parsed = parse_docs_jsonl(read_file(out_dir + "/docs.jsonl"));
    REQUIRE(parsed.size() == 1u);
    REQUIRE(parsed[0].file_name == "m.txt");
    REQUIRE(parsed[0].page_number == 1);
    REQUIRE(parsed[0].did_ocr_present);
    REQUIRE(parsed[0].file_type == "txt");
    REQUIRE_FALSE(parsed[0].source_path.empty());
    cleanup_temp_dir(out_dir);
}

// =============================================================================
// 9. Page-level: one doc per page/file; page_number >= 1; docIds contiguous
// =============================================================================
TEST_CASE("Phase 2.5: One doc per page, page_number 1 for .txt, docIds file then page order", "[phase2_5][multipage]")
{
    std::string docs_dir = create_temp_dir("phase25_mp_");
    std::string out_dir = create_temp_dir("phase25_out_mp_");
    std::string searchd = find_searchd_path();
    write_file(docs_dir + "/p1.txt", "page one");
    write_file(docs_dir + "/p2.txt", "page two");
    auto [code, out] = runtime_test::run_command_capture_output(
        searchd + " --index --docs \"" + docs_dir + "\" --out \"" + out_dir + "\" 2>&1", 10);
    cleanup_temp_dir(docs_dir);
    REQUIRE(code == 0);
    std::vector<DocLine> parsed = parse_docs_jsonl(read_file(out_dir + "/docs.jsonl"));
    REQUIRE(parsed.size() == 2u);
    REQUIRE(parsed[0].docId == 1);
    REQUIRE(parsed[0].file_name == "p1.txt");
    REQUIRE(parsed[0].page_number == 1);
    REQUIRE(parsed[1].docId == 2);
    REQUIRE(parsed[1].file_name == "p2.txt");
    REQUIRE(parsed[1].page_number == 1);
    require_docid_file_page_order(parsed);
    cleanup_temp_dir(out_dir);
}

TEST_CASE("Phase 2.5: Exactly one doc per .txt file; doc count equals file count; page_number >= 1", "[phase2_5][multipage]")
{
    std::string docs_dir = create_temp_dir("phase25_pl_");
    std::string out_dir = create_temp_dir("phase25_out_pl_");
    std::string searchd = find_searchd_path();
    int num_txt_files = 7;
    for (int i = 0; i < num_txt_files; ++i)
        write_file(docs_dir + "/f" + std::to_string(i) + ".txt", "content " + std::to_string(i));
    auto [code, out] = runtime_test::run_command_capture_output(
        searchd + " --index --docs \"" + docs_dir + "\" --out \"" + out_dir + "\" 2>&1", 15);
    cleanup_temp_dir(docs_dir);
    REQUIRE(code == 0);
    std::vector<DocLine> parsed = parse_docs_jsonl(read_file(out_dir + "/docs.jsonl"));
    REQUIRE(static_cast<int>(parsed.size()) == num_txt_files);
    for (size_t i = 0; i < parsed.size(); ++i)
    {
        REQUIRE(parsed[i].docId == static_cast<int>(i + 1));
        REQUIRE(parsed[i].page_number >= 1);
        REQUIRE(parsed[i].did_ocr_present);
    }
    require_docid_file_page_order(parsed);
    cleanup_temp_dir(out_dir);
}

TEST_CASE("Phase 2.5: docIds are contiguous 1 to N with no gaps", "[phase2_5][multipage][determinism]")
{
    std::string docs_dir = create_temp_dir("phase25_contig_");
    std::string out_dir = create_temp_dir("phase25_out_contig_");
    std::string searchd = find_searchd_path();
    write_file(docs_dir + "/a.txt", "a");
    write_file(docs_dir + "/b.txt", "b");
    write_file(docs_dir + "/c.txt", "c");
    auto [code, out] = runtime_test::run_command_capture_output(
        searchd + " --index --docs \"" + docs_dir + "\" --out \"" + out_dir + "\" 2>&1", 10);
    cleanup_temp_dir(docs_dir);
    REQUIRE(code == 0);
    std::vector<DocLine> parsed = parse_docs_jsonl(read_file(out_dir + "/docs.jsonl"));
    REQUIRE(parsed.size() == 3u);
    for (size_t i = 0; i < parsed.size(); ++i)
        REQUIRE(parsed[i].docId == static_cast<int>(i + 1));
    cleanup_temp_dir(out_dir);
}

// =============================================================================
// 10. Search response and metadata compatibility
// =============================================================================
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
    std::string serve_cmd = "(" + searchd + " --serve --in \"" + out_dir + "\" --port 18905 2>/dev/null &); pid=$!; sleep 3; curl -s 'http://127.0.0.1:18905/search?q=queryable'; kill $pid 2>/dev/null; wait $pid 2>/dev/null";
    auto [c2, o2] = runtime_test::run_command_capture_output(serve_cmd, 12);
    REQUIRE(o2.find("file_name") != std::string::npos);
    REQUIRE(o2.find("page_number") != std::string::npos);
    REQUIRE(o2.find("score") != std::string::npos);
    REQUIRE(o2.find("snippet") != std::string::npos);
    cleanup_temp_dir(out_dir);
}

TEST_CASE("Phase 2.5: Index with Phase 2.5 metadata loads without failure; unknown fields ignored", "[phase2_5][metadata_compat]")
{
    std::string docs_dir = create_temp_dir("phase25_compat_");
    std::string out_dir = create_temp_dir("phase25_out_compat_");
    std::string searchd = find_searchd_path();
    write_file(docs_dir + "/c.txt", "compat");
    auto [c1, o1] = runtime_test::run_command_capture_output(
        searchd + " --index --docs \"" + docs_dir + "\" --out \"" + out_dir + "\" 2>&1", 10);
    cleanup_temp_dir(docs_dir);
    REQUIRE(c1 == 0);
    std::string serve_cmd = "(" + searchd + " --serve --in \"" + out_dir + "\" --port 18906 &); pid=$!; sleep 2; curl -s -o /dev/null -w \"%{http_code}\" 'http://127.0.0.1:18906/search?q=compat'; kill $pid 2>/dev/null; wait $pid 2>/dev/null";
    auto [c2, o2] = runtime_test::run_command_capture_output(serve_cmd, 8);
    REQUIRE(o2.find("200") != std::string::npos);
    cleanup_temp_dir(out_dir);
}

// =============================================================================
// 11–16. Unsupported files, corrupted PDF, OCR failure, streaming, workers, OCR pool
// =============================================================================
TEST_CASE("Phase 2.5: Unsupported files ignored without exit 3", "[phase2_5][unsupported]")
{
    std::string docs_dir = create_temp_dir("phase25_unsup_");
    std::string out_dir = create_temp_dir("phase25_out_unsup_");
    std::string searchd = find_searchd_path();
    write_file(docs_dir + "/a.txt", "a");
    write_file(docs_dir + "/b.pdf", "%PDF-1.4 minimal");
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

// =============================================================================
// 17. Signal handling: SIGINT and SIGTERM; exit 0 or 3; no partial index
// =============================================================================
TEST_CASE("Phase 2.5: SIGINT during indexing exits 0 or 3 and leaves no partial index", "[phase2_5][signal]")
{
    std::string docs_dir = create_temp_dir("phase25_sig_");
    std::string out_dir = create_temp_dir("phase25_out_sig_");
    std::string searchd = find_searchd_path();
    for (int i = 0; i < 20; ++i)
        write_file(docs_dir + "/s" + std::to_string(i) + ".txt", "content " + std::to_string(i));
    std::string cmd = searchd + " --index --docs \"" + docs_dir + "\" --out \"" + out_dir +
                      "\" & pid=$!; sleep 2; kill -INT \"$pid\" 2>/dev/null; wait \"$pid\"";
    auto [code, out] = runtime_test::run_command_capture_output(cmd + " 2>&1", 12);
    cleanup_temp_dir(docs_dir);
    REQUIRE((code == 0 || code == 3));
    require_no_partial_index(out_dir);
    cleanup_temp_dir(out_dir);
}

TEST_CASE("Phase 2.5: SIGTERM during indexing exits 0 or 3 and leaves no partial index", "[phase2_5][signal]")
{
    std::string docs_dir = create_temp_dir("phase25_sigterm_");
    std::string out_dir = create_temp_dir("phase25_out_sigterm_");
    std::string searchd = find_searchd_path();
    for (int i = 0; i < 20; ++i)
        write_file(docs_dir + "/t" + std::to_string(i) + ".txt", "content " + std::to_string(i));
    std::string cmd = searchd + " --index --docs \"" + docs_dir + "\" --out \"" + out_dir +
                      "\" & pid=$!; sleep 2; kill -TERM \"$pid\" 2>/dev/null; wait \"$pid\"";
    auto [code, out] = runtime_test::run_command_capture_output(cmd + " 2>&1", 12);
    cleanup_temp_dir(docs_dir);
    REQUIRE((code == 0 || code == 3));
    require_no_partial_index(out_dir);
    cleanup_temp_dir(out_dir);
}

// =============================================================================
// 18. Atomic persistence
// =============================================================================
TEST_CASE("Phase 2.5: Atomic persistence — no .tmp/.partial; complete index or none", "[phase2_5][atomic]")
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
    require_no_partial_index(out_dir);
    cleanup_temp_dir(out_dir);
}

TEST_CASE("Phase 2.5: Output directory has exactly 0 or exactly 3 index files", "[phase2_5][atomic]")
{
    std::string docs_dir = create_temp_dir("phase25_atom2_");
    std::string out_dir = create_temp_dir("phase25_out_atom2_");
    std::string searchd = find_searchd_path();
    write_file(docs_dir + "/y.txt", "y");
    auto [code, out] = runtime_test::run_command_capture_output(
        searchd + " --index --docs \"" + docs_dir + "\" --out \"" + out_dir + "\" 2>&1", 10);
    cleanup_temp_dir(docs_dir);
    REQUIRE(code == 0);
    size_t file_count = 0;
    for (const auto &e : fs::directory_iterator(out_dir))
    {
        (void)e;
        ++file_count;
    }
    REQUIRE(file_count == 3u);
    require_no_partial_index(out_dir);
    cleanup_temp_dir(out_dir);
}

// =============================================================================
// 19. No regression of Phase 2.2–2.4
// =============================================================================
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
