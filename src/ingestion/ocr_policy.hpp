#pragma once

#include <string>

namespace haystack {
namespace phase2_5 {

/**
 * Decides whether OCR should run for a page's extracted text.
 *
 * TODO: Phase 2.5 implementation must apply:
 * - MIN_TEXT_CHARS: if text length < MIN_TEXT_CHARS (e.g. 50), run OCR.
 * - MIN_TOKEN_COUNT: if token count < MIN_TOKEN_COUNT (e.g. 10), run OCR.
 *   Token count MUST use the same tokenizer as the indexing pipeline (spec 3.2.1).
 * - OCR runs when either condition is true; otherwise no OCR.
 */
bool should_run_ocr(const std::string& text);

} // namespace phase2_5
} // namespace haystack
