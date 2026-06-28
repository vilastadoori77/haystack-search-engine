#include "bm25_fixture.hpp"
#include "core/score_order.h"
#include <catch2/catch_test_macros.hpp>
#include <cmath>

static const phase3_test::ReferenceFixture &reference_fixture()
{
    static const phase3_test::ReferenceFixture fixture = phase3_test::load_bm25_reference_fixture();
    return fixture;
}

TEST_CASE("Phase 3: BM25 reference fixture loads", "[phase3][bm25][fixture]")
{
    const auto &fixture = reference_fixture();
    REQUIRE(fixture.k1 == 1.2);
    REQUIRE(fixture.b == 0.75);
    REQUIRE(fixture.tolerance == 1e-9);
    REQUIRE_FALSE(fixture.cases.empty());
    REQUIRE_FALSE(fixture.tie_break.name.empty());
}

TEST_CASE("Phase 3: BM25 reference cases match engine scores", "[phase3][bm25][reference]")
{
    const auto &fixture = reference_fixture();
    for (const auto &ref_case : fixture.cases)
    {
        INFO("case: " << ref_case.name);
        SearchService ss;
        phase3_test::populate_service(ss, ref_case.documents);
        auto actual = ss.search_scored(ref_case.query);
        phase3_test::require_scored_matches(actual, ref_case.expected, fixture.tolerance);
    }
}

TEST_CASE("Phase 3: epsilon tie comparator prefers ascending docId on near-equal scores", "[phase3][bm25][tie_break][epsilon]")
{
    const double base = 0.3;
    const double near = base + 1e-10;
    REQUIRE(haystack::scores_tied(base, near));
    REQUIRE(haystack::compare_scored_desc(1, base, 2, near));
    REQUIRE_FALSE(haystack::compare_scored_desc(2, near, 1, base));
}

TEST_CASE("Phase 3: BM25 tie_break uses ascending docId on equal scores", "[phase3][bm25][tie_break]")
{
    const auto &tie = reference_fixture().tie_break;
    SearchService ss;
    phase3_test::populate_service(ss, tie.documents);
    auto actual = ss.search_scored(tie.query);
    phase3_test::require_scored_matches(actual, tie.expected, reference_fixture().tolerance);
    REQUIRE(actual.size() >= 2);
    REQUIRE(actual[0].first < actual[1].first);
    REQUIRE(std::abs(actual[0].second - actual[1].second) < 1e-9);
}

TEST_CASE("Phase 3: IDF — rarer term contributes more to score", "[phase3][bm25][idf]")
{
    SearchService ss;
    ss.add_document(1, "rareword");
    ss.add_document(2, "common");
    ss.add_document(3, "common");
    ss.add_document(4, "common");
    const auto rare_scored = ss.search_scored("rareword");
    const auto common_scored = ss.search_scored("common");
    REQUIRE(rare_scored.size() == 1);
    REQUIRE(common_scored.size() == 3);
    // Same tf/dl per doc; rareword has higher IDF than common in this corpus.
    REQUIRE(rare_scored.front().second > common_scored.front().second);
}

TEST_CASE("Phase 3: length normalization — shorter relevant doc outranks padded doc", "[phase3][bm25][length_norm]")
{
    const auto &fixture = reference_fixture();
    const phase3_test::ReferenceCase *ln_case = nullptr;
    for (const auto &c : fixture.cases)
    {
        if (c.name == "length_norm")
        {
            ln_case = &c;
            break;
        }
    }
    REQUIRE(ln_case != nullptr);
    SearchService ss;
    phase3_test::populate_service(ss, ln_case->documents);
    auto scored = ss.search_scored(ln_case->query);
    REQUIRE_FALSE(scored.empty());
    REQUIRE(scored.front().first == 2);
    phase3_test::require_scored_matches(scored, ln_case->expected, fixture.tolerance);
}
