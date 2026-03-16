// Phase 2.5: per-page PDF text extraction, OCR decision, canonical merge.
#include "ingestion/pdf_processor.hpp"
#include "ingestion/document_builder.hpp"
#include "ingestion/ocr_policy.hpp"
#include "core/tokenizer.h"
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unistd.h>
#include <vector>

namespace fs = std::filesystem;

namespace haystack {
namespace phase2_5 {

namespace {

static std::string shell_escape(const std::string& path_str)
{
    std::string escaped;
    escaped.reserve(path_str.size() + 4);
    escaped += "'";
    for (char c : path_str)
    {
        if (c == '\'')
            escaped += "'\\''";
        else
            escaped += c;
    }
    escaped += "'";
    return escaped;
}

static std::string run_cmd_capture(const std::string& cmd)
{
    std::FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe)
        return "";
    std::string out;
    char buf[4096];
    while (std::fgets(buf, sizeof(buf), pipe) != nullptr)
        out += buf;
    pclose(pipe);
    return out;
}

static int get_pdf_page_count(const fs::path& path)
{
    std::string escaped = shell_escape(path.string());
    std::string cmd = "pdfinfo " + escaped + " 2>/dev/null";
    std::string out = run_cmd_capture(cmd);
    if (out.empty())
        return 0;
    // Parse "Pages: N"
    std::istringstream is(out);
    std::string line;
    while (std::getline(is, line))
    {
        if (line.size() > 6 && line.compare(0, 6, "Pages:") == 0)
        {
            int n = 0;
            size_t i = 6;
            while (i < line.size() && (line[i] == ' ' || line[i] == '\t'))
                ++i;
            while (i < line.size() && std::isdigit(static_cast<unsigned char>(line[i])))
            {
                n = n * 10 + (line[i] - '0');
                ++i;
            }
            return n;
        }
    }
    return 0;
}

static std::string get_page_text_layer(const fs::path& path, int page_one_based)
{
    std::string escaped = shell_escape(path.string());
    std::string cmd = "pdftotext -layout -f " + std::to_string(page_one_based)
        + " -l " + std::to_string(page_one_based) + " " + escaped + " - 2>/dev/null";
    return run_cmd_capture(cmd);
}

static std::string run_ocr_for_page(const fs::path& path, int page_one_based)
{
    std::string path_esc = shell_escape(path.string());
    fs::path temp_dir = fs::temp_directory_path() / ("pdfproc_" + std::to_string(static_cast<unsigned>(getpid())) + "_" + std::to_string(page_one_based));
    std::error_code ec;
    if (!fs::create_directories(temp_dir, ec) && !fs::exists(temp_dir))
        return "";
    std::string temp_esc = shell_escape(temp_dir.string());
    // pdftoppm -png -r 300 -f N -l N file outprefix -> outprefix-N.png
    std::string prefix = "p";
    std::string pdftoppm_cmd = "pdftoppm -png -r 300 -f " + std::to_string(page_one_based)
        + " -l " + std::to_string(page_one_based) + " " + path_esc + " " + temp_esc + "/" + prefix + " 2>/dev/null";
    int ret = std::system(pdftoppm_cmd.c_str());
    if (ret != 0)
    {
        fs::remove_all(temp_dir, ec);
        return "";
    }
    // Find the generated image (p-1.png for page 1)
    std::string img_name = prefix + "-" + std::to_string(page_one_based) + ".png";
    fs::path img_path = temp_dir / img_name;
    if (!fs::exists(img_path, ec))
    {
        fs::remove_all(temp_dir, ec);
        return "";
    }
    std::string img_esc = shell_escape(img_path.string());
    std::string ocr_cmd = "tesseract " + img_esc + " stdout --psm 11 2>/dev/null";
    std::string ocr_text = run_cmd_capture(ocr_cmd);
    fs::remove_all(temp_dir, ec);
    return ocr_text;
}

} // namespace

void process_pdf(const fs::path& path, IDocumentEmitter& emitter)
{
    if (!fs::exists(path) || !fs::is_regular_file(path))
        throw std::runtime_error("PDF file does not exist or is not a regular file: " + path.string());

    int num_pages = get_pdf_page_count(path);
    if (num_pages <= 0)
        throw std::runtime_error("Could not get page count or PDF has no pages: " + path.string());

    std::string file_name = path.filename().string();
    fs::path abs_path = fs::absolute(path).lexically_normal();
    std::string source_path = abs_path.generic_string();

    for (int page = 1; page <= num_pages; ++page)
    {
        std::string text_layer = get_page_text_layer(path, page);
        std::vector<std::string> tokens = tokenize(text_layer);
        size_t text_len = text_layer.size();
        size_t token_count = tokens.size();
        bool did_ocr = should_apply_ocr_for_page(text_len, token_count);

        std::string final_text;
        if (did_ocr)
        {
            std::string ocr_text = run_ocr_for_page(path, page);
            final_text = text_layer;
            if (!final_text.empty() && final_text.back() != '\n')
                final_text += '\n';
            final_text += ocr_text;
        }
        else
        {
            final_text = text_layer;
        }

        IngestedDocument doc;
        doc.docId = -1;
        doc.file_name = file_name;
        doc.file_type = "pdf";
        doc.source_path = source_path;
        doc.page_number = page;
        doc.text = final_text;
        doc.did_ocr = did_ocr;
        emitter.emit(std::move(doc));
    }
}

} // namespace phase2_5
} // namespace haystack
