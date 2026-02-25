// Implementation added in Phase 2.5 step-by-step.
#include "ingestion/pdf_processor.hpp"
#include <stdexcept>

namespace haystack {
namespace phase2_5 {

void process_pdf(const std::filesystem::path& path, IDocumentEmitter& emitter)
{
    (void)path;
    (void)emitter;
    throw std::logic_error("Phase 2.5 not implemented");
}

} // namespace phase2_5
} // namespace haystack
