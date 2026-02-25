// Implementation added in Phase 2.5 step-by-step.
#include "ingestion/txt_processor.hpp"
#include <stdexcept>
#include "ingestion/document_builder.hpp"

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

            // Construct IngestedDocument
            IngestedDocument doc;
            doc.docId = -1; // Assigned later by deterministic aggregator
            doc.file_name = path.filename().string();

            // Normalize absolute path deterministically
            fs::path abs_path = fs::absolute(path).lexically_normal();
            doc.source_path = abs_path.generic_string();
            doc.file_type = "txt";
            doc.page_number = 1;
            doc.text = ss.str();
            doc.did_ocr = false; // OCR policy applied later in the pipeline

            // --Emit Document --
            emitter.emit(std::move(doc));
        }

    } // namespace phase2_5
} // namespace haystack
