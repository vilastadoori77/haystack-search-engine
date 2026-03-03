#pragma once

#include "ingestion/ingestion_types.hpp"
#include <filesystem>
#include <vector>

namespace haystack
{
    namespace phase2_5
    {

        /**
         * Runs the full Phase 2.5 ingestion pipeline over a directory root.
         *
         * - Scans root for supported files (.txt, .pdf) in deterministic order.
         * - Processes each file with the appropriate processor (txt or pdf).
         * - Finalizes documents: sorted by (source_path, page_number), docIds assigned sequentially.
         *
         * @param root Directory to scan for input files.
         * @return Finalized vector of IngestedDocument in deterministic order.
         */
        std::vector<IngestedDocument> ingest_directory(const std::filesystem::path &root);

    } // namespace phase2_5
} // namespace haystack
