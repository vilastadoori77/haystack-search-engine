// Implementation added in Phase 2.5 step-by-step.
#include "ingestion/ocr_policy.hpp"
#include <stdexcept>

namespace haystack {
namespace phase2_5 {

bool should_run_ocr(const std::string& text)
{
    (void)text;
    throw std::logic_error("Phase 2.5 not implemented");
}

} // namespace phase2_5
} // namespace haystack
