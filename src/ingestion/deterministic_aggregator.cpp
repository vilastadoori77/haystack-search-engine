// Implementation added in Phase 2.5 step-by-step.
#include "ingestion/deterministic_aggregator.hpp"
#include <stdexcept>

namespace haystack
{
    namespace phase2_5
    {

        std::vector<IngestedDocument> finalize_documents(std::vector<IngestedDocument> &&docs)
        {
            //(void)docs;
            // throw std::logic_error("Phase 2.5 not implemented");

            // Step 1 : Sort Deterministically by (source_path, page_number)
            std::sort(docs.begin(), docs.end(), [](const IngestedDocument &a, const IngestedDocument &b)
                      {
                          if (a.source_path != b.source_path)
                              return a.source_path < b.source_path; // Sort by source_path first
                          return a.page_number < b.page_number;     // Then sort by page_number
                      });

            // Step 2 : Assign docIds sequentially
            int id = 1; // Start docId from 1
            for (auto &doc : docs)
            {
                doc.docId = id++;
            }

            // step 3: return finalized documents
            return std::move(docs);
        }

    } // namespace phase2_5
} // namespace haystack
