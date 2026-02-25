// Implementation added in Phase 2.5 step-by-step.
#include "ingestion/deterministic_aggregator.hpp"
#include <stdexcept>

namespace haystack {
namespace phase2_5 {

std::vector<IngestedDocument> finalize_documents(std::vector<IngestedDocument>&& docs)
{
    (void)docs;
    throw std::logic_error("Phase 2.5 not implemented");
}

} // namespace phase2_5
} // namespace haystack
