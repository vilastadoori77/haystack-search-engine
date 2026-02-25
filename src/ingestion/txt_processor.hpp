#pragma once

#include "ingestion/ingestion_types.hpp"
#include <filesystem>

namespace haystack {
namespace phase2_5 {

class IDocumentEmitter;

/**
 * Process a single .txt file and emit one IngestedDocument.
 * TODO: Phase 2.5 implementation — read full file, set file_name, source_path,
 * file_type "txt", page_number 1, text, did_ocr per ocr_policy.
 */
void process_txt(const std::filesystem::path& path, IDocumentEmitter& emitter);

} // namespace phase2_5
} // namespace haystack
