#pragma once

#include "ingestion/ingestion_types.hpp"

namespace haystack {
namespace phase2_5 {

/**
 * Interface for emitting ingested documents (e.g. to index or buffer).
 * No implementation logic in this header.
 */
class IDocumentEmitter
{
public:
    virtual void emit(IngestedDocument&& doc) = 0;
    virtual ~IDocumentEmitter() = default;
};

} // namespace phase2_5
} // namespace haystack
