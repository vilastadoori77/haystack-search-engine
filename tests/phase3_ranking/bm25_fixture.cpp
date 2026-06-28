#include "bm25_fixture.hpp"
#include <catch2/catch_test_macros.hpp>
#include <fstream>
#include <cmath>

namespace phase3_test
{

std::filesystem::path bm25_reference_fixture_path()
{
    return std::filesystem::path(__FILE__).parent_path() / "fixtures" / "bm25_reference.json";
}

static ReferenceCase parse_case(const Json::Value &node)
{
    ReferenceCase c;
    c.name = node["name"].asString();
    c.query = node["query"].asString();
    for (const auto &doc : node["documents"])
    {
        c.documents.emplace_back(doc["docId"].asInt(), doc["text"].asString());
    }
    for (const auto &hit : node["expected"])
    {
        c.expected.push_back({hit["docId"].asInt(), hit["score"].asDouble()});
    }
    return c;
}

ReferenceFixture load_bm25_reference_fixture()
{
    const auto path = bm25_reference_fixture_path();
    std::ifstream in(path);
    REQUIRE(in.good());

    Json::Value root;
    Json::CharReaderBuilder builder;
    std::string errors;
    REQUIRE(Json::parseFromStream(builder, in, &root, &errors));

    ReferenceFixture fixture;
    fixture.k1 = root["k1"].asDouble();
    fixture.b = root["b"].asDouble();
    fixture.tolerance = root["tolerance"].asDouble();
    for (const auto &node : root["cases"])
    {
        fixture.cases.push_back(parse_case(node));
    }
    fixture.tie_break = parse_case(root["tie_break"]);
    return fixture;
}

void populate_service(SearchService &ss, const std::vector<std::pair<int, std::string>> &documents)
{
    for (const auto &[docId, text] : documents)
    {
        ss.add_document(docId, text);
    }
}

void require_scored_matches(
    const std::vector<std::pair<int, double>> &actual,
    const std::vector<ExpectedHit> &expected,
    double tolerance)
{
    REQUIRE(actual.size() == expected.size());
    for (size_t i = 0; i < expected.size(); ++i)
    {
        INFO("index " << i << " docId expected=" << expected[i].docId
                      << " actual=" << actual[i].first);
        REQUIRE(actual[i].first == expected[i].docId);
        const double diff = std::abs(actual[i].second - expected[i].score);
        REQUIRE(diff < tolerance);
    }
}

} // namespace phase3_test
