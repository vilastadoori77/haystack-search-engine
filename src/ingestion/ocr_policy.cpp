// Implementation added in Phase 2.5 step-by-step.
#include "ingestion/ocr_policy.hpp"
#include <stdexcept>
#include <algorithm>
#include <set>

namespace haystack
{
    namespace phase2_5
    {
        // What are the image extensions we want to support for OCR?
        static const std::set<std::string> image_extensions = {".jpg", ".jpeg", ".png", ".bmp", ".tiff"};

        // What are the text extensions we want to support for OCR?
        static const std::set<std::string> text_extensions = {"txt", "json", "xml", "html", "csv", "md"};

        // Let us normalize the text by removing whitespace and converting to lowercase
        static std::string normalize_extension(std::string ext)
        {
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
            return ext;
        }

        bool should_apply_ocr(const DocumentMetadata &metadata)
        {

            std::string ext = normalize_extension(metadata.extension);
            // Case 1 Image Files
            if (image_extensions.count(ext) > 0)
            {
                return true; // Always apply OCR to image files
            }

            // PDF Files
            if (ext == "pdf")
            {
                // If PDF has no text layer, apply OCR
                if (!metadata.has_text_layer)
                {
                    return true;
                }
                // If PDF has a text layer, we can skip OCR
                return false;
            }

            // Case 3 Text files
            if (text_extensions.count(ext) > 0)
            {
                // For text files, we can assume they have a text layer, so we skip OCR
                return false;
            }
            // Default No OCR for unsupported file types
            return false;
        }

    } // namespace phase2_5
} // namespace haystack
