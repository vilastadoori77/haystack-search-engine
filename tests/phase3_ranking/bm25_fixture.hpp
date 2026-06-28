#pragma once

#include "core/search_service.h"
#include <json/json.h>
#include <filesystem>
#include <string>
#include <vector>

namespace phase3_test
{

struct ExpectedHit
{
    int docId = 0;
    double score = 0.0;
};

struct ReferenceCase
{
    std::string name;
    std::vector<std::pair<int, std::string>> documents;
    std::string query;
    std::vector<ExpectedHit> expected;
};

struct ReferenceFixture
{
    double k1 = 1.2;
    double b = 0.75;
    double tolerance = 1e-9;
    std::vector<ReferenceCase> cases;
    ReferenceCase tie_break;
};

std::filesystem::path bm25_reference_fixture_path();

ReferenceFixture load_bm25_reference_fixture();

void populate_service(SearchService &ss, const std::vector<std::pair<int, std::string>> &documents);

void require_scored_matches(
    const std::vector<std::pair<int, double>> &actual,
    const std::vector<ExpectedHit> &expected,
    double tolerance);

} // namespace phase3_test
