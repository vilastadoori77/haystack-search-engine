#pragma once

#include "ingestion/ingestion_types.hpp"
#include <filesystem>

namespace haystack {
namespace phase2_5 {

class IDocumentEmitter;

/**
 * Process a single .pdf file: one IngestedDocument per page.
 * TODO: Phase 2.5 implementation — per-page text extraction, optional OCR per
 * ocr_policy, canonical text format (text_layer + newline + ocr_content).
 */
void process_pdf(const std::filesystem::path& path, IDocumentEmitter& emitter);

} // namespace phase2_5
} // namespace haystack
