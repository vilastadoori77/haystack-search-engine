// Implementation added in Phase 2.5 step-by-step.
#include "ingestion/txt_processor.hpp"
#include <stdexcept>
#include "ingestion/document_builder.hpp"
#include "ingestion/ocr_policy.hpp"
#include "core/tokenizer.h"

#include <fstream>
#include <sstream>
#include <filesystem>

namespace fs = std::filesystem;

namespace haystack
{
    namespace phase2_5
    {

        void process_txt(const std::filesystem::path &path, IDocumentEmitter &emitter)
        {
            //-- Validation
            if (!fs::exists(path))
            {
                throw std::runtime_error("File does not exist: " + path.string());
            }
            if (!fs::is_regular_file(path))
            {
                throw std::runtime_error("Not a regular file: " + path.string());
            }

            //-- Open the file deterministically (read-only, no buffering)
            std::ifstream file(path, std::ios::in | std::ios::binary);
            if (!file)
            {
                throw std::runtime_error("Failed to open file: " + path.string());
            }

            //-- Read the entire file content into a string
            std::ostringstream ss;
            ss << file.rdbuf();
            std::string text = ss.str();

            // Phase 2.5 OCR policy applies to .txt as well: text that is too short
            // or has too few tokens flags did_ocr=true. No image OCR is run for a
            // text file, but we record the policy decision and emit the canonical
            // "text_layer + newline + ocr" format (ocr portion is empty for .txt).
            const std::size_t text_len = text.size();
            const std::size_t token_count = tokenize(text).size();
            const bool did_ocr = should_apply_ocr_for_page(text_len, token_count);
            if (did_ocr && !text.empty() && text.back() != '\n')
                text += '\n';

            // Construct IngestedDocument
            IngestedDocument doc;
            doc.docId = -1; // Assigned later by deterministic aggregator
            doc.file_name = path.filename().string();

            // Normalize absolute path deterministically
            fs::path abs_path = fs::absolute(path).lexically_normal();
            doc.source_path = abs_path.generic_string();
            doc.file_type = "txt";
            doc.page_number = 1;
            doc.text = std::move(text);
            doc.did_ocr = did_ocr;

            // --Emit Document --
            emitter.emit(std::move(doc));
        }

    } // namespace phase2_5
} // namespace haystack
