#pragma once

#include <string>

namespace haystack {
namespace phase2_5 {

/**
 * Represents one indexed document produced by Phase 2.5 ingestion.
 * No logic; data only.
 */
struct IngestedDocument
{
    int docId = 0;
    std::string file_name;
    std::string file_type;
    std::string source_path;
    int page_number = 0;
    std::string text;
    bool did_ocr = false;
};

} // namespace phase2_5
} // namespace haystack
