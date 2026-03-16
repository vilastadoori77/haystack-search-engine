#pragma once

#include <cstddef>
#include <string>

namespace haystack
{
    namespace phase2_5
    {

        struct DocumentMetadata
        {
            std::string file_path;
            std::string extension;
            bool has_text_layer;
        };

        /** Phase 2.5: minimum text length below which OCR is applied. */
        constexpr size_t MIN_TEXT_CHARS = 50u;
        /** Phase 2.5: minimum token count below which OCR is applied. */
        constexpr size_t MIN_TOKEN_COUNT = 10u;

        /**
         * Decides whether OCR should run for a page's extracted text (Phase 2.5).
         * OCR runs when text_length < MIN_TEXT_CHARS OR token_count < MIN_TOKEN_COUNT.
         * Token count must be from the indexing tokenizer for determinism.
         */
        inline bool should_apply_ocr_for_page(size_t text_length, size_t token_count)
        {
            return text_length < MIN_TEXT_CHARS || token_count < MIN_TOKEN_COUNT;
        }

        /**
         * Decides whether OCR should run (file-level metadata).
         */
        class OcrPolicy
        {
        public:
            static bool should_apply_ocr(const DocumentMetadata &metadata);
        };

    } // namespace phase2_5
} // namespace haystack
