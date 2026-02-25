#pragma once

#include "ingestion/ingestion_types.hpp"
#include <vector>

namespace haystack {
namespace phase2_5 {

/**
 * Finalizes a collection of ingested documents into deterministic order and docIds.
 *
 * - Sorting must occur: documents must be ordered by (file path, page_number) so that
 *   logical order is stable regardless of completion order.
 * - docId is assigned sequentially (1, 2, 3, ...) in that sorted order.
 * - Completion order must not matter: even if documents were produced by parallel
 *   workers, the output must be as if processed sequentially in file-then-page order.
 *
 * @param docs Movable vector of documents (e.g. from parallel processing).
 * @return Sorted vector with docIds assigned in sequence.
 */
std::vector<IngestedDocument> finalize_documents(std::vector<IngestedDocument>&& docs);

} // namespace phase2_5
} // namespace haystack
